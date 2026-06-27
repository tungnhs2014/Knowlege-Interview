# Chapter 8 - Socket Server Design

> Topics: 8.5 Socket Server Design - iterative, concurrent, fork/thread per client, prefork/prethread, `inetd`.
> Main sources: TLPI Ch60; TLPI Ch59 helper patterns; DevLinux module 06.
> Production context: network daemons, embedded gateways, local control services, request/response servers, connection-heavy backend services, and long-running Linux processes that must survive client churn.

---

## Coverage Notes

This file covers Coverage Matrix row 8.5 and the Chapter 8 server-design Must Cover item.

- Covered here: iterative servers, fork-per-connection, thread-per-connection, prefork/prethread pools, event-loop recognition, `inetd`, backlog pressure, backpressure and overload behavior, fd ownership after `fork()`, child reaping, graceful shutdown, timeout policy for slow clients, production bugs, debugging commands, Embedded capacity constraints, checklist, and interview readiness.
- Cross-file coverage: base socket call lifecycle is in `ch08_socket_overview.md`; Internet addressing is in `ch08_socket_tcp.md`; UNIX-domain local-control and fd-passing details are in `ch08_socket_unix.md`; nonblocking I/O primitives, `SO_RCVTIMEO`/`SO_SNDTIMEO`, TCP keepalive, and socket-option details are in `ch08_socket_advanced.md`.
- Detailed `select()`/`poll()`/`epoll()` mechanics are intentionally moved to Chapter 9 I/O multiplexing; no Chapter 8 server-design concept is intentionally out of scope.

## Problem It Solves

A socket server is not finished when it can accept one client. The real design question is: what happens when clients are slow, numerous, malicious, or long-lived?

Server design decides:

```text
incoming connections/datagrams
        |
        v
kernel queues
        |
        v
accept/recv loop
        |
        v
work execution model
        |
        v
cleanup, limits, observability
```

The same socket API can produce a tiny iterative server or a production daemon. The difference is lifecycle discipline: who owns each fd, who reaps children, how concurrency is bounded, and what happens under load.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | iterative server, concurrent server, `accept()` loop, fork-per-client, fd close rules | Build a correct small TCP server |
| Work useful | zombie reaping, concurrency limits, backlog pressure, daemon logging, graceful shutdown | Keep a server alive under real clients |
| Recognize | prefork/prethread pools, event loops, `inetd`, server farms/load balancers | Understand common production architectures |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Iterative server | Handles one client/request at a time | Good for quick UDP request/reply |
| Concurrent server | Handles multiple clients at the same time | Fork/thread/event loop |
| Listening socket | Passive socket used only for accepting stream connections | Parent keeps this fd |
| Connected socket | Per-client fd returned by `accept()` | Worker handles this fd |
| Worker | Process/thread/task handling client work | Child process in fork-per-client |
| Zombie | Exited child not yet waited for | Reap with `waitpid()` |
| Backlog | Pending connection queue size requested by `listen()` | Capped by kernel settings |
| Accept loop | Server loop around `accept()` | Must handle `EINTR` |
| Fork-per-client | New child process per accepted connection | Simple isolation, higher cost |
| Thread-per-client | New thread per connection | Shared memory, cheaper than process, more shared-state risk |
| Prefork pool | Fixed/managed set of worker processes | Avoid fork cost during bursts |
| Prethread pool | Fixed/managed set of worker threads | Common in service daemons |
| Event loop | One/few threads multiplex many fds | `epoll` in Linux, covered later |
| `inetd` | Legacy superserver that accepts/starts services | Recognize in old UNIX systems |
| Remote fork bomb | Client load causes server to fork too many children | Use concurrency limits |

## Concept Overview

### Choose Model by Request Shape

| Workload | Suitable model | Reason |
|----------|----------------|--------|
| Tiny independent UDP requests | Iterative | One datagram at a time is enough |
| Short TCP requests, low traffic | Iterative TCP can be acceptable | Simplicity |
| Long TCP conversations | Concurrent | One slow client must not block all others |
| Bursty many clients | Prefork/prethread or event loop | Avoid per-request creation cost |
| Thousands of mostly idle connections | Event loop | Processes/threads per client become expensive |

### The Critical FD Ownership Rule

After `fork()`:

```text
parent: close(connected_fd), keep(listening_fd)
child:  close(listening_fd), handle(connected_fd), close(connected_fd), exit
```

If either side keeps an fd it does not need, EOF and resource cleanup can break. This is one of the most common server bugs.

### Process vs Thread Tradeoff

| Model | Strength | Risk |
|-------|----------|------|
| Process per client | Isolation, simple crash containment | More memory/context-switch overhead |
| Thread per client | Lower overhead, easy shared cache | Data races, one process address space |
| Pool | Bounded resources, predictable load | More coordination |
| Event loop | Scales idle connections | More complex state machine |

## System Context

| Subsystem | Server design impact |
|-----------|----------------------|
| Scheduler | Processes/threads compete for CPU |
| FD table | Each connection consumes fds; leaks eventually break `accept()` |
| Signals | `SIGCHLD` for child exit, `SIGPIPE` for closed peers, `SIGTERM` for shutdown |
| TCP state | `TIME_WAIT`, `ESTABLISHED`, `CLOSE_WAIT` show lifecycle health |
| Kernel queues | SYN/accept queues absorb bursts but are finite |
| Resource limits | `RLIMIT_NOFILE`, process/thread limits cap concurrency |
| Logging | Per-request logs under attack can become a bottleneck |

Under production pressure, a server may fail even when the socket calls are correct: too many fds, too many children, full accept queues, blocked worker pool, or a signal handler that does not reap zombies.

## Architecture

### Iterative TCP Server

```text
accept client A
    |
    v
handle all of A
    |
    v
close A
    |
    v
accept client B
```

Simple, but one slow client delays everyone behind it.

### Fork-per-client Server

```text
parent:
    accept()
    fork()
    close(cfd)
    continue accept loop

child:
    close(lfd)
    serve cfd
    close(cfd)
    _exit()
```

Good first concurrent design. Add a child limit for real services.

### Thread-per-client Server

```text
accept()
    |
    v
pthread_create(worker, cfd)
    |
    v
worker closes cfd when done
```

Cheaper than processes but every shared object must be thread-safe.

### Prefork / Prethread Pool

```text
startup creates N workers
        |
        v
workers wait for accepted clients
        |
        v
each worker handles one client then returns to pool
```

This amortizes process/thread creation and gives an explicit concurrency budget.

### `inetd`

`inetd` is a legacy superserver. It listens on configured ports, accepts/receives, forks, duplicates the socket onto standard fds, and execs the service. It reduces boilerplate for low-traffic services but is not the common design for high-throughput modern daemons.

## Execution Flow

### Iterative UDP Echo

```text
socket(SOCK_DGRAM)
    |
    v
bind(port)
    |
    v
recvfrom(datagram, client address)
    |
    v
sendto(reply, client address)
    |
    v
repeat
```

No `listen()` or `accept()`. One socket can receive from all clients.

### Fork-per-client TCP Lifecycle

```text
socket -> bind -> listen
    |
    v
accept() -> cfd
    |
    v
fork()
    |
    +-- parent closes cfd and accepts more
    |
    +-- child closes lfd, serves cfd, exits
```

### Zombie Reaping

```text
child exits
    |
    v
kernel sends SIGCHLD
    |
    v
handler loops waitpid(-1, WNOHANG)
    |
    v
zombie entries removed
```

### Backlog Pressure

```text
clients connect faster than accept loop drains
        |
        v
pending queues fill
        |
        v
clients see delay, timeout, or refusal
```

Backlog is not a substitute for enough workers or a healthy accept loop.

### Slow-Client Timeout Flow

```text
worker accepts client
    |
    v
deadline or socket timeout starts
    |
    +-- client sends complete request in time: serve normally
    |
    +-- client trickles or stalls: close connection, free worker
```

Timeouts are not only client features. A server needs read, write, request, and idle deadlines so one peer cannot hold a scarce process, thread, fd, or RAM buffer forever.

### Graceful Shutdown

```text
SIGTERM
    |
    v
stop accepting new clients
    |
    v
close listening fd
    |
    v
let workers finish or timeout
    |
    v
close connected fds and reap children
```

## 8.5 API / Topic Sections

### Iterative Servers

Use for quick bounded work. UDP echo, simple status endpoints, or toy TCP examples fit here. Avoid for long-lived TCP sessions because one client can monopolize the process.

### Concurrent Servers

Use when requests can block, take variable time, or involve a back-and-forth conversation. The simplest version is fork-per-client. The production version must cap concurrency and handle shutdown.

### Prefork and Prethread

Use pools when load is frequent enough that creating a process/thread per client is costly. Pools also make capacity explicit: when all workers are busy, backpressure is visible.

### Event-Loop Servers

A single process or small thread group can monitor many fds with I/O multiplexing. This scales well for many idle connections but moves complexity into application state management. Linux `epoll` is covered in Chapter 9.

### Timeout Strategy

Timeouts are part of server capacity control, not just convenience. A server that waits forever on one slow client can pin a process, thread, buffer, or fd until the worker pool is exhausted.

| Timeout style | Mechanism | Use |
|---------------|-----------|-----|
| Socket receive/send timeout | `SO_RCVTIMEO`, `SO_SNDTIMEO` via `setsockopt()` | Simple blocking servers with bounded waits |
| Per-call nonblocking | `MSG_DONTWAIT` | Occasional nonblocking operation without changing fd status |
| Nonblocking fd + event loop | `O_NONBLOCK` plus `poll()`/`epoll()` | Many clients, explicit state machine |
| Application deadline | monotonic clock around protocol steps | End-to-end request limit across many syscalls |

Set timeouts according to protocol state: accept loop, request header, body transfer, response write, and graceful shutdown may need different budgets.

For Embedded gateways, these limits are part of correctness. A device with a small fd table and limited RAM cannot afford unlimited clients even on a private LAN.

### `inetd`

Recognize it in older systems or low-frequency services. A service invoked by `inetd` often reads/writes on `STDIN_FILENO`/`STDOUT_FILENO` because `inetd` already accepted and duplicated the socket.

## Work Checklist

| Pattern | Why it matters |
|---------|----------------|
| Close unused fds immediately after `fork()` | Prevent leaks and stuck EOF |
| Reap children with `SIGCHLD` + `waitpid(WNOHANG)` | Prevent zombies |
| Bound worker count | Prevent remote fork/thread bombs |
| Set `SO_REUSEADDR` before `bind()` | Restart TCP servers cleanly |
| Ignore or handle `SIGPIPE` | Client disconnect should not kill server |
| Use receive/send/deadline timeouts | Protect worker pool from slow clients |
| Make logs rate-limited | Avoid log-based denial-of-service |
| Track active connections and fd count | Detect leaks before outage |
| Separate accept from heavy work | Keep accept loop responsive |
| Design shutdown state | Avoid killing in-flight requests unexpectedly |
| Size for Embedded limits | Bound fds, threads, stack size, socket buffers, and log volume |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Prefork accept contention | Multiple workers can block in `accept()`; Linux handles the common case well |
| Parent accepts then passes fd | Uses UNIX-domain fd passing to dispatch to worker process |
| `SO_REUSEPORT` | Linux option for multiple sockets binding same address/port with kernel load distribution |
| `inetd` wait/nowait | Controls whether inetd or the child manages the service socket |
| Server farms | Multiple machines behind DNS/load balancer; outside core socket API |
| Thundering herd | Many workers wake for one event on some old designs/APIs; know the term |

## Example

### Example - Fork-per-client TCP Echo Server

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
#include <sys/wait.h>

#define PORT 9092
#define BACKLOG 32
#define BUF_SIZE 1024

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        buf += n;
        len -= (size_t)n;
    }
    return 0;
}

static void reap_children(int sig)
{
    (void)sig;
    int saved = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        continue;
    }
    errno = saved;
}

static void handle_client(int cfd)
{
    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read(cfd, buf, sizeof(buf))) > 0) {
        if (write_all(cfd, buf, (size_t)n) == -1) {
            break;
        }
    }
}

int main(void)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        die("signal");
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = reap_children;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        die("sigaction");
    }

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        die("socket");
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

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(cfd);
            continue;
        }

        if (pid == 0) {
            close(lfd);
            handle_client(cfd);
            close(cfd);
            _exit(EXIT_SUCCESS);
        }

        close(cfd);
    }
}
```

What it teaches:

- Parent and child must close different fds.
- A concurrent process server must reap children.
- This is correct for learning, but production code should add a child limit and shutdown policy.

## Debugging

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| No `SIGCHLD` reap | `ps` shows zombie children | Install handler or reap loop |
| Parent does not close `cfd` | fd leak, client EOF delayed | Close in parent after fork |
| Child does not close `lfd` | confusing fd ownership, late cleanup | Close in child before serving |
| Unlimited fork/thread | Load spike can exhaust system | Add max workers/backpressure |
| Blocking work in accept loop | New clients time out | Move work to worker or event loop |
| No `SO_REUSEADDR` | Restart fails | Set before `bind()` |
| No timeout for slow client | Worker pool stuck, fds stay `ESTABLISHED` idle | Use `SO_RCVTIMEO`, `SO_SNDTIMEO`, deadlines, or event loop timers |
| Timeout too global | Valid slow upload is killed or idle client lives forever | Use state-specific deadlines and log timeout reason |
| Excessive per-error logs | Disk/log service pressure | Rate-limit logs |

### Commands

```bash
# Count processes/threads for a server
ps -o pid,ppid,stat,cmd -C server_name
ps -L -p <pid>

# Watch listening sockets and accept queue info
ss -ltnp

# Show active connections by state
ss -tan state established
ss -tan state time-wait
ss -tan state close-wait

# Inspect fd usage
ls /proc/<pid>/fd | wc -l
cat /proc/<pid>/limits | grep 'open files'

# Find zombies
ps -eo pid,ppid,stat,cmd | awk '$3 ~ /Z/ {print}'

# Trace accept/fork/close lifecycle
strace -f -e trace=accept,accept4,fork,clone,wait4,close,setsockopt,getsockopt ./server

# Check kernel listen backlog cap
cat /proc/sys/net/core/somaxconn

# Inspect TCP timers and socket memory
ss -tinp
```

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Toy echo or health server | Iterative TCP/UDP is fine |
| CLI talking to local daemon | UNIX stream server, often iterative or small thread pool |
| Embedded gateway with few clients | Thread or process per client with hard limits |
| Backend service with many clients | Event loop or bounded worker pool |
| Privilege-separated service | Parent accepts or opens resource, passes fd to worker |
| Legacy low-frequency service | `inetd`/`xinetd` may appear |
| High availability service | External load balancer plus multiple server instances |
| Cellular or field device server | Longer connect/read timeouts, small worker/fd budget, numeric logs |

## Interview Readiness

1. What is the difference between iterative and concurrent servers?
2. When is an iterative server acceptable?
3. Why is iterative TCP dangerous for long-lived clients?
4. Explain the fd ownership rules after `fork()`.
5. Why does a fork-per-client server need to reap children?
6. What is a zombie process in server context?
7. What is a remote fork bomb, and how do you prevent it?
8. Compare process-per-client and thread-per-client.
9. Why use a prefork or prethread pool?
10. What does the listen backlog protect against, and what does it not solve?
11. How would you debug a server with many `CLOSE_WAIT` sockets?
12. Why can a server run out of file descriptors?
13. What is `inetd`, and why did it exist?
14. Why might an event-loop server scale better for many idle clients?
15. What metrics would you watch in a production socket server?
16. How should a server handle graceful shutdown?
17. How do receive/send timeouts differ from application-level deadlines?
18. Why are unbounded thread stacks and socket buffers dangerous on Embedded Linux?
19. Which server timeouts would you add to protect a worker pool from slow clients?

## Key Takeaways

1. Socket server design is mainly concurrency plus resource lifecycle.
2. `accept()` gives a per-client fd; manage ownership carefully.
3. Iterative servers are simple but fragile for slow TCP clients.
4. Fork-per-client is easy to reason about but must reap children.
5. Processes isolate better; threads are cheaper but share memory risks.
6. Pools make concurrency bounded and predictable.
7. Event loops scale many fds but require explicit state machines.
8. Backlog helps absorb bursts but does not replace worker capacity.
9. Production servers need fd limits, worker limits, timeouts, and logging discipline.
10. `inetd` is useful historical context and still appears in old systems.
11. Debug servers with `ss`, `ps`, `/proc/<pid>/fd`, `strace`, and resource limits.
12. A correct server closes what it does not own.
13. Backpressure must be explicit: backlog, worker limits, deadlines, and request-size limits protect different resources.
