# Chapter 8 Interview - Sockets & Networking

> Scope: Linux socket API fundamentals, UNIX domain sockets, TCP/IP and Internet sockets, server design, partial I/O, message framing, socket options, and production debugging.
> Interview style: scenario-first. API names and flags are drill-down keywords, not the main headline.

---

## Review Basis

This file was reviewed against the Chapter 8 learning map and the mapped repo sources:

- Knowledge files: `ch08_socket_overview.md`, `ch08_socket_unix.md`, `ch08_socket_tcp.md`, `ch08_socket_server.md`, `ch08_socket_advanced.md`.
- TLPI-derived docs: Chapter 56 Sockets Introduction, Chapter 57 UNIX Domain Sockets, Chapter 58 TCP/IP Fundamentals, Chapter 59 Internet Domain Sockets, Chapter 60 Socket Server Design, Chapter 61 Advanced Socket Topics.
- DevLinux docs: `INDEX.md`, `README.md`, `06-IPC-Socket/README.md`, and the chat mini-project in `07-Mini-Project-Chat-Application/`.
- Linux man-pages for API semantics: `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `connect(2)`, `send(2)`, `recv(2)`, `sendmsg(2)`, `recvmsg(2)`, `shutdown(2)`, `getsockopt(2)`, `setsockopt(2)`, `socketpair(2)`, `unix(7)`, `ip(7)`, `tcp(7)`, `udp(7)`, `getaddrinfo(3)`, `getnameinfo(3)`, `inet_pton(3)`, `inet_ntop(3)`, `select(2)`, `poll(2)`, and `epoll(7)`.

External calibration sources were used only to prioritize interview style and topic likelihood:

- Amazon official SDE prep lists operating systems and internet topics: <https://amazon.jobs/en/landing_pages/software-development-topics>
- Microsoft official technical interviewing page emphasizes problem solving, technical principles, system design, and computer networking: <https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html>
- Google Careers hiring/process and SWE postings emphasize broad technical skill, large-scale systems, networking, debugging, and software design: <https://www.google.com/about/careers/applications/how-we-hire/> and <https://www.google.com/about/careers/applications/jobs/results/>
- Meta official SWE full-loop prep page emphasizes technical skills, communication, and prepared engineering discussion: <https://www.metacareers.com/careers/SWE-prep-onsite>
- Linux man-pages from man7.org were used as technical authority, including `socket(7)`, `unix(7)`, `getaddrinfo(3)`, `send(2)`, and `setsockopt(2)`: <https://man7.org/linux/man-pages/>
- Recurring interview banks and public Q&A were used only as topic signal: common repeats are TCP vs UDP, TCP message boundaries, partial `send()`/`recv()`, `TIME_WAIT`, `SO_REUSEADDR`, `SIGPIPE`, and server debug workflow.

Correctness priority: repo knowledge, TLPI-derived docs, and Linux man-pages.
Interview priority: company prep pages and recurring real-world networking questions.
Production framing: Linux debugging behavior, DevLinux project code, and common backend/embedded failure modes.

---

## Coverage Trace

| Coverage item | Priority coverage |
|---------------|-------------------|
| 8.1 socket lifecycle: endpoint, fd, domain/type/protocol, `bind()`/`listen()`/`accept()`/`connect()` | A1, A4, A5, A12, B14, B30, B33 |
| 8.2 UNIX domain sockets: local IPC, pathname/abstract namespace, `socketpair()`, credentials, fd passing | A8, A13, B16, B22, B27, C |
| 8.3 TCP/IP fundamentals: layers, TCP stream, UDP datagram, byte order, ports, addresses, DNS | A2, A7, A11, A12, B15, B24, B32 |
| 8.4 Internet sockets: `sockaddr_in`, `sockaddr_in6`, `getaddrinfo()`, dual-stack portability | A6, A7, B20, B31 |
| 8.5 server design: iterative, fork/thread, prefork/prethread, event loop, backpressure, overload | A1, A9, B23, B28 |
| 8.6 advanced sockets: partial I/O, `shutdown()`, `sendmsg()`/`recvmsg()`, options, nonblocking, timeout, keepalive | A3, A10, A13, B19, B21, B25, B26, B29, B33 |
| Production debugging: `ss`, `tcpdump`, `strace`, logs, states, `/proc` | A1, A4, A5, A12 |
| Embedded constraints: fd/RAM limits, bounded buffers, watchdog, cellular/NAT, DNS/boot network absence, power cost | A9, A11, B15, B29 |
| Must Cover: socket model and lifecycle | A1, A4, A6, B14, B30, B33 |
| Must Cover: UNIX domain local IPC, pathname/abstract namespace, credentials, `socketpair()`, fd passing | A8, A13, B16, B22, B27, C |
| Must Cover: TCP/IP layers, byte order, TCP stream, UDP datagram, ports, addresses, DNS | A2, A7, A11, A12, B15, B24, B32 |
| Must Cover: Internet sockets, `sockaddr_in`, `sockaddr_in6`, `getaddrinfo()`, IPv4/IPv6 portability | A6, A7, B20, B31 |
| Must Cover: server designs, event loop recognition, backpressure, overload | A1, A9, B23, B28 |
| Must Cover: advanced APIs/options/debugging: ancillary data, `SO_REUSEADDR`, nonblocking, timeouts, keepalive, `ss`/`tcpdump`/`strace`/logs | A3, A5, A10, A12, A13, B17, B19, B21, B26, B29, B33, C |

No Chapter 8 mapped row or Must Cover concept is intentionally out of scope. Detailed event-loop APIs remain Chapter 9 material and appear here only as server-design recognition.

---

## Priority Map

### A - Project and production scenarios

Study these deeply. A strong answer should explain mechanism, failure mode, debug workflow, and design trade-offs.

1. A TCP service works in a demo but fails with multiple clients.
2. A chat/file protocol over TCP randomly merges or splits messages.
3. A server occasionally truncates large responses or receives partial requests.
4. A disconnected client sometimes kills the server or leaves broken connections.
5. A restarted daemon fails with `Address already in use`.
6. A service works on `localhost` but not from another host, container, or board.
7. A client must support IPv4, IPv6, DNS names, numeric addresses, and changing networks.
8. A same-host daemon needs a secure local control channel.
9. A server must handle many slow clients without exhausting RAM, fds, or threads.
10. A request/response protocol hangs because both sides wait for EOF.
11. A UDP telemetry or discovery design must tolerate loss and size limits.
12. A production socket outage must be debugged from symptoms, not guesses.
13. A privileged broker must hand an accepted connection or device fd to an unprivileged worker.

### B - Design comparisons and senior follow-ups

Know the trade-off and the production use case. Do not memorize every flag.

- UNIX socket vs TCP loopback.
- Iterative vs fork-per-client vs thread-per-client vs prefork/prethread vs event loop.
- `SO_REUSEADDR` vs `SO_REUSEPORT`.
- `close()` vs `shutdown()`.
- `read()`/`write()` vs `send()`/`recv()`.
- `accept()` vs `accept4()`.
- `inet_pton()` vs `getaddrinfo()`.
- `getsockname()` vs `getpeername()`.
- `socketpair()` vs pipe.
- `sendmsg()`/`recvmsg()` and ancillary data.
- `SO_PEERCRED` vs `SCM_CREDENTIALS`.
- Socket timeouts vs application deadlines vs TCP keepalive.
- `inetd`/superserver flow vs standalone daemon.
- Domain/address family vs socket type vs protocol.
- `sockaddr_in` vs `sockaddr_in6` vs `sockaddr_storage`.
- Network byte order and protocol marshalling vs raw C structs.
- `read()`/`write()` vs `send()`/`recv()` vs `sendto()`/`recvfrom()`.
- `TCP_NODELAY`, `TCP_CORK`, `MSG_MORE`, and Nagle-level recognition.
- `select()`, `poll()`, and `epoll()` at server-design recognition level.

### C - Lower-priority / know enough to recognize

Recognize these terms and know when to read the manual. Do not spend first-pass time memorizing every field or option.

- Raw sockets, packet sockets, SCTP, DCCP, and out-of-band TCP data.
- Full TCP header fields and every TCP state transition.
- Every `getaddrinfo()` flag.
- Every socket option in `socket(7)`, `tcp(7)`, `udp(7)`, `ip(7)`, and `unix(7)`.
- Linux abstract UNIX sockets beyond basic behavior.
- `recvmmsg()` and `sendmmsg()` batching APIs.
- Detailed `inetd` configuration beyond wait/nowait and fd setup.
- BPF socket filters.

---

## Final Interview List

### Priority A

1. Your TCP server handles one client in testing, but in production slow clients block other clients. How do you redesign and debug it?
2. Your chat protocol sometimes receives two messages as one or half a message. What is wrong with the design?
3. A file transfer over TCP sometimes truncates data even though `send()` returned success. What bug do you suspect?
4. A client disconnect causes the server to terminate or leak sockets. What mechanisms explain this?
5. After restart, the daemon fails at `bind()` with `EADDRINUSE`. How do you distinguish `TIME_WAIT`, stale processes, and fd leaks?
6. The service works with `curl localhost` but is unreachable from another machine or container. What do you check?
7. You are writing a client library for hostnames, IPv4, IPv6, and numeric addresses. How should address resolution be designed?
8. A local admin CLI must talk to a privileged daemon on the same Linux host. Would you use UNIX sockets or TCP loopback?
9. A socket server must run on an embedded gateway with limited RAM and fd limits. Which concurrency model do you choose?
10. A request body is sent to a TCP server, but both sides hang waiting for each other. When does `shutdown(SHUT_WR)` help?
11. You need device discovery or telemetry over UDP. What reliability and packet-size assumptions must be explicit?
12. Production says "network is down", but you only have shell access. How do you isolate DNS, routing, firewall, bind, app, and protocol bugs?
13. A root broker accepts TCP clients or opens device files, then must hand work to an unprivileged process. How do UNIX sockets, fd passing, and credentials fit?

### Priority B

14. Compare listening sockets and connected sockets.
15. Compare TCP and UDP for backend and embedded systems.
16. Compare UNIX stream, UNIX datagram, TCP, and UDP semantics.
17. Compare `SO_REUSEADDR` and `SO_REUSEPORT`.
18. What is a connected UDP socket?
19. What are accepted-fd inheritance rules on Linux?
20. When do `getsockname()` and `getpeername()` matter?
21. When are `sendmsg()` and `recvmsg()` worth using?
22. What problem does file descriptor passing solve?
23. How do `select()`, `poll()`, and `epoll()` fit into server design?
24. Why can reverse DNS become a latency bug?
25. What are `CLOSE_WAIT`, `TIME_WAIT`, and `SYN_SENT` telling you?
26. How do `setsockopt()` and `getsockopt()` fit into production socket code?
27. Compare `SO_PEERCRED` and `SCM_CREDENTIALS`.
28. What is `inetd`, and how is an inetd-style service different from a standalone daemon?
29. Compare socket timeouts, application deadlines, and TCP keepalive.
30. Compare domain/address family, socket type, and protocol.
31. How do `sockaddr_in`, `sockaddr_in6`, and `sockaddr_storage` affect IPv4/IPv6 portability?
32. Why should a network protocol avoid raw C structs?
33. Compare `read()`/`write()`, `send()`/`recv()`, and `sendto()`/`recvfrom()`.

### Priority C

- Recognize raw/packet sockets, SCTP, DCCP, out-of-band data, `SOCK_SEQPACKET`, `inetd`, batching APIs, and advanced socket options.

---

## High-Value Comparisons

| Comparison | Strong interview answer |
|------------|-------------------------|
| Socket vs file | Both are represented by fds, but a socket is a kernel communication endpoint with domain/type/protocol, buffers, peer state, and socket options. |
| Domain vs type vs protocol | Domain chooses address family and scope; type chooses stream/datagram/message semantics; protocol chooses the concrete transport, usually selected with `0`. |
| Listening socket vs connected socket | The listening socket accepts new stream connections. `accept()` returns a separate connected fd for per-client I/O. |
| TCP vs UDP | TCP is a reliable ordered byte stream with flow and congestion control. UDP is best-effort datagrams with lower setup overhead and multicast/broadcast use cases. |
| Stream vs datagram | Stream has no message boundaries. Datagram preserves message boundaries, but Internet datagrams may be lost, duplicated, reordered, or truncated. |
| `INADDR_ANY` vs loopback | Wildcard bind accepts traffic on all matching local interfaces. Loopback accepts only same-host traffic. |
| `inet_pton()` vs `getaddrinfo()` | `inet_pton()` parses numeric IP strings. `getaddrinfo()` resolves host/service names and returns IPv4/IPv6-ready addresses. |
| UNIX socket vs TCP loopback | UNIX sockets are same-host IPC with permissions, credentials, and fd passing. TCP loopback uses IP+port and mirrors network code. |
| `socketpair()` vs pipe | `socketpair()` creates a bidirectional connected pair. A pipe is normally unidirectional unless two pipes are used. |
| `close()` vs `shutdown()` | `close()` closes one fd reference. `shutdown()` disables socket communication directions even if duplicated fds exist. |
| Process per client vs thread per client | Processes isolate better and cost more. Threads are lighter but share address space and need synchronization. |
| Prefork/prethread vs event loop | Pools bound resource use and creation cost. Event loops scale many idle fds but require nonblocking state-machine code. |
| `SO_REUSEADDR` vs `SO_REUSEPORT` | `SO_REUSEADDR` helps normal TCP server rebinding/restart cases. `SO_REUSEPORT` is Linux-specific load distribution among sockets that all opt in; use it deliberately because accept distribution and rolling restart behavior are part of the design. |
| `TIME_WAIT` vs `CLOSE_WAIT` | `TIME_WAIT` is normal TCP cleanup after active close. `CLOSE_WAIT` often means the peer closed and the local app forgot to close. |

---

## Common Project Failure Patterns

| Failure pattern | Production symptom | Interview-grade fix |
|-----------------|--------------------|---------------------|
| Assuming TCP preserves messages | Random parse errors, combined commands, half commands | Add explicit framing and parser state. |
| Ignoring partial sends | Truncated responses under load | Use `write_all()`/send loop and handle `EINTR`, `EAGAIN`, and `EPIPE`. |
| Treating `recv() == 0` as retry | Busy loop or stuck connection | Treat it as peer EOF after buffered data is consumed. |
| Leaking accepted sockets | Growing fd count, `EMFILE`, many `CLOSE_WAIT` | Define ownership and close every fd in all paths. |
| Parent keeps connected fd after `fork()` | Client never sees EOF | Parent closes connected fd; child closes listening fd. |
| No concurrency plan | One slow TCP client blocks all clients | Use bounded workers or an event loop. |
| Binding to `127.0.0.1` accidentally | Works locally, fails remotely | Bind to the intended address and verify with `ss -ltnp`. |
| Binding to `0.0.0.0` accidentally | Service exposed on unintended interfaces | Bind to loopback or a specific interface when required. |
| IPv4-only assumptions | Fails when DNS returns IPv6 first | Use `getaddrinfo(AF_UNSPEC)` and try all results. |
| Misunderstanding `SO_REUSEADDR` | Restart still fails or unsafe port-sharing assumptions | Set it before `bind()` and still investigate live owners. |
| No `SIGPIPE` policy | Server exits when peer disconnects | Ignore `SIGPIPE` or use `MSG_NOSIGNAL`, then handle `EPIPE`. |
| Blocking DNS or reverse DNS in hot path | Latency spikes during incidents | Use numeric logging and isolate resolver failures. |
| Container/network namespace mismatch | Host and container see different listeners/routes | Check inside the namespace/container, not only on host. |
| No embedded resource budget | Fork/thread storm, fd exhaustion, watchdog resets | Bound workers, set timeouts, track fd/thread counts, and fail gracefully. |

---

## Detailed Answers - Priority A

### 1. Your TCP server handles one client in testing, but in production slow clients block other clients. How do you redesign and debug it?

**What the interviewer is testing**

They are testing whether you understand the difference between socket API correctness and server design correctness. A server can call `socket()`, `bind()`, `listen()`, and `accept()` correctly and still fail because one blocking client monopolizes the accept loop or worker.

**Strong answer**

I would first identify the workload. For tiny bounded UDP request/reply, iterative may be fine. For TCP sessions, file transfers, chat, shell-like protocols, or anything with slow clients, I would use concurrency: fork-per-client for simple isolation, thread-per-client for moderate connection counts, prefork/prethread for bounded capacity, or an event loop for many idle connections. On embedded Linux I would avoid unbounded fork/thread creation and choose a small worker pool or event loop with explicit limits.

**Mechanism**

`listen()` creates a passive listening socket and a finite pending-connection queue. `accept()` returns a new connected fd, while the listening fd remains open. If the process handles the connected fd synchronously and blocks in `read()`, `write()`, DNS, disk I/O, or application logic, it may stop accepting new connections. Backlog absorbs only a burst; it is not a worker pool.

**Pitfalls**

Common failures include unlimited fork/thread creation, no timeout for slow clients, parent leaking accepted fds, children keeping the listening fd, no `SIGCHLD` reaping, and assuming backlog fixes application starvation.

**Debug angle**

Use `ss -ltnp` for listening sockets, `ss -tan state established`, `ss -tan state close-wait`, `ls /proc/<pid>/fd | wc -l`, `cat /proc/<pid>/limits`, `ps -L -p <pid>`, and `strace -f -e trace=accept,clone,fork,read,write,close`. Check logs for accept latency and worker saturation.

**Follow-up keywords**

`accept()`, backlog, `RLIMIT_NOFILE`, fork-per-client, thread-per-client, prefork, prethread, `select()`, `poll()`, `epoll`, `SIGCHLD`, `waitpid(WNOHANG)`.

### 2. Your chat protocol sometimes receives two messages as one or half a message. What is wrong with the design?

**What the interviewer is testing**

They want to hear that TCP is a byte stream, not a message transport.

**Strong answer**

The application is assuming that one `send()` equals one `recv()`. TCP preserves byte order, but it does not preserve application message boundaries. I would define framing: newline-delimited commands for text protocols, fixed headers plus length, length-prefixed binary frames, or an existing protocol format.

**Mechanism**

The TCP stack may split, coalesce, retransmit, buffer, and deliver bytes independently from application write calls. A receiver must parse from a stream buffer and only process complete frames.

**Pitfalls**

The bug often passes localhost tests, then fails under TLS, Nagle/delayed ACK behavior, buffering, high latency, slow readers, or large messages. It also appears in simple chat projects that call `recv()` into a fixed buffer and print whatever arrived as a complete message.

**Debug angle**

Log parsed frame boundaries, not just raw buffers. Reproduce with small writes, large writes, delayed writes, and multiple messages back-to-back. Use `strace -e trace=send,recv` to see syscall sizes and `tcpdump -n` to avoid confusing packet segmentation with application framing.

**Follow-up keywords**

Byte stream, message framing, length prefix, delimiter, fixed header, parser state, `MSG_PEEK`, `readn()`, `writen()`.

### 3. A file transfer over TCP sometimes truncates data even though `send()` returned success. What bug do you suspect?

**What the interviewer is testing**

They are checking partial I/O discipline and error handling.

**Strong answer**

I suspect the code treats a successful short `send()` or `write()` as if the whole buffer was transferred. The return value is the number of bytes accepted, not a guarantee that all requested bytes moved. The sender needs a loop until all bytes are sent or an unrecoverable error occurs. The receiver must also loop according to the protocol frame length or EOF.

**Mechanism**

Partial writes can happen because the socket send buffer has limited space, a signal interrupts the call after some bytes, the socket is nonblocking, or an asynchronous connection error occurs. Partial reads happen whenever fewer bytes are currently available than requested.

**Pitfalls**

Do not use `strlen()` for binary payloads. Do not assume `MSG_WAITALL` removes all short-return cases. Do not ignore `EINTR`, `EAGAIN`, `EWOULDBLOCK`, `EPIPE`, or `ECONNRESET`.

**Debug angle**

Instrument requested length versus returned length. Test with payloads larger than socket buffers, slow receivers, nonblocking mode, and signal interruptions. Trace `send`, `recv`, `write`, and `read` with `strace`.

**Follow-up keywords**

Partial read, partial write, `send()`, `recv()`, `EINTR`, `EAGAIN`, `EWOULDBLOCK`, `MSG_WAITALL`, socket buffers, `SO_SNDBUF`, `SO_RCVBUF`.

### 4. A client disconnect causes the server to terminate or leak sockets. What mechanisms explain this?

**What the interviewer is testing**

They are testing closed-peer behavior, EOF handling, `SIGPIPE`, and fd lifecycle.

**Strong answer**

When the peer closes its write side, local reads eventually return `0` after buffered data is consumed. When the server writes to a stream socket whose peer has closed, Linux can deliver `SIGPIPE` and the call fails with `EPIPE`. A robust server ignores or suppresses `SIGPIPE`, checks write errors, and closes the connection fd in every cleanup path.

**Mechanism**

The socket is full duplex. EOF in one direction does not automatically mean the local fd is closed. `close()` drops one fd reference; duplicated or inherited references can keep the connection alive. After `fork()`, parent and child must close fds they do not own.

**Pitfalls**

Default `SIGPIPE` can terminate the process. Ignoring `SIGPIPE` but ignoring `EPIPE` creates leaks. Treating `recv() == 0` as "no data yet" creates busy loops. In threaded code, stale connection table entries can outlive the fd.

**Debug angle**

Use `strace -f -e trace=send,recv,read,write,close` and look for `EPIPE`, `ECONNRESET`, and missing `close()`. Use `ss -tanp` for `CLOSE_WAIT` and `/proc/<pid>/fd` for leaks.

**Follow-up keywords**

`SIGPIPE`, `EPIPE`, `ECONNRESET`, `recv() == 0`, `close()`, fd reference, `fork()`, `MSG_NOSIGNAL`.

### 5. After restart, the daemon fails at `bind()` with `EADDRINUSE`. How do you distinguish `TIME_WAIT`, stale processes, and fd leaks?

**What the interviewer is testing**

They want a production debug path, not just "set `SO_REUSEADDR`".

**Strong answer**

I would first check whether a process is still listening or holding the port. If not, I would look for TCP endpoints in `TIME_WAIT` or children that still own accepted sockets. A TCP server should normally set `SO_REUSEADDR` before `bind()`, but that does not replace understanding who owns the address.

**Mechanism**

A connected TCP socket is identified by local IP, local port, remote IP, and remote port. The active closer enters `TIME_WAIT` so old duplicate segments expire and final ACKs can be retransmitted. `SO_REUSEADDR` relaxes normal rebinding constraints for common server restart cases. It does not mean two unrelated servers can safely own the same listener.

**Pitfalls**

Setting `SO_REUSEADDR` after `bind()` is too late. Killing `TIME_WAIT` or blindly changing kernel tunables is usually the wrong instinct. UNIX domain sockets have a different `EADDRINUSE` cause: a stale pathname.

**Debug angle**

Use `ss -ltnp 'sport = :<port>'`, `ss -tan state time-wait 'sport = :<port>'`, `lsof -iTCP:<port> -nP`, `/proc/<pid>/fd`, and `strace -e trace=setsockopt,bind`. For UNIX sockets, use `ss -xap`, `stat`, and controlled `unlink()` of paths the service owns.

**Follow-up keywords**

`SO_REUSEADDR`, `SO_REUSEPORT`, `TIME_WAIT`, `EADDRINUSE`, 4-tuple, `bind()`, `setsockopt()`, stale UNIX socket path.

### 6. The service works with `curl localhost` but is unreachable from another machine or container. What do you check?

**What the interviewer is testing**

They are testing address binding, IPv4/IPv6, namespace, routing, and firewall debugging.

**Strong answer**

I would separate bind-address bugs from network-path bugs. First verify what address and protocol the service is actually listening on. `127.0.0.1` and `::1` are local-only. `0.0.0.0` and `::` are wildcard binds. Then I would test from the same namespace, the host, and the remote peer, checking DNS, route, firewall, NAT, and container port mapping.

**Mechanism**

An Internet socket address is IP address plus port. Binding to loopback receives only loopback traffic. Binding to wildcard receives traffic on all matching local interfaces. Containers and network namespaces have their own interfaces, routes, and listeners.

**Pitfalls**

IPv6-only or IPv4-only listeners can confuse clients. Host `localhost` may resolve to `::1` before `127.0.0.1`. A service bound to `0.0.0.0` may be exposed more broadly than intended. A container publishing rule may be missing even though the app listens correctly inside the container.

**Debug angle**

Use `ss -ltnup`, `ip addr`, `ip route`, `ip route get <peer>`, `curl -v`, `nc -vz`, `getent hosts`, `dig`, `ping`, firewall tooling, container inspect commands, `journalctl`, and `tcpdump -n 'port <port>'`.

**Follow-up keywords**

Loopback, wildcard bind, specific interface bind, `INADDR_ANY`, `in6addr_any`, IPv4/IPv6 mismatch, network namespace, NAT, firewall.

### 7. You are writing a client library for hostnames, IPv4, IPv6, and numeric addresses. How should address resolution be designed?

**What the interviewer is testing**

They are testing modern Internet socket code and portability.

**Strong answer**

I would use `getaddrinfo()` with clear hints, usually `AF_UNSPEC` plus the needed socket type. The client should iterate through all returned addresses, creating a socket and trying `connect()` for each until one succeeds. It should call `freeaddrinfo()` and use `gai_strerror()` for errors. I would use `inet_pton()` only when I specifically require a numeric IP literal.

**Mechanism**

`getaddrinfo()` returns a linked list because a hostname can map to multiple IPv4/IPv6 addresses and a service can map to different socket types. For servers, `AI_PASSIVE` with a null host returns wildcard bind addresses. For hot-path logging, `getnameinfo()` with numeric flags avoids reverse DNS.

**Pitfalls**

Trying only the first result breaks on dual-stack systems. Using `gethostbyname()` bakes in legacy IPv4-style assumptions. Calling `perror()` for `getaddrinfo()` errors is wrong unless the error is `EAI_SYSTEM`.

**Debug angle**

Use `getent hosts <name>`, `dig A`, `dig AAAA`, `strace -e trace=network,connect,sendto,recvfrom`, and logs that include numeric host and service. Check `/etc/hosts`, `/etc/resolv.conf`, and DNS timeout behavior.

**Follow-up keywords**

`getaddrinfo()`, `getnameinfo()`, `gai_strerror()`, `freeaddrinfo()`, `AF_UNSPEC`, `AI_PASSIVE`, `AI_NUMERICHOST`, `AI_NUMERICSERV`, `sockaddr_storage`.

### 8. A local admin CLI must talk to a privileged daemon on the same Linux host. Would you use UNIX sockets or TCP loopback?

**What the interviewer is testing**

They are testing domain choice, local security, and daemon design.

**Strong answer**

I would usually choose a UNIX domain socket under an owned runtime directory such as `/run/mydaemon/control.sock`. It is same-host only, avoids IP exposure, supports filesystem permissions and peer credentials, and can pass fds if the daemon uses privilege separation. TCP loopback is reasonable when I want the same protocol to work across hosts or with existing network tooling.

**Mechanism**

Pathname UNIX sockets use the filesystem namespace as a rendezvous point. `bind()` creates a socket file; the path normally remains until `unlink()`. Linux also has abstract UNIX sockets, but they are Linux-specific and not visible in the filesystem. A daemon can authenticate the connected peer with `SO_PEERCRED`, or receive explicit credentials with `SCM_CREDENTIALS` when enabled. For privilege-separated designs, `sendmsg()` can carry `SCM_RIGHTS` ancillary data so the receiver gets a new fd referring to the same open file description.

**Pitfalls**

Do not place predictable production sockets in `/tmp` without a private directory. Do not rely only on socket-file permissions for portability; directory permissions are the better boundary. UNIX stream sockets are still byte streams and need framing. Do not trust client-supplied identity strings when kernel peer credentials are available. Remember that a received fd must be closed on every error path.

**Debug angle**

Use `ss -xap`, `cat /proc/net/unix`, `lsof -U`, `ls -l /run/mydaemon/control.sock`, `stat`, and `strace -e trace=socket,bind,connect,sendmsg,recvmsg,unlink`.

**Follow-up keywords**

`AF_UNIX`, `sockaddr_un`, pathname socket, abstract namespace, `SO_PEERCRED`, `SCM_RIGHTS`, `socketpair()`, `unlink()`.

### 9. A socket server must run on an embedded gateway with limited RAM and fd limits. Which concurrency model do you choose?

**What the interviewer is testing**

They want resource-aware design, not generic "use threads" advice.

**Strong answer**

I would bound resources first: maximum clients, fd budget, worker count, per-client buffers, timeouts, reconnect behavior, and backpressure. For a few management clients, thread-per-client can be acceptable if capped. For many idle telemetry clients, an event loop with nonblocking sockets is usually better. For strong isolation, a small prefork pool may be worth the cost. I would avoid unbounded fork-per-client on constrained devices.

**Mechanism**

Every connection consumes at least one fd and kernel socket buffers. Threads consume stacks and scheduler overhead. Processes consume more memory but isolate faults. Event loops reduce per-connection execution context but require explicit state machines and careful nonblocking I/O.

**Pitfalls**

Large default thread stacks can waste RAM. Logging every socket error can fill flash or slow the system. No timeout lets one peer hold a scarce worker forever. No `FD_CLOEXEC` can leak sockets into helper programs. Field devices also face boot-time network absence, DNS outages, cellular/NAT reconnects, watchdog restarts, and keepalive probes that waste power if tuned blindly.

**Debug angle**

Track `RLIMIT_NOFILE`, `/proc/<pid>/fd`, thread count, memory, socket states, DNS/connect failures, reconnect loops, and watchdog resets. Use `ss -s`, `ss -tanp`, `pmap`, `top`, `journalctl`, and application counters for active clients, dropped connections, timeout reasons, and retry backoff.

**Follow-up keywords**

Embedded Linux, worker pool, event loop, `epoll`, nonblocking I/O, fd limit, socket buffer, timeout, watchdog, DNS outage, NAT reconnect, keepalive power cost, `SOCK_CLOEXEC`, `FD_CLOEXEC`.

### 10. A request body is sent to a TCP server, but both sides hang waiting for each other. When does `shutdown(SHUT_WR)` help?

**What the interviewer is testing**

They are testing half-close semantics and protocol termination.

**Strong answer**

If the protocol uses EOF to mark the end of the request body but the client still needs to read the response, the client should call `shutdown(fd, SHUT_WR)`. That sends EOF to the server's read side while keeping the client's read side open for the response. The client still calls `close()` when fully done.

**Mechanism**

`close()` closes one fd reference. If duplicated fds exist, the connection may stay open. `shutdown()` acts on the socket communication direction. `SHUT_WR` starts TCP write-side termination and lets the peer eventually read `0`.

**Pitfalls**

A protocol that relies on EOF for message boundaries cannot keep the connection open for multiple requests unless it has another framing scheme. `SHUT_RD` is not portable for TCP behavior. `shutdown()` is not a replacement for `close()`.

**Debug angle**

Use `strace -e trace=read,write,shutdown,close` to see whether EOF is ever signaled. Use `tcpdump` to observe FIN direction. Use `ss -tanp` to inspect `FIN_WAIT`, `CLOSE_WAIT`, and stuck `ESTABLISHED`.

**Follow-up keywords**

`shutdown()`, `SHUT_WR`, half-close, EOF, full-duplex TCP, protocol framing, `close()` references.

### 11. You need device discovery or telemetry over UDP. What reliability and packet-size assumptions must be explicit?

**What the interviewer is testing**

They want to see that you do not treat UDP as "TCP but faster".

**Strong answer**

UDP is useful for small independent messages, discovery, broadcast/multicast, DNS-like request/reply, and telemetry where loss is acceptable. The protocol must define maximum message size, duplicate handling, ordering expectations, retries, timeouts, sequence numbers if needed, and what happens when data is lost.

**Mechanism**

UDP preserves datagram boundaries but does not guarantee delivery, order, or duplicate suppression. IP fragmentation makes large datagrams fragile because losing one fragment loses the whole datagram. UDP does not provide TCP's connection setup, retransmission, flow control, or congestion control.

**Pitfalls**

Large datagrams may be silently lost or truncated. If the design needs reliable ordered delivery, flow control, and congestion control, TCP is usually the better starting point. A UDP receiver must size buffers and detect truncation when the API supports it.

**Debug angle**

Use `ss -lunp`, `tcpdump -n udp port <port>`, packet counters, app sequence numbers, and loss tests. Check MTU with `ip link`, route with `ip route get`, and firewall rules for UDP separately from TCP.

**Follow-up keywords**

UDP, datagram boundary, MTU, fragmentation, broadcast, multicast, retry, duplicate detection, `recvfrom()`, `sendto()`, `MSG_TRUNC`.

### 12. Production says "network is down", but you only have shell access. How do you isolate DNS, routing, firewall, bind, app, and protocol bugs?

**What the interviewer is testing**

They are testing operational debugging sequence and avoiding premature conclusions.

**Strong answer**

I would debug by layers. First confirm the process and listener. Then confirm local address and namespace. Then test name resolution, route, firewall/NAT, TCP handshake or UDP packet flow, syscall behavior, and finally application protocol parsing. I would compare localhost, host IP, container IP, and remote client behavior.

**Mechanism**

Socket failures can look similar at the application level: refused, timeout, reset, EOF, partial data, or hang. The underlying cause may be no listener, wrong bind address, DNS returning the wrong family, no route, firewall drop, accept queue saturation, fd exhaustion, or protocol deadlock.

**Pitfalls**

`Connection refused` usually means a reachable host actively rejected the connection or no listener exists on that address/port. Timeout often points to packet drop, route, firewall, or backlog/SYN issues. An established TCP connection with application hang often points to framing, EOF, or worker blockage.

**Debug angle**

Use `ss`, `lsof`, `/proc/<pid>/fd`, `/proc/net/tcp`, `strace`, `tcpdump`, `curl`, `nc`, `telnet`, `ip addr`, `ip route`, `ping`, `dig`, `getent hosts`, `systemctl`, and `journalctl`. For containers, run equivalent checks inside the container or namespace.

**Follow-up keywords**

`ECONNREFUSED`, timeout, RST, SYN, SYN/ACK, FIN, DNS, route, firewall, namespace, `/proc/net/tcp`, `tcpdump`, `strace`.

### 13. A root broker accepts TCP clients or opens device files, then must hand work to an unprivileged process. How do UNIX sockets, fd passing, and credentials fit?

**What the interviewer is testing**

They are testing whether you can connect UNIX-domain sockets, ancillary data, fd lifetime, and privilege separation into one practical design.

**Strong answer**

I would keep privileged operations in a small broker and pass already-open fds to less-privileged workers over a UNIX domain socket. The broker can authenticate local clients or workers with kernel credentials, then use `sendmsg()` with `SCM_RIGHTS` to transfer a reference to an accepted TCP connection, device fd, pipe, or file. The worker receives a new fd number that refers to the same open file description and becomes responsible for closing it.

**Mechanism**

`sendmsg()` and `recvmsg()` use `struct msghdr`. Normal bytes are described by `iovec`; ancillary/control data is carried in `msg_control` as one or more `struct cmsghdr` records. For fd passing, the control message uses `SOL_SOCKET` plus `SCM_RIGHTS`; code sizes the buffer with `CMSG_SPACE()` and fills length with `CMSG_LEN()`. For credentials, Linux commonly uses `SO_PEERCRED` on connected UNIX sockets, or `SO_PASSCRED` plus `SCM_CREDENTIALS` when receiving explicit credential messages.

**Pitfalls**

The fd number changes in the receiver; only the underlying open file description is shared. Both processes must close their copies when done. If `recvmsg()` succeeds but later validation fails, close the received fd before returning. Credential passing is Linux/UNIX-specific, and privileged processes can affect reported credentials in ways that must be understood before using them as an authorization boundary.

**Debug angle**

Trace `sendmsg`, `recvmsg`, `getsockopt`, and `close` with `strace -f`. Inspect UNIX sockets with `ss -xap` and `/proc/net/unix`, and count fds under `/proc/<pid>/fd`. Log numeric uid/gid/pid from credentials and fd ownership transitions.

**Follow-up keywords**

`AF_UNIX`, `sendmsg()`, `recvmsg()`, `msghdr`, `iovec`, `cmsghdr`, `CMSG_SPACE`, `CMSG_LEN`, `SCM_RIGHTS`, `SO_PEERCRED`, `SO_PASSCRED`, `SCM_CREDENTIALS`, open file description.

---

## Short Answers - Priority B

### 14. Compare listening sockets and connected sockets.

A listening socket is passive and used only to accept stream connections. `accept()` returns a new connected fd for one client; that connected fd is used for I/O. The listening fd remains open for future clients.

### 15. Compare TCP and UDP for backend and embedded systems.

TCP is the default when data must arrive reliably and in order. UDP is useful for bounded independent messages, discovery, broadcast/multicast, and real-time data where loss is acceptable or handled by the application.

### 16. Compare UNIX stream, UNIX datagram, TCP, and UDP semantics.

UNIX stream and TCP are byte streams without message boundaries. UNIX datagram and UDP preserve message boundaries. UDP is unreliable over networks; UNIX datagram sockets are local and reliable/ordered on Linux per TLPI, though queue limits still matter.

### 17. Compare `SO_REUSEADDR` and `SO_REUSEPORT`.

`SO_REUSEADDR` is commonly used before `bind()` so a TCP server can restart during normal `TIME_WAIT`/old-endpoint cases. `SO_REUSEPORT` allows multiple sockets to bind the same address and port for kernel distribution when all participants opt in. Treat `SO_REUSEPORT` as a Linux-specific server-design choice: distribution can surprise you, rolling restarts must be planned, and it is not a replacement for `SO_REUSEADDR` restart hygiene.

### 18. What is a connected UDP socket?

Calling `connect()` on a UDP socket records a default peer and filters inbound datagrams to that peer. It does not create a TCP-like session. It lets the process use `send()`/`write()` instead of `sendto()` for that peer.

### 19. What are accepted-fd inheritance rules on Linux?

On Linux, accepted sockets do not inherit `O_NONBLOCK`, `FD_CLOEXEC`, or signal-driven I/O ownership from the listening socket. They inherit many socket options. Use `accept4()` or explicitly set required flags.

### 20. When do `getsockname()` and `getpeername()` matter?

`getsockname()` returns the local address, useful after binding port `0` or inheriting a socket. `getpeername()` returns the connected peer, useful when the accepting process handed the fd to another process.

### 21. When are `sendmsg()` and `recvmsg()` worth using?

Use them for scatter/gather socket I/O, ancillary data, file descriptor passing, credentials, or detailed datagram metadata. The key objects are `struct msghdr`, `iovec` data buffers, `msg_control`, and `cmsghdr` control messages sized with `CMSG_SPACE()` and described with `CMSG_LEN()`. Most simple connected I/O can use `read()`/`write()` or `send()`/`recv()`.

### 22. What problem does file descriptor passing solve?

It lets one same-host process pass a reference to an open file description to another process over a UNIX domain socket. The receiver gets a new fd number, but it refers to the same underlying open file description, so ownership and cleanup must be explicit. This supports privilege separation, broker/worker designs, and parent-accepts-then-dispatches server pools.

### 23. How do `select()`, `poll()`, and `epoll()` fit into server design?

They let one process or a small thread group monitor many fds. `select()` and `poll()` are portable but less scalable for large fd sets. `epoll` is Linux-specific and commonly used for many concurrent sockets.

### 24. Why can reverse DNS become a latency bug?

`getnameinfo()` without numeric flags may block on resolver behavior. Doing reverse DNS in an accept or request hot path can turn DNS trouble into service latency. Use numeric host/service logging in hot paths.

### 25. What are `CLOSE_WAIT`, `TIME_WAIT`, and `SYN_SENT` telling you?

`CLOSE_WAIT` usually means the peer closed and the local app has not closed. `TIME_WAIT` means the local side actively closed and TCP is preserving correctness. `SYN_SENT` means a client sent SYN but has not completed the handshake.

### 26. How do `setsockopt()` and `getsockopt()` fit into production socket code?

Use `setsockopt()` to enable options before the lifecycle step they affect, such as `SO_REUSEADDR` before `bind()`. Use `getsockopt()` for diagnostics, inherited-fd inspection, socket type checks, peer credentials, and option verification. Keep options documented near the reason they are needed.

### 27. Compare `SO_PEERCRED` and `SCM_CREDENTIALS`.

`SO_PEERCRED` is a Linux `getsockopt()` path for getting pid/uid/gid of the peer on a connected UNIX domain socket. `SCM_CREDENTIALS` is ancillary data received through `recvmsg()` when credential passing is enabled, typically with `SO_PASSCRED`. Both are local UNIX-socket authentication tools, not portable network authentication. Treat credentials as kernel evidence, but still understand namespace, privilege, and capability caveats before using them as an authorization boundary.

### 28. What is `inetd`, and how is an inetd-style service different from a standalone daemon?

`inetd` is a legacy superserver. It owns listening sockets for configured services, waits with a multiplexing call, accepts or receives activity, forks, sets up the service socket on standard fds, optionally changes user/group, and execs the service program. A standalone daemon performs its own `socket()`/`bind()`/`listen()`/`accept()` loop, logging, concurrency, and reaping. Interview-level coverage is recognizing the flow, `wait`/`nowait`, and why old services may read from `STDIN_FILENO` instead of calling `accept()`.

### 29. Compare socket timeouts, application deadlines, and TCP keepalive.

`SO_RCVTIMEO` and `SO_SNDTIMEO` bound individual blocking socket calls. Application deadlines bound a whole logical request, response, or idle state across many syscalls. TCP keepalive probes long-idle connections to detect dead peers, often after long defaults, and does not prove the remote application is healthy. On Embedded or cellular devices, keepalive timing affects power and traffic, so explicit protocol deadlines and reconnect policy often matter more.

### 30. Compare domain/address family, socket type, and protocol.

The domain or address family chooses the addressing scope and structure, such as `AF_UNIX`, `AF_INET`, or `AF_INET6`. The socket type chooses communication semantics, such as `SOCK_STREAM` byte stream or `SOCK_DGRAM` datagram. The protocol chooses the concrete transport for that family/type pair; most code passes `0` so the kernel selects TCP for Internet stream sockets and UDP for Internet datagram sockets.

### 31. How do `sockaddr_in`, `sockaddr_in6`, and `sockaddr_storage` affect IPv4/IPv6 portability?

`sockaddr_in` is IPv4-specific and carries a 32-bit address plus port. `sockaddr_in6` is IPv6-specific and carries a 128-bit address, port, and fields such as scope id for link-local addresses. `sockaddr_storage` is large and aligned enough to store either family, so it is the right buffer type for generic `accept()`, `recvfrom()`, and address-logging code.

### 32. Why should a network protocol avoid raw C structs?

Raw C structs bake in host byte order, type sizes, padding, alignment, and compiler ABI. A real protocol should define an explicit wire format: text, length-prefixed binary, protobuf/CBOR/JSON, or another stable encoding. Numeric fields in socket address structures and binary protocol headers need defined byte order, commonly network byte order with helpers such as `htons()` and `htonl()`.

### 33. Compare `read()`/`write()`, `send()`/`recv()`, and `sendto()`/`recvfrom()`.

For connected sockets, `read()`/`write()` and plain `recv()`/`send()` cover normal byte I/O, but `send()`/`recv()` add socket flags such as `MSG_NOSIGNAL`, `MSG_PEEK`, `MSG_WAITALL`, and `MSG_DONTWAIT`. `sendto()`/`recvfrom()` are the usual unconnected datagram APIs because each packet may need a destination or source address. `sendmsg()`/`recvmsg()` are the advanced form for scatter/gather I/O and ancillary data.

---

## Recognition Notes - Priority C

- Raw sockets and packet sockets are specialized, privilege-sensitive APIs for lower-level network access.
- Out-of-band TCP data is historical urgent-data behavior. Modern designs usually use an explicit control channel.
- `SOCK_SEQPACKET` is reliable, connection-oriented, and preserves message boundaries where supported, notably UNIX domain sockets on Linux.
- SCTP is reliable and message-oriented with multistreaming. DCCP is unreliable but congestion-controlled. Recognize them; do not treat them as normal interview core.
- Abstract UNIX sockets are Linux-specific. They avoid filesystem path cleanup but are not portable and are less visible to normal file tools.
- `recvmmsg()` and `sendmmsg()` are Linux batching APIs for high-rate datagrams.
- `inetd` is a legacy superserver that can accept or receive activity, then exec service programs with sockets on standard fds.
- BPF socket filters and raw packet capture are production-specialist topics, not core Chapter 8 interview material.

---

## Extra Questions Worth Adding

1. A service binds to `0.0.0.0` in development. What security and deployment risks does that create?
2. How would you test that a stream parser handles fragmented and coalesced messages?
3. Why should a network protocol avoid raw C structs?
4. How would you design graceful shutdown for a server with active clients?
5. How do you prevent a remote fork bomb in a fork-per-client server?
6. Why might a client work from the host but fail from inside a container?
7. How would you debug an fd leak using `ss`, `/proc/<pid>/fd`, and `strace`?
8. When is `socketpair()` cleaner than a pathname UNIX socket?
9. How should a client handle multiple `getaddrinfo()` results and connection failures?
10. What metrics would you expose from a production socket server?

---

## One-Minute Review

- A socket is a kernel communication endpoint exposed as a file descriptor.
- Server flow is `socket()` -> `setsockopt()` -> `bind()` -> `listen()` -> `accept()` -> per-client I/O.
- Client flow is usually `socket()` -> `connect()` -> `send()`/`recv()` -> `close()`.
- `accept()` returns a new connected fd; the listening fd remains open.
- TCP is reliable and ordered, but it is a byte stream with no message boundaries.
- UDP preserves datagrams but does not guarantee delivery, order, or duplicate suppression.
- Partial reads and writes are normal; use framing and exact I/O loops.
- `recv() == 0` on a stream socket means peer EOF.
- Closed peer write paths can raise `SIGPIPE` and return `EPIPE`.
- Set `SO_REUSEADDR` before `bind()` for normal TCP server restart behavior.
- `TIME_WAIT` is normal TCP correctness. `CLOSE_WAIT` often means application cleanup is broken.
- Use `getaddrinfo()` for IPv4/IPv6-aware code and `gai_strerror()` for its errors.
- Use `htons()` for ports and avoid sending raw C structs over the network.
- `INADDR_ANY` listens on all matching interfaces; loopback is same-host only.
- UNIX domain sockets are same-host IPC with permissions, credentials, `socketpair()`, and fd passing.
- Production debugging combines `ss`, `/proc`, `strace`, logs, resolver tools, route tools, and `tcpdump`.
- Embedded and backend socket design is mostly resource lifecycle, explicit protocol framing, and bounded concurrency.
