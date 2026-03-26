# Chapter 3 — Process Core
## Topics: 3.1 Process Fundamentals · 3.2 Process Creation · 3.3 Process Termination
> Sources: TLPI Ch06, Ch24, Ch25 | DevLinux Module 03

---

## 3.1 Process Fundamentals

### Program vs Process

A **program** is a static ELF file on disk — machine code, data, symbol tables, shared-library metadata. It consumes no CPU or RAM until executed.

A **process** is a running instance of a program. From the kernel's perspective, a process has two parts:

- **User-space memory**: text, data, BSS, heap, stack
- **Kernel data structures**: PID/PPID, page tables, FD table, signal disposition, resource limits, working directory, credentials, etc.

The kernel owns the "file" on a process. The process is just the execution context.

One program → many processes. Each process gets its own stack/heap/data. The text segment is **shared read-only** across all instances.

---

### PID and PPID

```c
#include <unistd.h>
pid_t getpid(void);   // PID of calling process
pid_t getppid(void);  // PID of parent process
```

- PID is a positive integer, unique on the system at any given time.
- Default upper limit: **32767**. Adjustable via `/proc/sys/kernel/pid_max` (up to ~4M on 64-bit).
- When counter reaches 32767, it **resets to 300** (not 1) — low PIDs are permanently occupied by system daemons.
- All processes form a **tree** rooted at PID 1 (init/systemd).
- If a parent dies before its child → child becomes **orphan** → adopted by init → `getppid()` returns 1.

---

### Memory Layout

```
High address  ┌──────────────────────┐
              │  Stack (↓ grows down)│  auto variables, call frames
              ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤
              │  (unallocated)       │
              ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤
              │  Heap (↑ grows up)   │  malloc/free; top = program break
              ├──────────────────────┤  ← brk
              │  BSS                 │  global/static uninitialized → zeroed by kernel
              ├──────────────────────┤
              │  Initialized Data    │  global/static with explicit init values
              ├──────────────────────┤
              │  Text (read-only)    │  machine code; shared across processes
Low address   └──────────────────────┘
```

**Why BSS is separate from Initialized Data**: BSS doesn't need to be stored in the ELF file (just record location + size). Kernel zero-fills it at load time → smaller executable.

**Why Text is read-only**: Prevents accidental self-modification via bad pointers. Enables safe sharing among processes running the same program.

**Why Stack grows down, Heap grows up**: Hardware design (x86 stack pointer decrements on push). Sharing the unallocated middle region gives both flexibility to grow independently.

---

### Virtual Memory Management

The kernel does not expose physical RAM addresses to processes. Instead, each process operates in a **virtual address space**.

**Mechanism**:
- Virtual address space is divided into fixed-size **pages** (typically 4096 bytes).
- Physical RAM is divided into **page frames** of the same size.
- The kernel maintains a **page table per process**: virtual page → physical frame mapping.
- Hardware **PMMU** translates virtual → physical on every memory access.

**Page Fault**: When a process accesses a virtual page not currently in RAM (it's in swap):
1. CPU raises page fault exception.
2. Kernel suspends the process.
3. Kernel loads the page from disk into a free frame.
4. Page table entry updated.
5. Process resumes — transparently.

**Benefits of virtual memory**:

| Benefit | Mechanism |
|---------|-----------|
| Isolation | Each process has its own page table → cannot read other processes' RAM |
| Shared code | Text pages mapped read-only; multiple processes share same physical frames |
| Shared memory (IPC) | `mmap()`/`shmget()` make page table entries of two processes point to same frames |
| Memory protection | Page table entries carry read/write/exec permission bits |
| Overcommit | Virtual footprint can exceed physical RAM; swap bridges the gap |

---

### Stack and Stack Frames

Each function call **pushes a stack frame** containing:
- Local (automatic) variables
- Function arguments
- Return address (caller's program counter)
- Saved CPU registers

When the function returns, the frame is popped and the saved return address is restored into the PC.

**User Stack vs Kernel Stack**:

| | User Stack | Kernel Stack |
|---|---|---|
| Location | User space (high address) | Kernel space, per-process |
| Used for | User code function calls | Internal kernel calls during system calls |
| Size | Default 8MB (`ulimit -s`) | Small, fixed (~8KB–16KB) |

The kernel **cannot use the user stack** during a system call — the user stack is in unprotected memory. An attacker could craft a malicious stack to exploit the kernel.

---

### Command-line Arguments

```c
int main(int argc, char *argv[])
```

- `argc` ≥ 1 always (`argv[0]` = program name).
- `argv[argc]` = **NULL** (sentinel).
- `argv` and `environ` reside in a contiguous region just above the stack.
- Total size limit: `ARG_MAX` = 1/4 of soft `RLIMIT_STACK` at `execve()` time (since kernel 2.6.23).
- Check programmatically: `sysconf(_SC_ARG_MAX)`.
- Read any process's cmdline: `/proc/PID/cmdline` (null-delimited arguments).

**`argv[0]` trick**: `gzip`, `gunzip`, `zcat` are hard links to the same ELF. The program checks `argv[0]` to determine its behavior.

---

### Environment List

```c
extern char **environ;  // null-terminated array of "NAME=VALUE" strings
```

**Inheritance**: When `fork()` creates a child, the child receives a **copy** of the parent's environment at that moment. Changes after fork are **one-way and one-time** — neither side sees the other's changes.

```c
#include <stdlib.h>
char *getenv(const char *name);
int   setenv(const char *name, const char *value, int overwrite);  // copies args
int   putenv(char *string);    // stores pointer directly — does NOT copy
int   unsetenv(const char *name);
```

**Critical difference — `setenv` vs `putenv`**:
- `setenv()` **copies** name and value into a new buffer → safe with stack variables.
- `putenv()` stores the **pointer itself** → if `string` is a local (stack) variable, the environment entry becomes a dangling pointer after the function returns → undefined behavior.

**Sanitizing environment** (e.g., before exec'ing a set-user-ID program):
```c
environ = NULL;  // wipe entire environment
```

---

## 3.2 Process Creation

### Why fork() Exists

The shell cannot become `ls` — it must keep running after `ls` finishes. Linux solves this with two separate steps:
1. **`fork()`** — duplicate the calling process
2. **`exec()`** — replace the duplicate's program with a new one

This separation keeps each API simple and allows arbitrary work between the two steps.

### fork() — Duplicating a Process

```c
#include <unistd.h>
pid_t fork(void);
// Parent: returns child's PID (> 0)
// Child:  returns 0
// Error:  returns -1
```

After `fork()`, **two processes exist**, both continuing execution from the point immediately after the call. The return value is the only difference between them.

```c
switch (childPid = fork()) {
    case -1: /* error */ break;
    case 0:  /* child */
        _exit(EXIT_SUCCESS);
    default: /* parent */
        wait(NULL);
}
```

**What gets duplicated**: stack, data, heap, page tables, FD table, signal dispositions, environment.
**What is NOT duplicated**: PID (child gets a new one), pending signals, file locks (not inherited by child).

---

### Copy-on-Write (CoW)

Naively copying gigabytes of memory on every `fork()` would be prohibitively slow — especially since `fork()` is often followed immediately by `exec()`.

**Mechanism**:

1. After `fork()`, child receives a new page table but its entries **point to the same physical frames** as the parent. All these shared pages are marked **read-only**.
2. When either process attempts to **write** to a shared page:
   - CPU triggers a page fault (write to read-only page).
   - Kernel **copies that one page** to a new frame.
   - The faulting process's page table entry is updated to the new frame.
   - The process resumes and writes to its private copy.

```
Before write:                      After child writes to page 211:
Parent PT → frame 1998 ─┐          Parent PT → frame 1998  (unchanged)
Child  PT → frame 1998  ┘shared    Child  PT → frame 2038  (new copy)
```

**Text segment**: already read-only before fork → shared permanently, no CoW needed.

**Result**: `fork()` costs only a new page table allocation → nearly instant regardless of process size.

---

### File Sharing After fork()

Child inherits a **copy of the FD table** from parent. Copied FDs refer to the **same open file description** (OFD) in the kernel — same as `dup()`.

```
Parent fd3 ──────────────────> OFT entry: offset, flags
Child  fd3 ──────────────────> (same OFT entry)
```

**Consequence**: Parent and child share the file offset and status flags (e.g., `O_APPEND`). A `lseek()` in the child is visible to the parent.

**When useful**: Parent and child writing to the same log file — shared offset prevents overwrites.

**When problematic**: Each side wants independent access — must `close()` the other side's FDs immediately after fork.

---

### Race Condition After fork()

After `fork()`, the scheduler decides who runs next — there is **no POSIX guarantee**.

Linux history:
- 2.2: parent first (default)
- 2.6: child first (CoW optimization — child likely to exec immediately)
- 2.6.32+: parent first again (parent's TLB is warm)
- Tunable via `/proc/sys/kernel/sched_child_runs_first`

**Never write code that assumes a particular ordering.** Use explicit synchronization (signals, semaphores, pipes) when order matters.

**Synchronization with signal**:
```c
sigprocmask(SIG_BLOCK, &mask, NULL);  // block BEFORE fork to avoid race
switch (fork()) {
    case 0:   /* child does work, then notifies parent */
        kill(getppid(), SIGUSR1);
        _exit(0);
    default:  /* parent waits for child's signal */
        sigsuspend(&empty_mask);
}
```

---

### vfork() — Know It to Avoid It

`vfork()` was created before CoW existed. Its semantics:
- Child runs **in the parent's memory space** (no copy, no CoW).
- Parent is **blocked** until child calls `exec()` or `_exit()`.

**Dangers**:
- Any modification by the child (local variables, global data) **directly modifies the parent's memory**.
- If child `return`s from the function containing `vfork()` → undefined behavior (usually SIGSEGV).
- Child must only call `exec()` or `_exit()` — nothing else.

SUSv4 removed `vfork()` from the standard. With CoW, `fork()` approaches `vfork()` in speed. **Do not use `vfork()` in new code.**

---

## 3.3 Process Termination

### Two Termination Paths

| Path | Trigger | Cleanup |
|------|---------|---------|
| **Normal** | `_exit()`, `exit()`, `return` from `main()` | Controlled |
| **Abnormal** | Signal (SIGKILL, SIGSEGV, SIGTERM...) | No exit handlers, possible core dump |

---

### `_exit()` vs `exit()`

```c
#include <unistd.h>
void _exit(int status);    // system call — goes directly to kernel

#include <stdlib.h>
void exit(int status);     // library wrapper
```

`exit()` does three things before calling `_exit()`:
1. Call all registered exit handlers in **reverse order**
2. Flush all stdio buffers (`fflush`)
3. Call `_exit(status)`

**Only the bottom 8 bits** of `status` are passed to the parent via `wait()`.
```c
exit(257);  // parent sees 1  (257 & 0xFF = 1)
```

Convention: `0` = success, `!= 0` = failure. Avoid values > 128 in shell context — the shell uses `128 + signal_number` to encode "killed by signal".

---

### Kernel Cleanup on _exit()

Regardless of how a process exits, the kernel performs:
- Close all FDs, directory streams, message catalog descriptors
- Release all file locks
- Detach System V shared memory segments (decrement `shm_nattch`)
- Close POSIX semaphores and message queues
- If controlling process of a terminal → send SIGHUP to foreground process group
- Remove memory locks (`mlock`/`mlockall`)
- Unmap all `mmap()` mappings

---

### Exit Handlers — atexit()

```c
#include <stdlib.h>
int atexit(void (*func)(void));   // returns 0 on success, nonzero on error
```

- Registers `func` in a list. Executed in **LIFO order** on `exit()`.
- LIFO rationale: later-registered handlers often depend on earlier-registered resources, so they should clean up first.
- Child inherits parent's exit handler list after `fork()`.
- `exec()` **clears** all registrations (new program code replaces the handlers).
- Returns nonzero on error (not necessarily -1).

**Limitations**:
- No access to exit status
- Cannot pass arguments
- Cannot deregister (workaround: check a global flag inside the handler)

**Exit handlers are NOT called when**:
- `_exit()` is called directly
- Process is killed by a signal

`on_exit()` (glibc extension) solves the limitations but is non-portable — avoid in portable code.

---

### Critical Trap: fork() + stdio Buffer + exit()

```c
printf("Hello world\n");        // goes into stdio buffer
write(STDOUT_FILENO, "Ciao\n", 5);  // goes directly to kernel buffer
fork();
exit(EXIT_SUCCESS);             // both parent and child call exit()
```

**When redirected to a file** (block-buffered stdout):

"Hello world" sits in the **stdio buffer in user space** at the time of `fork()`. CoW duplicates this buffer into the child. When both call `exit()`, both flush → **"Hello world" appears twice**.

`write()` bypasses stdio → goes directly to kernel buffer → not duplicated → appears once (and appears first because kernel buffers are flushed to disk immediately).

**Fixes**:

Option 1 — Flush before `fork()`:
```c
fflush(stdout);
fork();
```

Option 2 — Child uses `_exit()`:
```c
if (fork() == 0) {
    _exit(EXIT_SUCCESS);   // no stdio flush, no exit handlers
}
exit(EXIT_SUCCESS);        // only parent flushes
```

> **Rule**: In a `fork()` application, **only one process** (usually the parent) should call `exit()`. All others should call `_exit()`. This ensures exit handlers and stdio are flushed exactly once.

---

## System Context — Processes in the Linux Architecture

**Where processes fit:**
Processes are the fundamental unit of resource allocation and isolation in the Linux kernel. Every running program is a process — a `task_struct` in the kernel with its own virtual address space, FD table, credentials, and scheduling state.

```
User Space                         Kernel Space
──────────────────                 ──────────────────────────────────────
text / data / BSS      ◄──────►   task_struct:
heap (malloc/brk)                    PID, PPID, credentials
stack (local vars)                   page table         → Memory Manager
                                     FD table           → VFS
                                     signal table       → Signal subsystem
                                     scheduling info    → Scheduler
                                     resource limits    → Resource Manager

fork() ──────────────────────────► do_fork():
                                     CoW page table clone
                                     new task_struct

exit() ──────────────────────────► kernel cleanup:
                                     close all FDs      (VFS)
                                     unmap memory       (MM)
                                     notify parent      (SIGCHLD)
                                     state → ZOMBIE until wait()
```

**Subsystem interactions:**
- **Memory Manager:** Each process has its own page table. `fork()` creates a CoW clone — child's page table entries point to parent's physical frames, marked read-only. The first write to a shared page triggers a fault and copies that one frame. `exec()` unmaps the old address space and loads a new ELF.
- **Scheduler:** `task_struct` is the scheduler's primary entity. `fork()` adds a new schedulable entity to the run queue. `exit()` marks the task as `TASK_ZOMBIE` — it remains in the process table until the parent calls `wait()`.
- **VFS (Filesystem):** `fork()` duplicates the FD table; child inherits the same open file descriptions (shared offsets). `exec()` loads a new binary via VFS. `exit()` closes all FDs, releasing any file locks.
- **Signal subsystem:** `fork()` clears pending signals in the child. `exit()` sends `SIGCHLD` to the parent. `exec()` resets handler dispositions to `SIG_DFL` (handler code is replaced by the new program text).
- **Process hierarchy:** If parent exits before child, the child is reparented to init (PID 1) — `getppid()` returns 1. `exit()` of the controlling process of a terminal sends `SIGHUP` to the foreground process group.

**Failure scenarios:**
- `fork()` fails with `EAGAIN` when `RLIMIT_NPROC` is reached — the process table for this real UID is full. Retry loops without backoff make this worse; handle the error and back off.
- Zombie accumulation: parent never calls `wait()` → each dead child retains a process table entry. When the table is full, `fork()` fails system-wide. Fix: install a `SIGCHLD` handler that loops `waitpid(-1, NULL, WNOHANG)`.
- CoW storm: parent forks with a large heap and both sides immediately write to every page → all pages copied → OOM killer triggered. Use `madvise(MADV_DONTFORK)` on large buffers the child does not need.
- `vfork()` misuse: child modifies any local variable or returns from the calling function → directly corrupts parent's memory (they share the same address space) → undefined behavior.
- stdio double-flush: both parent and child call `exit()` after `fork()` with block-buffered stdout → the stdio buffer (copied by CoW) is flushed twice → duplicate output. Fix: `fflush()` before `fork()`, or have child call `_exit()`.
- `putenv()` with a local variable: if the `string` argument goes out of scope, the environment entry becomes a dangling pointer → subsequent `getenv()` returns garbage or crashes.
- Stack overflow: `SIGSEGV` handler cannot run because there is no stack space left, unless `sigaltstack()` + `SA_ONSTACK` was set up beforehand.

---

## Debugging

**Inspect process state via `/proc`:**

```bash
# Process tree with PIDs
pstree -p

# Find zombie processes
ps aux | grep 'Z'
ps -eo pid,ppid,stat,cmd | awk '$3 ~ /Z/'

# Inspect virtual memory layout (all VMAs with permissions and mapped files)
cat /proc/<PID>/maps
cat /proc/<PID>/smaps        # per-VMA stats: RSS, PSS, dirty pages

# Check memory footprint
cat /proc/<PID>/status | grep -E "VmRSS|VmPeak|VmSize|VmStk|VmData"

# View all open file descriptors
ls -la /proc/<PID>/fd        # symlinks to actual files/sockets/pipes
lsof -p <PID>

# Check resource limits
cat /proc/<PID>/limits

# Inspect command-line arguments and environment
strings /proc/<PID>/cmdline  # NUL-separated argv
strings /proc/<PID>/environ  # NUL-separated environment variables

# Trace fork/exec/exit syscalls (follow children with -f)
strace -f -e trace=fork,clone,execve,wait4,_exit ./program
```

**GDB for fork/exec debugging:**

```
(gdb) set follow-fork-mode child    # follow child on fork() (default: parent)
(gdb) set detach-on-fork off        # keep both parent and child in debugger
(gdb) info inferiors                # list all processes being debugged
(gdb) inferior 2                    # switch to process #2
(gdb) catch exec                    # breakpoint when execve() succeeds
```

**Common bugs to catch in review:**

```c
/* BUG: not checking fork() return value for error */
pid_t pid = fork();
if (pid == 0) { /* child */ }
else { /* parent — but pid could be -1! */ }

/* FIX */
pid_t pid = fork();
if (pid == -1) errExit("fork");
else if (pid == 0) { /* child */ }
else { /* parent */ }

/* BUG: both parent and child call exit() — stdio double flush */
fork();
exit(0);

/* FIX: child uses _exit() — no stdio flush, no exit handlers */
if (fork() == 0) _exit(0);
exit(0);   /* only parent flushes stdio */

/* BUG: putenv() with stack variable → dangling pointer */
void setup_env(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "FOO=bar");
    putenv(buf);   /* stores pointer to stack — invalid after return */
}
/* FIX: use setenv() which copies the string */
setenv("FOO", "bar", 1);
```

---

## Real-world Usage

| Use case | API | Pattern |
|---|---|---|
| Shell command execution | `fork()` + `execve()` | Shell forks; child sets up I/O redirects with `dup2()`, then execs the command |
| Test process isolation | `fork()` | Each test runs in a child; parent collects exit status; crash in child cannot kill the test runner |
| Web server worker pool | `fork()` + shared listen socket | Pre-fork N workers; each child calls `accept()` independently on the shared fd |
| Privilege drop before exec | `fork()` + `seteuid()` + `execv()` | Fork; drop to non-root in child; exec command with reduced privilege |
| Check process liveness | `kill(pid, 0)` | Returns 0 = alive; `ESRCH` = gone; `EPERM` = alive, no permission to signal |
| Avoid CoW overhead for large buffers | `madvise(MADV_DONTFORK)` | Mark large mmap'd regions to not be inherited by child (e.g., large read-only ML model) |
| Prevent core dumps exposing credentials | `setrlimit(RLIMIT_CORE, 0)` | Set once in server startup; inherited by all forked children |
| Daemon initialization | `fork()` + `setsid()` + `fork()` | Double-fork to fully detach from terminal and become un-sessionable (see Topic 3.9) |

---

## Summary

| Topic | Core Mechanism |
|-------|---------------|
| Program vs Process | Kernel maintains all process state in kernel data structures; process is just the execution space |
| PID/PPID | Counter resets to 300; orphans adopted by init |
| Memory Layout | BSS separate → smaller ELF; text shared read-only across instances |
| Virtual Memory | Per-process page table → isolation, CoW sharing, transparent swap |
| Stack Frames | Kernel stack separate from user stack for security |
| argv/environ | `argv[argc]` = NULL; ARG_MAX = 1/4 RLIMIT_STACK; `putenv` stores pointer, not copy |
| fork() | Returns value distinguishes parent/child; both continue from same point |
| Copy-on-Write | Page table clone + read-only mark; physical copy only on write → fork() is fast |
| File sharing | Shared OFT entry → shared offset/flags; close unused FDs post-fork |
| Race condition | No ordering guarantee; use signals/semaphores for sync |
| vfork() | Deprecated; child runs in parent's memory — only exec()/_exit() allowed |
| exit() vs _exit() | exit() = handlers + flush + _exit(); child after fork should use _exit() |
| atexit() | LIFO, no status/arg access, cleared by exec(), not called on signal kill |
| stdio + fork() trap | stdio buffer copied by CoW → duplicate output on exit(); fix: fflush before fork or _exit() in child |
