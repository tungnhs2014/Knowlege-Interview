# Chapter 0 - History & Standards

> Topics: UNIX, C, BSD, System V, GNU, Linux kernel, POSIX, SUS, LSB
> Main sources: `docs/Linux-Programming-Interface/ch01_history_and_standards.md`; `LINUX_SYSTEM_LEARNING_MAP.md`; [The Open Group SUSv5 overview](https://www.unix.org/overview.html); [IEEE 1003.1-2024](https://standards.ieee.org/ieee/1003.1/7700/)
> Production context: This chapter explains why backend and embedded Linux code often targets POSIX APIs first, why Linux also exposes non-POSIX extensions, why BSD sockets and System V/POSIX IPC coexist, and how to read portability notes in man pages.

---

## Problem It Solves

Linux system programming looks strange at first:

- `read()` and `write()` work on files, pipes, terminals, sockets, and devices.
- C is still the core language for low-level Linux code.
- Linux has both System V IPC and POSIX IPC.
- Linux behaves like UNIX, but many distributions are not officially UNIX-branded.
- Some APIs are portable POSIX, while others are Linux-specific.

Chapter 0 is not about memorizing dates. It gives the reason behind the API shape you will use in later chapters.

The practical question is:

```text
When I write Linux code, which behavior is a stable standard contract,
and which behavior is Linux implementation detail?
```

If you can answer that, you can make better production decisions: write portable code when needed, use Linux-specific power when justified, and debug compatibility issues without guessing.

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | UNIX philosophy, C portability, GNU + Linux, POSIX vs Linux, BSD vs System V | Understand why Linux APIs look like traditional UNIX APIs |
| Work useful | How to read standards notes in `man` pages, POSIX source portability vs LSB binary compatibility, feature-test mindset | Decide whether code should be portable or Linux-specific |
| Recognize | SUS, XSI, LSB, legacy interfaces, weakly specified behavior, kernel release history | Recognize terms when reading TLPI, man pages, build docs, and vendor documentation |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| UNIX | Operating-system family and API tradition that began at Bell Labs | Linux is UNIX-like even though it is usually not officially UNIX-branded |
| UNIX-like | Behaves like classic UNIX without necessarily passing UNIX certification | Linux, FreeBSD, OpenBSD, and macOS are commonly discussed this way |
| Kernel | Privileged core that manages processes, files, memory, devices, and networking | The Linux kernel implements system calls such as `read()` and `fork()` |
| User space | Where normal programs and libraries run | Your C program and `glibc` run here |
| System call | Controlled entry from user space into the kernel | `open()`, `read()`, `write()`, `fork()` |
| C language | Systems programming language used to rewrite UNIX for portability | C made it realistic to move UNIX across CPU architectures |
| Portability | Ability to move code across systems with little or no source change | POSIX targets source portability |
| ABI | Binary-level calling convention and object format contract | ABI compatibility decides whether an already-compiled program can run |
| BSD | Berkeley UNIX branch known for TCP/IP, sockets, virtual memory, and many tools | The socket API comes from the BSD lineage |
| System V | AT&T commercial UNIX branch | System V IPC APIs include `msgget()`, `shmget()`, and `semget()` |
| GNU | Free-software project that built key UNIX-like user-space tools | `gcc`, `bash`, `make`, and `glibc` come from GNU |
| Linux | Strictly the kernel; commonly the full OS distribution | A distro combines the kernel, libraries, tools, init system, packages, and configuration |
| glibc | GNU C Library used by many Linux distributions | Wraps system calls and implements much of the C/POSIX library API |
| POSIX | Standard API contract for portable operating-system interfaces | Defines APIs such as `fork()`, `open()`, `pthread_*`, and POSIX IPC |
| SUS | Single UNIX Specification from The Open Group | A broader UNIX specification aligned with POSIX base volumes |
| XSI | X/Open System Interface option set | Adds interfaces required for UNIX branding, including System V IPC in SUSv3 context |
| LSB | Linux Standard Base | Focused on binary compatibility across Linux distributions, not general UNIX portability |
| Feature test macro | Macro that asks headers to expose a standard or extension set | `_POSIX_C_SOURCE`, `_GNU_SOURCE` |
| Linux-specific extension | API or behavior provided by Linux but not guaranteed by POSIX | `epoll`, `signalfd`, `timerfd`, many `/proc` details |
| Legacy interface | Standardized or supported mostly for compatibility with old code | Keep recognizing it; avoid it in new design when a better API exists |
| Source portability | Recompile the same source on another conforming OS | POSIX is mainly about this |
| Binary compatibility | Run the same compiled binary on compatible systems | LSB tried to address this inside Linux distributions |

---

## Concept Overview

The useful mental model is a layered stack:

```text
Application source code
        |
        v
C library / POSIX library API
        |
        v
Linux system calls and kernel behavior
        |
        v
Hardware, devices, file systems, network stack
```

Standards mainly describe the contract above the implementation:

```text
POSIX/SUS says: "A conforming system must provide this API behavior."
Linux says:     "Here is one implementation, plus extra Linux features."
glibc says:     "Here are C/POSIX library functions and syscall wrappers."
```

That distinction matters in real code:

- If your code uses `open()`, `read()`, `write()`, and `pthread_mutex_lock()`, it may be portable across POSIX-like systems.
- If your code uses `epoll`, `timerfd`, `signalfd`, or Linux-specific `/proc` details, it is intentionally Linux-specific.
- If your code depends on behavior not specified by POSIX, it may work on your distro and break elsewhere.

The big lesson:

```text
UNIX history explains the shape.
POSIX explains the portable contract.
Linux explains the concrete implementation and extensions.
```

---

## System Context

Chapter 0 connects directly to the rest of Linux system programming:

| Later topic | Why history/standards matter |
|-------------|------------------------------|
| File I/O | The "everything is a file descriptor" model comes from UNIX design |
| Processes | `fork()`, `exec()`, `wait()` are traditional UNIX process APIs standardized by POSIX |
| Signals | Signal semantics are old UNIX behavior with POSIX-defined pieces and Linux details |
| Threads | POSIX threads came later, so thread behavior is a standard layer over OS implementation |
| IPC | System V IPC and POSIX IPC both exist because standards and compatibility preserved both |
| Sockets | BSD introduced the sockets API; Linux implements it and POSIX/SUS standardized much of it |
| Memory mapping | `mmap()` is part of the UNIX/POSIX/SUS programming model, with Linux-specific extensions |
| Security | UID/GID, set-user-ID, capabilities, and permissions mix UNIX heritage with Linux additions |

Failure mode in production is usually not "you forgot history." It is more concrete:

- You use a Linux-specific API in code that must build on macOS or BSD.
- You rely on a glibc behavior that is not required by POSIX.
- You port old System V code and misunderstand how its IPC objects persist.
- You read a man page but ignore `STANDARDS`, `HISTORY`, or feature-test macro notes.
- You assume every Linux distribution ships the same kernel, libc, headers, or tool versions.

---

## Architecture

### 0.1 UNIX Lineage

```text
Bell Labs UNIX
    |
    +--> BSD lineage
    |       +--> TCP/IP stack
    |       +--> sockets API
    |       +--> modern BSD systems and macOS lineage
    |
    +--> System V lineage
            +--> commercial UNIX systems
            +--> System V IPC
```

This split explains why Linux inherited more than one style for similar problems.

### 0.2 Linux System Composition

```text
Linux distribution
    |
    +--> Linux kernel
    +--> C library, commonly glibc
    +--> GNU and non-GNU user-space tools
    +--> init/service manager
    +--> package manager
    +--> distribution patches and configuration
```

Strictly, Linux is the kernel. In daily conversation, "Linux" often means the full distribution.

### 0.3 Standards Stack

```text
C standard
    |
    +--> C language and standard C library

POSIX
    |
    +--> source-level OS API contract
    +--> system interfaces, shell, utilities, threads, realtime, IPC

SUS / XSI
    |
    +--> broader UNIX conformance profile
    +--> UNIX branding rules through The Open Group

Linux implementation
    |
    +--> POSIX-like behavior
    +--> Linux-specific APIs and kernel behavior
```

As of the 2024 revision, POSIX.1-2024 is IEEE Std 1003.1-2024 and The Open Group Base Specifications Issue 8; The Open Group describes this as part of the Single UNIX Specification Version 5. TLPI Chapter 1 discusses the older SUSv3/SUSv4 timeline because that was current for the book.

---

## Execution Flow

### Flow 1 - Why UNIX became portable

```text
OS code in assembly
    |
    v
Tied to one CPU architecture
    |
    v
UNIX rewritten mostly in C
    |
    v
Recompile for another machine
    |
    v
C becomes the natural systems language
```

### Flow 2 - Why standards became necessary

```text
Many UNIX variants
    |
    v
Different APIs and behavior
    |
    v
Porting applications becomes expensive
    |
    v
C standard + POSIX + SUS
    |
    v
Portable source-code contract
```

### Flow 3 - How Linux became a usable OS

```text
GNU tools exist
    |
    v
Missing piece: working kernel
    |
    v
Linux kernel appears
    |
    v
GNU tools + Linux kernel
    |
    v
Complete UNIX-like operating system distribution
```

### Flow 4 - How to choose an API in production

```text
Need a system feature
    |
    v
Check man page and standards notes
    |
    +--> POSIX API is enough?
    |       |
    |       v
    |   Prefer POSIX for portability
    |
    +--> Need Linux-only feature?
            |
            v
        Use Linux extension deliberately and document it
```

### Flow 5 - How a portability bug happens

```text
Code builds on Linux
    |
    v
Uses GNU or Linux extension implicitly
    |
    v
Build moves to another UNIX-like system or libc
    |
    v
Header/API/behavior differs
    |
    v
Fix by checking POSIX contract, feature macros, or adding compatibility layer
```

---

## 0.1 UNIX, C, and Portability

UNIX began as a small operating system built by programmers for programmers. Its lasting design ideas are more important than its release dates:

- hierarchical file system;
- shell as a user-space command interpreter;
- files as byte streams;
- small tools composed by pipes;
- process creation and program execution as core abstractions;
- device and file access through common interfaces.

The key technical turn was rewriting UNIX mostly in C. Before that, operating systems were commonly tied to a CPU through assembly language. C gave UNIX a practical path to portability: rewrite small hardware-dependent pieces, compile the rest.

For Linux programming today:

- C remains close to the kernel ABI and POSIX API.
- Most system call documentation is C-first.
- Many higher-level runtimes eventually call the same C/POSIX/Linux interfaces.

Use this as a mental model, not nostalgia:

```text
C did not become important because it is modern.
C became important because UNIX made it the practical language of portable systems code.
```

---

## 0.2 BSD vs System V

After early UNIX spread into universities and commercial vendors, development split into major branches.

| Branch | Main contribution for this course | What you will see later |
|--------|-----------------------------------|--------------------------|
| BSD | TCP/IP stack, sockets API, virtual memory work, many user tools | `socket()`, `bind()`, `connect()`, `accept()` |
| System V | Commercial UNIX lineage and System V IPC | `msgget()`, `shmget()`, `semget()`, `ipcs`, `ipcrm` |

Linux supports features from both worlds. This is why some Linux topics have two families of APIs.

Example:

| Problem | Older / compatibility style | Newer / POSIX style |
|---------|-----------------------------|----------------------|
| Message queue | System V `msgget()` | POSIX `mq_open()` |
| Shared memory | System V `shmget()` | POSIX `shm_open()` + `mmap()` |
| Semaphore | System V `semget()` | POSIX `sem_open()` or unnamed `sem_init()` |

Do not treat "older" as automatically useless. In production, old codebases, vendor SDKs, and embedded systems may still use System V IPC. The right skill is to recognize both and choose deliberately.

---

## 0.3 GNU, glibc, and the Linux Kernel

GNU built much of the user-space environment before the Linux kernel existed:

- compiler: `gcc`;
- shell: `bash`;
- C library: `glibc`;
- build and text tools: `make`, `sed`, `awk`, and many others.

The Linux kernel provided the missing kernel. Together, GNU user space plus the Linux kernel formed a complete UNIX-like system.

For a C program, the path often looks like this:

```text
your code
    |
    v
glibc function or wrapper
    |
    v
Linux system call
    |
    v
kernel subsystem
```

Important distinction:

| Call style | Example | Meaning |
|------------|---------|---------|
| C library function | `printf()` | Implemented in user-space library; may or may not enter kernel |
| POSIX library API | `pthread_mutex_lock()` | Standardized API, implemented by libc/thread library and kernel support |
| System call wrapper | `open()`, `read()` | C function that enters the kernel for the actual operation |
| Linux-specific API | `epoll_wait()` | Available on Linux; not portable POSIX |

In interviews and production debugging, be precise: `glibc` is not the kernel, and Linux is not just `bash` plus tools.

---

## 0.4 POSIX, SUS, and Linux

POSIX is a standard API contract. Linux is one implementation.

```text
POSIX: "What behavior should portable programs be able to rely on?"
Linux: "How this system actually implements behavior, plus extra features."
```

This distinction explains common man-page sections:

| Man-page clue | How to use it |
|---------------|---------------|
| `STANDARDS` | Tells whether an interface is POSIX, C standard, Linux-specific, etc. |
| `HISTORY` | Explains where the API came from and version caveats |
| `NOTES` | Often contains Linux/glibc differences and gotchas |
| `VERSIONS` | Shows kernel or libc version dependency |
| feature-test macros | Shows which macros expose declarations in headers |

Portable first-pass rule:

```text
If POSIX solves the problem cleanly, start there.
If Linux gives a major operational advantage, use Linux-specific APIs explicitly.
```

Examples:

| Need | Portable-ish choice | Linux-specific choice |
|------|---------------------|-----------------------|
| Basic file I/O | `open/read/write/close` | Linux flags or io_uring for special cases |
| Multiplex many FDs | `poll()` | `epoll` |
| Timer FD in event loop | POSIX timers may work conceptually | `timerfd` integrates with Linux FD polling |
| Signal as FD | POSIX signal APIs | `signalfd` |
| Process/system info | POSIX APIs where available | `/proc` details are Linux-specific |

---

## 0.5 LSB and Distribution Reality

POSIX focuses on source portability:

```text
same source code
    |
    v
compile on another POSIX system
```

LSB focused on binary compatibility across Linux distributions:

```text
same compiled binary
    |
    v
run on compatible Linux distributions on the same architecture
```

In practical Linux production, distribution differences still matter:

- kernel version and enabled config options;
- glibc vs musl;
- package versions;
- filesystem layout;
- service manager and boot model;
- vendor patches in enterprise distributions;
- embedded build systems such as Yocto or Buildroot.

So the production question is not only "Is this Linux?" It is:

```text
Which kernel, libc, architecture, distro policy, and runtime environment am I targeting?
```

---

## Work-Useful Patterns

### Pattern 1 - Read the man page like a contract

When using an unfamiliar API:

```text
man 2 open
man 3 pthread_create
man 7 feature_test_macros
```

Check:

- Is it POSIX, C standard, GNU, BSD, or Linux-specific?
- Which header and feature-test macro are required?
- Are there version notes for kernel or libc?
- Are errors and edge cases specified or implementation-dependent?

### Pattern 2 - Put portability decisions in code review

Good review comment:

```text
This uses epoll, so the component is Linux-only. That is acceptable because
the service runs only on Linux and needs scalable FD readiness.
```

Weak review comment:

```text
It compiles on my machine.
```

### Pattern 3 - Prefer POSIX vocabulary in first design

Before choosing a Linux extension, describe the problem with standard concepts:

- file descriptor;
- process;
- signal;
- thread;
- mutex;
- socket;
- shared memory;
- message queue.

Then decide whether Linux-specific behavior is worth the dependency.

### Pattern 4 - Treat legacy APIs as maintenance knowledge

System V IPC, old networking calls, and legacy time APIs can appear in inherited code. You should know what they are and how to debug them, but new code should usually choose the clearer modern API unless compatibility requires otherwise.

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| SUSv3 / SUSv4 / SUSv5 | Versions of the Single UNIX Specification; TLPI emphasizes SUSv3/SUSv4, while POSIX.1-2024 aligns with newer Issue 8 / SUSv5 material |
| XSI conformance | A stricter UNIX profile on top of POSIX baseline; relevant when docs discuss UNIX branding |
| X/Open, Austin Group | Organizations/processes behind POSIX/SUS consolidation |
| SVID | System V Interface Definition; useful when reading old UNIX compatibility notes |
| K&R C, C89, C99 | C standard history; useful for old C code and compiler flags |
| Kernel version numbering | Useful when docs say an API appeared in a specific Linux kernel |
| LSB | Linux binary compatibility effort; recognize the term, but modern portability work usually checks actual distro/libc/container constraints |
| Weakly specified behavior | Standard does not fully define details; avoid depending on it across platforms |
| LEGACY in standards | Kept for old applications; prefer replacement APIs in new code |

---

## Example

### Example 1 - Check whether an API is portable

Suppose you want to wait for readiness on many file descriptors.

```c
/*
 * poll() is standardized by POSIX.
 * epoll is Linux-specific and scales better for many FDs.
 */
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    struct pollfd pfd = {
        .fd = STDIN_FILENO,
        .events = POLLIN,
    };

    int ready = poll(&pfd, 1, 1000);
    if (ready < 0) {
        perror("poll");
        return 1;
    }

    if (ready == 0) {
        puts("timeout");
    } else if (pfd.revents & POLLIN) {
        puts("stdin is readable");
    }

    return 0;
}
```

Compile:

```sh
cc -Wall -Wextra -O2 poll_demo.c -o poll_demo
```

What it teaches:

- POSIX APIs are the safer first choice when portability matters.
- Linux-specific APIs are fine when you intentionally target Linux and need their behavior.
- The portability decision belongs in design, not after the code accidentally fails elsewhere.

### Example 2 - Feature-test macro mindset

Some declarations are exposed only when you request a standard or extension set.

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

int main(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        return 1;
    }

    printf("%ld.%09ld\n", (long)ts.tv_sec, ts.tv_nsec);
    return 0;
}
```

Compile:

```sh
cc -Wall -Wextra -O2 clock_demo.c -o clock_demo
```

What it teaches:

- Feature-test macros document which API surface your source expects.
- `_POSIX_C_SOURCE` asks headers for POSIX declarations.
- `_GNU_SOURCE` exposes GNU/Linux extensions and is less portable.

---

## Debugging

### Commands

| Task | Command | What to look for |
|------|---------|------------------|
| See kernel version | `uname -a` | Kernel release and architecture |
| See libc version | `ldd --version` | Often shows glibc version on glibc-based distros |
| Inspect syscall behavior | `strace -f ./program` | Which syscalls actually run |
| Read API contract | `man 2 open`, `man 3 pthread_create` | Standards, notes, errors, versions |
| Read feature macro rules | `man 7 feature_test_macros` | Which macro exposes which API |
| Check POSIX shell/tool behavior | `man 1 sh`, `man 1 getconf` | Utility behavior and limits |
| Query runtime limits | `getconf -a` | POSIX/system limits reported by the environment |
| Inspect linked libraries | `ldd ./program` | libc and dynamic library dependencies |
| Inspect binary ABI | `readelf -h ./program` | Architecture and ELF information |
| Check IPC leftovers | `ipcs`, `ipcrm` | System V IPC objects left by old programs |

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Code assumes GNU extension is POSIX | Build fails on musl, BSD, or macOS | Check `STANDARDS`; guard with feature macros or compatibility layer |
| Missing feature-test macro | Function or constant not declared | Read `man 7 feature_test_macros`; define `_POSIX_C_SOURCE` or appropriate macro before headers |
| Linux-specific API used in portable library | Consumers cannot build outside Linux | Document Linux-only support or provide portable fallback |
| Old System V IPC object remains after crash | Next run sees stale queue/shared memory/semaphore | Use `ipcs`; cleanup with `ipcrm`; design cleanup path |
| Binary built on one distro fails on another | Loader or symbol version error | Check `ldd`, glibc version, architecture, container/base image |
| Man page says behavior is unspecified | Works in testing, differs after port | Avoid depending on that behavior; use a specified API path |
| Confusing Linux kernel with distribution | Works on one distro, missing on another | Check kernel config/version, libc, package versions, and vendor patches |

---

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Embedded Linux product | Fix target kernel/libc/toolchain early; avoid accidental glibc-only assumptions if using musl/uClibc |
| Backend service only deployed on Linux | Use POSIX for simple parts, Linux-specific APIs such as `epoll` when they simplify operations or performance |
| Cross-platform CLI tool | Stay close to POSIX/C standard APIs; isolate Linux-specific code behind small adapters |
| Porting old UNIX code | Identify BSD vs System V assumptions; replace legacy APIs only when behavior is understood |
| Debugging vendor SDK | Check whether it uses System V IPC, `/proc`, fixed glibc assumptions, or old kernel APIs |
| Interview system design | Explain whether your design relies on portable POSIX semantics or Linux-only primitives |
| Containerized deployment | Remember containers share the host kernel; libc and user-space tools come from the image |

---

## Interview-Relevant Questions

1. What is the difference between UNIX, Linux, and POSIX?
2. Why is Linux called UNIX-like instead of simply UNIX?
3. Why did rewriting UNIX in C matter so much?
4. Why is C still common in Linux system programming?
5. What is the difference between the Linux kernel and a Linux distribution?
6. What role does `glibc` play between a C program and the kernel?
7. What is the difference between a system call and a C library function?
8. Why did UNIX variants create a need for POSIX?
9. What does POSIX standardize: source code, binaries, or kernel internals?
10. What is the difference between POSIX and the Single UNIX Specification?
11. Why does Linux support both System V IPC and POSIX IPC?
12. What major API family came from BSD UNIX?
13. Why can a program compile on Linux but fail on macOS or BSD?
14. What does it mean when a man page marks an API as Linux-specific?
15. When would you choose a Linux-specific API over a POSIX API?
16. What is a feature-test macro, and why can missing one break compilation?
17. What is the difference between source portability and binary compatibility?
18. What problem did the Linux Standard Base try to solve?
19. How do you check whether an API is POSIX-standardized?
20. Why are legacy APIs still present in modern Linux?
21. What does "unspecified behavior" mean in a standards document?
22. In production, why is "runs on Linux" less precise than naming kernel, libc, and distribution?
23. How would you explain GNU/Linux to someone who thinks Linux is the whole operating system?
24. Why do old UNIX design choices still affect sockets, processes, files, and IPC today?

---

## Key Takeaways

- Chapter 0 is context, not date memorization.
- UNIX gave Linux the core programming model: processes, files, byte streams, shells, pipes, and small composable tools.
- C became the systems language because it made UNIX portable across hardware.
- BSD and System V both shaped Linux; sockets come from BSD influence, while System V IPC remains for compatibility and standards history.
- GNU supplied much of the user-space system; Linux supplied the kernel.
- POSIX is the portable API contract; Linux is an implementation with extra features.
- SUS/XSI explains official UNIX conformance language; Linux is UNIX-like in practice but usually not UNIX-branded.
- POSIX portability is mostly source-code portability; LSB targeted Linux binary compatibility.
- In real work, always check `STANDARDS`, `NOTES`, `VERSIONS`, and feature-test macro requirements in man pages.
- Use POSIX when portability matters; use Linux-specific APIs deliberately when their production value is worth the dependency.
