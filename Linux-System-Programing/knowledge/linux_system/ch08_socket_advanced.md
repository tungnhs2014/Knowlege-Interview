# Advanced Socket Topics — I/O, Options, TCP Internals

## Problem It Solves

The basic socket API (`socket/bind/listen/accept/read/write/close`) works for simple cases,
but real-world applications hit a set of subtle problems:

- Stream sockets do **not** guarantee that one `write(n)` transmits exactly n bytes — partial
  I/O causes silent data corruption if not handled.
- `close()` on a server socket after `fork()` does not send FIN if the child still holds the
  fd — the connection hangs.
- Restarting a server immediately after crash fails with `EADDRINUSE` because the old TCP
  endpoint is in `TIME_WAIT`.
- Sending a large file through `read() + write()` wastes CPU on unnecessary kernel↔userspace
  copies.
- Knowing which IP/port was actually assigned (when binding to port 0) requires an explicit
  query call.

This chapter covers the **advanced mechanics** that make production-quality socket
applications reliable and efficient.

---

## Concept Overview

### Partial I/O on Stream Sockets

A single `write()` or `read()` on a stream socket may transfer **fewer bytes than requested**:

| Cause | Description |
|-------|-------------|
| Send buffer full | Kernel cannot accept more data right now |
| Signal interrupt | `EINTR` — the call was interrupted mid-way |
| Nonblocking fd | `O_NONBLOCK` causes `EAGAIN` immediately if buffer not ready |
| Async error | Remote reset while writing |

The fix is always a **loop** — keep calling until all bytes are transferred or an error occurs.

### shutdown() vs close()

| Call | FIN sent? | Reference count? | fd closed? |
|------|-----------|-----------------|------------|
| `close(fd)` | Only when refcount → 0 | Decrements | Yes |
| `shutdown(fd, SHUT_WR)` | **Always** (regardless of refcount) | Unchanged | **No** |
| `shutdown(fd, SHUT_RDWR)` | Always | Unchanged | No — still need close() |

`shutdown(SHUT_WR)` is the **half-close** pattern: signals EOF to peer while keeping the
socket readable for the peer's response. Essential for HTTP/1.0 and any request-response
protocol where the request boundary is EOF.

> `SHUT_RD` on TCP is **not portable** — behavior varies across implementations. Avoid it.

### recv() / send() Flags

| Flag | Direction | Effect |
|------|-----------|--------|
| `MSG_DONTWAIT` | both | Non-blocking for this call only (no need for `O_NONBLOCK`) |
| `MSG_PEEK` | recv | Read without consuming — data remains in socket buffer |
| `MSG_WAITALL` | recv | Block until exactly `length` bytes received (or EOF/error) |
| `MSG_MORE` | send | Hint: more data coming — buffer this segment (per-call `TCP_CORK`) |
| `MSG_NOSIGNAL` | send | Don't send `SIGPIPE` on broken pipe — return `EPIPE` instead |
| `MSG_OOB` | both | Send/receive TCP urgent (out-of-band) data — 1 byte maximum |

### sendfile() — Zero-Copy Transfer

Normal `read() + write()` path involves **4 copies**:
```
disk → kernel page cache → userspace buffer → socket buffer → NIC
```

`sendfile()` eliminates the middle two:
```
disk → kernel page cache ──────────────── → NIC
```

- `in_fd` must be a regular file (must support `mmap()`)
- `out_fd` must be a **socket** (Linux-specific restriction)
- Linux-only — not in POSIX

**TCP_CORK** complements `sendfile()` for HTTP-style transfers: holds data in TCP until
un-corked, allowing HTTP header + file body to be sent in the **minimum number of TCP
segments**.

### Socket Options: getsockopt() / setsockopt()

```c
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```

Options are scoped by `level`:

| Level | Applies to |
|-------|-----------|
| `SOL_SOCKET` | Socket API layer (domain-independent) |
| `IPPROTO_TCP` | TCP protocol behavior |
| `IPPROTO_IP` / `IPPROTO_IPV6` | IP layer behavior |

Socket options are associated with the **open file description** — shared across `dup()` and
`fork()`.

Key options:

| Option | Level | Type | Purpose |
|--------|-------|------|---------|
| `SO_REUSEADDR` | `SOL_SOCKET` | `int` | Bind to port with TIME_WAIT endpoint |
| `SO_KEEPALIVE` | `SOL_SOCKET` | `int` | Send idle keepalive probes |
| `SO_SNDBUF` | `SOL_SOCKET` | `int` | Send buffer size (kernel may double it) |
| `SO_RCVBUF` | `SOL_SOCKET` | `int` | Receive buffer size |
| `SO_LINGER` | `SOL_SOCKET` | `struct linger` | Behavior of `close()` with unsent data |
| `SO_TYPE` | `SOL_SOCKET` | `int` | Read-only: `SOCK_STREAM` or `SOCK_DGRAM` |
| `TCP_NODELAY` | `IPPROTO_TCP` | `int` | Disable Nagle algorithm — send small packets immediately |
| `TCP_CORK` | `IPPROTO_TCP` | `int` | Buffer all output until disabled or 200ms timeout |

### SO_REUSEADDR and TIME_WAIT

When a TCP server performs an **active close**, its endpoint enters `TIME_WAIT` for **2×MSL
= 60 seconds** (Linux MSL = 30s). During this time:

1. The endpoint cannot accept new connections
2. By default, `bind()` to the same port fails with `EADDRINUSE`

`SO_REUSEADDR` relaxes the kernel constraint from "no socket on this port" to "no
**established or listening** socket on this exact 4-tuple `{src-ip, src-port, dst-ip,
dst-port}`". The TIME_WAIT endpoint still completes its 2MSL — reliability is preserved.

**Every TCP server should set `SO_REUSEADDR` before `bind()`.**

Why TIME_WAIT exists (two reasons):
1. **Reliable termination** — if the final ACK is lost, the peer retransmits its FIN. The
   endpoint must still be alive to respond.
2. **Prevent ghost segments** — old duplicate segments from a previous incarnation of the
   connection must expire before a new connection on the same 4-tuple is allowed.

### getsockname() / getpeername()

```c
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

- `getsockname()` — local address bound to `sockfd`
- `getpeername()` — peer address connected to `sockfd`

Common uses:
- After `bind(port=0)` → kernel assigns ephemeral port → `getsockname()` reveals which one
- In child process after `accept()` without saving peer address at time of accept
- In inetd-launched programs that inherit a connected socket without knowing peer address

### Inheritance Across accept()

The new socket returned by `accept()` on Linux:

| Attribute | Inherited? |
|-----------|-----------|
| Socket options (`setsockopt`) | **Yes** — copied |
| `O_NONBLOCK`, `O_ASYNC` | **No** |
| `FD_CLOEXEC` | **No** |
| `F_SETOWN`, `F_SETSIG` | **No** |

Consequence: if listening socket is nonblocking (for epoll), accepted sockets are **blocking**
by default. Must explicitly call `fcntl(cfd, F_SETFL, O_NONBLOCK)` after each `accept()`.

### sendmsg() / recvmsg() and Ancillary Data

The most general socket I/O calls — superset of all others:

```c
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

struct msghdr {
    void         *msg_name;       // peer address (DGRAM)
    socklen_t     msg_namelen;
    struct iovec *msg_iov;        // scatter-gather buffer array
    size_t        msg_iovlen;     // number of iovec elements
    void         *msg_control;   // ancillary data buffer
    size_t        msg_controllen;
    int           msg_flags;      // output only (recvmsg)
};
```

**Ancillary data** types:

| Type | Domain | Purpose |
|------|--------|---------|
| `SCM_RIGHTS` | `AF_UNIX` | Pass file descriptors to another process |
| `SCM_CREDENTIALS` | `AF_UNIX` | Pass pid/uid/gid for sender authentication |
| `IP_PKTINFO` | `AF_INET` | Identify which interface/address received a UDP packet |

---

## System Context

```
Application
    │
    ├── readn() / writen()          [retry loop over read/write]
    ├── shutdown(SHUT_WR)           [half-close: sends FIN]
    ├── recv(MSG_PEEK/MSG_WAITALL)  [controlled receive]
    ├── sendfile(out_sock, in_file) [zero-copy: page cache → NIC]
    ├── setsockopt(SO_REUSEADDR)    [must be before bind()]
    ├── getsockname / getpeername   [query assigned addresses]
    └── sendmsg / recvmsg           [scatter-gather + ancillary]
         │
         └── struct msghdr ──► iovec[] + cmsghdr (SCM_RIGHTS, etc.)
```

**TCP State Machine (simplified)**:

```
[CLOSED]
    │ socket() + listen()
    ▼
[LISTEN] ◄── server waiting
    │ client connect() → 3-way handshake (SYN / SYN+ACK / ACK)
    ▼
[ESTABLISHED] ◄── data transfer
    │ close() (active side) → sends FIN
    ▼
[FIN_WAIT1] → [FIN_WAIT2] → [TIME_WAIT] (2×MSL = 60s) → [CLOSED]
                                     ↑
                              kernel holds endpoint here
                              SO_REUSEADDR lets bind() proceed despite this
```

---

## Architecture

```
 Server restart scenario:
 ┌──────────────────────────────────────────────────────────┐
 │  Old server crashed  →  TCP in TIME_WAIT (port 8080)     │
 │                                                           │
 │  Without SO_REUSEADDR:                                    │
 │    new bind(8080)  →  EADDRINUSE ✗                        │
 │                                                           │
 │  With SO_REUSEADDR:                                       │
 │    new bind(8080)  →  OK ✓  (TIME_WAIT still runs)       │
 └──────────────────────────────────────────────────────────┘

 sendfile() zero-copy path:
 ┌──────────────────────────────────────────────────────────┐
 │  read()+write()  :  file → page cache → user buf → sock  │
 │                                     ↑ 2 extra copies     │
 │                                                           │
 │  sendfile()      :  file → page cache ──────────► sock   │
 │                        zero user-space involvement        │
 └──────────────────────────────────────────────────────────┘

 shutdown(SHUT_WR) half-close:
 ┌─────────────┐          ┌─────────────┐
 │   Client    │          │   Server    │
 │  write(req) │─────────►│             │
 │  shutdown   │──FIN────►│ read→EOF    │
 │  (SHUT_WR)  │          │ write(resp) │
 │  read(resp) │◄─────────│             │
 │  close(fd)  │          │  close(fd)  │
 └─────────────┘          └─────────────┘
```

---

## Execution Flow

### readn() Loop

```
readn(fd, buf, 4096):
  remain = 4096
  loop:
    n = read(fd, p, remain)   → might return 1024
    remain -= 1024             → 3072 left
    n = read(fd, p, remain)   → might return 3072
    remain = 0                 → done
  return 4096
```

### setsockopt(SO_REUSEADDR) Must Precede bind()

```
socket()
  │
setsockopt(SO_REUSEADDR=1)   ← MUST be here, before bind()
  │
bind(port)
  │
listen()
  │
accept() loop ...
```

### TCP_CORK + sendfile() Flow

```
setsockopt(TCP_CORK=1)      ← kernel starts buffering
  │
write(header)               ← buffered, not sent yet
  │
sendfile(file)              ← appended to buffer
  │
setsockopt(TCP_CORK=0)      ← kernel flushes: header+file in 1-2 segments
```

---

## Example

### Example 1: readn()/writen() + shutdown(SHUT_WR) Half-Close

```c
/* HTTP-style: client sends full request with half-close,
 * server reads until EOF, then sends response */

// ===== server.c =====
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT     9000
#define BUF_SIZE 65536

/* Read exactly count bytes, retrying on partial reads and EINTR */
static ssize_t readn(int fd, void *buf, size_t count) {
    size_t remain = count;
    char *p = (char *)buf;
    while (remain > 0) {
        ssize_t n = read(fd, p, remain);
        if (n == 0)  return (ssize_t)(count - remain);  /* EOF */
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        remain -= (size_t)n;
        p      += n;
    }
    return (ssize_t)count;
}

/* Write exactly count bytes, retrying on partial writes and EINTR */
static ssize_t writen(int fd, const void *buf, size_t count) {
    size_t remain = count;
    const char *p = (const char *)buf;
    while (remain > 0) {
        ssize_t n = write(fd, p, remain);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        remain -= (size_t)n;
        p      += n;
    }
    return (ssize_t)count;
}

int main(void) {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Must be set BEFORE bind() to avoid EADDRINUSE on restart */
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(lfd, 5);
    printf("[server] Listening on port %d\n", PORT);

    int cfd = accept(lfd, NULL, NULL);
    printf("[server] Client connected\n");

    /* Read entire request — client will shutdown(SHUT_WR), we get EOF */
    char req[BUF_SIZE];
    ssize_t total = 0, n;
    while ((n = read(cfd, req + total, sizeof(req) - (size_t)total - 1)) > 0)
        total += n;
    req[total] = '\0';
    printf("[server] Request (%zd bytes):\n%s\n", total, req);

    /* Send response */
    const char *resp = "HTTP/1.0 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    writen(cfd, resp, strlen(resp));
    printf("[server] Response sent\n");

    close(cfd);
    close(lfd);
    return 0;
}

// ===== client.c =====
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     9000
#define BUF_SIZE 65536

int main(void) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sfd, (struct sockaddr *)&addr, sizeof(addr));

    /* Send request */
    const char *req = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
    write(sfd, req, strlen(req));
    printf("[client] Request sent\n");

    /* Half-close write end — server's read() returns EOF
     * but socket remains readable for response              */
    if (shutdown(sfd, SHUT_WR) == -1) {
        perror("shutdown"); return 1;
    }
    printf("[client] Write-half closed (FIN sent), waiting for response\n");

    /* Read response until server closes for writing */
    char resp[BUF_SIZE];
    ssize_t total = 0, n;
    while ((n = read(sfd, resp + total, sizeof(resp) - (size_t)total - 1)) > 0)
        total += n;
    resp[total] = '\0';
    printf("[client] Response (%zd bytes):\n%s\n", total, resp);

    close(sfd);   /* must still close() to release fd */
    return 0;
}
```

**Key points:**
- `writen()` loop guarantees full write even if kernel only accepts partial data
- `shutdown(SHUT_WR)` sends FIN immediately regardless of reference count
- After shutdown, socket fd is still valid for reading
- `SO_REUSEADDR` is set before `bind()` — server can restart without waiting 60s

---

### Example 2: sendfile() + TCP_CORK — Zero-Copy File Server

```c
/* Serves a static file over HTTP with header + body in minimum TCP segments */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define PORT 9001

static void serve_file(int cfd, const char *path) {
    int ffd = open(path, O_RDONLY);
    if (ffd == -1) {
        const char *e = "HTTP/1.0 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        write(cfd, e, strlen(e));
        return;
    }

    struct stat st;
    fstat(ffd, &st);
    off_t size = st.st_size;

    /* TCP_CORK: buffer all output until uncorked.
     * Header + file body will be sent in as few segments as possible. */
    int cork = 1;
    setsockopt(cfd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

    /* Write HTTP header — buffered by TCP_CORK, not yet sent */
    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\nContent-Length: %lld\r\nContent-Type: text/plain\r\n\r\n",
        (long long)size);
    write(cfd, header, hlen);

    /* Zero-copy file transfer: kernel maps file pages directly to socket buffer.
     * No data passes through userspace — two fewer copies vs read()+write().  */
    off_t offset = 0;
    while (offset < size) {
        ssize_t sent = sendfile(cfd, ffd, &offset, (size_t)(size - offset));
        if (sent == -1) {
            if (errno == EINTR) continue;
            perror("sendfile");
            break;
        }
    }

    /* Uncork: kernel flushes all buffered data */
    cork = 0;
    setsockopt(cfd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

    printf("[server] Served %s (%lld bytes) via zero-copy sendfile\n",
           path, (long long)size);
    close(ffd);
}

int main(int argc, char *argv[]) {
    const char *file = (argc > 1) ? argv[1] : "/etc/hostname";

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(lfd, 5);
    printf("[server] File server on port %d, serving: %s\n", PORT, file);

    /* Single connection demo */
    int cfd = accept(lfd, NULL, NULL);
    printf("[server] Client connected\n");

    /* Read and discard the HTTP request */
    char buf[4096];
    read(cfd, buf, sizeof(buf));

    serve_file(cfd, file);
    close(cfd);
    close(lfd);
    return 0;
}
```

**To test:**
```bash
gcc -o file_server server_sendfile.c
./file_server /etc/passwd &
curl http://127.0.0.1:9001/
```

---

### Example 3: setsockopt() / getsockopt() + getsockname()

```c
/* Demonstrates setting key socket options and querying assigned addresses */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static void show_int_opt(int sfd, int level, int optname, const char *name) {
    int val;
    socklen_t len = sizeof(val);
    getsockopt(sfd, level, optname, &val, &len);
    printf("  %-20s = %d\n", name, val);
}

static void set_int_opt(int sfd, int level, int optname, const char *name, int val) {
    if (setsockopt(sfd, level, optname, &val, sizeof(val)) == -1)
        perror(name);
    else
        printf("  %-20s set to %d\n", name, val);
}

int main(void) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) { perror("socket"); return 1; }

    printf("=== Before configuration ===\n");
    show_int_opt(sfd, SOL_SOCKET,   SO_REUSEADDR, "SO_REUSEADDR");
    show_int_opt(sfd, SOL_SOCKET,   SO_KEEPALIVE, "SO_KEEPALIVE");
    show_int_opt(sfd, IPPROTO_TCP,  TCP_NODELAY,  "TCP_NODELAY");

    printf("\n=== Applying options ===\n");
    /* Allow binding to port even if TIME_WAIT endpoint exists */
    set_int_opt(sfd, SOL_SOCKET,  SO_REUSEADDR, "SO_REUSEADDR", 1);

    /* Send keepalive probes after ~2 hours idle (detects dead peers) */
    set_int_opt(sfd, SOL_SOCKET,  SO_KEEPALIVE, "SO_KEEPALIVE", 1);

    /* Disable Nagle algorithm: send small packets immediately.
     * Essential for interactive protocols (SSH, game servers, RPC). */
    set_int_opt(sfd, IPPROTO_TCP, TCP_NODELAY,  "TCP_NODELAY",  1);

    /* Increase socket buffers — kernel may double the requested value */
    set_int_opt(sfd, SOL_SOCKET, SO_SNDBUF, "SO_SNDBUF", 131072);
    set_int_opt(sfd, SOL_SOCKET, SO_RCVBUF, "SO_RCVBUF", 131072);

    printf("\n=== After configuration ===\n");
    show_int_opt(sfd, SOL_SOCKET,  SO_REUSEADDR, "SO_REUSEADDR");
    show_int_opt(sfd, SOL_SOCKET,  SO_KEEPALIVE, "SO_KEEPALIVE");
    show_int_opt(sfd, IPPROTO_TCP, TCP_NODELAY,  "TCP_NODELAY");
    show_int_opt(sfd, SOL_SOCKET,  SO_SNDBUF,    "SO_SNDBUF (actual)");
    show_int_opt(sfd, SOL_SOCKET,  SO_RCVBUF,    "SO_RCVBUF (actual)");

    /* Read-only option — reveals socket type */
    int type; socklen_t tlen = sizeof(type);
    getsockopt(sfd, SOL_SOCKET, SO_TYPE, &type, &tlen);
    printf("  %-20s = %d (%s)\n", "SO_TYPE", type,
           type == SOCK_STREAM ? "SOCK_STREAM" : "SOCK_DGRAM");

    /* Bind to ephemeral port (port=0) — kernel assigns a free port */
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(0),       /* 0 = let kernel choose */
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind"); close(sfd); return 1;
    }

    /* getsockname() reveals the ephemeral port kernel assigned */
    struct sockaddr_in bound;
    socklen_t blen = sizeof(bound);
    getsockname(sfd, (struct sockaddr *)&bound, &blen);
    printf("\ngetsockname() after bind(port=0): assigned port %u\n",
           ntohs(bound.sin_port));

    /* Accept one connection so we can demonstrate getpeername() */
    listen(sfd, 1);
    printf("Listening on port %u — connect to see getpeername() demo\n",
           ntohs(bound.sin_port));
    printf("(Ctrl+C to skip)\n");

    int cfd = accept(sfd, NULL, NULL);
    if (cfd != -1) {
        /* Demonstrate: accepted socket does NOT inherit O_NONBLOCK */
        int flags = fcntl(cfd, F_GETFL);
        printf("\naccepted socket O_NONBLOCK: %s (not inherited from listener)\n",
               (flags & O_NONBLOCK) ? "set" : "not set");

        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        getpeername(cfd, (struct sockaddr *)&peer, &plen);
        printf("getpeername(): peer is %s:%u\n",
               inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
        close(cfd);
    }

    close(sfd);
    return 0;
}
```

---

## Debugging

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `EADDRINUSE` on bind() | Port has TIME_WAIT or ESTABLISHED TCP | Set `SO_REUSEADDR` before `bind()` |
| Partial data received | Stream socket — `read()` returns less than expected | Use `readn()` retry loop |
| `SIGPIPE` crash | Writing to socket after peer closed | Use `MSG_NOSIGNAL` or `signal(SIGPIPE, SIG_IGN)` |
| `EPIPE` on write() | Peer closed after we ignored `SIGPIPE` | Check return value; reconnect |
| sendfile() `EINVAL` | `out_fd` is not a socket, or `in_fd` not mmapable | Verify fd types |
| Accepted socket blocks | Forgot to set `O_NONBLOCK` after `accept()` | `fcntl(cfd, F_SETFL, O_NONBLOCK)` |
| Server can't restart | `SO_REUSEADDR` set **after** bind | Move `setsockopt` before `bind()` |

### Diagnostic Commands

```bash
# Show all TCP connections and their states (including TIME_WAIT)
netstat -ant | grep 8080

# Modern equivalent (ss is faster)
ss -ant | grep 8080

# Watch TIME_WAIT drain in real time
watch -n1 'ss -s'

# Monitor TCP traffic between client and server
tcpdump -i lo -n 'port 9000'

# Verify socket options on a running process (Linux /proc)
cat /proc/net/tcp         # all TCP sockets (hex addresses)
cat /proc/net/sockstat    # socket statistics summary
```

### Verifying Zero-Copy

```bash
# perf stat shows fewer kernel cache misses with sendfile vs read/write
perf stat -e cache-misses ./file_server_sendfile
perf stat -e cache-misses ./file_server_readwrite

# strace shows sendfile() syscall instead of read()+write() pair
strace -e trace=read,write,sendfile ./file_server_sendfile
```

---

## Real-world Usage

### Where These Techniques Appear

| Technique | Where used |
|-----------|-----------|
| `readn()`/`writen()` | Every production socket library (libuv, Boost.Asio, grpc-core) |
| `shutdown(SHUT_WR)` | HTTP/1.0 clients, FTP data transfer, Redis protocol |
| `SO_REUSEADDR` | All TCP servers — nginx, Apache, PostgreSQL, Redis, sshd |
| `sendfile()` | nginx static file serving, Apache `mod_sendfile`, kernel TLS |
| `TCP_CORK` | nginx (combines with `sendfile()`), lighttpd |
| `TCP_NODELAY` | Redis, PostgreSQL, distributed databases, SSH |
| `SO_KEEPALIVE` | PostgreSQL client connections, long-lived RPC connections |
| `MSG_NOSIGNAL` | Any server that writes to potentially-closed connections |
| `getsockname()` | Test frameworks (random port assignment), STUN protocol |
| `sendmsg(SCM_RIGHTS)` | Nginx worker processes receiving fd from master |

### Production Pattern: Robust TCP Server Init

```c
int create_tcp_server(uint16_t port) {
    int lfd = socket(AF_INET6, SOCK_STREAM, 0);

    /* Key options — must be set before bind() */
    int val = 1;
    setsockopt(lfd, SOL_SOCKET,  SO_REUSEADDR, &val, sizeof(val));
    setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY,  &val, sizeof(val));
    setsockopt(lfd, SOL_SOCKET,  SO_KEEPALIVE, &val, sizeof(val));

    /* IPV6_V6ONLY=0 means accept both IPv4 and IPv6 on one socket */
    val = 0;
    setsockopt(lfd, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof(val));

    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_port   = htons(port),
        .sin6_addr   = in6addr_any
    };
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(lfd, SOMAXCONN);
    return lfd;
}
```

---

## Key Takeaways

1. **Stream sockets require retry loops** — `read()`/`write()` may transfer fewer bytes than
   requested; `readn()`/`writen()` are mandatory for correctness.

2. **`shutdown(SHUT_WR)` ≠ `close()`** — `shutdown` sends FIN immediately regardless of
   reference count, enables half-close; `close()` only sends FIN when refcount hits 0.

3. **`SO_REUSEADDR` is mandatory for every TCP server** — set it before `bind()` to avoid
   `EADDRINUSE` on restart; TIME_WAIT still runs and provides reliability.

4. **TIME_WAIT has two jobs**: ensure reliable final ACK delivery, and prevent old duplicate
   segments from being accepted by new connections on the same 4-tuple.

5. **`sendfile()` eliminates two copies** — data moves from kernel page cache directly to
   socket buffer without passing through userspace; combine with `TCP_CORK` for HTTP
   header + body in minimum segments.

6. **`accept()` does not inherit `O_NONBLOCK`** — must explicitly `fcntl(cfd, F_SETFL,
   O_NONBLOCK)` for event-driven servers after each `accept()`.

7. **`SO_REUSEPORT`** (Linux 3.9+) goes further than `SO_REUSEADDR` — multiple processes can
   bind the same port, with kernel-level load balancing across them.

8. **`sendmsg()`/`recvmsg()`** are necessary (not optional) for pass-fd patterns and
   scatter-gather I/O; `send()`/`recv()` cannot transmit ancillary data.
