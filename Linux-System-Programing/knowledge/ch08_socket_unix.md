# Chapter 8 - UNIX Domain Sockets

> Topics: 8.2 UNIX Domain Sockets - local IPC, pathname sockets, `socketpair()`, fd passing, credentials.
> Main sources: TLPI Ch57, Ch61.13; DevLinux module 06.
> Production context: local service control sockets, system daemons, container runtimes, desktop/DBus-like IPC, supervisor-worker designs, and high-performance same-host communication.

---

## Coverage Notes

This file covers Coverage Matrix row 8.2 and the Chapter 8 UNIX-domain-socket Must Cover item.

- Covered here: local IPC mental model, `AF_UNIX`, pathname sockets, Linux abstract namespace, `sockaddr_un`, pathname lifecycle and cleanup, stream/datagram behavior, permissions, `socketpair()`, peer credentials, `sendmsg()`/`recvmsg()` ancillary data for fd passing, production bugs, debugging commands, Embedded/local-daemon constraints, checklist, and interview readiness.
- Cross-file coverage: generic socket lifecycle is in `ch08_socket_overview.md`; TCP/IP transport and Internet address structures are in `ch08_socket_tcp.md`; fd-passing server designs are connected to `ch08_socket_server.md`; the general `sendmsg()`/`recvmsg()` mechanism is expanded in `ch08_socket_advanced.md`.
- No mapped UNIX-domain concept is intentionally out of scope.

## Problem It Solves

Not every socket is for the network. Many production systems need processes on the same host to communicate quickly and safely: a CLI talks to a daemon, a supervisor passes work to workers, or one process transfers an already-open fd to another process.

UNIX domain sockets solve local IPC with the socket API:

```text
process A
  socket fd
      |
      v
kernel local socket implementation
      |
      v
process B
  socket fd
```

No IP routing, no TCP handshake over a NIC, and no port namespace. The address is usually a filesystem pathname, and access can be controlled with filesystem permissions and peer credentials.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | `AF_UNIX`, `sockaddr_un`, pathname lifecycle, stream vs datagram, `unlink()` cleanup | Build and debug local client/server IPC |
| Work useful | secured socket directories, stale socket files, `socketpair()`, `SO_PEERCRED`, fd inheritance | Design daemon control channels safely |
| Recognize | abstract namespace, `SCM_RIGHTS`, `SCM_CREDENTIALS`, `SOCK_SEQPACKET` | Understand advanced Linux daemon patterns |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| UNIX domain socket | Same-host socket using `AF_UNIX` / `AF_LOCAL` | Does not communicate across machines |
| `sockaddr_un` | UNIX-domain address structure | Contains `sun_family` and `sun_path` |
| Socket pathname | Filesystem name bound to a UNIX socket | `/run/myapp/control.sock` |
| Socket file | Filesystem entry created by `bind()` | `ls -l` shows type `s` |
| Stale socket file | Path remains after process exits/crashes | Remove with `unlink()` before `bind()` |
| Stream UNIX socket | Reliable byte stream on same host | Like TCP semantics without IP |
| Datagram UNIX socket | Message-oriented local socket | Reliable and ordered per TLPI because transfer stays in kernel |
| `socketpair()` | Creates two already-connected UNIX sockets | Common after `fork()` |
| Abstract namespace | Linux-specific UNIX socket name not in filesystem | First byte of `sun_path` is `'\0'` |
| Peer credentials | Kernel-provided pid/uid/gid of peer | Linux: `SO_PEERCRED` |
| Ancillary data | Extra metadata sent with `sendmsg()` | Used for fd passing via `SCM_RIGHTS` |
| `SCM_RIGHTS` | Control message type for passing fds | Receiver gets a new fd referring to same open file description |
| `SCM_CREDENTIALS` | Linux credential-passing ancillary data | Advanced authentication path |

## Concept Overview

UNIX domain sockets are sockets optimized for local IPC. They keep the API shape of network sockets but replace IP+port addressing with local names.

| Feature | UNIX domain | Internet domain |
|---------|-------------|-----------------|
| Domain | `AF_UNIX` | `AF_INET`, `AF_INET6` |
| Address | Pathname or Linux abstract name | IP address + port |
| Scope | Same host | Network or loopback |
| Access control | Directory/socket permissions, credentials | Firewall, bind address, protocol auth |
| Special ability | Pass file descriptors | Not available over TCP/UDP |

### Pathname Socket Mental Model

```text
bind("/run/myapp/control.sock")
      |
      v
kernel creates a socket entry in filesystem namespace
      |
      v
clients connect/send to that pathname
```

The socket file is a name, not storage. You cannot `open()` it as a normal file to exchange data. You must use the socket API.

### UNIX Datagram vs UDP

Do not transfer UDP assumptions blindly:

| Property | UNIX datagram | UDP |
|----------|---------------|-----|
| Message boundary | Preserved | Preserved |
| Delivery/order | Reliable and ordered on same host per TLPI | Not guaranteed |
| Queue full behavior | Sender can block or fail depending mode/options | Incoming datagram may be dropped |
| Address | Pathname/abstract | IP+port |

## System Context

UNIX domain sockets connect socket programming with filesystem and process concepts:

| Subsystem | Interaction |
|-----------|-------------|
| Filesystem | `bind()` creates a pathname entry; directory permissions matter |
| Credentials | Kernel knows local peer uid/gid/pid and can expose them |
| File descriptors | Fds can be inherited across `fork()` or passed with `SCM_RIGHTS` |
| Process lifecycle | Crashes can leave stale pathname socket entries |
| Security | `/tmp` pathnames are risky; prefer `/run/<service>` or another owned directory |
| Event loops | UNIX sockets work with `select()`, `poll()`, `epoll()` like other fds |

If a local daemon fails to clean up its socket path, the next `bind()` can fail with `EADDRINUSE`. If the socket is placed in a world-writable directory with predictable names, another user can cause denial-of-service or confuse clients.

## Architecture

### Address Structure

```text
struct sockaddr_un {
    sa_family_t sun_family;  /* AF_UNIX */
    char        sun_path[];  /* pathname, size is implementation-dependent */
};
```

TLPI notes that `sun_path` size is not standardized. Linux commonly exposes 108 bytes, but portable code should check length and avoid overflowing the field.

### Pathname Socket Lifecycle

```text
socket(AF_UNIX, SOCK_STREAM, 0)
    |
    v
unlink(old_path)          optional but common before bind
    |
    v
bind(path)
    |
    v
listen()/accept() or recvfrom()
    |
    v
close(fd)
    |
    v
unlink(path)              remove name from filesystem
```

### `socketpair()` Architecture

```text
socketpair(AF_UNIX, SOCK_STREAM, 0, sv)
              |
              v
sv[0] <------------------> sv[1]
```

`socketpair()` creates an unnamed connected pair. This avoids pathname cleanup and access-control races because no other process can discover the sockets by name.

### Credential and FD Passing

```text
sender open("/path/file") -> fd 5
      |
      v
sendmsg(unix_socket, SCM_RIGHTS(fd 5))
      |
      v
receiver recvmsg() -> new fd 8

fd 5 and fd 8 refer to the same open file description
```

This is a foundation for prefork servers, sandbox brokers, and supervisors that accept a resource once and hand it to a less-privileged process.

## Execution Flow

### UNIX Stream Server

```text
socket(AF_UNIX, SOCK_STREAM, 0)
    |
    v
unlink(path)
    |
    v
bind(path)
    |
    v
listen(backlog)
    |
    v
accept() -> connected fd
    |
    v
read/write protocol
```

### UNIX Stream Client

```text
socket(AF_UNIX, SOCK_STREAM, 0)
    |
    v
connect(path)
    |
    v
read/write protocol
    |
    v
close()
```

### UNIX Datagram Request/Reply

```text
server bind("/run/svc.sock")
client bind("/tmp/client.<pid>.sock")  if it expects reply
client sendto(server path)
server recvfrom() -> client path
server sendto(client path)
```

### Parent/Child `socketpair()`

```text
socketpair()
    |
    v
fork()
    |
    +-- parent closes sv[1], uses sv[0]
    |
    +-- child closes sv[0], uses sv[1]
```

### Stale Path Failure

```text
previous daemon crashed
    |
    v
/run/myapp/control.sock remains
    |
    v
new bind(path) -> EADDRINUSE
    |
    v
verify path is a socket and unlink during controlled startup
```

## 8.2 API / Topic Sections

### Pathname Binding

Use pathname sockets when unrelated processes need a stable rendezvous point. Put the pathname in a directory with controlled ownership and permissions.

Avoid predictable names in `/tmp` for production services. If you must use a temporary directory, create a private directory with safe permissions first.

### Permissions

For pathname UNIX sockets, connecting to a stream socket or sending to a datagram socket requires write permission on the socket file on Linux, and search permission on every directory in the path. TLPI also notes that some UNIX implementations ignore socket file permissions, so the hosting directory is the more portable security boundary.

### `socketpair()`

Use `socketpair()` when the communicating processes are related by `fork()`. It is simpler and safer than creating a pathname socket just for parent-child communication.

### Abstract Namespace

Linux supports abstract UNIX socket names. They do not create filesystem entries and disappear when the last reference closes. They are useful in chroot/container-like cases, but are Linux-specific and not portable POSIX behavior.

### FD Passing and Credentials

FD passing uses `sendmsg()`/`recvmsg()` with ancillary data. Peer credentials can be retrieved on Linux with `SO_PEERCRED` for connected UNIX sockets. These features are work-useful in daemon design but should not be the first thing a newbie memorizes.

## Work Checklist

| Pattern | Why it matters |
|---------|----------------|
| Store sockets under `/run/<service>/` | Runtime location, easier ownership and cleanup |
| `unlink(path)` before `bind()` only for paths you own | Avoid stale socket startup failures without deleting arbitrary files |
| Set restrictive directory permissions | More portable than relying only on socket file mode |
| Use `socketpair()` for parent-child IPC | No pathname, no cleanup race, bidirectional |
| Use peer credentials for local authorization | Avoid trusting client-supplied uid strings |
| Use fd passing for privilege separation | Broker opens privileged resource, worker receives fd |
| Keep stream framing explicit | UNIX stream is still a byte stream |
| Bound message sizes for datagrams | Oversized datagrams can be truncated or fail |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Abstract socket namespace | Linux-only, first byte of `sun_path` is zero, not visible in filesystem |
| `SCM_RIGHTS` | Passes references to open file descriptions over UNIX sockets |
| `SCM_CREDENTIALS` | Linux credential passing via ancillary data |
| `SO_PEERCRED` | Linux `getsockopt()` for peer pid/uid/gid on UNIX sockets |
| `SOCK_SEQPACKET` | Reliable message boundaries with connection semantics on Linux UNIX sockets |
| Autobind | Linux can auto-bind some UNIX sockets in credential-passing scenarios; recognize, do not rely casually |

## Example

### Example 1 - Parent/Child IPC with `socketpair()`

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int write_all(int fd, const char *msg)
{
    size_t len = strlen(msg);
    while (len > 0) {
        ssize_t n = write(fd, msg, len);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        msg += n;
        len -= (size_t)n;
    }
    return 0;
}

int main(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        die("socketpair");
    }

    pid_t pid = fork();
    if (pid == -1) {
        die("fork");
    }

    if (pid == 0) {
        close(sv[0]);
        if (write_all(sv[1], "hello from child\n") == -1) {
            die("write");
        }
        close(sv[1]);
        _exit(EXIT_SUCCESS);
    }

    close(sv[1]);

    char buf[128];
    ssize_t n = read(sv[0], buf, sizeof(buf) - 1);
    if (n == -1) {
        die("read");
    }
    buf[n] = '\0';
    printf("%s", buf);

    close(sv[0]);
    wait(NULL);
    return 0;
}
```

What it teaches:

- `socketpair()` is ideal for related processes.
- Both sides must close the unused endpoint after `fork()`.
- The channel is bidirectional even though this example uses one direction.

### Example 2 - Minimal UNIX Stream Server

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/demo_unix_stream.sock"
#define BACKLOG 8
#define BUF_SIZE 256

static void cleanup(void)
{
    unlink(SOCK_PATH);
}

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    int lfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lfd == -1) {
        die("socket");
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        die("signal");
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCK_PATH);

    unlink(SOCK_PATH);
    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        die("bind");
    }
    atexit(cleanup);

    if (listen(lfd, BACKLOG) == -1) {
        die("listen");
    }

    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd == -1) {
            if (errno == EINTR) {
                continue;
            }
            die("accept");
        }

        char buf[BUF_SIZE];
        ssize_t n = read(cfd, buf, sizeof(buf));
        if (n > 0) {
            (void)write(cfd, buf, (size_t)n);
        }
        close(cfd);
    }
}
```

What it teaches:

- `bind()` creates a socket pathname.
- `unlink()` is part of the lifecycle.
- UNIX stream sockets still need stream protocol discipline.

## Debugging

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Stale socket path | `bind()` fails with `EADDRINUSE` | `ls -l`, verify type `s`, controlled `unlink()` |
| Socket in `/tmp` with predictable name | Local denial-of-service or wrong peer | Use owned runtime directory |
| Path too long | `bind()`/`connect()` fails or truncates in bad code | Check `sizeof(addr.sun_path)` before copying |
| Forgetting client bind for UNIX datagram reply | Server cannot reply | Client binds a unique pathname |
| Treating socket file like normal file | `open()`/file I/O does not communicate | Use `socket()` + `connect()`/`sendto()` |
| Leaking socketpair endpoints | EOF never arrives | Close unused fd in parent and child |
| Assuming socket permissions are portable | Access control differs across UNIX systems | Use directory permissions and credentials |

### Commands

```bash
# List UNIX domain sockets
ss -xap

# Show UNIX sockets from proc
cat /proc/net/unix

# Inspect a pathname socket
ls -l /run/myapp/control.sock
stat /run/myapp/control.sock

# Match process fds to sockets
ls -l /proc/<pid>/fd

# Trace local socket syscalls
strace -f -e trace=socket,bind,listen,accept,connect,sendmsg,recvmsg,unlink ./daemon

# Find processes using a pathname socket
lsof -U | grep control.sock
```

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Service control CLI | `AF_UNIX + SOCK_STREAM` under `/run/service/` |
| Privilege-separated daemon | Root broker passes fds to unprivileged worker with `SCM_RIGHTS` |
| Parent-child worker IPC | `socketpair()` before `fork()` |
| Local metrics collector | UNIX datagram if messages are independent and bounded |
| Container/runtime API | UNIX stream socket plus peer credential checks |
| Secure local admin API | Directory permissions plus `SO_PEERCRED`, not client-supplied identity |

## Interview Readiness

1. When would you choose `AF_UNIX` instead of `AF_INET` loopback?
2. What does `bind()` create for a pathname UNIX socket?
3. Why must many UNIX socket servers call `unlink()`?
4. Why is `/tmp/my.sock` a risky production address?
5. How do UNIX stream sockets differ from pipes?
6. How do UNIX datagram sockets differ from UDP sockets?
7. What does `socketpair()` create, and where is it useful?
8. Why does `socketpair()` avoid a class of security issues?
9. How do filesystem permissions affect UNIX socket access?
10. What is the portable access-control boundary for pathname sockets?
11. What are peer credentials, and why are they safer than trusting client text?
12. What does passing a file descriptor really pass?
13. How can fd passing support prefork or privilege-separated servers?
14. What is the Linux abstract namespace?
15. Which commands help debug a stale UNIX socket path?

## Key Takeaways

1. UNIX domain sockets are same-host sockets, not network sockets.
2. Pathname sockets live in the filesystem namespace but do not store data there.
3. `bind()` fails if the socket pathname already exists.
4. Clean up pathname sockets with `unlink()`.
5. Prefer secured runtime directories over world-writable paths.
6. UNIX stream sockets are reliable byte streams with no message boundaries.
7. UNIX datagram sockets preserve message boundaries and are reliable/ordered per TLPI.
8. `socketpair()` is the cleanest parent-child socket IPC pattern.
9. UNIX sockets can expose peer credentials for local authentication.
10. FD passing is a major reason UNIX sockets matter in production daemons.
11. Abstract namespace sockets are Linux-specific.
12. Debug UNIX sockets with `ss -x`, `/proc/net/unix`, `lsof -U`, and `strace`.
