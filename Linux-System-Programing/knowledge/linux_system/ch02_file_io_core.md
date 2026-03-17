# Chapter 2 — File I/O: Core Model, Details & Buffering

> Covers: Topic 2.1 (Universal I/O Model), Topic 2.2 (Further Details), Topic 2.3 (Buffering)
> TLPI: Ch04, Ch05, Ch13 | DevLinux: Module 02

---

## Table of Contents

1. [The Problem: Why File I/O Needs a Unified Design](#1-the-problem)
2. [Universal I/O Model — 4 System Calls for Everything](#2-universal-io-model)
3. [File Descriptor — The Key Abstraction](#3-file-descriptor)
4. [Kernel's 3-Table Structure](#4-kernels-3-table-structure)
5. [open() — Opening Files in Detail](#5-open)
6. [read() and write() — How Data Flows](#6-read-and-write)
7. [lseek() — Navigating the File Offset](#7-lseek)
8. [Atomicity and Race Conditions](#8-atomicity-and-race-conditions)
9. [fcntl() — Runtime File Descriptor Control](#9-fcntl)
10. [dup() and dup2() — Duplicating File Descriptors](#10-dup-and-dup2)
11. [pread() and pwrite() — I/O Without Moving the Offset](#11-pread-and-pwrite)
12. [Scatter-Gather I/O: readv() and writev()](#12-scatter-gather-io)
13. [I/O Buffering — The 2-Layer System](#13-io-buffering)
14. [Controlling Kernel Buffering: fsync(), fdatasync(), O_SYNC](#14-controlling-kernel-buffering)
15. [Bypassing Kernel Cache: O_DIRECT](#15-odirect)
16. [Complete Mental Model](#16-complete-mental-model)
17. [Common Bugs and How to Avoid Them](#17-common-bugs)
18. [Real-World Usage Patterns](#18-real-world-usage-patterns)

---

## 1. The Problem

Before UNIX, every device had a different API. Reading a file used different code than reading from a serial port or a network socket. Programs were device-specific, fragile, and hard to compose.

**The UNIX solution:** treat everything as a file and give all I/O a single, uniform interface.

This design principle — "Everything is a file" — means:
- A regular file, a terminal, a pipe, a network socket, and a hardware device all respond to the same 4 system calls.
- Writing a program to process data makes it automatically usable with any input source.

```sh
./copy file.txt backup.txt         # file → file
./copy /dev/tty output.txt         # terminal → file
./copy /dev/sda1 /dev/sdb1         # block device → block device
```

The same program. The same calls. Different underlying resources.

---

## 2. Universal I/O Model

The entire file I/O system is built on **4 system calls**:

```
open()   →  get a handle (file descriptor) to a resource
read()   →  pull data in through that handle
write()  →  push data out through that handle
close()  →  release the handle
```

These 4 calls work on: regular files, directories, block devices (`/dev/sda`), character devices (`/dev/tty`), named pipes (FIFOs), sockets, and pseudo-files (`/proc/...`).

**How universality is achieved inside the kernel:**

```
User Code
  │  open/read/write/close (system call)
  ▼
Kernel — VFS Layer (Virtual File System)
  │  dispatches to the correct driver based on file type
  ├──► ext4 / btrfs driver → disk I/O
  ├──► TTY driver          → terminal hardware
  ├──► socket layer        → network stack
  ├──► pipe buffer         → memory ring buffer
  └──► /proc handler       → kernel data structures
```

The **VFS (Virtual File System)** is an abstraction layer inside the kernel. It exposes one interface to user programs while dispatching each call to the appropriate underlying driver. Your `read()` call doesn't know or care what kind of resource it is reading from.

When a device needs features outside the universal model (e.g., setting terminal baud rate), the `ioctl()` system call is available as an escape hatch.

---

## 3. File Descriptor

A **file descriptor (FD)** is a small non-negative integer that represents an open connection to a resource. It is the handle returned by `open()` and used by all subsequent I/O calls.

```c
int fd = open("data.txt", O_RDONLY);
// fd is now an integer like 3, 4, 5 ...
read(fd, buf, 1024);
close(fd);
```

### Standard File Descriptors

Every process starts with 3 file descriptors pre-opened by the shell:

| FD | Name   | POSIX Constant  | Default Target  |
|----|--------|-----------------|-----------------|
| 0  | stdin  | `STDIN_FILENO`  | keyboard        |
| 1  | stdout | `STDOUT_FILENO` | screen          |
| 2  | stderr | `STDERR_FILENO` | screen          |

### The Lowest-Available Rule

`open()` always returns the **lowest unused** file descriptor number. This is guaranteed by the kernel and is not accidental — it is the mechanism that makes I/O redirection work.

```c
close(STDIN_FILENO);          // close fd 0
fd = open("input.txt", O_RDONLY);
// fd is guaranteed to be 0 — stdin is now the file
```

---

## 4. Kernel's 3-Table Structure

Understanding this structure is essential. It explains the behavior of `fork()`, `dup2()`, file locking, and threads — topics you will encounter throughout this series.

The kernel maintains **three separate data structures** to manage file I/O:

```
Process A                         Process B
FD Table (per-process)            FD Table (per-process)
┌──────────────────────┐          ┌──────────────────────┐
│ fd 0: flags, ptr ────┼──┐    ┌──┼─ fd 0: flags, ptr   │
│ fd 1: flags, ptr ────┼──┼─┐  │  │ fd 1: flags, ptr... │
│ fd 2: flags, ptr ────┼──┼─┼──┤  │ fd 2: flags, ptr ───┼──┐
│ fd 20: flags, ptr────┼──┼─┘  │  │ fd 3: flags, ptr ───┼──┼─┐
└──────────────────────┘  │    │  └──────────────────────┘  │ │
                           │    │                             │ │
          ┌────────────────┘    │                             │ │
          │                     │                             │ │
          ▼                     ▼                             ▼ ▼
┌────────────────────────────────────────────────────────────────────┐
│              Open File Table (system-wide)                          │
│  [entry 23]: offset=100,  flags=O_RDONLY,  ──────► inode 1976     │
│  [entry 73]: offset=0,    flags=O_RDWR,    ──────► inode 1976     │
│  [entry 86]: offset=55,   flags=O_WRONLY,  ──────► inode 5139     │
└────────────────────────────────────────────────────────────────────┘
                                 │
                    ┌────────────┘
                    ▼
┌──────────────────────────────────────────┐
│         inode Table (system-wide)        │
│  inode 1976: type=file, perms, size, ... │
│  inode 5139: type=file, perms, size, ... │
└──────────────────────────────────────────┘
```

### What Each Table Stores

| Table | Scope | Stores |
|-------|-------|--------|
| **FD Table** | Per-process | FD → pointer to Open File entry; `close-on-exec` flag |
| **Open File Table** | System-wide | current file offset, open status flags, pointer to inode |
| **inode Table** | System-wide | file type, permissions, size, timestamps, disk block locations |

### Key Implications

**Case 1: Two FDs in the same process point to the same Open File entry**

This happens after `dup()`, `dup2()`, or `fcntl(F_DUPFD)`.

```
fd 1 ──┐
        ├──► [Open File Entry: offset=100, O_WRONLY]
fd 20 ─┘
```

- `read(fd1, ...)` advances the offset → `read(fd20, ...)` continues from the new offset.
- Changing `O_APPEND` via `fd1` is visible through `fd20`.
- But `close-on-exec` flag is **private** to each FD and is not shared.

**Case 2: Two processes each `open()` the same file independently**

```
Process A fd 3 ──► [entry 0:  offset=0]  ──┐
                                            ├──► inode 1976
Process B fd 4 ──► [entry 86: offset=0]  ──┘
```

- Two **separate** entries in the Open File Table → offsets are **independent**.
- Both point to the **same inode** → same physical file on disk.
- Writing concurrently **without** `O_APPEND` → race condition, data corruption.

**Case 3: After `fork()`**

Parent and child inherit the **same Open File entries** (not copies). This is how a parent and child can coordinate reading from the same pipe.

### Connection to Future Topics

| Topic | How 3-Table explains it |
|-------|------------------------|
| `fork()` (Ch3) | Child copies FD Table, shares Open File entries |
| `dup2()` | Creates a new FD pointing to the same Open File entry |
| File locking (2.7) | Locks are attached to the **inode**, not the FD |
| Threads (Ch6) | All threads share the same FD Table and thus the same offsets |

---

## 5. open()

```c
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags, ... /* mode_t mode */);
// Returns: file descriptor on success, -1 on error
```

`mode` is only needed (and only has effect) when `O_CREAT` is specified.

### Access Mode Flags (mandatory — choose one)

| Flag | Effect |
|------|--------|
| `O_RDONLY` | Open for reading only |
| `O_WRONLY` | Open for writing only |
| `O_RDWR` | Open for reading and writing |

Note: `O_RDWR ≠ O_RDONLY | O_WRONLY`. The latter is a logic error.

### File Creation Flags (optional — combine with `|`)

| Flag | Effect |
|------|--------|
| `O_CREAT` | Create the file if it does not exist |
| `O_TRUNC` | Truncate file to zero length on open (requires write permission) |
| `O_EXCL` | With `O_CREAT`: fail with `EEXIST` if file already exists (atomic check-and-create) |
| `O_APPEND` | All writes always go to end of file (atomic seek-and-write) |
| `O_DIRECTORY` | Fail if pathname is not a directory |
| `O_NOFOLLOW` | Do not dereference symbolic links |
| `O_CLOEXEC` | Set close-on-exec flag on the new FD |

### Open File Status Flags (can be changed later via `fcntl()`)

| Flag | Effect |
|------|--------|
| `O_NONBLOCK` | Non-blocking I/O |
| `O_SYNC` | Each write blocks until data reaches disk |
| `O_DSYNC` | Each write blocks until data (not metadata) reaches disk |
| `O_ASYNC` | Signal-driven I/O (must enable via `fcntl()`) |
| `O_NOATIME` | Do not update last-access time on read |
| `O_DIRECT` | Bypass kernel buffer cache |

### Common Usage Patterns

```c
// Read an existing file
fd = open("config.txt", O_RDONLY);

// Create a new file (fail if exists), write-only
fd = open("output.bin", O_WRONLY | O_CREAT | O_EXCL, 0644);

// Create or overwrite, write-only
fd = open("dump.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

// Append-only log — safe for concurrent writers
fd = open("app.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

// Read-write, create if missing
fd = open("db.dat", O_RDWR | O_CREAT, 0600);
```

### Common open() Errors

| errno | Meaning |
|-------|---------|
| `ENOENT` | File does not exist and `O_CREAT` not specified |
| `EEXIST` | File exists and both `O_CREAT | O_EXCL` were specified |
| `EACCES` | Permission denied |
| `EISDIR` | Attempted to open a directory for writing |
| `EMFILE` | Per-process FD limit reached |
| `ENFILE` | System-wide open file limit reached |

---

## 6. read() and write()

```c
#include <unistd.h>

ssize_t read(int fd, void *buffer, size_t count);
// Returns: number of bytes read, 0 on EOF, -1 on error

ssize_t write(int fd, const void *buffer, size_t count);
// Returns: number of bytes written, -1 on error
```

### File Offset Behavior

Each open file has a **current file offset** — a byte position pointer maintained in the Open File Table entry. It starts at 0 when the file is opened and advances automatically with each `read()` or `write()`.

```
File content:   [H][e][l][l][o][ ][W][o][r][l][d]
                 ^
                 offset = 0

After read(fd, buf, 5):
buf = "Hello"
                                ^
                                offset = 5

After read(fd, buf, 6):
buf = " World"
                                              ^
                                              offset = 11 (EOF)

Next read returns 0 → EOF
```

### Partial Reads and Writes

Both `read()` and `write()` may transfer **fewer bytes than requested**. This is not a bug — it is normal behavior for:
- `read()` near EOF
- `read()`/`write()` on pipes, sockets, or terminals
- `write()` when disk is full or a resource limit is hit

**Always check the return value and loop if necessary:**

```c
// Robust read — keeps reading until count bytes received or EOF/error
ssize_t readAll(int fd, void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = read(fd, (char *)buf + total, count - total);
        if (n == 0) break;   // EOF
        if (n == -1) {
            if (errno == EINTR) continue;  // interrupted by signal, retry
            return -1;                     // real error
        }
        total += n;
    }
    return total;
}
```

### read() Does Not Null-Terminate

`read()` deals with raw bytes — it has no concept of strings. If you need a C string, add `\0` manually:

```c
char buf[256];
ssize_t n = read(fd, buf, sizeof(buf) - 1);
if (n > 0) buf[n] = '\0';   // null-terminate manually
```

### Buffer Size and Performance

The number of `read()`/`write()` calls matters. Each system call has overhead (kernel transition). Benchmark results for copying 100 MB:

| Buffer size | Elapsed time |
|-------------|-------------|
| 1 byte (100M calls) | 107 seconds |
| 1024 bytes | 2.05 seconds |
| 4096 bytes | 2.05 seconds |
| 65536 bytes | 2.06 seconds |

**Rule of thumb:** use buffer sizes of at least **4096 bytes** (one disk block). Beyond that, gains are minimal.

---

## 7. lseek()

```c
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
// Returns: new file offset on success, -1 on error
```

`lseek()` repositions the file offset without doing any I/O. Nothing is read or written.

| `whence` | Meaning |
|----------|---------|
| `SEEK_SET` | Set offset to `offset` bytes from the start |
| `SEEK_CUR` | Move offset by `offset` bytes from current position |
| `SEEK_END` | Set offset to file size plus `offset` |

```c
lseek(fd, 0, SEEK_SET);    // go to the beginning
lseek(fd, 0, SEEK_END);    // go to one past the last byte
lseek(fd, -1, SEEK_END);   // go to the last byte
lseek(fd, -10, SEEK_CUR);  // go back 10 bytes

// Get current position without moving
off_t pos = lseek(fd, 0, SEEK_CUR);
```

**`lseek()` does not work on:** pipes, FIFOs, sockets, terminals.
Attempting it returns `-1` with `errno = ESPIPE`.

### File Holes

Seeking past the end of a file and writing creates a **file hole** — the gap between the old EOF and the new data.

```c
lseek(fd, 1000000, SEEK_END);
write(fd, "X", 1);
// File is now 1000001 bytes larger, but the gap contains null bytes
// and consumes NO disk space until data is written into it
```

This is used by sparse files (e.g., virtual disk images, core dumps).

---

## 8. Atomicity and Race Conditions

A **race condition** occurs when two concurrent operations on shared data produce results that depend on the order of scheduling — results that should never happen.

### Race: Non-Atomic File Creation

```c
// BROKEN: two processes can both believe they created the file exclusively
fd = open(path, O_WRONLY);
if (fd == -1 && errno == ENOENT) {
    // <<< Process B creates the file HERE
    fd = open(path, O_WRONLY | O_CREAT, 0600);  // Process A still succeeds
}
```

**Fix:** `O_CREAT | O_EXCL` performs check-and-create as a **single atomic kernel operation**:

```c
fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
if (fd == -1 && errno == EEXIST) {
    // Someone else created it first — handle appropriately
}
```

### Race: Non-Atomic Append

```c
// BROKEN: two processes writing to the same log file
lseek(fd, 0, SEEK_END);   // Process A seeks to offset 100
                           // <<< Process B seeks to offset 100 AND writes "BBB"
                           //     B's data is at offset 100
write(fd, "AAA", 3);      // Process A writes at offset 100 — overwrites B's data!
```

**Fix:** `O_APPEND` makes the seek-to-end and write a **single atomic operation**:

```c
fd = open("app.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
write(fd, "AAA", 3);  // always atomically appended to the end
```

**Why this matters in practice:** log files written by multiple processes or threads, database journal files, any shared output file.

---

## 9. fcntl()

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ...);
// Return value depends on cmd; -1 on error
```

`fcntl()` is a multipurpose system call for controlling file descriptors and open file properties **after** the file has already been opened.

### Reading and Modifying Open File Status Flags

```c
// Read current flags
int flags = fcntl(fd, F_GETFL);
if (flags == -1) errExit("fcntl F_GETFL");

// Check a specific flag
if (flags & O_APPEND)
    printf("append mode is on\n");

// Check access mode (O_RDONLY=0, O_WRONLY=1, O_RDWR=2 are not single bits)
int mode = flags & O_ACCMODE;
if (mode == O_WRONLY || mode == O_RDWR)
    printf("file is writable\n");

// Add O_NONBLOCK (e.g., on a socket received from accept())
flags |= O_NONBLOCK;
if (fcntl(fd, F_SETFL, flags) == -1) errExit("fcntl F_SETFL");

// Remove O_NONBLOCK
flags &= ~O_NONBLOCK;
fcntl(fd, F_SETFL, flags);
```

Flags that can be modified via `F_SETFL`: `O_APPEND`, `O_NONBLOCK`, `O_NOATIME`, `O_ASYNC`, `O_DIRECT`. Others are silently ignored.

### When to Use fcntl() Instead of open() Flags

- The FD was inherited (stdin, stdout, stderr) and its flags need changing.
- The FD came from another system call (`accept()`, `pipe()`, `socketpair()`) with no opportunity to set flags.
- Flags must be changed **at runtime** without closing and reopening the file.

---

## 10. dup() and dup2()

These calls create a **new FD** that refers to the **same Open File Table entry** as an existing FD. They share the file offset and status flags.

```c
#include <unistd.h>

int dup(int oldfd);              // returns lowest available FD
int dup2(int oldfd, int newfd);  // returns exactly newfd (closes newfd first if open)
int dup3(int oldfd, int newfd, int flags);  // like dup2, adds O_CLOEXEC support
```

After duplication:
```
fd 1 ──┐
        ├──► [Open File Entry: offset, flags, inode]
fd 20 ─┘
```

Both FDs share the same offset and status flags. The `close-on-exec` flag is **private** to each FD.

### The Classic Use Case: stdout Redirection

This is exactly how shells implement `./prog > output.txt`:

```c
int file_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
if (file_fd == -1) errExit("open");

// Make fd 1 (stdout) refer to output.txt
dup2(file_fd, STDOUT_FILENO);
close(file_fd);  // original fd no longer needed

// Now printf writes to output.txt
printf("This line goes into the file\n");
```

### Why Not Just Open the File Twice?

Two separate `open()` calls create **two separate Open File Table entries** — each with its own offset. If both write to the same file, they will overwrite each other's output because they do not share an offset.

`dup()`/`dup2()` create a second FD pointing to the **same entry** — they share the offset, so writes are sequential and do not corrupt each other.

### fcntl F_DUPFD — dup with minimum FD number

```c
// Duplicate oldfd, using the lowest available fd >= startfd
int newfd = fcntl(oldfd, F_DUPFD, startfd);

// Duplicate + set close-on-exec (Linux 2.6.24+)
int newfd = fcntl(oldfd, F_DUPFD_CLOEXEC, startfd);
```

---

## 11. pread() and pwrite()

```c
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
// Returns: bytes read, 0 on EOF, -1 on error

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
// Returns: bytes written, -1 on error
```

These are like `read()`/`write()`, but perform I/O at a **specified byte offset** without changing the current file offset.

```
pread(fd, buf, 100, 500)
→ reads 100 bytes starting at byte 500 in the file
→ file offset UNCHANGED after the call
```

Equivalent (but NOT atomic) to:
```c
off_t saved = lseek(fd, 0, SEEK_CUR);
lseek(fd, 500, SEEK_SET);
read(fd, buf, 100);
lseek(fd, saved, SEEK_SET);
```

### Why pread/pwrite Exists

**Multithreaded applications** — all threads in a process share the same FD Table and thus the same file offsets. If two threads use `lseek() + read()`:

```
Thread A: lseek(fd, pos_A, SEEK_SET)
          ← Thread B: lseek(fd, pos_B, SEEK_SET) — overwrites A's seek!
Thread A: read(fd, ...) — reads from the WRONG position
```

With `pread()`, each thread specifies its own offset as a parameter — no shared state, no race condition.

**Database engines** — reading arbitrary pages (fixed-size blocks) at random offsets in a large file is the core of any database. `pread()` is the natural tool for this.

---

## 12. Scatter-Gather I/O

```c
#include <sys/uio.h>

struct iovec {
    void  *iov_base;  // starting address of buffer
    size_t iov_len;   // number of bytes in this buffer
};

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
// Returns: total bytes read, 0 on EOF, -1 on error

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
// Returns: total bytes written, -1 on error
```

### The Problem They Solve

A network message or binary record often consists of a **header struct + variable-length payload + footer**. These live in separate memory buffers.

Without scatter-gather:
```c
// 3 separate system calls — not atomic, 3x kernel overhead
write(fd, &header, sizeof(header));
write(fd, body,    body_len);
write(fd, &footer, sizeof(footer));
```

With `writev()`:
```c
struct iovec iov[3];
iov[0].iov_base = &header;  iov[0].iov_len = sizeof(header);
iov[1].iov_base = body;     iov[1].iov_len = body_len;
iov[2].iov_base = &footer;  iov[2].iov_len = sizeof(footer);

writev(fd, iov, 3);
// One system call. Atomic. No intermediate buffer needed.
```

### Advantages

| Aspect | Multiple write() | writev() |
|--------|-----------------|----------|
| System calls | 3 | 1 |
| Atomicity | No — other processes can interleave | Yes — all data written contiguously |
| User-space buffer | Required if you want single write | Not required |
| Code complexity | Simple | Slightly more setup |

### readv() — Scatter Input

`readv()` reads a contiguous sequence from the file and **scatters** it into multiple buffers in order. Buffers are filled completely from `iov[0]`, then `iov[1]`, etc.

```c
struct iovec iov[3];
struct stat st;
int x;
char str[100];

iov[0].iov_base = &st;   iov[0].iov_len = sizeof(st);
iov[1].iov_base = &x;    iov[1].iov_len = sizeof(x);
iov[2].iov_base = str;   iov[2].iov_len = sizeof(str);

ssize_t n = readv(fd, iov, 3);
// st, x, str are now populated from consecutive bytes in the file
```

### preadv() and pwritev() (Linux 2.6.30+)

Combine scatter-gather with offset-based I/O — scatter-gather without moving the file offset:

```c
preadv(fd, iov, iovcnt, offset);
pwritev(fd, iov, iovcnt, offset);
```

---

## 13. I/O Buffering — The 2-Layer System

This is one of the most misunderstood areas in systems programming. When you call `write()`, **the data does not go to disk immediately**. There are two layers of buffering between your code and the physical disk.

```
Your Code
  │  printf() / fwrite() / fprintf()
  ▼
┌─────────────────────────────────────┐
│  stdio Buffer  (Layer 1)            │  ← User space, managed by glibc
│  ~8KB default                       │  ← Accumulates data, reduces syscalls
│  Lives in your process's memory     │
└─────────────────────────────────────┘
  │  write() system call (when buffer full, on flush, or on newline)
  ▼
┌─────────────────────────────────────┐
│  Kernel Buffer Cache  (Layer 2)     │  ← Kernel space, the page cache
│  Can be gigabytes if RAM allows     │  ← Absorbs writes, enables read-ahead
│  Shared across all processes        │
└─────────────────────────────────────┘
  │  kernel decides when to write to disk
  ▼
[ Physical Hard Drive / SSD ]
```

**Bottom line:**
- `write()` returns → data is in **kernel buffer**. Not necessarily on disk.
- `fwrite()` / `printf()` returns → data might still be in **stdio buffer**. Not even in kernel yet.

---

## 14. Controlling Kernel Buffering

### Layer 1: stdio Buffer

**Three buffering modes:**

| Mode | Constant | When does data leave the buffer? | Default for |
|------|----------|----------------------------------|-------------|
| Unbuffered | `_IONBF` | Immediately on every call | `stderr` |
| Line-buffered | `_IOLBF` | On newline `\n`, or when buffer full | `stdout` → terminal |
| Fully-buffered | `_IOFBF` | When buffer full (~8KB) | `stdout` → file/pipe |

```c
// Change buffering mode
setvbuf(stream, NULL, _IONBF, 0);     // turn off buffering
setvbuf(stream, NULL, _IOLBF, 0);     // line buffering
setvbuf(stream, NULL, _IOFBF, 8192);  // full buffering, 8KB buffer

// Force flush: push stdio buffer → kernel
fflush(stdout);
fflush(NULL);  // flush ALL stdio streams
```

**Important behavior difference:**

```sh
./program           # stdout is a terminal → line-buffered → prints promptly
./program > out.txt # stdout is a file    → fully-buffered → may not appear until exit!
```

### Layer 2: Kernel Buffer — fsync() and fdatasync()

After `write()` returns, data is in the kernel's page cache. To force it to physical disk:

```c
#include <unistd.h>

int fsync(int fd);     // flush data + ALL metadata → blocks until disk confirms
int fdatasync(int fd); // flush data + only essential metadata → blocks until disk confirms
void sync(void);       // flush ALL dirty kernel buffers system-wide (non-blocking on some UNIX)
```

**Difference between fsync and fdatasync:**

```
Data content changed:          both flush
File size changed:             both flush (needed to retrieve data)
Last-modification timestamp:   only fsync() flushes — fdatasync() skips this
```

`fdatasync()` can reduce disk operations from 2 to 1 (data update only, no metadata seek). For high-frequency writes (e.g., append-only logs), this is a significant performance gain.

### O_SYNC — Synchronize Every Write

```c
fd = open("critical.db", O_WRONLY | O_SYNC);
write(fd, data, len);
// This write() BLOCKS until data reaches disk hardware
```

**Performance cost — 100MB write benchmark:**

| Buffer size | Without O_SYNC | With O_SYNC | Slowdown |
|-------------|---------------|-------------|----------|
| 1 byte | 0.73s | 1030s | **1411x slower** |
| 16 bytes | 0.05s | 65.0s | 1300x slower |
| 256 bytes | 0.02s | 4.07s | 200x slower |
| 4096 bytes | 0.01s | 0.34s | 34x slower |

**When to use O_SYNC:** only when every individual write must survive a power failure before the next operation (e.g., financial transaction journal, medical device logging).

**Better alternative for most cases:** write in large batches, then call `fsync()` at transaction boundaries. This amortizes the sync cost.

### Summary: When Does Data Actually Reach Disk?

```
printf("data")              → data in stdio buffer
fflush(stream) / newline    → data in kernel buffer cache (write() called)
fsync(fd) / fdatasync(fd)   → data physically on disk (or disk cache)
hdparm -W0 (disable cache)  → data guaranteed on magnetic platter / flash cell
```

---

## 15. O_DIRECT

```c
fd = open("database.dat", O_RDWR | O_DIRECT);
```

Bypasses the **kernel buffer cache entirely**. Data transfers directly between user memory and disk hardware.

### Alignment Requirements (strict)

All three must be true, or `open()` / `read()` / `write()` returns `EINVAL`:
- Memory buffer starting address: multiple of physical block size (typically 512 bytes)
- File offset: multiple of physical block size
- Transfer length: multiple of physical block size

```c
void *buf;
posix_memalign(&buf, 512, 4096);  // 4096-byte buffer, aligned to 512 bytes
pread(fd, buf, 4096, 4096 * page_number);
```

### Why Does O_DIRECT Exist?

Database engines (PostgreSQL, MySQL, Oracle) implement their own buffer pool that is significantly larger and smarter than the kernel page cache for their specific workload. Using the kernel cache on top of their own cache is **double-buffering** — wasteful of RAM and CPU.

`O_DIRECT` lets them bypass the kernel cache and manage I/O themselves.

### Warning

If one process opens a file with `O_DIRECT` and another opens it normally, the kernel cannot maintain coherency between the two views. **This is undefined behavior.** Never mix `O_DIRECT` and normal I/O on the same file from different processes.

---

## 16. Complete Mental Model

```
┌─────────────────────────────────────────────────────────────────┐
│                        Your Program                             │
│                                                                 │
│  fprintf/fwrite/printf → [stdio buffer: ~8KB, user space]      │
│                                    │                            │
│  fflush() / newline / buffer full  │                            │
│                                    ▼                            │
│  open/read/write/close/lseek/pread/pwrite/readv/writev          │
│  (system calls)                   │                            │
└───────────────────────────────────┼────────────────────────────┘
                                    │ kernel boundary
                                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Kernel                                     │
│                                                                 │
│  FD Table  →  Open File Table  →  inode Table                  │
│  (per-proc)    (system-wide)       (system-wide)                │
│                  [offset, flags]    [type, perms, blocks]       │
│                                                                 │
│  VFS Layer → dispatches to correct filesystem driver            │
│                                                                 │
│  [Page Cache / Buffer Cache: kernel memory, lazy write]         │
│       │                                                         │
│  fsync()/fdatasync() forces flush                               │
│  pdflush thread flushes dirty pages every ~30 seconds           │
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                             [ Physical Disk ]
```

### API Quick Reference

| Goal | What to Use |
|------|-------------|
| Open a file, get FD | `open()` |
| Read sequentially | `read()` |
| Write sequentially | `write()` |
| Change read/write position | `lseek()` |
| Read at offset without moving position | `pread()` |
| Write at offset without moving position | `pwrite()` |
| Read into multiple buffers (single syscall) | `readv()` |
| Write from multiple buffers (single syscall, atomic) | `writev()` |
| Change flags on existing FD | `fcntl(F_GETFL / F_SETFL)` |
| Make a second FD pointing to same open file | `dup()` / `dup2()` |
| Force flush stdio buffer to kernel | `fflush()` |
| Force flush kernel buffer to disk | `fsync()` / `fdatasync()` |
| Make every individual write go to disk | Open with `O_SYNC` |
| Bypass kernel cache entirely | Open with `O_DIRECT` |

---

## 17. Common Bugs and How to Avoid Them

### Bug 1: Ignoring partial read/write

```c
// Wrong
write(fd, buf, 1000);  // might write only 600 bytes!

// Right
ssize_t written = write(fd, buf, 1000);
if (written != 1000) { /* handle partial write or error */ }
```

### Bug 2: Not null-terminating after read()

```c
// Wrong
char buf[256];
read(fd, buf, 255);
printf("%s", buf);  // undefined behavior — no null terminator

// Right
ssize_t n = read(fd, buf, 255);
if (n > 0) { buf[n] = '\0'; printf("%s", buf); }
```

### Bug 3: Leaking file descriptors

```c
// Wrong: fd is opened in a loop, never closed
while (1) {
    int fd = open(get_next_file(), O_RDONLY);
    process(fd);
    // forgot close(fd) → leak one FD per iteration
    // eventually: open() returns EMFILE "Too many open files"
}

// Right: always close(fd) when done
```

### Bug 4: Assuming write() means "on disk"

```c
// Wrong assumption: data is safe after write() returns
write(fd, critical_data, sizeof(critical_data));
power_cut();  // kernel buffer not flushed → data lost

// Right: call fsync() when durability is required
write(fd, critical_data, sizeof(critical_data));
fsync(fd);   // blocks until data is on disk
```

### Bug 5: Concurrent append without O_APPEND

```c
// Wrong: two processes appending to the same file
lseek(fd, 0, SEEK_END);
write(fd, data, len);  // race condition — see Section 8

// Right:
fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
write(fd, data, len);  // atomic append, safe for concurrent use
```

### Bug 6: stdout not flushed when redirected to file

```c
// Symptom: output missing in ./prog > out.txt if prog crashes
printf("Starting computation...");  // stuck in stdio buffer — line has no \n
do_long_computation();              // crash here → printf output never appears

// Fix option 1: add newline
printf("Starting computation...\n");

// Fix option 2: force flush
printf("Starting computation...");
fflush(stdout);

// Fix option 3: write to stderr (always unbuffered)
fprintf(stderr, "Starting computation...\n");
```

---

## 18. Real-World Usage Patterns

### Pattern 1: Safe Log File Writer

```c
// Multiple processes can safely append without corruption
int log_fd = open("/var/log/myapp.log",
                  O_WRONLY | O_CREAT | O_APPEND, 0644);

void log_write(const char *msg) {
    // O_APPEND makes this atomic for concurrent writers
    write(log_fd, msg, strlen(msg));
    write(log_fd, "\n", 1);
}
```

### Pattern 2: Atomic Configuration File Update

```c
// Never corrupt the config file even if the process crashes mid-write
void save_config(const char *path, const char *data, size_t len) {
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, data, len);
    fsync(fd);            // ensure data is on disk before rename
    close(fd);
    rename(tmp_path, path); // atomic on POSIX — readers always see complete file
}
```

### Pattern 3: Database Page Reader (multithreaded)

```c
// Multiple threads reading different pages simultaneously
void *read_page(int fd, int page_num, size_t page_size) {
    void *buf = malloc(page_size);
    off_t offset = (off_t)page_num * page_size;
    // pread is thread-safe — no shared offset state
    pread(fd, buf, page_size, offset);
    return buf;
}
```

### Pattern 4: Sending a Network Packet (scatter-gather)

```c
// Assemble packet from header + body without extra memcpy
struct pkt_header hdr = { .magic = 0xCAFE, .len = body_len };

struct iovec iov[2];
iov[0].iov_base = &hdr;   iov[0].iov_len = sizeof(hdr);
iov[1].iov_base = body;   iov[1].iov_len = body_len;

// One syscall, atomic write — no partial packet possible
writev(sock_fd, iov, 2);
```

### Pattern 5: Redirect Child Process Output

```c
// In parent: fork a child whose stdout goes to a file
int out_fd = open("child_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

pid_t pid = fork();
if (pid == 0) {
    // In child: redirect stdout to the file
    dup2(out_fd, STDOUT_FILENO);
    close(out_fd);          // original fd no longer needed in child
    execlp("ls", "ls", "-l", NULL);  // child's stdout now goes to file
}
close(out_fd);              // parent closes its copy
```

---

## Key Takeaways

1. **4 system calls** (`open/read/write/close`) handle all I/O in Linux via the VFS abstraction.
2. **FD** is an index into the per-process FD Table, which points into the system-wide Open File Table, which points into the inode Table.
3. **Sharing an Open File entry** (via `dup`, `fork`) means sharing the file offset — changes through one FD are visible through the other.
4. **O_CREAT | O_EXCL** and **O_APPEND** solve the most common race conditions in file I/O atomically.
5. **Two layers of buffering**: stdio buffer (user space) and kernel page cache. `write()` success only guarantees data reached the kernel buffer.
6. **`fsync()`** is the correct tool when durability is required. `O_SYNC` is correct when every individual write must be durable — at high performance cost.
7. **`pread()`/`pwrite()`** are essential for thread-safe and random-access I/O.
8. **`writev()`** provides atomic multi-buffer writes in a single system call — important for structured data transmission.
