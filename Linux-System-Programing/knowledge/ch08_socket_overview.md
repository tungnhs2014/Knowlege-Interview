# Chapter 8 - Socket API Overview

> Topics: 8.1 Sockets Introduction - `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, stream vs datagram.
> Main sources: TLPI Ch56; DevLinux module 06.
> Production context: backend services, local agents, embedded gateways, log collectors, RPC daemons, and any Linux program that talks to another process through a file descriptor.

---

## Coverage Notes

This file covers Coverage Matrix row 8.1 and the shared socket model required by Chapter 8.

- Covered here: endpoint mental model, fd interface, address family, socket type, protocol selection, `socket()`/`bind()`/`listen()`/`accept()`/`connect()` lifecycle, stream/datagram behavior, cleanup, common production bugs, first-pass debugging commands, Embedded usage, checklist, and interview readiness.
- Cross-file coverage: UNIX-domain pathname/abstract/fd-passing details are in `ch08_socket_unix.md`; TCP/IP, byte order, DNS, IPv4/IPv6, and `getaddrinfo()` are in `ch08_socket_tcp.md`; server concurrency and overload behavior are in `ch08_socket_server.md`; `sendmsg()`/`recvmsg()`, timeouts, keepalive, and advanced options are in `ch08_socket_advanced.md`.
- No mapped Chapter 8 socket-model concept is intentionally out of scope.

## Problem It Solves

Programs rarely live alone. A web server must accept clients, an embedded gateway must receive telemetry, a local daemon must expose a control channel, and tools often need to talk across process boundaries.

Sockets solve this by giving user-space a single API for communication:

```text
process A writes bytes/datagrams into a socket fd
        |
        v
kernel socket layer routes them by domain/type/address
        |
        v
process B reads them from another socket fd
```

The important mental shift is this: a socket is not "the network". A socket is a kernel communication endpoint exposed as a file descriptor. The same basic API can target local IPC (`AF_UNIX`) or network communication (`AF_INET`, `AF_INET6`).

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | `socket()`, domain, type, address, stream flow, datagram flow | Build a basic TCP/UDP or UNIX-domain client/server without guessing |
| Work useful | `SO_REUSEADDR`, partial I/O, `SIGPIPE`, backlog, cleanup, byte-stream framing | Avoid common production bugs in small servers |
| Recognize | `accept4()`, connected datagram sockets, `SOCK_CLOEXEC`, `SOCK_NONBLOCK`, advanced socket options | Understand codebases that use Linux-specific improvements |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Socket | Kernel communication endpoint represented by a file descriptor | Returned by `socket()` |
| Domain / address family | Scope and address format for communication | `AF_UNIX`, `AF_INET`, `AF_INET6` |
| Socket type | Communication semantics | `SOCK_STREAM` or `SOCK_DGRAM` |
| Protocol | Concrete protocol selected for a domain/type pair | Usually `0`; TCP for `AF_INET + SOCK_STREAM`, UDP for `AF_INET + SOCK_DGRAM` |
| Address | Name used to locate a socket | UNIX pathname, IPv4/IPv6 address plus port |
| `struct sockaddr` | Generic address pointer type accepted by socket syscalls | Real object is `sockaddr_in`, `sockaddr_in6`, or `sockaddr_un` |
| Stream socket | Reliable bidirectional byte stream with no message boundaries | TCP, UNIX stream |
| Datagram socket | Message-oriented socket; each send is one datagram | UDP, UNIX datagram |
| Listening socket | Passive stream socket that accepts connections | Result of `socket()` + `bind()` + `listen()` |
| Connected socket | Per-peer stream endpoint used for I/O | Returned by `accept()` or connected by `connect()` |
| Passive open | Server-side readiness to accept connections | `listen()` then `accept()` |
| Active open | Client-side connection attempt | `connect()` |
| Backlog | Limit for pending stream connection queue | `listen(fd, backlog)` |
| Peer | The socket/application at the other end | Used in logs as remote address |
| EOF on stream | Peer closed its write side after buffered data is read | `read()` returns `0` |
| `SIGPIPE` / `EPIPE` | Write to a stream whose peer closed | Ignore `SIGPIPE` or use socket-specific handling |

## Concept Overview

Sockets are built from three choices:

| Choice | Question | Common answer |
|--------|----------|---------------|
| Domain | Where can the peer live? | Same host: `AF_UNIX`; network: `AF_INET`/`AF_INET6` |
| Type | Is data a stream or messages? | Stream: `SOCK_STREAM`; datagram: `SOCK_DGRAM` |
| Address | How does the peer find this socket? | Pathname or IP+port |

### Stream Mental Model

Stream sockets are like a reliable pipe in both directions:

```text
client write("hello")
client write("world")
        |
        v
server read(buf, 1024) may return "helloworld"
```

TCP preserves byte order and reliability, not application messages. If your protocol has requests, frames, or records, your application must define framing: newline, fixed-size header, length-prefix, or a real protocol format.

### Datagram Mental Model

Datagram sockets preserve message boundaries:

```text
sendto("one")
sendto("two")
        |
        v
recvfrom() -> "one"
recvfrom() -> "two"
```

For UDP, delivery is not guaranteed: datagrams can be lost, duplicated, or reordered. For UNIX domain datagrams, TLPI notes delivery is reliable and ordered because transfer stays inside the kernel, but queue limits still matter.

## System Context

Sockets sit at the boundary of several Linux subsystems:

| Subsystem | Socket interaction |
|-----------|--------------------|
| File descriptor table | A socket fd behaves like other fds for `read()`, `write()`, `close()`, `poll()`, `epoll()` |
| VFS | UNIX domain pathname sockets appear as special filesystem entries |
| Network stack | Internet sockets pass data through TCP/UDP/IP and network drivers |
| Process lifecycle | `fork()` duplicates socket fds; `exec()` preserves them unless `FD_CLOEXEC` is set |
| Signals | Writing to a closed stream may raise `SIGPIPE` |
| Threads/event loops | Servers commonly combine sockets with threads or I/O multiplexing |

Failure modes are production-visible: `EADDRINUSE` on restart, leaked connected sockets, blocked `accept()`, dropped UDP packets, partial stream reads, and protocols that accidentally assume TCP message boundaries.

## Architecture

### Objects the Kernel Tracks

| Object | Tracked state | Why it matters |
|--------|---------------|----------------|
| Socket fd | Per-process descriptor number | What user-space passes to syscalls |
| Open file description | Status flags such as nonblocking | Shared after `dup()`/`fork()` for many fd operations |
| Socket object | Domain, type, protocol, buffers, options | The real communication endpoint |
| Bound address | Local name for receiving traffic | Pathname or IP+port |
| Peer association | Remote endpoint for stream or connected datagram | Decides where data goes |
| Listen queue | Pending stream connections | Backlog pressure shows up under load |

### Core API Shape

```text
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest, socklen_t destlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *srclen);
```

### Stream Server Shape

```text
listening fd
    |
    | accept()
    v
connected fd for client A

listening fd remains open for future clients
```

This is the most important `accept()` detail. `accept()` does not "turn the listening socket into a client socket"; it returns a new connected socket.

## Execution Flow

### Stream Server Flow

```text
socket(AF_*, SOCK_STREAM, 0)
    |
    v
bind(local address)
    |
    v
listen(backlog)
    |
    v
accept() -> connected fd
    |
    v
read()/write() or recv()/send()
    |
    v
close(connected fd)
```

### Stream Client Flow

```text
socket(AF_*, SOCK_STREAM, 0)
    |
    v
connect(server address)
    |
    v
application protocol exchange
    |
    v
close()
```

### Datagram Flow

```text
receiver: socket(AF_*, SOCK_DGRAM, 0)
              |
              v
          bind(local address)
              |
              v
          recvfrom() -> data + sender address

sender:   socket(AF_*, SOCK_DGRAM, 0)
              |
              v
          sendto(destination address)
```

### Cleanup Flow

```text
error after socket()
    |
    v
close(fd)
    |
    v
if AF_UNIX pathname was bound: unlink(path)
```

### Common Failure Flow: Peer Closed

```text
peer close()
    |
    v
local read() returns remaining bytes, then 0
    |
    v
local write() may raise SIGPIPE and fail with EPIPE
```

## 8.1 API / Topic Sections

### `socket()`

Use `socket()` to allocate a socket endpoint. In production code, prefer setting close-on-exec and nonblocking atomically when available on Linux:

| Need | Common approach |
|------|-----------------|
| Portable baseline | `socket(domain, type, 0)` then `fcntl()` |
| Linux race avoidance | `socket(domain, type | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)` |

`SOCK_CLOEXEC` and `SOCK_NONBLOCK` are Linux-specific extensions, not portable POSIX baseline behavior.

### `bind()`

Use `bind()` when the process must receive traffic at a known address. Servers almost always bind. Clients often do not; for Internet sockets the kernel can assign an ephemeral port.

Pitfall: `bind()` fails with `EADDRINUSE` if the address is already in use. For TCP servers, set `SO_REUSEADDR` before `bind()`.

### `listen()` and `accept()`

`listen()` marks a stream socket as passive. `accept()` returns a new connected fd. The listening fd remains open.

Pitfall: backlog is not infinite. Under load, the kernel queues can fill and clients may time out or fail. On Linux, `/proc/sys/net/core/somaxconn` caps the effective backlog.

### `connect()`

For stream sockets, `connect()` establishes the connection. If `connect()` fails and you want a portable retry, close the socket and create a new one.

For datagram sockets, `connect()` only records a default peer and filters incoming datagrams to that peer. It does not create a TCP-like session.

### `read()` / `write()` vs `send()` / `recv()`

`read()` and `write()` are fine for connected sockets. Use `send()`/`recv()` when you need socket-specific flags such as `MSG_DONTWAIT`, `MSG_PEEK`, or `MSG_NOSIGNAL`.

## Work Checklist

| Pattern | Why it matters |
|---------|----------------|
| Always close the connected fd in the parent after `fork()` | Prevent fd leaks and connections that never reach EOF |
| Ignore or handle `SIGPIPE` in servers | A closed client should not kill the process |
| Write framing for stream protocols | TCP does not preserve request boundaries |
| Loop for partial writes | `write()`/`send()` can transfer fewer bytes than requested |
| Set `SO_REUSEADDR` before `bind()` for TCP servers | Makes restart after `TIME_WAIT` practical |
| Keep UDP datagrams bounded | Large datagrams risk fragmentation and loss |
| Use absolute UNIX socket paths in secured directories | Avoid `/tmp` races and stale path surprises |
| Use `getaddrinfo()` in Internet code | Keeps code IPv4/IPv6-aware |
| Make cleanup paths boring | `close()` fds and `unlink()` UNIX socket pathnames |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| `accept4()` | Linux syscall that can set `SOCK_CLOEXEC`/`SOCK_NONBLOCK` on accepted fds atomically |
| Connected datagram socket | `connect()` sets a default peer and lets you use `write()`/`send()` |
| `SOCK_SEQPACKET` | Reliable connection-oriented messages; supported for UNIX domain sockets on Linux |
| `SO_RCVBUF` / `SO_SNDBUF` | Tune socket buffers, but do not use as a first fix for bad protocol design |
| `FIONREAD` | Linux can report unread bytes; useful for debugging, not portable core design |

## Example

### Example 1 - Minimal TCP Echo Server

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 9090
#define BACKLOG 16
#define BUF_SIZE 1024

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;

    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        p += n;
        len -= (size_t)n;
    }

    return 0;
}

int main(void)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        die("socket");
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        die("signal");
    }

    int yes = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        die("bind");
    }

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
        ssize_t n;
        while ((n = read(cfd, buf, sizeof(buf))) > 0) {
            if (write_all(cfd, buf, (size_t)n) == -1) {
                break;
            }
        }

        close(cfd);
    }
}
```

What it teaches:

- `accept()` returns a connected fd; the listening fd stays alive.
- TCP I/O must handle partial writes and closed peers.
- `SO_REUSEADDR` belongs before `bind()`.

### Example 2 - Minimal UDP Echo Server

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 9091
#define BUF_SIZE 512

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        die("socket");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        die("bind");
    }

    for (;;) {
        char buf[BUF_SIZE];
        struct sockaddr_storage peer;
        socklen_t peer_len = sizeof(peer);

        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                             (struct sockaddr *)&peer, &peer_len);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            die("recvfrom");
        }

        if (sendto(fd, buf, (size_t)n, 0,
                   (struct sockaddr *)&peer, peer_len) == -1) {
            perror("sendto");
        }
    }
}
```

What it teaches:

- UDP has no `listen()` or `accept()`.
- `recvfrom()` returns both data and sender address.
- One UDP socket can serve many clients because each datagram carries addressing context.

## Debugging

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Treating TCP as messages | Server reads half a request or two requests together | Add protocol framing |
| Forgetting `SO_REUSEADDR` | Server restart fails with `EADDRINUSE` | Set option before `bind()` |
| Parent keeps connected fd after `fork()` | Client never sees EOF; fd count grows | Parent closes `cfd`, child closes `lfd` |
| Ignoring partial writes | Truncated responses under load or signals | Use `write_all()` / `writen()` |
| Not handling `SIGPIPE` | Server exits when client disconnects | Ignore `SIGPIPE` or use `MSG_NOSIGNAL` on Linux |
| UDP buffer too small | Datagram is truncated | Define max message size and check truncation |
| Backlog too small | Connect timeouts during bursts | Inspect `ss`, `somaxconn`, app accept rate |

### Commands

```bash
# Listening TCP/UDP sockets with process info
ss -ltnup

# All TCP states, numeric addresses
ss -tan

# Show sockets owned by a process
ls -l /proc/<pid>/fd

# Trace socket lifecycle
strace -f -e trace=socket,bind,listen,accept,connect,sendto,recvfrom,close ./server

# Check backlog cap and ephemeral port range on Linux
cat /proc/sys/net/core/somaxconn
cat /proc/sys/net/ipv4/ip_local_port_range

# Legacy but still useful when available
netstat -anp
```

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Simple local daemon control | `AF_UNIX + SOCK_STREAM`, secured directory, length-prefixed commands |
| Backend HTTP-like service | `AF_INET6`/`AF_INET`, `SOCK_STREAM`, `getaddrinfo()`, framing/parser |
| Metrics or telemetry packets | UDP only if loss is acceptable and messages are bounded |
| Embedded gateway | TCP for command/control, UDP for discovery or periodic best-effort status |
| Parent-child IPC | `socketpair(AF_UNIX, SOCK_STREAM, 0, fds)` |
| Service restart friendliness | `SO_REUSEADDR`, graceful shutdown, clear ownership of fds |

## Interview Readiness

1. What is a socket, and why is it represented as a file descriptor?
2. What does the socket domain decide?
3. Compare `SOCK_STREAM` and `SOCK_DGRAM`.
4. Why does TCP not preserve message boundaries?
5. What is the difference between a listening socket and a connected socket?
6. What exactly does `accept()` return?
7. Why does a server usually call `bind()` but a client often does not?
8. What does `listen(backlog)` control, and what does it not guarantee?
9. What happens when you write to a stream socket whose peer closed?
10. When would you use `send()`/`recv()` instead of `read()`/`write()`?
11. What is a connected UDP socket?
12. Why should stream protocols define framing?
13. How do sockets interact with `fork()`?
14. Why is `SO_REUSEADDR` commonly set by TCP servers?
15. Which commands would you use to debug a socket server that is not accepting clients?

## Key Takeaways

1. A socket is a kernel communication endpoint exposed through an fd.
2. Domain chooses address family and communication range.
3. Type chooses stream vs datagram semantics.
4. Stream sockets are reliable byte streams, not message streams.
5. Datagram sockets preserve message boundaries.
6. `accept()` returns a new connected fd; the listening fd remains open.
7. Servers bind to known addresses; clients can often use ephemeral addresses.
8. TCP servers should handle `SIGPIPE`, partial I/O, and `TIME_WAIT` restarts.
9. UDP must tolerate loss, duplication, reordering, and truncation unless the domain guarantees otherwise.
10. Socket fds are inherited across `fork()` and possibly `exec()`, so close/`CLOEXEC` discipline matters.
11. Debug sockets with `ss`, `/proc`, `strace`, and packet/state tools.
12. Production socket code is mostly resource lifecycle plus protocol correctness.
