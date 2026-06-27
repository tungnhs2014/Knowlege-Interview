# Chapter 3 - Process Advanced

> Topics: 3.7 Process Groups, Sessions & Job Control · 3.8 Process Credentials · 3.9 Daemons · 3.10 Process Priorities & Scheduling · 3.11 Process Resources & Limits
> Main sources: TLPI Ch34, Ch09, Ch35, Ch36, Ch37
> Production context: shells, terminals, privileged helpers, daemons, service hardening, latency/resource debugging

## Learning Goal

Understand the process attributes that matter after basic `fork()`, `execve()`, `exit()`, and `waitpid()` are clear. After this file, you should be able to explain process groups and sessions, terminal job control, credentials, daemon lifecycle, scheduling policy, CPU priority, and resource limits in production terms.

- Credentials overlap with [ch01_users_and_groups.md](ch01_users_and_groups.md); this file focuses on process behavior and lifecycle.
- Process creation basics are in [ch03_process_core.md](ch03_process_core.md).
- Child monitoring and program execution are in [ch03_process_execution.md](ch03_process_execution.md).

## Coverage Notes

This file covers learning-map rows **3.7 Process Groups, Sessions & Job Control**, **3.8 Process Credentials**, **3.9 Daemons**, **3.10 Process Priorities & Scheduling**, and **3.11 Process Resources & Limits**.

Covered here:

- Process groups, sessions, controlling terminals, foreground/background signal behavior, job-control races, orphaned process groups, `SIGHUP`, `SIGCONT`, `SIGTTIN`, and `SIGTTOU`.
- Process credentials as runtime attributes: real/effective/saved IDs, supplementary groups, set-user-ID/set-group-ID behavior, privilege drop, and deployment rules that suppress elevation.
- Classic daemon creation, modern foreground service-manager lifecycle, `syslog()`, shutdown/reload signals, and Embedded watchdog/service constraints.
- CPU scheduling, nice values, realtime policies, context switches, CPU affinity, and runaway realtime risk.
- Resource usage, `getrusage()`, soft/hard `rlimit` values, process accounting, core dumps, `/proc`, `ps`, `strace`, `perf`, service logs, and service-manager/cgroup limits.

Moved or intentionally scoped elsewhere:

- Process fundamentals, basic `fork()`, CoW, FD sharing, globals-not-IPC, and termination API basics are in [ch03_process_core.md](ch03_process_core.md).
- `execve()`, `exec*()`, close-on-exec, wait/reap, zombies/orphans, subreapers/PID namespaces, and detailed fork/exec attribute rules are in [ch03_process_execution.md](ch03_process_execution.md).
- Credential fundamentals also appear in [ch01_users_and_groups.md](ch01_users_and_groups.md); this file keeps the process-lifecycle and privilege-transition angle.

## Problem It Solves

Production systems need more than "start a child and wait". They need grouped signal delivery, terminal control, privilege boundaries, service detachment, CPU behavior, and resource containment.

- `Ctrl-C` should affect the foreground job, not the whole login session.
- A background job should not freely read from the terminal.
- A helper may need privilege briefly, then must drop it.
- A daemon must survive without depending on a terminal.
- A realtime task must not lock up the device.
- A server must not consume every FD, process slot, byte of memory, or core dump budget.

```text
basic lifecycle:
    fork -> exec -> exit -> wait

advanced controls:
    process group/session -> terminal/job behavior
    credentials           -> permission checks
    daemon lifecycle      -> service behavior
    scheduling            -> CPU latency/share
    resource limits       -> containment and failure mode
```

## Mental Model

Linux process state has several "trees" and attribute sets. Parent-child tells you who created and waits for whom; process groups and sessions tell you which processes act together as terminal jobs.

| Structure | Answers | Example |
|---|---|---|
| Parent-child tree | Who created whom? Who reaps whom? | Shell waits for command child. |
| Process group | Which processes form one job? | `sort file \| uniq -c` in one PGID. |
| Session | Which process groups share terminal/login context? | Shell session with foreground/background jobs. |
| Credentials | Who is this process allowed to act as? | Effective UID checked for opening protected files. |
| Scheduling attributes | How should CPU time be distributed? | `nice`, `SCHED_FIFO`, CPU affinity. |
| Resource limits | What consumption is allowed? | `RLIMIT_NOFILE`, `RLIMIT_CORE`, `RLIMIT_NPROC`. |
| Process accounting | What did this process consume before it exited? | Optional terminated-process audit records. |

Parent tree vs job-control tree:

```text
Parent-child:
shell
  +-- sort
  +-- uniq

Job control:
Session SID=400
  +-- PGID=400 shell
  +-- PGID=660 sort | uniq
```

## Mechanism

Advanced process behavior is mostly controlled by kernel attributes attached to the process or to related process groups/sessions.

### Process Groups, Sessions, And Terminals

A **process group** is a set of related processes, usually one shell job. A **session** is a set of process groups, usually tied to one login or terminal context.

```text
Session SID=400
|
+-- Process Group PGID=400  shell
|   +-- bash PID=400
|
+-- Process Group PGID=658  background job
|   +-- find PID=658
|   +-- wc   PID=659
|
+-- Process Group PGID=660  foreground job
    +-- sort PID=660
    +-- uniq PID=661
```

Terminal model:

```text
controlling terminal
    |
    +--> belongs to one session
    +--> has one foreground process group
    +--> sends Ctrl-C/Ctrl-Z/Ctrl-\ signals to that foreground process group
```

Important rules:

- `Ctrl-C` generates `SIGINT` for the foreground process group.
- `Ctrl-Z` generates `SIGTSTP` for the foreground process group.
- Background reads from the terminal can trigger job-control stop behavior.
- `SIGHUP` often means terminal hangup; daemons also use it by convention for reload/reopen.

### Orphaned Process Groups

An orphaned process group is a job-control condition, not just "a child whose parent died". A process group is orphaned when no member has a parent in a different process group within the same session.

Why this matters:

- A stopped background job in an orphaned process group may have no shell left to notice it and continue it.
- When a process group newly becomes orphaned and has stopped members, the kernel sends `SIGHUP` followed by `SIGCONT` to all members of that group.
- If the group has no stopped members when it becomes orphaned, that particular orphaned-group rule does not send those signals.

Terminal edge behavior:

| Situation | Behavior |
|---|---|
| Background process reads from controlling terminal | Normally gets `SIGTTIN` and stops. |
| Background process writes while terminal `TOSTOP` is set | Normally gets `SIGTTOU` and stops. |
| Orphaned process group would be stopped by `SIGTTIN`, `SIGTTOU`, or `SIGTSTP` | Stop action is suppressed because nobody may be left to resume it. |
| Orphaned process group performs terminal I/O where stopping would be useless | Operation can fail with `EIO`. |

Debug clue:

- If a detached job receives unexpected `SIGHUP`/`SIGCONT`, or terminal I/O fails with `EIO`, inspect `pid`, `ppid`, `pgid`, `sid`, `tpgid`, `tty`, and `stat`.

### Credentials

Process credentials are the identity the kernel consults for permission checks. See [ch01_users_and_groups.md](ch01_users_and_groups.md) for the user/group model; here, focus on process attributes.

```text
real UID/GID
    -> who started the process

effective UID/GID
    -> identity normally used for permission checks

saved set-user-ID/GID
    -> lets set-ID programs drop and regain privilege under rules

supplementary groups
    -> additional group permissions
```

Set-user-ID and set-group-ID executables can change effective credentials during `exec()`. That is powerful and dangerous, but it is not unconditional on Linux.

- Validate inputs before privileged operations.
- Minimize the privileged code path.
- Drop privileges as soon as possible.
- Do not trust environment variables, `PATH`, current directory, or inherited descriptors in privileged code.
- Check deployment conditions that suppress or change elevation: `nosuid` mounts, `no_new_privs`, tracing/security policy, Linux's set-ID script handling, and capability rules.

### Daemons

A daemon is a long-running service process that does not depend on a controlling terminal. Classic daemonization detaches the process; modern service managers often prefer a foreground process.

Classic shape:

```text
fork(); parent exits
child calls setsid()
optional second fork
set umask intentionally
chdir("/") or safe working directory
close inherited FDs
redirect stdin/stdout/stderr
log through syslog, journald, or explicit logs
handle SIGTERM and often SIGHUP
```

Modern service-manager shape:

```text
service manager starts process
process stays foreground
stdout/stderr handled by manager
manager supervises restart, status, logs, limits
process handles SIGTERM cleanly
```

The correct design depends on deployment. A self-daemonizing program may confuse a supervisor that expects foreground readiness.

### Scheduling And Resources

Scheduling decides which runnable task gets CPU time. Resource limits decide how much a process may consume.

```text
runnable process
    |
    +--> scheduling class/policy
    +--> nice value or realtime priority
    +--> CPU affinity
    +--> resource limits
    +--> cgroups/service-manager constraints outside classic rlimit APIs
```

For normal work, nice values influence CPU share but do not create hard isolation. Realtime policies can preempt normal tasks and must be treated as a system-risk feature.

## Key APIs And Objects

### Process Groups And Sessions

| API/object | Role | Production note |
|---|---|---|
| `getpgrp()` | Get caller's process group ID | Good for inspection/logging. |
| `getpgid(pid)` | Get PGID for a process | Useful for debuggers/shell-like tools. |
| `setpgid(pid, pgid)` | Move child/self into process group | Shells call in parent and child to avoid races. |
| `setsid()` | Create new session and process group | Caller must not already be a process group leader. |
| `getsid(pid)` | Get session ID | Inspect terminal/session relationships. |
| `tcgetpgrp(fd)` | Get foreground PGID for terminal | Job-control shell primitive. |
| `tcsetpgrp(fd, pgid)` | Set terminal foreground process group | Shell must restore itself after foreground job. |
| `kill(-pgid, sig)` | Signal a process group | Negative PID targets group in `kill()`. |

### Credentials

| API/object | Role | Production note |
|---|---|---|
| `getuid()` / `geteuid()` | Real/effective user IDs | Effective ID is normally used for permission checks. |
| `getgid()` / `getegid()` | Real/effective group IDs | Group checks include supplementary groups. |
| `getgroups()` | Supplementary groups | Important for file access surprises. |
| `setuid()` / `seteuid()` | Change user IDs | Semantics differ for privileged/unprivileged callers. |
| `setgid()` / `setegid()` | Change group IDs | Drop groups before dropping root. |
| `setresuid()` / `setresgid()` | Linux/BSD-style real/effective/saved control | Clearer for privilege transitions where available. |
| `initgroups()` / `setgroups()` | Manage supplementary groups | Required when switching service user. |

Safe privilege-drop order:

```text
start privileged
    |
    +--> sanitize environment and inherited FDs
    +--> initialize groups for target user
    +--> drop GID / supplementary groups
    +--> drop UID
    +--> verify privilege cannot be regained unless intentionally designed
```

### Daemons And Logging

| API/object | Role |
|---|---|
| `daemon()` | Library helper for daemonizing; nonportable details vary. |
| `setsid()` | Detach from old session/controlling terminal. |
| `umask()` | Control default permissions for created files. |
| `chdir()` | Avoid pinning a mount or unexpected working directory. |
| `syslog()` / `openlog()` / `closelog()` | Traditional daemon logging. |
| `sigaction()` | Reliable signal handling for shutdown/reload. |

Avoid designing daemons around `printf()` to a terminal. Decide where logs and errors go before detaching.

### Scheduling

| API/object | Role | Notes |
|---|---|---|
| `nice()` | Adjust caller's nice value | Higher nice means lower priority. |
| `getpriority()` / `setpriority()` | Get/set nice value for process/group/user | Permissions apply; range is typically -20 to +19. |
| `sched_setscheduler()` | Set scheduling policy and realtime priority | Requires privilege or resource-limit allowance for many changes. |
| `sched_getscheduler()` | Inspect policy | Useful for debugging latency surprises. |
| `sched_setparam()` / `sched_getparam()` | Set/get realtime priority | Priority matters for realtime policies. |
| `sched_yield()` | Voluntarily yield CPU | Meaningful mainly for realtime design; avoid as a casual fix. |
| `sched_setaffinity()` / `sched_getaffinity()` | Restrict/inspect eligible CPUs | Linux-specific. |
| `SCHED_OTHER` | Normal time-sharing policy | Default for ordinary processes. |
| `SCHED_FIFO` | Realtime FIFO policy | Can starve lower-priority work. |
| `SCHED_RR` | Realtime round-robin policy | Time-slices among same-priority realtime tasks. |
| `SCHED_BATCH`, `SCHED_IDLE` | Linux-specific lower-interactivity policies | Useful for background/best-effort work. |

### Resource Usage And Limits

| API/object | Role | Notes |
|---|---|---|
| `getrusage()` | Read CPU/page-fault/context-switch usage | `RUSAGE_SELF` and `RUSAGE_CHILDREN` are common. |
| `getrlimit()` | Read soft/hard limit | Limits are inherited across `fork()` and usually preserved across `exec()`. |
| `setrlimit()` | Set soft/hard limit | Unprivileged code can lower hard limits, not raise them. |
| `rlim_cur` | Soft limit | Currently enforced value. |
| `rlim_max` | Hard limit | Ceiling for soft limit. |
| `RLIM_INFINITY` | No finite limit | Still may be constrained by cgroups/systemd/kernel config. |

Common limits:

| Limit | Controls | Common symptom |
|---|---|---|
| `RLIMIT_NOFILE` | Maximum FD number plus one | `EMFILE` from `open()`, `socket()`, `accept()`, `pipe()`. |
| `RLIMIT_NPROC` | Tasks/threads for real user ID on Linux | `fork()` or `pthread_create()` can fail with `EAGAIN`. |
| `RLIMIT_CORE` | Core dump file size | No core file after crash. |
| `RLIMIT_AS` | Virtual address space | `malloc()`/`mmap()` fail with `ENOMEM`. |
| `RLIMIT_STACK` | Stack size | Deep recursion or large stack arrays crash. |
| `RLIMIT_CPU` | CPU seconds | `SIGXCPU`, then possible termination. |
| `RLIMIT_FSIZE` | File size | Writes fail or signal when file too large. |
| `RLIMIT_RTPRIO` | Realtime priority unprivileged process may set | Realtime scheduling change denied. |
| `RLIMIT_RTTIME` | Realtime CPU burst time | Protects against runaway realtime loops. |

### Process Accounting

Process accounting is terminated-process audit data. It is not a replacement for `waitpid()`, supervision, logs, or live metrics.

| API/object | Role | Notes |
|---|---|---|
| `acct(path)` | Enable process accounting to an existing file | Requires privilege such as `CAP_SYS_PACCT`; usually managed by boot/service scripts. |
| `acct(NULL)` | Disable process accounting | Stops new records from being written. |
| `lastcomm` | List commands from accounting records | Useful after short-lived processes are gone. |
| `sa` | Summarize accounting data | Traditional administrative reporting tool. |
| `/proc/sys/kernel/acct` | Disk-space watermarks and check frequency | Prevents accounting from filling storage indefinitely. |

Mechanism:

- When enabled, the kernel writes an accounting record as each process terminates.
- The record is ordered by termination time, not by process start time.
- Linux support depends on kernel configuration, commonly BSD process accounting options.
- If the system crashes while a process is still running, no termination record is written for that process.
- The command name reflects the last executed program image, so `execve()` changes what later appears in accounting.

Record content to recognize:

| Field idea | Why it matters |
|---|---|
| UID/GID, controlling terminal, start time | Who/where attribution. |
| User CPU, system CPU, elapsed time, memory/page-fault-style data | Post-exit resource audit. |
| Exit status, signal death, core-dump flag where available | Postmortem outcome. |
| Last executed command name | Helps identify short-lived commands after they disappear. |
| Flags such as fork-without-exec or superuser use | Forensics and audit clues. |

Production and Embedded notes:

- Useful when a short-lived process escaped normal monitoring.
- Dangerous on storage-constrained embedded systems if left unbounded.
- Treat accounting records as potentially sensitive operational data.
- Do not use accounting to decide immediate parent/child control flow; use `wait*()`, supervisor state, and logs.

## Lifecycle / Data Flow

### Flow 1 - Terminal `Ctrl-C`

```text
1. User presses Ctrl-C.
2. Terminal driver generates SIGINT.
3. Kernel sends SIGINT to the foreground process group.
4. Every process in that group takes its SIGINT action.
5. Shell regains terminal foreground control after job exits or stops.
```

Debug clue:

- If `Ctrl-C` kills only part of a pipeline or misses it entirely, inspect PGID/SID and terminal foreground PGID.

### Flow 1B - Orphaned Background Job

```text
shell/session leader exits or job-control parent disappears
    |
    +--> process group may become orphaned
    |
    +--> if the group has stopped members:
            kernel sends SIGHUP then SIGCONT to the group
```

Operational meaning:

- A background job can suddenly resume and receive hangup when the shell/session structure disappears.
- If terminal I/O would normally stop an orphaned group, Linux/POSIX behavior avoids creating a permanently stopped job with nobody to continue it.

### Flow 2 - Job-Control Shell Starting A Pipeline

```text
1. Shell parses: sort file | uniq -c.
2. Shell chooses a new PGID for the job.
3. Shell forks pipeline children.
4. Parent and children call setpgid() early to avoid races.
5. If foreground, shell calls tcsetpgrp() to give terminal to the job.
6. Shell waits for job state changes.
7. Shell restores itself as foreground process group.
```

Race rule:

- A shell-like program should set process groups before `exec()`; after `exec()`, timing is harder to control.

### Flow 3 - Privileged Helper

```text
1. User execs set-user-ID helper.
2. exec may change effective UID according to file owner/mode, unless Linux security/deployment rules suppress it.
3. Program sanitizes environment, FDs, and inputs.
4. Program performs the narrow privileged operation.
5. Program drops privilege and verifies the drop.
6. Program continues or execs target code with low privilege.
```

Production rule:

- Privilege is a temporary tool, not a process lifestyle.

### Flow 4 - Daemon Lifecycle

```text
1. Process starts from shell, boot script, or service manager.
2. It either daemonizes or stays foreground by contract.
3. It sets working directory, umask, descriptors, logging, and signal handlers.
4. It writes PID/ready notification only if deployment expects it.
5. It serves requests/events.
6. It handles SIGTERM for shutdown.
7. It may handle SIGHUP for config reload or log reopen.
```

Embedded notes:

- Avoid daemonizing behind a watchdog that expects a stable foreground process.
- Make shutdown idempotent; power loss and watchdog restarts happen.
- Do not keep device nodes, update files, or mount points open accidentally.

### Flow 5 - Scheduling And Limit Failure

```text
server accepts connections
    |
    +--> each connection consumes an FD
    |
    v
RLIMIT_NOFILE reached
    |
    +--> accept/open/socket/pipe returns EMFILE
    |
    v
inspect limits, FD leak, capacity model, service-manager config
```

For CPU latency:

```text
latency spike
    |
    +--> inspect policy/nice/affinity
    +--> inspect runnable load and CPU usage
    +--> check realtime tasks before changing priority
```

### Flow 6 - Process Accounting After Exit

```text
process runs and maybe execs another program
    |
    | exits or dies by signal
    v
kernel writes accounting record if accounting is enabled
    |
    +--> admin/debugger later reads with lastcomm/sa/custom parser
```

Use it for postmortem audit, not for live supervision. The parent still needs `wait*()` to reap child status.

## Production Bugs And Debugging

Advanced process bugs often look like "Linux is ignoring me": signals hit the wrong process, service restarts behave oddly, permission checks surprise you, or limits fail only under load.

| Symptom | Likely cause | Evidence | Fix pattern |
|---|---|---|---|
| `Ctrl-C` kills shell or only one pipeline process | Wrong PGID/foreground group | `ps -o pid,ppid,pgid,sid,tpgid,stat,cmd` | Put whole job in one PGID; manage `tcsetpgrp()`. |
| Background process stops unexpectedly | Terminal read triggers `SIGTTIN`, or terminal-control/write behavior triggers `SIGTTOU` when configured | `STAT` shows `T`; shell says stopped | Keep background jobs off terminal, foreground them, or review `TOSTOP`/terminal-control behavior. |
| Daemon dies after logout | Still tied to controlling terminal/session | `ps -o sid,tty,stat,cmd`; `SIGHUP` logs | Use service manager or correct `setsid()`/descriptor handling. |
| Service writes to dead stdout/stderr | Inherited terminal descriptors | `/proc/<PID>/fd` points to tty/pipe | Redirect or let service manager own logs. |
| Privileged helper opens wrong file | Trusted `PATH`, cwd, env, or inherited FD | `strace -f -e execve,openat`; inspect env | Use absolute paths, sanitized env, close-on-exec, privilege drop. |
| Permission denied despite login user | Effective UID/GID or groups differ | `/proc/<PID>/status`, `id`, `getfacl` | Inspect real/effective/supplementary IDs. |
| CPU-heavy task hurts system responsiveness | Bad nice/policy/affinity | `ps -o pid,ni,pri,rtprio,policy,psr,pcpu,cmd` | Lower priority, adjust affinity, fix workload. |
| System nearly locks up | Runaway realtime task | `chrt -p <PID>`, top shows RT task | Add watchdog, `RLIMIT_RTTIME`, safe priority, blocking points. |
| Server cannot accept more clients | `RLIMIT_NOFILE` or FD leak | `cat /proc/<PID>/limits`, `ls /proc/<PID>/fd \| wc -l` | Raise limit deliberately; fix leaks; reserve emergency FD. |
| No core after crash | `RLIMIT_CORE=0` or core pattern policy | `ulimit -c`, `/proc/<PID>/limits`, `/proc/sys/kernel/core_pattern` | Enable core dumps safely for debug environment. |
| `fork()` fails under one user only | `RLIMIT_NPROC` or cgroup process limit | errno `EAGAIN`, limits, service manager config | Tune limit and process model. |
| Detached job gets `SIGHUP`/`SIGCONT` unexpectedly | Process group became orphaned while stopped | `ps -o pid,ppid,pgid,sid,tpgid,tty,stat,cmd`; service/session logs | Design session/job ownership deliberately; use service manager for long-lived jobs. |
| Terminal I/O fails with `EIO` after detaching | Orphaned process group terminal stop behavior | `strace -e read,write,ioctl`; inspect PGID/SID/TTY | Remove terminal dependency or reconnect stdio/logging. |
| Need audit for a short-lived process already gone | Normal logs/supervisor missed it; process accounting may help if enabled | `lastcomm`, `sa`, accounting file, `/proc/sys/kernel/acct` | Enable accounting deliberately with storage limits and privacy controls. |

Practical commands:

```bash
ps -o pid,ppid,pgid,sid,tpgid,tty,stat,ni,pri,rtprio,policy,cmd -p <PID>
pstree -ap
cat /proc/<PID>/status
cat /proc/<PID>/limits
ls -l /proc/<PID>/fd
lsof -p <PID>
strace -f -e trace=process,signal,setuid,setgid,setpgid,setsid,execve ./program
id
getfacl <path>
renice -n 10 -p <PID>
chrt -p <PID>
taskset -pc <PID>
perf stat -p <PID>
journalctl -u <service>
ulimit -a
cat /proc/sys/kernel/acct
lastcomm
sa
```

## Work Checklist

Use this checklist when designing shells, supervisors, daemons, privileged helpers, and embedded services.

Process groups and terminals:

- [ ] Separate parent-child responsibility from process-group/session responsibility.
- [ ] Put all processes in one pipeline/job into the intended PGID.
- [ ] Set process groups before `exec()` when building shell-like behavior.
- [ ] Use terminal foreground control only from the session that owns the terminal.
- [ ] Send signals to a process group when the job, not one PID, is the target.
- [ ] Account for orphaned process-group behavior when detaching jobs or exiting a shell-like parent.
- [ ] Remove terminal dependencies from services and daemons; redirect or delegate stdio/logging deliberately.

Credentials and privilege:

- [ ] Know real, effective, saved, and supplementary group IDs.
- [ ] Sanitize environment, cwd, and inherited FDs before privileged execution.
- [ ] Drop supplementary groups, GID, then UID when moving to a service user.
- [ ] Verify privilege cannot be regained unless intentionally required.
- [ ] Keep credential details cross-checked with [ch01_users_and_groups.md](ch01_users_and_groups.md).

Daemons:

- [ ] Decide foreground service-manager mode vs classic daemonization.
- [ ] Set `umask`, working directory, descriptors, and logging deliberately.
- [ ] Handle `SIGTERM` for clean shutdown.
- [ ] Use `SIGHUP` reload/reopen only if documented and safe.
- [ ] Avoid holding update files, device nodes, or mount points open accidentally.

Scheduling and limits:

- [ ] Use nice values for normal CPU preference before realtime policies.
- [ ] Treat `SCHED_FIFO`/`SCHED_RR` as privileged, risky tools.
- [ ] Add runaway protection for realtime work.
- [ ] Inspect `/proc/<PID>/limits` before blaming code for resource failures.
- [ ] Budget `RLIMIT_NOFILE`, `RLIMIT_NPROC`, stack, memory, and core dumps.
- [ ] Remember cgroups/systemd limits can be stricter than classic rlimits.
- [ ] Use process accounting only when the audit value justifies storage, privacy, and kernel-config cost.
- [ ] For embedded targets, bound accounting/core/log retention before enabling postmortem data collection.

## Recognize / Advanced

These are important in real systems, but keep them behind the main model.

| Topic | Recognition point |
|---|---|
| Controlling terminal acquisition | Session leaders can acquire a controlling terminal under rules; daemons avoid this. |
| Double fork | Prevents the final daemon from being a session leader, reducing chance of reacquiring a terminal. |
| `SIGHUP` convention | Terminal hangup by origin; reload/reopen by daemon convention. |
| Linux capabilities | Split root privileges into capability bits; important beyond basic UID/GID. |
| Namespaces and cgroups | Container/service isolation layers that interact with process IDs, credentials, and limits. |
| `PR_SET_CHILD_SUBREAPER` | Lets a supervisor adopt orphaned descendants. |
| `SCHED_RESET_ON_FORK` | Prevents privileged scheduling policy/nice inheritance into children. |
| `getrusage(RUSAGE_CHILDREN)` | Aggregates waited-for child usage; unwaited children can change what you observe. |
| Process accounting v3 | Linux accounting record format has version/config differences; verify target kernel and tools. |
| Core dump security | Core files may expose secrets; enable deliberately and protect storage. |

## Final Coverage Check

- Rows 3.7, 3.8, 3.9, 3.10, and 3.11 are covered in this file.
- Must-cover items covered here: process groups, sessions, job control, controlling terminals, foreground/background signal behavior, orphaned process groups, credentials, privilege drop, set-ID behavior, daemons, service-manager lifecycle, `syslog()`, scheduling, nice values, realtime policies, context switches, CPU affinity, `getrusage()`, resource limits, process accounting, core dumps, and production debugging with `/proc`, `ps`, `strace`, `perf`-style evidence, and service logs.
- Must-cover items explicitly moved: core process model and basic `fork()`/termination are in `ch03_process_core.md`; `exec*()`, wait/reap, close-on-exec, subreapers/PID namespaces, and fork/exec attribute rules are in `ch03_process_execution.md`; credential fundamentals are cross-covered in `ch01_users_and_groups.md`.
- No known blockers remain in this file.

## Interview Readiness

A strong answer connects a visible system behavior to the process attribute that controls it.

You should be able to explain:

- Parent-child trees and process groups solve different problems.
- A terminal sends `Ctrl-C` to the foreground process group.
- Orphaned process groups can receive `SIGHUP`/`SIGCONT` and have special terminal-stop behavior.
- `setsid()` creates a new session and detaches from the old session context.
- Effective credentials usually decide permission checks; real credentials explain origin/accounting.
- A safe privileged helper narrows privilege, sanitizes inputs, and drops privilege.
- A daemon either follows classic detach rules or cooperates with a service manager in the foreground.
- Nice values affect normal scheduling; realtime policies can starve normal tasks.
- Resource limits are inherited and explain many production failures.
- Process accounting is post-exit audit evidence, not live supervision.

Good interview flow:

```text
visible symptom -> process attribute -> API/control point -> debugging evidence -> safe fix
```

Scenario prompts to practice:

- Why does `Ctrl-C` kill every process in `cat file | grep x`?
- Why does a background job stop when it reads from the terminal?
- Why can a daemon die when the SSH session closes?
- How would you safely run one privileged operation and then drop root?
- A server gets `EMFILE`. What do you inspect and change?
- A realtime process makes the board unresponsive. How do you prevent and debug it?
