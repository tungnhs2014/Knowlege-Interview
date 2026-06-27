# Chapter 8 - Advanced Socket Topics

> Topics: 8.6 Advanced Socket Topics - partial I/O, `shutdown()`, `send()`/`recv()` flags, `sendfile()`, TCP state, socket options, `SO_REUSEADDR`, `sendmsg()`/`recvmsg()`.
> Main sources: TLPI Ch61; TLPI Ch60 for server implications.
> Production context: robust TCP services, file-serving paths, graceful shutdown, restart behavior, advanced UNIX-socket daemon patterns, and socket debugging under load.

---

## Coverage Notes

This file covers Coverage Matrix row 8.6 and the Chapter 8 advanced-socket-topic Must Cover item.

- Covered here: partial reads/writes, `shutdown()`, `send()`/`recv()` flags, `sendfile()`, TCP states, `SO_REUSEADDR`, accepted-fd inheritance, `sendmsg()`/`recvmsg()`, ancillary data, nonblocking I/O, per-call nonblocking flags, socket timeouts, TCP keepalive, production bugs, debugging with `ss`, `tcpdump`, `strace`, logs, Embedded constraints, checklist, and interview readiness.
- Cross-file coverage: base socket lifecycle is in `ch08_socket_overview.md`; UNIX-domain fd passing and credentials are in `ch08_socket_unix.md`; TCP/IP addressing and DNS are in `ch08_socket_tcp.md`; server concurrency and overload policy are in `ch08_socket_server.md`.
- Detailed event-loop APIs are covered in Chapter 9; raw sockets, SCTP/DCCP, and OOB data remain recognize-only.

## Problem It Solves

Basic socket code often works in a demo and fails in production because production exposes edge behavior:

```text
signals interrupt I/O
clients disconnect mid-write
TCP returns partial reads
server restart hits TIME_WAIT
accepted fds miss nonblocking flags
UDP is faster but unreliable
```

Advanced socket knowledge is not about memorizing every flag. It is about knowing which mechanisms explain real failures and which options are safe tools.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | partial I/O, `SIGPIPE`/`EPIPE`, `shutdown(SHUT_WR)`, `SO_REUSEADDR`, TCP states | Debug robust TCP behavior |
| Work useful | `send()`/`recv()` flags, `getsockname()`, `getpeername()`, `tcpdump`, socket option inheritance | Build production-ready server/client loops |
| Recognize | `sendfile()`, `TCP_CORK`, `sendmsg()`/`recvmsg()`, ancillary data, OOB, SCTP/DCCP | Understand optimized and specialized code |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Partial read | `read()` returns fewer bytes than requested | Normal for stream sockets |
| Partial write | `write()` sends fewer bytes than requested | Handle with a write loop |
| Half-close | Close one direction of a full-duplex stream | `shutdown(fd, SHUT_WR)` |
| Full close | Close fd reference and eventually both directions | `close(fd)` |
| `SIGPIPE` | Signal raised when writing to closed stream peer | Often ignored in servers |
| `EPIPE` | Error returned for write to closed stream after/without `SIGPIPE` handling | Check write errors |
| `MSG_PEEK` | Read a copy without consuming socket buffer | Debug/protocol lookahead |
| `MSG_DONTWAIT` | Nonblocking behavior for one I/O call | Per-call alternative to `O_NONBLOCK` |
| `MSG_NOSIGNAL` | Linux: avoid `SIGPIPE` for one send | `send(..., MSG_NOSIGNAL)` |
| `sendfile()` | Kernel-assisted file-to-socket transfer | Common in static file serving |
| `TCP_CORK` | Linux option to coalesce TCP output | Header + file body optimization |
| Socket option | Tunable/readable socket behavior | `setsockopt()`, `getsockopt()` |
| `SO_REUSEADDR` | Allows common safe TCP server rebinding cases | Set before `bind()` |
| `SO_RCVTIMEO` / `SO_SNDTIMEO` | Bound blocking receive/send calls | Useful for simple blocking servers |
| `SO_KEEPALIVE` | Ask TCP to probe long-idle connections | Failure detector, not an application heartbeat |
| `TIME_WAIT` | TCP state after active close | Protects reliable termination and old segment expiry |
| `CLOSE_WAIT` | Local app has not closed after peer sent FIN | Often indicates app fd leak/logic bug |
| Ancillary data | Control metadata in `sendmsg()`/`recvmsg()` | FD passing with `SCM_RIGHTS` |

## Concept Overview

### Partial I/O Is Normal

Stream sockets are byte streams. The kernel is free to return the bytes currently available or accept only part of a write.

```text
application wants 4096 bytes
        |
        v
read(fd, buf, 4096) returns 800
        |
        v
not an error; continue according to protocol framing
```

Write loops are mandatory when the application must send a complete buffer.

### `shutdown()` vs `close()`

| Operation | Effect |
|-----------|--------|
| `close(fd)` | Closes this fd reference; connection ends only when all duplicated references are closed |
| `shutdown(fd, SHUT_WR)` | Sends EOF to peer for local write side, while local read side can remain open |
| `shutdown(fd, SHUT_RDWR)` | Disables both directions on the socket object, even if other fd references exist |

`SHUT_WR` is the common half-close for TCP. Avoid relying on `SHUT_RD` for portable TCP behavior; TLPI notes it varies across implementations.

### TCP State Explains Symptoms

| State | Practical meaning |
|-------|-------------------|
| `LISTEN` | Server waiting for connections |
| `SYN_SENT` | Client sent connect request |
| `ESTABLISHED` | Data can flow |
| `FIN_WAIT*` | Local side initiated close |
| `CLOSE_WAIT` | Peer closed; local app has not closed yet |
| `TIME_WAIT` | Active closer waits before connection tuple can fully disappear |

`TIME_WAIT` is not a bug. It protects TCP reliability. Use `SO_REUSEADDR` for normal server restart cases instead of trying to disable `TIME_WAIT`.

## System Context

| System area | Advanced socket interaction |
|-------------|-----------------------------|
| Signals | `EINTR`, `SIGPIPE`, `SIGCHLD`, `SIGURG` |
| FD/open file descriptions | `dup()`/`fork()` affect when `close()` really closes a connection |
| TCP stack | State machine, retransmission, flow control, congestion control |
| `/proc/net` | Kernel socket state visible to tools |
| Packet capture | `tcpdump` shows SYN/ACK/FIN/RST and retransmission |
| Filesystem/page cache | `sendfile()` can avoid user-space copy for file serving |
| UNIX sockets | `sendmsg()`/`recvmsg()` can pass fds and credentials |

This chapter connects directly to earlier chapters: file descriptors, signals, process inheritance, threads, and later I/O multiplexing.

## Architecture

### Robust Stream I/O

```text
protocol says: next frame length = N
        |
        v
read exactly N bytes using loop
        |
        v
process complete frame

response buffer length = M
        |
        v
write until all M bytes sent or error
```

### Socket Options

```text
socket()
    |
    v
setsockopt() before bind/listen/connect when option affects setup
    |
    v
bind/listen/connect
    |
    v
getsockopt() for diagnostics or inherited fd inspection
```

Options are generally associated with the socket/open file description. Accepted sockets inherit many socket options, but Linux does not inherit some fd/status flags such as `O_NONBLOCK` and `FD_CLOEXEC` from the listening socket. Use `accept4()` on Linux or set flags after `accept()`.

### Timeouts and Keepalive

Blocking sockets need an explicit policy for waiting. Without one, a peer that stops sending or stops reading can hold a worker, fd, memory buffer, or protocol state forever.

| Tool | What it does | What it does not do |
|------|--------------|---------------------|
| `SO_RCVTIMEO` | Bounds blocking receive calls | It is not a full request deadline |
| `SO_SNDTIMEO` | Bounds blocking send calls when buffers stay full | It does not guarantee peer processed data |
| `O_NONBLOCK` | Makes operations return `EAGAIN` instead of sleeping | Requires retry/event-loop logic |
| `MSG_DONTWAIT` | Nonblocking behavior for one call | Does not change future calls |
| `SO_KEEPALIVE` | Lets TCP probe an idle connection after long inactivity | Does not replace protocol heartbeats or short failure detection |
| Application deadline | User-space timer around a full request, response, or idle period | The kernel will not enforce this for you |

Use socket timeouts for simple blocking designs. Use nonblocking I/O plus timers when many clients share a small worker set. Use application-level deadlines when one logical request spans multiple reads, writes, DNS work, or backend calls.

For Embedded devices, prefer explicit protocol deadlines and small buffers. TCP keepalive can wake radios or links, so tune probe intervals deliberately instead of blindly enabling it.

### `sendmsg()` / `recvmsg()`

```text
msghdr
    |
    +-- normal data buffers (scatter/gather)
    |
    +-- optional address
    |
    +-- ancillary data: fd, credentials, packet metadata
```

Most applications do not need these calls. UNIX daemon infrastructure often does.

## Execution Flow

### `writen()` Flow

```text
want to send count bytes
    |
    v
write remaining bytes
    |
    +-- n > 0: advance pointer
    +-- EINTR: retry
    +-- other error: fail
    |
    v
done only when total == count
```

### Half-Close Request Flow

```text
client sends request body
    |
    v
shutdown(fd, SHUT_WR)
    |
    v
server read() eventually returns 0
    |
    v
server sends response
    |
    v
client still reads response
```

### `SO_REUSEADDR` Restart Flow

```text
old server had connection and active-closed
    |
    v
TCP endpoint remains TIME_WAIT
    |
    v
new server sets SO_REUSEADDR before bind
    |
    v
bind well-known port succeeds for normal restart case
```

### TCP Handshake Flow

```text
client -> SYN
server -> SYN/ACK
client -> ACK
state becomes ESTABLISHED
```

### TCP Close Flow

```text
active closer -> FIN
peer          -> ACK
peer later    -> FIN
active closer -> ACK
active closer enters TIME_WAIT
```

## 8.6 API / Topic Sections

### Partial Reads/Writes

Use loops for exact-length protocols. For stream protocols, a short read is not an error. For datagrams, one receive reads one datagram; if the buffer is too small, excess data can be discarded depending API/flags.

### `shutdown()`

Use `SHUT_WR` when you need to tell the peer "I am done sending" but still want to read the peer response. This appears in protocols where request body EOF is meaningful.

### `send()` / `recv()` Flags

| Flag | Use |
|------|-----|
| `MSG_DONTWAIT` | Per-call nonblocking receive/send |
| `MSG_PEEK` | Look without consuming |
| `MSG_WAITALL` | Try to wait for full length, but still handle short returns |
| `MSG_NOSIGNAL` | Linux: avoid `SIGPIPE` on send |
| `MSG_MORE` | Linux: hint more output is coming |

Keep mainline code simple. Use flags when they solve a concrete problem.

### `sendfile()` and `TCP_CORK`

`sendfile()` is useful when sending file contents through a socket without transforming them. `TCP_CORK` or `MSG_MORE` can help combine small headers with file data on Linux. These are optimization tools; correctness comes first.

### `getsockname()` and `getpeername()`

Use `getsockname()` to learn the local address/ephemeral port selected by the kernel. Use `getpeername()` when a process inherited a connected socket and needs peer info.

### `SO_REUSEADDR`

Set before `bind()` for TCP servers. It solves common restart cases without defeating the purpose of `TIME_WAIT`.

### Blocking, Nonblocking, and Timeout APIs

For blocking sockets, `setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, ...)` and `SO_SNDTIMEO` make individual I/O calls fail after a bounded wait, commonly with `EAGAIN`/`EWOULDBLOCK`. For nonblocking sockets, use `fcntl(fd, F_SETFL, O_NONBLOCK)` or create/accept with `SOCK_NONBLOCK`/`accept4()`, then treat `EAGAIN` as "try again when readiness/timer says so."

Socket timeouts protect syscalls, not whole protocol states:

```text
socket timeout protects one blocking call
application deadline protects one logical request
idle timeout protects scarce connection slots
```

Keepalive belongs to long-lived TCP connections where an idle dead peer is worse than extra probes. On Linux, `SO_KEEPALIVE` enables probing; probe timing is controlled by system defaults and TCP-level options such as `TCP_KEEPIDLE`, `TCP_KEEPINTVL`, and `TCP_KEEPCNT` when code needs per-socket tuning. These options are Linux/POSIX-sensitive, so document assumptions near the code.

Keepalive is not a substitute for application health:

- It may take minutes or hours with defaults.
- It only proves the TCP peer stack responded, not that the remote application is healthy.
- On battery/cellular Embedded devices, probes can cost power and network traffic.
- For request/response services, explicit request deadlines are usually more important.

### Accept Inheritance

On Linux, accepted fds do not inherit `O_NONBLOCK`, `FD_CLOEXEC`, or signal-driven I/O owner settings from the listening fd. Socket options mostly are inherited. POSIX/SUS leaves details loose, so portable code sets required flags explicitly.

### TCP vs UDP Revisited

UDP can be faster for small request/reply traffic and supports broadcast/multicast. TCP is usually better when reliability, flow control, and congestion control matter.

## Work Checklist

| Pattern | Why it matters |
|---------|----------------|
| Wrap writes in `write_all()` | Prevent silent truncation |
| Treat `read() == 0` as peer EOF | Correct stream lifecycle |
| Ignore `SIGPIPE` in servers | Do not let one client kill process |
| Use `shutdown(SHUT_WR)` for request EOF | Avoid deadlock in request/response protocols |
| Set `SO_REUSEADDR` before `bind()` | Server restart reliability |
| Set accepted fd flags explicitly | Avoid Linux/portable inheritance surprises |
| Add a wait policy | Prevent dead peers from pinning workers forever |
| Use keepalive only with clear timing expectations | Avoid mistaking idle probes for app-level health checks |
| Inspect `CLOSE_WAIT` | Usually means app forgot to close after peer EOF |
| Use numeric logging in hot paths | Avoid reverse DNS stalls |
| Capture packets for handshake/FIN/RST bugs | `tcpdump` shows what the kernel sees |
| Keep advanced options documented near use | Future maintainers need the reason |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Out-of-band data | Historical TCP urgent data; discouraged; often replace with separate control channel |
| `sendmsg()`/`recvmsg()` | General socket I/O, scatter/gather, ancillary data |
| `SCM_RIGHTS` | UNIX socket fd passing |
| `SCM_CREDENTIALS` | Linux credential ancillary data |
| `recvmmsg()`/`sendmmsg()` | Linux batching for high-rate datagrams |
| `TCP_KEEPIDLE`/`TCP_KEEPINTVL`/`TCP_KEEPCNT` | Linux TCP keepalive tuning knobs |
| `/proc/sys/net/ipv4/tcp_keepalive_*` | System-wide Linux keepalive defaults |
| `SOCK_SEQPACKET` | Reliable message boundary sockets for supported domains/protocols |
| SCTP | Reliable message-oriented transport with multistreaming; specialized |
| DCCP | Datagram transport with congestion control but no reliability; specialized |

## Example

### Example 1 - Exact Write and TCP-Style Half-Close with `socketpair()`

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

        char buf[64];
        ssize_t n;
        while ((n = read(sv[1], buf, sizeof(buf))) > 0) {
            if (write_all(STDOUT_FILENO, buf, (size_t)n) == -1) {
                die("write stdout");
            }
        }

        const char reply[] = "child saw EOF\n";
        if (write_all(sv[1], reply, strlen(reply)) == -1) {
            die("write reply");
        }
        close(sv[1]);
        _exit(EXIT_SUCCESS);
    }

    close(sv[1]);

    const char request[] = "request body\n";
    if (write_all(sv[0], request, strlen(request)) == -1) {
        die("write request");
    }

    if (shutdown(sv[0], SHUT_WR) == -1) {
        die("shutdown");
    }

    char reply[64];
    ssize_t n = read(sv[0], reply, sizeof(reply) - 1);
    if (n == -1) {
        die("read reply");
    }
    reply[n] = '\0';
    printf("%s", reply);

    close(sv[0]);
    wait(NULL);
    return 0;
}
```

What it teaches:

- `write_all()` handles partial writes and `EINTR`.
- `shutdown(SHUT_WR)` sends EOF while keeping the read side open.
- Duplicated/inherited fds affect when EOF is observed.

### Example 2 - Bind Port 0 and Discover the Ephemeral Port

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        die("socket");
    }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        die("bind");
    }

    socklen_t len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == -1) {
        die("getsockname");
    }

    printf("kernel chose port %u\n", (unsigned)ntohs(addr.sin_port));
    close(fd);
    return 0;
}
```

What it teaches:

- Port `0` asks the kernel to choose an ephemeral port.
- `getsockname()` retrieves the local address actually bound.
- `SO_REUSEADDR` must be set before `bind()` when it affects binding.

### Example 3 - `sendfile()` with `TCP_CORK` Helper

```c
#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>

int send_file_with_header(int sockfd, int filefd, off_t size)
{
    int one = 1;
    int zero = 0;
    const char header[] = "HTTP/1.0 200 OK\r\n\r\n";
    const char *p = header;
    size_t left = sizeof(header) - 1;

    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &one, sizeof(one)) == -1) {
        return -1;
    }

    while (left > 0) {
        ssize_t n = write(sockfd, p, left);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        p += n;
        left -= (size_t)n;
    }

    off_t off = 0;
    while (off < size) {
        ssize_t n = sendfile(sockfd, filefd, &off, (size_t)(size - off));
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            break;
        }
    }

    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &zero, sizeof(zero)) == -1) {
        return -1;
    }

    return 0;
}
```

What it teaches:

- `sendfile()` avoids copying file data through user-space for common file-serving paths.
- `TCP_CORK` is Linux-specific and should be disabled after the grouped output.
- Optimization code must still handle partial transfers and `EINTR`.

## Debugging

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| No write loop | Large response sometimes truncated | Use `write_all()`/`writen()` |
| Confusing EOF and error | Bad close handling | `read()==0` means EOF; `-1` means error |
| Writing after peer close | `SIGPIPE` or `EPIPE` | Ignore `SIGPIPE`, check write errors |
| Misusing `SHUT_RD` on TCP | Nonportable behavior | Prefer `SHUT_WR` for half-close |
| Restart hits `EADDRINUSE` | Port cannot bind | Set `SO_REUSEADDR` before `bind()` |
| Many `CLOSE_WAIT` sockets | App not closing after peer EOF | Fix connection cleanup path |
| Many unexpected `TIME_WAIT` sockets | Active closer is local side | Usually normal; inspect close behavior |
| Accepted fd is blocking unexpectedly | Listening fd was nonblocking but accepted fd is not on Linux | Set flags on accepted fd or use `accept4()` |
| Dead client pins worker | Thread/process stuck in blocking read/write | Add timeouts, nonblocking I/O, or request deadlines |
| Timeout fires but connection still leaks | Error path does not close fd or unregister event | Audit cleanup path with `strace` and fd count |
| Keepalive enabled but failure detected too late | Default probe timing is long | Tune TCP keepalive options or add app heartbeat |
| Keepalive traffic drains field device | Frequent probes on idle cellular/battery link | Increase intervals or use product-aware heartbeat |
| Reverse DNS in diagnostics | Logs stall under network issues | Use numeric `getnameinfo()` flags |

### Commands

```bash
# TCP states
ss -tan
ss -tan state time-wait
ss -tan state close-wait

# Listening sockets with processes
ss -ltnp

# Socket memory and options summary
ss -tinp
ss -o state established

# Legacy equivalent from TLPI-era systems
netstat -anp

# Packet-level handshake/FIN/RST/debug
sudo tcpdump -n -tt 'tcp port 9090'

# Trace advanced calls
strace -f -e trace=send,recv,sendfile,setsockopt,getsockopt,fcntl,shutdown,accept4 ./server

# Kernel socket tables
cat /proc/net/tcp
cat /proc/net/tcp6
cat /proc/net/unix

# Keepalive defaults on Linux
cat /proc/sys/net/ipv4/tcp_keepalive_time
cat /proc/sys/net/ipv4/tcp_keepalive_intvl
cat /proc/sys/net/ipv4/tcp_keepalive_probes
```

### Reading TCP Symptoms

| Observation | Likely meaning |
|-------------|----------------|
| `SYN_SENT` stuck | Cannot complete connection: firewall, routing, server not listening |
| Many `ESTABLISHED` idle | Long-lived clients or missing timeouts |
| Many `CLOSE_WAIT` | Peer closed; local app failed to close |
| Many `TIME_WAIT` | Local side actively closed; often normal |
| RST in `tcpdump` | Connection reset; peer/app/kernel rejected state |

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Request body then response | Client sends body, `shutdown(SHUT_WR)`, then reads response |
| Static file server | `sendfile()` after correctness is solid |
| Hot restart TCP daemon | `SO_REUSEADDR`, clean shutdown, observable states |
| Local privileged broker | UNIX socket + `sendmsg()` fd passing |
| Slow-client resistant server | Nonblocking I/O or socket timeouts plus request/idle deadlines |
| Embedded command channel | Short deadlines, bounded buffers, optional keepalive only if probe timing is acceptable |
| High-rate UDP receiver | Batching APIs may matter after profiling |
| Production incident | Combine `ss`, `strace`, logs, and `tcpdump` |

## Interview Readiness

1. Why can partial reads occur on stream sockets?
2. Why can partial writes occur, and how do you handle them?
3. What does `read()` returning `0` mean on a stream socket?
4. What happens when writing to a closed stream socket?
5. Compare `close()` and `shutdown()`.
6. When would you use `shutdown(SHUT_WR)`?
7. Why should portable TCP code avoid relying on `SHUT_RD`?
8. What does `MSG_PEEK` do?
9. Why use `MSG_NOSIGNAL` or ignore `SIGPIPE`?
10. What problem does `sendfile()` solve?
11. What does `TCP_CORK` try to optimize?
12. What are `getsockname()` and `getpeername()` used for?
13. Why does `TIME_WAIT` exist?
14. Why is disabling `TIME_WAIT` a bad instinct?
15. How does `SO_REUSEADDR` help TCP server restart?
16. Which attributes are not inherited by accepted sockets on Linux?
17. What does many `CLOSE_WAIT` sockets suggest?
18. What can `tcpdump` show that application logs cannot?
19. What are `sendmsg()` and `recvmsg()` used for?
20. Why might UDP be chosen over TCP despite unreliability?
21. When would you use socket timeouts instead of nonblocking I/O?
22. Why is TCP keepalive not the same as an application heartbeat?
23. What is the difference between `SO_RCVTIMEO` and an application deadline?

## Key Takeaways

1. Short reads and writes are normal on stream sockets.
2. Application protocols need framing and exact I/O loops.
3. `read()==0` means peer EOF after buffered bytes are consumed.
4. Closed peers produce `SIGPIPE`/`EPIPE` on write paths.
5. `shutdown(SHUT_WR)` is the practical TCP half-close.
6. `close()` affects fd references; `shutdown()` affects socket communication directions.
7. `TIME_WAIT` protects TCP correctness; do not fight it blindly.
8. `SO_REUSEADDR` belongs before `bind()`.
9. Accepted socket flag inheritance differs across systems; set what you require.
10. Socket timeouts protect syscalls; protocol deadlines protect services.
11. TCP keepalive can detect long-idle dead peers, but it is not protocol health.
12. `sendfile()` and `TCP_CORK` are optimization tools, not core correctness.
13. `sendmsg()`/`recvmsg()` matter for fd passing and ancillary data.
14. Debug advanced socket issues with state (`ss`), syscalls (`strace`), and packets (`tcpdump`).
