# Chapter 4 — Signals Core
## Topics: 4.1 Signals Fundamentals · 4.2 Signal Handlers · 4.3 Signals Advanced
> Sources: TLPI Ch20, Ch21, Ch22 | DevLinux Module 04

---

## 4.1 Signals Fundamentals

### What Problem Signals Solve

A process runs in isolation — it only sees its own virtual address space. Signals are the mechanism for the kernel or another process to **asynchronously interrupt** a running process and notify it of an event.

Analogous to hardware interrupts (CPU interrupted by a device), a signal interrupts the normal execution flow of a process.

Three concrete scenarios:
- User presses Ctrl+C → terminal driver needs to stop the foreground process immediately
- Child process dies → parent needs to be notified to clean up (avoid zombies)
- Process accesses invalid memory → kernel must terminate it before damage spreads

---

### Signal Lifecycle: Generation → Pending → Delivery

```
Event occurs
     │
[GENERATION]  — kernel writes to target process's task_struct:
               sets bit in pending signal mask
               if signal is blocked → stops here
     │
[PENDING]     — signal waits in kernel, not yet visible to process
     │
     │ (at kernel→user mode transition: after syscall return,
     │  after interrupt handler, when process is rescheduled)
     ▼
[DELIVERY]    — kernel acts on the signal
     │
[ACTION]      — ignore / default action / signal handler
```

**Key timing point:** Kernel delivers a pending signal only at the **transition from kernel mode to user mode**. This is why asynchronous signals cannot be predicted — they arrive at the next safe handoff point, not at the exact moment of generation.

---

### Kernel Storage of Signal State

In each process's `task_struct`:

```
task_struct {
    sigset_t  pending;   ← bit mask: which signals are pending
    sigset_t  blocked;   ← bit mask: signal mask (currently blocked)
    sigaction sa[NSIG];  ← disposition for each signal
}
```

`pending` is a **bit mask, not a queue**. If SIGUSR1 is pending and sent 999 more times → still just 1 bit set. This is why **standard signals are not queued** — delivering more than once requires realtime signals.

---

### Signal Disposition

Disposition defines what happens when a signal is delivered. Three forms:

| Disposition | Meaning |
|---|---|
| `SIG_DFL` | Execute the default action for this signal |
| `SIG_IGN` | Kernel silently discards — process never knows it occurred |
| handler function | Kernel invokes programmer-defined function |

**Five categories of default actions:**

| Action | Meaning |
|---|---|
| `term` | Terminate process |
| `core` | Terminate + produce core dump file |
| `stop` | Suspend process execution |
| `cont` | Resume a stopped process |
| `ignore` | Discard signal |

**Key signals:**

| Signal | No. | Source | Default | Notes |
|---|---|---|---|---|
| SIGKILL | 9 | process/kernel | term | **cannot catch/ignore/block** |
| SIGTERM | 15 | process | term | graceful termination |
| SIGINT | 2 | terminal (Ctrl+C) | term | catchable |
| SIGQUIT | 3 | terminal (Ctrl+\\) | core | quit + core dump |
| SIGSTOP | 19 | process/kernel | stop | **cannot catch/ignore/block** |
| SIGTSTP | 20 | terminal (Ctrl+Z) | stop | catchable |
| SIGCONT | 18 | process/kernel | cont | resume stopped process |
| SIGCHLD | 17 | kernel | ignore | child terminated/stopped |
| SIGSEGV | 11 | hardware/kernel | core | invalid memory access |
| SIGFPE | 8 | hardware/kernel | core | arithmetic exception |
| SIGHUP | 1 | kernel | term | terminal closed / daemon reload |
| SIGPIPE | 13 | kernel | term | write to broken pipe |
| SIGALRM | 14 | kernel | term | real-time timer expired |
| SIGUSR1/2 | 10/12 | process | term | application-defined |

**SIGKILL and SIGSTOP** cannot be caught, ignored, or blocked. This is the kernel's **absolute control escape hatch** — ensures system administrator can always terminate or stop any process.

---

### `signal()` vs `sigaction()`

**`signal()` — old API, do not use in production:**

```c
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
// Returns: previous disposition, or SIG_ERR on error
```

Problem: behavior is inconsistent across UNIX implementations (System V resets handler to `SIG_DFL` after invocation; BSD does not). Cannot read current disposition without changing it.

**`sigaction()` — POSIX standard, use this:**

```c
int sigaction(int sig,
              const struct sigaction *act,
              struct sigaction *oldact);
// Returns: 0 on success, -1 on error
```

```c
struct sigaction {
    void     (*sa_handler)(int);        // handler, SIG_DFL, or SIG_IGN
    sigset_t   sa_mask;                 // signals blocked DURING handler execution
    int        sa_flags;                // control flags
    void     (*sa_restorer)(void);      // internal — do not use directly
};
```

**`sa_flags` summary:**

| Flag | Meaning |
|---|---|
| `SA_RESTART` | Auto-restart syscalls interrupted by this signal |
| `SA_SIGINFO` | Handler receives `siginfo_t` (sender info, fault address) |
| `SA_NODEFER` | Do not block the signal being caught during handler execution |
| `SA_RESETHAND` | Reset disposition to `SIG_DFL` after handler is invoked once |
| `SA_ONSTACK` | Invoke handler on alternate signal stack |

**Canonical usage:**

```c
struct sigaction sa;
sa.sa_handler = my_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;

if (sigaction(SIGINT, &sa, NULL) == -1)
    perror("sigaction");
```

**Read current disposition without changing it:**

```c
struct sigaction old;
sigaction(SIGINT, NULL, &old);   // pass NULL as act → read-only
```

---

### Sending Signals — `kill()` and Related

```c
int kill(pid_t pid, int sig);
// Returns: 0 on success, -1 on error (EPERM or ESRCH)
```

**`pid` interpretation:**

| `pid` value | Signal sent to |
|---|---|
| `> 0` | Process with that PID |
| `== 0` | All processes in caller's process group |
| `== -1` | All processes caller has permission to signal (except init and self) |
| `< -1` | Process group with PGID = `|pid|` |

**Permission rules:**
- Privileged process (CAP_KILL) → can signal any process
- Unprivileged: sender's `real/effective UID` must match receiver's `real UID` or `saved set-user-ID`
- Exception: SIGCONT can be sent to any process in the same session (for job control)

**Null signal — check if process exists:**

```c
if (kill(pid, 0) == 0) {
    // process exists, we have permission to signal it
} else if (errno == EPERM) {
    // process exists, no permission
} else { // errno == ESRCH
    // process does not exist
}
```

Note: process existence ≠ program is still running normally — it may be a zombie.

**Related functions:**

```c
raise(sig);            // send signal to self
                       // equivalent to kill(getpid(), sig) in single-thread
killpg(pgrp, sig);     // send to entire process group
                       // equivalent to kill(-pgrp, sig)
```

---

### Signal Sets and Signal Mask

**Signal set — `sigset_t` (bit mask representing a group of signals):**

```c
sigset_t set;

sigemptyset(&set);              // initialize to empty — MUST call before use
sigfillset(&set);               // initialize to contain all signals

sigaddset(&set, SIGINT);        // add SIGINT
sigdelset(&set, SIGTERM);       // remove SIGTERM
sigismember(&set, SIGINT);      // test membership: returns 1 or 0
```

Do not use `memset()` or zero-init a `sigset_t` — it may not be a simple bit mask on all implementations.

**Signal mask — `sigprocmask()` (signals currently blocked for this process):**

```c
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
```

| `how` | Operation |
|---|---|
| `SIG_BLOCK` | `mask = mask ∪ set` |
| `SIG_UNBLOCK` | `mask = mask ∖ set` |
| `SIG_SETMASK` | `mask = set` |

`oldset` (if not NULL) returns the previous mask.

**Standard pattern — protect a critical section:**

```c
sigset_t blockSet, prevMask;
sigemptyset(&blockSet);
sigaddset(&blockSet, SIGINT);

sigprocmask(SIG_BLOCK, &blockSet, &prevMask);   // block
// --- critical section ---
sigprocmask(SIG_SETMASK, &prevMask, NULL);       // restore
// → any pending SIGINT is delivered immediately here
```

SIGKILL and SIGSTOP cannot be blocked — `sigprocmask()` silently ignores them.

**Pending signals — `sigpending()`:**

```c
sigset_t pending;
sigpending(&pending);   // get set of currently pending signals
```

---

### Standard Signals Are NOT Queued

The pending signal set is a **bit mask** — it records whether a signal has occurred, not how many times. If the same signal is generated multiple times while blocked, it is delivered exactly **once** when unblocked.

Proof from TLPI: sending SIGUSR1 one million times while the receiver has it blocked → signal caught **1 time** after unblock.

Implication: do not use standard signals to count events. If counting is required → use **realtime signals** (Section 4.3).

---

## 4.2 Signal Handlers

### The Fundamental Problem: Reentrancy

A signal handler can interrupt the main program **at any instruction** — including in the middle of `malloc()`, `printf()`, `strtok()`. Both the main program and the handler form two concurrent execution contexts on the **same stack** within the same process.

A function is **reentrant** if it can be safely executed by multiple concurrent execution contexts. It is non-reentrant if it uses:
- Static or global data structures
- Internal state shared between calls
- Non-reentrant functions internally

**Concrete failure example:**

```
Main program is inside malloc() — updating the heap free-block linked list
                        ↓
    Signal arrives → handler runs
                        ↓
    Handler also calls malloc()
                        ↓
    Both modify the same linked list → HEAP CORRUPTION
```

---

### Async-Signal-Safe Functions

An **async-signal-safe** function is guaranteed by the implementation to be safe when called from a signal handler (either it is reentrant, or it cannot be interrupted by a signal handler).

**Safe to call in handlers:**

```
write()    read()    open()    close()    fork()    execve()
_exit()    kill()    raise()   signal()   sigaction()
sigprocmask()   sigpending()   sigsuspend()
getpid()   getppid()   getuid()   geteuid()
```

**NOT safe (common mistakes):**

```
printf()  fprintf()  sprintf()   ← stdio uses internal buffers
malloc()  free()  realloc()      ← heap linked list management
exit()                           ← flushes stdio before _exit()
strtok()  strerror()  getenv()   ← use static storage
syslog()                         ← uses static buffer
```

Rule of thumb: if the manual page mentions "not async-signal-safe", "uses static storage", or "not reentrant" → **do not call it from a handler**.

---

### `volatile sig_atomic_t` — Safe Communication with Handler

The best signal handler is one that **only sets a flag**:

```c
volatile sig_atomic_t got_sigterm = 0;

void sigterm_handler(int sig) {
    got_sigterm = 1;   // only this
}

int main() {
    /* install handler... */
    while (!got_sigterm) {
        /* main work */
    }
    /* cleanup */
}
```

**Why `volatile`:** Prevents the compiler from caching `got_sigterm` in a register. Forces every read to go to memory, so the main program always sees the handler's write.

**Why `sig_atomic_t`:** Read/write of ordinary variables may require multiple machine instructions. If a signal arrives mid-write, the value can be corrupt. `sig_atomic_t` is guaranteed to be read/written atomically.

**Important:** `sig_atomic_t` is safe only for simple **set and check**. Do not use `++` or `--` — those are not atomic.

**Save/restore `errno` in handlers:**

```c
void handler(int sig) {
    int saved_errno = errno;   // save
    /* call async-safe functions that may modify errno */
    errno = saved_errno;       // restore
}
```

---

### Design Patterns for Signal Handlers

**Pattern 1 — Flag + main loop (recommended):**

```c
volatile sig_atomic_t got_signal = 0;

void handler(int sig) { got_signal = 1; }

int main() {
    struct sigaction sa = { .sa_handler = handler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    while (!got_signal) { /* work */ }
    /* cleanup */
}
```

**Pattern 2 — Self-pipe trick (for event-driven I/O loops):**

```c
int pipefd[2];

void handler(int sig) {
    int saved_errno = errno;
    write(pipefd[1], "x", 1);   // write() is async-signal-safe
    errno = saved_errno;
}

int main() {
    pipe(pipefd);
    /* set pipefd[1] to non-blocking */
    /* install handler */
    /* add pipefd[0] to select/poll/epoll fd set */
    /* when pipefd[0] is readable → a signal arrived */
}
```

**Terminating in handler:**

```c
void cleanup_handler(int sig) {
    unlink("/tmp/lockfile");   // async-safe
    _exit(EXIT_FAILURE);       // _exit(), NOT exit()
}
```

---

### Nonlocal Goto from Handler — `sigsetjmp()` / `siglongjmp()`

**Problem with `longjmp()` from a handler:**

When a handler is invoked, the kernel automatically adds the triggering signal to the signal mask. On normal return, the mask is restored. But if the handler exits via `longjmp()` → **signal mask is not restored** → that signal blocked forever.

**Solution — `sigsetjmp()` + `siglongjmp()`:**

```c
#include <setjmp.h>

static sigjmp_buf env;
static volatile sig_atomic_t can_jump = 0;   // guard variable

void handler(int sig) {
    if (!can_jump) return;   // env not yet initialized
    siglongjmp(env, 1);      // restores signal mask automatically
}

int main() {
    struct sigaction sa = { .sa_handler = handler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    if (sigsetjmp(env, 1) == 0) {   // 1 = save signal mask
        can_jump = 1;
        /* main flow */
    } else {
        /* jumped here from handler; signal mask restored */
    }
}
```

`sigsetjmp(env, 1)` saves the current signal mask into `env`. `siglongjmp()` restores it when jumping. The **guard variable** `can_jump` prevents a jump from an uninitialized buffer if the signal arrives before `sigsetjmp()` is called.

---

### `sigaltstack()` — Alternate Signal Stack

**Problem:** Stack overflow → SIGSEGV is generated → kernel tries to create a stack frame for the SIGSEGV handler → **no stack space** → handler cannot run → process dies silently.

**Solution:** Allocate a separate memory region as an alternate stack, tell the kernel about it, and establish handlers with `SA_ONSTACK`.

```c
#include <signal.h>

stack_t altstack;
altstack.ss_sp    = malloc(SIGSTKSZ);   // SIGSTKSZ = 8192 bytes
altstack.ss_size  = SIGSTKSZ;
altstack.ss_flags = 0;

if (sigaltstack(&altstack, NULL) == -1)
    perror("sigaltstack");

struct sigaction sa;
sa.sa_handler = sigsegv_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_ONSTACK;    // ← use alternate stack for this handler

sigaction(SIGSEGV, &sa, NULL);
```

```c
typedef struct {
    void   *ss_sp;     // pointer to alternate stack memory
    int     ss_flags;  // SS_ONSTACK (currently in use) or SS_DISABLE
    size_t  ss_size;   // size of the stack
} stack_t;
```

Use case: embedded/real-time systems that need to catch SIGSEGV for logging and restarting rather than crashing silently.

---

### `SA_SIGINFO` — Additional Signal Information

Standard handler receives only the signal number. To know **who sent it, from where, why** → use `SA_SIGINFO`.

```c
void handler(int sig, siginfo_t *info, void *ucontext);

struct sigaction sa;
sa.sa_sigaction = handler;         // NOTE: sa_sigaction, NOT sa_handler
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_SIGINFO;
sigaction(SIGUSR1, &sa, NULL);
```

**`siginfo_t` key fields:**

```c
typedef struct {
    int     si_signo;   // signal number
    int     si_code;    // origin: SI_USER, SI_KERNEL, SI_QUEUE, SI_TIMER...
    pid_t   si_pid;     // PID of sending process (if from kill/sigqueue)
    uid_t   si_uid;     // real UID of sender
    void   *si_addr;    // faulting address (SIGSEGV, SIGBUS, SIGILL, SIGFPE)
    int     si_status;  // exit status or signal number (SIGCHLD)
} siginfo_t;
```

**`si_code` key values:**

| `si_code` | Meaning |
|---|---|
| `SI_USER` | Sent by `kill()` from a user process |
| `SI_KERNEL` | Sent by kernel |
| `SI_QUEUE` | Sent by `sigqueue()` (realtime signal) |
| `SI_TIMER` | POSIX timer expired |
| `CLD_EXITED` | Child exited (SIGCHLD) |
| `CLD_KILLED` | Child killed (SIGCHLD) |
| `SEGV_MAPERR` | Address not mapped in address space (SIGSEGV) |

---

### `SA_RESTART` — System Call Interruption

When a blocking syscall is waiting and a signal arrives → handler runs → handler returns → **syscall fails with `errno = EINTR`**.

```
Process is blocked in read() waiting for data
                   ↓
        SIGALRM arrives → handler runs → returns
                   ↓
    read() returns -1, errno = EINTR
```

**Option 1 — Manual restart:**

```c
ssize_t n;
while ((n = read(fd, buf, sizeof(buf))) == -1 && errno == EINTR)
    continue;
if (n == -1) perror("read");
```

**Option 2 — `SA_RESTART` flag (kernel handles it):**

```c
sa.sa_flags = SA_RESTART;
```

**Syscalls that DO auto-restart with `SA_RESTART`:**
- `read()`, `write()`, `readv()`, `writev()` on slow devices (terminal, pipe, socket)
- `wait()`, `waitpid()`, `wait3()`, `wait4()`
- `accept()`, `connect()`, `send()`, `recv()` (unless socket timeout is set)
- `flock()`, `fcntl()` (file locking)
- `sem_wait()`, `pthread_mutex_lock()`, `pthread_cond_wait()`

**Syscalls that NEVER auto-restart (always EINTR):**
- `select()`, `poll()`, `epoll_wait()` → **always handle EINTR manually**
- `sleep()`, `nanosleep()`
- `pause()`, `sigsuspend()`, `sigwaitinfo()`
- System V IPC: `semop()`, `msgrcv()`, `msgsnd()`

---

## 4.3 Signals Advanced

### Hardware-Generated Signals — Do Not Return from Handler

`SIGSEGV`, `SIGFPE`, `SIGBUS`, `SIGILL` are generated by hardware exceptions.

**What NOT to do:**
- **Return normally from handler** → CPU resumes the faulting instruction → signal generated again → infinite loop
- **Ignore** → Linux forces delivery regardless (hardware exceptions cannot be ignored)
- **Block** → Linux 2.6+: process is killed immediately even if a handler is installed

**Correct approach:**

```c
void sigsegv_handler(int sig) {
    /* cleanup if needed using only async-safe functions */
    _exit(EXIT_FAILURE);        // terminate immediately
    /* OR: siglongjmp(env, 1); to jump to a recovery point */
}
```

---

### Synchronous vs Asynchronous Signal Generation

| Type | Examples | Timing |
|---|---|---|
| **Asynchronous** | `kill()` from another process, Ctrl+C, SIGCHLD | Unpredictable — arrive at the next kernel→user transition |
| **Synchronous** | `raise()`, hardware exception, `kill(getpid(), ...)` | Delivered immediately — before the triggering call returns |

**Order when multiple signals are simultaneously unblocked:** Linux delivers in ascending signal number order. However, SUSv3 does not mandate this — do not rely on specific delivery order.

---

### Realtime Signals

Standard signals have structural limitations that realtime signals address:

| Property | Standard signals (1–31) | Realtime signals (SIGRTMIN–SIGRTMAX) |
|---|---|---|
| User-available range | 2 (SIGUSR1, SIGUSR2) | 32 on Linux |
| Queuing | No — pending is a bit | **Yes — each instance queued separately** |
| Associated data | None | **Integer or pointer** |
| Delivery order of different signals | Not guaranteed | **Lowest number first** |
| Multiple copies | Only delivered once | **Delivered exactly as many times as sent** |

**Referencing realtime signal numbers:**

```c
SIGRTMIN          // lowest realtime signal (usually 34 with NPTL)
SIGRTMAX          // highest realtime signal (usually 64)

SIGRTMIN + 0      // first realtime signal
SIGRTMIN + 1      // second realtime signal
```

NPTL (Linux threading library) internally uses the first 2 realtime signals, so `SIGRTMIN = 34`, not 32. **Never hard-code integer values** — always use `SIGRTMIN + n`.

**Queuing limit:** controlled by `RLIMIT_SIGPENDING` (per real user ID). When the limit is exceeded, `sigqueue()` fails with `errno = EAGAIN`.

---

### `sigqueue()` — Send Realtime Signal with Data

```c
#define _POSIX_C_SOURCE 199309
#include <signal.h>

int sigqueue(pid_t pid, int sig, const union sigval value);
// Returns: 0 on success, -1 on error
```

```c
union sigval {
    int   sival_int;   // integer data
    void *sival_ptr;   // pointer data (rarely useful between processes)
};
```

**Send with integer data:**

```c
union sigval sv;
sv.sival_int = 42;
sigqueue(target_pid, SIGRTMIN, sv);
```

**Receive data via `SA_SIGINFO` handler:**

```c
void handler(int sig, siginfo_t *info, void *ucontext) {
    // info->si_code == SI_QUEUE  when sent via sigqueue()
    // info->si_value.sival_int   contains the data
    // info->si_pid               PID of sender
}
```

`sigqueue()` can only target a single process (unlike `kill()` which can target process groups).

---

### `sigsuspend()` — Atomic Unblock + Wait

**The race condition problem:**

```c
/* BUG: race condition between unblock and wait */
sigprocmask(SIG_SETMASK, &prevMask, NULL);   // unblock
/* ← signal could be delivered HERE and handled */
pause();   /* waits forever — signal already handled */
```

**`sigsuspend()` solves this atomically:**

```c
int sigsuspend(const sigset_t *mask);
// Returns: always -1 with errno = EINTR
```

`sigsuspend(mask)` performs these three steps **atomically**:
1. Replace signal mask with `mask`
2. Suspend process
3. On signal handler return: **auto-restore** signal mask to pre-call value

```c
/* Correct pattern */
sigset_t blockSet, prevMask;
sigemptyset(&blockSet);
sigaddset(&blockSet, SIGINT);

sigprocmask(SIG_BLOCK, &blockSet, &prevMask);

/* --- critical section --- */

/* Atomic: unblock SIGINT + suspend + restore mask after handler returns */
sigsuspend(&prevMask);

/* Here: signal mask is restored, SIGINT has been handled */
```

`sigsuspend()` always returns -1 with `errno = EINTR` — this is not an error, it signals that a handler was invoked and returned.

---

### `sigwaitinfo()` / `sigtimedwait()` — Synchronous Signal Acceptance

Instead of an async handler, **actively wait for a signal** in the main execution flow. No handler needed. No async-safety concerns.

```c
#define _POSIX_C_SOURCE 199309
#include <signal.h>

int sigwaitinfo(const sigset_t *set, siginfo_t *info);
// Returns: signal number on success, -1 on error

int sigtimedwait(const sigset_t *set, siginfo_t *info,
                 const struct timespec *timeout);
// Returns: signal number on success, -1 on error/timeout (EAGAIN)
```

**Standard pattern:**

```c
sigset_t waitSet;
siginfo_t info;

sigemptyset(&waitSet);
sigaddset(&waitSet, SIGUSR1);
sigaddset(&waitSet, SIGRTMIN);

/* MUST block first — so signals don't get handled by disposition */
sigprocmask(SIG_BLOCK, &waitSet, NULL);

while (1) {
    int sig = sigwaitinfo(&waitSet, &info);
    if (sig == -1) { perror("sigwaitinfo"); break; }

    printf("Got signal %d from PID %d, data=%d\n",
           sig, info.si_pid, info.si_value.sival_int);

    if (sig == SIGTERM) break;
}
```

**`sigtimedwait()` with timeout:**

```c
struct timespec timeout = { .tv_sec = 5, .tv_nsec = 0 };
int sig = sigtimedwait(&waitSet, &info, &timeout);
if (sig == -1 && errno == EAGAIN) {
    /* timed out — no signal arrived within 5 seconds */
}
```

Advantages over async handler:
- Normal C code — no async-signal-safe restrictions
- No `volatile sig_atomic_t` needed
- Realtime signals fetched in order (lowest number first)
- `siginfo_t` gives full sender information

---

### `signalfd()` — Signals as a File Descriptor

Converts signal delivery into a readable file descriptor event. Signals become I/O events that integrate naturally with `select()`/`poll()`/`epoll`.

```c
#include <sys/signalfd.h>

int signalfd(int fd, const sigset_t *mask, int flags);
// fd = -1: create new fd;  or existing signalfd fd to update mask
// flags: 0, SFD_CLOEXEC, SFD_NONBLOCK
// Returns: file descriptor, or -1 on error
```

**Standard pattern:**

```c
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGTERM);
sigaddset(&mask, SIGRTMIN);

/* MUST block — otherwise signals handled before we can read them */
sigprocmask(SIG_BLOCK, &mask, NULL);

int sfd = signalfd(-1, &mask, SFD_CLOEXEC);

/* Read signals */
struct signalfd_siginfo fdsi;
ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
if (s == sizeof(fdsi)) {
    printf("Signal %d from PID %d\n", fdsi.ssi_signo, fdsi.ssi_pid);
}

close(sfd);
```

**`signalfd_siginfo` key fields:**

```c
struct signalfd_siginfo {
    uint32_t ssi_signo;   // signal number
    int32_t  ssi_code;    // SI_USER, SI_QUEUE, SI_KERNEL...
    uint32_t ssi_pid;     // sender PID
    uint32_t ssi_uid;     // sender real UID
    int32_t  ssi_int;     // integer data from sigqueue()
    uint64_t ssi_ptr;     // pointer data from sigqueue()
    int32_t  ssi_status;  // exit status (SIGCHLD)
    uint64_t ssi_addr;    // faulting address (SIGSEGV, SIGBUS)
};
```

**`signalfd()` + `epoll` — unified event loop (production pattern):**

```c
int epfd = epoll_create1(EPOLL_CLOEXEC);

struct epoll_event ev = { .events = EPOLLIN, .data.fd = sfd };
epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);
/* also add other fds (sockets, pipes, etc.) */

struct epoll_event events[10];
while (1) {
    int n = epoll_wait(epfd, events, 10, -1);
    for (int i = 0; i < n; i++) {
        if (events[i].data.fd == sfd) {
            struct signalfd_siginfo fdsi;
            read(sfd, &fdsi, sizeof(fdsi));
            /* handle signal in normal code — no async restrictions */
        } else {
            /* handle I/O event */
        }
    }
}
```

This replaces the self-pipe trick. Signals and I/O events are handled in the **same event loop** with the **same code style**.

---

### Choosing Between Synchronous Signal Approaches

| Method | API | When to use |
|---|---|---|
| Handler + `sigsuspend()` | Classic POSIX | Portable code, handler must run immediately on signal arrival |
| `sigwaitinfo()` | POSIX.1b | Dedicated signal-handling thread, simple synchronous waiting |
| `signalfd()` | Linux-specific | Event-driven architecture with epoll, signals as I/O events |

---

## System Context — Signals in the Linux Architecture

**Where signals fit:**
Signals are a kernel subsystem for asynchronous event notification between hardware, kernel, and processes. Generated by multiple sources, delivered through a single mechanism stored in `task_struct`.

```
Hardware (MMU page fault, FPU)       ──┐
Terminal driver (Ctrl+C, hangup)     ──┤
Another process (kill, sigqueue)     ──┼──► Signal subsystem in task_struct:
Kernel (SIGCHLD, SIGPIPE, SIGALRM)  ──┘         pending  (bit mask / rt linked list)
                                                  blocked  (signal mask)
                                                  sigaction table
                                                       │
                          ┌────────────────────────────┤
                          │                            │
               ┌──────────▼──────┐         ┌──────────▼──────────┐
               │  SCHEDULER      │         │  MEMORY MANAGER     │
               │                 │         │                     │
               │ TASK_INTERRUP-  │         │ page fault →        │
               │ TIBLE sleep     │         │ SIGSEGV             │
               │ → woken by      │         │ core dump →         │
               │ signal (EINTR)  │         │ write via VFS       │
               └─────────────────┘         └─────────────────────┘
```

**Subsystem interactions:**
- **Scheduler:** `TASK_INTERRUPTIBLE` sleep is interrupted by a signal → syscall returns `EINTR`. `TASK_UNINTERRUPTIBLE` (D state, disk I/O) is **immune** — signal is queued, delivered on wakeup.
- **Memory Manager:** illegal memory access → MMU page fault → `do_page_fault()` → kernel raises `SIGSEGV` on the process. Core dump writes the process address space via `vfs_write()`.
- **Terminal driver:** Ctrl+C → `SIGINT`; Ctrl+Z → `SIGTSTP`; terminal hangup → `SIGHUP` to the entire foreground process group.
- **Process hierarchy:** child `exit()` → `SIGCHLD` to parent; `exec()` resets all non-ignored dispositions to `SIG_DFL`.
- **VFS:** `write()` to a pipe with no readers → `SIGPIPE` generated by kernel and sent to the writing process.

**Failure scenarios:**
- `sigqueue()` exceeds `RLIMIT_SIGPENDING` → returns `EAGAIN` → **signal silently dropped**; always check return value.
- Hardware signals (`SIGSEGV`, `SIGFPE`) set to `SIG_IGN` → kernel overrides and **forces delivery**; returning from handler → re-executes faulting instruction → **infinite loop**.
- `sigaltstack()` not configured + stack overflow → `SIGSEGV` handler cannot run (no stack space) → **process killed silently**.
- Signal sent to a zombie process → silently discarded (zombie has no execution context).
- Blocking `SIGKILL`/`SIGSTOP` via `sigprocmask()` → silently ignored; they are always deliverable.

**Signal delivery flow:**

```
SENDER                         KERNEL                       RECEIVER
                          ┌────────────────┐
kill()       ────────────>│ task_struct    │
sigqueue()   ────────────>│   pending      │── standard: bit mask, no queue
raise()      ────────────>│   (bit mask)   │── realtime: linked list, queued
hardware     ────────────>│                │
terminal     ────────────>└────────────────┘
                                  │
                        delivery at kernel→user
                        mode transition
                                  │
            ┌─────────────────────┴──────────────────────┐
      async handler                               sync receive
      (sa_handler)                                sigwaitinfo()
      (sa_sigaction)                              signalfd()
            │                                          │
    on process stack                         in thread/event loop
    async-safe only                          normal code OK
    volatile sig_atomic_t                    siginfo_t available
```

---

## Debugging

**Inspect signal state via `/proc`:**

```bash
# Show signal bitmasks for a running process
cat /proc/<PID>/status | grep -E "Sig(Blk|Pnd|Cgt|Ign)"
# SigBlk: blocked signals (hex bitmask)
# SigPnd: pending signals
# SigCgt: signals with installed handler
# SigIgn: ignored signals

# Decode hex bitmask to signal names
python3 -c "
import signal
mask = 0x0000000000003000   # replace with actual value from status
print([s.name for s in signal.Signals if mask & (1 << (s - 1))])
"

# Check realtime signal queue depth
cat /proc/<PID>/status | grep SigQ
# SigQ: 3/31609  → 3 queued, limit 31609 (RLIMIT_SIGPENDING)
```

**Trace signal syscalls:**

```bash
strace -e trace=signal -p <PID>     # trace signal calls for running process
kill -l                             # list all signal names and numbers
kill -SIGUSR1 <PID>                 # send signal for testing
kill -RTMIN+1 <PID>                 # send SIGRTMIN+1
```

**GDB signal debugging:**

```
(gdb) info signals              # all signals and GDB handling policy
(gdb) handle SIGINT nopass      # catch in GDB, do not pass to program
(gdb) handle SIGUSR1 pass print # pass to program, print notification
(gdb) catch signal SIGSEGV      # breakpoint on signal arrival
(gdb) backtrace                 # shows: sigreturn → handler → interrupted frame
```

**Common bugs to catch in review:**

```c
/* BUG: calling non-async-safe function in handler */
void bad_handler(int sig) {
    printf("signal!\n");  /* NOT async-signal-safe */
    free(ptr);            /* NOT async-signal-safe */
}

/* BUG: forgetting to save/restore errno */
void handler(int sig) {
    write(fd, buf, n);    /* write() may modify errno on error */
    /* main program's errno checks now see wrong value */
}

/* FIX */
void good_handler(int sig) {
    int saved_errno = errno;
    write(fd, buf, n);
    errno = saved_errno;
    got_signal = 1;        /* only set a flag */
}
```

---

## Real-world Usage

| Use case | Signal / API | Pattern |
|---|---|---|
| Graceful server shutdown | `SIGTERM` | Handler sets flag → main loop exits cleanly, drains connections |
| Force terminate hung process | `SIGKILL` | Last resort from process manager — cannot be caught |
| Daemon config reload | `SIGHUP` | `kill -HUP $(pidof nginx)` → re-reads config without restart |
| Process pool reaping | `SIGCHLD` | `waitpid(-1, NULL, WNOHANG)` in handler to collect zombies |
| Check process liveness | `kill(pid, 0)` | 0 = exists; `ESRCH` = gone; `EPERM` = exists, no permission |
| IPC with typed data | `sigqueue()` + `SA_SIGINFO` | Pass job ID as `sival_int` to worker process |
| Event-driven signal handling | `signalfd()` + `epoll` | systemd, s6: signals as I/O events in unified event loop |
| Stack overflow diagnosis | `SIGSEGV` + `sigaltstack` + `SA_ONSTACK` | Log diagnostics, then `_exit()` — critical in embedded/RT |
| Syscall timeout | `alarm()` + no `SA_RESTART` + `EINTR` | Use `pselect()`/`ppoll()` to eliminate race condition |

---

## Key Takeaways — Chapter 4 Signals

### Topic 4.1 — Fundamentals
1. Signal = software interrupt — kernel interrupts process at any instruction to notify an event
2. Lifecycle: Generation → Pending (if blocked) → Delivery (at kernel→user transition) → Action
3. Kernel stores state in `task_struct`: pending bit mask, blocked mask, sigaction table
4. Handler runs on the **same stack** — can interrupt main program at any point
5. Use `sigaction()`, not `signal()` — portable, flexible, has `sa_mask` and `sa_flags`
6. SIGKILL and SIGSTOP cannot be caught, ignored, or blocked — kernel's absolute control
7. `kill(pid, 0)` = null signal trick to check process existence
8. Standard signals are not queued — pending is a bit mask, not a counter

### Topic 4.2 — Signal Handlers
9. Non-reentrant functions (malloc, printf, exit...) must never be called from a handler
10. Always save/restore `errno` if calling any async-safe function from a handler
11. Best pattern: handler only sets `volatile sig_atomic_t flag`; main program checks it
12. Use `siglongjmp()` not `longjmp()` for nonlocal goto from handlers — to restore signal mask
13. `sigaltstack()` + `SA_ONSTACK` is required to handle SIGSEGV from stack overflow
14. `SA_RESTART` auto-restarts many blocking syscalls — but `select()`/`poll()` always need manual EINTR handling

### Topic 4.3 — Advanced
15. Hardware-generated signals: do not return from handler, do not ignore, do not block
16. Realtime signals: queued, carry data, guaranteed delivery order (lowest first)
17. `SIGRTMIN` is not 32 on systems using NPTL — always use `SIGRTMIN + n`, never hard-coded integers
18. `sigqueue()` sends realtime signal with integer/pointer data — receiver needs `SA_SIGINFO`
19. `sigsuspend()` solves the race condition between `sigprocmask()` + `pause()` — atomic operation
20. `sigwaitinfo()` accepts signals synchronously — no handler, normal code, full siginfo
21. `signalfd()` + `epoll` = modern pattern — signals become I/O events in unified event loop
