# Chapter 3 — Process Execution
## Topics: 3.4 Monitoring Child Processes · 3.5 Program Execution · 3.6 Process Creation in Detail
> Sources: TLPI Ch26, Ch27, Ch28 | DevLinux Module 03

---

## 3.4 Monitoring Child Processes

### Why Monitoring Is Needed

Process termination is asynchronous — a parent cannot predict when a child will die. Without monitoring:
- Parent doesn't know exit status or termination cause.
- Dead children become **zombies**: kernel retains a process table entry (PID, exit status, resource stats) until parent calls `wait()`. Zombies cannot be killed by any signal, including SIGKILL.
- If zombies accumulate, the kernel process table fills up → no new processes can be created.

---

### wait() — Basic Reaping

```c
#include <sys/wait.h>
pid_t wait(int *status);
// Returns: PID of terminated child, or -1 on error
```

`wait()` does four things:
1. **Blocks** until any child terminates (returns immediately if one already has).
2. Returns the **PID** of the terminated child.
3. Writes termination info into `*status` (if non-NULL).
4. Adds child's CPU time and resource stats to parent's running totals.

**Limitations**: cannot select a specific child; always blocks; cannot detect stopped/continued children.

Loop to reap all children:
```c
while ((childPid = wait(NULL)) != -1)
    continue;
if (errno != ECHILD)
    errExit("wait");
```

---

### waitpid() — Full Control

```c
pid_t waitpid(pid_t pid, int *status, int options);
```

**`pid` selection**:

| Value | Meaning |
|-------|---------|
| `> 0` | Wait for specific child with that PID |
| `== 0` | Wait for any child in caller's process group |
| `== -1` | Wait for any child (equivalent to `wait()`) |
| `< -1` | Wait for any child in process group `abs(pid)` |

**Key `options`**:

```c
WNOHANG    // Non-blocking: return 0 immediately if no child has changed state
WUNTRACED  // Also return when child is stopped by a signal
WCONTINUED // Also return when stopped child is resumed by SIGCONT (Linux 2.6.10+)
```

Non-blocking poll pattern:
```c
pid = waitpid(-1, &status, WNOHANG);
// 0  = no children changed state yet
// >0 = child PID that changed state
// -1 = error (ECHILD = no children remain)
```

---

### Decoding Wait Status — W* Macros

`status` is a packed 2-byte integer encoding the event type. **Always use macros — never read raw bits.**

```c
int status;
waitpid(childPid, &status, 0);

if (WIFEXITED(status))
    printf("exited, code=%d\n", WEXITSTATUS(status));
else if (WIFSIGNALED(status)) {
    printf("killed by signal %d\n", WTERMSIG(status));
    if (WCOREDUMP(status)) printf("(core dumped)\n");
} else if (WIFSTOPPED(status))
    printf("stopped by signal %d\n", WSTOPSIG(status));
else if (WIFCONTINUED(status))
    printf("continued\n");
```

| Macro | True when | Companion macro |
|-------|-----------|-----------------|
| `WIFEXITED(s)` | Child called `exit()`/`_exit()` | `WEXITSTATUS(s)` |
| `WIFSIGNALED(s)` | Child killed by signal | `WTERMSIG(s)`, `WCOREDUMP(s)` |
| `WIFSTOPPED(s)` | Child stopped by signal | `WSTOPSIG(s)` |
| `WIFCONTINUED(s)` | Child resumed by SIGCONT | — |

---

### Zombie and Orphan

**Zombie**: Child that has exited but not yet been `wait()`ed by parent.
- Kernel keeps a minimal process table entry (PID, exit status, resource stats).
- Displays as `<defunct>` in `ps`.
- **Cannot be killed by any signal**, including SIGKILL — by design, to guarantee parent can always `wait()`.
- Removal: kill the parent. Init inherits zombies → auto-waits → zombies disappear.

**Orphan**: Parent exits before child.
- Kernel reassigns parent to **init (PID 1)**.
- `getppid()` returns 1 in the child.
- Not harmful — init reaps all adopted children.
- Trick: `getppid() == 1` can detect that the original parent is gone.

---

### SIGCHLD — Asynchronous Reaping

Kernel sends **SIGCHLD** to parent whenever a child terminates (or stops/continues with appropriate `waitpid` options). Default disposition: ignored.

Install a handler to reap zombies without blocking:

```c
static void sigchldHandler(int sig) {
    int savedErrno = errno;         // protect errno — syscalls in handler may change it
    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;                   // loop until no more zombies
    errno = savedErrno;
}
```

**Why loop with WNOHANG**: SIGCHLD is a standard (non-queued) signal. If 3 children die while the handler runs, only 1 SIGCHLD is queued. A single `waitpid()` call would leave 2 zombies.

**Race condition — block SIGCHLD before fork()**:
```c
sigprocmask(SIG_BLOCK, &mask, NULL);   // block BEFORE any fork()
for (...) fork();

while (numLiveChildren > 0)
    sigsuspend(&empty_mask);           // atomic: unblock + sleep until signal

sigprocmask(SIG_UNBLOCK, &mask, NULL);
```
Without blocking first: a child could die and deliver SIGCHLD *before* `sigsuspend()` is reached → parent blocks forever.

---

## 3.5 Program Execution

### execve() — The System Call

```c
#include <unistd.h>
int execve(const char *pathname, char *const argv[], char *const envp[]);
// Never returns on success; returns -1 on error
```

`execve()` **replaces** the current process's text, data, BSS, heap, and stack with a new program. The PID remains the same.

- `pathname` — absolute or relative path to ELF binary or script.
- `argv[]` — NULL-terminated; `argv[0]` = program name by convention.
- `envp[]` — NULL-terminated `NAME=VALUE` strings for the new program's environment.

A successful `execve()` never returns. Code after it is only reached on error:
```c
execve("/bin/ls", args, env);
errExit("execve");   // only runs if execve failed
```

---

### exec() Family — Six Variants

All are wrappers over `execve()`. Differ only in argument passing style:

```c
execve (pathname, argv[],  envp[])   // array + custom env
execle (pathname, arg,..., envp[])   // list  + custom env
execvp (filename, argv[])            // array + PATH search + inherit env
execlp (filename, arg,...)           // list  + PATH search + inherit env
execv  (pathname, argv[])            // array + inherit env
execl  (pathname, arg,...)           // list  + inherit env
```

**Suffix meaning**:

| Suffix | Meaning |
|--------|---------|
| `v` | **v**ector — argv as array |
| `l` | **l**ist — argv as variadic args, terminated by `(char *) NULL` |
| `p` | **p**ath — search `PATH` env variable for filename |
| `e` | **e**nvironment — explicit `envp[]` argument |

**`NULL` terminator for list variants requires explicit cast**:
```c
execl("/bin/ls", "ls", "-l", "/tmp", (char *) NULL);
//                                    ↑ cast required to avoid UB on 64-bit
```

**PATH search** (`execvp`, `execlp`): Searches colon-separated directories. Ignored if filename contains `/`.

> **Security**: Never use `execvp`/`execlp` in set-user-ID programs — attacker can manipulate `PATH`. Always use `execv`/`execve` with absolute pathname.

---

### File Descriptors Across exec()

By default, **all open FDs survive exec()** — same FD numbers in the new program. The shell uses this for I/O redirection:

```
$ ls /tmp > dir.txt
```
1. Shell `fork()`s a child.
2. Child: closes stdout (fd 1), opens `dir.txt` as fd 1.
3. Child: `execve("/bin/ls", ...)` — `ls` writes to fd 1 → goes to `dir.txt`.

**`FD_CLOEXEC` flag**: Mark FD to auto-close on successful exec:
```c
// Via fcntl:
int flags = fcntl(fd, F_GETFD);
fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

// Or at open time:
int fd = open(path, O_RDONLY | O_CLOEXEC);
```
FD closes on successful exec; stays open if exec fails. Library functions should always set `FD_CLOEXEC` on FDs they open internally.

---

### Signals Across exec()

| Signal state | After exec() |
|--------------|-------------|
| Installed handlers | **Reset to SIG_DFL** (handler code is gone with old text segment) |
| SIG_IGN dispositions | **Preserved** |
| SIG_DFL dispositions | Preserved |
| Signal mask (blocked) | **Preserved** |
| Pending signals | **Preserved** |
| Alternate signal stack | **Cleared** (stack is gone) |

Implication: if parent has `SIGCHLD` ignored (to prevent zombies), child after `exec()` still has it ignored. For portability, call `signal(SIGCHLD, SIG_DFL)` before exec'ing arbitrary code.

---

### Interpreter Scripts — `#!` Mechanism

Kernel allows exec of text files if the first line is:
```
#! interpreter-path [optional-arg]
```

When `execve()` sees `#!`, it automatically translates:
```
execve("script.sh", ["script.sh", "arg1"], env)
→ runs: /bin/sh script.sh arg1
```

With optional arg:
```
#!/usr/bin/awk -f
```
→ kernel runs: `/usr/bin/awk -f script.awk input.txt`

- `#!` line limit: **127 characters** on Linux.
- `execlp()`/`execvp()`: if file has execute permission but no `#!` and is not ELF → exec `/bin/sh` on it automatically.

---

### system() — Convenience with Caveats

```c
#include <stdlib.h>
int system(const char *command);
```

Internally: `fork()` → child `execl("/bin/sh", "sh", "-c", command, NULL)` → parent `waitpid()`.

**Return value** (a wait status, decode with W* macros):

| Case | Return |
|------|--------|
| `command == NULL` | Nonzero if shell available |
| `fork()`/`waitpid()` fails | `-1` |
| Shell could not be exec'd | `127` (as if shell did `_exit(127)`) |
| Success | Exit status of the shell |

```c
status = system("ls | wc");
if (status == -1)
    errExit("system");
else if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
    fprintf(stderr, "shell failed\n");
else
    /* use WIFEXITED/WIFSIGNALED etc. */;
```

**Cost**: Creates at least **2 processes** (shell + command) → use direct `fork()`+`exec()` when performance matters.

**Security — NEVER use `system()` in set-user-ID programs**: Shell reads environment variables (`PATH`, `IFS`, `LD_PRELOAD`...) that an attacker can control → privilege escalation. Use `fork()` + `execv()`/`execve()` with absolute pathname instead.

---

## 3.6 Process Creation in Detail

### clone() — The Low-Level Foundation

```c
#define _GNU_SOURCE
#include <sched.h>
int clone(int (*func)(void *), void *child_stack, int flags, void *func_arg, ...
          /* pid_t *ptid, struct user_desc *tls, pid_t *ctid */);
// Returns: PID of child on success, -1 on error
```

`fork()`, `vfork()`, and POSIX threads all build on `clone()`. It differs from `fork()`:

| | `fork()` | `clone()` |
|---|---|---|
| Execution starts | At fork() return (same code) | At specified `func` |
| Stack | Copy of parent's | Caller allocates and passes |
| Attribute sharing | Fixed — everything copied | Per-flag bitmask control |
| Primary use | Process creation | Thread library implementation |

Stack must point to **high end** of allocated buffer (stack grows downward):
```c
char *stack = malloc(STACK_SIZE);
clone(childFunc, stack + STACK_SIZE, flags, arg);
```

All three of `fork()`, `vfork()`, and `clone()` are ultimately implemented by the same kernel function (`do_fork()`).

---

### flags — Per-Attribute Sharing Control

| Flag | Shared when set | Copied when not set |
|------|-----------------|---------------------|
| `CLONE_VM` | Virtual memory pages | Memory copy (CoW, like fork) |
| `CLONE_FILES` | FD table | Copy of FD table |
| `CLONE_FS` | umask, cwd, root dir | Copy of fs info |
| `CLONE_SIGHAND` | Signal disposition table | Copy of dispositions |
| `CLONE_THREAD` | Thread group with parent | New thread group |
| `CLONE_NEWNS` | — | New mount namespace |
| `CLONE_VFORK` | Parent suspended until child exec/_exit | Parent runs immediately |

Low byte of `flags` = **termination signal** sent to parent when child dies (usually `SIGCHLD`).

---

### fork / vfork / Thread as clone() Calls

```
fork()  ≈  clone(SIGCHLD)

vfork() ≈  clone(CLONE_VM | CLONE_VFORK | SIGCHLD)

POSIX thread (NPTL) = clone(
    CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_SIGHAND |
    CLONE_THREAD | CLONE_SETTLS |
    CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
)
```

> **Key insight**: "Process" vs "thread" is a matter of degree of sharing. The Linux kernel has only one concept: **kernel scheduling entity (KSE)**. A thread is a KSE that shares more attributes; a process is a KSE that shares fewer.

---

### Thread Groups — How Threads Share a PID

POSIX requires all threads in a process to share one PID. Linux implements this via **thread groups**:

- Each KSE has a unique **TID** (`gettid()` returns TID).
- KSEs in the same group share a **TGID** — `getpid()` returns **TGID**.
- The first thread (thread group leader) has `TID == TGID`.

```
Process PID 2001 (= TGID 2001)
├── Thread A: TID=2001, TGID=2001  ← group leader
├── Thread B: TID=2002, TGID=2001
├── Thread C: TID=2003, TGID=2001
└── Thread D: TID=2004, TGID=2001
```

With `CLONE_THREAD`: child joins parent's thread group → `getpid()` returns the same TGID.

**Thread termination**: A `CLONE_THREAD` thread does NOT send SIGCHLD on death. Parent is notified via **futex** (what `pthread_join()` uses internally). SIGCHLD is sent only when **all threads in the group terminate**.

---

### Process Attributes: fork() vs exec()

| Attribute | After `fork()` | After `exec()` |
|-----------|---------------|----------------|
| PID / PPID | New PID for child | Unchanged |
| Memory (text/data/heap/stack) | CoW copy | Replaced by new program |
| FD table | Copied (shared OFT entries) | Kept (unless `FD_CLOEXEC` set) |
| File offset / status flags | Shared with parent via OFT | Unchanged |
| Signal dispositions | Copied | Handlers → SIG_DFL; SIG_IGN kept |
| Signal mask | Copied | Unchanged |
| Pending signals | NOT inherited (child starts clean) | Unchanged |
| Environment | Copied | Replaced by `envp` argument |
| Working directory | Copied | Unchanged |
| umask | Copied | Unchanged |
| Resource limits (`rlimit`) | Copied | Unchanged |
| Process group / session | Unchanged (same group) | Unchanged |
| Exit handlers (`atexit`) | Copied | Cleared |
| Alternate signal stack | Copied | Cleared |
| Memory locks (`mlock`) | NOT inherited | Unchanged |

---

### Containers via clone() Namespaces

`CLONE_NEW*` flags give the child an **isolated namespace** — the foundation of Linux containers (Docker, LXC, systemd-nspawn):

| Flag | Isolated resource |
|------|------------------|
| `CLONE_NEWPID` | PID space (PID 1 inside container) |
| `CLONE_NEWNET` | Network interfaces, routing, firewall |
| `CLONE_NEWIPC` | System V IPC objects |
| `CLONE_NEWNS` | Mount points |
| `CLONE_NEWUTS` | Hostname and domain name |
| `CLONE_NEWUSER` | UID/GID mapping |

A container is a process (or process group) with full `CLONE_NEW*` isolation — appears to run on a separate machine, but shares the same kernel.

---

## Summary

| Topic | Core Mechanism |
|-------|---------------|
| wait() | Blocks until any child exits; reaps zombie and returns status |
| waitpid() | Select by PID/group; WNOHANG = non-blocking; WUNTRACED/WCONTINUED |
| W* macros | Decode wait status; never read raw bits |
| Zombie | Dead child not yet waited — kept in process table; immune to all signals |
| Orphan | Parent died first — init adopts; not harmful |
| SIGCHLD handler | Loop with WNOHANG; block before fork to avoid race with sigsuspend |
| execve() | Replaces program image; PID unchanged; never returns on success |
| exec() family | 6 variants: v/l = array/list, p = PATH search, e = custom env |
| FDs across exec | Survive by default; FD_CLOEXEC closes on exec; shell uses for I/O redirect |
| Signals across exec | Handlers reset to SIG_DFL; SIG_IGN kept; mask and pending kept |
| #! scripts | Kernel detects and transparently execs the named interpreter |
| system() | Convenience wrapper: fork+sh -c+waitpid; never use in set-user-ID programs |
| clone() | Foundation for fork/vfork/threads; flags bitmask controls each shared attribute |
| Thread vs Process | Degree of attribute sharing — kernel only has KSEs |
| Thread groups | TGID = PID; TID per thread; getpid() returns TGID |
| fork/exec attribute table | Signals, FDs, memory — know what is copied, shared, or reset |
| Containers | CLONE_NEW* flags create isolated namespaces — basis of Docker/LXC |
