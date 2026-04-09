# Alternative I/O Models — select(), poll(), epoll

## Problem It Solves

Traditional blocking I/O handles **one file descriptor at a time** — if no data is
available, `read()` blocks until data arrives. This breaks down when a single process
must handle **multiple I/O sources simultaneously**: many client sockets, a combination
of pipes, terminals, and sockets, or any scenario where blocking on one FD would starve
all others.

Two naive workarounds exist but both have severe costs:

| Approach | Problem |
|---|---|
| One thread/process per FD | Resource overhead, IPC complexity to coordinate |
| Nonblocking + polling loop | CPU waste (busy-wait), high latency or tight loop |

**I/O multiplexing** solves the problem elegantly: hand the kernel a list of FDs to watch,
and block until **at least one becomes ready** — then perform I/O only on ready FDs. No
CPU waste, no thread explosion.

Three primary mechanisms are used in this topic: `select()`, `poll()`, and `epoll`.
For large descriptor sets, `epoll` usually scales much better.

---

## Concept Overview

### Level-Triggered vs Edge-Triggered Notification

This distinction is the foundation for understanding all three APIs:

| Model | When notified | API |
|---|---|---|
| **Level-triggered** | FD *is currently* ready (re-notifies on each check) | `select()`, `poll()`, `epoll` (default) |
| **Edge-triggered** | FD just had *new* I/O activity (fires once per transition) | Signal-driven I/O, `epoll` + `EPOLLET` |

**Edge-triggered rule**: after receiving notification, the program must drain **all available
data** (loop until `EAGAIN`). Missing data is not re-notified — it stays in the kernel
buffer silently until the next new event arrives.

### The Three APIs at a Glance

| Property | `select()` | `poll()` | `epoll` |
|---|---|---|---|
| Portability | POSIX | POSIX | Linux-specific |
| FD limit | `FD_SETSIZE` = 1024 | None | None |
| Reinitialize per call | Yes (value-result) | No (separate fields) | No (persistent list) |
| Kernel scan per call | All FDs in range | All FDs in array | Only ready FDs |
| Notification model | Level-triggered | Level-triggered | Level or Edge |
| Scaling | O(n FDs) | O(n FDs) | O(n events) |

---

## System Context

I/O multiplexing operates between **user space** (application logic) and the **kernel VFS
layer** (file descriptor readiness tracking). It does not perform I/O itself — it only
tells the process *when* to perform I/O.

```
User space                        Kernel space
─────────────────────────────     ──────────────────────────────
select() / poll() / epoll_wait()  ←→  kernel FD readiness poll routines
                                       (same internal routines for all three)
                                       ↓
                                  VFS layer: pipes, sockets, terminals, FIFOs
```

All three APIs use the **same internal kernel poll routines** per file descriptor — the
difference is in how the calling convention manages the list of FDs between calls.

Key integration points:
- `select()` / `poll()` can monitor regular files, but those descriptors are typically
  reported as ready immediately.
- `epoll` cannot monitor regular files or directories (`epoll_ctl()` returns `EPERM`).
- Commonly monitored descriptors: pipes, FIFOs, sockets, terminals, pseudoterminals,
  character devices, inotify FDs, signalfd FDs, POSIX MQ FDs.
- Pairs naturally with `O_NONBLOCK` — especially mandatory for edge-triggered mode.

---

## Architecture & Internal Mechanics

### `select()` — value-result fd_set

```c
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

**fd_set** is a fixed-size bit mask (`FD_SETSIZE = 1024` bits on Linux). The caller
initializes three sets, passes them in, and the kernel **overwrites** them in-place to
indicate which FDs are ready. Because the same structure serves as both input and output
(value-result), the caller must **reinitialize before every call** when looping.

```c
// Macros to manipulate fd_set
FD_ZERO(&set);         // clear all bits
FD_SET(fd, &set);      // mark fd as monitored
FD_CLR(fd, &set);      // unmark fd
FD_ISSET(fd, &set);    // test if fd is set (after select returns)
```

`nfds` must be `max_fd + 1` — kernel scans from 0 to `nfds-1`. Even if only FD 1000 is
monitored, the kernel walks all 1001 positions in each set.

**Timeout**:
- `NULL` → block indefinitely
- `{0, 0}` → non-blocking poll, return immediately
- Other → block up to that duration (Linux modifies the struct to show remaining time —
  non-portable; reinitialize before each call anyway)

### `poll()` — separate events/revents fields

```c
#include <poll.h>
int poll(struct pollfd fds[], nfds_t nfds, int timeout);  // timeout in ms

struct pollfd {
    int   fd;       // file descriptor
    short events;   // events to monitor (caller sets, never modified)
    short revents;  // events that occurred (kernel writes on return)
};
```

Key event bits:

| Bit | Direction | Meaning |
|---|---|---|
| `POLLIN` | events + revents | Data available for reading |
| `POLLOUT` | events + revents | Space available for writing |
| `POLLERR` | revents only | Error occurred |
| `POLLHUP` | revents only | Hangup (peer closed) |
| `POLLNVAL` | revents only | FD not open |

`poll()` avoids re-initialization by keeping `events` (input) untouched and writing only
to `revents` (output). To temporarily disable an FD without rebuilding the array, set
`fds[i].fd = -1` — kernel skips negative FD values.

**Poll vs select performance**: on sparse FD sets (e.g., only FD 5000 in range 0–5000),
`poll()` can outperform `select()` because it checks only listed FDs, while `select()`
checks up to `nfds - 1`.

### `epoll` — persistent kernel-side interest list

`epoll` solves the fundamental scaling problem of `select()`/`poll()`: the kernel does not
remember the watch list between calls. With `epoll`, the kernel maintains a **persistent
interest list** internally. After registering FDs once via `epoll_ctl()`, subsequent
`epoll_wait()` calls receive only **ready FDs** — no FD list is passed in.

Three system calls:

```c
#include <sys/epoll.h>

// 1. Create epoll instance — returns an fd referring to the instance
int epfd = epoll_create1(0);

// 2. Register/modify/remove FDs in the interest list
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
// op: EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL

// 3. Wait for events — returns only ready FDs
int epoll_wait(int epfd, struct epoll_event *evlist, int maxevents, int timeout);
```

**epoll_event structure**:

```c
struct epoll_event {
    uint32_t    events;   // bit mask: EPOLLIN, EPOLLOUT, EPOLLET, ...
    epoll_data_t data;    // user data — passed back as-is on ready notification
};

typedef union epoll_data {
    void     *ptr;   // pointer to any user struct
    int       fd;    // file descriptor number
    uint32_t  u32;
    uint64_t  u64;
} epoll_data_t;
```

The `data` field is the only way to identify which FD triggered the event — set
`ev.data.fd = fd` when registering, then read `evlist[i].data.fd` on return.

**epoll event bits**:

| Bit | Meaning |
|---|---|
| `EPOLLIN` | Data available for reading |
| `EPOLLOUT` | Space available for writing |
| `EPOLLHUP` | Hangup on FD |
| `EPOLLERR` | Error — always monitored even if not requested |
| `EPOLLET` | Enable edge-triggered notification for this FD |
| `EPOLLONESHOT` | Notify once, then disable this FD in the interest list |
| `EPOLLRDHUP` | Remote end shut down writing half (stream sockets, since 2.6.17) |

**Why epoll scales**: when `epoll_ctl(ADD)` is called, the kernel attaches a callback to
the underlying open file description. When an I/O event occurs, the kernel directly adds
the FD to the **ready list**. `epoll_wait()` simply retrieves from the ready list — cost
is O(number of events), not O(number of monitored FDs).

Performance data from TLPI (Linux 2.6.25, 100,000 monitoring ops, 1 FD ready per op):

| N FDs monitored | poll() | select() | epoll |
|---|---|---|---|
| 10 | 0.61s | 0.73s | 0.41s |
| 100 | 2.9s | 3.0s | 0.42s |
| 1,000 | 35s | 35s | 0.53s |
| 10,000 | 990s | 930s | 0.66s |

---

## Execution Flow

### select() loop

```
1. FD_ZERO / FD_SET — initialize fd_set (EVERY iteration)
2. select(nfds, &readfds, &writefds, NULL, &timeout)
3. Check return: -1 (error/EINTR), 0 (timeout), >0 (ready count)
4. FD_ISSET(fd, &readfds) → perform I/O on ready FDs
5. Go to 1
```

### poll() loop

```
1. Build pollfd array once (or add/remove as needed)
2. poll(fds, nfds, timeout_ms)
3. Check return: -1, 0, >0
4. Inspect revents for each entry
5. Disable FD: fds[i].fd = -1
6. Go to 2
```

### epoll loop (level-triggered, typical)

```
1. epfd = epoll_create1(0)
2. epoll_ctl(ADD) for each FD of interest
3. Loop:
   a. epoll_wait(epfd, evlist, MAX_EVENTS, timeout)
   b. For each entry in evlist: check events, perform I/O
   c. Use epoll_ctl(MOD/DEL) to update interest list as needed
```

### epoll edge-triggered — the mandatory pattern

```
1. Set every monitored FD to O_NONBLOCK
2. epoll_ctl(ADD, fd, EPOLLIN | EPOLLET)
3. On notification:
   while (read(fd, buf, size) > 0) { process(buf); }
   // loop exits when read() returns -1 with errno == EAGAIN
   // This drains ALL available data — do not miss the EAGAIN check
```

### Waiting on signals and file descriptors together

From TLPI Ch63, there is a race if we install a signal handler and then call `select()`
in separate steps. Two standard solutions are:

1. `pselect()` (and Linux analogs `ppoll()` / `epoll_pwait()`): atomically change
   signal mask during the wait call.
2. Self-pipe trick: signal handler writes one byte to a nonblocking pipe; the read end
   is included in the monitored FD set.

---

## Code Examples

### Example 1: select() — monitor stdin and a pipe simultaneously

```c
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int pipefd[2];
    pipe(pipefd);
    write(pipefd[1], "hello pipe", 10);

    fd_set readfds;
    struct timeval timeout;
    char buf[128];

    while (1) {
        /* Reinitialize every iteration — select() modifies fd_set in-place */
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(pipefd[0], &readfds);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int nfds = pipefd[0] + 1;  /* max_fd + 1 */
        int ready = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ready == -1) { perror("select"); break; }
        if (ready == 0)  { printf("timeout\n"); break; }

        if (FD_ISSET(pipefd[0], &readfds)) {
            int n = read(pipefd[0], buf, sizeof(buf));
            printf("pipe (%d bytes): %.*s\n", n, n, buf);
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int n = read(STDIN_FILENO, buf, sizeof(buf));
            printf("stdin (%d bytes): %.*s\n", n, n, buf);
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
```

### Example 2: poll() — monitor array of FDs, handle disconnect

```c
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_FDS 64

int main(void) {
    struct pollfd fds[MAX_FDS];
    int nfds = 0;

    /* Add stdin to watch list */
    fds[nfds].fd     = STDIN_FILENO;
    fds[nfds].events = POLLIN;
    nfds++;

    char buf[256];
    while (nfds > 0) {
        int ready = poll(fds, nfds, 3000);  /* 3 second timeout */
        if (ready == -1) { perror("poll"); break; }
        if (ready == 0)  { printf("timeout\n"); continue; }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int n = read(fds[i].fd, buf, sizeof(buf) - 1);
                if (n <= 0) {
                    fds[i].fd = -1;  /* disable; poll() skips negative fds */
                    continue;
                }
                buf[n] = '\0';
                printf("fd %d: %s", fds[i].fd, buf);
            }
            if (fds[i].revents & (POLLERR | POLLHUP)) {
                printf("fd %d: error or hangup\n", fds[i].fd);
                fds[i].fd = -1;
            }
        }
    }
    return 0;
}
```

### Example 3: epoll edge-triggered — non-blocking drain loop

```c
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define MAX_EVENTS 10

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    int epfd = epoll_create1(0);

    set_nonblocking(STDIN_FILENO);

    struct epoll_event ev = {
        .events  = EPOLLIN | EPOLLET,  /* edge-triggered */
        .data.fd = STDIN_FILENO,
    };
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    struct epoll_event evlist[MAX_EVENTS];
    char buf[256];

    while (1) {
        int ready = epoll_wait(epfd, evlist, MAX_EVENTS, 5000);
        if (ready == -1) {
            if (errno == EINTR) continue;  /* interrupted by signal, restart */
            perror("epoll_wait");
            break;
        }
        if (ready == 0) { printf("timeout\n"); continue; }

        for (int i = 0; i < ready; i++) {
            if (evlist[i].events & EPOLLIN) {
                /* Edge-triggered: must drain ALL data until EAGAIN */
                while (1) {
                    int n = read(evlist[i].data.fd, buf, sizeof(buf));
                    if (n == -1) {
                        if (errno == EAGAIN) break;  /* no more data */
                        perror("read");
                        break;
                    }
                    if (n == 0) { printf("EOF\n"); goto done; }
                    printf("read %d bytes: %.*s", n, n, buf);
                }
            }
            if (evlist[i].events & (EPOLLHUP | EPOLLERR)) {
                printf("fd %d: hup/err, removing\n", evlist[i].data.fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, evlist[i].data.fd, NULL);
            }
        }
    }
done:
    close(epfd);
    return 0;
}
```

---

## Debugging & Common Pitfalls

### 1. Forgot to reinitialize `fd_set` in select() loop

```c
/* BUG: fd_set from previous iteration is already modified by kernel */
while (1) {
    FD_SET(fd, &readfds);  /* only sets one fd, missing FD_ZERO first */
    select(...);
}

/* FIX */
while (1) {
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    select(...);
}
```

### 2. Edge-triggered epoll: not draining all data

```c
/* BUG: reads only once — remaining data silently stays in buffer */
if (evlist[i].events & EPOLLIN) {
    read(fd, buf, sizeof(buf));
}

/* FIX: loop until EAGAIN */
if (evlist[i].events & EPOLLIN) {
    while ((n = read(fd, buf, sizeof(buf))) > 0) { process(buf, n); }
    /* n == -1 with errno == EAGAIN means "buffer drained" — that is normal */
}
```

### 3. Edge-triggered epoll without O_NONBLOCK

```c
/* BUG: blocking fd + edge-triggered = program hangs after data is drained */
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);  /* fd is still blocking */
/* After EAGAIN-equivalent condition: read() blocks forever instead of returning EAGAIN */

/* FIX: always set O_NONBLOCK on edge-triggered FDs */
set_nonblocking(fd);
```

### 4. EINTR not handled

```c
/* All three APIs return -1 with errno == EINTR when interrupted by a signal */
/* Correct pattern: */
int ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
if (ready == -1 && errno == EINTR) continue;  /* restart the call */
```

### 5. epoll duplicate FD gotcha

If `fd1` is added to epoll and then `fd2 = dup(fd1)` is created, closing `fd1` does **not**
remove it from the epoll interest list — the underlying open file description is still
referenced by `fd2`. The FD is removed only when all duplicates are closed.

### Diagnostic commands

```bash
# Check current FD limits
ulimit -n                         # soft limit
cat /proc/sys/fs/file-max         # system-wide max

# Inspect epoll max watches per user
cat /proc/sys/fs/epoll/max_user_watches

# Trace select/poll/epoll calls for a process
strace -e select,poll,epoll_wait,epoll_ctl -p <pid>

# Check open FDs for a process
ls -l /proc/<pid>/fd
```

---

## Real-World Usage

### Network server with epoll (common pattern)

```
listen_fd  →  accept() new connections
per-client fd  →  EPOLLIN for incoming requests
                  EPOLLOUT when write buffer becomes available
                  EPOLLRDHUP to detect half-close
```

Typical loop (level-triggered, simpler to reason about for most servers):

```c
// Accept new client
if (evlist[i].data.fd == listen_fd) {
    int client_fd = accept(listen_fd, NULL, NULL);
    struct epoll_event cev = { .events = EPOLLIN, .data.fd = client_fd };
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
}
// Handle client data
else {
    // read, process, respond
    // use epoll_ctl(MOD) to add EPOLLOUT when write buffer not empty
}
```

### Embedded: monitor serial + socket + event FD simultaneously

```c
// Example using poll() for portability across UNIX-like systems
struct pollfd fds[] = {
    { .fd = serial_fd, .events = POLLIN },   // UART input
    { .fd = socket_fd, .events = POLLIN },   // network command
    { .fd = event_fd,  .events = POLLIN },   // hardware interrupt via eventfd
};
poll(fds, 3, -1);
```

### libevent abstraction layer

For portable code, `libevent` and similar libraries provide a unified API that
transparently uses `epoll` on Linux, `kqueue` on BSD/macOS, and falls back to `poll()` /
`select()` on other systems.

---

## Key Takeaways

1. **select()**: portable but limited to 1024 FDs, requires reinitializing `fd_set` every
   call (value-result), scans O(nfds) kernel-side.

2. **poll()**: no FD limit, `events`/`revents` separation avoids re-init, use `fd = -1`
   to disable entries. Still O(n FDs) per call.

3. **epoll**: Linux-specific, persistent interest list eliminates per-call copy overhead,
   O(events) scaling — the standard choice for servers monitoring thousands of FDs.

4. **Level-triggered** (default for all three): notify whenever FD is ready — safe with
   blocking FDs, simpler code. **Edge-triggered** (epoll `EPOLLET`): notify only on
   transitions — requires `O_NONBLOCK` + drain-until-`EAGAIN` loop.

5. **Critical edge-triggered rule**: after `EPOLLIN` notification, loop `read()` until
   `EAGAIN`. Missing this means data stays in the buffer and no further notification
   fires until new data arrives.

6. **None of these APIs perform I/O** — they report *when* I/O is possible. Always
   follow with an actual `read()` / `write()` call.

7. **Always handle `EINTR`**: all three return `-1` with `errno == EINTR` when a signal
   interrupts the wait. Check and restart the call.
