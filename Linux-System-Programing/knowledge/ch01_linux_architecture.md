# Chapter 1 - Linux Architecture and System Calls

> Topics: 1.1 Fundamental Concepts - Kernel, Shell, Process, FD; 1.2 System Calls vs Library Functions
> Main sources: TLPI Ch02, Ch03; DevLinux Module 01
> Production context: backend services, embedded daemons, CLI tools, and debugging sessions where a program must interact with files, devices, sockets, processes, and the kernel safely.

---

## Problem It Solves

A C program cannot safely talk to hardware, schedule itself, read another process's memory, or bypass file permissions by itself. If every program could do that directly, one bug could corrupt the whole machine.

Linux solves this by putting privileged control in the kernel. User-space programs run with limited privilege and ask the kernel to do system work through system calls.

The foundation question for this chapter is:

> When my code calls `open()`, `read()`, `printf()`, or starts another program, which layer is actually doing the work?

Once that is clear, later topics such as file I/O, process creation, signals, sockets, and debugging become much easier.

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | kernel vs shell, user space vs kernel space, process, file descriptor, system call, library function | Explain what happens when a program asks Linux to do work. |
| Work useful | return-value checks, `errno`, stdio buffering, `strace`, build/link/runtime distinction | Debug failures and avoid basic production mistakes. |
| Recognize | feature-test macros, glibc wrappers, syscall entry mechanism, static vs shared libraries | Know what these mean when reading docs, traces, or build output. |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Kernel | Privileged core of the OS that manages CPU, memory, files, devices, networking, and processes. | The kernel decides whether `open()` is allowed. |
| User space | Memory and CPU mode where ordinary programs run. | Shells, services, and CLI tools run here. |
| Kernel space | Protected memory and CPU mode where the kernel runs. | User code cannot directly read kernel memory. |
| Shell | User-space command interpreter that starts programs. | `bash` parses `ls -l` and launches `ls`. |
| Process | A running program instance with PID, memory, credentials, FDs, and execution state. | One executable can have many running processes. |
| PID | Numeric process identifier assigned by the kernel. | Used by `ps`, `kill`, `/proc/<PID>`. |
| File descriptor | Small integer in a process that refers to an open I/O object. | `0`, `1`, `2` are stdin, stdout, stderr. |
| System call | Controlled entry from user space into the kernel. | `read()`, `write()`, `fork()`, `execve()`. |
| libc / glibc | C library used by most Linux programs; provides wrappers and higher-level functions. | `printf()` and `fopen()` are library functions. |
| Syscall wrapper | libc function that prepares arguments, enters kernel mode, and maps kernel errors to `errno`. | Calling `open()` in C normally calls a wrapper. |
| `errno` | Per-thread error indicator used after many failed syscalls/library calls. | Check it only after return value indicates failure. |
| Stdio | Buffered C I/O layer above low-level file descriptors. | `printf()` may not call `write()` immediately. |
| Executable | Binary file produced by compile/link steps and loaded when a program runs. | Usually ELF on Linux. |
| Dynamic linker | Runtime component that loads shared libraries needed by an executable. | Seen via `ldd ./program`. |

---

## Concept Overview

The most useful mental model is layered:

```text
your C source
    |
    v
compiler + linker
    |
    v
executable file
    |
    v
process in user space
    |
    v
libc / glibc
    |
    v
system call interface
    |
    v
kernel subsystems
    |
    v
hardware
```

The shell is not the kernel. It is one user-space program that starts other programs. Your service, a shell, and tools such as `ls` all use the same kernel interface.

The kernel exposes many resources through file descriptors. That is why files, pipes, sockets, terminals, and many devices can be handled with similar low-level APIs.

---

## System Context

Linux system programming sits at the boundary between user-space code and kernel-managed resources.

```text
User-space programs
    |
    +-- shell
    +-- service / daemon
    +-- CLI utility
    |
    v
libc / runtime
    |
    v
system calls
    |
    v
kernel
    +-- scheduler
    +-- memory manager
    +-- VFS
    +-- networking stack
    +-- device drivers
```

If this boundary is misunderstood, common bugs become confusing:

| Symptom | Real cause to consider |
|---------|------------------------|
| Output appears late | stdio buffered data before syscall. |
| `Permission denied` | kernel permission check failed. |
| Program works in shell but not service | different environment, cwd, credentials, FDs, or limits. |
| `strace` shows fewer writes than expected | library buffering combined multiple user-space calls. |

---

## Architecture

### Kernel Responsibilities

| Responsibility | What it means in practice |
|----------------|---------------------------|
| Process scheduling | The kernel decides which runnable process/thread gets CPU time. |
| Memory management | Each process gets an isolated virtual address space. |
| File system and VFS | Pathnames and FDs are mapped to files, devices, pipes, sockets, and more. |
| Device access | Drivers mediate hardware access. |
| Networking | Packets are sent and received on behalf of processes. |
| System call API | User-space programs request privileged work through controlled entry points. |

### User Mode vs Kernel Mode

Modern CPUs enforce privilege levels:

| Mode | Used by | Can do |
|------|---------|--------|
| User mode | normal application code | access its own user-space memory and call library code |
| Kernel mode | kernel code | access protected kernel memory and perform privileged operations |

The exact syscall entry instruction depends on architecture and kernel/libc implementation. The first-pass lesson is simple: a syscall crosses a hardware-enforced privilege boundary.

### File Descriptor Model

```text
process fd table
    0 -> stdin
    1 -> stdout
    2 -> stderr
    3 -> open file / socket / pipe / device
```

A file descriptor is not "a file on disk." It is a process-local handle to an open I/O object managed by the kernel.

---

## Execution Flow

### Flow 1 - Command Becomes a Process

```text
user types command
    |
    v
shell parses command
    |
    v
shell creates/execs target program
    |
    v
kernel loads executable and sets process state
    |
    v
program runs in user space
```

### Flow 2 - C Source Becomes an Executable

```text
source.c
    |
    v
preprocess: headers/macros
    |
    v
compile: C -> assembly
    |
    v
assemble: assembly -> object file
    |
    v
link: objects + libraries -> executable
```

This DevLinux build-flow knowledge matters because runtime behavior depends on what was linked, which libraries are loaded, and which symbols are resolved.

### Flow 3 - `open()` Crosses into the Kernel

```text
program calls open()
    |
    v
glibc wrapper prepares syscall arguments
    |
    v
CPU enters kernel mode
    |
    v
kernel validates pathname, credentials, flags, limits
    |
    v
kernel returns fd or error
    |
    v
wrapper returns fd or -1 and sets errno
```

### Flow 4 - `printf()` vs `write()`

```text
printf("x")
    |
    v
stdio formats/buffers in user space
    |
    v
eventually calls write()
    |
    v
kernel writes bytes to fd
```

`write()` is low-level byte I/O. `printf()` is a library abstraction with formatting and buffering.

### Flow 5 - Failure Path

```text
syscall cannot complete
    |
    v
kernel returns negative error internally
    |
    v
libc wrapper sets errno
    |
    v
program sees -1 / NULL / failure status
    |
    v
program must handle or report it
```

---

## 1.1 Fundamental Concepts

### Kernel

The kernel is the privileged resource manager. It does not exist to make C code pretty; it exists to isolate, arbitrate, and protect shared resources.

Must know:

- a process requests kernel work;
- the kernel validates the request;
- the kernel performs the operation or denies it.

### Shell

The shell reads commands and starts programs. It is important for interactive work, but it is not a magical OS layer. A shell script, a backend service, and a C program are all user-space code.

### Process

A process is the execution container the kernel manages. It has:

- PID and parent PID;
- virtual memory;
- file descriptors;
- user/group credentials;
- current working directory;
- environment variables;
- signal state.

Many later chapters expand these fields. For Chapter 1, remember that a process is the unit that asks the kernel for work.

### File Descriptor

File descriptors are the first big UNIX/Linux unifying idea:

```text
same API shape
    |
    +-- regular file
    +-- pipe
    +-- socket
    +-- terminal
    +-- device file
```

This is why FD leaks, FD inheritance, and FD limits become production issues later.

---

## 1.2 System Calls vs Library Functions

| Kind | Runs where | Example | Practical note |
|------|------------|---------|----------------|
| System call | enters kernel | `read()`, `write()`, `fork()` | Crosses privilege boundary; usually visible in `strace`. |
| Thin libc wrapper | user space + syscall | `close()` | Looks like a function call but invokes kernel work. |
| Higher-level libc function | mostly user space, may syscall | `printf()`, `fopen()` | Adds buffering, formatting, allocation, or convenience. |
| Pure library helper | user space only | `strlen()` | No kernel transition. |

Why this matters:

- syscalls have overhead compared with plain function calls;
- library buffering can change when bytes actually reach the kernel;
- error conventions differ across library functions;
- portability depends on standards, feature-test macros, and libc behavior.

Basic error rule:

```text
call function
    |
    v
check documented failure return
    |
    v
only then inspect errno if that API uses errno
```

Do not read `errno` after a successful call and assume it means anything.

---

## Work-Useful Patterns

| Pattern | Why it matters |
|---------|----------------|
| Check every syscall that can fail | Missing one `-1` check often hides the real bug. |
| Print syscall failures with context | `perror("open config")` is more useful than a silent return. |
| Use `strace` when behavior disagrees with expectation | It shows kernel-facing calls, not every library call. |
| Treat FD `0/1/2` as ordinary descriptors | Redirection, services, and tests often replace them. |
| Flush stdio before expecting output timing | Buffered data may still be in user space. |
| Separate build-time and runtime questions | A program can compile successfully but fail because a shared library or file is missing at runtime. |
| Document Linux-specific assumptions | `/proc`, many flags, and some behaviors are Linux-specific, not portable POSIX guarantees. |

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Syscall numbers | The kernel identifies syscalls numerically; C code normally uses libc wrappers by name. |
| `int 0x80`, `sysenter`, `syscall` | Architecture-specific mechanisms for entering kernel mode. Recognize the terms; do not memorize them first-pass. |
| Feature-test macros | Macros such as `_GNU_SOURCE` expose nonstandard APIs. Use deliberately. |
| glibc version | Can affect wrapper behavior and available APIs. Rarely the first thing to debug. |
| Static vs shared linking | Static copies library code into the executable; shared linking loads `.so` files at runtime. |
| Makefile automation | Useful for builds, but not the core Linux runtime model. |

---

## Example

### Example 1 - Write to stdout through a file descriptor

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int write_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        buf += n;
        len -= (size_t)n;
    }

    return 0;
}

int main(void) {
    const char msg[] = "hello through fd 1\n";

    if (write_all(STDOUT_FILENO, msg, strlen(msg)) == -1) {
        perror("write");
        return 1;
    }

    return 0;
}
```

What it teaches:

- `STDOUT_FILENO` is fd `1`;
- low-level I/O sends bytes to a kernel-managed object;
- production code should handle interruption and partial writes.

### Example 2 - Show normal syscall error handling

```c
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd = open("/definitely/not/here", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}
```

What it teaches:

- failure is normal in system programming;
- return value first, `errno` only after documented failure.

### Example 3 - `printf()` is a library abstraction

```c
#include <stdio.h>

int main(void) {
    printf("stdio formats and may buffer output before write()\n");

    if (fflush(stdout) == EOF) {
        perror("fflush");
        return 1;
    }

    return 0;
}
```

What it teaches:

- `printf()` is not the same layer as `write()`;
- flushing is explicit when output timing matters.

---

## Debugging

Useful commands:

```bash
# Trace kernel-facing calls
strace -f ./program

# See open file descriptors of this shell process
ls -la /proc/self/fd

# See a running process's executable, cwd, and FDs
ls -la /proc/<PID>/{exe,cwd,fd}

# Show dynamic library dependencies
ldd ./program

# Inspect symbols in an executable or object file
nm ./program 2>/dev/null | head

# Inspect memory mappings
cat /proc/<PID>/maps
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Treating shell as kernel | Wrong assumption about where commands run | Remember shell is just a process. |
| Ignoring return values | Later crash or misleading output | Check every syscall/library failure contract. |
| Misreading `errno` | Error message from an older call | Read `errno` only after a failure return. |
| Confusing `printf()` and `write()` | Output appears late or out of order | Understand stdio buffering; use `fflush()` when needed. |
| Assuming FD means disk file | Socket/pipe behavior surprises | Inspect `/proc/<PID>/fd` and use FD mental model. |
| Missing shared library at runtime | Program starts with loader error | Use `ldd`, RPATH, or correct deployment. |

---

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Backend service cannot read config | Check cwd, credentials, path, permissions, and `open()` failure with `strace`. |
| Embedded daemon writes logs to stdout/stderr | Treat fd `1/2` as runtime-provided handles, often captured by init/systemd. |
| CLI pipeline behaves strangely | Remember shell connects processes using FDs and pipes. |
| Performance investigation | Distinguish many small syscalls from buffered user-space work. |
| Cross-distro build issue | Separate compile/link errors from runtime library loading errors. |

---

## Interview-Relevant Questions

- What is the difference between the kernel and the shell?
- Why do we need user mode and kernel mode?
- What is a process?
- What does a file descriptor represent?
- Why can Linux use similar I/O calls for files, pipes, sockets, and devices?
- What is a system call?
- How is a libc function different from a system call?
- What happens when `open()` succeeds? What happens when it fails?
- Why should `errno` be checked only after a failure return?
- Why can `printf()` produce different timing than `write()`?
- What does `strace` show, and what does it not show?
- Why do system calls have more overhead than ordinary C function calls?
- What is the role of the dynamic linker?
- What does the shell do when you type a command?
- Why does understanding FDs matter for production debugging?

---

## Key Takeaways

- The kernel is the privileged manager of CPU, memory, files, devices, networking, and processes.
- The shell is a user-space command interpreter, not the OS core.
- A process is a running program with kernel-tracked state.
- A file descriptor is a process-local handle to an open I/O object.
- System calls are controlled entries into the kernel.
- libc functions may be pure user-space helpers, syscall wrappers, or higher-level abstractions.
- `printf()` adds formatting and buffering; `write()` writes bytes to an FD.
- Always check the documented failure return before inspecting `errno`.
- `strace` is the first practical tool for seeing user/kernel interaction.
- Build-time success does not guarantee runtime success.
- Linux-specific details should be documented when portability matters.
- Chapter 1 is the mental model used by every later chapter.
