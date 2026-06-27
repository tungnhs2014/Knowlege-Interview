# Chapter 3 - Process Execution

> Topics: 3.4 Monitoring Child Processes · 3.5 Program Execution · 3.6 Process Creation in Detail
> Main sources: TLPI Ch26, Ch27, Ch28 · DevLinux Module 03 exercises
> Production context: shells, pipelines, supervisors, test runners, process-based servers, embedded helpers

## Learning Goal

Understand the complete `fork()` -> setup -> `execve()` -> run -> exit -> `waitpid()` lifecycle. After this file, you should be able to run a helper safely, reap children without zombies, explain orphan reparenting, prevent close-on-exec descriptor leaks, and recognize when `clone()`, `vfork()`, or `posix_spawn()` matters.

- Read [ch03_process_core.md](ch03_process_core.md) first for process ownership, `fork()`, CoW, FD sharing, `exit()`, and `_exit()`.
- Read [ch03_process_advanced.md](ch03_process_advanced.md) next for process groups, credentials, daemons, scheduling, and limits.

## Coverage Notes

This file covers learning-map rows **3.4 Monitoring Child Processes**, **3.5 Program Execution**, and **3.6 Process Creation in Detail**.

Covered here:

- The production lifecycle `fork()` -> child setup -> `execve()` -> run -> exit/signal death -> `wait*()` reap.
- `wait()`, `waitpid()`, `waitid()`, `wait3()`, `wait4()`, wait-status macros, zombies, orphans, `SIGCHLD`, and nonblocking reaper design.
- `execve()` and the `exec*()` wrappers, PATH/environment risk, interpreter scripts, dynamic linker recognition, `system()` risks, and safe replacement with explicit `argv[]`/`envp[]`.
- FD inheritance, close-on-exec, shell redirection, pipeline close discipline, and child error reporting before `exec()`.
- `clone()`, `vfork()`, `posix_spawn()`, fork/exec attribute inheritance/reset rules, subreapers, and PID namespace supervision concerns.

Moved or intentionally scoped elsewhere:

- Process fundamentals, `fork()` basics, CoW, FD sharing, globals-not-IPC, `exit()`, and `_exit()` basics are in [ch03_process_core.md](ch03_process_core.md).
- Process groups, credentials, daemon/service-manager behavior, scheduling, limits, process accounting, and core-dump operations are in [ch03_process_advanced.md](ch03_process_advanced.md).
- `pidfd` APIs are out of scope for the mapped TLPI rows in this chapter; recognize them as modern Linux process-handle APIs for later study.

## Problem It Solves

Creating a child process is only useful if the parent can control what the child runs and learn how it ended. That is why Unix/Linux separates process creation, program replacement, termination, and status collection.

- `fork()` creates a process.
- Child setup changes FDs, environment, process group, or credentials.
- `execve()` replaces the child with a new program image.
- `exit()` or signal termination ends the child.
- `waitpid()` lets the parent collect status and remove the zombie entry.

```text
parent
  |
  | fork()
  v
child setup
  |
  | execve()
  v
new program runs
  |
  | exit or signal death
  v
zombie status
  |
  | parent waitpid()
  v
fully reaped
```

## Mental Model

`exec()` does not create a new process. It changes what the current process is running.

| Operation | What it does | PID changes? |
|---|---|---|
| `fork()` | Creates a child process | Child gets a new PID |
| child setup | Adjusts inherited state before replacement | No |
| `execve()` | Replaces process image with another program | No |
| `exit()` / signal | Terminates process | No; PID becomes zombie status until reaped |
| `waitpid()` | Parent collects child state | Removes zombie entry |

Why shells need `fork()` before `exec()`:

```text
shell PID 100
    |
    | fork()
    +--> parent: remains shell, later waits
    |
    +--> child: redirects FDs, execs /bin/ls
```

If the shell directly called `execvp("ls", ...)`, the shell would become `ls` and no prompt would return.

## Mechanism

The process execution lifecycle connects four kernel/libc mechanisms.

| Mechanism | Owner of state | Key idea |
|---|---|---|
| Child status | Kernel, visible to parent | Dead children remain as zombies until waited for. |
| Program image | Current process | `execve()` discards old user-space image and loads a new one. |
| Descriptor inheritance | Process FD table | FDs survive `exec()` unless marked close-on-exec. |
| Creation variants | Kernel task creation path | `fork()`, `vfork()`, and Linux `clone()` share ancestry but expose different sharing rules. |

### Waiting And Zombies

When a child exits, the kernel releases most resources but keeps a small record: PID, termination status, and accounting data. That record is a **zombie** until the parent waits.

```text
child running
    |
    | exit(7)
    v
zombie: PID + status + accounting
    |
    | parent waitpid(child, &status, 0)
    v
removed from process table
```

- Zombies are not running and do not consume normal memory/CPU.
- Many zombies still consume process table slots and signal broken parent logic.
- A parent that ignores children must still arrange reaping through `wait*()`, `SIGCHLD` handling, or service design.

### Orphans And Reparenting

An **orphan** is a live process whose original parent exited. Linux reparents it to `init`/systemd or a configured subreaper.

```text
parent exits first
    |
    v
child keeps running
    |
    v
new parent becomes init/systemd/subreaper
```

- Orphans are not automatically bugs.
- Daemons and detached helpers often become reparented intentionally.
- The new parent is responsible for eventual reaping.
- In a container or PID namespace, "PID 1" means the namespace's init process, not necessarily the host's global init.
- A supervisor can become a **subreaper** so orphaned descendants are reparented to it instead of escaping to the namespace init.

Subreaper design matters when the process tree has grandchildren:

```text
supervisor
  +-- child helper
        +-- grandchild worker

child helper exits
    |
    +--> without subreaper: grandchild adopted by namespace init/PID 1
    +--> with subreaper : grandchild adopted by supervisor
```

Production rule:

- If you supervise a tree, decide who owns descendant cleanup. Waiting only for direct children is not enough when helpers fork their own workers.
- In containers, a weak PID 1 implementation can leak zombies because PID 1 must reap orphaned descendants.

### `execve()` Replacement

`execve(path, argv, envp)` loads a new program into the current process. On success, it never returns to the old code.

What `execve()` replaces:

- Text, data, BSS, heap, stack, and most user mappings.
- Old `atexit()` handlers.
- Caught signal handlers, reset to default.
- The old program's argument and environment view.

What usually survives:

- PID and PPID.
- Process group and session.
- Current working directory and root directory.
- Umask.
- Open FDs unless close-on-exec is set.
- Signal mask.
- Resource limits, nice value, and many scheduling attributes.

### Fork/Exec Attribute Rules

The hard part of process execution is not memorizing every attribute. The useful habit is to ask: **is this state copied, shared, preserved, reset, or dropped?**

| Attribute family | After `fork()` | After successful `execve()` | Why it matters |
|---|---|---|---|
| PID | Child gets a new PID | Same PID remains | Logs and supervisors track one process through exec. |
| PPID | Child parent is the caller, unless later reparented | Usually preserved | Parent death changes PPID through orphan reparenting. |
| Process group / session | Inherited | Preserved | Signals hit the wrong job if PGID setup races. |
| Address space | Private copy using CoW for normal mappings | Replaced by new program image | Globals are not IPC; exec discards old heap/stack. |
| Memory mappings | Usually inherited, with exceptions such as `MADV_DONTFORK` | Old mappings are replaced; shared memory attachments are detached | Child may briefly see parent mapping state before exec. |
| Open FDs | FD table copied; open file descriptions shared | Preserved unless `FD_CLOEXEC` | Redirection works; leaked FDs cause hangs and security bugs. |
| File offset/status flags | Shared through open file descriptions | Preserved on surviving FDs | Parent/child can affect each other's offsets and `O_NONBLOCK`. |
| Cwd/root/umask | Inherited | Preserved | Helpers can open relative paths in surprising places. |
| Environment | Copied | Replaced only by explicit `envp`; otherwise inherited by wrappers | PATH/env injection risk. |
| Signal dispositions | Inherited | Caught handlers reset; ignored/default usually remain | New program must not inherit unexpected handlers. |
| Signal mask | Inherited | Preserved | Execing arbitrary programs with blocked signals can surprise them. |
| Pending signals | Not inherited | Preserved | Rare, but matters in signal-heavy launchers. |
| Alternate signal stack | Inherited | Reset | Signal-handler assumptions do not survive exec. |
| Threads | Only the calling thread exists in child | Old threads gone | Child-after-fork in multithreaded code must be tiny. |
| Exit handlers | Inherited | Removed | Child `exit()` can run parent cleanup unless `_exit()` is used. |
| Credentials/capabilities | Inherited | May change for set-user-ID, set-group-ID, or file capabilities | Privileged exec must sanitize env and FDs. |
| Resource limits / nice / many scheduling attrs | Inherited | Preserved | Limits and priorities follow helpers unless changed. |
| Resource usage counters | Child starts fresh for many counters | Process accounting follows the process through exec | Metrics attribution can be misread. |
| Timers / locks / IPC attachments | Mixed | Mixed; many are dropped or detached | Check the manual when correctness depends on them. |

Interview shortcut:

```text
fork(): duplicate process-style ownership, sharing some referenced kernel objects
exec(): replace user-space image, preserve process identity and selected kernel attributes
```

## Key APIs And Objects

Prefer APIs that make ownership and error paths explicit.

### Monitoring Children

| API/object | Role | Notes |
|---|---|---|
| `wait(&status)` | Wait for any child to terminate | Simple but imprecise for many children. |
| `waitpid(pid, &status, options)` | Wait for selected child/process group | Main workhorse for supervisors. |
| `waitid()` | More precise child state API | Useful when you need `siginfo_t` detail. |
| `wait3()` / `wait4()` | Wait plus resource usage | Nonportable/BSD-origin; useful to recognize. |
| `SIGCHLD` | Signal on child state change | Notification only; parent still calls `waitpid()`. |
| Wait status macros | Decode status safely | Never decode raw bits manually. |

Wait status macros:

| Macro | Meaning |
|---|---|
| `WIFEXITED(status)` | Child exited normally. |
| `WEXITSTATUS(status)` | Low 8 bits of normal exit code. |
| `WIFSIGNALED(status)` | Child was killed by a signal. |
| `WTERMSIG(status)` | Signal that killed the child. |
| `WCOREDUMP(status)` | Core was produced, where available. |
| `WIFSTOPPED(status)` | Child stopped. |
| `WSTOPSIG(status)` | Signal that stopped the child. |
| `WIFCONTINUED(status)` | Child resumed after stop. |

Common `waitpid()` targets and options:

| Expression | Meaning |
|---|---|
| `waitpid(child_pid, &st, 0)` | Wait for one known child. |
| `waitpid(-1, &st, 0)` | Wait for any child. |
| `waitpid(0, &st, 0)` | Wait for any child in caller's process group. |
| `waitpid(-pgid, &st, 0)` | Wait for any child in process group `pgid`. |
| `WNOHANG` | Return immediately if no child is waitable. |
| `WUNTRACED` | Report stopped children. |
| `WCONTINUED` | Report continued children. |

### Program Execution

There is no single function named `exec()`. It is a family of wrappers around `execve()`.

| Function | Program lookup | Arguments | Environment |
|---|---|---|---|
| `execve()` | Exact pathname | `argv[]` | explicit `envp[]` |
| `execv()` | Exact pathname | `argv[]` | inherit |
| `execvp()` | `PATH` search if no slash | `argv[]` | inherit |
| `execl()` | Exact pathname | variadic list | inherit |
| `execlp()` | `PATH` search if no slash | variadic list | inherit |
| `execle()` | Exact pathname | variadic list | explicit environment |

Practical choices:

- Use `execve()` when path and environment must be controlled exactly.
- Use `execvp()` for shell-like command lookup in non-privileged tools.
- Avoid `execl*()` for dynamic argument lists; arrays are safer.
- Remember: successful `exec*()` never returns.

### Descriptor Control

FD inheritance is powerful for redirection and pipelines, but dangerous for long-running services.

| API/flag | Role |
|---|---|
| `dup2(oldfd, newfd)` | Redirection: make `newfd` refer to `oldfd`. |
| `pipe()` / `pipe2()` | Create pipe endpoints for parent-child communication. |
| `close(fd)` | Remove one process's descriptor. |
| `fcntl(fd, F_SETFD, FD_CLOEXEC)` | Close descriptor on successful `exec()`. |
| `O_CLOEXEC` | Create FD with close-on-exec atomically. |
| `pipe2(O_CLOEXEC)` | Create pipe endpoints already close-on-exec. |
| `dup3(..., O_CLOEXEC)` | Duplicate FD with close-on-exec atomically. |
| `accept4(..., SOCK_CLOEXEC)` | Accept a socket with close-on-exec already set. |

Production rule:

- Prefer atomic close-on-exec creation (`O_CLOEXEC`, `pipe2`, `dup3`, `accept4`) when available.
- In multi-threaded programs, setting `FD_CLOEXEC` after creation can race with another thread doing `fork()`/`exec()`.

### Creation Variants

| API | What to know | Use carefully because |
|---|---|---|
| `fork()` | General-purpose process creation | Parent/child scheduling order is not guaranteed. |
| `vfork()` | Child shares address space until `exec()` or `_exit()` | Very restricted; avoid unless measured need. |
| `clone()` | Linux-specific low-level task creation with sharing flags | Used for threads/namespaces; direct app use is rare. |
| `posix_spawn()` | Spawn abstraction with file actions and attributes | Often useful in constrained or multi-threaded programs. |
| `system()` | Runs `/bin/sh -c command` | Convenient but risky with untrusted strings and implicit shell behavior. |
| `prctl(PR_SET_CHILD_SUBREAPER)` | Make this process adopt orphaned descendants | Useful for service supervisors and containers; Linux-specific. |
| `prctl(PR_GET_CHILD_SUBREAPER)` | Inspect whether this process is a subreaper | Useful when debugging supervision chains. |
| `/proc/<PID>/status` `NSpid` | Show nested PID namespace values when available | Helps match host and container observations. |

## Lifecycle / Data Flow

The safest way to understand process execution is to follow ownership transfer step by step.

### Flow 1 - Shell-Style Command

```text
1. Parent builds argv: ["ls", "-l", "/tmp", NULL].
2. Parent calls fork().
3. Child adjusts FDs/environment if needed.
4. Child calls execvp("ls", argv).
5. On success, child is now /bin/ls with same PID.
6. Parent calls waitpid(child_pid, &status, 0).
7. Parent decodes status with WIF* macros.
```

Safe child skeleton:

```c
pid_t pid = fork();

if (pid == -1) {
    /* parent: handle fork failure */
} else if (pid == 0) {
    execvp(argv[0], argv);
    _exit(127); /* exec failed */
} else {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        /* handle wait failure */
    }
}
```

Why `_exit(127)` is common:

- `127` conventionally means command could not be executed/found in shell-like code.
- `_exit()` avoids inherited stdio flush and parent cleanup handlers.
- A real program may report `errno` through a pipe before `_exit()`.

### Flow 2 - Redirection

For `cmd >out.txt`, the child changes `stdout` before `exec()`.

```text
child:
    fd = open("out.txt", O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0644)
    dup2(fd, STDOUT_FILENO)
    close(fd)
    execvp("cmd", argv)
```

After `dup2()`, file descriptor `1` points to the file. The new program simply writes to stdout; it does not need to know about the shell's redirection.

### Flow 3 - Pipeline

For `producer | consumer`, two children share a pipe through inherited descriptors.

```text
parent pipe()
    |
    +--> producer child:
    |       dup2(pipe_write, STDOUT_FILENO)
    |       close both original pipe fds
    |       exec producer
    |
    +--> consumer child:
    |       dup2(pipe_read, STDIN_FILENO)
    |       close both original pipe fds
    |       exec consumer
    |
    +--> parent:
            close both pipe fds
            wait for children
```

Critical rule:

- Every process closes the pipe ends it does not use.
- If a write end stays open in any process, the reader may wait forever for EOF.

### Flow 4 - Nonblocking Reaper

Long-running parents must reap children without blocking the main loop forever.

```text
on SIGCHLD or periodic tick:
    while waitpid(-1, &status, WNOHANG) > 0:
        record child result
```

Rules:

- `SIGCHLD` means "check children"; it does not carry all child statuses by itself.
- Loop until no more waitable children; signals can coalesce.
- Handle `EINTR` in blocking waits.
- Keep signal handlers minimal; many programs use a self-pipe, eventfd, signalfd, or main-loop wakeup.

### Flow 5 - `clone()` Attribute Sharing

`clone()` lets Linux create a task while choosing which attributes to share.

| Flag idea | Effect |
|---|---|
| `CLONE_VM` | Share virtual memory. |
| `CLONE_FILES` | Share FD table. |
| `CLONE_FS` | Share current directory, root directory, and umask. |
| `CLONE_SIGHAND` | Share signal dispositions; requires related sharing constraints on modern Linux. |
| `CLONE_THREAD` | Place task in same thread group. |

Recognition rule:

```text
fork()  = separate process-style ownership
clone() = selectable sharing, foundation for threads and namespaces
```

Application code usually uses `pthread_create()`, `fork()`, or `posix_spawn()` instead of raw `clone()`.

### Flow 6 - Attribute Audit Before Launch

Before a service launches a helper, audit what the helper should inherit.

```text
parent process
    |
    +--> choose argv[] and envp[]
    +--> choose cwd/root/umask if relevant
    +--> choose FDs to keep; mark the rest close-on-exec
    +--> choose signal mask/dispositions expected by child
    +--> choose credentials/process group if relevant
    |
    v
fork or posix_spawn
    |
    v
execve target
```

Rule of thumb:

- In a shell, inheritance is a feature.
- In a privileged service, inheritance is a liability until intentionally allowed.

## Production Bugs And Debugging

Execution bugs usually leave very visible fingerprints: zombies, stuck pipes, leaked sockets, wrong exit statuses, or unexpected shell behavior.

| Symptom | Likely cause | Evidence | Fix pattern |
|---|---|---|---|
| `<defunct>` processes accumulate | Parent does not reap children | `ps -o pid,ppid,stat,cmd`, `pstree -p` | Add `waitpid()` loop or delegate to supervisor/subreaper. |
| Parent blocks forever waiting | Waiting for wrong PID/group or child stuck | `strace -p <parent>`, `ps -o stat,wchan` | Track child PIDs; add timeout/cancel path. |
| Pipeline never exits | Extra pipe write end inherited | `lsof -p <PID>`, `/proc/<PID>/fd` | Close unused pipe ends; use `O_CLOEXEC`. |
| Child runs parent code after `exec()` failure | Missing `_exit()` after failed exec | Logs show duplicate parent behavior | Child branch must terminate immediately on exec failure. |
| Secret socket visible in helper | Missing close-on-exec | `ls -l /proc/<PID>/fd` after exec | Set `O_CLOEXEC` when creating descriptors. |
| Wrong command executed | `PATH` search used unexpectedly | `strace -f -e execve` shows path | Use absolute path and controlled `envp`. |
| Shell injection | `system()` with untrusted string | Command contains user-controlled metacharacters | Use `fork()` + `execve()` with argv array. |
| Exit code looks huge or wrong | Raw wait status treated as exit code | Logs show status like `256` for exit 1 | Decode with `WIFEXITED()` and `WEXITSTATUS()`. |
| Supervisor misses child events | Handles only one child per `SIGCHLD` | Multiple exits under load | Loop `waitpid(-1, ..., WNOHANG)`. |
| Grandchildren keep running after helper exits | Supervisor only owns direct child, no subreaper/tree cleanup | `pstree -ap`, `ps -o pid,ppid,stat,cmd` | Use a service manager, subreaper, process group kill, or explicit shutdown protocol. |
| Helper starts with blocked signals | Parent execed arbitrary program with inherited signal mask | `/proc/<PID>/status` `SigBlk`; `strace -f -e signal` | Reset masks/dispositions before `exec()` when launching unknown code. |
| `exec()` target behaves differently under service | Inherited cwd, env, umask, limits, or FDs differ from shell | Compare `/proc/<PID>/{cwd,environ,limits,fd}` | Build explicit launch contract instead of relying on ambient process state. |

Practical commands:

```bash
ps -eo pid,ppid,pgid,sid,stat,wchan,cmd | grep '<defunct>'
pstree -ap
strace -f -e trace=process,execve,wait4,desc ./program
ls -l /proc/<PID>/fd
cat /proc/<PID>/status
grep '^NSpid:' /proc/<PID>/status
cat /proc/<PID>/limits
readlink /proc/<PID>/ns/pid
lsof -p <PID>
gdb -p <PID>
```

Debugging `exec()` failures:

```text
child reports errno through pipe
    |
    +--> parent reads pipe before/while waiting
    +--> if pipe closes without data, exec likely succeeded
```

This pattern avoids confusing "child exited 127" with no detail.

Embedded debugging notes:

- Prefer `posix_spawn()` or a small helper process when memory pressure makes `fork()` expensive.
- Keep inherited descriptors under control; leaked device FDs can block firmware update, suspend, or hotplug flows.
- Avoid `system()` in privileged embedded services; shell expansion plus environment surprises are hard to audit.
- For watchdog-managed helpers, make exit status and signal death visible in logs.
- For containerized embedded services, confirm who is PID 1 and who reaps orphaned descendants.

## Work Checklist

Use this checklist when launching or supervising child processes.

- [ ] Build `argv[]` as data; avoid shell strings unless shell behavior is required.
- [ ] Use absolute paths or a controlled `PATH` for privileged/service code.
- [ ] Decide exactly which FDs the child should inherit.
- [ ] Set close-on-exec atomically for all unrelated FDs.
- [ ] In the child, perform only required setup before `exec()`.
- [ ] On failed `exec()`, report error if needed and call `_exit()`.
- [ ] Parent stores child PID and waits for the correct child or group.
- [ ] Decode wait status with `WIF*` macros.
- [ ] Reap all waitable children in long-running parents.
- [ ] Decide whether grandchildren are allowed; if not, use a subreaper, process group, service manager, or explicit protocol.
- [ ] Close unused pipe ends in parent and every child.
- [ ] Treat `SIGCHLD` as a notification, not as the reaping operation itself.
- [ ] Reset or intentionally preserve signal masks/dispositions before `exec()` of arbitrary programs.
- [ ] Audit cwd, root, umask, env, credentials, limits, scheduling attributes, and FDs before privileged launches.
- [ ] Avoid raw `clone()` unless you are deliberately building low-level Linux runtime behavior.

## Recognize / Advanced

These topics often appear in production reviews and interviews, even when day-to-day code uses simpler APIs.

| Topic | Recognition point |
|---|---|
| `SA_NOCLDWAIT` | Can prevent zombies for children, but changes wait behavior; use deliberately. |
| Ignoring `SIGCHLD` | On Linux/POSIX systems this can affect zombie creation; do not cargo-cult it. |
| Subreaper | A process can adopt orphaned descendants, useful for supervisors and containers. |
| `execve()` scripts | Kernel handles `#!` interpreter scripts by executing the interpreter with script path. |
| Dynamic linker | ELF execution may load the runtime linker before user `main()`. |
| `fexecve()` / `execveat()` | Execute by FD or with advanced path rules; useful in specialized secure launchers. |
| `posix_spawn()` file actions | Express redirection/close actions without manual child code. |
| Process accounting | Records terminated process data; useful for audit, not for live control. |
| PID namespace | PID values and PID 1 responsibilities are namespace-relative; debug both host and namespace views. |
| `pidfd` APIs | Modern Linux process handles that reduce PID-reuse races; out of scope for the mapped TLPI rows here. |

## Final Coverage Check

- Rows 3.4, 3.5, and 3.6 are covered in this file.
- Must-cover items covered here: fork/setup/exec/exit/wait lifecycle, parent/child supervision, error reporting, FD hygiene, close-on-exec, `exec*()` variants, PATH/env risk, scripts, `system()`, wait APIs, status macros, zombies, orphans, subreapers, PID namespaces, and fork/exec attribute inheritance/reset.
- Must-cover items explicitly moved: process fundamentals and termination basics are in `ch03_process_core.md`; process groups, credentials, daemons, scheduling, resource limits, process accounting, core dumps, and service logs are in `ch03_process_advanced.md`.
- `pidfd` APIs remain out of scope for the mapped TLPI Chapter 03 rows and are recognize-only for interview awareness.

## Interview Readiness

A strong answer explains the lifecycle and then uses APIs as evidence.

You should be able to explain:

- `fork()` creates a process; `execve()` replaces the current process image.
- Successful `execve()` never returns and keeps the same PID.
- Open FDs survive `exec()` unless `FD_CLOEXEC` is set.
- A zombie is a dead child with uncollected status; an orphan is a live process whose original parent died.
- `waitpid()` both obtains child status and reaps the zombie.
- `SIGCHLD` is notification; a parent still needs `waitpid()`.
- Subreapers and PID namespace PID 1 decide who adopts and reaps orphaned descendants.
- `fork()` copies or shares selected attributes; `exec()` replaces the user image while preserving selected process attributes.
- `system()` invokes a shell and is unsafe for untrusted input.

Good interview flow:

```text
lifecycle -> what survives exec -> child status model -> production bug -> debug command
```

Scenario prompts to practice:

- A service has many `<defunct>` children. What exactly is broken?
- A pipeline hangs after the writer exits. Which FDs do you inspect?
- A helper unexpectedly inherits a listening socket. How do you prevent it?
- Why is `execvp()` convenient but risky in privileged code?
- Why does a parent see status `256` when the child returned `1`?
