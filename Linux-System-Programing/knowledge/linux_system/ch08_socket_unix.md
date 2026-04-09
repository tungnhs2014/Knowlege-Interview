# UNIX Domain Sockets — Local IPC, socketpair(), Fd Passing

## Problem It Solves

All IPC mechanisms before sockets — pipes, FIFOs, System V / POSIX IPC — operate only within
a single host. Internet domain sockets (`AF_INET`) solve cross-host communication, but when
both processes are on the **same machine**, routing data through the full TCP/IP stack is
unnecessary overhead: TCP state machine, segmentation, checksum calculation, IP routing
lookup, loopback device processing — all of this happens even for localhost traffic.

`AF_UNIX` (UNIX domain sockets) solve local IPC with the **same socket API** but a completely
different data path: data travels **directly through the kernel** from one socket buffer to
another, bypassing the entire network stack.

Additionally, `AF_UNIX` enables capabilities that `AF_INET` cannot provide:
- **Credential passing**: receive verified `pid/uid/gid` of the connecting peer via
  `SO_PEERCRED` — unforgeable, enforced by the kernel
- **File descriptor passing**: transfer open file descriptors across process boundaries via
  `sendmsg()` + `SCM_RIGHTS` ancillary data
- **Abstract namespace** (Linux): bind to a kernel-only name with no filesystem entry

---

## Concept Overview

UNIX domain sockets use a **filesystem pathname** as their address instead of IP:port.
The domain-specific address structure is:

```c
#include <sys/un.h>

struct sockaddr_un {
    sa_family_t sun_family;    /* Always AF_UNIX */
    char        sun_path[108]; /* Null-terminated filesystem pathname */
};
```

Both stream (`SOCK_STREAM`) and datagram (`SOCK_DGRAM`) socket types are supported.

### Key Differences from AF_INET

| Property | `AF_UNIX` | `AF_INET` (loopback) |
|----------|-----------|----------------------|
| Address | Filesystem pathname | IP address + port |
| Data path | Kernel socket buffers only | Full TCP/IP stack |
| Latency (round-trip) | ~1–5 µs | ~5–20 µs |
| TCP overhead | None | Full state machine, handshake |
| Peer credentials | `SO_PEERCRED` (unforgeable) | None (IP spoofable) |
| Fd passing | Yes (`SCM_RIGHTS`) | No |
| Scope | Same host only | Network |
| `SOCK_DGRAM` reliability | **Reliable** (blocks sender) | Unreliable (drops) |

### UNIX Domain Datagram — Reliable (Critical Distinction)

Unlike UDP, `AF_UNIX SOCK_DGRAM` datagrams pass entirely through kernel memory. If the
receiver's buffer is full, the kernel **blocks the sender** rather than dropping the message.
All messages arrive **in order** and **without duplication** — behavior closer to a pipe than
to UDP.

Consequence: with `AF_UNIX SOCK_DGRAM`, the **client must also `bind()`** to a pathname
before the server can send replies. Client pathnames typically embed the process ID to ensure
uniqueness:

```c
snprintf(claddr.sun_path, sizeof(claddr.sun_path),
         "/tmp/client.%ld", (long)getpid());
```

---

## System Context

```
Process A                              Process B
  write(fd, data, n)                     read(fd, buf, n)
       │                                      ▲
       ▼                                      │
  AF_UNIX send buffer  ──memcpy──►  AF_UNIX recv buffer
       (kernel)                          (kernel)

No TCP state machine. No IP routing. No checksum. No device driver.
```

Because UNIX sockets are still file descriptors, they integrate with the full fd ecosystem:
- Visible in `/proc/<pid>/fd/`
- Work with `select()`, `poll()`, `epoll()`
- Support `SOCK_CLOEXEC` and `SOCK_NONBLOCK` flags on `socket()`
- Reference-counted — connection closes only when all referring FDs close

**Permission model**: `bind()` creates a socket special file (`s` type in `ls -l`). Access is
controlled by standard file permissions:
- `connect()` (stream) or `sendto()` (datagram) requires **write permission** on the socket file
- Each directory in the path requires **execute (search) permission**

---

## Architecture

### Stream Socket (SOCK_STREAM)

```
server side                            client side
────────────────────────────────────────────────────
socket(AF_UNIX, SOCK_STREAM, 0)
remove(SOCK_PATH)     ← remove stale file
bind(fd, path)        ← kernel creates socket file 's'
listen(fd, backlog)
accept(fd, NULL, NULL) ◄────────────── socket(AF_UNIX, SOCK_STREAM, 0)
  │   returns connfd                   connect(fd, path)
  │                                        ↑
  │                                   no bind() needed on client
  ↓                                   (server doesn't need to reply to addr)
read(connfd)/write(connfd) ◄────────► read/write
close(connfd)
remove(SOCK_PATH)     ← cleanup on shutdown
```

### Datagram Socket (SOCK_DGRAM)

```
server side                            client side
────────────────────────────────────────────────────
socket(AF_UNIX, SOCK_DGRAM, 0)        socket(AF_UNIX, SOCK_DGRAM, 0)
remove(SV_PATH)                        bind(fd, "/tmp/client.<pid>")
bind(fd, SV_PATH)                          ← client MUST bind for replies

recvfrom(fd, buf, &claddr) ◄──────── sendto(fd, msg, SV_PATH)
sendto(fd, reply, &claddr) ─────────► recvfrom(fd, buf, NULL)
```

### socketpair() — Pre-connected Pair

```
socketpair(AF_UNIX, SOCK_STREAM, 0, sv)
                │
    ┌───────────┴───────────┐
    sv[0] ◄────────────────► sv[1]
    (bidirectional channel — no bind/listen/accept)

After fork():
    Parent: close(sv[1]), use sv[0]
    Child:  close(sv[0]), use sv[1]
```

### Abstract Socket Namespace

```
Regular UNIX socket:
  sun_path = "/tmp/myservice\0"
  → kernel creates inode in filesystem
  → survives process death (must unlink manually)

Abstract socket (Linux-specific):
  sun_path[0] = '\0', sun_path[1..] = "myservice"
  → exists only in kernel namespace
  → automatically removed when last fd is closed
  → no filesystem entry, no permission issues
  → visible via: ss -lxp  (shows as @myservice)
```

---

## Execution Flow

### Full Stream Socket Lifecycle

```
SERVER                                    CLIENT
──────────────────────────────────────────────────────────────
socket(AF_UNIX, SOCK_STREAM, 0)
  │  → kernel: struct socket allocated
remove("/tmp/myservice.sock")
  │  → removes stale file if exists
bind(listenfd, &addr, addrlen)
  │  → kernel creates socket file on filesystem
  │  → ownership: current uid/gid, mode: umask-applied
listen(listenfd, backlog)
  │  → socket enters LISTEN state
  │  → accept queue created

accept(listenfd, NULL, NULL)
  │  BLOCKS                               socket(AF_UNIX, SOCK_STREAM, 0)
  │                                       connect(sockfd, &addr, addrlen)
  │                                         │  → kernel checks write permission
  │                                         │     on socket file
  │  ◄──── kernel connects socket pair ────┤
  │  → accept() returns connfd             │  → connect() returns 0
  │
  │  getsockopt(connfd, SOL_SOCKET,
  │             SO_PEERCRED, &cred, ...)
  │  → cred.pid, cred.uid, cred.gid        ← verified by kernel, unforgeable

read(connfd, buf, n)                     write(sockfd, data, n)
  │  ◄──── data via kernel memcpy ─────────┤

write(connfd, reply, n)                  read(sockfd, buf, n)
  │  ──── reply via kernel memcpy ────────►│

close(connfd)                            close(sockfd)
remove("/tmp/myservice.sock")
```

### Fd Passing Flow (sendmsg + SCM_RIGHTS)

```
Sender process                         Receiver process
────────────────────────────────────────────────────────
fd = open("secret.txt", O_RDONLY)

/* Build ancillary message */
struct msghdr  msg  = { ... }
struct cmsghdr cmsg = {
  .cmsg_level = SOL_SOCKET,
  .cmsg_type  = SCM_RIGHTS,   ← "I am sending file descriptors"
  .cmsg_data  = &fd           ← integer fd value
}

sendmsg(sockfd, &msg, 0)
  │  → kernel: looks up struct file from sender's fd table
  │  → increments file reference count
  │  → queues struct file pointer in socket buffer

                                        recvmsg(sockfd, &msg, 0)
                                          │  → kernel: allocates new fd in
                                          │     receiver's fd table
                                          │  → points new fd at same struct file
                                          │  → returns new fd number in cmsg_data
                                          new_fd = *(int *)CMSG_DATA(cmsg)
                                          read(new_fd, buf, n)  ← works!
```

Key insight: the **integer fd value** is not meaningful across processes. What crosses the
boundary is the **kernel file description** (struct file) — like a cross-process `dup()`.

---

## Example (Code)

### Example 1 — UNIX Domain Stream Socket Server + Client

```c
/* unix_stream_server.c
 * Compile: gcc unix_stream_server.c -o unix_stream_server
 * Run:     ./unix_stream_server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/demo_stream.sock"
#define BUF_SIZE  256

int main(void) {
    /* Remove stale socket file from a previous run.
     * bind() fails with EADDRINUSE if file already exists. */
    remove(SOCK_PATH);

    int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) { perror("socket"); exit(1); }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));   /* zero entire struct — good practice */
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    /* bind() creates the socket file entry in the filesystem */
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind"); exit(1);
    }
    if (listen(listenfd, 5) == -1) { perror("listen"); exit(1); }
    printf("Listening on %s\n", SOCK_PATH);

    while (1) {
        /* For AF_UNIX, we don't need peer address — pass NULL/NULL */
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd == -1) { perror("accept"); continue; }

        /* SO_PEERCRED: kernel-verified credentials of connecting process */
        struct ucred cred;
        socklen_t credlen = sizeof(cred);
        if (getsockopt(connfd, SOL_SOCKET, SO_PEERCRED, &cred, &credlen) == 0)
            printf("Peer: pid=%d uid=%d gid=%d\n", cred.pid, cred.uid, cred.gid);

        char buf[BUF_SIZE];
        ssize_t n;
        while ((n = read(connfd, buf, sizeof(buf))) > 0) {
            buf[n] = '\0';
            printf("Received: %s", buf);
            write(connfd, buf, n);   /* echo back */
        }
        close(connfd);
    }

    remove(SOCK_PATH);   /* cleanup socket file on shutdown */
    close(listenfd);
    return 0;
}
```

```c
/* unix_stream_client.c
 * Compile: gcc unix_stream_client.c -o unix_stream_client
 * Run:     ./unix_stream_client
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/demo_stream.sock"

int main(void) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    /* Client does NOT need bind() — server communicates via connfd, not address */
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(1);
    }
    printf("Connected to %s\n", SOCK_PATH);

    const char *msg = "Hello from UNIX socket!\n";
    write(sockfd, msg, strlen(msg));

    char buf[256] = {0};
    ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
    if (n > 0) printf("Echo: %s", buf);

    close(sockfd);
    return 0;
}
```

### Example 2 — `socketpair()` Bidirectional IPC Between Parent and Child

```c
/* socketpair_demo.c — Bidirectional IPC using socketpair()
 * Compile: gcc socketpair_demo.c -o socketpair_demo
 * Run:     ./socketpair_demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

int main(void) {
    int sv[2];  /* sv[0] = parent end, sv[1] = child end */

    /* Creates two pre-connected sockets — no bind/listen/accept needed.
     * Only works with AF_UNIX. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair"); exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); exit(1); }

    if (pid == 0) {
        /* CHILD: use sv[1], close unused sv[0] */
        close(sv[0]);

        char buf[128];
        ssize_t n = read(sv[1], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("[child pid=%d] received: %s\n", getpid(), buf);

        const char *reply = "ACK from child";
        write(sv[1], reply, strlen(reply));

        close(sv[1]);
        exit(0);
    }

    /* PARENT: use sv[0], close unused sv[1] */
    close(sv[1]);

    const char *msg = "Hello from parent";
    write(sv[0], msg, strlen(msg));

    char buf[128];
    ssize_t n = read(sv[0], buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("[parent pid=%d] received: %s\n", getpid(), buf);

    close(sv[0]);
    wait(NULL);
    return 0;
}
```

### Example 3 — Abstract Socket Namespace (Linux-specific)

```c
/* abstract_socket.c — No filesystem entry, auto-cleanup on close
 * Compile: gcc abstract_socket.c -o abstract_socket
 * Usage:
 *   Terminal 1: ./abstract_socket server
 *   Terminal 2: ./abstract_socket client
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define ABSTRACT_NAME "demo_abstract"

/* Build a sockaddr_un for abstract namespace.
 * sun_path[0] = '\0' signals abstract; name starts at sun_path[1].
 * addrlen must be computed manually — not sizeof(sockaddr_un). */
static void make_abstract_addr(struct sockaddr_un *addr, socklen_t *len) {
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;
    /* sun_path[0] is already '\0' from memset — that's the trigger */
    strncpy(&addr->sun_path[1], ABSTRACT_NAME, sizeof(addr->sun_path) - 2);
    *len = sizeof(sa_family_t) + 1 + strlen(ABSTRACT_NAME);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [server|client]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_un addr;
    socklen_t addrlen;
    make_abstract_addr(&addr, &addrlen);

    if (strcmp(argv[1], "server") == 0) {
        int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listenfd == -1) { perror("socket"); exit(1); }

        /* No remove() needed — abstract names don't exist in the filesystem */
        if (bind(listenfd, (struct sockaddr *)&addr, addrlen) == -1) {
            perror("bind"); exit(1);
        }
        listen(listenfd, 5);
        printf("Abstract server bound to @%s\n", ABSTRACT_NAME);
        printf("Verify: ss -lxp | grep %s\n\n", ABSTRACT_NAME);

        int connfd = accept(listenfd, NULL, NULL);
        char buf[256] = {0};
        ssize_t n = read(connfd, buf, sizeof(buf) - 1);
        printf("Received: %s\n", buf);

        close(connfd);
        close(listenfd);
        /* Abstract name is automatically removed — no unlink() needed */

    } else {
        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1) { perror("socket"); exit(1); }

        if (connect(sockfd, (struct sockaddr *)&addr, addrlen) == -1) {
            perror("connect"); exit(1);
        }
        printf("Connected to @%s\n", ABSTRACT_NAME);
        write(sockfd, "Hello abstract socket!", 22);
        close(sockfd);
    }

    return 0;
}
```

---

## Debugging

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `EADDRINUSE` on `bind()` | Socket file already exists (stale from crash) | Call `remove(path)` before `bind()` |
| `ENOENT` on `connect()` | Socket file doesn't exist — server not running | Start server first |
| `EACCES` on `connect()` | No write permission on socket file | Check file permissions with `ls -l` |
| `ECONNREFUSED` on `connect()` | Socket file exists but no process listening | Server crashed, stale file — remove and restart |
| FD leak | `close(connfd)` missing after handling each client | Always close connfd in server loop |
| Truncated datagram | `recvfrom()` buffer smaller than datagram | Use buffer ≥ max expected message size |

### Inspection Commands

```bash
# List all UNIX domain sockets with bound paths and pids
ss -lxp

# Show all UNIX socket connections (including connected pairs)
ss -xap

# Check if a specific abstract socket is active
ss -lxp | grep myservice
# Output: ... @myservice ...  (@ prefix = abstract)

# Find which process owns a socket fd
ls -la /proc/<pid>/fd/ | grep socket
# Then match inode: cat /proc/net/unix | grep <inode>

# Inspect socket file on filesystem
ls -lF /tmp/myservice.sock
# srwxrwxrwx ... /tmp/myservice.sock=
# ^--- 's' = socket type

# Check socket file permissions (for access control)
stat /tmp/myservice.sock
```

### strace Snippet

```bash
# Trace UNIX socket calls
strace -e trace=socket,bind,listen,accept,connect,sendmsg,recvmsg \
       ./unix_stream_server

# Watch credential exchange
strace -e trace=getsockopt ./unix_stream_server 2>&1 | grep PEERCRED
```

---

## Real-world Usage

| Application | Socket type | Path / Name | Purpose |
|-------------|-------------|-------------|---------|
| **systemd** | `AF_UNIX SOCK_STREAM` | `/run/systemd/private/...` | Service control, journal |
| **DBus** | `AF_UNIX SOCK_STREAM` | `/run/dbus/system_bus_socket` | System message bus |
| **Docker** | `AF_UNIX SOCK_STREAM` | `/var/run/docker.sock` | Docker API endpoint |
| **PostgreSQL** | `AF_UNIX SOCK_STREAM` | `/var/run/postgresql/.s.PGSQL.5432` | Local DB connections (2x faster than TCP) |
| **X11** | `AF_UNIX SOCK_STREAM` | `/tmp/.X11-unix/X0` | Display server IPC |
| **nginx workers** | `AF_UNIX SOCK_STREAM` + fd passing | internal | Master passes accepted FD to worker |
| **Chrome sandbox** | `socketpair()` + fd passing | in-process | Pass file FDs to sandboxed renderer |
| **syslog** | `AF_UNIX SOCK_DGRAM` | `/dev/log` | Log message submission |

**Pattern — `SO_PEERCRED` for authentication**: services like DBus use `SO_PEERCRED` to verify
the connecting process without any application-level token. The kernel guarantees the
`pid/uid/gid` are accurate because the process cannot fake them at connection time.

**Pattern — fd passing for privilege separation**: a privileged process opens a file (or
socket), then passes the fd to an unprivileged worker. The worker can use the already-open
fd without ever having the permission to `open()` it directly. This is the foundation of
Chrome's sandboxing model and systemd socket activation.

---

## Key Takeaways

| Concept | Core point |
|---------|-----------|
| Address format | Filesystem pathname in `sockaddr_un.sun_path` (max 108 bytes) |
| Before `bind()` | Always `remove(path)` first — kernel won't overwrite existing file |
| `SOCK_DGRAM` reliability | **Reliable & ordered** within a host — blocks sender, never drops |
| DGRAM client `bind()` | Required if server needs to reply — use PID in path for uniqueness |
| `socketpair()` | Pre-connected pair, `AF_UNIX` only — bidirectional pipe equivalent |
| `SO_PEERCRED` | Kernel-verified `pid/uid/gid` of peer — use for access control |
| Fd passing | `sendmsg()` + `SCM_RIGHTS` — copies file description cross-process |
| Abstract namespace | `sun_path[0] = '\0'` — Linux only, auto-removes on socket close |
| vs `AF_INET loopback` | 3–5x lower latency, no TCP overhead, credential passing supported |
| Cleanup | `remove()` socket path on server shutdown, or use abstract namespace |
