# Chapter 2 — File I/O Core

> Topics: 2.1 File I/O Universal Model · 2.2 File I/O Further Details · 2.3 File I/O Buffering
> Main sources: TLPI Ch04, Ch05, Ch13 | DevLinux Module 02
> Production context: backend services, CLI tools, log writers, embedded daemons, device-facing programs, and any long-running process that opens files, pipes, sockets, or `/proc` data through file descriptors.

---

## Problem It Solves

Linux programs need a practical way to move bytes between user space and many kinds of
resources:

- regular files on disk;
- terminals;
- pipes;
- sockets;
- device files;
- virtual files such as `/proc/...`.

Without a unified model, every resource would need a different API and a different mental
model.
Programs would become harder to compose, harder to debug, and much harder to port.

This chapter answers one central question:

> When a program reads or writes data, what object is it really talking to, and what path do
> those bytes take through the system?

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | `open()`, `read()`, `write()`, `close()`, FD table, open file description, file offset, partial I/O | Write correct low-level I/O and explain why descriptors sometimes share state. |
| Work useful | `O_CLOEXEC`, `O_APPEND`, `dup2()`, `fcntl(F_GETFL/F_SETFL)`, `pread()/pwrite()`, `fflush()` vs `fsync()` | Avoid descriptor leaks, offset races, lost log writes, and fake durability assumptions. |
| Recognize | `readv()/writev()`, `O_DIRECT`, `O_SYNC/O_DSYNC`, `posix_fadvise()`, large-file portability details | Know when these appear in databases, storage-heavy services, and performance-sensitive tools. |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| file descriptor | Small per-process integer handle for an open resource. | `0`, `1`, `2` are stdin/stdout/stderr; `open()` returns the lowest free FD. |
| FD table | Per-process table mapping FD numbers to kernel open file descriptions. | `fork()` copies this table. |
| open file description | Kernel object storing current offset and file status flags. | Shared by `dup()` and inherited across `fork()`. |
| file offset | Byte position used by the next sequential `read()` or `write()`. | Advanced by I/O; changed explicitly by `lseek()`. |
| inode / underlying object | Kernel/filesystem object the open file description points to. | Regular file inode, pipe object, socket object, device object. |
| access mode | Read/write mode chosen at `open()`. | Exactly one of `O_RDONLY`, `O_WRONLY`, `O_RDWR`. |
| file status flags | Flags stored in the open file description. | `O_APPEND`, `O_NONBLOCK`, `O_DIRECT`; read/change with `fcntl()`. |
| FD flags | Flags stored on one FD table entry. | `FD_CLOEXEC`; set atomically with `O_CLOEXEC` when opening. |
| partial read/write | Successful I/O that transfers fewer bytes than requested. | Common on pipes, sockets, terminals; possible for writes on error/limits. |
| EOF | `read()` returns `0`, meaning no more bytes are available for that file position. | Not the same as `-1`, which means error. |
| page cache | Kernel memory cache for file data and dirty writes. | `write()` usually returns before disk persistence. |
| stdio buffer | User-space buffer inside `FILE *` streams. | `fflush()` moves data to kernel, not necessarily to disk. |
| durability | Data survives crash or power loss after being forced to stable storage. | Usually needs `fsync()`/`fdatasync()` plus correct directory sync for rename patterns. |
| atomic append | Append position and write happen as one kernel operation. | `O_APPEND` prevents `lseek(SEEK_END) + write()` races. |
| offset-independent I/O | I/O at a given offset without changing shared current offset. | `pread()` / `pwrite()` are useful in threaded code. |

---

## Concept Overview

### Roadmap

```text
pathname
   |
   v
open()
   |
   v
file descriptor in the process
   |
   v
open file description in the kernel
   |
   v
inode / pipe / socket / device object
   |
   v
VFS and kernel subsystems
   |
   +--> page cache / writeback for regular files
   +--> pipe buffer for pipes
   +--> socket buffers for networking
   +--> device drivers for hardware
```

### The Core Mental Model

At a high level, Linux file I/O is built around four ideas:

| Idea | Meaning |
|------|---------|
| **pathname** | human-readable name used to find an object |
| **file descriptor** | per-process handle returned by `open()` |
| **open file description** | kernel object that stores offset and file status flags |
| **underlying file object** | inode for a file, or another kernel object such as a pipe or socket |

### What This Chapter Must Make Clear

After this chapter, a learner should be able to explain:

- why `open()`, `read()`, `write()`, and `close()` work across many resource types;
- what a file descriptor is and what it is not;
- why two file descriptors sometimes share an offset and sometimes do not;
- why a successful `write()` does not necessarily mean "the data is already on disk";
- why `pread()` and `pwrite()` exist;
- why bugs around partial I/O, buffering, and descriptor inheritance are so common.

### Important Beginner Correction

The UNIX slogan "everything is a file" is useful, but it is a simplification.

The better statement is:

> Many Linux resources expose a file-descriptor-based interface, so one low-level I/O model
> works across many object types.

That does **not** mean every resource supports every file operation.

Examples:

- `lseek()` works on regular files, but not on pipes or sockets;
- `read()` and `write()` work on sockets, but the semantics are not identical to disk files;
- some device-specific operations require `ioctl()`.

---

## System Context

### Where File I/O Sits in Linux

```text
User program
    |
    +--> open() / read() / write() / close() / fcntl()
    |
    v
System call boundary
    |
    v
Kernel
    |
    +--> process FD table
    +--> VFS
    +--> page cache / writeback
    +--> filesystem drivers
    +--> pipe implementation
    +--> socket layer
    +--> device drivers
```

### Subsystems That Interact with File I/O

- **VFS** gives user space one common interface while routing operations to the correct
  filesystem or kernel object.
- **Process management** matters because the file descriptor table belongs to the process.
  `fork()` duplicates that table; `exec()` preserves descriptors unless close-on-exec is set.
- **Memory management** matters because regular file I/O often passes through the page cache,
  which is part of the kernel's memory system.
- **Filesystems** matter because path lookup, inodes, permissions, and writeback policy all
  live there.
- **Networking and IPC** matter because sockets and pipes also use file descriptors.

### Resources That Use the Same Low-Level Model

| Resource | `open()`? | `read()` / `write()`? | Notes |
|----------|-----------|-----------------------|-------|
| regular file | yes | yes | offset-based sequential I/O |
| directory | yes | not with normal file I/O APIs | usually accessed via directory APIs such as `opendir()` |
| pipe / FIFO | yes or inherited | yes | no seek; stream semantics |
| socket | often created by `socket()` | yes | no seek; networking semantics |
| device file | yes | yes | behavior depends on driver |
| `/proc` file | yes | often yes | data generated by kernel on demand |

---

## Architecture

### The Three-Kernel-Table Model

This is the most important mental model in the chapter.

```text
Process A
FD table
  fd 0 --> [open file description X]
  fd 1 --> [open file description Y]
  fd 3 --> [open file description Z]

Process B
FD table
  fd 0 --> [open file description X]
  fd 4 --> [open file description W]

Kernel open file descriptions
  X: current offset, file status flags, pointer to underlying object
  Y: current offset, file status flags, pointer to underlying object
  Z: current offset, file status flags, pointer to underlying object
  W: current offset, file status flags, pointer to underlying object

Underlying objects
  inode for regular file
  pipe object
  socket object
  device object
```

### What Each Layer Stores

| Layer | Scope | Stores |
|-------|-------|--------|
| **FD table** | per process | descriptor number, pointer to open file description, FD flags such as close-on-exec |
| **open file description** | kernel-wide | current file offset, file status flags such as `O_APPEND`, pointer to the underlying object |
| **underlying object** | kernel-wide | actual file/socket/pipe/device state |

### Why This Model Matters

It explains several behaviors that confuse beginners:

- `dup()` creates a new FD that refers to the **same** open file description;
- after `fork()`, parent and child share the same open file descriptions;
- two separate `open()` calls on the same pathname usually create **different** open file
  descriptions with independent offsets;
- file status flags such as `O_APPEND` belong to the open file description;
- close-on-exec belongs to the FD entry, not to the open file description.

### Standard File Descriptors

Every normal process starts with three conventional descriptors:

| FD | Name | Typical meaning |
|----|------|-----------------|
| `0` | `stdin` | standard input |
| `1` | `stdout` | standard output |
| `2` | `stderr` | standard error |

This is why shell redirection works so naturally:

- replace FD `0` to change input;
- replace FD `1` to change normal output;
- replace FD `2` to change error output.

### The Lowest-Available Rule

When the kernel allocates a new file descriptor, it returns the lowest currently unused number.

That rule is not just trivia.
It is one reason shell redirection is so elegant.

```c
close(STDOUT_FILENO);                     // close fd 1
int fd = open("out.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
/* fd is now 1, so future writes to stdout go to out.log */
```

---

## Execution Flow

### Flow 1: Open, Read, Close

```text
1. Program calls open("data.txt", O_RDONLY)
2. Kernel resolves the pathname through VFS
3. Kernel checks permissions and creates an open file description
4. Kernel places a new FD in the process FD table
5. Program calls read(fd, buf, n)
6. Kernel reads from the current file offset and advances the offset
7. Program calls close(fd)
8. Kernel removes the FD entry and releases the open file description when no references remain
```

### Flow 2: Writing a Regular File

```text
1. Program calls write(fd, buf, n)
2. Kernel copies bytes from user space into kernel-managed buffers
3. write() returns once the kernel has accepted the bytes
4. Kernel later flushes dirty data to storage
5. fsync() / fdatasync() forces synchronization when durability matters
```

### Flow 3: Shell Redirection

```text
1. Shell opens the target file
2. Shell uses dup2() so fd 1 points at that file
3. Shell execs the child program
4. Child writes to stdout
5. Kernel routes those writes to the redirected file instead of the terminal
```

This flow is one of the most practical reasons to understand descriptor duplication.

---

## 2.1 File I/O Universal Model

### The Four Foundational Calls

Linux low-level file I/O starts with four system calls:

```text
open()   -> obtain a descriptor
read()   -> bring bytes from kernel object into user space
write()  -> send bytes from user space into kernel object
close()  -> release the descriptor
```

These four calls are the base vocabulary of file I/O.

### `open()`

```c
#include <fcntl.h>
#include <sys/stat.h>

int open(const char *pathname, int flags, ... /* mode_t mode */);
```

`open()` does two different jobs:

- resolve a pathname;
- create an open file description and return a descriptor for it.

#### Flag Categories

| Category | Examples | Meaning |
|----------|----------|---------|
| access mode | `O_RDONLY`, `O_WRONLY`, `O_RDWR` | how the file will be accessed |
| creation flags | `O_CREAT`, `O_EXCL`, `O_TRUNC`, `O_CLOEXEC`, `O_NOFOLLOW` | affect opening itself |
| status flags | `O_APPEND`, `O_NONBLOCK`, `O_SYNC` | affect later I/O behavior |

#### Important `open()` Patterns

```c
/* Open existing file for reading */
int fd = open("data.txt", O_RDONLY);

/* Create a new file; fail if it already exists */
int fd2 = open("lockfile", O_WRONLY | O_CREAT | O_EXCL, 0600);

/* Open log file for atomic append */
int fd3 = open("app.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
```

#### Why `O_CLOEXEC` Matters

If a program spawns another process with `exec()`, descriptors are inherited by default.
That is sometimes useful, but sometimes a security bug or resource leak.

`O_CLOEXEC` requests that the descriptor be closed automatically across `exec()`.

For race-free code, setting this at `open()` time is better than opening first and then
calling `fcntl()` afterward.

### `read()`

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

`read()` attempts to copy up to `count` bytes into `buf`.

Possible results:

- positive value: number of bytes actually read;
- `0`: end-of-file on a regular file, or peer closed in some stream cases;
- `-1`: error.

#### Why `read()` May Return Less Than Requested

Short reads are normal in several cases:

- EOF was reached;
- a pipe or socket currently has fewer bytes available;
- the call was interrupted;
- the kernel chose to return available data now rather than wait for more.

So this is wrong:

```c
read(fd, buf, 4096);   /* assumes exactly 4096 bytes arrive */
```

This is the right mindset:

> `read()` returns how many bytes actually arrived this time.

### `write()`

```c
ssize_t write(int fd, const void *buf, size_t count);
```

`write()` attempts to transfer bytes from user space into the kernel.

Possible results:

- positive value: number of bytes accepted;
- `-1`: error.

#### Why `write()` May Also Be Partial

Short writes are especially common with:

- pipes;
- sockets;
- nonblocking descriptors;
- resource limits or signals.

For robust code, especially when writing a buffer completely matters, loop until all bytes
have been written or an error occurs.

### `close()`

```c
int close(int fd);
```

`close()` removes the FD entry from the process.
If that was the last reference to the open file description, the kernel releases it too.

#### Why `close()` Return Value Matters

For regular file writes, especially on networked filesystems, delayed write errors may show up
only at `close()` time.

So when data integrity matters, this is not enough:

```c
write(fd, buf, n);
close(fd);   /* ignored */
```

This is better:

```c
if (close(fd) == -1) {
    /* treat as real write-path failure */
}
```

---

## 2.2 File I/O Further Details

### File Offset

For seekable objects such as regular files, the open file description stores a **current file
offset**.

Sequential `read()` and `write()` use and update that offset automatically.

That means:

- two descriptors that share one open file description also share one offset;
- two independent `open()` calls on the same file usually have independent offsets.

### `lseek()`

```c
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

`lseek()` changes the current file offset for a seekable object.

Common modes:

| `whence` | Meaning |
|----------|---------|
| `SEEK_SET` | absolute offset from start of file |
| `SEEK_CUR` | relative to current offset |
| `SEEK_END` | relative to end of file |

#### Important Rule

`lseek()` changes the offset in the **open file description**.
So if two descriptors share that description, a seek through one descriptor affects the other.

#### Non-Seekable Objects

Pipes, FIFOs, and sockets do not have a normal seekable file position.
Calling `lseek()` on them fails, typically with `ESPIPE`.

#### Sparse Files and Holes

If you seek forward past the current end of file and then write, the gap becomes a **file
hole**.

Logically the file is larger, but the hole may not consume physical disk blocks.

This is why:

- `st_size` can be large;
- `st_blocks` can still be small.

### Atomicity and Race Conditions

File I/O bugs often come from assuming several operations are "close enough" to be safe.
They are not.

#### Safe File Creation

This sequence has a race:

```c
if (access("x.lock", F_OK) == -1)
    open("x.lock", O_CREAT, 0600);
```

Another process may create the file between the check and the `open()`.

This is better:

```c
open("x.lock", O_WRONLY | O_CREAT | O_EXCL, 0600);
```

#### Safe Append

This sequence is not atomic:

```c
lseek(fd, 0, SEEK_END);
write(fd, buf, len);
```

Two writers can interleave.

If multiple writers append to the same file, use `O_APPEND`.
That makes each write position-at-end step atomic with respect to the kernel's file offset
handling.

### `fcntl()`

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ...);
```

`fcntl()` is the "control panel" for descriptor-related behavior.

Common uses in this chapter:

| Goal | Typical command |
|------|-----------------|
| get file status flags | `F_GETFL` |
| set file status flags | `F_SETFL` |
| get FD flags | `F_GETFD` |
| set FD flags | `F_SETFD` |
| duplicate descriptor with constraints | `F_DUPFD` |

#### File Status Flags vs FD Flags

This distinction matters:

| Kind | Examples | Lives where |
|------|----------|-------------|
| file status flags | `O_APPEND`, `O_NONBLOCK` | open file description |
| FD flags | `FD_CLOEXEC` | per-process FD entry |

This is why changing `O_APPEND` via one shared descriptor affects the other, but
close-on-exec does not.

### `dup()`, `dup2()`, and `dup3()`

```c
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
```

These calls create another FD that refers to the same open file description.

That means the duplicated descriptor shares:

- current file offset;
- file status flags;
- access mode.

But it does **not** share the FD number itself, because that is per entry.

#### Why `dup2()` Is So Important

`dup2(oldfd, newfd)` is the standard way to implement redirection:

- redirect stdout to a file;
- redirect stderr to stdout;
- wire pipe ends into child processes before `exec()`.

### `pread()` and `pwrite()`

```c
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
```

These calls do I/O at a specified offset **without changing the shared file offset** in the
open file description.

That solves two common problems:

- multithreaded code where threads should not fight over one shared offset;
- code that wants random access without a separate `lseek()` step.

#### Why `pread()` Is Better Than `lseek() + read()`

This sequence is not atomic with respect to other users of the same open file description:

```c
lseek(fd, offset, SEEK_SET);
read(fd, buf, n);
```

Another thread or process sharing the same open file description can move the offset in
between.

`pread()` avoids that race completely.

### Scatter-Gather I/O: `readv()` and `writev()`

```c
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

Scatter-gather I/O lets one system call move data across multiple buffers.

#### Why It Exists

Sometimes one logical message is split across several memory regions:

- protocol header;
- message body;
- checksum or trailer.

Without `writev()`, code often copies everything into one temporary buffer first.

With `writev()`, the kernel gathers the pieces directly.

This helps:

- reduce copying;
- keep code organized by logical structure;
- preserve "one logical write" semantics more naturally.

---

## 2.3 File I/O Buffering

### The Two-Layer Buffering Model

For regular file output, there are usually two separate buffering layers to think about.

```text
Application
   |
   +--> stdio buffer (if using FILE *, printf, fwrite, ...)
   |
   v
write() system call boundary
   |
   v
kernel page cache / buffer cache
   |
   v
storage device
```

### Layer 1: User-Space stdio Buffer

This layer exists only if you use stdio APIs such as:

- `printf()`;
- `fprintf()`;
- `fwrite()`;
- `fputs()`.

This buffer is managed by the C library, not by the kernel.

Important consequence:

> `printf()` may not call `write()` immediately.

That is why output timing changes depending on whether stdout is connected to a terminal, a
pipe, or a regular file.

### Layer 2: Kernel Buffering for Regular Files

When `write()` succeeds on a regular file, the bytes are often placed into kernel memory first.
Actual device write-back may happen later.

So:

- `write()` success means the kernel accepted the bytes;
- it does **not** automatically mean the bytes are durable on disk.

### `fflush()` vs `fsync()`

This is one of the most important interview distinctions in the chapter.

| Call | Flushes what? | Typical use |
|------|----------------|-------------|
| `fflush(stream)` | stdio user-space buffer to the kernel | make `printf()` output leave libc |
| `fsync(fd)` | file data and metadata from kernel buffers to storage | durability for file updates |
| `fdatasync(fd)` | file data and minimal required metadata | durability with less metadata cost |

A useful short rule:

> `fflush()` gets data out of libc. `fsync()` gets data out of the kernel.

### `O_SYNC` and `O_DSYNC`

If a file is opened with synchronous-write flags, each write is forced toward storage with
stronger durability guarantees.

Use cases:

- journals;
- databases;
- crash-sensitive state files.

Trade-off:

- stronger durability;
- much lower throughput and higher latency if overused.

This is why many production systems do **not** use `O_SYNC` on every ordinary write.
They batch writes and call `fsync()` strategically instead.

### `O_DIRECT`

`O_DIRECT` asks the kernel to minimize page-cache involvement for I/O on that file.

Important cautions:

- it usually has strict alignment requirements on buffers, offsets, and sizes;
- it is not a "free performance boost";
- it makes application design harder;
- it is appropriate only for specialized workloads such as some database engines.

For most application code, normal buffered I/O plus careful synchronization is the better
choice.

### Durability Pattern for Important File Updates

For a file update where durability matters, the rough sequence is:

```text
1. write new data
2. fflush() if using stdio
3. fsync() or fdatasync() if durability matters
4. close() and check the return value
```

If the update also changes a pathname via `rename()`, that full safe-update pattern belongs in
the filesystem section of Chapter 2.

### Embedded Storage Constraints

Embedded Linux systems make buffering decisions more visible because storage may be slow,
wear-limited, or vulnerable to sudden power loss.

Practical rules:

- decide which data is **best effort** and which data is **state that must survive reboot**;
- batch noncritical writes instead of calling `fsync()` after every small record;
- use `fsync()` or `fdatasync()` at explicit commit points for important state;
- prefer append/checkpoint designs when they reduce rewrite frequency on flash;
- treat `O_SYNC` as a correctness tool with a high latency and wear cost;
- use `O_DIRECT` only when the application can satisfy alignment rules and manage caching
  deliberately.

Typical embedded bugs:

- a device says "settings saved" but loses them after power removal;
- frequent log syncs shorten flash lifetime or stall the main loop;
- a read-only or nearly full filesystem turns ordinary writes into late failures;
- ignoring `fsync()` or `close()` errors hides media and writeback problems.

---

## Work-Useful Patterns

| Pattern | Use it when | Production trap |
|---------|-------------|-----------------|
| Robust write loop | You must write an exact byte count to an FD. | A single `write()` can be partial; loop until done or real error. |
| Retry only interruptible calls | A blocking syscall fails with `errno == EINTR`. | Retry the syscall, but do not blindly retry `close()` as if it were an ordinary write. |
| Open with `O_CLOEXEC` | Opening files in code that may later `exec()`. | Setting close-on-exec later with `fcntl()` has a race in multithreaded programs. |
| Use `dup2()`/`dup3()` for redirection | Implementing shells, supervisors, child stdout/stderr capture. | `close(1); open(...)` is fragile if another thread opens an FD between the calls. |
| Use `O_APPEND` for shared logs | Multiple processes append to the same regular file. | `lseek(fd, 0, SEEK_END); write(...)` is not atomic. |
| Prefer `pread()/pwrite()` for positioned concurrent I/O | Threads or processes share an open file description. | `lseek() + read()` races on the shared offset. |
| Flush the correct layer | Mixing `stdio` and raw syscalls or requiring durability. | `fflush()` only drains stdio to kernel; `fsync()` handles kernel-to-storage. |
| Check `close()` on important output | NFS, quotas, delayed writeback, or storage errors matter. | Some writeback errors may be reported late. |

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| `readv()` / `writev()` | Scatter-gather I/O avoids copying multiple buffers into one temporary buffer; still check for partial transfer. |
| `preadv()` / `pwritev()` | Linux/BSD extension combining scatter-gather with explicit offsets. |
| `O_DIRECT` | Specialized path that tries to bypass page cache; alignment-heavy and often used by databases, not normal app code. |
| `O_SYNC` / `O_DSYNC` | Force synchronous writes; useful for correctness but can be extremely expensive. |
| `posix_fadvise()` | Performance hint only; it should not change program semantics. |
| large-file feature macros | Use `off_t` correctly; on old 32-bit environments `_FILE_OFFSET_BITS=64` avoids `EOVERFLOW` surprises. |
| `ioctl()` | Escape hatch for operations outside the universal I/O model, especially devices. |

---

## Example

### Example 1 — Robust write loop

```c
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

static int write_all(int fd, const char *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t n = write(fd, buf + total, len - total);
        if (n > 0) {
            total += (size_t)n;
            continue;
        }

        if (n == -1 && errno == EINTR)
            continue;

        return -1;
    }

    return 0;
}

int main(void) {
    const char msg[] = "hello through robust write\n";

    if (write_all(STDOUT_FILENO, msg, sizeof(msg) - 1) == -1) {
        perror("write_all");
        return 1;
    }

    return 0;
}
```

What it teaches:

- a successful `write()` does not guarantee "all bytes were written";
- loops are the normal pattern for reliable output;
- `EINTR` handling belongs in real system code.

### Example 2 — Redirect child stdout to a file

```c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        int fd = open("child.out", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            _exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }

        close(fd);
        execlp("echo", "echo", "redirected output", (char *)NULL);
        perror("execlp");
        _exit(1);
    }

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    return 0;
}
```

What it teaches:

- shell-style redirection is just descriptor manipulation;
- `dup2()` is the key operation behind redirecting stdout and stderr;
- descriptors must be arranged before `exec()`.

### Example 3 — `pread()` avoids shared-offset races

```c
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    char buf[16];
    int fd = open("data.bin", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    ssize_t n = pread(fd, buf, sizeof(buf), 128);
    if (n == -1) {
        perror("pread");
        close(fd);
        return 1;
    }

    printf("read %zd bytes from offset 128\n", n);
    close(fd);
    return 0;
}
```

What it teaches:

- random-access reads do not need to disturb the shared file offset;
- `pread()` is especially useful for multithreaded file readers and page-oriented designs.

### Example 4 — Flush both stdio and kernel layers

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>

int main(void) {
    FILE *fp = fopen("important.log", "w");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    fprintf(fp, "important line\n");

    if (fflush(fp) == EOF) {
        perror("fflush");
        fclose(fp);
        return 1;
    }

    if (fsync(fileno(fp)) == -1) {
        perror("fsync");
        fclose(fp);
        return 1;
    }

    if (fclose(fp) == EOF) {
        perror("fclose");
        return 1;
    }

    return 0;
}
```

What it teaches:

- `fprintf()` first interacts with the stdio buffer;
- `fflush()` and `fsync()` solve different problems;
- `fclose()` should still be checked.

---

## Debugging

Useful commands:

```bash
# Trace low-level I/O syscalls
strace -e open,openat,read,write,close,lseek,fcntl ./program

# See which files a process has open
ls -la /proc/<PID>/fd
lsof -p <PID>

# Inspect descriptor-specific state
cat /proc/<PID>/fdinfo/3

# Check file size vs blocks for sparse files
stat sparse.bin
du -h sparse.bin

# Inspect buffering-related behavior from the shell
stdbuf -o0 ./program

# Inspect storage and mount context when durability behavior is surprising
findmnt
dmesg | tail
```

Common pitfalls:

- treating file descriptors as if they were the file itself rather than handles;
- confusing file descriptor with open file description;
- assuming `read()` or `write()` transfers all requested bytes in one call;
- forgetting that `read()` does not null-terminate a buffer;
- assuming `write()` means "already on disk";
- forgetting to check `close()` on important write paths;
- using `lseek() + read()` where `pread()` is the correct race-free tool;
- leaking descriptors across `exec()` because `FD_CLOEXEC` was not set.

---

## Real-world Usage

### Where This Knowledge Shows Up

- shell redirection and pipelines;
- log writers that use `O_APPEND` and periodic synchronization;
- servers that inherit descriptors intentionally or accidentally across `exec()`;
- database-style code that reads fixed offsets with `pread()`;
- tools that inspect live system state via `/proc`;
- systems code that must explain why data was "written" but disappeared after a crash.

### Design Decisions You Make with This Chapter

| Scenario | Practical design |
|----------|------------------|
| multiple writers append to one file | use `O_APPEND` |
| child process should not inherit an FD | use `O_CLOEXEC` / `FD_CLOEXEC` |
| random-access reads in concurrent code | prefer `pread()` |
| one logical message is split across buffers | consider `writev()` |
| output must survive a crash before continuing | use `fsync()` or `fdatasync()` carefully |
| embedded state update | batch ordinary writes, sync only commit points, and check errors |

---

## Coverage Notes

| Coverage item | Status | Notes |
|---------------|--------|-------|
| 2.1 Universal I/O | Covered | `open/read/write/close`, FD model, EOF, partial I/O, error handling. |
| 2.2 Further details | Covered | `fcntl`, FD flags vs status flags, `dup*`, OFD sharing, `pread/pwrite`, scatter-gather. |
| 2.3 Buffering | Covered | stdio vs page cache, durability APIs, sync flags, direct I/O, embedded tradeoffs. |
| Must-cover race/debug workflow | Covered | append races, shared offsets, FD leaks, `strace`, `/proc/<PID>/fd`, `fdinfo`, `lsof`. |
| Embedded storage constraints | Covered | flash wear, power loss, slow sync, commit-point `fsync`, delayed errors, and mount/media checks. |

No remaining core-file coverage gap is known.

---

## Interview-Relevant Questions

- What is the difference between a file descriptor and an open file description?
- Why do two descriptors sometimes share a file offset?
- Why is `dup2()` important for shell redirection?
- Why can `write()` return before data reaches disk?
- What is the difference between `fflush()` and `fsync()`?
- Why is `pread()` safer than `lseek() + read()` in concurrent code?
- What problem does `O_APPEND` solve?
- Why should `close()` be checked on write-heavy paths?
- What is the difference between file status flags and FD flags?
- When would you use `writev()` instead of concatenating buffers manually?
- Why can `read()` or `write()` transfer fewer bytes than requested?
- What happens to file descriptors across `fork()` and `exec()`?
- Why is setting close-on-exec at `open()` time safer in multithreaded code?
- When is `O_SYNC` too expensive, and what design alternative would you consider?
- How would you debug a process that appears to leak file descriptors?

---

## Key Takeaways

- Linux low-level I/O is built around `open()`, `read()`, `write()`, and `close()`.
- A file descriptor is a per-process handle, not the file itself.
- The open file description holds the shared offset and file status flags.
- `dup()` and `fork()` often matter because they create shared references to the same open
  file description.
- `read()` and `write()` may complete partially; robust code must handle that.
- `lseek()` changes the shared offset of a seekable open file description.
- `pread()` and `pwrite()` perform offset-based I/O without disturbing that shared offset.
- `writev()` and `readv()` let one call operate across multiple buffers.
- stdio buffering and kernel buffering are different layers.
- `fflush()` flushes libc buffers; `fsync()` flushes kernel-managed file state toward storage.
- `O_APPEND`, `O_CLOEXEC`, and `O_EXCL` are not minor details; they solve real correctness and
  security problems.
