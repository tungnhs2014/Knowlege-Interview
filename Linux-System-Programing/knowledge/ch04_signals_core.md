# Chapter 4 - Signals Core

> Topics: 4.1 Signals Fundamentals · 4.2 Signal Handlers · 4.3 Signals Advanced
> Main sources: TLPI Ch20, Ch21, Ch22 · DevLinux Module 04 · Linux man-pages
> Production context: service managers and container platforms graceful shutdown behavior

---

## Coverage Notes

This file covers Chapter 4 mapped rows 4.1, 4.2, and 4.3:

- 4.1 Signals Fundamentals: disposition, default action, `kill()`, signal sets, masks, and pending signals.
- 4.2 Signal Handlers: `sigaction()`, async-signal-safety, `SA_SIGINFO`, `sigaltstack()`, `EINTR`, and `SA_RESTART`.
- 4.3 Signals Advanced: standard versus realtime signals, `sigqueue()`, `sigsuspend()`, `sigwaitinfo()`/`sigwait()`, `signalfd()`, and signal-aware wait patterns.
- Must-cover production concepts: lifecycle/data flow, per-process versus per-thread behavior, blocking versus ignoring, handler safety, standard signal coalescing, debugging evidence, and Embedded constraints.
- Related timer and clock APIs are covered in `ch04_timers_and_time.md`; deep crash dump postmortem belongs with debugging/process material, while this file keeps the signal evidence and core-dump trigger model.

---

## Problem It Solves

A process normally runs its own code in its own flow:

```text
read config -> open files -> accept requests -> do work -> write output
```

But Linux also needs a way to interrupt or notify that process when something important
happens outside that normal flow:

- the user presses `Ctrl+C`;
- a service manager asks the process to stop;
- a child process exits;
- a timer expires;
- a process writes to a pipe or socket whose reader is gone;
- the CPU detects invalid memory access;
- another process sends a simple control notification.

A **signal** is Linux's small notification mechanism for those events.

Precise mental model:

```text
A signal is a small event notification that the kernel delivers
to a process or thread.
```

The signal does not carry a large message. It mostly says:

```text
"This event happened. Apply this signal's action."
```

Example:

```text
User presses Ctrl+C
    |
    v
Terminal driver tells the kernel
    |
    v
Kernel sends SIGINT to the foreground process group
    |
    v
Each process terminates, ignores it, or runs a handler
```

This is why signals feel strange at first: the process is not calling a function by normal
control flow. The kernel diverts the process to a signal action before normal user code
continues.

---

## Learning Roadmap

Learn Chapter 4 in this order:

| Priority | Learn this | Why it matters |
|----------|------------|----------------|
| Must know | signal mental model | without this, every API feels random |
| Must know | lifecycle: generated, pending, blocked, delivered, handled | explains most signal behavior |
| Must know | disposition, mask, pending set | the three core pieces of signal state |
| Must know | `SIGTERM`, `SIGKILL`, `SIGINT`, `SIGCHLD`, `SIGPIPE`, `SIGALRM`, `SIGSEGV` | common production signals |
| Must know | `sigaction()` over `signal()` | production handler installation |
| Must know | minimal handler, async-signal-safe, `sig_atomic_t` | prevents unsafe handler bugs |
| Must know | `EINTR` and `SA_RESTART` | signals interrupt blocking system calls |
| Work useful | graceful shutdown, child reaping, timeout, broken pipe | common production patterns |
| Work useful | `sigsuspend()`, `sigwaitinfo()`, `signalfd()` | safer waiting/integration patterns |
| Recognize | realtime signals, `sigqueue()`, `sigaltstack()`, core dump details | useful in systems work, not first-pass material |

If you remember only one sentence:

```text
Signals are small kernel-delivered notifications; handle them minimally
and do real work in normal code.
```

---

## Core Vocabulary

Read this table before the API sections. These words are the foundation of signal
programming.

| Term | Meaning | Example / note |
|------|---------|----------------|
| Signal | Small event notification delivered by the kernel to a process or thread | `SIGTERM`, `SIGINT`, `SIGCHLD` |
| Signal number | Integer ID of a signal; code should use symbolic names, not raw numbers | use `SIGTERM`, not `15` |
| Signal target | Process, process group, or specific thread that should receive the signal | `kill(pid, SIGTERM)` targets a process |
| Generated | The event happened and the kernel created a signal for a target | child exits -> `SIGCHLD` generated |
| Pending | Signal has arrived but has not been delivered yet, usually because it is blocked | blocked `SIGTERM` waits pending |
| Delivered | Kernel applies the signal action to the target | default action, ignore, or handler |
| Disposition | The configured action for a signal | default, ignore, or run handler |
| Signal handler | User-defined function that runs when a caught signal is delivered | `void handler(int sig)` |
| Signal mask | Set of signals temporarily blocked from delivery | changed with `sigprocmask()` |
| Blocked signal | Signal whose delivery is delayed; it may become pending | block during critical section |
| Ignored signal | Signal that is discarded when delivered | `SIG_IGN` |
| Standard signal | Traditional non-queued signal; one pending bit per signal type | repeated `SIGUSR1` can collapse to one |
| Realtime signal | Queued signal range that can carry a small value | `SIGRTMIN + n`, `sigqueue()` |
| Async-signal-safe | Safe to call from a signal handler | `write()` safe, `printf()` unsafe |
| `sig_atomic_t` | Integer type safe for simple handler-to-main flags | set flag in handler, check in main |
| `EINTR` | Error meaning a blocking syscall was interrupted by a signal handler | retry, stop, or return based on policy |
| `SA_RESTART` | `sigaction()` flag asking Linux to restart some interrupted syscalls | useful for `SIGCHLD`, risky for timeouts |

One common beginner mistake:

```text
Blocked != ignored.

Blocked means: deliver later.
Ignored means: discard.
```

---

## Concept Overview

### Signal vs Normal Function Call

Normal function call:

```text
main code decides to call cleanup()
```

Signal delivery:

```text
kernel diverts the process to the signal action before normal code continues
```

That is the root of the danger. A signal handler can run while the main program is in the
middle of `malloc()`, `printf()`, updating global state, or blocking in `read()`.

### Signal vs IPC vs Exception vs Interrupt

| Mechanism | Receiver | Purpose | Example |
|-----------|----------|---------|---------|
| Hardware interrupt | kernel | device needs service | network card interrupt |
| CPU exception/fault | kernel first, then often process as signal | current instruction failed | invalid memory access -> `SIGSEGV` |
| Signal | process/thread | event notification or process control | `SIGTERM`, `SIGCHLD`, `SIGINT` |
| IPC | process via kernel object/shared memory | exchange data | pipe, socket, message queue, shared memory |

Signals can be used as primitive IPC, but they are not good for data transfer. Use them
for control events. Use proper IPC for real data.

### Signal Lifecycle

This is the most important diagram in the chapter:

```text
Event happens
    |
    v
Signal is generated
    |
    +--> if the signal is blocked
    |       |
    |       v
    |   signal becomes pending
    |       |
    |       v
    |   delivered later when unblocked
    |
    +--> if the signal is not blocked
            |
            v
        signal is delivered
            |
            v
        disposition decides the action
```

Disposition means what happens when the signal is delivered:

```text
default action  OR  ignore  OR  run a handler
```

### The Three Core Concepts

| Concept | Question it answers | Common APIs |
|---------|---------------------|-------------|
| Disposition | What happens when this signal is delivered? | `sigaction()`, `SIG_DFL`, `SIG_IGN` |
| Signal mask | Which signals are temporarily blocked? | `sigprocmask()`, `pthread_sigmask()` |
| Pending set | Which blocked signals already arrived? | `sigpending()` |

Do not move forward until these three are clear.

### Blocking vs Ignoring

| Behavior | Meaning | Later delivery? |
|----------|---------|-----------------|
| Block | delay delivery | yes, signal can remain pending |
| Ignore | discard the signal | no |

Example:

```text
Block SIGTERM:
    "Do not interrupt me right now. Deliver it later."

Ignore SIGTERM:
    "Throw it away."
```

### Pending vs Delivered

| State | Event happened? | Process already acted on it? |
|-------|-----------------|------------------------------|
| Pending | yes | no |
| Delivered | yes | yes |

For standard signals, pending is like a bit, not a counter. If `SIGUSR1` is blocked and
sent 100 times, the process may see only one pending `SIGUSR1`.

Realtime signals are different: they can be queued. That is useful, but still not a
replacement for a real message queue in normal application design.

### Standard vs Realtime Signals

| Signal family | Queued? | Payload? | Typical use |
|---------------|---------|----------|-------------|
| Standard signals | no, one pending bit per signal | signal number only | process control and simple notifications |
| Realtime signals | yes, multiple instances can queue | optional integer/pointer value via `sigqueue()` | small ordered notifications in systems code |

Do not hard-code realtime signal numbers. Use `SIGRTMIN + n` and check that the chosen
value does not exceed `SIGRTMAX`.

---

## System Context

Signals connect many Linux subsystems:

```text
terminal driver       -> Ctrl+C, Ctrl+Z, hangup
process management    -> child exits -> SIGCHLD
timers                -> alarm expires -> SIGALRM
memory/CPU faults     -> invalid access -> SIGSEGV
IPC/file descriptors  -> broken pipe -> SIGPIPE
service managers      -> stop service -> SIGTERM, then SIGKILL
other processes       -> kill(), sigqueue()
```

Real production systems use this pattern constantly:

```text
orchestrator/service manager sends SIGTERM
    |
    v
application stops accepting new work
    |
    v
application finishes or cancels current work
    |
    v
application exits before grace period
    |
    +--> if it does not exit, manager sends SIGKILL
```

This is how systems such as Kubernetes, Cloud Run, Heroku, and systemd expect Linux
services to behave.

---

## Architecture

### What the Kernel Tracks

Simplified process signal state:

```text
process / thread
    |
    +-- signal dispositions
    |      default / ignore / handler
    |
    +-- signal mask
    |      blocked signals
    |
    +-- pending standard signals
    |      one pending bit per standard signal
    |
    +-- queued realtime signals
           multiple queued instances with small values
```

Thread preview:

| State | In multithreaded process |
|-------|---------------------------|
| Disposition | process-wide |
| Signal mask | per-thread |
| Pending signals | can be process-directed or thread-directed |
| Process-directed signal | delivered to one eligible unblocked thread |

For threaded programs, a common design is to block signals in worker threads and receive
them in one dedicated signal-handling thread with `sigwait()` or `sigwaitinfo()`.

### When Signals Are Delivered

For asynchronous signals, the kernel normally delivers a pending unblocked signal when the
process is about to return from kernel mode to user mode:

```text
system call completes
    |
    v
kernel notices pending unblocked signal
    |
    v
handler/default action runs before user code continues
```

That is why a blocking syscall may return early with `EINTR`, and why a signal can appear
to arrive "between" ordinary C statements.

### Default Actions

Signal default actions fall into a few categories:

| Default action | Meaning |
|----------------|---------|
| terminate | process exits abnormally |
| terminate + core | process exits and may produce a core dump |
| stop | process is suspended |
| continue | stopped process resumes |
| ignore | signal is discarded |

`SIGKILL` and `SIGSTOP` are special:

```text
They cannot be caught, ignored, or blocked.
```

That design gives administrators and the kernel a reliable last resort.

---

## Execution Flow

### Flow 1: Graceful Shutdown

```text
systemd / Kubernetes / platform sends SIGTERM
    |
    v
handler sets stop_requested = 1
    |
    v
main loop notices the flag
    |
    v
stop accepting work
    |
    v
close sockets, flush state, release resources
    |
    v
exit normally
```

The handler should not perform the shutdown itself. It should only notify normal code.

### Flow 2: Child Reaping

```text
child exits
    |
    v
kernel stores child status as a zombie record
    |
    v
kernel sends SIGCHLD to parent
    |
    v
parent calls waitpid()
    |
    v
zombie record is released
```

`SIGCHLD` is only the notification. `waitpid()` is what reaps the child.

### Flow 3: Blocking a Signal Around a Critical Section

```text
block SIGINT
    |
    v
update critical state
    |
    +--> SIGINT arrives here -> pending
    |
    v
restore old mask
    |
    v
pending SIGINT is delivered
```

Blocking delays delivery. It does not erase the event.

### Flow 4: Signal Interrupts a Blocking Syscall

```text
process blocks in read()
    |
    v
SIGTERM arrives and handler runs
    |
    v
read() may fail with -1, errno = EINTR
```

This is why signal-aware code needs an `EINTR` policy.

---

## 4.1 Signals Fundamentals

### Must-Know Signals

| Signal | Typical source | Default | What you should know |
|--------|----------------|---------|----------------------|
| `SIGINT` | terminal `Ctrl+C` | terminate | user interrupts foreground job |
| `SIGTERM` | `kill`, systemd, containers | terminate | graceful shutdown request |
| `SIGKILL` | admin/kernel last resort | terminate | cannot catch, ignore, or block |
| `SIGCHLD` | child changes state | ignore | parent should reap children |
| `SIGPIPE` | write to pipe/socket with no reader | terminate | handle `EPIPE` or ignore `SIGPIPE` |
| `SIGALRM` | `alarm()` / timer | terminate | simple timeout signal |
| `SIGSEGV` | invalid memory access | core | crash/debug signal, not normal control flow |
| `SIGHUP` | terminal hangup / daemon convention | terminate | often used by daemons to reload config |
| `SIGUSR1/2` | application-defined | terminate | simple custom notifications |

Important `SIGCHLD` trap:

```text
SIGCHLD's default disposition is "ignore", but a normal parent still
needs wait()/waitpid() to reap child status and avoid zombies.
```

Interview-grade distinction:

```text
SIGTERM asks the process to terminate and can be handled.
SIGKILL forces termination and cannot be handled.
```

### `kill()` Sends Signals

Despite its name, `kill()` sends a signal. It does not necessarily kill.

```c
#include <signal.h>

int kill(pid_t pid, int sig);
```

Targets:

| `pid` | Target |
|-------|--------|
| `> 0` | one process |
| `0` | caller's process group |
| `< -1` | process group `abs(pid)` |
| `-1` | every permitted process, with special exclusions |

`kill(pid, 0)` sends no signal. It checks whether the process exists and whether the
caller has permission to signal it.

### Signal Sets and Signal Mask

Use `sigset_t` to describe a group of signals:

```c
#include <signal.h>

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigismember(const sigset_t *set, int sig);
```

Change the signal mask:

```c
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigpending(sigset_t *set);
```

| `how` | Meaning |
|-------|---------|
| `SIG_BLOCK` | add signals to the current mask |
| `SIG_UNBLOCK` | remove signals from the current mask |
| `SIG_SETMASK` | replace the mask |

Important details:

- `sigpending()` reports blocked signals that have arrived but not yet been delivered;
- unblocking a pending signal causes delivery before normal execution continues;
- attempts to block `SIGKILL` or `SIGSTOP` are ignored.

In multithreaded code, use `pthread_sigmask()` instead of relying on process-centered
intuition.

---

## 4.2 Signal Handlers

### Use `sigaction()` Over `signal()`

`signal()` is simple and appears in many beginner examples, including DevLinux exercises.
For real code, prefer `sigaction()`.

| Aspect | `signal()` | `sigaction()` |
|--------|------------|---------------|
| Handler semantics | historically inconsistent | explicit and POSIX-defined |
| Extra mask during handler | limited | `sa_mask` |
| Restart behavior | not explicit | `SA_RESTART` |
| Extra signal info | no | `SA_SIGINFO` |
| Recommended for production | no | yes |

Typical setup:

```c
struct sigaction sa;

sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sa.sa_handler = handler;

if (sigaction(SIGTERM, &sa, NULL) == -1) {
    /* handle error */
}
```

Useful fields and flags:

| Field/flag | Meaning |
|------------|---------|
| `sa_handler` | simple handler: `void handler(int)` |
| `sa_sigaction` + `SA_SIGINFO` | extended handler with `siginfo_t` |
| `sa_mask` | extra signals blocked while the handler runs |
| `SA_RESTART` | restart some interrupted syscalls |
| `SA_ONSTACK` | run the handler on an alternate stack registered with `sigaltstack()` |
| `SA_NOCLDSTOP` | for `SIGCHLD`, skip child stop/resume notifications |
| `SA_RESETHAND` | reset disposition to default when handler starts |
| `SA_NODEFER` | do not automatically block the current signal during its handler |

Most production handlers need only `sa_handler`, `sa_mask`, and a deliberate choice about
`SA_RESTART`.

### `SA_SIGINFO` and `siginfo_t`

Use `SA_SIGINFO` when the handler needs structured evidence about where the signal came
from or what generated it.

Mechanism:

```text
sigaction() installs sa_sigaction + SA_SIGINFO
    |
    v
kernel delivers signal
    |
    v
handler receives (signal number, siginfo_t *, ucontext pointer)
```

Common useful fields:

| Field | What it can explain |
|-------|---------------------|
| `si_pid` | sender PID for signals sent by another process |
| `si_uid` | real user ID of sender |
| `si_code` | whether signal came from user, kernel, timer, child, fault, etc. |
| `si_value` | small value supplied by `sigqueue()` or POSIX timer setup |
| `si_status` | child status for `SIGCHLD` cases |
| `si_addr` | fault address for signals such as `SIGSEGV` or `SIGBUS` |
| `si_overrun` | missed POSIX timer expirations on Linux |

The third handler argument is a saved user-context pointer. Treat it as advanced crash
tooling material, not normal application state; most production handlers should not poke
registers or resume execution from a fault.

This does not make complex handler work safe. The same async-signal-safety rules apply.
For many services, `SA_SIGINFO` should capture only minimal evidence or notify normal
code to inspect state later.

Typical production uses:

- distinguish user-sent signal versus timer-generated signal;
- inspect sender PID/UID for control-plane debugging;
- receive a small realtime-signal or POSIX-timer payload;
- record crash evidence before terminating.

### `sigaltstack()` and `SA_ONSTACK`

`sigaltstack()` gives selected handlers a separate stack. It matters when the normal
thread stack may already be unusable, most commonly stack overflow or crash handling.

Data flow:

```text
allocate alternate stack
    |
    v
sigaltstack() registers it for this thread
    |
    v
sigaction(..., SA_ONSTACK) selects it for a signal
    |
    v
kernel runs that handler on the alternate stack
```

Important constraints:

- alternate signal stacks are per-thread state, so threaded programs must install them
  for threads that need crash handling;
- the kernel does not grow the alternate stack for you;
- the handler still must avoid unsafe cleanup, heap allocation, logging frameworks, and
  lock-taking code;
- use it to make crash reporting more reliable, not to continue normal execution after
  memory corruption.

Embedded note: stack sizes are often tight. If a product needs a crash handler, budget
the alternate stack deliberately and test stack-overflow behavior on the target build.

### The Handler Rule

A signal handler runs in an interrupted context. It might interrupt unsafe library code.

Bad handler:

```text
printf()
malloc()
free()
exit()
pthread_mutex_lock()
complex business logic
```

Good handler:

```text
set a sig_atomic_t flag
write one byte to a pipe/eventfd
call _exit() only for emergency termination
```

Practical rule:

```text
The handler should notify normal code. Normal code should do the real work.
```

### Async-Signal-Safe

A function is **async-signal-safe** if POSIX guarantees it can be called safely from a
signal handler.

Common safe examples:

```text
write()   _exit()   kill()   getpid()   waitpid()
```

Common unsafe examples:

```text
printf()  fprintf()  malloc()  free()  exit()  syslog()
```

This is one of the most important production signal rules.

### `volatile sig_atomic_t`

Use this for simple handler-to-main notification:

```c
static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}
```

What it guarantees:

- simple reads and writes are atomic with respect to signal interruption;
- `volatile` prevents the compiler from caching the value invisibly.

What it does not guarantee:

```text
flag++ is not guaranteed atomic.
complex shared structures are not safe.
```

### Preserve `errno`

If a handler calls functions that may modify `errno`, preserve it:

```c
static void handler(int sig)
{
    int saved_errno = errno;

    (void)sig;
    /* async-signal-safe work only */

    errno = saved_errno;
}
```

### `SA_RESTART` and `EINTR`

When a handler interrupts a blocking syscall, the syscall may fail:

```text
return value = -1
errno = EINTR
```

`SA_RESTART` asks the kernel to automatically restart some interrupted syscalls.

Use `SA_RESTART` when:

- the signal should not break normal blocking I/O;
- the program can keep waiting.

Avoid `SA_RESTART` when:

- the signal is used to stop the program;
- the signal is used to implement a timeout;
- the event loop must regain control.

Production rule:

```text
Every blocking syscall in signal-aware code needs an EINTR policy.
```

---

## 4.3 Work-Useful Signal Patterns

### Graceful Shutdown

This is the most important signal pattern for backend/cloud services.

What should happen on `SIGTERM`:

1. stop accepting new work;
2. notify worker loops to stop;
3. finish or cancel current work;
4. close sockets/files;
5. exit before the platform sends `SIGKILL`.

This pattern appears in container platforms, process managers, and PaaS environments.

### Child Reaping With `SIGCHLD`

When children exit, the parent receives `SIGCHLD`. The parent must call `wait()` or
`waitpid()` to reap them.

Correct mindset:

```text
SIGCHLD means: "check for child status now."
```

Do not assume one `SIGCHLD` equals one child. Standard signals are not queued reliably as
event counters. Reap in a loop:

```text
while waitpid(-1, &status, WNOHANG) > 0:
    handle child status
```

### `SIGPIPE` in Network and Pipe Code

`SIGPIPE` happens when writing to a pipe/socket whose reader is gone.

Common production choices:

| Choice | Behavior |
|--------|----------|
| default `SIGPIPE` | process terminates |
| ignore `SIGPIPE` | `write()`/`send()` fails with `EPIPE` |
| socket flag such as `MSG_NOSIGNAL` | suppress signal for that send call on Linux |

Backend code often ignores `SIGPIPE` or uses send flags so broken connections become
ordinary I/O errors instead of process termination.

### Timeout With `SIGALRM`

`alarm()` sends `SIGALRM` after a number of seconds. It is simple but process-global and
signal-based.

Use it for:

- simple CLI timeout;
- learning;
- emergency guard in small programs.

Avoid it for:

- libraries;
- event loops;
- multiple concurrent timers;
- precise scheduling.

Timers are covered in `ch04_timers_and_time.md`.

### Daemon Reload With `SIGHUP`

Historically, `SIGHUP` means terminal hangup. Many daemons reuse it as:

```text
"Reload configuration."
```

This is a convention, not a universal kernel rule.

---

## 4.4 Advanced Signals: Recognize First, Deep Dive Later

These topics are real and useful in systems work, but they should not distract from the
main signal model.

| Topic | When it matters | First-pass expectation |
|-------|-----------------|------------------------|
| `sigsuspend()` | avoid unblock-then-wait race | know why `pause()` can race |
| `sigwaitinfo()` / `sigwait()` | synchronous signal handling | know this avoids async handler work |
| `signalfd()` | Linux `epoll` services | know signals can become readable fd events |
| realtime signals | queued tiny notifications | know standard signals are not queued |
| `sigqueue()` | realtime signal with small payload | recognize, not default IPC choice |
| `sigaltstack()` | stack overflow/crash handling | recognize for crash handlers |
| core dumps | debugging crashes | know `SIGSEGV` can produce core dump |
| nonlocal goto from handler | rare recovery pattern | avoid unless deeply understood |

### `sigsuspend()` in One Minute

Buggy idea:

```text
unblock signal
pause()
```

If the signal arrives between those two steps, `pause()` can sleep forever.

`sigsuspend()` atomically changes the mask and waits, so it avoids that race.

### `sigwaitinfo()` in One Minute

Instead of running an async handler:

```text
block SIGTERM
call sigwaitinfo()
handle returned signal in normal code
```

This is easier to reason about in threaded services.

### `signalfd()` in One Minute

Linux-specific pattern:

```text
block signals
create signalfd
add signalfd to epoll
read signal info like normal fd data
```

This is useful when everything else in the service is already fd/event-loop based.

---

## Example

### Example 1 - Graceful Shutdown With `SIGTERM`

```c
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;

static void handle_stop(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static int install_stop_handler(int sig)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handle_stop;

    return sigaction(sig, &sa, NULL);
}

int main(void)
{
    if (install_stop_handler(SIGINT) == -1 ||
        install_stop_handler(SIGTERM) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("pid=%ld; send SIGTERM or press Ctrl+C\n", (long)getpid());

    while (!stop_requested) {
        puts("working");
        sleep(1);
    }

    puts("cleanup in normal code");
    return EXIT_SUCCESS;
}
```

What it teaches:

- `SIGTERM`/`SIGINT` should request shutdown, not do cleanup inside the handler;
- `sigaction()` installs handlers explicitly;
- `volatile sig_atomic_t` is enough for a simple stop flag;
- normal code owns cleanup.

### Example 2 - Reap Children on `SIGCHLD`

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t child_event = 0;

static void handle_sigchld(int sig)
{
    (void)sig;
    child_event = 1;
}

int main(void)
{
    struct sigaction sa;
    sigset_t block_set;
    sigset_t old_mask;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sa.sa_handler = handle_sigchld;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block_set);
    sigaddset(&block_set, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &block_set, &old_mask) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }

        if (pid == 0) {
            _exit(10 + i);
        }
    }

    int remaining = 3;

    while (remaining > 0) {
        while (!child_event) {
            if (sigsuspend(&old_mask) == -1 && errno != EINTR) {
                perror("sigsuspend");
                return EXIT_FAILURE;
            }
        }

        child_event = 0;

        for (;;) {
            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG);

            if (pid > 0) {
                remaining--;
                printf("reaped child %ld\n", (long)pid);
                continue;
            }

            if (pid == 0) {
                break;
            }

            if (errno == ECHILD) {
                remaining = 0;
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("waitpid");
            return EXIT_FAILURE;
        }
    }

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        perror("sigprocmask restore");
        return EXIT_FAILURE;
    }

    puts("all children reaped");
    return EXIT_SUCCESS;
}
```

What it teaches:

- `SIGCHLD` wakes the parent when child state changes;
- the handler only sets a flag;
- `waitpid(-1, ..., WNOHANG)` must loop;
- blocking `SIGCHLD` plus `sigsuspend()` avoids the lost-signal race;
- reaping is what removes zombie records.

### Example 3 - Turn Broken Pipe Into an Error

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int pipefd[2];

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    close(pipefd[0]);

    ssize_t n = write(pipefd[1], "hello\n", strlen("hello\n"));
    if (n == -1 && errno == EPIPE) {
        puts("reader is gone; handle EPIPE in normal code");
    } else if (n == -1) {
        perror("write");
    }

    close(pipefd[1]);
    return 0;
}
```

What it teaches:

- default `SIGPIPE` can terminate the process;
- ignoring `SIGPIPE` lets `write()` fail with `EPIPE`;
- broken connections should usually be handled as normal I/O errors in servers.

### Example 4 - Receive Signals Through `signalfd` (Linux)

```c
#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <unistd.h>

int main(void)
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    int sfd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (sfd == -1) {
        perror("signalfd");
        return EXIT_FAILURE;
    }

    printf("pid=%ld; press Ctrl+C or send SIGTERM\n", (long)getpid());

    for (;;) {
        struct signalfd_siginfo si;
        ssize_t n = read(sfd, &si, sizeof(si));

        if (n != (ssize_t)sizeof(si)) {
            perror("read");
            close(sfd);
            return EXIT_FAILURE;
        }

        printf("received signal %u\n", si.ssi_signo);

        if (si.ssi_signo == SIGINT || si.ssi_signo == SIGTERM) {
            break;
        }
    }

    close(sfd);
    return EXIT_SUCCESS;
}
```

What it teaches:

- block signals before using `signalfd`;
- signal handling can be normal `read()` logic;
- this is useful for Linux event loops with `select()`, `poll()`, or `epoll()`.

### Self-Pipe, `pselect()`, and `ppoll()` in One Minute

Signal-aware event loops need to avoid this race:

```text
check stop flag
    |
    +-- signal arrives here and sets flag
    |
    v
enter select()/poll() forever
```

Three common fixes:

| Pattern | How it works | Best fit |
|---------|--------------|----------|
| self-pipe | handler writes one byte to a pipe; event loop watches the read end | portable fd-based loops |
| `pselect()` / `ppoll()` | atomically changes signal mask while entering the wait | POSIX signal-aware waits |
| `signalfd()` | blocked signals are consumed by reading a Linux fd | Linux `epoll` services |

Self-pipe data flow:

```text
pipe2(O_NONBLOCK | O_CLOEXEC)
    |
    v
handler: write(pipe_write_end, "x", 1)
    |
    v
select/poll/epoll wakes on pipe_read_end
    |
    v
main loop drains pipe and handles flags, shutdown, or child reaping
```

Use a nonblocking pipe and tolerate `EAGAIN`, because the pipe is only a wakeup
mechanism, not a data queue. The real state should be a flag, a child-status loop, or an
application queue updated outside the handler.

`pselect()`/`ppoll()` data flow:

```text
block signal before checking shared state
    |
    v
prepare fd set / pollfd array
    |
    v
pselect()/ppoll() atomically installs wait mask and sleeps
    |
    v
signal handler runs or fd becomes ready
    |
    v
kernel restores previous mask before returning
```

Common bugs:

- using plain `select()`/`poll()` plus manual unblock can reintroduce the lost-wakeup
  race;
- forgetting `O_NONBLOCK` on the self-pipe can deadlock inside the handler;
- forgetting `O_CLOEXEC` leaks the wakeup pipe into child processes;
- expecting one byte per signal is wrong under burst load; coalesce wakeups and inspect
  real state in normal code;
- using `signalfd()` without blocking the same signals lets handlers or default
  dispositions consume them first.

---

## Debugging

Inspect signal state:

```bash
grep -E 'Sig(Pnd|Blk|Ign|Cgt)|ShdPnd|SigQ' /proc/<PID>/status
kill -l
kill -TERM <PID>
kill -0 <PID>
```

Trace signal behavior:

```bash
strace -e trace=signal -p <PID>
strace -e signal=all ./program
strace -e trace=pselect6,ppoll,select,poll,read,write ./program
```

Debug a crash:

```bash
ulimit -c unlimited
cat /proc/sys/kernel/core_pattern
gdb ./program core
```

Common bugs:

| Bug | Why it hurts |
|-----|--------------|
| handler calls `printf()` or `malloc()` | not async-signal-safe |
| handler performs real cleanup | can interrupt unsafe state |
| forget `EINTR` handling | blocking syscalls fail unexpectedly |
| assume one `SIGCHLD` per child | standard signals are not reliable counters |
| use `SIGKILL` for normal shutdown | skips cleanup |
| ignore `SIGPIPE` without checking `EPIPE` | hides broken connection behavior |
| use `alarm()` inside a library | process-global side effect |
| use `signalfd()` without blocking signals | default disposition may run first |

---

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| CLI interruption | handle `SIGINT`, set stop flag |
| service/container shutdown | handle `SIGTERM`, cleanup in main code |
| forced termination | platform sends `SIGKILL` after grace period |
| worker supervision | handle `SIGCHLD`, loop `waitpid()` |
| broken pipe/socket | ignore/suppress `SIGPIPE`, handle `EPIPE` |
| simple timeout | `SIGALRM` / `alarm()` for small programs |
| daemon reload | use `SIGHUP` by convention |
| Linux event loop | `signalfd()` integrated with `epoll` |

---

## Work Checklist

Use this checklist during design and review:

- Install handlers with `sigaction()`, not `signal()`, unless the code is only a small learning example.
- Decide for every handled signal whether `SA_RESTART` is wanted or harmful.
- Keep async handlers minimal: set `sig_atomic_t`, write to a self-pipe, or terminate with `_exit()` in emergencies.
- Preserve `errno` in handlers that call any function that may change it.
- In threaded programs, centralize signal handling with `pthread_sigmask()` plus `sigwait()`/`sigwaitinfo()` or `signalfd()` where possible.
- Never treat standard signals as reliable counters; use IPC, eventfd, pipes, sockets, or realtime signals only when the queueing limits are understood.
- For `SIGCHLD`, loop `waitpid(-1, ..., WNOHANG)` until there are no more waitable children.
- For pipe/socket servers, suppress or ignore `SIGPIPE` only if `EPIPE` is handled.
- For event loops, avoid lost wakeups with self-pipe, `pselect()`/`ppoll()`, or `signalfd()`.
- For Embedded targets, account for stack size, signal latency, watchdog shutdown budgets, and target-specific core-dump availability.

---

## Interview-Relevant Questions

These are the questions that commonly test whether you understand signal behavior, not
just signal names:

1. What is a signal, and why is it called a software interrupt?
2. Explain signal generation, pending state, blocking, delivery, and disposition.
3. What is the difference between blocking a signal and ignoring a signal?
4. Why can `SIGKILL` and `SIGSTOP` not be caught, ignored, or blocked?
5. What is the difference between `SIGTERM` and `SIGKILL` in service shutdown?
6. Why is `sigaction()` preferred over `signal()` for real code?
7. Why must a signal handler be minimal and async-signal-safe?
8. Why are `printf()`, `malloc()`, `free()`, and `exit()` unsafe in handlers?
9. What does `volatile sig_atomic_t` solve, and what does it not solve?
10. What is `EINTR`, and how should blocking syscall code handle it?
11. What does `SA_RESTART` do? When should you avoid it?
12. Why can standard signals not be used as reliable event counters?
13. How should a parent process handle `SIGCHLD` to avoid zombies?
14. Why must `waitpid(-1, ..., WNOHANG)` usually run in a loop?
15. What problem does `sigsuspend()` solve compared with unblock-then-`pause()`?
16. What is `SIGPIPE`, and how do servers usually handle broken connections?
17. How are realtime signals different from standard signals?
18. When would you choose `sigwaitinfo()` or `signalfd()` instead of an async handler?
19. When would `SA_SIGINFO` be useful, and what must the handler still avoid?
20. What problem does `sigaltstack()` solve, and why is it not a normal recovery mechanism?
21. How do self-pipe, `pselect()`/`ppoll()`, and `signalfd()` prevent signal/event-loop races?

---

## Key Takeaways

1. A signal is a small kernel-delivered event notification.
2. The signal lifecycle is generated -> pending if blocked -> delivered -> action.
3. The three core concepts are disposition, mask, and pending set.
4. Blocking delays delivery; ignoring discards the signal.
5. Standard signals are not reliable event counters.
6. Use `sigaction()` for real handler installation.
7. Signal handlers must be minimal and async-signal-safe.
8. Use `volatile sig_atomic_t` for simple handler-to-main flags.
9. `SIGTERM` is for graceful shutdown; `SIGKILL` is the last resort.
10. `SIGCHLD` notifies the parent to reap children with `waitpid()`.
11. `SIGPIPE` can terminate network/pipe programs unless handled.
12. `SA_SIGINFO` adds useful evidence, but does not relax handler-safety rules.
13. `sigaltstack()` helps crash/stack-overflow handlers run when the normal stack is suspect.
14. Use self-pipe, `pselect()`/`ppoll()`, or `signalfd()` to connect signals to event loops without lost wakeups.
15. Advanced APIs such as `sigwaitinfo()` and `signalfd()` are useful, but the core model matters first.

---

## Final Coverage Check

| Required item | Status |
|---------------|--------|
| 4.1 fundamentals | Covered |
| 4.2 handlers, `SA_SIGINFO`, `sigaltstack()` | Covered |
| 4.3 advanced signals and signal-aware wait patterns | Covered |
| Signal lifecycle and data flow | Covered |
| Per-process disposition and per-thread mask behavior | Covered |
| Async-signal-safety and Embedded constraints | Covered |
| Production bugs and debugging commands | Covered |
| Interview readiness | Covered |
