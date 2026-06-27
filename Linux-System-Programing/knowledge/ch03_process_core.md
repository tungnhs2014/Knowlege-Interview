# Chapter 3 - Process Core

> Topics: 3.1 Process Fundamentals · 3.2 Process Creation · 3.3 Process Termination
> Main sources: TLPI Ch06, Ch24, Ch25 · DevLinux Module 03
> Production context: shells, supervisors, embedded helper programs, worker lifecycle

## Learning Goal

Understand what a Linux process is, how `fork()` creates one, and how a process terminates cleanly. After this file, you should be able to explain program vs process, process ownership boundaries, copy-on-write, descriptor inheritance, and why child code usually exits with `_exit()` on failure paths before `exec()`.

- Use this file for the first half of the process lifecycle.
- Use [ch03_process_execution.md](ch03_process_execution.md) for `waitpid()`, zombies, `execve()`, and detailed fork/exec flow.
- Use [ch03_process_advanced.md](ch03_process_advanced.md) for process groups, credentials, daemons, scheduling, and limits.

## Coverage Notes

This file covers learning-map rows **3.1 Process Fundamentals**, **3.2 Process Creation**, and **3.3 Process Termination**.

Covered here:

- Program vs process, PID/PPID, process memory layout, command-line arguments, environment, and `/proc` evidence.
- Process as an ownership boundary for virtual memory, FD table, cwd/root, credentials, signals, scheduling state, resource limits, and termination status.
- `fork()` return paths, copy-on-write, parent/child scheduling nondeterminism, FD sharing, and why globals are not IPC.
- `exit()`, `_exit()`, `atexit()`, inherited stdio buffers, exit status width, and child error paths before `exec()`.
- Embedded constraints for memory pressure, strict overcommit, process count, FD count, and flash/log side effects.

Moved or intentionally scoped elsewhere:

- Child monitoring, `execve()`, zombies, subreapers, PID namespaces, close-on-exec, and fork/exec attribute tables are in [ch03_process_execution.md](ch03_process_execution.md).
- Process groups, credentials, daemon/service-manager behavior, scheduling, limits, process accounting, core dumps, and service-level debugging are in [ch03_process_advanced.md](ch03_process_advanced.md).
- Detailed credential mechanics also live in [ch01_users_and_groups.md](ch01_users_and_groups.md).

## Problem It Solves

Linux needs a runtime object that can own memory, file descriptors, credentials, signal state, CPU scheduling state, and termination status. That runtime object is the **process**.

- A program file can sit on disk without using CPU or RAM.
- A process is an active instance of a program plus kernel-managed state.
- `fork()` lets one process create another without losing the parent.
- `exit()` and `_exit()` define how a process gives resources back to the kernel.

```text
program on disk
    |
    | exec loads it into a runtime context
    v
process = program image + resources + kernel metadata + lifecycle
```

## Mental Model

Think of a process as an ownership boundary. Inside the boundary are the process's virtual memory, file descriptor table, current directory, credentials, signal settings, and scheduling/accounting state.

| Question | Process answer |
|---|---|
| Who owns this memory? | The process address space owns private virtual memory mappings. |
| Who can access this file? | Permission checks use the process credentials. |
| Where does `printf()` write? | Through file descriptor `1`, usually inherited from the parent. |
| Who created this task? | The parent process, visible through PPID until reparenting. |
| What happens when it dies? | Most resources are released; status waits for the parent to collect. |

Program vs process:

```text
/bin/bash file
    |
    +--> bash PID 1200
    +--> bash PID 2408
    +--> bash PID 3901
```

- **Program**: passive executable file and metadata.
- **Process**: active running instance with a PID and resources.
- One program may have many running processes.
- A PID belongs to a process instance, not to the program forever.

Process vs thread:

| Aspect | Process | Thread |
|---|---|---|
| Address space | Usually separate | Shared inside one process |
| File descriptor table | Usually separate copy | Shared by POSIX threads |
| Crash isolation | Stronger | Weaker |
| Sharing data | Requires IPC/shared memory | Direct shared memory |
| Best fit | Isolation, privilege split, helper processes | In-memory parallel work |

The useful intuition is not "process slow, thread fast". It is:

```text
process = stronger ownership boundary, higher coordination cost
thread  = easier sharing, higher shared-state risk
```

## Mechanism

The kernel represents each process with kernel metadata plus references to other kernel objects. User space sees handles such as PIDs and file descriptors; the kernel owns the real state.

Important ownership rules:

- The process owns a **virtual address space**: text, data, BSS, heap, mappings, stack, arguments, and environment.
- The process owns an **FD table**: small integers such as `0`, `1`, `2`, and descriptors returned by `open()`, `pipe()`, or `socket()`.
- FD table entries point to **open file descriptions** that hold file offset and file status flags.
- Credentials decide permission checks; Chapter 3 advanced links this to [users and groups](ch01_users_and_groups.md).
- Signal dispositions and many resource attributes belong to the process-level model; in threaded programs, signal masks and scheduling details also have per-thread rules.

Accounting note:

- In the core mental model, "accounting state" means kernel-maintained execution/resource state attached to a process lifetime.
- System-wide process accounting records and `acct()` administration are covered in [ch03_process_advanced.md](ch03_process_advanced.md), because they are operations/debugging topics rather than core creation mechanics.

Simplified process shape:

```text
Process
├── user virtual address space
│   ├── text, data, BSS
│   ├── heap and mmap() regions
│   ├── stack
│   └── argv[] and environment
└── kernel-side state
    ├── PID, PPID, process group, session
    ├── page tables
    ├── FD table -> open file descriptions
    ├── current/root directory and umask
    ├── credentials
    ├── signal state
    ├── scheduling state
    └── resource usage and limits
```

### `fork()` Mechanism

`fork()` creates a child process that starts as an almost duplicate of the caller. It returns twice: once in the parent and once in the child.

```text
pid = fork()
    |
    +--> parent: pid == child's PID
    |
    +--> child : pid == 0
```

What the child gets:

- A new PID.
- Same PPID as the parent's PID.
- A virtual memory layout that initially looks like the parent's.
- A copy of the parent's FD table.
- The same open file descriptions behind those copied FDs.
- Copied signal dispositions and many other process attributes.

What is not guaranteed:

- The parent is not guaranteed to run first.
- The child is not guaranteed to run first.
- On multiprocessor systems, both may run at nearly the same time.

### Copy-On-Write

Modern Linux does not eagerly copy all parent memory pages during `fork()`. It uses **copy-on-write (CoW)**.

```text
before write:
parent virtual page -> physical page A
child  virtual page -> physical page A

after child writes:
parent virtual page -> physical page A
child  virtual page -> physical page B
```

- Parent and child initially share physical pages read-only where possible.
- When one writes to a shared private page, the kernel copies that page.
- This makes `fork()` practical for the common `fork()` then `exec()` pattern.
- Embedded systems still care: large address spaces, strict overcommit policy, or memory pressure can make `fork()` fail.

### FD Sharing After `fork()`

After `fork()`, parent and child have separate FD table entries, but those entries often refer to the same open file description.

```text
parent fd 3 ----+
                +--> open file description: offset, status flags
child  fd 3 ----+
```

Consequences:

- If parent reads from `fd 3`, the shared file offset moves for the child too.
- If child sets file status flags such as `O_NONBLOCK`, parent may observe the effect.
- Closing `fd 3` in the child removes that child's descriptor, but not the parent's descriptor.
- Pipe/socket EOF may never arrive if any process keeps an unused write end open.

### Arguments And Environment

Every process starts with argument strings and an environment. They are ordinary process
memory, but they strongly affect program behavior and `exec()` decisions.

```text
main(argc, argv)
    |
    +--> argv[]: command-line words, ending with NULL
    +--> environ/envp: "NAME=value" strings, ending with NULL
```

Rules to keep straight:

- `argc` counts the entries in `argv`; `argv[0]` is usually the program name or path.
- The environment is inherited across `fork()` and normally passed across `exec()` unless the caller supplies a new `envp`.
- `getenv()`, `setenv()`, `putenv()`, and `clearenv()` change the current process environment; children created later inherit the changed environment.
- Environment inheritance is one-way: changing a child environment does not change the parent.
- Linux exposes startup strings through `/proc/<PID>/cmdline` and `/proc/<PID>/environ`; treat environment variables as visible to sufficiently privileged observers, not as secret storage.
- Very large argument/environment lists can make `execve()` fail with `E2BIG`; practical limits involve `ARG_MAX` and the process stack limit.

## Key APIs And Objects

Use these APIs through the mechanism above, not as a memorized list.

| API/object | Role | Production rule |
|---|---|---|
| `getpid()` | Return caller's PID | Useful for logs and `/proc/<PID>` inspection. |
| `getppid()` | Return parent PID | Can change after parent death due to reparenting. |
| `fork()` | Create child process | Check for `-1`; handle parent and child paths separately. |
| `exit(status)` | C library normal termination | Runs `atexit()` handlers and flushes stdio. |
| `_exit(status)` | Immediate process termination | Use in child error path after `fork()` when `exec()` fails. |
| `atexit(fn)` | Register normal-exit handler | Inherited across `fork()`, removed by successful `exec()`. |
| `on_exit()` | GNU extension exit handler | Nonportable; recognize but prefer `atexit()` for portable code. |
| `abort()` | Abnormal termination | Raises `SIGABRT`, may produce a core dump. |
| `argv`, `envp`, `environ` | Startup inputs | Inherited by children and replaced by `execve()` inputs. |
| `getenv()` / `setenv()` / `putenv()` / `clearenv()` | Inspect or modify current environment | Affects this process and future children, not the already-running parent. |
| `/proc/<PID>/cmdline` / `/proc/<PID>/environ` | Linux process argument/environment views | Useful for debugging; avoid storing secrets in environment. |
| `ARG_MAX` / `RLIMIT_STACK` | Argument/environment sizing context | Oversized `argv` + environment can make `execve()` fail with `E2BIG`. |
| File descriptor table | Per-process integer handles | Copied by `fork()`; FDs remain open until closed or `exec()` close-on-exec. |
| Open file description | Kernel open-file state | Shared by inherited or duplicated descriptors. |

Common memory regions:

| Region | Contains | Notes |
|---|---|---|
| Text | Executable instructions | Usually read-only and shareable. |
| Data | Initialized global/static data | Copied privately on write after `fork()`. |
| BSS | Zero-initialized global/static data | Starts as zeros. |
| Heap | Dynamic allocation | Managed by `malloc()` and friends. |
| Mappings | Shared libraries, files, anonymous mappings | Created by loader or `mmap()`. |
| Stack | Function calls and automatic variables | Grows and faults pages on demand. |
| Arguments/environment | Startup strings | Appear near the initial stack on Linux. |

## Lifecycle / Data Flow

The basic lifecycle starts before `exec()` and continues until the parent collects status. This file focuses on creation and termination; monitoring is covered in the execution file.

```text
parent process running
    |
    | fork()
    v
parent path                         child path
    |                                   |
    | child PID returned                | 0 returned
    |                                   |
    | may continue work                 | adjust state or exec later
    |                                   |
    +-------------------------------+---+
                                    |
                               exit/_exit
                                    |
                               status for parent
```

### Safe `fork()` Shape

Use explicit branches and keep child-side code narrow.

```c
pid_t pid = fork();

if (pid == -1) {
    /* parent only: handle EAGAIN/ENOMEM and report failure */
} else if (pid == 0) {
    /* child only: close/dup descriptors, then exec or _exit */
    _exit(127);
} else {
    /* parent only: record pid, continue, or wait later */
}
```

Rules:

- Never assume which branch runs first.
- In the child, avoid accidentally continuing parent control flow.
- In the child after `fork()` in a multi-threaded program, be extremely conservative until `exec()`; locks held by vanished threads can make library calls unsafe.
- Close descriptors that the child does not need.
- Use `_exit()` if the child cannot `exec()`.

### Termination Flow

`exit()` is a user-space cleanup path followed by kernel process termination. `_exit()` goes straight to kernel termination.

```text
return from main()
    |
    v
exit(status)
    |
    +--> call atexit/on_exit handlers
    +--> flush and close stdio streams
    +--> invoke kernel termination
    v
kernel releases resources and records wait status
```

Compare termination APIs:

| Case | Use | Why |
|---|---|---|
| Normal whole-program success/failure | `return` from `main()` or `exit()` | Runs normal C library cleanup. |
| Child after failed `exec()` | `_exit(127)` or another chosen code | Avoids duplicate stdio flush and inherited cleanup handlers. |
| Fatal internal invariant failure | `abort()` | Makes abnormal crash visible, may produce core. |
| Signal termination | `kill(pid, sig)` or kernel signal | Parent must decode wait status as signal death, not exit code. |

### Why `exit()` Can Be Wrong In A Child

If a parent has buffered stdio data before `fork()`, the child inherits a copy of that buffer. Calling `exit()` in both processes can flush duplicated data.

```text
parent printf("pending output")  (buffer not flushed)
parent fork()
    |
    +--> parent exit() flushes buffer
    +--> child  exit() flushes inherited buffer again
```

Child rule:

- If the child is going to `exec()`, prepare only what is needed.
- If `exec()` fails, report the error through a simple mechanism if needed.
- Then call `_exit()`.

## Production Bugs And Debugging

Most process-core bugs come from wrong ownership assumptions: assuming memory is shared, assuming descriptors are independent, assuming fork order, or using the wrong termination path.

| Symptom | Likely cause | Evidence | Fix pattern |
|---|---|---|---|
| Duplicate log line after spawning child | Child called `exit()` and flushed inherited stdio | `strace -f -e write,exit_group` shows both write | Flush before fork or use `_exit()` in child error path. |
| Parent and child file reads skip data | Shared open file description offset | `strace -f -e read,lseek` shows interleaved reads | Open separate file descriptions or synchronize. |
| Pipe never reaches EOF | Unused pipe write end inherited | `lsof -p <PID>` shows extra pipe ends | Close unused FDs in every process. |
| Worker start fails under load | `fork()` returns `EAGAIN` or `ENOMEM` | Logs, `strace -f -e fork,clone`, `/proc/<PID>/limits` | Check limits, memory policy, process count, retry strategy. |
| Rare race after `fork()` | Code assumes parent or child runs first | Reproduces under CPU load only | Add synchronization: pipe, signal, wait, lock, or protocol. |
| Child accidentally starts a second server loop | Failed `exec()` falls through | Logs show parent and child accepting work | Child branch must `_exit()` after failed `exec()`. |

Practical commands:

```bash
ps -o pid,ppid,pgid,sid,stat,comm -p <PID>
pstree -ap <PID>
cat /proc/<PID>/status
cat /proc/<PID>/maps
ls -l /proc/<PID>/fd
lsof -p <PID>
strace -f -e trace=process,desc ./program
gdb -p <PID>
```

Embedded debugging notes:

- Check whether the target uses strict memory overcommit; `fork()` may fail earlier than on a desktop.
- Watch process count and FD limits in long-running systems.
- Avoid large parent heaps before spawning frequent helpers when memory is tight.
- Prefer explicit helper protocols so parent and child do not both write to flash logs unexpectedly.

## Work Checklist

Use this checklist when writing or reviewing process creation and termination code.

- [ ] Distinguish program file, process instance, PID, and parent-child relationship.
- [ ] Check every `fork()` return value.
- [ ] Keep parent and child branches visibly separate.
- [ ] Do not depend on parent/child scheduling order.
- [ ] Close FDs the child does not need.
- [ ] Understand which inherited FDs share open file offsets and status flags.
- [ ] Flush or avoid stdio buffering before `fork()` when duplicate output matters.
- [ ] Use `_exit()` in child failure paths before `exec()`.
- [ ] Use `exit()` or `return` from `main()` for normal whole-program termination.
- [ ] Plan how the parent will collect child status; see the execution file.
- [ ] In embedded services, budget memory, PID count, and FDs before spawning helpers.

## Recognize / Advanced

These details are useful, but should not distract from the core lifecycle.

| Topic | What to know |
|---|---|
| `vfork()` | Child shares parent's memory and parent is suspended until child `exec()`s or `_exit()`s; avoid unless you truly need it. |
| `posix_spawn()` | Often used as a safer/faster spawn abstraction in constrained or multi-threaded programs; details are in the execution file. |
| `clone()` | Linux-specific primitive behind threads and namespace/container mechanisms; covered in process execution detail. |
| Process accounting | Covered with resource/debug behavior in `ch03_process_advanced.md`; keep core focused on the process ownership model. |
| `atexit()` inheritance | Handlers are inherited across `fork()` but discarded on successful `exec()`. |
| Exit status width | Normal exit status available to the parent is conventionally the low 8 bits. |

## Final Coverage Check

- Rows 3.1, 3.2, and 3.3 are covered in this file.
- Must-cover items covered here: program/process distinction, ownership boundary, `fork()`, CoW, nondeterministic parent/child scheduling, globals-not-IPC, FD sharing, process-vs-thread tradeoff, `exit()` vs `_exit()`, and Embedded fork/resource constraints.
- Must-cover items explicitly moved: `exec*()`, wait/reap, zombies/orphans/subreapers, close-on-exec, process groups/sessions, credentials, daemons, scheduling, limits, process accounting, and core dumps.
- No known blockers remain in this file if the moved topics remain covered in the linked Chapter 03 files.

## Interview Readiness

A strong answer starts with ownership and lifecycle, then names APIs.

You should be able to explain:

- A program is a passive file; a process is a running ownership boundary.
- `fork()` creates a child that returns from the same call site with a different return value.
- Copy-on-write makes `fork()` efficient, but not free under memory pressure.
- Parent and child have separate FD tables pointing to shared open file descriptions.
- `exit()` runs C library cleanup; `_exit()` terminates without flushing inherited stdio.
- `fork()` order is not deterministic; synchronization must be explicit.

Good interview flow:

```text
mental model -> fork mechanics -> CoW/FD inheritance -> termination choice -> bug/debug example
```

Cross-check yourself with scenarios:

- Why can `fork()` followed by `printf()` produce duplicated output?
- Why can a child closing `fd 3` leave the parent's `fd 3` working?
- Why can parent and child read from the same file and affect each other's offsets?
- Why should a child call `_exit()` after failed `exec()`?
