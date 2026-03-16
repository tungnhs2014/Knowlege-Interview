# History and Standards

> **Source:** TLPI Chapter 1
> **Why this matters:** Understanding where Linux came from explains *why* the API is designed the way it is,
> why two sets of IPC exist, and why `open()` on Linux looks the same as on macOS.

---

## 1. The Problem That Started Everything

In the early days of computing, every operating system had its own rules.
Code written for one machine **could not run on another**.

There was no shared standard. No portable API.

This was painful for developers, universities, and companies alike.
The entire history of UNIX and Linux is a story of people trying to solve this problem.

---

## 2. The Birth of UNIX (1969)

| Year | Event |
|------|-------|
| 1969 | Ken Thompson writes the first UNIX at Bell Labs — for a PDP-7, in assembly |
| 1973 | Dennis Ritchie designs the **C language** — UNIX is rewritten in C |
| 1974 | UNIX paper published in ACM journal — world starts paying attention |
| 1975+ | AT&T distributes UNIX source code to universities for nearly free |

**Why C mattered:**
Before C, operating systems were written in assembly — tied to one CPU architecture.
Writing UNIX in C meant it could be **compiled on any hardware**.
This is why C became the dominant systems programming language.

**Key insight:** UNIX was not designed by a committee. It was built by a few programmers, for themselves.
That is why it is small, consistent, and powerful.

---

## 3. The Fork: BSD vs System V

UNIX spread to universities. Developers added features. The code diverged.

### BSD (Berkeley Software Distribution)
- Born at UC Berkeley (1979+) — led by Ken Thompson as visiting professor
- Added: TCP/IP networking, sockets API, virtual memory, C shell, vi editor
- **4.2BSD (1983)** — first complete TCP/IP stack. This is the ancestor of the socket API you use today.
- Eventually became: FreeBSD, NetBSD, OpenBSD, macOS

### System V
- Born at AT&T (1983) — after AT&T was allowed to sell UNIX commercially
- System V Release 4 (SVR4, 1989) — merged many BSD features back in
- Added: System V IPC (msgget, shmget, semget) — the "older style" IPC
- Eventually became: Solaris, AIX, HP-UX

**Why this matters today:**
You will encounter **two sets of IPC APIs** in Linux:
- **System V IPC** — `msgget()`, `shmget()`, `semget()` — older, from AT&T
- **POSIX IPC** — `mq_open()`, `shm_open()`, `sem_open()` — newer, standardized

Both exist because Linux had to be compatible with both worlds.

---

## 4. The GNU Project (1984)

Richard Stallman at MIT had a vision: a completely **free** (as in freedom) UNIX-like operating system.

He started the **GNU Project** — "GNU's Not Unix".

GNU produced almost everything needed:
- `gcc` — C compiler
- `bash` — shell
- `glibc` — C standard library
- `make`, `sed`, `awk`, `emacs` — dozens of essential tools

**By 1990:** GNU had everything — except a working kernel.
The GNU kernel (HURD) was too ambitious and never finished.

---

## 5. Linux Kernel (1991)

Linus Torvalds, a Finnish student at the University of Helsinki, wanted a real UNIX for his Intel 386 PC.

He had studied **Minix** — a small teaching OS — but it was limited.

On **October 5, 1991**, Torvalds posted to the `comp.os.minix` newsgroup:

> *"I'm working on a free version of a Minix-look-alike for AT-386 computers... just version 0.02, but I've successfully run bash, gcc, gnu-make, gnu-sed under it."*

Other developers joined. Features were added fast:
- File system improvements
- Networking (TCP/IP)
- Device drivers
- Multi-processor support

**1994:** Linux 1.0 released.

**The combination that changed the world:**
```
GNU tools + Linux kernel = Complete free operating system
```

This is why Stallman calls it "GNU/Linux" — the kernel is Linux, but most of the system is GNU.

---

## 6. The Standardization Problem

By the late 1980s, there were many UNIX variants: BSD, System V, SunOS, AIX, HP-UX...

Each had slightly different APIs. A program written for BSD might not compile on System V.

This created strong pressure for a **single standard**.

### C Standard (1989 / 1999)
- **C89** (ANSI C) — first formal C standard, 1989
- **C99** — updated standard with new types, `//` comments, variable-length arrays

### POSIX
**POSIX** = Portable Operating System Interface

- Created by IEEE to standardize the UNIX API at the source code level
- **POSIX.1 (1988)** — core system calls: `fork()`, `open()`, `read()`, `write()`...
- **POSIX.1b (1993)** — realtime extensions: POSIX timers, POSIX IPC (semaphores, shared memory, message queues)
- **POSIX.1c (1995)** — POSIX threads (`pthread_create`, `pthread_mutex_lock`...)

### Single UNIX Specification (SUS)
- Produced by The Open Group — a higher-level standard built on top of POSIX
- **SUSv3 / POSIX.1-2001** — the major consolidation standard, still in wide use today
- **SUSv4 / POSIX.1-2008** — current standard

**Key insight:**
> POSIX does not require a UNIX kernel. Any OS that implements the POSIX API can be POSIX-conformant.
> Linux is POSIX-conformant but is **not** officially branded "UNIX" — because of certification costs.

---

## 7. Linux and Standards Today

Linux aims to conform to POSIX and SUSv3/SUSv4.

In practice:
- The Linux kernel + glibc implement the POSIX API
- Most `man 2` (system calls) and `man 3` (library functions) are POSIX-defined
- Extensions beyond POSIX (Linux-specific) are marked in man pages

**Linux Standard Base (LSB):**
Ensures that a compiled binary runs on any Linux distribution — binary compatibility across distros.

---

## 8. Mental Model — Why Things Are the Way They Are

```
1969  UNIX born at Bell Labs (C + portability idea)
        ↓
1979  BSD branch (TCP/IP, sockets)
        ↓
1983  System V branch (SysV IPC: msgget/shmget/semget)
        ↓
1984  GNU Project (free tools: gcc, glibc, bash)
        ↓
1988  POSIX.1 standard (portable API)
        ↓
1991  Linux kernel (Torvalds) + GNU tools = Linux OS
        ↓
1993  POSIX.1b (POSIX IPC: mq_open/shm_open/sem_open)
        ↓
2001  SUSv3 — major consolidation (the standard most code targets today)
```

---

## 9. What to Remember (Practical)

| Fact | Why it matters |
|------|---------------|
| UNIX was written in C for portability | Explains why C is the systems programming language |
| BSD added the socket API (1983) | `socket()`, `bind()`, `connect()` come from BSD, not AT&T |
| AT&T added System V IPC | `msgget()`, `shmget()`, `semget()` are the "old" way |
| POSIX standardized a clean IPC API | `mq_open()`, `shm_open()`, `sem_open()` are the "new" way |
| GNU + Linux kernel = Linux OS | Most tools (`gcc`, `bash`, `glibc`) are GNU, kernel is Linux |
| POSIX defines the API contract | Code written to POSIX runs on Linux, macOS, BSD — portably |
| Linux is not officially "UNIX" | But it behaves like UNIX in every practical sense |

---

## 10. Interview Questions

**Q: What is the difference between POSIX and Linux?**
> POSIX is a standard that defines what API an OS must provide. Linux is an OS that implements that standard. POSIX is the contract; Linux is one implementation of it.

**Q: Why does Linux have both System V IPC and POSIX IPC?**
> System V IPC came from AT&T UNIX (1983). POSIX IPC was standardized later with a cleaner API. Linux supports both for backward compatibility with old code, and because both are part of the POSIX/SUSv3 standard.

**Q: What is glibc?**
> The GNU C Library — implements the C standard library and the POSIX API on top of the Linux kernel's system calls. When you call `printf()` or `open()`, you go through glibc.
