# System Information — /proc Filesystem & System Limits

> **Source:** TLPI Ch11, Ch12 | DevLinux Module 01
> **Topics:** 1.4 /proc Filesystem + 1.5 System Limits & Options

---

## Problem It Solves

### /proc

The kernel is running — inside it are hundreds of live data structures:
process list, socket table, memory stats, CPU load...

Before `/proc` existed, getting this information required:
- Privileged programs that read raw kernel memory addresses
- Custom tools that had to be rewritten every kernel version

**`/proc` solves this:** a standard, stable interface to read (and sometimes write)
kernel state using ordinary file I/O — `open()`, `read()`, `write()`.

### System Limits

How many files can a process open?
What is the page size of this CPU architecture?
What is the maximum filename length on this filesystem?

These values **differ across systems**:
- ARM64 may use 64KB pages; x86 uses 4KB
- ext4 allows 255-byte filenames; old System V filesystems allow 14
- The kernel can be compiled with different resource limits

Hard-coding these values breaks portability.
`sysconf()` and `pathconf()` let you **query at runtime**.

---

## Concept Overview

### /proc — A Virtual Filesystem

`/proc` is a **virtual filesystem** — no bytes live on disk.
When you read `/proc/meminfo`, the kernel **generates the content on demand**
from its internal data structures, then returns it to you.

```
Regular file:     bytes stored on disk
/proc/meminfo:    kernel generates content at read time
```

**Analogy:** `/proc` is a window into a running engine.
Not a log — the live state at the moment you look.

### sysconf() / pathconf() — Runtime Limit Queries

These POSIX functions let you ask the kernel:
- "What is the page size of this hardware?"
- "How many files can I open at once?"
- "What is the max filename length on this filesystem?"

The answers may differ between systems, filesystem types, and kernel configurations.

---

## Architecture — /proc Structure

```
/proc/
├── [PID]/              ← One directory per running process
│   ├── status          ← Comprehensive process info
│   ├── cmdline         ← Command-line arguments (null-separated)
│   ├── environ         ← Environment variables (null-separated)
│   ├── fd/             ← Symlinks to every open file descriptor
│   ├── maps            ← Virtual memory layout
│   ├── exe             ← Symlink to the running executable
│   ├── cwd             ← Symlink to current working directory
│   └── task/           ← Subdir per thread
│
├── self/               ← Shortcut: always points to /proc/PID of the reading process
│
├── meminfo             ← System memory overview
├── cpuinfo             ← CPU hardware info
├── version             ← Kernel version string
├── uptime              ← System uptime
├── loadavg             ← CPU load averages (1/5/15 min)
├── filesystems         ← Supported filesystem types
│
└── sys/                ← Kernel parameters — READ and WRITE
    ├── kernel/
    │   ├── pid_max         ← Maximum PID number
    │   ├── hostname        ← System hostname
    │   └── msgmax          ← Max message queue message size
    ├── vm/
    │   ├── swappiness      ← Swap aggressiveness (0-100)
    │   └── overcommit_memory
    ├── net/
    │   ├── ipv4/ip_forward ← Enable IP routing (for gateways)
    │   └── core/somaxconn  ← Max socket connection backlog
    └── fs/
        └── file-max        ← System-wide max open files
```

---

## Internal Mechanism

### /proc/PID — Per-Process Information

Each running process gets its own directory `/proc/PID`.
The directory is **created when the process starts** and
**disappears when the process exits**.

**`/proc/PID/status` — most useful file:**
```
Name:   nginx                  ← process name
State:  S (sleeping)           ← current state
Pid:    1234                   ← process ID
PPid:   1                      ← parent PID (init/systemd)
Uid:    33  33  33  33         ← Real Effective Saved FS UIDs
Gid:    33  33  33  33         ← Real Effective Saved FS GIDs
Threads: 4                     ← number of threads
VmRSS:  12345 kB               ← actual RAM used right now
VmSize: 56789 kB               ← total virtual address space
SigBlk: 0000000000000000       ← blocked signals bitmask
```

**`/proc/PID/fd/` — open file descriptors:**
```bash
$ ls -la /proc/1234/fd/
lrwxrwxrwx /proc/1234/fd/0 -> /dev/pts/0       ← stdin
lrwxrwxrwx /proc/1234/fd/1 -> /dev/pts/0       ← stdout
lrwxrwxrwx /proc/1234/fd/2 -> /dev/pts/0       ← stderr
lrwxrwxrwx /proc/1234/fd/3 -> /var/log/app.log
lrwxrwxrwx /proc/1234/fd/4 -> socket:[98765]   ← TCP socket
```

**`/proc/PID/maps` — memory layout:**
```
08048000-08049000 r-xp 00000000 08:01 123  /usr/bin/myapp  ← text
08049000-0804a000 rw-p 00001000 08:01 123  /usr/bin/myapp  ← data
b7e00000-b7e21000 r-xp 00000000 08:01 456  /lib/libc.so.6  ← shared lib
bffeb000-c0000000 rwxp 00000000 00:00 0    [stack]         ← stack
```

### /proc/sys/ — Tunable Kernel Parameters

Unlike most `/proc` files which are read-only, files under `/proc/sys/`
can be **written to change kernel behavior at runtime**.

```bash
# Read current max PID
$ cat /proc/sys/kernel/pid_max
32768

# Increase it (requires root)
$ echo 65536 > /proc/sys/kernel/pid_max

# Enable IP forwarding (essential for embedded gateways/routers)
$ echo 1 > /proc/sys/net/ipv4/ip_forward

# Check system-wide file descriptor limit
$ cat /proc/sys/fs/file-max
1048576
```

**These changes are lost on reboot.**
For persistent changes, add to `/etc/sysctl.conf`:
```
net.ipv4.ip_forward = 1
kernel.pid_max = 65536
```
Then apply: `sysctl -p`

### /proc/self — A Process Reads Its Own Info

Any process can inspect itself without knowing its own PID:

```c
// These are equivalent:
open("/proc/self/fd", ...)
open("/proc/1234/fd", ...)   // if current PID is 1234
```

---

## System Limits — sysconf() and pathconf()

### Three Ways to Get Limits

```
1. Compile-time constant  → #include <limits.h>         hardcoded, not portable
2. sysconf()              → system-wide runtime limits   ask at runtime
3. pathconf()             → filesystem-specific limits   ask per filesystem
```

### sysconf() — System-Wide Limits

```c
#include <unistd.h>
long sysconf(int name);   // returns limit value or -1
```

**Important limits:**

| Constant | What it asks | Typical Linux value |
|----------|-------------|---------------------|
| `_SC_OPEN_MAX` | Max FDs a process can open | 1024 (default) |
| `_SC_PAGESIZE` | Memory page size (hardware) | 4096 (x86), 65536 (ARM64) |
| `_SC_NGROUPS_MAX` | Max supplementary groups | 65536 |
| `_SC_ARG_MAX` | Max bytes for exec() args + env | 2097152 |
| `_SC_CLK_TCK` | Clock ticks per second | 100 |
| `_SC_RTSIG_MAX` | Max distinct realtime signals | 32 |

**Correct usage pattern:**

```c
// WRONG — hard-coded, breaks on ARM64, MIPS, etc.
char buf[4096];        // assumes page size = 4096
int max_fds = 1024;    // assumes OPEN_MAX

// CORRECT — query at runtime
long page_size = sysconf(_SC_PAGESIZE);
char *buf = malloc(page_size);     // works on any architecture

long max_fds = sysconf(_SC_OPEN_MAX);
```

**Handling indeterminate limits:**

```c
errno = 0;
long lim = sysconf(_SC_OPEN_MAX);
if (lim == -1) {
    if (errno == 0)
        lim = 1024;   // limit exists but is indeterminate — use safe default
    else
        perror("sysconf");  // actual error
}
```

### pathconf() — Filesystem-Specific Limits

```c
long pathconf(const char *path, int name);
long fpathconf(int fd, int name);   // same but takes open fd
```

Same system, different filesystems can have different limits:
- `/` on ext4 → `NAME_MAX` = 255
- `/mnt/vfat` on FAT32 → effectively 255 with VFAT extension
- `/mnt/oldfs` on legacy System V fs → `NAME_MAX` = 14

**Important limits:**

| Constant | What it asks | POSIX minimum |
|----------|-------------|---------------|
| `_PC_NAME_MAX` | Max filename bytes (not counting `\0`) | 14 |
| `_PC_PATH_MAX` | Max pathname bytes (including `\0`) | 256 |
| `_PC_PIPE_BUF` | Max bytes written to pipe atomically | 512 |

```c
// Query the filesystem at /home
long name_max = pathconf("/home", _PC_NAME_MAX);
// → 255 on ext4

// Allocate a buffer that is always large enough for any pathname
long path_max = pathconf("/", _PC_PATH_MAX);
char *buf = malloc(path_max);
```

### Querying from Shell

```bash
# System limits
$ getconf ARG_MAX
2097152

$ getconf OPEN_MAX
1024

$ getconf PAGE_SIZE
4096

# Filesystem-specific
$ getconf NAME_MAX /
255

$ getconf PIPE_BUF /tmp
4096
```

---

## Debugging

```bash
# How much RAM is this process using?
$ cat /proc/1234/status | grep VmRSS
VmRSS: 12345 kB

# Is this process leaking file descriptors? (count should stay stable)
$ watch -n 1 'ls /proc/1234/fd | wc -l'

# What binary is this process running?
$ readlink /proc/1234/exe
/usr/bin/nginx

# What sockets does this process have open?
$ ls -la /proc/1234/fd | grep socket

# What is the current system load?
$ cat /proc/loadavg
0.52 0.48 0.41 2/342 12345
# 1min 5min 15min  running/total_threads  last_pid

# View full memory map of a process
$ cat /proc/1234/maps
```

---

## Real-world Usage

### Embedded — /proc for Runtime Diagnostics

```bash
# Read CPU architecture info on embedded board
$ cat /proc/cpuinfo | grep "Hardware"
Hardware : BCM2711

# Check available memory on constrained device
$ cat /proc/meminfo | grep -E "MemFree|MemAvailable"
MemFree:     45000 kB
MemAvailable: 120000 kB

# Monitor kernel messages in real time
$ cat /proc/kmsg   # or use dmesg
```

### /proc vs /sys — Both Exist on Embedded

```
/proc  ← process information + system parameters (older)
/sys   ← hardware/driver interface (newer, better structured)
```

```bash
# GPIO control via sysfs (classic embedded pattern)
$ echo 17 > /sys/class/gpio/export
$ echo out > /sys/class/gpio/gpio17/direction
$ echo 1 > /sys/class/gpio/gpio17/value     ← set GPIO pin 17 HIGH

# Read temperature sensor
$ cat /sys/class/thermal/thermal_zone0/temp
45000   ← millidegrees Celsius → 45°C
```

### Embedded — Why Page Size Matters

```
x86 / x86-64  →  4096 bytes  (4 KB)
ARM Cortex-A  →  4096 bytes  (or 64KB with CONFIG_ARM64_64K_PAGES)
ARM64         →  4096, 16384, or 65536 bytes depending on kernel config
MIPS          →  4096 bytes
```

For DMA buffers, memory-mapped I/O, and hardware register access on embedded:
**always use `sysconf(_SC_PAGESIZE)`** — never hard-code `4096`.
Wrong page alignment causes bus errors, cache coherency issues, or silently
broken data transfers.

---

## How /proc Works Internally

When you `open("/proc/meminfo")`, here is what happens:

```
1. VFS (Virtual File System) layer receives the open() call
2. VFS recognizes the path is under /proc
3. VFS calls the procfs driver instead of a real filesystem driver
4. procfs driver calls kernel functions to collect memory statistics
5. Data is formatted as text into a kernel buffer
6. Your read() call receives that buffer
→ No disk I/O ever happens
```

The file exists **only while you are reading it** — it is not stored anywhere.

---

## Key Takeaways

```
/proc = window into the running kernel
  → Not on disk — kernel generates content on demand
  → /proc/PID/      → per-process info (volatile — disappears on exit)
  → /proc/sys/      → tunable kernel params — writable by root
  → /proc/self/     → current process reads its own info
  → Use regular file I/O: open / read / write

sysconf()  = ask kernel: "what is the system-wide limit for X?"
pathconf() = ask kernel: "what is the limit of filesystem at this path?"

Never hard-code:
  page size, max open files, max filename length, max path length
Always query at runtime → correct on any hardware, any filesystem
```
