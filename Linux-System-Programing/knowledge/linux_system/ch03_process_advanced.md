# Process Advanced — Groups, Sessions, Credentials, Daemons, Scheduling & Resources

> **Topics covered:** 3.7 · 3.8 · 3.9 · 3.10 · 3.11
> **Sources:** TLPI Ch34, Ch09, Ch37, Ch35, Ch36

---

## Topic 3.7 — Process Groups, Sessions & Job Control (TLPI Ch34)

### Process Groups

Every process belongs to a **process group**, identified by a **PGID (Process Group ID)**. A process group is a collection of related processes, typically from a single pipeline or job.

```
Process Group (PGID = 1226)
├── cmd1   PID=1226  ← group leader (PID == PGID)
└── cmd2   PID=1227
```

**Process Group Leader:** a process whose PID equals its PGID. The group survives even after the leader exits, as long as at least one member remains.

```c
pid_t getpgrp(void);                      // PGID of calling process
int   setpgid(pid_t pid, pid_t pgid);     // move pid into pgid group
```

**Constraints on `setpgid()`:**
- Cannot change PGID of a session leader.
- Can only move a process into a group within the same session.
- Can only be called on self or a child before that child calls `exec()`.

**Job-control shell race fix** — after `fork()`, BOTH parent AND child call `setpgid()`:
```c
// In parent:
setpgid(child_pid, child_pid);
// In child:
setpgid(0, child_pid);  // 0 = self
```
This avoids the race condition where the child calls `exec()` before the parent sets its PGID.

---

### Sessions

A **session** is a collection of process groups, typically corresponding to one terminal login.

```
Session (SID = 1204)
├── Process Group 1204  ← shell (session leader, foreground)
├── Process Group 1226  ← job 1 (background)
└── Process Group 1228  ← job 2 (background)
```

```c
pid_t getsid(pid_t pid);   // SID of pid (0 = self)
pid_t setsid(void);        // create new session
```

**`setsid()` mechanism:**
- Creates a new session + new process group; calling process becomes leader of both.
- The new session has **no controlling terminal**.
- Returns `EPERM` if calling process is already a process group leader.
- **Standard pattern** (mandatory for daemon creation):
```c
if (fork() != 0) exit(0);   // parent exits; child is not group leader
setsid();                    // child creates new session — succeeds
```

---

### Controlling Terminal & Controlling Process

Each session has **at most one controlling terminal** (tty or pty). Acquired when the session leader first calls `open()` on a terminal device (unless `O_NOCTTY` is specified).

- **Controlling process** = session leader that opened the terminal.
- `/dev/tty` — special file always referring to the process's controlling terminal (even when stdin/stdout are redirected).

**Foreground process group:**
```c
pid_t tcgetpgrp(int fd);               // get PGID of foreground group
int   tcsetpgrp(int fd, pid_t pgid);   // set foreground group
```

Only the foreground group has unrestricted access to the terminal. The shell calls `tcsetpgrp()` whenever it moves a job between foreground and background.

---

### Foreground vs Background — SIGTTIN / SIGTTOU

| Situation | Signal | Default action |
|---|---|---|
| Background process reads terminal | `SIGTTIN` | Stop process |
| Background process writes terminal (if `TOSTOP` set) | `SIGTTOU` | Stop process |
| Background process calls `tcsetpgrp()`, `tcsetattr()`... | `SIGTTOU` | Stop process |

**Special cases:**
- If SIGTTIN is blocked/ignored → `read()` fails with `errno = EIO` instead of stopping.
- If SIGTTOU is blocked/ignored → write succeeds even with `TOSTOP` set.

---

### SIGHUP Cascade on Terminal Disconnect

When a terminal is closed (modem hangup, xterm closed, etc.):

```
1. Kernel → SIGHUP → controlling process (shell)
2. Shell handler → SIGHUP → all its jobs (each process group)
3. Kernel → SIGHUP → foreground process group (direct, independent of #2)
```

**Protecting jobs from SIGHUP:**
- `nohup cmd &` — sets `SIG_IGN` for SIGHUP before `exec()`.
- `disown` (bash) — removes job from shell's job list; shell no longer sends SIGHUP.

---

### Job Control — States and Transitions

```
[Running fg]  --Ctrl-Z (SIGTSTP)-->  [Stopped]
[Stopped]     --fg (SIGCONT + tcsetpgrp)--> [Running fg]
[Stopped]     --bg (SIGCONT)-->  [Running bg]
[Running bg]  --terminal read--> SIGTTIN --> [Stopped]
[Running bg]  --fg (SIGCONT + tcsetpgrp)--> [Running fg]
```

**SIGTSTP vs SIGSTOP:**
- `SIGTSTP` (Ctrl-Z): catchable/ignorable — used by job-control shells.
- `SIGSTOP`: cannot be caught, blocked, or ignored — always stops immediately.

**Correct SIGTSTP handler pattern** (used in `vi`, `less`, and terminal UI apps):
```c
static void tstpHandler(int sig) {
    // 1. Reset disposition to SIG_DFL
    signal(SIGTSTP, SIG_DFL);
    // 2. Raise SIGTSTP
    raise(SIGTSTP);
    // 3. Unblock SIGTSTP → process stops here immediately
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &mask, &prevMask);
    // ... process is now stopped; resumes when SIGCONT arrives ...
    // 4. Reblock SIGTSTP
    sigprocmask(SIG_SETMASK, &prevMask, NULL);
    // 5. Reestablish this handler
    sigaction(SIGTSTP, &sa, NULL);
}
```
> Why not `raise(SIGSTOP)`? Because the parent would see the child stopped by SIGSTOP (misleading), not SIGTSTP.

**SIGCONT privilege exception:** A process in the same session can send SIGCONT to any process in that session, regardless of credentials. This allows the shell to resume set-user-ID programs even if they changed their real UID.

---

### Orphaned Process Groups

**Definition:** A process group is *orphaned* when all parents of every member are either in the same group or in a different session — meaning no process monitors the group from outside.

**Problem:** A stopped process in an orphaned group has no one to send it `SIGCONT` → it stays stopped forever.

**Kernel solution (SUSv3):** When a process group becomes orphaned AND has stopped members:
```
Kernel → SIGHUP  → all members  (notify: disconnected from session)
Kernel → SIGCONT → all members  (ensure they can run and handle SIGHUP)
```
If no stopped members exist → no signals sent.

---

## Topic 3.8 — Process Credentials (TLPI Ch09)

### The Five Credential Types

Every Linux process carries five types of UIDs and GIDs:

| Credential | Abbreviation | Purpose |
|---|---|---|
| Real UID/GID | `ruid`, `rgid` | Identifies the owner of the process |
| Effective UID/GID | `euid`, `egid` | Used for permission checks (signals, IPC, etc.) |
| Saved set UID/GID | `suid`, `sgid` | Saves euid/egid for privilege drop/regain |
| Filesystem UID/GID | `fsuid`, `fsgid` | Linux-only: used for file permission checks |
| Supplementary GIDs | — | Additional groups for permission checks |

In the common case: `real == effective == saved == filesystem`. They diverge only in the context of set-user-ID programs.

**Inspect via `/proc`:**
```bash
cat /proc/self/status | grep -E "^Uid:|^Gid:|^Groups:"
# Uid: real  effective  saved  filesystem
```

---

### Set-User-ID / Set-Group-ID Programs

Mechanism to allow a normal user to execute a program with the privileges of another user (typically root).

**Example — `passwd` writes to `/etc/shadow` (root-only):**
```bash
ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root ... /usr/bin/passwd
#    ↑ 's' = set-user-ID bit is set
```

**What happens on `exec()` of a set-user-ID program:**
```
Before exec(): real=1000  effective=1000  saved=1000
exec() /usr/bin/passwd (owner=root):
After exec():  real=1000  effective=0     saved=0
```
- `real` → unchanged (still the original user)
- `effective` ← file owner's UID (root = 0)
- `saved` ← copy of the new effective UID

---

### Saved Set-User-ID — Drop and Regain Privilege

The saved set-user-ID exists so a set-user-ID program can **temporarily drop and later regain** its elevated privilege.

**Standard secure pattern:**
```c
uid_t saved_euid = geteuid();   // save euid = 0 (root)

seteuid(getuid());              // drop: effective = real = 1000
// ... do unprivileged work ...

seteuid(saved_euid);            // regain: effective = 0 (root)
// ... do privileged work ...

setuid(getuid());               // permanently drop: real=eff=saved=1000
                                // ⚠️ one-way trip — cannot regain root
```

> **`seteuid()` is preferred** for drop/regain: it only changes the effective UID, leaving real and saved untouched.

---

### Credential API Summary

**Retrieval:**
```c
uid_t getuid(void);    // real UID
uid_t geteuid(void);   // effective UID
gid_t getgid(void);    // real GID
gid_t getegid(void);   // effective GID

// Linux-specific: read all three at once
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
```

**Modification comparison:**

| Syscall | Unprivileged | Privileged (euid=0) | Portability |
|---|---|---|---|
| `setuid(u)` | Sets effective only (= real or saved) | Sets **all three** — one-way trip! | SUSv3 |
| `seteuid(e)` | Sets effective = real or saved | Sets effective to any value | SUSv3 |
| `setreuid(r,e)` | Limited changes | Sets real + effective freely | SUSv3 |
| `setresuid(r,e,s)` | Any among {real, eff, saved} | Sets all three freely | Linux-specific |

**Critical trap — `setuid()` for privileged process:**
```c
// Running as euid=0:
setuid(1000);  // → real=1000, effective=1000, saved=1000
               // ⚠️ ROOT ACCESS PERMANENTLY LOST — irreversible

// To only temporarily drop:
seteuid(1000); // → effective=1000, saved still=0
seteuid(0);    // → regain root
```

---

### Filesystem UID/GID (Linux-specific)

`fsuid`/`fsgid` normally equal `euid`/`egid` and are updated automatically whenever `euid`/`egid` changes. They were introduced for the Linux NFS server to access files as a client user ID without changing `euid` (and thus without becoming vulnerable to signals from that user).

Since Linux 2.0, signal-sending rules no longer involve the target's `euid`, making fsuid/fsgid functionally redundant. **Do not use `setfsuid()`/`setfsgid()` in new code.**

---

### Supplementary Group IDs

```c
int getgroups(int gidsetsize, gid_t grouplist[]); // get list
int setgroups(size_t n, const gid_t *list);       // set list (privileged only)
int initgroups(const char *user, gid_t group);    // init from /etc/group
```

`initgroups()` is used by `login(1)` to set up the supplementary groups for a login shell. Permission checks use: effective GID + all supplementary GIDs.

---

## Topic 3.9 — Daemons (TLPI Ch37)

### What is a Daemon?

A daemon has two defining characteristics:
1. **Long-lived** — typically created at system boot, runs until shutdown.
2. **No controlling terminal** — the kernel never auto-generates SIGINT, SIGTSTP, or SIGHUP for it.

Examples: `crond`, `sshd`, `httpd`, `syslogd`. Convention: names end with `d`.

---

### The 7 Steps to Create a Daemon

```
Step 1: fork() → parent exit, child continues
        [WHY] (a) Shell sees parent exit → returns prompt
              (b) Child is guaranteed not to be a process group leader
                  → setsid() will succeed (no EPERM)

Step 2: setsid()
        [WHY] Child becomes session leader of a new session with no controlling terminal

Step 3: fork() again → parent exit, grandchild continues
        [WHY] Grandchild is NOT session leader → can never reacquire a
              controlling terminal (System V convention: only session leaders
              can acquire a controlling terminal via open())

Step 4: umask(0)
        [WHY] Ensures daemon controls its own file permissions without
              inheriting the parent's umask

Step 5: chdir("/")
        [WHY] Daemon is long-lived; if cwd is on a mounted filesystem,
              that filesystem cannot be unmounted while the daemon runs

Step 6: close() all inherited file descriptors
        [WHY] Release terminal FDs (0,1,2), avoid holding open files on
              other filesystems, prevent fd leaks

Step 7: Reopen fd 0, 1, 2 → /dev/null
        [WHY] Ensures standard FDs always exist (library functions that
              write to stdout/stderr won't fail or corrupt files)
```

**Reference implementation:**
```c
int becomeDaemon(void) {
    // Step 1
    switch (fork()) {
    case -1: return -1;
    case 0:  break;
    default: _exit(EXIT_SUCCESS);
    }
    // Step 2
    if (setsid() == -1) return -1;
    // Step 3
    switch (fork()) {
    case -1: return -1;
    case 0:  break;
    default: _exit(EXIT_SUCCESS);
    }
    // Step 4-7
    umask(0);
    chdir("/");
    int maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1) maxfd = 8192;
    for (int fd = 0; fd < maxfd; fd++) close(fd);
    int fd = open("/dev/null", O_RDWR);  // fd = 0 (stdin)
    dup2(fd, STDOUT_FILENO);             // fd = 1 (stdout)
    dup2(fd, STDERR_FILENO);             // fd = 2 (stderr)
    return 0;
}
```

**Verify with `ps`:**
```bash
ps -C my_daemon -o "pid ppid pgid sid tty command"
# PID  PPID  PGID   SID TT COMMAND
# 123     1   122   122  ?  ./my_daemon
#                        ↑ '?' = no controlling terminal ✅
# PID ≠ SID → not session leader → cannot reacquire terminal ✅
```

---

### SIGHUP as Reload Signal

Daemons have no controlling terminal → kernel **never** sends them SIGHUP. This makes SIGHUP a safe channel for "reload configuration" commands.

```c
static volatile sig_atomic_t hupReceived = 0;

static void sighupHandler(int sig) {
    hupReceived = 1;  // async-signal-safe: only set flag
}

// Main loop:
for (;;) {
    sleep(INTERVAL);
    if (hupReceived) {
        logClose(); logOpen(LOG_FILE);
        readConfigFile(CONFIG_FILE);
        hupReceived = 0;
    }
    // ... do work ...
}
```

**`logrotate` integration:** rename old log → send `SIGHUP` → daemon opens a new log file.

---

### Logging with syslog

Daemons have no terminal → cannot `printf()`. syslog provides centralized system-wide logging.

**Architecture:**
```
User process → syslog(3) → /dev/log (UNIX socket) → syslogd → /var/log/...
Kernel        → printk() → klogd  → /dev/log      → syslogd
```

**API:**
```c
#include <syslog.h>

openlog("my_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
//       ident        options              default facility

syslog(LOG_ERR, "Failed to open %s: %m", filename);
//     priority   format (%m = strerror(errno) — no extra call needed)

closelog();
```

**Priority levels (most → least severe):**
`LOG_EMERG` · `LOG_ALERT` · `LOG_CRIT` · `LOG_ERR` · `LOG_WARNING` · `LOG_NOTICE` · `LOG_INFO` · `LOG_DEBUG`

**Common facilities:** `LOG_DAEMON`, `LOG_AUTH`, `LOG_CRON`, `LOG_USER`

---

### Daemon Guidelines

1. **Single instance** — use a lock file (`/var/run/daemon.pid`) with `fcntl()` record locking.
2. **SIGTERM handler** — system shutdown sends SIGTERM; daemon must clean up quickly (< 5s before SIGKILL).
3. **No memory/FD leaks** — long-lived process; leaks accumulate.
4. **Standard paths** — config: `/etc/`; logs: `/var/log/`.

---

## Topic 3.10 — Process Priorities & Scheduling (TLPI Ch35)

### Nice Value — Default Scheduling

Linux defaults to **round-robin time-sharing (SCHED_OTHER)**: each process gets a time slice in turn.

**Nice value** influences (but does not control) scheduling priority:
```
-20 ← highest priority (only privileged)
  0 ← default
+19 ← lowest priority ("being nice" to others)
```

```c
#include <sys/resource.h>

// ⚠️ Must set errno=0 before call: getpriority() may legitimately return -1
errno = 0;
int prio = getpriority(PRIO_PROCESS, 0);  // 0 = self
if (prio == -1 && errno != 0) errExit("getpriority");

setpriority(PRIO_PROCESS, 0, 10);  // lower priority (be "nicer")
// which: PRIO_PROCESS | PRIO_PGRP | PRIO_USER
// who:   PID/PGID/UID (0 = self)
```

**Rules:**
- Unprivileged: can only *increase* nice value (lower priority).
- `CAP_SYS_NICE`: can set any value in [-20, +19].
- Inherited across `fork()`, preserved across `exec()`.

---

### Realtime Scheduling — SCHED_FIFO and SCHED_RR

For applications requiring **guaranteed response times** (industrial control, audio/video, navigation systems).

**Two POSIX realtime policies:**

| Policy | Mechanism | Time slice |
|---|---|---|
| `SCHED_FIFO` | Runs until it blocks, yields, terminates, or is preempted by higher priority | None |
| `SCHED_RR` | Same as FIFO but round-robin among equal-priority processes | ~0.1 seconds |

**Priority range:** 1 (lowest) – 99 (highest) for both policies.

> **Critical rule:** Any realtime process (priority ≥ 1) **always preempts** any SCHED_OTHER process. A runaway SCHED_FIFO process can lock up the entire system.

---

### Realtime API

```c
#include <sched.h>

// Set policy and priority
struct sched_param sp = { .sched_priority = 50 };
sched_setscheduler(pid, SCHED_FIFO, &sp);  // pid=0 → self

// Get policy
int policy = sched_getscheduler(pid);
// returns: SCHED_FIFO | SCHED_RR | SCHED_OTHER | ...

// Get priority
struct sched_param sp;
sched_getparam(pid, &sp);  // sp.sched_priority

// Query valid priority range (portable: do not hard-code 1/99)
int min = sched_get_priority_min(SCHED_RR);  // = 1 on Linux
int max = sched_get_priority_max(SCHED_RR);  // = 99 on Linux

// Voluntarily yield CPU (to same-priority processes)
sched_yield();
```

Policy and priority are inherited across `fork()` and preserved across `exec()`.

---

### Preventing Realtime Lockup

```c
// 1. CPU time soft limit → SIGXCPU; hard limit → SIGKILL
setrlimit(RLIMIT_CPU, &rl);

// 2. Wall clock alarm
alarm(seconds);              // → SIGALRM

// 3. Linux 2.6.25+: RT CPU burst limit (microseconds, no blocking)
setrlimit(RLIMIT_RTTIME, &rl);
```

---

### Additional Policies (Linux-specific)

| Policy | Description |
|---|---|
| `SCHED_BATCH` | Like `SCHED_OTHER` but penalizes frequent wakeups; for batch jobs |
| `SCHED_IDLE`  | Lower priority than nice +19; runs only when CPU is fully idle |

---

### CPU Affinity (Multiprocessor)

The Linux kernel maintains **soft CPU affinity** — it tries to keep a process on the same CPU to exploit cache warmth, but does not guarantee it.

**Hard affinity** pins a process to specific CPUs:

```c
#define _GNU_SOURCE
#include <sched.h>

cpu_set_t set;
CPU_ZERO(&set);
CPU_SET(0, &set);   // allow CPU 0
CPU_SET(1, &set);   // allow CPU 1

sched_setaffinity(pid, sizeof(set), &set);
sched_getaffinity(pid, sizeof(set), &set);

if (CPU_ISSET(0, &set)) { /* process may run on CPU 0 */ }
```

**Use cases:**
- Avoid cache invalidation overhead when migrating CPUs.
- Isolate a realtime task on a dedicated CPU (combined with `isolcpus` kernel boot param).
- Pin multiple threads sharing the same data to one CPU for cache locality.

---

## Topic 3.11 — Process Resources & Limits (TLPI Ch36)

### `getrusage()` — Monitor Resource Usage

```c
#include <sys/resource.h>

int getrusage(int who, struct rusage *res_usage);
// who: RUSAGE_SELF | RUSAGE_CHILDREN | RUSAGE_THREAD (Linux 2.6.26+)
```

**Fields actually filled by Linux** (others remain 0):

```c
struct rusage {
    struct timeval ru_utime;  // CPU time in user mode
    struct timeval ru_stime;  // CPU time in kernel mode
    long ru_maxrss;           // Peak resident set size (KB) [since 2.6.32]
    long ru_minflt;           // Minor (soft) page faults — no disk I/O needed
    long ru_majflt;           // Major (hard) page faults — disk I/O required
    long ru_inblock;          // Block input operations  [since 2.6.22]
    long ru_oublock;          // Block output operations [since 2.6.22]
    long ru_nvcsw;            // Voluntary context switches   [since 2.6]
    long ru_nivcsw;           // Involuntary context switches [since 2.6]
    // ... other fields unused on Linux ...
};
```

**`RUSAGE_CHILDREN`** accumulates stats of all terminated, waited-for descendants. Stats of a child are only added to the parent's `RUSAGE_CHILDREN` after the parent calls `wait()` for that child.

> This is a secondary reason (beyond zombie cleanup) why `wait()` is essential.

---

### `rlimit` — Soft and Hard Limits

```c
struct rlimit {
    rlim_t rlim_cur;  // Soft limit — enforced by kernel
    rlim_t rlim_max;  // Hard limit — ceiling on rlim_cur
};

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
// RLIM_INFINITY = no limit
```

**Rules:**

| Process type | Allowed |
|---|---|
| Unprivileged | Set soft ∈ [0, hard]; lower hard limit only (irreversible) |
| `CAP_SYS_RESOURCE` | Set soft and hard to any values |

Resource limits are inherited across `fork()` and preserved across `exec()`.

**Shell interface:**
```bash
ulimit -a              # view all current limits
ulimit -n 65536        # set RLIMIT_NOFILE soft limit
cat /proc/PID/limits   # view limits of any process
```

---

### Important Resource Limits

| Resource | Limits | On soft limit violation |
|---|---|---|
| `RLIMIT_CPU` | CPU time (seconds) | `SIGXCPU` → handler/cleanup → eventually `SIGKILL` at hard limit |
| `RLIMIT_FSIZE` | Max file size (bytes) | `SIGXFSZ` + `write()`/`truncate()` fail with `EFBIG` |
| `RLIMIT_NOFILE` | Max FD number + 1 | `open()`/`socket()`/`dup()` fail with `EMFILE` |
| `RLIMIT_NPROC` | Max processes per real UID | `fork()` fails with `EAGAIN` |
| `RLIMIT_AS` | Virtual memory size (bytes) | `mmap()`/`brk()`/`mremap()` fail with `ENOMEM` |
| `RLIMIT_STACK` | Stack size (bytes) | `SIGSEGV` (use `sigaltstack()` to handle it) |
| `RLIMIT_CORE` | Core dump size (bytes) | Core file truncated (0 = disable core dumps) |
| `RLIMIT_MEMLOCK` | Locked memory for `mlock()` | `mlock()` fails |
| `RLIMIT_RTPRIO` | Realtime scheduling priority ceiling | Connects with Topic 3.10 |
| `RLIMIT_RTTIME` | RT CPU burst time (µs, no blocking) | `SIGXCPU` → `SIGKILL` at hard limit |

---

### Common Patterns

**Raise `RLIMIT_NOFILE` for server processes:**
```c
struct rlimit rl;
getrlimit(RLIMIT_NOFILE, &rl);
rl.rlim_cur = 65536;
if (rl.rlim_cur > rl.rlim_max)
    rl.rlim_max = rl.rlim_cur;  // requires CAP_SYS_RESOURCE
setrlimit(RLIMIT_NOFILE, &rl);
```

**Disable core dumps for production:**
```c
struct rlimit rl = { .rlim_cur = 0, .rlim_max = 0 };
setrlimit(RLIMIT_CORE, &rl);
```

**Standard get-modify-set pattern:**
```c
struct rlimit rl;
if (getrlimit(RLIMIT_NPROC, &rl) == -1) errExit("getrlimit");
rl.rlim_cur = 256;   // adjust soft limit only
if (setrlimit(RLIMIT_NPROC, &rl) == -1) errExit("setrlimit");
```

**`RLIMIT_NPROC` is measured per real UID, not per process:**
```
All processes with real UID X combined must stay under the limit.
→ fork bomb prevention: each new process also inherits the limit.
```

**`RLIMIT_CPU` enforcement flow:**
```
CPU time reaches soft limit → SIGXCPU (handler can cleanup)
Further SIGXCPU each second  (if process continues)
CPU time reaches hard limit → SIGKILL (cannot be caught)
```

---

## Summary Table — Chapter 3 Advanced Topics

| Topic | Core Concept | Key APIs |
|---|---|---|
| 3.7 Sessions | Process Group < Session < Terminal; SIGHUP cascade; job control (SIGTSTP/SIGCONT) | `setpgid()`, `setsid()`, `tcsetpgrp()` |
| 3.8 Credentials | real/effective/saved triad; set-user-ID programs; drop/regain pattern | `seteuid()`, `setresuid()`, `getresuid()` |
| 3.9 Daemons | 7-step creation (fork→setsid→fork→umask→chdir→close→/dev/null); SIGHUP=reload; syslog | `setsid()`, `syslog()`, `openlog()` |
| 3.10 Scheduling | Nice (-20..+19) for SCHED_OTHER; SCHED_FIFO/RR (1..99) for realtime; CPU affinity | `setpriority()`, `sched_setscheduler()`, `sched_setaffinity()` |
| 3.11 Resources | `getrusage()` monitors usage; `rlimit` soft/hard ceiling on CPU/FD/memory/processes | `getrusage()`, `getrlimit()`, `setrlimit()` |
