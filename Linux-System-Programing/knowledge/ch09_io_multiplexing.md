# Chapter 9.1 - Alternative I/O Models

> Topics: I/O readiness, `select()`, `poll()`, `epoll`, level-triggered vs edge-triggered notification, nonblocking I/O, waiting for signals and FDs together.
> Main sources: TLPI Ch63, `LINUX_SYSTEM_LEARNING_MAP.md`.
> Production context: event loops in network servers, device/serial monitors, embedded gateways, terminal/PTY relays, and any Linux service that must watch many FDs without one blocking read freezing the process.

---

## Problem It Solves

A normal blocking `read()` or `write()` works well when a process talks to one FD. It becomes a bad design when the same process must handle many sources:

- a server with many client sockets;
- an embedded process watching UART, GPIO/eventfd, timerfd, and a control socket;
- a terminal tool relaying between stdin and a PTY master;
- a daemon that must react to pipe/socket input and still shut down on signals.

The naive fixes are expensive:

| Approach | Why it hurts |
|---|---|
| One thread/process per FD | Higher memory cost, scheduling overhead, synchronization complexity. |
| Nonblocking reads in a tight loop | Either burns CPU or increases latency if the loop sleeps too long. |

Alternative I/O models let the kernel tell the process, "one or more FDs can be used now without blocking." Then the program performs the actual I/O only on those ready FDs.

## Learning Roadmap

| Level | Learn | Goal |
|---|---|---|
| Must know | Readiness, `select()`, `poll()`, `epoll_wait()`, `EINTR`, `EAGAIN`, level-triggered behavior | Write a correct event loop for sockets, pipes, terminals, or PTYs. |
| Work useful | `O_NONBLOCK`, edge-triggered `epoll`, `EPOLLHUP`/`EPOLLERR`, signal waiting with `pselect()` or self-pipe | Avoid production hangs, missed events, CPU spin, and shutdown races. |
| Recognize | signal-driven I/O, POSIX AIO, BSD `kqueue`, Solaris `/dev/poll`, `EPOLLONESHOT`, epoll duplicate-FD semantics | Recognize advanced designs and debug unfamiliar event-loop code. |

## Core Vocabulary

| Term | Meaning | Example / note |
|---|---|---|
| File descriptor (FD) | Per-process integer handle for an open file, socket, pipe, terminal, etc. | `read(fd, buf, n)` uses the FD. |
| Blocking I/O | I/O call sleeps until it can make progress or fails. | `read()` on an empty pipe blocks while writer is open. |
| Nonblocking I/O | I/O call returns immediately if it cannot make progress. | `read()` may fail with `EAGAIN` / `EWOULDBLOCK`. |
| Readiness | State where an I/O call would not block. | It may still return EOF or error; readiness is not "success". |
| I/O multiplexing | Monitoring multiple FDs in one wait call. | `select()` and `poll()`. |
| Event loop | Loop that waits for ready FDs, handles them, then waits again. | Common in servers and embedded dispatchers. |
| Level-triggered | Reports readiness as long as the FD remains ready. | `select()`, `poll()`, default `epoll`. |
| Edge-triggered | Reports a new activity edge since last wait. | `epoll` with `EPOLLET`; drain until `EAGAIN`. |
| `fd_set` | Bit-set used by `select()`. | Modified in-place by the kernel; rebuild each loop. |
| `nfds` | One greater than highest FD monitored by `select()`. | If max FD is 12, pass `13`. |
| `struct pollfd` | Entry used by `poll()` with input `events` and output `revents`. | Set `fd = -1` to temporarily ignore an entry. |
| `revents` | Event bits returned by `poll()`. | Check `POLLIN`, `POLLHUP`, `POLLERR`, `POLLNVAL`. |
| epoll instance | Kernel object, referenced by an FD, holding epoll state. | Created by `epoll_create1()`. |
| Interest list | FDs registered in an epoll instance. | Updated by `epoll_ctl()`. |
| Ready list | Kernel list of registered FDs that became ready. | Returned by `epoll_wait()`. |
| `EPOLLET` | Enables edge-triggered notification for an FD. | Requires nonblocking drain loops. |
| `EPOLLONESHOT` | Disables an FD after one event until rearmed. | Useful with worker-thread handoff. |
| `EPOLLHUP` / `EPOLLERR` | Hangup/error notifications. | Returned even if not requested. |
| `EPOLLRDHUP` | Linux flag for peer half-close on stream socket. | Useful for TCP/UNIX stream sockets. |
| `EINTR` | Wait was interrupted by a signal handler. | Usually retry the wait after handling signal state. |
| `EAGAIN` | Nonblocking I/O would block now. | Normal stopping condition in edge-triggered reads. |
| Self-pipe trick | Signal handler writes a byte to a pipe monitored by the event loop. | Converts signal delivery into FD readiness. |
| `pselect()` / `ppoll()` / `epoll_pwait()` | Wait APIs that atomically swap signal mask while waiting. | Avoid signal-vs-wait race. |

## Concept Overview

The key mental model:

```text
readiness API
    tells you: "this FD probably will not block"
    does not:  transfer bytes for you
    next step: call read(), write(), accept(), recv(), send(), etc.
```

Readiness can mean data, EOF, hangup, or error. For example, a pipe read end is "readable" when data is present, and also when all writers closed because `read()` can now return EOF without blocking.

`select()` and `poll()` are portable and simple, but the kernel must receive and scan the monitored FDs on each wait. `epoll` is Linux-specific and keeps the watch set in the kernel, so it scales better when many FDs are mostly idle.

## System Context

Alternative I/O models sit between application control flow and kernel FD readiness callbacks:

```text
Application event loop
    |
    v
select() / poll() / epoll_wait()
    |
    v
Kernel checks readiness for sockets, pipes, terminals, PTYs, FIFOs, devices
    |
    v
Application performs actual I/O on ready FDs
```

Important interactions:

| Subsystem | Interaction |
|---|---|
| File I/O | Regular files are usually always ready for `select()`/`poll()`; `epoll_ctl()` rejects regular files/directories with `EPERM`. |
| Sockets | Main production use: accept clients, read requests, write pending responses, detect peer shutdown. |
| Terminals / PTYs | Used for key-by-key input and relay loops; packet-mode PTY state changes appear as exceptional/POLLPRI events. |
| Signals | `select()`/`poll()`/`epoll_wait()` can fail with `EINTR`; signal-aware loops need a race-free shutdown path. |
| Threads / fork | `epoll` instances and duplicate FDs refer to underlying open file descriptions; closing one duplicate may not remove the watched object. |

## Architecture

### `select()` - user-space bit sets

`select()` receives up to three `fd_set` bit masks:

```text
readfds    -> input possible?
writefds   -> output possible?
exceptfds  -> exceptional condition, not general error
```

`fd_set` is value-result: the caller passes the interest set, and the kernel overwrites it with the ready set. Therefore every loop iteration must rebuild it.

Practical limits:

| Detail | Why it matters |
|---|---|
| `FD_SETSIZE` is commonly 1024 on Linux/glibc | `select()` is a poor choice for high-FD services. |
| `nfds = max_fd + 1` | Too small misses FDs; too large wastes scan work. |
| Linux may modify `timeout` | Portable code reinitializes timeout before every call. |

### `poll()` - array of descriptors

`poll()` uses an array:

```text
struct pollfd {
    int fd;
    short events;   // caller interest
    short revents;  // kernel result
};
```

`events` is not overwritten, so the array is easier to reuse than `fd_set`. `poll()` still scans all array entries each call, so it is simpler than `select()` but not a high-scale solution.

Core bits:

| Bit | Meaning |
|---|---|
| `POLLIN` | Read would not block. |
| `POLLOUT` | Write would not block for some data. |
| `POLLPRI` | High-priority/exceptional data, such as PTY packet-mode control. |
| `POLLHUP` | Peer/other end hung up. |
| `POLLERR` | Error condition. |
| `POLLNVAL` | FD was not open at the time of the call. |

### `epoll` - persistent kernel state

An epoll instance has:

```text
interest list: FDs the process wants to monitor
ready list:    subset that currently has events
```

The three-step API is:

| Step | API | Job |
|---|---|---|
| Create | `epoll_create1(EPOLL_CLOEXEC)` | Get epoll instance FD. |
| Register/update/remove | `epoll_ctl(epfd, EPOLL_CTL_*, fd, &ev)` | Maintain interest list. |
| Wait | `epoll_wait(epfd, evlist, maxevents, timeout)` | Fetch ready events. |

Use `ev.data.fd` or `ev.data.ptr` to identify the object when `epoll_wait()` returns. The kernel only returns the `data` value you registered; it does not separately tell you the FD number.

## Execution Flow

### Blocking problem flow

```text
read(fd_a)
    |
    | no data on fd_a
    v
process sleeps
    |
    | fd_b becomes ready but process is asleep on fd_a
    v
latency / deadlock / missed service window
```

### Level-triggered event loop

```text
build interest set
    |
    v
wait for readiness
    |
    v
for each ready FD: do bounded I/O
    |
    v
repeat; if FD remains ready, it is reported again
```

### `select()` loop

```text
FD_ZERO / FD_SET every iteration
    |
    v
select(max_fd + 1, &readfds, &writefds, NULL, &timeout)
    |
    +--> -1 + EINTR: handle signal state and retry
    +-->  0: timeout work
    +--> >0: FD_ISSET() then read/write/accept
```

### `poll()` loop

```text
build pollfd array
    |
    v
poll(fds, nfds, timeout_ms)
    |
    v
scan revents
    |
    +--> POLLIN/POLLOUT: perform I/O
    +--> POLLHUP/POLLERR/POLLNVAL: close/remove/fix entry
```

### `epoll` level-triggered loop

```text
epoll_create1()
    |
    v
epoll_ctl(ADD) once per FD
    |
    v
epoll_wait()
    |
    v
handle returned events only
    |
    v
epoll_ctl(MOD/DEL) when interest changes
```

### `epoll` edge-triggered flow

```text
set O_NONBLOCK
    |
    v
epoll_ctl(ADD, EPOLLIN | EPOLLET)
    |
    v
epoll_wait reports edge
    |
    v
read/accept/recv in a loop until EAGAIN
```

### Signal-safe wait flow

```text
block target signal
    |
    v
install handler or prepare signalfd/self-pipe
    |
    v
pselect()/ppoll()/epoll_pwait() atomically waits with chosen mask
    |
    v
handle FD readiness or signal state without lost wakeup race
```

## 9.1 API / Topic Sections

### 9.1.1 `select()`

Use when portability matters and FD count is small.

```text
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

When to use:

- small tools;
- portable examples;
- waiting on stdin plus one or two pipes/sockets.

Avoid when:

- FDs can exceed `FD_SETSIZE`;
- FD set is large or sparse;
- you need a high-scale Linux server loop.

Production pitfalls:

| Pitfall | Result | Fix |
|---|---|---|
| Not rebuilding `fd_set` | Missing FDs after first call. | `FD_ZERO` and `FD_SET` every iteration. |
| Wrong `nfds` | Highest FD never monitored. | Track `max_fd + 1`. |
| Reusing `timeout` on Linux | Timeout shrinks unexpectedly. | Reinitialize each call. |
| Treating `exceptfds` as "errors" | Wrong handling. | Use it mainly for OOB socket data or PTY packet mode. |

### 9.1.2 `poll()`

Use when you need POSIX portability without `select()`'s fixed `FD_SETSIZE`.

```text
int poll(struct pollfd fds[], nfds_t nfds, int timeout);
```

Timeout behavior:

| Timeout | Meaning |
|---|---|
| `-1` | Wait indefinitely. |
| `0` | Do not block. |
| `>0` | Wait up to that many milliseconds. |

Production notes:

- `events` is input; `revents` is output.
- `POLLERR`, `POLLHUP`, and `POLLNVAL` are returned in `revents`.
- Set `fd = -1` to disable an entry temporarily.
- Still O(number of array entries), so do not mistake it for an epoll replacement.

### 9.1.3 `epoll`

Use for Linux services that monitor many mostly-idle descriptors.

```text
int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
int epoll_wait(int epfd, struct epoll_event *evlist,
               int maxevents, int timeout);
```

When to use:

- large socket servers;
- gateways with many client/control/device FDs;
- event loops where the watch set changes over time but is reused many times.

When to avoid:

- strict non-Linux portability;
- regular files/directories;
- tiny tools where `poll()` is simpler and sufficient.

Production pitfalls:

| Pitfall | Symptom | Fix |
|---|---|---|
| Forgetting `ev.data` | Cannot identify ready FD/object. | Store `fd` or pointer in `ev.data`. |
| Adding same FD twice | `epoll_ctl()` fails with `EEXIST`. | Use `MOD` for existing entries. |
| Modifying missing FD | `ENOENT`. | Keep ownership of add/remove state. |
| Closing one duplicate FD | Events may still appear for old number. | `EPOLL_CTL_DEL` before `dup()`/handoff surprises, or close all duplicates. |
| Monitoring regular file | `EPERM`. | Do not use epoll for regular files. |

### 9.1.4 Readiness Semantics by FD Type

| FD type | Read readiness means | Write readiness means |
|---|---|---|
| Regular file | `read()` returns data, EOF, or error immediately. | `write()` transfers to cache or fails immediately. |
| Pipe/FIFO read end | Data exists, or writers closed so EOF is available. | Not applicable. |
| Pipe/FIFO write end | Not applicable. | Space exists, or read end closed so error is discoverable. |
| Listening socket | `accept()` can complete without blocking. | Usually not relevant. |
| Stream socket | Data, EOF, half-close, or error can be observed. | Some send-buffer space exists; a large write can still block unless nonblocking. |
| Terminal/PTY | Input line/byte is available according to terminal mode. | Output queue has room. |

Readiness is a promise about blocking, not a promise that useful data will be transferred.

### 9.1.5 Waiting for Signals and FDs

Race to avoid:

```text
install signal handler
    |
    | signal arrives here
    v
select() starts and may sleep forever
```

Use one of these patterns:

| Pattern | When useful | Core idea |
|---|---|---|
| `pselect()` | POSIX `select()`-style code | Atomically swap signal mask during wait. |
| `ppoll()` | Linux `poll()`-style code | Same idea for `poll()`. |
| `epoll_pwait()` | Linux epoll code | Same idea for `epoll_wait()`. |
| Self-pipe trick | Portable event loops | Signal handler writes one byte to nonblocking pipe; main loop reads it. |
| `signalfd` | Linux-specific event loops | Treat signals as readable FD events. |

## Work-Useful Patterns

| Pattern | Use it when | Why it works |
|---|---|---|
| Prefer level-triggered first | New event loop or small/medium service | Easier correctness; missed drains are less fatal. |
| Use `poll()` for small portable multi-FD tools | CLI, embedded POSIX portability | No `FD_SETSIZE`; simple array model. |
| Use `epoll` for many sockets on Linux | Thousands of mostly idle FDs | Kernel keeps interest list; wait returns ready events. |
| Make writes interest-driven | Output buffer is not empty | Add `POLLOUT`/`EPOLLOUT` only while there is data to send. |
| Always handle hangup/error bits | Sockets, pipes, PTYs | Peer close often appears as readiness plus HUP/ERR. |
| Use nonblocking with edge-triggered | `EPOLLET` or signal-driven I/O | Lets drain loops stop at `EAGAIN` instead of hanging. |
| Bound per-FD work | Edge-triggered high-throughput FDs | Prevent one busy FD from starving others. |
| Integrate shutdown as an event | Daemons and servers | Self-pipe, `signalfd`, eventfd, or `pselect()` avoids signal races. |

## Advanced / Recognize First

| Topic | Know this much |
|---|---|
| Signal-driven I/O (`O_ASYNC`, `SIGIO`) | Edge-triggered readiness via signals; scalable but complex and rarely chosen over `epoll` in modern Linux services. |
| POSIX AIO | Queues I/O operations and reports completion, different from readiness. TLPI notes Linux/glibc historically used a thread-based implementation. |
| `kqueue` / `/dev/poll` | Non-Linux event mechanisms on BSD/Solaris-like systems. Know them when reading portable event libraries. |
| `libevent` / similar libraries | Abstraction over `select`, `poll`, `epoll`, and platform-specific mechanisms. Useful for portable production code. |
| `EPOLLONESHOT` | One event disables the FD until `EPOLL_CTL_MOD` rearms it; useful for worker ownership. |
| epoll nested instances | Linux allows monitoring some epoll FDs, but designs can become subtle. Recognize; avoid unless using a mature framework. |
| `EPOLLRDHUP` | Linux-specific stream-socket half-close detection; useful but not portable. |

## Example

### Example 1: `poll()` for stdin with timeout

```c
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    struct pollfd fds[1];
    char buf[128];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    for (;;) {
        int ready = poll(fds, 1, 3000);
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            return 1;
        }
        if (ready == 0) {
            puts("timeout");
            continue;
        }
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            puts("stdin closed or invalid");
            return 0;
        }
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) {
                return 0;
            }
            if (write(STDOUT_FILENO, buf, (size_t) n) == -1) {
                perror("write");
                return 1;
            }
        }
    }
}
```

What it teaches:

- `poll()` keeps interest in `events` and reports result in `revents`.
- `EINTR` is a normal wait-loop condition.
- HUP/error handling belongs next to data handling.

### Example 2: Edge-triggered `epoll` drain loop

```c
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#define MAX_EVENTS 8

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    int epfd;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    char buf[256];

    if (set_nonblocking(STDIN_FILENO) == -1) {
        perror("fcntl");
        return 1;
    }

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("epoll_ctl");
        close(epfd);
        return 1;
    }

    for (;;) {
        int ready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < ready; i++) {
            if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                close(epfd);
                return 0;
            }
            while (events[i].events & EPOLLIN) {
                ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
                if (n > 0) {
                    if (write(STDOUT_FILENO, buf, (size_t) n) == -1) {
                        perror("write");
                        close(epfd);
                        return 1;
                    }
                    continue;
                }
                if (n == 0) {
                    close(epfd);
                    return 0;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                perror("read");
                close(epfd);
                return 1;
            }
        }
    }

    close(epfd);
    return 1;
}
```

What it teaches:

- `EPOLLET` requires `O_NONBLOCK`.
- After a read event, drain until `EAGAIN`/`EWOULDBLOCK`.
- `EPOLLHUP`/`EPOLLERR` must be handled even if the loop mainly expects `EPOLLIN`.

### Example 3: Self-pipe shape for signal-aware loops

```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

static int sigpipe_fds[2] = {-1, -1};

static void on_sigint(int sig) {
    unsigned char byte = (unsigned char) sig;
    ssize_t unused = write(sigpipe_fds[1], &byte, 1);
    (void) unused;
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    struct sigaction sa;

    if (pipe(sigpipe_fds) == -1) {
        perror("pipe");
        return 1;
    }
    if (set_nonblocking(sigpipe_fds[0]) == -1 ||
        set_nonblocking(sigpipe_fds[1]) == -1) {
        perror("fcntl");
        return 1;
    }

    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    for (;;) {
        fd_set rfds;
        int nfds = sigpipe_fds[0] + 1;
        FD_ZERO(&rfds);
        FD_SET(sigpipe_fds[0], &rfds);

        if (select(nfds, &rfds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            return 1;
        }

        if (FD_ISSET(sigpipe_fds[0], &rfds)) {
            unsigned char buf[32];
            while (read(sigpipe_fds[0], buf, sizeof(buf)) > 0) {
            }
            puts("shutdown requested");
            break;
        }
    }

    close(sigpipe_fds[0]);
    close(sigpipe_fds[1]);
    return 0;
}
```

What it teaches:

- A signal handler should do minimal async-signal-safe work.
- Writing to a nonblocking pipe turns signal delivery into FD readiness.
- The main loop drains the pipe and handles shutdown in normal code.

## Debugging

Useful commands:

```bash
ulimit -n
cat /proc/sys/fs/file-max
cat /proc/sys/fs/epoll/max_user_watches
ls -l /proc/<pid>/fd
strace -e trace=select,pselect6,poll,ppoll,epoll_ctl,epoll_wait,epoll_pwait -p <pid>
```

Common bugs:

| Bug | Symptom | Fix / check |
|---|---|---|
| Forgot `FD_ZERO()` in loop | `select()` stops reporting expected FDs. | Rebuild all sets before each call. |
| `nfds` too low | Highest FD never wakes the loop. | Compute max watched FD plus one. |
| Treating timeout as reusable | Loop times out too early on Linux. | Reinitialize timeout before each wait. |
| Ignoring `EINTR` | Service exits on harmless signal. | Retry wait after handling signal state. |
| Edge-triggered read once | Data remains stuck until new activity. | Drain until `EAGAIN`/`EWOULDBLOCK`. |
| Edge-triggered blocking FD | Event loop hangs inside `read()`/`accept()`. | Set `O_NONBLOCK` before registering. |
| Always monitoring write readiness | Loop wakes continuously because sockets are often writable. | Enable write interest only while output buffer is nonempty. |
| Ignoring HUP/ERR | Closed peer looks like mysterious idle connection. | Check HUP/ERR next to IN/OUT. |
| epoll watched duplicate FD | Event reports FD number already closed locally. | Understand open file description lifetime; remove before duplication when needed. |
| Busy loop on regular files | `select()`/`poll()` returns immediately forever. | Do not build readiness loops around regular files. |

## Real-world Usage

| Scenario | Practical design |
|---|---|
| Small CLI watching stdin plus one pipe | `poll()` with 2 entries; handle `EINTR` and HUP. |
| TCP server on Linux | `epoll` level-triggered; nonblocking sockets; add `EPOLLOUT` only for pending writes. |
| High-throughput proxy | `epoll` edge-triggered with per-connection buffers and bounded drain loops. |
| Embedded gateway | `poll()` or `epoll` over serial FD, socket FD, eventfd/timerfd/signalfd. |
| PTY relay | `poll()` or `select()` over real terminal and PTY master. |
| Portable library | Hide backend behind an abstraction; use `epoll` on Linux, fallback elsewhere. |
| Graceful shutdown | `pselect()`/`ppoll()`/self-pipe/signalfd to avoid lost signal wakeups. |

## Interview-Relevant Questions

1. What does it mean for an FD to be "ready"?
2. Why do `select()` and `poll()` not perform I/O themselves?
3. Why must `select()` rebuild `fd_set` each loop?
4. What does `nfds` mean in `select()`?
5. How does `poll()` avoid the value-result problem of `select()`?
6. Why does `poll()` still scale poorly for many FDs?
7. What are epoll's interest list and ready list?
8. Why does `epoll` scale better than `select()`/`poll()` for many idle sockets?
9. What is the difference between level-triggered and edge-triggered notification?
10. Why does edge-triggered `epoll` require nonblocking FDs?
11. What bug happens if you read only once after an `EPOLLET` notification?
12. Why should write readiness usually be enabled only when there is pending output?
13. How do `POLLHUP`, `POLLERR`, `EPOLLHUP`, and `EPOLLERR` affect cleanup?
14. Why can readiness still lead to EOF or an error?
15. How do regular files behave with readiness APIs?
16. What race does `pselect()` solve?
17. How does the self-pipe trick convert signals into event-loop work?
18. What is surprising about epoll and duplicated FDs?
19. When would you choose `poll()` over `epoll`?
20. How would you debug a server stuck inside `epoll_wait()` or spinning at 100% CPU?

## Key Takeaways

1. Readiness means an I/O call should not block; it does not guarantee useful data.
2. `select()` is portable but limited by `FD_SETSIZE` and value-result `fd_set`.
3. `poll()` is cleaner than `select()` and avoids the fixed FD bit-set, but still scans all entries.
4. `epoll` is Linux-specific and scales well by keeping interest and ready lists in the kernel.
5. Level-triggered notification is easier to reason about and should be the default learning path.
6. Edge-triggered notification is powerful but requires `O_NONBLOCK` and drain-until-`EAGAIN`.
7. Always handle `EINTR` in wait loops.
8. Always handle HUP/error events; peer closure is part of normal I/O lifecycle.
9. Do not keep `EPOLLOUT` always enabled for sockets unless you want wakeup storms.
10. Signal-aware event loops need `pselect()`, `ppoll()`, `epoll_pwait()`, self-pipe, or `signalfd`.
11. Regular files do not benefit from readiness loops; they are typically immediately ready.
12. For production Linux servers, use a mature event abstraction or implement epoll state ownership carefully.
