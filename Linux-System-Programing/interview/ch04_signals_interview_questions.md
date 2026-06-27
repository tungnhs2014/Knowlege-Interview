# Chapter 4 Interview - Signals, Timers, and Time

> Scope: Linux/POSIX signals, signal handlers, masks, pending signals, synchronous signal APIs, service shutdown, `SIGCHLD`, `SIGPIPE`, realtime signals, signal-aware waits, timers, sleeping, and time API choices.
> Primary repo sources: `knowledge/ch04_signals_core.md`, `knowledge/ch04_timers_and_time.md`.
> Supporting repo sources: TLPI-derived docs `ch20`, `ch21`, `ch22`, `ch23`, `ch10`, plus DevLinux Module 04 and its signal exercises.

---

## Review Basis

This interview set was reviewed and tightened against the Chapter 4 knowledge files first, then checked against the mapped TLPI-derived docs and DevLinux practice material. Existing questions were kept only when they matched the scenario-first interview standard.

Correctness sources:

- Repo knowledge: signals core and timers/time API notes.
- TLPI-derived docs: signal lifecycle, dispositions, masks, pending state, handler safety, `sigaction()`, `EINTR`, `SA_RESTART`, realtime signals, `sigsuspend()`, `sigwaitinfo()`, `signalfd()`, timers, sleeping, clocks, calendar time, and process CPU time.
- DevLinux Module 04: practical examples for `SIGINT`, `SIGALRM`, `SIGUSR1`, `SIGTSTP`, `kill()`, `pause()`, `select()`, and signal interruption.
- Linux man-pages: [`signal(7)`](https://man7.org/linux/man-pages/man7/signal.7.html), [`sigaction(2)`](https://man7.org/linux/man-pages/man2/sigaction.2.html), [`signal-safety(7)`](https://man7.org/linux/man-pages/man7/signal-safety.7.html), [`kill(2)`](https://man7.org/linux/man-pages/man2/kill.2.html), [`sigprocmask(2)`](https://man7.org/linux/man-pages/man2/sigprocmask.2.html), [`pthread_sigmask(3)`](https://man7.org/linux/man-pages/man3/pthread_sigmask.3.html), [`sigsuspend(2)`](https://man7.org/linux/man-pages/man2/sigsuspend.2.html), [`pselect(2)`](https://man7.org/linux/man-pages/man2/select.2.html), [`sigwaitinfo(2)`](https://man7.org/linux/man-pages/man2/sigwaitinfo.2.html), [`signalfd(2)`](https://man7.org/linux/man-pages/man2/signalfd.2.html), [`timer_create(2)`](https://man7.org/linux/man-pages/man2/timer_create.2.html), [`timerfd_create(2)`](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [`clock_gettime(2)`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html), [`clock_nanosleep(2)`](https://man7.org/linux/man-pages/man2/clock_nanosleep.2.html), and [`nanosleep(2)`](https://man7.org/linux/man-pages/man2/nanosleep.2.html).

Interview-priority calibration:

- Amazon's SDE preparation page lists operating systems as an interview topic and emphasizes applied problem solving over memorization: <https://www.amazon.jobs/content/en/how-we-hire/interview-prep/software-development-topics>
- Microsoft's technical interviewing guide emphasizes principles, design, testing, trade-offs, and edge cases: <https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html/>
- Google interview resources were used as general calibration for clear communication and practice-oriented preparation: <https://www.google.com/about/careers/applications/interview-tips>
- Meta SWE interview guidance was used only to calibrate expectations around correctness, design trade-offs, operational debugging, security, and failure points: <https://d3no4ktch0fdq4.cloudfront.net/public/course/files/Meta_SWE_tech_screen_guide.pdf>
- Production/debug calibration: Kubernetes documents graceful Pod termination using TERM followed by KILL after a grace period: <https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#pod-termination>
- Docker documents `STOPSIGNAL`, the default use of `SIGTERM` for container stop behavior, and the signal-delivery risk of shell-form `ENTRYPOINT`: <https://docs.docker.com/reference/dockerfile/#stopsignal>
- NGINX documents signal-based runtime control for graceful shutdown, reload, log reopen, and fast shutdown: <https://docs.nginx.com/nginx/admin-guide/basic-functionality/runtime-control/>

Technical authority remains: repo knowledge, TLPI-derived docs, Linux man-pages, and official project documentation. Interview/company sources are used only for priority calibration, not as API authority.

---

## Coverage Trace

| Coverage item | Interview coverage |
|---|---|
| 4.1 Signals Fundamentals: lifecycle, disposition, `kill()`, mask | A1, A6, A7, B15-B17, C |
| 4.2 Signal Handlers: async-safety, `SA_SIGINFO`, `sigaltstack()` | A2, A10, B18-B20, B24, C |
| 4.3 Signals Advanced: realtime, `sigqueue()`, `signalfd()` | A7-A9, B21-B23, C |
| 4.4 Timers & Sleeping: `setitimer()`, POSIX timers, `timerfd` | A3, A12-A14, B25-B26, C |
| 4.5 Time API: `clock_gettime()`, `gettimeofday()`, process time | A11-A12, B27-B30, C |
| Must: signal lifecycle and default/ignore/catch | A1, B15-B17, High-Value Comparisons |
| Must: masks, per-process vs per-thread behavior | A7-A8, B16, B23 |
| Must: async-signal-safety, reentrancy, self-pipe/signalfd | A2, A7, B18, B23 |
| Must: standard vs realtime, payloads, ordering, `sigqueue()` | A9, B21-B22 |
| Must: timeout design, timer delivery, wall/process/elapsed time | A11-A14, B25-B30 |
| Must: production debugging and core-dump signal evidence | A1-A14, Common Project Failure Patterns |
| Useful existing concepts not to lose | Docker/Kubernetes shutdown A1; `SIGCHLD` A4; `SIGPIPE` A5; crash signals A10; embedded timing A12 |

---

## Priority Map

### A - Project and production scenarios

| Theme | Why it is Priority A |
|---|---|
| Graceful service shutdown under systemd, Docker, or Kubernetes | Real services are expected to handle `SIGTERM` and exit before escalation to `SIGKILL`. |
| Container entrypoint and signal forwarding | Shell wrappers or shell-form `ENTRYPOINT` can prevent the real application from receiving termination signals. |
| Safe signal handler design | The most common production mistake is doing too much inside an asynchronous handler. |
| `EINTR` and `SA_RESTART` policy | Signals change blocking syscall behavior; wrong policy causes hangs or false failures. |
| `SIGCHLD` and child reaping | Supervisors and process-based services must avoid zombies and lost child-status handling. |
| `SIGPIPE` in pipe/socket servers | Broken peers should become normal I/O errors, not unexpected process death. |
| Signal target and permission debugging | `kill()` targets processes or process groups; misunderstandings kill the wrong tasks or miss intended workers. |
| Lost wakeup around signal waits | `pause()` and manual unblock-then-wait patterns are classic race bugs. |
| Signals in multithreaded services | Dispositions are process-wide, masks are per-thread, and delivery can land in the wrong worker. |
| Standard signals as counters | Standard signals coalesce; using them as a queue loses events. |
| Fault signals and crash handling | `SIGSEGV`, `SIGBUS`, `SIGFPE`, and `SIGABRT` should be treated as crash diagnostics, not normal recovery paths. |
| Clock choice for timeouts and latency | `CLOCK_REALTIME` can jump; `CLOCK_MONOTONIC` is the default for durations. |
| Periodic loops and timer drift | Relative sleep and signal interruptions create timing bugs in embedded polling loops and agents. |
| `alarm()` versus `timerfd` | `alarm()` is process-global and signal-based; `timerfd` fits modern Linux event loops. |
| POSIX timer lifecycle and overruns | Timer-heavy code must understand arming, periodic intervals, absolute deadlines, and missed expirations. |

### B - Design comparisons and senior follow-ups

| Theme | Expected depth |
|---|---|
| Generated vs pending vs delivered | Explain signal lifecycle without confusing "sent" with "handled." |
| Disposition vs mask | Explain action policy versus temporary blocking. |
| Blocking vs ignoring | Explain delayed delivery versus discard. |
| `signal()` vs `sigaction()` | Choose `sigaction()` for explicit, portable production behavior. |
| Async handler vs `sigwaitinfo()` / `sigwait()` / `signalfd()` | Explain when to avoid asynchronous handlers. |
| Standard vs realtime signals | Know queueing, ordering, payload, and limits. |
| `sigqueue()` versus `kill()` | Know payload, queue limit, permission, and target semantics. |
| `SA_SIGINFO` and `siginfo_t` | Know what extra information is useful in debugging. |
| `sigaltstack()` | Recognize stack-overflow and crash-handler use cases. |
| POSIX timers vs `setitimer()` vs `timerfd` | Explain why different timer models exist. |
| Calendar time vs elapsed time vs CPU time | Choose APIs by meaning, not by habit. |
| `localtime()` vs `localtime_r()` | Know reentrancy/thread-safety issue. |
| Year 2038 | Recognize embedded and ABI/storage impact. |

### C - Lower-priority / know enough to recognize

| Topic | Know this much |
|---|---|
| Every signal number | Do not memorize numbers; use names such as `SIGTERM`. Numbers vary across architectures. |
| Every `sigaction` flag | Learn `SA_RESTART`, `SA_SIGINFO`, `SA_NOCLDSTOP`, `SA_NOCLDWAIT`, and `SA_ONSTACK` first. |
| Legacy signal APIs | `signal()`, `siginterrupt()`, and old System V/BSD semantics are recognition topics. |
| `setitimer()` details | Recognize `ITIMER_REAL`, `ITIMER_VIRTUAL`, and `ITIMER_PROF`; new code often chooses POSIX timers or `timerfd`. |
| `SIGEV_THREAD` | Useful but not the default answer for timer-heavy services. |
| Specialized clocks | `CLOCK_BOOTTIME`, alarm clocks, coarse clocks, and raw clocks are role-specific. |
| Timezone internals | Know `TZ`, `/etc/localtime`, `tzset()`, and `strftime()` practically; avoid trivia. |
| Core dump naming internals | Useful in crash debugging, but Chapter 3 owns the deeper core-dump postmortem workflow. |

---

## Final Interview List

### A - High-Probability Scenario Questions

1. A service in Docker or Kubernetes receives `SIGTERM` during deploy but often gets killed by `SIGKILL`. How would you design graceful shutdown?
2. A signal handler prints logs, allocates memory, and sometimes deadlocks during shutdown. What is wrong, and how would you fix it?
3. A blocking `read()`, `accept()`, `select()`, or `epoll_wait()` behaves differently after you add signal handling. How do `EINTR` and `SA_RESTART` affect the design?
4. A process supervisor still accumulates zombies even though it installed a `SIGCHLD` handler. How would you debug and redesign the reaping path?
5. A network server exits when a client disconnects during `write()` or `send()`. How does `SIGPIPE` explain this, and what is the production fix?
6. Your tool must send `SIGTERM` to one worker, a whole process group, or only test whether a PID is signalable. How do you use `kill()` safely?
7. An event loop occasionally sleeps forever after a signal arrives. What lost-wakeup race is likely, and how do `pselect()`, `ppoll()`, self-pipe, or `signalfd()` help?
8. A multithreaded service handles `SIGTERM` in a random worker thread and sometimes deadlocks. What signal-handling pattern would you use?
9. A project uses repeated `SIGUSR1` as a job counter and loses events under load. Why does this happen, and what should replace it?
10. A process crashes with `SIGSEGV`, `SIGBUS`, `SIGFPE`, or `SIGABRT`. What should the signal-level debugging answer look like?
11. A timeout or latency metric breaks when system time is adjusted. Which clock should be used and why?
12. An embedded polling loop drifts over hours or behaves badly after signal interruptions. How would you design the loop?
13. An `epoll` service uses `alarm()` for timeouts and becomes fragile. When would you use `timerfd` instead?
14. A POSIX timer fires repeatedly under load, but your service handles fewer timer events than expected. How would you reason about lifecycle, overruns, and notification choice?

### B - Design Comparisons and Senior Follow-Ups

15. Generated vs pending vs delivered: what is the lifecycle?
16. Disposition vs signal mask vs pending set: what does each track?
17. Blocking vs ignoring a signal: what is the production difference?
18. Why prefer `sigaction()` over `signal()`?
19. What does `volatile sig_atomic_t` solve, and what does it not solve?
20. What does `SA_SIGINFO` add through `siginfo_t`?
21. Standard signals vs realtime signals: what changes?
22. When would you use `sigqueue()`?
23. When are `sigwaitinfo()`, `sigwait()`, or `signalfd()` better than a handler?
24. What is `sigaltstack()`, and when is it useful?
25. What do POSIX timers solve compared with `alarm()` and `setitimer()`?
26. What is a timer overrun?
27. What does `clock_getres()` tell you, and what does it not guarantee?
28. Calendar time vs elapsed time vs process CPU time: which API fits each?
29. Why should reusable or threaded code prefer `localtime_r()` and `gmtime_r()`?
30. What is the Year 2038 problem?

### C - Lower-Priority / Know Enough to Recognize

- Use symbolic signal names, not raw signal numbers.
- Know common defaults: terminate, terminate with core, stop, continue, ignore.
- Recognize `SA_NODEFER`, `SA_RESETHAND`, `siginterrupt()`, and legacy semantics without leading with them.
- Recognize `SIGEV_THREAD`, `CLOCK_BOOTTIME`, `CLOCK_MONOTONIC_RAW`, and coarse clocks.
- Know that signal-driven I/O with `SIGIO` exists, but modern Linux services usually prefer readiness APIs such as `epoll`.
- Know core-dump controls such as `ulimit -c` and `/proc/sys/kernel/core_pattern`, but keep the main Chapter 4 answer focused on signal behavior.

---

## High-Value Comparisons

| Comparison | Strong interview answer |
|---|---|
| Signal vs normal function call | A normal call is chosen by program flow. A signal action is triggered by the kernel when a signal is delivered. |
| Generated vs pending vs delivered | Generated means the event happened. Pending means delivery is delayed. Delivered means the disposition is applied. |
| Disposition vs mask | Disposition says what action to take. The mask says which signals are temporarily blocked. |
| Blocking vs ignoring | Blocking delays delivery and may leave a pending signal. Ignoring discards the delivered signal. |
| `SIGTERM` vs `SIGKILL` | `SIGTERM` supports graceful shutdown. `SIGKILL` cannot be handled and skips cleanup. |
| `SIGINT` vs `SIGTERM` | `SIGINT` is usually interactive `Ctrl-C`; `SIGTERM` is normal service/container termination. |
| `SIGQUIT` vs `SIGINT` | `SIGQUIT` commonly terminates with a core dump; `SIGINT` is ordinary interrupt. |
| `signal()` vs `sigaction()` | `sigaction()` gives explicit handler, mask, flags, restart behavior, and `siginfo_t`. |
| Async handler vs `sigwait()` thread | Async handlers interrupt arbitrary code. `sigwait()` receives blocked signals synchronously in normal code. |
| `signalfd()` vs handler | `signalfd()` makes signals readable fd events for Linux event loops; handlers run asynchronously. |
| Standard vs realtime signals | Standard signals coalesce while pending. Realtime signals queue and can carry a small value. |
| `SIGCHLD` vs `waitpid()` | `SIGCHLD` is notification. `waitpid()` actually collects status and removes zombies. |
| `SIGPIPE` default vs ignored | Default may terminate the process. Ignoring or suppressing it lets writes fail with `EPIPE`. |
| `CLOCK_REALTIME` vs `CLOCK_MONOTONIC` | Realtime is wall-clock and can jump. Monotonic is for elapsed time and deadlines. |
| Relative sleep vs absolute sleep | Relative sleep can accumulate drift. Absolute sleep targets a fixed clock point. |
| `alarm()` vs `timerfd` | `alarm()` is one signal-based process timer. `timerfd` is fd-based and composes with `epoll`. |
| Resolution vs accuracy | Resolution is clock granularity; wakeup accuracy also depends on scheduler and hardware behavior. |

---

## Common Project Failure Patterns

| Failure pattern | Typical symptom | Root cause | First debug tools |
|---|---|---|---|
| Service ignores `SIGTERM` | Deploy/restart ends with `SIGKILL`, lost cleanup | Signal ignored, blocked, or handler does not notify main loop | `docker inspect`, `kubectl describe pod`, `/proc/<PID>/status`, `strace -e signal=all` |
| Container wrapper swallows shutdown | App never sees `SIGTERM`, rollout waits for forced kill | Shell-form `ENTRYPOINT`, wrapper script missing `exec`, or no signal forwarding | `docker inspect`, `ps -ef` in container, `strace -p 1 -e signal=all` |
| Handler deadlock | Service freezes during shutdown | Handler calls locks, malloc, stdio, syslog, or complex code | Code audit, `gdb -p`, `pstack`, `strace -f` |
| Lost shutdown wakeup | Process sleeps forever after signal | Unblock-then-`pause()` race or event loop not signal-aware | `strace -e signal=all`, inspect mask, use `pselect()`/`ppoll()` |
| Spurious syscall failure | `read()`/`accept()`/`select()` returns error under signals | Missing `EINTR` policy | `strace -e trace=read,accept,select,poll,epoll_wait -e signal=all` |
| Shutdown signal hidden | Main loop never regains control | `SA_RESTART` restarts blocking calls when stop should break wait | `sigaction()` audit, `strace`, signal mask check |
| Zombie leak | `<defunct>` children under load | `SIGCHLD` handler reaps only one child or parent never waits | `ps`, `pstree -ap`, `strace -e wait4` |
| Server dies on disconnect | Process exits during pipe/socket write | Default `SIGPIPE` action | Exit status/logs, `strace -e signal=SIGPIPE,write,sendto` |
| Event counter loses values | Fewer handler invocations than sends | Standard signals coalesce while pending | `strace -e kill,rt_sigqueueinfo`, replace with IPC |
| Signal in wrong thread | Random worker handles shutdown or crashes | Process-directed signal delivered to any eligible unblocked thread | `/proc/<PID>/task/*/status`, `pthread_sigmask()` audit |
| `alarm()` conflict | Library timeout breaks application timer | `alarm()` is process-global and shares state with other timer APIs | Code search, `strace -e alarm,setitimer` |
| Timer drift | Periodic task slowly shifts | Relative sleeps include work time and interruption delay | Trace timestamps, use `clock_nanosleep(..., TIMER_ABSTIME, ...)` |
| Bad timeout clock | Negative or huge latency after NTP/time change | `CLOCK_REALTIME` or `gettimeofday()` used for duration/deadline | Code search, compare `CLOCK_MONOTONIC` |
| Timerfd lag hidden | Event loop misses periodic work | Code ignores `uint64_t` expiration count | `strace -e timerfd_create,timerfd_settime,read`, log expiration count |
| Fault handler returns | Repeated crash or corrupted state | Handler treats `SIGSEGV`/`SIGBUS` as recoverable control flow | `dmesg`, core dump, `gdb`, handler audit |
| Hard-coded signal number | Works on one target, fails on another | Signal numbers differ by architecture | Use signal names, inspect `kill -l` |

---

## Detailed Answers - Priority A

### 1. A service in Docker or Kubernetes receives `SIGTERM` during deploy but often gets killed by `SIGKILL`. How would you design graceful shutdown?

**What the interviewer is testing**

They want to know whether you understand signals as production control flow, not just textbook events. They are also testing handler safety, service lifecycle, and container/service-manager behavior.

**Strong answer**

I would handle `SIGTERM` as a graceful shutdown request. The signal handler should do minimal work: set a `volatile sig_atomic_t` flag or notify the main loop through a safe fd path. Normal code should stop accepting new work, mark readiness false if the platform supports it, drain or cancel in-flight work, close sockets and device files deliberately, flush durable state if needed, and exit within the configured grace period.

I would not rely on `SIGKILL` for normal shutdown. `SIGKILL` is a last resort and gives the process no cleanup opportunity.

For containers, I would also verify that the real application process receives the stop signal. Prefer exec-form `ENTRYPOINT` or make wrapper scripts end with `exec "$@"`. If a wrapper must coordinate multiple processes, it must catch the termination signal, forward it to children or a process group, wait/reap, and then exit with a meaningful status.

**Mechanism**

Kubernetes and Docker-style shutdown generally starts with a termination signal, usually `SIGTERM` unless configured otherwise, followed by `SIGKILL` if the grace period expires. Docker `STOPSIGNAL` changes the signal sent by container stop operations. The handler runs asynchronously in the process context. It should only notify normal code because complex cleanup inside the handler is unsafe.

In a container, signal delivery also depends on the process tree. A shell-form `ENTRYPOINT` runs through `/bin/sh -c`; if the shell stays as PID 1 and does not `exec` or forward signals, the application may never see `SIGTERM`.

**Pitfalls**

- Ignoring or blocking `SIGTERM` indefinitely.
- Starting the app through a shell wrapper that does not `exec` or forward signals.
- Doing cleanup directly in the handler.
- Blocking forever in `accept()`, `read()`, `epoll_wait()`, or a driver call without a wakeup path.
- Exiting before child workers are stopped or reaped.
- Relying on `SIGKILL`, which skips user-space cleanup.
- On embedded targets, failing to flush state before power loss or watchdog reset.

**Debug angle**

Use:

```bash
cat /proc/<PID>/status | grep -E 'SigBlk|SigIgn|SigCgt|SigPnd|ShdPnd'
strace -f -e signal=all -e trace=rt_sigaction,rt_sigprocmask,ppoll,pselect6,epoll_wait,read,write -p <PID>
ps -o pid,ppid,pgid,sid,stat,cmd -p <PID>
ps -ef
kubectl describe pod <pod>
docker inspect <container>
strace -p 1 -e signal=all
```

Confirm whether `SIGTERM` is caught, blocked, ignored, or delivered, whether PID 1 is the real application or a wrapper, and whether the main loop wakes up after the signal.

**Follow-up keywords**

`SIGTERM`, `SIGKILL`, graceful shutdown, `STOPSIGNAL`, `ENTRYPOINT`, PID 1, signal forwarding, `sigaction()`, `sig_atomic_t`, self-pipe, `signalfd`, `EINTR`, `SA_RESTART`, readiness, grace period

### 2. A signal handler prints logs, allocates memory, and sometimes deadlocks during shutdown. What is wrong, and how would you fix it?

**What the interviewer is testing**

They are testing async-signal-safety and whether you know why handler bugs are timing-dependent and painful in production.

**Strong answer**

The handler is doing too much. An asynchronous signal can arrive while the program is inside libc, the allocator, stdio, or a locked region. Calling `printf()`, `malloc()`, `free()`, `exit()`, `syslog()`, or lock-taking APIs from the handler can deadlock or corrupt state.

I would change the handler to set a `volatile sig_atomic_t` flag or write a byte to a nonblocking self-pipe. The main loop or a dedicated signal thread would do logging, cleanup, memory management, lock operations, and shutdown sequencing in normal control flow.

**Mechanism**

Only async-signal-safe functions are safe inside a handler. Many common library functions use internal locks or global state. If the signal interrupts code while that state is already being modified, reentering the same library from the handler is unsafe.

**Pitfalls**

- Demo code often prints from handlers; production code should not copy that pattern.
- `exit()` runs atexit handlers and flushes stdio; it is not handler-safe.
- `volatile sig_atomic_t` is fine for simple flags, not counters, queues, or structures.
- If a handler changes `errno`, normal code may observe the wrong error unless the handler preserves it.

**Debug angle**

Use:

```bash
gdb -p <PID>
thread apply all bt
strace -f -e signal=all -p <PID>
cat /proc/<PID>/status | grep -E 'SigBlk|SigIgn|SigCgt'
```

Then audit the handler call graph against `signal-safety(7)`.

**Follow-up keywords**

async-signal-safe, reentrancy, `signal-safety(7)`, `sig_atomic_t`, self-pipe, `write()`, `_exit()`, `errno`, deadlock

### 3. A blocking `read()`, `accept()`, `select()`, or `epoll_wait()` behaves differently after you add signal handling. How do `EINTR` and `SA_RESTART` affect the design?

**What the interviewer is testing**

They want a practical syscall boundary answer: how signal delivery interacts with blocking operations and how to choose restart behavior intentionally.

**Strong answer**

When a caught signal is delivered while a thread is blocked in a syscall, the syscall may return early with `-1` and `errno == EINTR`. The code must decide what `EINTR` means: retry, stop, recompute timeout, return to the event loop, or propagate cancellation.

`SA_RESTART` asks the kernel to restart some interrupted syscalls automatically. It is useful when the signal should be transparent to the blocking operation. It is harmful when the signal is meant to wake the program for shutdown or timeout handling.

**Mechanism**

`sigaction()` controls handler flags. `SA_RESTART` affects many but not all blocking calls. Some wait APIs and sleep APIs are intentionally not restarted in the same way. Timeout-bearing calls often need careful recomputation after interruption.

**Pitfalls**

- Blindly retrying every `EINTR` and making shutdown unresponsive.
- Treating every `EINTR` as fatal and causing false production errors.
- Assuming `SA_RESTART` applies to all syscalls.
- Forgetting that `nanosleep()` and timeout waits need deadline-aware retry logic.

**Debug angle**

Use:

```bash
strace -tt -f -e signal=all -e trace=read,accept,select,pselect6,poll,ppoll,epoll_wait,nanosleep,clock_nanosleep ./service
```

Look for `ERESTART*` in traces, `EINTR` returns, and whether the application retries or exits.

**Follow-up keywords**

`EINTR`, `SA_RESTART`, `sigaction()`, `read()`, `accept()`, `select()`, `pselect()`, `ppoll()`, `epoll_wait()`, `nanosleep()`, timeout recomputation

### 4. A process supervisor still accumulates zombies even though it installed a `SIGCHLD` handler. How would you debug and redesign the reaping path?

**What the interviewer is testing**

They are testing whether you understand that `SIGCHLD` is notification, not cleanup, and that standard signals are not one event per child.

**Strong answer**

The handler alone does not reap children. The parent must call `waitpid()` or a related wait API. In a robust supervisor, `SIGCHLD` should trigger a loop that calls `waitpid(-1, &status, WNOHANG)` until no waitable children remain. The handler itself should be minimal: set a flag or wake the main loop.

**Mechanism**

When a child exits, the kernel keeps a zombie record containing PID, status, and accounting data. `SIGCHLD` tells the parent that child state changed. `waitpid()` collects the status and releases the zombie. Standard `SIGCHLD` notifications can coalesce, so one signal may represent multiple child exits.

**Pitfalls**

- Reaping only one child per signal.
- Doing complex logging or allocation in the handler.
- Ignoring `waitpid()` errors such as `EINTR` and `ECHILD`.
- Losing crash information by not decoding `WIFSIGNALED()`, `WTERMSIG()`, and `WEXITSTATUS()`.
- Using `SA_NOCLDWAIT` in a supervisor that actually needs child status.

**Debug angle**

Use:

```bash
ps -eo pid,ppid,stat,cmd | awk '$3 ~ /Z/ {print}'
pstree -ap <PARENT_PID>
strace -f -e signal=SIGCHLD -e trace=wait4,waitid -p <PARENT_PID>
cat /proc/<PARENT_PID>/status | grep -E 'SigBlk|SigIgn|SigCgt'
```

Confirm whether the parent receives `SIGCHLD` and whether it loops over all waitable children.

**Follow-up keywords**

`SIGCHLD`, zombie, `waitpid(-1, ..., WNOHANG)`, `ECHILD`, `SA_NOCLDSTOP`, `SA_NOCLDWAIT`, `WIFEXITED`, `WIFSIGNALED`, self-pipe

### 5. A network server exits when a client disconnects during `write()` or `send()`. How does `SIGPIPE` explain this, and what is the production fix?

**What the interviewer is testing**

They want to know whether you can connect a common server crash to default signal behavior and proper I/O error handling.

**Strong answer**

Writing to a pipe or socket whose peer is gone can generate `SIGPIPE`. The default action is process termination. A server normally should not die because one client disconnected. I would either ignore `SIGPIPE` process-wide or suppress it per send where supported, then handle `EPIPE` as a normal broken-connection error.

**Mechanism**

The kernel reports that the write side has no reader. If `SIGPIPE` is not ignored or suppressed, the signal may terminate the process before the write path returns an ordinary error. If suppressed, the call fails with `EPIPE`.

**Pitfalls**

- Ignoring `SIGPIPE` but not checking for `EPIPE`.
- Applying process-wide `SIGPIPE` policy without understanding libraries in the same process.
- Assuming TCP disconnects always show up as `read() == 0`; write paths can discover them too.
- On embedded systems, one broken serial/network peer should not kill the whole gateway service.

**Debug angle**

Use:

```bash
strace -f -e signal=SIGPIPE -e trace=write,sendto,sendmsg ./server
cat /proc/<PID>/status | grep -E 'SigIgn|SigCgt'
```

Check whether the process terminated by signal 13 or whether writes return `EPIPE`.

**Follow-up keywords**

`SIGPIPE`, `EPIPE`, `write()`, `send()`, `MSG_NOSIGNAL`, broken pipe, socket peer close, signal disposition

### 6. Your tool must send `SIGTERM` to one worker, a whole process group, or only test whether a PID is signalable. How do you use `kill()` safely?

**What the interviewer is testing**

They are testing target semantics, permission checks, process groups, and whether you know `kill()` does not necessarily kill.

**Strong answer**

`kill(pid, sig)` sends a signal. For `pid > 0`, it targets one process. For `pid == 0`, it targets the caller's process group. For `pid < -1`, it targets process group `abs(pid)`. `kill(pid, 0)` sends no signal; it checks existence and permission.

For a service or shell-like tool, I would be explicit about whether I want one PID or a whole process group. I would avoid raw PID assumptions where PID reuse matters.

**Mechanism**

The kernel checks signal permissions based on credentials and capabilities. `kill(pid, 0)` can return success for a signalable existing process, `EPERM` for an existing process without permission, or `ESRCH` if no such process exists.

**Pitfalls**

- Sending to one PID when the job has multiple children.
- Sending to a process group accidentally because `pid` is negative.
- Treating `kill(pid, 0)` as proof that the same program is still alive.
- Hard-coding signal numbers instead of using signal names.

**Debug angle**

Use:

```bash
ps -o pid,ppid,pgid,sid,user,stat,cmd -p <PID>
kill -0 <PID>; echo $?
strace -e trace=kill,rt_sigqueueinfo ./tool
cat /proc/<PID>/status | grep -E 'Uid|Gid|Groups|Cap'
```

Verify PID, PGID, SID, credentials, and errno.

**Follow-up keywords**

`kill()`, `killpg()`, `raise()`, `pid == 0`, negative PID, process group, `EPERM`, `ESRCH`, `kill -0`, PID reuse

### 7. An event loop occasionally sleeps forever after a signal arrives. What lost-wakeup race is likely, and how do `pselect()`, `ppoll()`, self-pipe, or `signalfd()` help?

**What the interviewer is testing**

They want to know whether you can reason about signal masks and waits atomically, a classic systems interview topic.

**Strong answer**

The likely bug is checking a flag, unblocking the signal, then calling `pause()` or a blocking wait. If the signal arrives between the check and the sleep, the program may miss the wakeup and sleep forever.

Use an atomic signal-aware wait: `sigsuspend()` for pure signal waits, `pselect()` or `ppoll()` for fd waits with a temporary signal mask, a self-pipe to turn signals into fd readiness, or Linux `signalfd()` after blocking those signals.

**Mechanism**

`pselect()` and `ppoll()` take a signal-mask argument and can temporarily install that mask for the duration of the wait, atomically with entering the blocking operation. That closes the gap between "unblock signal" and "sleep." The self-pipe trick writes from a minimal handler to a nonblocking pipe monitored by the event loop. `signalfd()` lets a process read signal information from a file descriptor, but the target signals should be blocked so normal delivery does not race with fd consumption.

**Pitfalls**

- Using `pause()` after a flag check.
- Forgetting to block signals before using `signalfd()`.
- Writing too much to a self-pipe or using a blocking pipe in the handler.
- Treating `select()` plus manual mask changes as equivalent to `pselect()`.
- Forgetting that timeout arguments may be modified by some wait APIs on Linux; recompute deadlines instead of reusing stale relative timeout structures blindly.

**Debug angle**

Use:

```bash
strace -tt -e signal=all -e trace=rt_sigprocmask,sigsuspend,pselect6,ppoll,select,poll,epoll_wait,signalfd4,read ./service
cat /proc/<PID>/status | grep -E 'SigBlk|SigPnd|ShdPnd'
```

Look for mask changes and whether the program enters the wait after the signal was already delivered.

**Follow-up keywords**

lost wakeup, `sigsuspend()`, `pselect()`, `ppoll()`, self-pipe trick, `signalfd()`, signal mask, pending signal, event loop

### 8. A multithreaded service handles `SIGTERM` in a random worker thread and sometimes deadlocks. What signal-handling pattern would you use?

**What the interviewer is testing**

They are testing the distinction between process-wide dispositions and per-thread masks, plus a practical design for threaded services.

**Strong answer**

In a multithreaded process, signal dispositions are process-wide, but signal masks are per-thread. A process-directed signal can be delivered to any eligible unblocked thread. For shutdown signals, I would block them in the main thread before creating workers so the mask is inherited, then dedicate one signal thread to `sigwait()` or `sigwaitinfo()`. That thread receives the signal synchronously and can use normal locking to notify workers.

**Mechanism**

`pthread_sigmask()` controls a thread's mask. If all worker threads block `SIGTERM`, they will not receive it asynchronously. A dedicated signal thread waits for the blocked set synchronously, so it runs in normal control flow rather than inside an asynchronous handler.

**Pitfalls**

- Installing a handler and assuming it runs in the main thread.
- Calling pthread APIs from an asynchronous handler.
- Blocking signals after workers are already created.
- Having multiple threads wait for the same signal without a clear ownership policy.

**Debug angle**

Use:

```bash
for t in /proc/<PID>/task/*/status; do echo "$t"; grep -E 'Name|Pid|Tgid|SigBlk|SigIgn|SigCgt' "$t"; done
strace -f -e signal=all -e trace=rt_sigprocmask,rt_sigtimedwait ./service
```

Confirm thread masks and which thread receives the signal.

**Follow-up keywords**

`pthread_sigmask()`, process-directed signal, thread-directed signal, `sigwait()`, `sigwaitinfo()`, signal thread, dispositions, masks, NPTL

### 9. A project uses repeated `SIGUSR1` as a job counter and loses events under load. Why does this happen, and what should replace it?

**What the interviewer is testing**

They want to know whether you understand signal queueing semantics and when signals are the wrong IPC tool.

**Strong answer**

Standard signals are not reliable counters. If the same standard signal is pending and more instances arrive, they collapse into one pending signal. So repeated `SIGUSR1` can lose event count. Use a pipe, socket, eventfd, message queue, shared memory with synchronization, or another real IPC mechanism for event counts or work queues.

Realtime signals can queue and `sigqueue()` can carry a small value, but they are still low-bandwidth notifications with queue limits. They are not a replacement for a real job queue.

**Mechanism**

For standard signals, the pending state is like one bit per signal number. Realtime signals have queueing semantics and defined ordering, subject to system limits.

**Pitfalls**

- Counting handler invocations as if they equal sender events.
- Blocking a standard signal and expecting all instances to be preserved.
- Using `SIGUSR1` for high-rate telemetry, jobs, or data transfer.
- Forgetting realtime signal queue limits and failure handling.

**Debug angle**

Use:

```bash
strace -f -e trace=kill,rt_sigqueueinfo -e signal=SIGUSR1,SIGRTMIN ./program
cat /proc/<PID>/status | grep -E 'SigPnd|ShdPnd|SigQ'
```

If counts matter, instrument the real queue or IPC mechanism rather than counting standard-signal deliveries.

**Follow-up keywords**

standard signals, realtime signals, `sigqueue()`, `SIGUSR1`, `SIGRTMIN`, pending bit, `SigQ`, eventfd, pipe, message queue

### 10. A process crashes with `SIGSEGV`, `SIGBUS`, `SIGFPE`, or `SIGABRT`. What should the signal-level debugging answer look like?

**What the interviewer is testing**

They are checking whether you treat fault signals as crash evidence and know the limits of signal handlers in corrupted state.

**Strong answer**

I would treat these as crash or fault signals, not normal control flow. `SIGSEGV` usually means invalid memory access or protection violation. `SIGBUS` often means an address is in a mapping but the backing object is invalid, or an alignment/bus issue. `SIGFPE` is arithmetic exception. `SIGABRT` usually comes from `abort()` or failed assertions.

For debugging, collect wait status or logs, check whether a core dump exists, and debug with `gdb` using the exact binary and symbols. A handler may write minimal crash metadata or let the default action produce a core, but it should not try complex recovery.

**Mechanism**

Hardware-generated signals are synchronous with the faulting instruction. `SA_SIGINFO` can provide `siginfo_t`, including fault address for `SIGSEGV`/`SIGBUS` and instruction address for some hardware signals. Returning from a handler after a real hardware fault is usually not a valid recovery strategy unless the program has a very specialized design.

**Pitfalls**

- Calling unsafe code from a crash handler.
- Returning from a `SIGSEGV` handler and looping on the same fault.
- Treating `SIGBUS` like generic `SIGSEGV` and missing file-backed mapping issues.
- Debugging stripped target cores without matching symbols and libraries.
- Writing large core files to small flash storage without policy.

**Debug angle**

Use:

```bash
dmesg | tail
ulimit -c
cat /proc/sys/kernel/core_pattern
coredumpctl list
coredumpctl debug <PID>
gdb <binary> <core>
strace -f -e signal=SIGSEGV,SIGBUS,SIGFPE,SIGABRT ./program
```

For stack-overflow crash handlers, recognize `sigaltstack()` and `SA_ONSTACK`, but keep the handler minimal.

**Follow-up keywords**

`SIGSEGV`, `SIGBUS`, `SIGFPE`, `SIGABRT`, `SIGILL`, `SA_SIGINFO`, `si_addr`, `sigaltstack()`, `SA_ONSTACK`, core dump, `gdb`

### 11. A timeout or latency metric breaks when system time is adjusted. Which clock should be used and why?

**What the interviewer is testing**

They are testing whether you distinguish human calendar time from elapsed time.

**Strong answer**

Use `CLOCK_MONOTONIC` for durations, latency measurement, timeout deadlines, retry backoff, and periodic scheduling. Use `CLOCK_REALTIME` for human timestamps such as logs. `gettimeofday()` reads wall-clock time and is a poor default for new elapsed-time code because wall-clock time can jump.

**Mechanism**

`CLOCK_REALTIME` represents settable wall-clock calendar time. It can move forward or backward due to NTP, manual changes, or RTC correction. `CLOCK_MONOTONIC` measures elapsed time from an unspecified point and is designed for intervals.

**Pitfalls**

- Negative latency after wall-clock adjustment.
- Timeouts that expire immediately or wait too long after time sync.
- Mixing readings from different clocks.
- Using monotonic values as human timestamps.

**Debug angle**

Use:

```bash
rg 'gettimeofday|CLOCK_REALTIME|time\\(' .
strace -e trace=clock_gettime,gettimeofday,clock_nanosleep,nanosleep ./program
timedatectl status
```

In code reviews, require the caller to state whether the value is a timestamp, duration, deadline, or CPU-time measurement.

**Follow-up keywords**

`CLOCK_REALTIME`, `CLOCK_MONOTONIC`, `gettimeofday()`, `clock_gettime()`, timestamp, duration, timeout, NTP, deadline

### 12. An embedded polling loop drifts over hours or behaves badly after signal interruptions. How would you design the loop?

**What the interviewer is testing**

They want to know whether you understand relative sleep drift, signal interruption, and embedded timing constraints.

**Strong answer**

For a periodic loop, I would use a monotonic clock and absolute deadlines. Compute the next target time once, do work, then sleep until that absolute time with `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`. On `EINTR`, retry the same absolute target unless shutdown was requested. Then increment the target by the period.

This avoids accumulating work time, signal interruption time, and scheduler delay into the next period.

If the embedded device can suspend and the deadline should include time spent suspended, I would consider `CLOCK_BOOTTIME` on Linux. If the requirement is "run every period while awake," `CLOCK_MONOTONIC` is usually the better default. I would avoid wall-clock time for periodic control because RTC/NTP correction can jump.

**Mechanism**

Relative sleep means "sleep N from now." If each iteration does work before sleeping, the real period becomes work time plus sleep time plus delay. Absolute sleep means "sleep until this clock time," so the loop keeps a stable schedule.

**Pitfalls**

- Recomputing relative sleep after every signal and drifting.
- Ignoring `EINTR`.
- Expecting Linux scheduling to be hard realtime without realtime configuration.
- Using `CLOCK_REALTIME` for periodic scheduling.
- Choosing `CLOCK_MONOTONIC` when the product requirement is suspend-aware elapsed time, or choosing `CLOCK_BOOTTIME` when only awake time should count.
- On embedded boards, ignoring CPU frequency scaling, thermal throttling, interrupt load, and power constraints.

**Debug angle**

Use:

```bash
strace -tt -e trace=clock_gettime,clock_nanosleep,nanosleep -e signal=all ./poller
pidstat -w -p <PID> 1
cat /proc/<PID>/sched
```

Log monotonic timestamps, loop duration, missed periods, and signal interruptions.

**Follow-up keywords**

`clock_nanosleep()`, `TIMER_ABSTIME`, `CLOCK_MONOTONIC`, `CLOCK_BOOTTIME`, drift, suspend, RTC, `EINTR`, periodic loop, scheduler delay, embedded timing

### 13. An `epoll` service uses `alarm()` for timeouts and becomes fragile. When would you use `timerfd` instead?

**What the interviewer is testing**

They are testing whether you can choose a timer model that fits an event-loop architecture.

**Strong answer**

In an fd-based Linux event loop, I would usually use `timerfd`. It turns timer expiration into a readable fd event, so the same `epoll` loop can handle sockets, pipes, eventfds, signalfds, and timers. `alarm()` is one process-wide signal timer and is easy to conflict with libraries or other modules.

**Mechanism**

`timerfd_create()` creates a timer object represented by a file descriptor. `timerfd_settime()` arms it. When the timer expires, the fd becomes readable, and reading a `uint64_t` returns the number of expirations since the last read.

**Pitfalls**

- Forgetting to read the timerfd, so it remains readable.
- Ignoring the expiration count and hiding event-loop lag.
- Leaking timerfd across `exec()` by not using `TFD_CLOEXEC`.
- Using realtime-clock timerfd deadlines without thinking about clock changes.
- Assuming `alarm()` can support independent timers for multiple modules.

**Debug angle**

Use:

```bash
strace -e trace=timerfd_create,timerfd_settime,read,epoll_wait ./service
ls -l /proc/<PID>/fd
```

Log the `uint64_t` expiration count to detect missed periods.

**Follow-up keywords**

`timerfd_create()`, `timerfd_settime()`, `TFD_CLOEXEC`, `TFD_NONBLOCK`, `TFD_TIMER_ABSTIME`, `epoll`, expiration count, `alarm()`, `setitimer()`

### 14. A POSIX timer fires repeatedly under load, but your service handles fewer timer events than expected. How would you reason about lifecycle, overruns, and notification choice?

**What the interviewer is testing**

They are testing whether you understand timer lifecycle instead of just naming `timer_create()`. They also want to see whether you can debug missed expirations under scheduler delay or overload.

**Strong answer**

I would first identify the timer model and notification path. A POSIX timer is created with `timer_create()`, armed or disarmed with `timer_settime()`, inspected with `timer_gettime()`, checked for missed expirations with `timer_getoverrun()`, and removed with `timer_delete()`.

`it_value` controls the first expiration. If it is zero, the timer is disarmed. `it_interval` controls whether the timer is periodic; zero interval means one-shot. With `TIMER_ABSTIME`, the first expiration is interpreted as an absolute clock time, and if that time has already passed, the timer can expire immediately. Under load, signal notification may not represent one delivered signal per expiration, so overrun accounting matters.

For an event-loop service, I would usually compare POSIX signal notification with `timerfd`. `timerfd` gives an fd and a `uint64_t` expiration count, which is easier to integrate with `epoll`.

**Mechanism**

The kernel owns the timer object after `timer_create()`. `timer_settime()` moves it between disarmed, one-shot, and periodic states. When it expires, the selected notification model runs: signal, thread callback, no notification, or an fd event if using `timerfd` instead. If the program is delayed, multiple expirations may collapse into one notification with an overrun count.

**Pitfalls**

- Treating a periodic timer signal as one signal per period.
- Forgetting to disarm or delete timers during shutdown.
- Confusing `it_value` with `it_interval`.
- Using absolute realtime deadlines without considering clock changes.
- Ignoring `timer_getoverrun()` or timerfd read counts and hiding overload.
- Choosing `SIGEV_THREAD` without understanding callback concurrency and resource use.

**Debug angle**

Use:

```bash
strace -tt -e trace=timer_create,timer_settime,timer_gettime,timer_getoverrun,timer_delete,rt_sigtimedwait,read ./service
cat /proc/<PID>/status | grep -E 'SigQ|SigPnd|ShdPnd|SigBlk'
```

Log timer configuration, target clock, `it_value`, `it_interval`, overrun counts, and actual handling timestamps. For `timerfd`, log the `uint64_t` read value and compare it with expected periods.

**Follow-up keywords**

`timer_create()`, `timer_settime()`, `timer_delete()`, `it_value`, `it_interval`, `TIMER_ABSTIME`, `timer_getoverrun()`, `SIGEV_SIGNAL`, `SIGEV_THREAD`, `timerfd`, overrun, scheduler latency

---

## Short Answers - Priority B

### 15. Generated vs pending vs delivered

Generated means the event happened and the kernel created a signal for a target. Pending means delivery has not happened yet, often because the signal is blocked. Delivered means the kernel applies the signal disposition: default action, ignore, or handler.

### 16. Disposition vs signal mask vs pending set

Disposition answers "what action happens when delivered?" The signal mask answers "which signals are temporarily blocked?" The pending set answers "which blocked signals have arrived but have not been delivered yet?"

### 17. Blocking vs ignoring

Blocking delays signal delivery and can leave the signal pending. Ignoring discards the signal when delivered. Blocking `SIGTERM` briefly during a critical section may be reasonable; ignoring it in a service is usually a production problem.

### 18. `sigaction()` vs `signal()`

`signal()` is simple but historically inconsistent and too limited for real systems code. `sigaction()` gives explicit handler setup, handler-time mask, flags such as `SA_RESTART` and `SA_SIGINFO`, and old-action retrieval.

### 19. `volatile sig_atomic_t`

Use it for simple handler-to-main flags such as `stop_requested = 1`. It does not make `counter++`, queues, structs, heap state, or general shared data safe.

### 20. `SA_SIGINFO` and `siginfo_t`

`SA_SIGINFO` changes the handler shape so it receives `siginfo_t`. Useful fields may include sender PID/UID, `sigqueue()` payload, fault address for hardware faults, child status for `SIGCHLD`, and timer overrun information.

### 21. Standard vs realtime signals

Standard signals coalesce while pending and should not be used as counters; their delivery order is not a portable design contract. Realtime signals queue, can carry a small value via `sigqueue()`, and have defined ordering: multiple pending instances of the same realtime signal are delivered FIFO, while different realtime signals are delivered in signal-number priority order on Linux/POSIX systems. All of this is subject to queue limits.

### 22. `sigqueue()`

Use `sigqueue(pid, sig, union sigval)` when you need a queued signal with a tiny integer or pointer-sized value delivered through `siginfo_t` with `SA_SIGINFO` or synchronous wait APIs. It uses signal permission checks like `kill()`, targets one process ID rather than negative-pid process groups, and can fail with `EAGAIN` when queued-signal limits are hit. Avoid it for bulk data, high-volume work queues, or protocols that need durable backpressure.

### 23. `sigwaitinfo()`, `sigwait()`, and `signalfd()`

Use synchronous signal acceptance when you can block the signals and handle them in normal code. `sigwait()`/`sigwaitinfo()` fit threaded shutdown designs. `signalfd()` fits Linux fd-based event loops.

### 24. `sigaltstack()`

`sigaltstack()` registers a per-thread alternate signal stack described by `stack_t`; a handler uses it when installed with `SA_ONSTACK`. It is mainly useful for stack-overflow or crash-handling scenarios where the normal stack may be unusable. Size it deliberately with `SIGSTKSZ`/`MINSIGSTKSZ` awareness, recognize `SS_ONSTACK` and `SS_DISABLE`, and remember that changing the alternate stack while running on it can fail with `EPERM`. It does not make complex handler logic safe.

### 25. POSIX timers

POSIX timers support multiple timers, `timespec` resolution, different clocks, different notification models, and overrun reporting. `timer_settime()` uses `it_value` for first expiration and `it_interval` for periodic behavior; zero `it_value` disarms the timer. They are more flexible than `alarm()` and `setitimer()`, but also more complex.

### 26. Timer overrun

A timer overrun means the timer expired additional times before the program consumed the notification. For `timerfd`, the `uint64_t` returned by `read()` is the expiration count.

### 27. `clock_getres()`

`clock_getres()` reports nominal clock resolution. It does not guarantee that your process wakes at that precision; scheduling delay, hardware, interrupt load, and system policy still matter.

### 28. Calendar vs elapsed vs CPU time

Calendar time is human wall-clock time: use `time()` or `CLOCK_REALTIME`. Elapsed time is duration: use `CLOCK_MONOTONIC`. CPU time is consumed CPU: use `CLOCK_PROCESS_CPUTIME_ID`, `CLOCK_THREAD_CPUTIME_ID`, `clock()`, or `getrusage()` depending on the need.

### 29. `localtime_r()` and `gmtime_r()`

The non-`_r` conversion functions may use static storage. Reusable or threaded code should prefer `localtime_r()` and `gmtime_r()` so the caller owns the output buffer. Time conversion has calendar traps: `tm_sec` may represent a leap second on systems that expose it, DST transitions can make local times ambiguous or nonexistent, and `mktime()` interprets the input as local time rather than UTC.

### 30. Year 2038

On systems with signed 32-bit `time_t`, calendar time overflows in January 2038. It matters for old 32-bit Linux, embedded products with long lifetimes, binary protocols, file formats, and persistent logs.

---

## Recognition Notes - Priority C

| Topic | Recognition note |
|---|---|
| Signal numbers | Use names. Numbers can vary across architectures. |
| Full default-action table | Know common actions and check `signal(7)` for the rest. |
| `SA_NODEFER` | Allows the same signal to interrupt its own handler; advanced and risky. |
| `SA_RESETHAND` | Resets disposition when the handler starts; useful in special cases. |
| `SA_NOCLDSTOP` | For `SIGCHLD`, avoid notification when children stop/continue. |
| `SA_NOCLDWAIT` | Avoid zombies but lose normal waitable status; not a default supervisor choice. |
| Legacy `signal()` semantics | Know why they are not the production default. |
| `setitimer()` | Legacy interval timers; recognize interaction with `alarm()` on Linux. |
| `SIGEV_THREAD` | Timer callback thread model; useful but not the simplest event-loop answer. |
| Signal-driven I/O | `SIGIO` exists but is usually less clean than readiness APIs. |
| `CLOCK_BOOTTIME` | Monotonic-like clock that includes suspend time; useful for suspend-aware devices. |
| `CLOCK_MONOTONIC_RAW` | Specialized raw hardware-based monotonic clock; not the default timeout clock. |
| Timezone internals | Know practical `TZ`, `/etc/localtime`, `tzset()`, `strftime()` behavior. |
| Core dump controls | Recognize `ulimit -c`, `core_pattern`, `coredumpctl`, and `gdb`, but do not turn Chapter 4 into a core-dump chapter. |

---

## Extra Questions Worth Adding

These are useful for mock interviews if the candidate already handles the Priority A list well:

1. How would you implement a timeout for one blocking operation without using process-global `alarm()`?
2. How would you handle `SIGHUP` for config reload and log reopen without doing the work in the handler?
3. How would you design signal handling for a parent process that owns many worker processes and threads?
4. How would you detect whether a signal is blocked, ignored, caught, or pending using `/proc`?
5. How would you test signal code deterministically when signal timing is nondeterministic?
6. What should a crash handler record, and what should it avoid doing?
7. Why can a containerized application fail to receive `SIGTERM` when `docker stop` or Kubernetes termination starts?
8. Why can a process still fail to terminate after `SIGKILL` appears to be sent?
9. How do `pselect()`, `ppoll()`, `epoll_pwait()`, and `signalfd()` compare for signal-aware event loops?
10. How would you choose between `CLOCK_MONOTONIC`, `CLOCK_BOOTTIME`, and `CLOCK_REALTIME` on an embedded device that can suspend?
11. How would you prove a timer loop is drifting, and how would you fix it?

---

## One-Minute Review

- A signal is a small kernel-delivered event notification, not a normal function call.
- Lifecycle: generated, possibly pending, delivered, then disposition action.
- Core state: disposition, signal mask, pending set.
- Blocking delays delivery; ignoring discards.
- `SIGTERM` is graceful termination; `SIGKILL` is last resort; `SIGKILL` and `SIGSTOP` cannot be caught, blocked, or ignored.
- Use `sigaction()` for real code.
- In containers, make sure the real service receives stop signals; shell-form `ENTRYPOINT` and wrapper scripts can hide `SIGTERM`.
- A handler should set a flag or notify through an async-signal-safe path; real work belongs in normal code.
- `printf()`, `malloc()`, `free()`, `exit()`, `syslog()`, and locks are handler traps.
- `volatile sig_atomic_t` is for simple flags, not complex shared state.
- Every blocking syscall in signal-aware code needs an `EINTR` policy.
- `SA_RESTART` is useful only when restarting is the desired behavior.
- Standard signals are not counters.
- `SIGCHLD` is notification; `waitpid()` reaps.
- `SIGPIPE` can kill servers; handle `EPIPE` deliberately.
- In threaded programs, block signals in workers and use a signal thread or `signalfd` design.
- Use `CLOCK_REALTIME` for human timestamps and `CLOCK_MONOTONIC` for durations and deadlines.
- Sleep can be interrupted; periodic relative sleep can drift.
- `alarm()` is simple but process-global and signal-based.
- `timerfd` is the clean Linux choice for fd-based event loops.
