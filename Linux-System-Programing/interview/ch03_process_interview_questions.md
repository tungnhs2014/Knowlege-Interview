# Chapter 3 Interview - Process

> Scope: process fundamentals, creation, termination, child monitoring, program execution, process creation details, process groups, sessions, job control, credentials, daemons, priorities, scheduling, resource usage, and limits.
> Primary knowledge files: [ch03_process_core.md](../knowledge/ch03_process_core.md), [ch03_process_execution.md](../knowledge/ch03_process_execution.md), [ch03_process_advanced.md](../knowledge/ch03_process_advanced.md), and credential cross-reference [ch01_users_and_groups.md](../knowledge/ch01_users_and_groups.md).

---

## Review Basis

This file is a scenario-first Chapter 03 interview set. It should train the mental model:

```text
program -> process -> fork child -> setup child -> exec new image
        -> exit or signal death -> parent wait/reap

advanced controls:
process group/session -> terminal and job behavior
credentials           -> permission boundary
daemon/service model  -> lifecycle and supervision
scheduler/limits      -> runtime behavior under pressure
```

Source grounding:

- Learning map: Chapter 03 topics 3.1 through 3.11, with Chapter 03 split into core, execution, and advanced knowledge files.
- Repo knowledge: process core, process execution, process advanced, and users/groups credential notes.
- TLPI-derived chapters: Ch06, Ch09, Ch24, Ch25, Ch26, Ch27, Ch28, Ch34, Ch35, Ch36, and Ch37.
- DevLinux Process Module 03: practical `fork()`, `exec()`, `wait()`, parent-child, zombie/orphan, and process observation exercises.
- Linux behavior focus: POSIX concepts first, Linux-specific/version-sensitive details marked where they matter.

Interview guidance:

- Strong answers start from the production symptom or design goal, then explain mechanism, failure modes, and debug workflow.
- API names are evidence for understanding, not the headline.
- Embedded Linux answers should mention RAM, PID/FD limits, device nodes, storage wear, stripped binaries, watchdog/init behavior, and target-vs-host differences when relevant.

---

## Coverage Trace

Every Chapter 03 learning-map row and Must Cover concept is traced to Priority A, B, or C coverage below.

Mapped topic rows:

| Row | Topic | Priority coverage |
|---|---|---|
| 3.1 | Process fundamentals: PID, memory layout, environment | A1, A5, B15 |
| 3.2 | Process creation: `fork()`, CoW, file sharing | A1, A2, A4, A5, B17, B18, B22 |
| 3.3 | Process termination: `exit()`, `_exit()`, `atexit()` | A2, A11, B21, B25 |
| 3.4 | Monitoring children: `wait*()`, zombies, orphans | A1, A3, A11, A13, B20, B21, B26 |
| 3.5 | Program execution: `exec*()`, `system()`, scripts | A1, A4, A12, B17, B24, C exec variants |
| 3.6 | Creation details: `clone()`, fork/exec attributes, accounting | A5, A13, A14, B18, B30, C clone/accounting/pidfd |
| 3.7 | Process groups, sessions, job control | A6, A7, A13, B27, C orphaned process groups |
| 3.8 | Process credentials | A8, A12, B28, C capabilities/filesystem IDs |
| 3.9 | Daemons | A7, A13, B32, C `daemon(3)` |
| 3.10 | Priorities and scheduling | A10, B29, B30, C scheduler details |
| 3.11 | Resources, limits, `getrusage()`, accounting | A9, A10, A11, A14, B30, B31, C `RLIMIT_*` catalog |

Must Cover concepts:

| Concept | Priority coverage |
|---|---|
| Program -> process -> fork -> child setup -> exec -> exit -> wait/reap lifecycle | A1, A2, A3, B17, B25 |
| Process ownership boundary: PID, VM, FDs, cwd/root, environment, credentials, signals, scheduling/accounting | A1, A4, A5, A8, A10, B15, B22, B30 |
| Multiprocess design: parent/child responsibilities, supervision, error reporting, FD hygiene, close-on-exec | A1, A3, A4, A7, A13 |
| `fork()`, CoW, scheduling nondeterminism, globals are not IPC, process vs thread tradeoffs | A2, A5, B16, B18, B19 |
| `execve()` and `exec*()` variants, environment/PATH risk, scripts, `system()` risks | A1, A8, A12, B24, C exec variants |
| `wait()`, `waitpid()`, `waitid()`, status macros, zombies, orphans, subreapers, PID namespaces | A3, A11, A13, B20, B21, B26, C namespaces |
| Process groups, sessions, job control, controlling terminals, foreground/background signal behavior | A6, A7, A13, B27, C orphaned process groups |
| Credentials, privilege drop, set-user-ID behavior, daemon/service-manager lifecycle | A7, A8, A12, B28, B32 |
| CPU scheduling, nice values, realtime policies, concurrency vs parallelism, context switches, CPU affinity | A10, B29, B30, C scheduler details |
| Resource limits, `getrusage()`, process accounting, core dumps, production debugging | A9, A10, A11, A14, B30, B31, C accounting/core controls |

---

## Priority Map

| Priority | Topics | Interview expectation |
|---|---|---|
| A | Safe helper launch, child error paths, zombies/reaping, FD inheritance, copy-on-write, job control, daemons, credentials, limits, scheduling/context switches, core dumps, safe replacement for `system()`, orphan/subreaper cleanup | Answer deeply with testing intent, strong answer, mechanism, pitfalls, debug angle, and follow-up keywords. |
| B | Program vs process, process vs thread, `fork()` variants, wait APIs including `waitid()`, status macros, FD table vs open file description, exec variants including `execvpe()`, `SIGCHLD` races, terminal background behavior, rlimit/cgroup/accounting comparisons | Compact trade-off answer with when to use each idea. |
| C | Rare APIs, Linux-specific extensions, deep scheduler internals, full `RLIMIT_*` catalog, namespaces, pidfds, accounting formats, advanced exec variants | Recognize the name, purpose, and when to read the manual. |

Priority A scenario coverage:

| Scenario theme | Chapter topics exercised |
|---|---|
| Safe helper launch | 3.1, 3.2, 3.3, 3.4, 3.5 |
| Child failure before `exec()` | 3.2, 3.3, 3.5 |
| Zombie reaping | 3.3, 3.4 |
| FD inheritance and close-on-exec | 3.1, 3.2, 3.5 |
| Copy-on-write and variables not IPC | 3.1, 3.2, 3.6 |
| Foreground pipeline and `Ctrl-C` | 3.4, 3.5, 3.7 |
| Daemon vs service manager | 3.7, 3.9 |
| Privilege dropping | 3.8 plus Chapter 01 credentials |
| RLIMIT/cgroup target failures | 3.10, 3.11 |
| Scheduling/context-switch slowdown | 3.10 |
| Embedded core dumps | 3.3, 3.4, 3.11 |
| Replacing `system()` safely and decoding its result | 3.5, 3.8 |
| Orphans, PID 1, subreapers | 3.4, 3.7, 3.9 |
| Process accounting after a short-lived helper disappears | 3.6, 3.11 |

---

## Scenario Questions

### 1. Your service must launch a helper, capture its output, and continue running. How would you design the `fork() -> setup -> exec() -> waitpid()` lifecycle?

**What the interviewer is testing**

Whether you understand the complete process lifecycle as a production pattern: parent ownership, child setup, descriptor hygiene, `exec()` replacement, child status collection, and failure reporting.

**Strong answer**

Keep the parent as supervisor. The parent creates pipes with close-on-exec where possible, calls `fork()`, records the child PID, closes child-only pipe ends, drains output without deadlocking, and reaps the child with `waitpid()`. The child does only setup that must happen before `exec()`: close/dup descriptors, set process group or credentials if required, sanitize environment, then call `execve()` or a controlled `exec*()` wrapper. If `exec()` fails, the child reports `errno` through a simple pipe if needed and exits with `_exit()`.

Use explicit `argv[]` and `envp[]` for services and privileged code. Use `execvp()` only for shell-like, non-privileged tools where `PATH` lookup is intended.

**Mechanism**

`fork()` creates a child with a new PID and copied process state. `execve()` replaces the child process image and keeps the same PID. Open FDs survive `exec()` unless close-on-exec is set. `waitpid()` collects the child status and removes the zombie entry. Decode status with `WIFEXITED()`, `WEXITSTATUS()`, `WIFSIGNALED()`, and `WTERMSIG()`.

If the parent needs more structured child-state information, use `waitid(idtype, id, &siginfo, options)`. It can select by `P_PID`, `P_PGID`, or `P_ALL`, report `WEXITED`, `WSTOPPED`, and `WCONTINUED` events, and use `WNOWAIT` to inspect state without reaping immediately.

**Pitfalls**

- Waiting before draining a full pipe, causing parent and child to deadlock.
- Letting the child continue parent code when `exec()` fails.
- Calling `exit()` in a post-`fork()` child failure path and flushing duplicated stdio buffers.
- Leaking sockets, secrets, pipe ends, or device FDs into the helper.
- Treating raw wait status as the exit code.
- Ignoring `fork()` failure from `EAGAIN`, `ENOMEM`, `RLIMIT_NPROC`, or cgroup PID limits.

**Debug angle**

Use:

```bash
strace -f -e trace=fork,vfork,clone,execve,wait4,pipe,dup2,close ./service
ps -o pid,ppid,pgid,sid,stat,cmd -p <PID>
pstree -ap <PID>
ls -l /proc/<PID>/fd
cat /proc/<PID>/limits
```

Confirm which process owns each pipe end, whether `execve()` succeeds, and whether the parent waits for the correct PID.

**Follow-up keywords**

`fork()`, `execve()`, `execvp()`, `waitpid()`, `waitid()`, `siginfo_t`, `WIFEXITED`, `WIFSIGNALED`, `pipe2(O_CLOEXEC)`, `dup2()`, `FD_CLOEXEC`, `_exit()`, `posix_spawn()`, `strace -f`

### 2. A child fails before `exec()` and production logs are duplicated. What happened, and why does `_exit()` matter?

**What the interviewer is testing**

Whether you distinguish C library termination from kernel process termination, especially in a child that inherited parent user-space state.

**Strong answer**

After `fork()`, the child inherits copies of the parent's stdio buffers and `atexit()` handlers. If the child calls `exit()` after an `exec()` failure, it can flush buffered log data that the parent will also flush, and it can run cleanup handlers meant for the parent. In a child failure path before successful `exec()`, use `_exit()` after minimal error reporting.

**Mechanism**

`exit()` runs user-space cleanup: `atexit()` handlers and stdio flushing, then eventually terminates the process. `_exit()` goes directly to kernel termination. Because stdio buffers are ordinary user memory, a buffered line present before `fork()` may exist in both parent and child.

**Pitfalls**

- Duplicate log lines appear when stdio data is unflushed at `fork()` time; redirection and block buffering make it common, but terminal output can duplicate too if partial data remains buffered.
- Child cleanup closes or mutates resources that only the parent should manage.
- A failed child returns into the parent's accept loop and creates extra workers.
- The child does too much work before preserving the real `exec()` `errno`.
- Exit status above 255 is truncated for normal wait-status reporting.

**Debug angle**

Use:

```bash
strace -f -e trace=execve,write,exit_group ./program >out.log
stdbuf -o0 ./program
```

Compare terminal and redirected output. Look for both parent and child writing the same buffered bytes.

**Follow-up keywords**

`exit()`, `_exit()`, `atexit()`, stdio buffering, failed `exec()`, `errno`, exit status low 8 bits, duplicated output

### 3. A long-running supervisor accumulates `<defunct>` children under load. How would you design child reaping?

**What the interviewer is testing**

Whether you know what a zombie is, why it exists, and how to design reaping in a real supervisor or event loop.

**Strong answer**

A zombie is a child that has exited but whose parent has not collected its status. The parent should treat `SIGCHLD` as a notification and then loop with `waitpid(-1, &status, WNOHANG)` until no more children are waitable. The supervisor should record exit code or signal death, apply restart/backoff policy, and avoid doing complex work directly in a signal handler.

**Mechanism**

When a child exits, the kernel releases most resources but keeps PID, status, and accounting data until the parent waits. Signals can coalesce, so one `SIGCHLD` can represent multiple exited children. `waitpid()` both obtains status and reaps the zombie. `waitid()` is useful when you want a `siginfo_t` result, explicit event selection, or `WNOWAIT` inspection before final reaping.

**Pitfalls**

- Reaping only one child per `SIGCHLD`.
- Calling non-async-signal-safe functions from a signal handler.
- Using `pause()` with a flag and missing a signal between the check and sleep.
- Trying to kill a zombie; it is already dead.
- Ignoring status and losing crash or core-dump evidence.
- Writing a PID 1 or container init that does not reap children.
- Using `WNOWAIT` with `waitid()` and forgetting to reap later.

**Debug angle**

Use:

```bash
ps -eo pid,ppid,stat,cmd | awk '$3 ~ /Z/ {print}'
ps -o pid,ppid,pgid,sid,stat,cmd -p <PID>
pstree -ap
strace -f -e trace=wait4,waitid ./supervisor
```

The zombie's PPID points to the process responsible for reaping, unless reparenting has already occurred. If `waitid(..., WNOHANG)` reports no waitable child, verify whether you selected the right `idtype`/`id` and whether another path already reaped it.

**Follow-up keywords**

zombie, orphan, `SIGCHLD`, `waitpid(-1, ..., WNOHANG)`, `waitid()`, `siginfo_t`, `P_PID`, `P_PGID`, `P_ALL`, `WEXITED`, `WSTOPPED`, `WCONTINUED`, `WNOWAIT`, `ECHILD`, self-pipe, `signalfd()`, `sigsuspend()`, PID 1, subreaper

### 4. A pipeline or helper hangs forever waiting for EOF. How do inherited FDs and close-on-exec bugs cause this?

**What the interviewer is testing**

Whether you can connect process FD inheritance to a classic production hang.

**Strong answer**

EOF on a pipe arrives only when every write end is closed. After `fork()`, children inherit the parent's FD table. After `exec()`, FDs remain open unless close-on-exec is set. If the parent, a sibling child, or an execed helper accidentally keeps a write end open, the reader blocks forever. The fix is to close unused pipe ends in every process and mark unrelated FDs close-on-exec at creation time.

**Mechanism**

An FD table entry is process-local, but it points to a kernel open file description. `fork()` copies FD table entries. `execve()` preserves descriptors unless `FD_CLOEXEC` is set. Close-on-exec is an FD flag; file status flags such as `O_NONBLOCK` live in the open file description.

**Pitfalls**

- Closing unused FDs in the parent but not in every child.
- Setting `FD_CLOEXEC` with `fcntl()` after `open()` in multi-threaded code, leaving a fork/exec race.
- Forgetting that `dup2()` can clear close-on-exec on the target descriptor.
- Leaking privileged device or socket FDs into untrusted helpers.
- Assuming `exec()` closes FDs by default.

**Debug angle**

Use:

```bash
ls -l /proc/<PID>/fd
lsof -p <PID>
strace -f -e trace=pipe,pipe2,openat,dup,dup2,dup3,fcntl,close,execve ./program
```

For a pipe hang, identify the pipe inode and every process that still has its write end open.

**Follow-up keywords**

FD table, open file description, `FD_CLOEXEC`, `O_CLOEXEC`, `SOCK_CLOEXEC`, `pipe2()`, `dup2()`, `dup3()`, EOF, `/proc/<PID>/fd`, `lsof`

### 5. A process forks after loading a large model on a memory-limited target. Explain copy-on-write and why globals are not IPC.

**What the interviewer is testing**

Whether you understand process memory ownership, `fork()` cost, and embedded memory pressure beyond the slogan "fork copies the process."

**Strong answer**

On Linux systems with an MMU, `fork()` usually uses copy-on-write. Parent and child initially map the same physical pages where possible. When either process writes to a shared private page, the kernel copies that page for the writer. This makes fork-then-exec efficient, but not free: page tables, task structures, overcommit policy, and dirty-page duplication still matter.

After `fork()`, normal globals, heap, and stack are logically private. If the child writes a global variable, the parent will not see the update. Use IPC, shared memory with synchronization, pipes, sockets, or another explicit channel.

**Mechanism**

The child gets a separate virtual address space and a copy of the parent's FD table. Private anonymous mappings are separated by CoW. `MAP_SHARED` mappings and shared open file descriptions are different: they can intentionally share data or offsets.

**Pitfalls**

- Assuming a parent's globals reflect child changes.
- Dirtying many pages after `fork()` and unexpectedly increasing RSS.
- Measuring only VSZ and missing RSS growth.
- Forking a huge multi-threaded service only to launch a tiny helper.
- Forgetting no-MMU or constrained embedded environments may favor `posix_spawn()` or `vfork()`-style patterns.

**Debug angle**

Use:

```bash
cat /proc/<PID>/maps
cat /proc/<PID>/smaps_rollup
pmap -x <PID>
ps -o pid,ppid,vsz,rss,stat,cmd -p <PID>
strace -f -e trace=fork,clone,vfork,execve,mmap,brk ./program
```

Watch RSS before and after child writes. Check target overcommit and cgroup memory policy when `fork()` fails only on the board.

**Follow-up keywords**

copy-on-write, page table, page fault, RSS, VSZ, `MAP_SHARED`, `MAP_PRIVATE`, pipe, socketpair, shared memory, no-MMU, `posix_spawn()`

### 6. A shell-like runner must make `Ctrl-C` stop a whole foreground pipeline without killing the parent shell. How do process groups and sessions solve this?

**What the interviewer is testing**

Whether you separate parent-child relationships from job-control relationships.

**Strong answer**

Put every process in the pipeline into one process group. The shell remains in its own process group. For a foreground job, the shell gives the terminal foreground process group to the job; terminal-generated `Ctrl-C` then sends `SIGINT` to that foreground process group, not just one PID and not the shell. After the job exits or stops, the shell restores itself as the foreground process group and reaps children.

**Mechanism**

The parent-child tree decides who created and waits for whom. Process groups decide which processes receive group-directed signals. Sessions group process groups under a login or terminal context. A controlling terminal has one foreground process group; terminal signals such as `SIGINT`, `SIGQUIT`, and `SIGTSTP` go there.

**Pitfalls**

- Sending `SIGINT` to one PID instead of the process group.
- Leaving pipeline children in the shell's process group.
- Calling `setpgid()` too late after `exec()`.
- Forgetting to restore terminal foreground control to the shell.
- Confusing PID, PPID, PGID, SID, and terminal foreground PGID.

**Debug angle**

Use:

```bash
ps -o pid,ppid,pgid,sid,tpgid,tty,stat,cmd
pstree -ap
strace -f -e trace=setpgid,setsid,ioctl,kill,wait4 ./runner
```

Inspect PGID/SID/TPGID. If a background job stops when reading terminal input, look for `SIGTTIN`.

**Follow-up keywords**

process group, session, controlling terminal, foreground process group, `setpgid()`, `setsid()`, `tcsetpgrp()`, `SIGINT`, `SIGTSTP`, `SIGTTIN`, `kill(-pgid, sig)`

### 7. A service is being ported from classic daemon mode to systemd or an embedded init system. What changes?

**What the interviewer is testing**

Whether you understand daemon mechanics and modern supervision contracts.

**Strong answer**

Classic daemonization detaches from the terminal: fork, parent exits, child calls `setsid()`, optionally fork again, sets `umask` and working directory deliberately, closes inherited FDs, redirects standard FDs, initializes logging, and handles shutdown/reload signals.

Under systemd or many embedded service managers, the process often should stay foreground. The manager tracks the process or cgroup, captures logs, applies limits, restarts on failure, and controls dependencies. Double-forking behind a manager that expects a foreground service can make PID tracking, readiness, logging, and restart behavior wrong.

**Mechanism**

`setsid()` creates a new session and process group when the caller is not already a process group leader. The optional second fork prevents the final daemon from being a session leader, reducing the chance of reacquiring a controlling terminal. Modern service managers externalize supervision and often prefer a simple foreground process.

**Pitfalls**

- Self-daemonizing under a `Type=simple` style service contract.
- Keeping terminal, socket, update-file, mount, or device FDs open accidentally.
- Writing logs to a removed file and never reopening.
- Not handling `SIGTERM` for clean shutdown.
- Treating `SIGHUP` reload/reopen as automatic rather than a documented convention.
- Changing cwd or `umask` and breaking relative paths or file modes.

**Debug angle**

Use:

```bash
ps -o pid,ppid,pgid,sid,tty,stat,cmd -p <PID>
readlink /proc/<PID>/cwd
ls -l /proc/<PID>/fd
systemctl status <service>
systemctl show <service> -p MainPID -p Type -p LimitNOFILE -p Restart
strace -f -e trace=fork,setsid,chdir,umask,close,dup2,openat ./service
```

On non-systemd targets, inspect init scripts, watchdog expectations, and the actual parent with `pstree -ap`.

**Follow-up keywords**

daemon, `setsid()`, double fork, controlling terminal, `SIGHUP`, `SIGTERM`, `syslog()`, journald, foreground service, systemd `Type=simple`, `Type=forking`, cgroup, BusyBox init

### 8. A privileged helper must open a protected device, then continue safely as an unprivileged user. How should credentials be handled?

**What the interviewer is testing**

Whether you can design least-privilege process behavior using real/effective/saved IDs and groups.

**Strong answer**

Run with privilege only for the narrow operation that needs it. Before privileged execution, sanitize argv, env, cwd, and inherited FDs. Open the protected device or perform setup, then drop supplementary groups and GID before dropping UID. Prefer a permanent drop for services. Verify with `getuid()`, `geteuid()`, `getgid()`, `getegid()`, and `/proc/<PID>/status`.

For set-user-ID helpers, understand that `exec()` can change the effective UID/GID and set saved IDs, but Linux can suppress elevation because of `nosuid`, `no_new_privs`, tracing/security policy, set-ID script rules, or capability interactions. Temporary effective-ID changes are only appropriate for small, audited code that intentionally regains privilege.

**Mechanism**

The real UID/GID identify the origin account. The effective UID/GID are normally used for permission checks. Saved IDs let some set-ID programs drop and regain effective privilege under rules. Supplementary groups also participate in group permission checks. Linux filesystem IDs usually track effective IDs in ordinary code.

**Pitfalls**

- Dropping UID before groups, then losing permission to drop groups.
- Using `system()` or `PATH` search while privileged.
- Trusting environment variables, current working directory, or inherited descriptors.
- Thinking `seteuid()` is always a permanent drop.
- Forgetting capabilities, device-node ownership, udev rules, mount options, and namespaces can differ on target.
- Forgetting `nosuid`, `no_new_privs`, tracing/security policy, or set-ID script behavior can explain why a set-ID helper does not elevate.
- Leaving a privileged FD open across `exec()` into untrusted code.

**Debug angle**

Use:

```bash
id
ps -o pid,user,euser,group,egroup,cmd -p <PID>
cat /proc/<PID>/status | grep -E 'Uid|Gid|Groups|Cap'
namei -l /dev/<device>
ls -l /proc/<PID>/fd
strace -f -e trace=setuid,setgid,setresuid,setresgid,setgroups,execve,openat ./helper
```

Confirm credentials at the protected operation and after the drop.

**Follow-up keywords**

real UID, effective UID, saved set-user-ID, supplementary groups, filesystem UID, set-user-ID, `setuid()`, `seteuid()`, `setresuid()`, `setgroups()`, capabilities, device nodes

### 9. `fork()`, `accept()`, or `open()` starts failing only on the target device. How do `RLIMIT_*`, cgroups, and `/proc` fit?

**What the interviewer is testing**

Whether you can debug resource exhaustion from syscall symptoms instead of guessing.

**Strong answer**

Start with the failing syscall and `errno`. `fork()` can fail with `EAGAIN` because of `RLIMIT_NPROC`, system thread limits, PID exhaustion, or cgroup PID limits. `open()`, `socket()`, `accept()`, `pipe()`, or `dup()` can fail with `EMFILE` when the process reaches `RLIMIT_NOFILE`. `malloc()` or `mmap()` failures may involve `RLIMIT_AS`, memory cgroups, or overcommit policy. Missing core dumps often trace to `RLIMIT_CORE=0` or core dump routing.

Inspect the real launch environment: `/proc/<PID>/limits`, `/proc/<PID>/fd`, process counts, cgroup settings, and service-manager limits.

**Mechanism**

Each process has soft and hard rlimits. The soft limit is enforced; the hard limit is the ceiling for raising the soft limit. Limits are inherited across `fork()` and usually survive `exec()`. Cgroups and service managers can impose additional process-group or service-wide constraints.

**Pitfalls**

- Raising `ulimit` in an interactive shell but not in the service unit or init script.
- Hiding an FD leak by raising `NOFILE`.
- Retrying forever on `EMFILE` or `EAGAIN`.
- Forgetting one client may consume multiple FDs.
- Enabling unlimited cores on flash storage.
- Assuming host and container limits match.

**Debug angle**

Use:

```bash
cat /proc/<PID>/limits
ls -l /proc/<PID>/fd | wc -l
ulimit -n
ulimit -u
ulimit -c
systemctl show <service> -p LimitNOFILE -p TasksMax
cat /sys/fs/cgroup/pids.max 2>/dev/null
strace -f -e trace=fork,clone,accept,openat,socket,pipe,dup ./service
```

Correlate errno with live counts and the process's actual inherited limits.

**Follow-up keywords**

`RLIMIT_NOFILE`, `RLIMIT_NPROC`, `RLIMIT_CORE`, `RLIMIT_AS`, `EMFILE`, `EAGAIN`, `ENOMEM`, cgroup `pids.max`, systemd `TasksMax`, `/proc/<PID>/limits`, `/proc/<PID>/fd`

### 10. A multi-process system becomes slower as you add workers. How would you reason about scheduling and context switches?

**What the interviewer is testing**

Whether you distinguish concurrency, parallelism, CPU saturation, blocking I/O, scheduler policy, and measurement-driven tuning.

**Strong answer**

Concurrency means tasks make progress over overlapping time. Parallelism means tasks run at the same instant on different CPUs. Adding processes can improve throughput only until another bottleneck dominates: CPU, memory bandwidth, cache locality, IPC, locks, I/O, FD limits, or context-switch overhead.

Measure before changing policy. Check CPU utilization, run queue pressure, voluntary and involuntary context switches, page faults, I/O wait, worker count, and latency. For normal workloads, tune worker count and nice values first. Use CPU affinity only after measuring cache locality, interrupt placement, and imbalance. Treat realtime policies such as `SCHED_FIFO` and `SCHED_RR` as privileged tools that can starve the system if misused.

**Mechanism**

A runnable task is ready for CPU; a running task is actually executing on a CPU. A context switch saves one task's execution state and restores another's. Voluntary switches usually happen when a task blocks or yields; `getrusage()` reports them as `ru_nvcsw`. Involuntary switches happen when the scheduler preempts a runnable task; `getrusage()` reports them as `ru_nivcsw`.

Nice affects normal scheduling weight; realtime policies can preempt ordinary work. CPU affinity is a Linux CPU mask restricting where a task can run. Affinity is inherited across `fork()` and preserved across `exec()`; on Linux it is effectively per thread/task, so thread-heavy programs need care.

**Pitfalls**

- Calling time-sliced single-core concurrency "parallelism".
- Adding CPU-bound workers beyond useful core capacity.
- Mistaking high CPU usage for useful throughput.
- Using realtime scheduling to cover up blocking or bad design.
- Pinning workers without checking core imbalance.
- Assuming process-level affinity automatically describes every thread's runtime behavior in a threaded service.
- Ignoring target-specific thermal throttling, CPU frequency scaling, interrupts, and driver behavior.
- Pinning work to an isolated CPU and then forgetting IRQs, kernel threads, or watchdog deadlines on the embedded target.

**Debug angle**

Use:

```bash
ps -eLo pid,tid,cls,rtprio,pri,ni,psr,stat,comm
pidstat -w -p <PID> 1
vmstat 1
top -H -p <PID>
perf stat -e context-switches,cpu-migrations,page-faults -p <PID>
cat /proc/<PID>/sched
cat /proc/<PID>/status | grep Cpus_allowed_list
chrt -p <PID>
taskset -pc <PID>
```

If available on target, use `perf sched`, ftrace, or `trace-cmd` to connect scheduling latency to workload behavior.

**Follow-up keywords**

concurrency, parallelism, runnable vs running, context switch, voluntary/involuntary switch, `ru_nvcsw`, `ru_nivcsw`, run queue, CPU-bound, I/O-bound, `getrusage()`, `pidstat`, `perf stat`, nice, `SCHED_FIFO`, `SCHED_RR`, CPU affinity, isolated CPU

### 11. A worker crashes only on the embedded target. How would you investigate and make core dumps useful safely?

**What the interviewer is testing**

Whether you can debug C/C++ process crashes after deployment while respecting embedded storage and security constraints.

**Strong answer**

First decode how the child died. If a parent reaps it, use `WIFSIGNALED()`, `WTERMSIG()`, and, where available, `WCOREDUMP()`. Then check whether core dumps are enabled and where they are routed: `RLIMIT_CORE`, `/proc/sys/kernel/core_pattern`, dumpability, permissions, filesystem state, and systemd-coredump policy. Debug the core with the exact target binary, build ID, debug symbols, and matching shared libraries.

On embedded devices, full cores may be too large or sensitive. Use a controlled debug image, bounded retention, `coredump_filter`, `MADV_DONTDUMP`, minidumps, or crash metadata when full dumps are unsafe.

**Mechanism**

A core dump is a postmortem memory image produced for certain fatal signal terminations. The kernel and user-space collector decide whether and where it is stored. The crashing process's cwd, root, mount namespace, permissions, and core pattern can affect traditional core file placement.

**Pitfalls**

- Looking only at logs and not wait status.
- `RLIMIT_CORE=0`, full/read-only filesystem, or core collector disabled.
- Debugging with a host binary that does not match the target image.
- Filling flash with repeated cores during a crash loop.
- Leaking secrets because cores contain process memory.
- Forgetting `SIGBUS` from bad mappings or device/storage-backed mappings.

**Debug angle**

Use:

```bash
ulimit -c
cat /proc/<PID>/limits
cat /proc/sys/kernel/core_pattern
cat /proc/<PID>/coredump_filter
coredumpctl list
coredumpctl info <PID>
gdb <binary> <core>
dmesg | tail
strace -f -e trace=wait4,waitid ./supervisor
```

Record signal, PID namespace context, executable path, build ID, library versions, and storage policy before moving evidence off target.

**Follow-up keywords**

core dump, `RLIMIT_CORE`, `core_pattern`, `coredumpctl`, `gdb`, `WIFSIGNALED`, `WTERMSIG`, `WCOREDUMP`, `SIGSEGV`, `SIGABRT`, `SIGBUS`, debug symbols, build ID, `coredump_filter`

### 12. A program uses `system()` with user-controlled input. How would you replace it safely?

**What the interviewer is testing**

Whether you understand shell injection, environment/PATH risk, secure process launch, `system()` wait-status semantics, and why privileged programs should avoid shell execution.

**Strong answer**

Avoid shell command strings for untrusted input. Build an explicit `argv[]`, choose an exact executable path, use a controlled `envp[]`, set up descriptors deliberately, then use `fork()` plus `execve()` or `posix_spawn()` with file actions. If privilege is involved, do not rely on `PATH`, inherited environment, cwd, or inherited FDs. Add timeout/cancellation and decode wait status.

If you must discuss existing `system()` code, remember that its return value is a wait status for the shell, not a plain exit code. Status `127` is ambiguous: it may mean the shell could not be execed, or it may be a shell command's real exit status. `system()` also creates an extra shell process and has signal-handling behavior that can interact with `SIGCHLD`, `SIGINT`, and `SIGQUIT`.

**Mechanism**

`system(command)` runs a shell, conceptually `/bin/sh -c command`, then waits for that shell. That invokes shell parsing, quoting, expansion, globbing, redirection, environment effects, and `PATH` behavior. The shell may create more children for pipelines or compound commands, so `system()` has more overhead and less direct control than explicit `fork()`/`execve()`/`waitpid()`.

`execve(path, argv, envp)` executes one program with explicit arguments and no shell metacharacter interpretation. The parent then waits for the exact child it created and decodes status with the normal wait macros.

**Pitfalls**

- Shell injection through semicolons, spaces, backticks, `$()`, globbing, redirection, or variable expansion.
- PATH hijacking in privileged code.
- Incorrect quoting that works in tests but fails on real input.
- Blocking forever without a timeout.
- Leaking descriptors into the child.
- Assuming BusyBox `/bin/sh` behaves exactly like a desktop shell.
- Treating `system()` return value as a plain exit code instead of a wait status.
- Misreading status `127` as definitely "command not found" or definitely "shell exec failed."
- Using `system()` in set-user-ID/set-group-ID or otherwise privileged code.
- Forgetting that `system()` signal handling may make `SIGINT`/`SIGQUIT` behavior surprising in loops or interactive tools.

**Debug angle**

Use:

```bash
strace -f -e trace=execve,clone,vfork,fork,wait4,waitid ./program
env -i ./program
readlink /proc/<PID>/exe
ls -l /proc/<PID>/fd
```

Confirm the exact executable, argv, environment, inherited descriptors, extra shell process, signal death vs normal exit, and final wait status.

**Follow-up keywords**

`system()`, `/bin/sh -c`, shell injection, ambiguous `127`, `SIGCHLD`, `SIGINT`, `SIGQUIT`, set-user-ID risk, wait status, `WIFEXITED`, `execve()`, `execvp()`, argv, envp, PATH, `posix_spawn()`, close-on-exec, timeout

### 13. Workers keep running after the parent dies inside a container or embedded init environment. How do orphaning, PID 1, and subreapers affect cleanup?

**What the interviewer is testing**

Whether you understand process ownership beyond the happy path, especially for supervisors, containers, and embedded init systems.

**Strong answer**

Parent death does not automatically kill children. Live children whose parent exits become orphans and are reparented to PID 1 or to the nearest configured subreaper. They continue running unless signaled or supervised by another mechanism. A container or embedded PID 1 must reap children and forward shutdown signals properly. If the design requires children to die with the parent, use process groups, cgroups, explicit signal forwarding, parent-death signals where appropriate, or a real supervisor contract.

**Mechanism**

An orphan is a live process whose original parent exited. A zombie is a dead child not yet waited for. Linux reparents orphaned descendants to init/systemd or a subreaper configured with `PR_SET_CHILD_SUBREAPER`. PID namespaces mean the same process can have different PIDs inside and outside a container.

**Pitfalls**

- Assuming killing the parent kills the whole process tree.
- Running a shell script as PID 1 that neither reaps nor forwards signals.
- Reaping direct children but losing grandchildren.
- Killing by process group without understanding what else is in the group.
- Confusing host PID with namespace PID.

**Debug angle**

Use:

```bash
pstree -ap
ps -eo pid,ppid,pgid,sid,stat,cmd
cat /proc/1/status
cat /proc/<PID>/status | grep -E 'Pid|PPid|NSpid'
strace -p 1 -e trace=wait4,waitid
```

Inspect who owns the workers now, whether PID 1 waits, and whether shutdown signals reach the intended process group or cgroup.

**Follow-up keywords**

orphan, zombie, PID 1, subreaper, `PR_SET_CHILD_SUBREAPER`, PID namespace, process group, cgroup, parent-death signal, signal forwarding

### 14. A short-lived helper disappeared before logs captured it. How would you use process accounting without confusing it with supervision?

**What the interviewer is testing**

Whether you can distinguish live supervision, wait status, `/proc` evidence, service logs, `getrusage()`, and kernel process accounting.

**Strong answer**

First, do not use process accounting as the control path. The parent or service manager still owns supervision, `wait*()` reaping, restart policy, and live logs. Process accounting is post-exit audit evidence: if enabled, the kernel writes a record when a process terminates. Use it to investigate short-lived commands that vanished before `/proc` inspection.

Check whether accounting is enabled, where records are stored, whether the kernel/config supports it, and whether storage/privacy limits are acceptable. Correlate accounting output with service logs, wait status, PID namespace context, executable path, UID/GID, start time, elapsed time, CPU time, exit/signal/core information, and the last executed command name.

**Mechanism**

`acct(path)` enables accounting to an existing file; `acct(NULL)` disables it. Records are written at process termination and are ordered by termination time. A process that `execve()`s another image may appear under the last executed command name. No termination record is written for a still-running process or for a process lost during a crash before termination accounting completes.

**Pitfalls**

- Treating accounting as a replacement for `waitpid()`, logs, or a supervisor.
- Forgetting accounting may be disabled, unsupported, or version/config dependent.
- Misreading termination order as start order.
- Missing processes still running at crash time.
- Filling embedded flash or leaking sensitive operational data.
- Confusing login accounting with process accounting.

**Debug angle**

Use:

```bash
cat /proc/sys/kernel/acct
lastcomm
sa
journalctl -u <service>
ps -eo pid,ppid,pgid,sid,stat,cmd
cat /proc/<PID>/status 2>/dev/null
```

For parent-owned helpers, compare accounting with `waitpid()`/`waitid()` status and `getrusage(RUSAGE_CHILDREN)` after reaping.

**Follow-up keywords**

process accounting, `acct()`, `lastcomm`, `sa`, `/proc/sys/kernel/acct`, termination record, `execve()` command name, UID/GID, elapsed time, CPU time, exit status, signal death, core flag, Embedded storage

---

## Comparison Questions

### 15. Program vs process

A program is a passive executable file and metadata on storage. A process is a running instance with PID, virtual address space, FDs, credentials, signal state, scheduler state, resource accounting, and lifecycle.

### 16. Process vs thread

Processes provide stronger isolation, separate address spaces, and clearer privilege/lifecycle boundaries. Threads share one process address space and FDs, making sharing cheaper but data races, deadlocks, and memory corruption more dangerous.

### 17. `fork()` vs `exec()`

`fork()` creates a new child process that continues from the same call site. `exec()` replaces the current process image with a new program and keeps the same PID. Shells combine them so the shell survives while the child becomes the command.

### 18. `fork()` vs `vfork()` vs `clone()` vs `posix_spawn()`

| API | Practical answer |
|---|---|
| `fork()` | General process creation; child gets separate process-style ownership, usually with CoW. |
| `vfork()` | Restricted fork-before-exec path; parent is suspended and child must do very little before `exec()` or `_exit()`. |
| `clone()` | Linux-specific low-level primitive for selectable sharing; basis for threads/namespaces, rare in normal app code. |
| `posix_spawn()` | Spawn abstraction with file actions and attributes; useful for constrained or multi-threaded programs. |

### 19. What is tricky about `fork()` from a multi-threaded process?

Only the calling thread exists in the child. Locks held by vanished threads remain in memory and may never be released. The child should do only async-signal-safe work before `exec()` or `_exit()`. `posix_spawn()` or a dedicated fork helper is often cleaner.

### 20. `wait()` vs `waitpid()` vs `waitid()` / `wait4()`

`wait()` reaps any waitable child. `waitpid()` is the main supervisor API because it can select one PID, any child, the caller's process group, or a child process group, and can use `WNOHANG`, `WUNTRACED`, and `WCONTINUED`.

`waitid(idtype, id, &info, options)` provides a structured `siginfo_t` interface. Use `P_ALL`, `P_PID`, or `P_PGID` to select children; use `WEXITED`, `WSTOPPED`, and `WCONTINUED` to choose event classes; combine with `WNOHANG` for polling or `WNOWAIT` to inspect status without consuming the waitable state yet. With `WNOHANG`, initialize `siginfo_t` and check `si_pid`, because success can mean either "event returned" or "no child changed state." Unlike `waitpid(0, ...)`, `waitid(P_PGID, ...)` needs an explicit process group ID such as `getpgrp()`.

`wait4()` is a BSD/Linux-style extension that combines selected waiting with `struct rusage`; recognize it for resource debugging but prefer portable APIs unless the platform contract allows it.

### 21. Why decode wait status with macros?

The integer wait status is encoded. Use `WIFEXITED()`, `WEXITSTATUS()`, `WIFSIGNALED()`, `WTERMSIG()`, and related macros. Do not treat raw status `256` as exit code `256`; it may mean the child exited with code `1`.

### 22. FD table vs open file description

The FD table is per process and maps integers to kernel objects. An open file description stores file offset and file status flags. After `fork()` or `dup()`, different FDs can refer to the same open file description, so offsets and status flags can be shared.

### 23. FD flags vs file status flags

`FD_CLOEXEC` is an FD flag on one descriptor entry. `O_APPEND` and `O_NONBLOCK` are file status flags in the open file description and may be observed through duplicated or inherited descriptors.

### 24. `execve()` vs `execvp()` / `execlp()` / `execvpe()`

| API | Lookup | Args | Environment | Interview rule |
|---|---|---|---|---|
| `execve()` | Exact path | vector | explicit `envp` | Best for privileged/service launch contracts. |
| `execvp()` | `PATH` if no slash | vector | inherit | Shell-like convenience; risky in privileged code. |
| `execlp()` | `PATH` if no slash | list | inherit | Same PATH risk, awkward for dynamic args. |
| `execvpe()` | GNU extension, PATH-style lookup | vector | explicit `envp` | Recognize it; verify portability and PATH semantics before using. |

### 25. `exit()` vs `_exit()`

`exit()` runs C library cleanup and flushes stdio. `_exit()` terminates directly through the kernel path. Normal programs use `return` from `main()` or `exit()`; child failure paths after `fork()` and before successful `exec()` usually use `_exit()`.

### 26. Zombie vs orphan

A zombie is dead but not reaped; it holds status until the parent waits. An orphan is alive but its original parent exited; it is reparented to init/systemd or a subreaper.

### 27. Parent tree vs process group

The parent tree answers "who created and waits for whom?" A process group answers "which processes form one job and receive group signals?" A shell pipeline can have multiple child processes in one process group.

### 28. `setuid()` vs `seteuid()` vs `setresuid()`

`setuid(nonroot)` from a privileged process is commonly used for permanent privilege drop. `seteuid()` changes only the effective UID and is used for temporary drop/regain when saved IDs allow it. `setresuid()` can explicitly set real, effective, and saved IDs on systems that provide it.

### 29. Nice vs realtime scheduling

Nice changes normal scheduling weight; it is not a hard latency guarantee. `SCHED_FIFO` and `SCHED_RR` are realtime policies that can starve normal tasks if misused. Use them only with clear privilege, watchdog, and blocking/yield design.

### 30. `getrusage()` vs `/proc/<PID>/stat` / `/proc/<PID>/sched` vs process accounting

`getrusage()` is a portable-ish API for CPU time, page faults, and context switches for self/children/thread scopes. It is useful while the program or parent is still in control. `/proc/<PID>/stat`, `/proc/<PID>/sched`, and `/proc/<PID>/status` expose Linux-specific live scheduling/accounting details such as current CPU, allowed CPUs, scheduler stats, and voluntary/involuntary switch counts.

Process accounting is different: it is optional post-exit audit data written when a process terminates. It can help explain a short-lived process after it disappeared, but it is not a live supervisor API and should be bounded carefully on embedded storage.

### 31. `rlimit` vs cgroups/systemd limits

`rlimit` is inherited per-process state. Cgroups control groups of processes, often services or containers. systemd can configure both classic limits and cgroup controls. Debug the process, the unit/init script, and the cgroup.

### 32. Classic daemon vs foreground service

A classic daemon detaches itself from terminal/session context and manages its own descriptor/logging behavior. A foreground service cooperates with a service manager that handles logs, limits, restarts, and process tracking. Modern Linux services usually prefer the latter unless deployment says otherwise.

---

## Recognize Only

| Topic | Recognition note |
|---|---|
| `exec*()` naming | `l` means argument list, `v` vector, `p` PATH search, `e` explicit environment; `execvpe()` is GNU-specific. Know the rule; read prototypes when coding. |
| `waitid()` | Structured child-state API using `siginfo_t`; know `WNOWAIT` and event flags, then read the manual when coding. |
| `wait4()` | Wait plus resource usage on systems that support it. |
| `SA_NOCLDWAIT` | Can prevent zombies for children but changes wait behavior; do not cargo-cult it. |
| Ignoring `SIGCHLD` | POSIX/Linux behavior around zombie creation can be subtle; use deliberately. |
| `pidfd` | Modern Linux process handle that helps avoid PID reuse races. |
| `PR_SET_CHILD_SUBREAPER` | Linux supervisor/container tool for adopting orphaned descendants before PID 1. |
| PID namespaces | Processes may see different PIDs inside containers than on the host. |
| `daemon(3)` | Helper exists, but interviews care more about lifecycle, descriptor handling, and supervision contract. |
| `fexecve()` / `execveat()` | Execute by FD or with advanced path semantics; useful in secure/specialized launchers. |
| `clone()` flags | Recognize `CLONE_VM`, `CLONE_FILES`, `CLONE_FS`, `CLONE_SIGHAND`, and `CLONE_THREAD`; normal code usually uses pthreads, `fork()`, or `posix_spawn()`. |
| Linux capabilities | Split root privilege into finer-grained bits; relevant to credential/security design beyond basic UID/GID. |
| Filesystem UID/GID | Linux-specific credential detail; usually tracks effective IDs in ordinary code. |
| `SCHED_BATCH`, `SCHED_IDLE`, `SCHED_RESET_ON_FORK` | Linux scheduling details worth recognizing, not first-pass memorization. |
| Full scheduler internals | CFS and realtime classes matter, but user-space interviews usually want symptoms, policy choice, and debug commands first. |
| Full `RLIMIT_*` catalog | Learn `NOFILE`, `NPROC`, `CORE`, `AS`, `STACK`, `CPU`, `FSIZE`, `RTPRIO`, and `RTTIME` first; look up the rest. |
| Process accounting | Kernel can record terminated-process accounting data; useful for audit, not normal control flow. |
| Orphaned process groups | Job-control edge case where stopped orphaned groups may receive hangup/continue behavior; recognize before deep-diving. |
| Core dump security controls | `coredump_filter`, dumpability, `MADV_DONTDUMP`, and collector policy matter when cores may expose secrets. |

## Remaining Coverage Gaps

No known interview coverage gaps remain for Chapter 03 after this trace:

- Every learning-map row 3.1 through 3.11 appears in Priority A, B, or C.
- Every Chapter 03 Must Cover concept appears as a scenario, comparison, or recognize-only item.
- High-impact production concepts are covered as Priority A scenarios or Priority B comparisons, not only as follow-up keywords.
