# Linux Architecture — Kernel, Shell, Process, File Descriptor & System Calls

> **Source:** TLPI Ch02, Ch03 | DevLinux Module 01
> **Topics:** 1.1 Fundamental Concepts + 1.2 System Calls vs Library Functions

---

## Problem It Solves

When you run a program, call `open()`, or create a new process — something must:
- Decide which process gets the CPU
- Protect one process's memory from another
- Provide a unified interface to hardware (disk, network, GPIO...)
- Control who can do what

Without this layer of management, any program could corrupt another program's memory,
access hardware directly with no coordination, or crash the system.

**The kernel solves all of this.** Everything else (shell, apps, libraries) is built on top.

---

## Concept Overview

Linux has a layered architecture. Understanding these layers is the foundation
for everything in system programming.

```
┌─────────────────────────────────────────────────────────────┐
│                       USER SPACE                            │
│                                                             │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │    Shell    │  │System Utility│  │  Your Application│   │
│  │  (bash, sh) │  │(ls, grep, ps)│  │  (your_program)  │   │
│  └──────┬──────┘  └──────┬───────┘  └────────┬─────────┘   │
│         └────────────────┴───────────────────┘             │
│                          │  (function calls)               │
│              ┌───────────▼────────────┐                     │
│              │     System Libraries   │                     │
│              │   glibc — libc.so.6    │                     │
│              │  printf → write()      │                     │
│              │  fopen  → open()       │                     │
│              │  malloc → brk()/mmap() │                     │
│              └───────────┬────────────┘                     │
│                          │                                  │
├──────────────────────────┼──────────────────────────────────┤
│        SYSTEM CALL INTERFACE  (~300+ syscalls)              │
│   open() read() write() fork() mmap() socket() ...         │
├─────────────────────────────────────────────────────────────┤
│                      KERNEL SPACE                           │
│                                                             │
│  ┌──────────┐ ┌──────────┐ ┌─────────┐ ┌───────────────┐  │
│  │Scheduler │ │  Memory  │ │   VFS   │ │  Networking   │  │
│  │  (CPU)   │ │  Manager │ │(File IO)│ │   (TCP/IP)    │  │
│  └──────────┘ └──────────┘ └─────────┘ └───────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Device Drivers                          │  │
│  └──────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                        HARDWARE                             │
│           CPU    RAM    Disk    NIC    GPIO    UART          │
└─────────────────────────────────────────────────────────────┘
```

**Key correction from common misconception:**
Shell is NOT a special layer between libraries and kernel.
Shell, system utilities, and your application are ALL equal — they are all
user-space programs that use the same libraries and the same system call interface.

---

## System Context

### The Kernel

The kernel is the central software that manages all computer resources.
It runs continuously in the background and performs 6 core tasks:

| Task | Description |
|------|-------------|
| **Process scheduling** | Decides which process uses CPU, and for how long |
| **Memory management** | Allocates RAM, isolates memory between processes |
| **File system** | Manages files and directories on disk |
| **Device access** | Communicates with hardware on behalf of processes |
| **Networking** | Sends and receives network packets |
| **System call API** | Provides the interface for user space to request services |

The kernel resides at `/boot/vmlinuz` (a compressed executable).

### Kernel Mode vs User Mode

The CPU operates in two privilege modes:

```
USER MODE
  - Process can only access its own memory
  - Cannot execute privileged hardware instructions
  - Violation → Segmentation Fault

KERNEL MODE
  - No memory restrictions
  - Can execute any hardware instruction
  - Only kernel code runs here
```

This separation is enforced by **hardware** (the CPU itself), not software.
You cannot bypass it from user space.

### The Shell

The shell is an **ordinary user-space program** — not a special OS layer.
Its job: read user commands → find the corresponding executable → create a
new process to run it.

Common shells:
| Shell | Notes |
|-------|-------|
| `sh` | Bourne shell — original, available on every UNIX system |
| `bash` | GNU shell — default on most Linux distros |
| `zsh` | Popular today, default on macOS |
| `busybox ash` | Common on embedded Linux — very small footprint |

### Process

A process is a **running instance of a program**. The same binary (`ls`) can
produce multiple separate processes. Each process has:
- A unique **PID** (Process ID)
- Its own isolated **memory space**
- A list of **open file descriptors**
- Its own **environment variables**
- A state: running, sleeping, stopped, zombie

### Process Memory Layout

```
High address ┌──────────────────────┐
             │  Command args, env   │  argc, argv, environ
             ├──────────────────────┤
             │       Stack          │  local variables, function call frames
             │    (grows down ↓)    │  auto allocated/freed
             │                      │
             │         ↕            │  (gap — virtual memory)
             │                      │
             │       Heap           │  malloc/free — you manage this
             │    (grows up ↑)      │
             ├──────────────────────┤
             │    BSS segment       │  uninitialized global/static variables
             ├──────────────────────┤
             │    Data segment      │  initialized global/static variables
             ├──────────────────────┤
Low address  │    Text segment      │  executable code (read-only)
             └──────────────────────┘
```

### File Descriptor — "Everything is a File"

UNIX philosophy: every I/O resource is accessed through the same interface.
A **file descriptor (FD)** is a small integer that represents an open connection
to any I/O resource.

| FD | Name | Default |
|----|------|---------|
| 0 | `stdin` | Keyboard |
| 1 | `stdout` | Terminal |
| 2 | `stderr` | Terminal |
| 3+ | created by you | `open()`, `socket()`, `pipe()`... |

The same `read()`, `write()`, `close()` API works for:
- Regular files on disk
- Network sockets
- Pipes between processes
- Device files `/dev/ttyS0` (serial port on embedded)

### Subsystem Interactions

| User Space Component | Communicates With |
|----------------------|------------------|
| Shell, apps, utilities | System libraries (glibc) — via function calls |
| glibc | Kernel — via `syscall` CPU instruction |
| Kernel Scheduler | All subsystems — controls CPU time allocation |
| Kernel VFS | Filesystem drivers, device drivers |
| Device Drivers | Physical hardware (disk, NIC, GPIO, UART) |

### Failure Scenarios

| What Fails | What Happens |
|------------|-------------|
| Kernel bug / panic | System halts — no recovery possible from user space |
| Process accesses kernel-space address | CPU hardware trap → kernel sends **SIGSEGV** to the process |
| System call returns -1 | `errno` is set; caller must check and handle — no automatic rollback |
| glibc not found at program start | Dynamic linker returns `ENOENT` — program cannot execute |
| Stack grows past `ulimit -s` limit | Hardware stack guard page hit → kernel sends **SIGSEGV** |

---

## Architecture

```
USER SPACE
  ┌─────────┐  ┌──────────────┐  ┌─────────────────┐
  │  Shell  │  │System Utility│  │  Your Program   │   ← all equal peers
  └────┬────┘  └──────┬───────┘  └────────┬────────┘
       └───────────────┼──────────────────┘
                       │ library function calls
                       ▼
              ┌─────────────────┐
              │  glibc / libc   │   ← C standard library + syscall wrappers
              └────────┬────────┘
                       │ syscall instruction (CPU ring 3 → ring 0)
─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
           SYSTEM CALL INTERFACE  (~300 syscalls)
─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
                       │ (now in kernel mode)
                       ▼
KERNEL SPACE
  ┌───────────┐ ┌──────────┐ ┌─────┐ ┌────────────┐
  │ Scheduler │ │Memory Mgr│ │ VFS │ │ Networking │
  └───────────┘ └──────────┘ └──┬──┘ └────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │      Device Drivers      │
                    └────────────┬────────────┘
                                 │
                    HARDWARE (CPU, RAM, Disk, NIC)
```

The dashed boundary cannot be crossed without a `syscall` instruction —
enforced by CPU hardware rings (ring 3 = user, ring 0 = kernel).

---

## Internal Mechanism — System Calls

### What is a System Call?

A system call is the **only official gateway** from user space into the kernel.
When a process needs the kernel to do something — create a file, send data
over a network, allocate memory — it makes a system call.

### How a System Call Works (x86)

```
1. App calls open("file.txt", O_RDONLY)
         ↓
2. glibc wrapper function receives the call (still in USER SPACE)
         ↓
3. glibc copies arguments into CPU registers
   glibc puts syscall number into register EAX  (open = syscall #2)
   glibc executes CPU instruction: "syscall"
         ↓
4. ── CPU SWITCHES TO KERNEL MODE ──────────────────────────
         ↓
5. Kernel trap handler receives the interrupt
   Looks up: sys_call_table[2] → sys_open()
   Calls sys_open() — this is the actual kernel code
   sys_open() checks permissions, finds inode, allocates fd
   Returns fd (or -1 on error)
         ↓
6. ── CPU SWITCHES BACK TO USER MODE ───────────────────────
         ↓
7. glibc receives the return value → returns to your code
```

**Overhead:** Each system call costs ~0.3 microseconds (CPU mode switch).
Not much per call, but calling it 1 million times in a tight loop matters.

### Library Functions vs System Calls

These are fundamentally different. Knowing the difference is essential for debugging.

| Aspect | System Call | Library Function |
|--------|-------------|-----------------|
| Location | Executes in kernel | Executes in user space |
| CPU mode switch | Yes — expensive | No — just a function call |
| Count | ~300 on Linux | Thousands (in glibc) |
| Error reporting | returns `-1`, sets `errno` | varies per function |
| Examples | `open`, `read`, `write`, `fork` | `printf`, `fopen`, `malloc` |

### 3 Types of Library Functions

```
Type 1 — NEVER calls a syscall
  strlen(), memcpy(), abs(), sqrt()
  → Pure computation in user space, zero kernel involvement

Type 2 — Wraps exactly ONE syscall
  remove() → calls unlink() or rmdir()
  → Thin wrapper, just adds error handling

Type 3 — Calls MULTIPLE syscalls + adds logic
  printf()  → buffers data → write()
  fopen()   → open() + allocates FILE struct in heap
  malloc()  → manages heap pool → brk() or mmap()
```

**Important:** Saying "library function = syscall wrapper" is wrong.
Type 1 functions have nothing to do with the kernel.

### glibc — The Bridge

glibc (GNU C Library) is the most important library in Linux user space.
It implements two things:
1. The C standard library: `stdio.h`, `string.h`, `math.h`...
2. POSIX API wrappers on top of Linux kernel syscalls

On embedded systems, `glibc` is often replaced with lighter alternatives:
- **uClibc** — smaller, for resource-constrained devices
- **musl libc** — modern, very clean implementation

---

## Execution Flow — How a shell command runs

```
You type: ls /home

bash (shell) detects the command
        ↓
bash calls fork() syscall → kernel creates a child process
        ↓
child calls exec("/bin/ls", ["/home"]) syscall
        → kernel loads /bin/ls binary into memory
        → replaces child's memory completely
        ↓
ls runs:
  calls getdents() syscall → kernel reads directory entries
  calls write() syscall    → kernel sends output to terminal
        ↓
ls calls exit() → kernel cleans up process resources
        ↓
bash (parent) was waiting with wait() syscall
bash resumes, shows prompt again
```

At no point does `ls` talk back through bash to reach the kernel.
Each process talks **directly** to the kernel via its own system calls.

---

## Example

### Reading a File Using System Calls

```c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(void) {
    /* syscall: open() — user→kernel transition */
    int fd = open("hello.txt", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    char buf[128];
    /* syscall: read() */
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        write(STDOUT_FILENO, buf, n);  /* syscall: write() */
    }

    close(fd);   /* syscall: close() */
    return 0;
}
```

**Verify with strace — see every syscall the process makes:**
```bash
$ strace ./a.out
openat(AT_FDCWD, "hello.txt", O_RDONLY) = 3   ← fd=3 returned
read(3, "hello world\n", 127)           = 12  ← 12 bytes read
write(1, "hello world\n", 12)           = 12  ← written to stdout
close(3)                                = 0
exit_group(0)                           = ?
```

Each line = one kernel entry = one user→kernel privilege transition.

---

## Debugging

### System Call Errors

```c
int fd = open("file.txt", O_RDONLY);
if (fd == -1) {
    perror("open");    // prints: "open: No such file or directory"
    exit(EXIT_FAILURE);
}
```

`errno` is set by the kernel when a syscall fails.
**Always save errno immediately** — the next function call may overwrite it.

```c
// WRONG
open("a.txt", O_RDONLY);
open("b.txt", O_RDONLY);    // errno from first open is GONE
fprintf(stderr, "%s\n", strerror(errno));  // shows error from second open only

// CORRECT
int fd = open("a.txt", O_RDONLY);
if (fd == -1) {
    int saved_errno = errno;  // save immediately
    // ... handle error using saved_errno
}
```

### Library Function Errors

Each library function has its own error convention — always check the man page:

```c
FILE *fp = fopen("file.txt", "r");
if (fp == NULL) { ... }             // NULL on error

char *p = malloc(100);
if (p == NULL) { ... }              // NULL on error

size_t n = fread(buf, 1, 100, fp);
if (ferror(fp)) { ... }             // use ferror(), not return value
```

---

## Real-world Usage

### When to bypass library functions and use syscalls directly

In embedded and system programming, there are cases where you need to call
syscalls directly instead of using library functions:

| Situation | Why bypass library function |
|-----------|----------------------------|
| Writing crash logs | `printf` has a buffer — if program crashes, buffer is lost. Use `write()` directly. |
| Serial port / GPIO timing | `fwrite` buffering disrupts precise timing. Use `write()` for immediate output. |
| After `fork()` in child | stdio buffers are duplicated — use `write()` to avoid double output. |
| Hard realtime tasks | Any unpredictable buffering overhead is unacceptable. |

### Bypassing glibc entirely

```c
#include <sys/syscall.h>
#include <unistd.h>

// Call open() syscall directly, bypassing glibc wrapper
long fd = syscall(SYS_open, "file.txt", O_RDONLY, 0);
```

This is rare in practice but exists for cross-compilation toolchains
where full glibc is not available.

---

## Key Takeaways

```
KERNEL     = resource manager — cannot be bypassed
SHELL      = an ordinary user-space program, NOT a special layer
PROCESS    = running program instance — has isolated memory
FD         = integer handle for any I/O resource (file/socket/pipe/device)
SYSCALL    = the only gateway into the kernel (~300 available)
glibc      = bridge layer: wraps syscalls + provides C standard library

MOST IMPORTANT RULE:
  Everything your program does that touches hardware, files, or other
  processes goes through a system call — always.
```
