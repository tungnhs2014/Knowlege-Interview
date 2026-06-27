# Chapter 0 Interview - History & Standards

> Scope: UNIX lineage, GNU/Linux composition, POSIX/SUS, BSD vs System V influence, libc/kernel boundaries, portability, feature-test macros, and production compatibility checks.
> Primary repo sources: `knowledge/ch00_history_and_standards.md`.
> Supporting repo sources: TLPI-derived doc `docs/Linux-Programming-Interface/ch01_history_and_standards.md`; `LINUX_SYSTEM_LEARNING_MAP.md`.

---

## Review Basis

This interview set is filtered from Chapter 0 knowledge and TLPI Chapter 1. Chapter 0 is context-heavy, so the useful interview target is not memorizing dates. The useful target is explaining why Linux system APIs look the way they do and how to make portability decisions in real projects.

Official correctness checks used for standards/version wording:

- The Open Group Single UNIX Specification V5 overview: <https://www.unix.org/overview.html>
- IEEE POSIX.1-2024 page: <https://standards.ieee.org/ieee/1003.1/7700/>
- Linux man-pages `standards(7)`: <https://www.man7.org/linux/man-pages/man7/standards.7.html>
- Linux man-pages `feature_test_macros(7)`: <https://www.man7.org/linux/man-pages/man7/feature_test_macros.7.html>

No DevLinux source is mapped for Chapter 0.

---

## Cách Ưu Tiên

| Priority | Cách học | Chủ đề |
|----------|----------|--------|
| A - Chắc / rất hay phỏng vấn | Phải trả lời được bằng cơ chế, ví dụ, lỗi production, và cách debug. | UNIX vs Linux vs POSIX, kernel vs distribution, system call vs libc function, `glibc`, POSIX source portability, Linux-specific API choice, feature-test macros, BSD/System V legacy, distro/libc/runtime differences |
| B - Hay hỏi nếu role system/senior | Hiểu đúng trade-off và đọc tài liệu chuẩn được; không cần thuộc timeline. | C and UNIX portability, GNU/Linux naming, SUS/XSI/UNIX branding, legacy/unspecified behavior, man-page contract reading, binary compatibility |
| C - Ít phỏng vấn / biết là đủ | Nhận ra thuật ngữ khi đọc docs hoặc maintain legacy code. | exact UNIX release dates, XPG, FIPS, SVID details, full SUS version timeline, LSB history, old kernel version numbering, K&R C/C89/C99 timeline |

---

## Danh Sách Đã Lọc

### Câu chắc hay hỏi

1. What is the difference between UNIX, Linux, and POSIX?
2. Why is Linux called UNIX-like instead of simply UNIX?
3. What is the difference between the Linux kernel and a Linux distribution?
4. What role does `glibc` play between a C program and the Linux kernel?
5. What is the difference between a system call and a C library function?
6. What does POSIX standardize: source code, binaries, or kernel internals?
7. Why can code compile on Linux but fail on macOS, BSD, or musl-based Linux?
8. When would you choose a Linux-specific API over a POSIX API?
9. What is a feature-test macro, and why can a missing one break compilation?
10. Why does Linux support both BSD-influenced APIs and System V/POSIX IPC families?
11. In production, why is saying "it runs on Linux" less precise than naming kernel, libc, architecture, distribution, and runtime environment?

### Câu ít hỏi nhưng vẫn đáng biết

12. Why did rewriting UNIX mostly in C matter for portability?
13. Why is C still common in Linux system programming?
14. What did GNU provide, and what did the Linux kernel provide?
15. What is the difference between POSIX, SUS, and XSI?
16. What does it mean when a standards document or man page says behavior is unspecified, weakly specified, obsolete, or legacy?
17. How do you check whether an API is POSIX-standardized, Linux-specific, GNU-specific, or version-dependent?
18. What is the difference between POSIX source portability and Linux binary compatibility?

### Câu recognize only

- Exact dates for early UNIX editions, BSD releases, System V releases, GNU releases, and Linux kernel releases.
- Full standards timeline: XPG3, XPG4, XPG4v2, SUSv1, SUSv2, SUSv3, SUSv4, SUSv5.
- FIPS 151, SVID issue numbers, Spec 1170, and old UNIX branding labels such as UNIX 95/98/03.
- Old Linux kernel version numbering rules and development/stable branch history.
- Details of every historical distribution such as MCC Interim Linux, TAMU, SLS, Slackware, Debian, Red Hat, SUSE, and Ubuntu.

Filtered out: date trivia, long lists of historical releases, names of early UNIX utilities, full standards genealogy, and API-family dumps that do not test system-programming judgment.

---

## Các Cặp So Sánh Hay Bị Hỏi Xoáy

| Comparison | Interview answer |
|------------|------------------|
| UNIX vs Linux vs POSIX | UNIX is a lineage and trademarked conformance family; Linux is a UNIX-like kernel commonly used in full distributions; POSIX is a portable API contract. |
| Linux kernel vs Linux distribution | The kernel manages processes, memory, files, devices, and networking. A distribution adds libc, tools, init, package policy, patches, and configuration. |
| `glibc` vs kernel | `glibc` is user-space library code and syscall wrappers. The kernel is the privileged implementation behind system calls. |
| System call vs C library function | A system call enters the kernel. A C library function may run entirely in user space or may call a syscall internally. |
| POSIX API vs Linux-specific API | POSIX favors source portability. Linux-specific APIs can give stronger operational value on Linux but create an explicit portability dependency. |
| Source portability vs binary compatibility | Source portability means recompile the same source on another conforming system. Binary compatibility means an already-built binary runs on compatible systems. |
| BSD influence vs System V influence | BSD explains sockets and TCP/IP heritage. System V explains older IPC APIs such as message queues, semaphores, and shared memory. |
| System V IPC vs POSIX IPC | System V IPC is older and common in legacy systems. POSIX IPC is usually clearer for new portable-ish code, but both may appear in Linux. |
| POSIX vs SUS/XSI | POSIX is the baseline portable interface. SUS/XSI adds UNIX conformance profile requirements and UNIX branding context. |
| `_POSIX_C_SOURCE` vs `_GNU_SOURCE` | `_POSIX_C_SOURCE` requests POSIX declarations. `_GNU_SOURCE` exposes GNU/Linux extensions and reduces portability. |
| `STANDARDS` vs `NOTES` vs `VERSIONS` in man pages | `STANDARDS` tells portability status; `NOTES` captures implementation gotchas; `VERSIONS` shows kernel/libc dependency. |

---

## Đáp Án

### Priority A - Trả lời đầy đủ

#### 1. What is the difference between UNIX, Linux, and POSIX?

UNIX is both a historical operating-system family and, in the strict trademark sense, a label for systems certified against the Single UNIX Specification. In day-to-day engineering, people also use "UNIX" more loosely for systems that follow the classic UNIX model: hierarchical file system, processes, byte-stream files, shells, pipes, permissions, and small composable tools.

Linux is a UNIX-like implementation. Strictly, Linux is the kernel. In normal speech, "Linux" often means a full operating system distribution containing the Linux kernel plus libc, tools, services, package policy, and configuration.

POSIX is not an operating system. POSIX is a standard API contract for portable source code: functions, headers, shell behavior, utilities, threads, IPC, and other interfaces that conforming systems should provide.

Pitfall: saying "Linux is POSIX" is sloppy. Linux aims to support many POSIX interfaces, but it also has Linux-specific behavior and APIs. Production angle: when portability matters, check whether your dependency is POSIX, GNU, BSD, or Linux-specific before assuming it will build on another platform.

#### 2. Why is Linux called UNIX-like instead of simply UNIX?

Linux follows the UNIX programming model, but most Linux distributions are not certified to use the UNIX trademark. The strict "UNIX" label depends on conformance testing and branding through The Open Group.

Mechanically, Linux behaves like UNIX in the interfaces programmers use: processes, file descriptors, permissions, signals, sockets, pipes, and the C/POSIX API surface. That is why TLPI and man pages often discuss Linux beside other UNIX implementations.

Pitfall: do not use "not officially UNIX" to mean "not compatible." For system programming, the real question is which specific API and behavior your code relies on. Debug angle: when behavior differs, compare the Linux man page, POSIX wording, and target platform documentation instead of relying on the label "UNIX-like."

#### 3. What is the difference between the Linux kernel and a Linux distribution?

The Linux kernel is the privileged core. It implements system calls and manages processes, virtual memory, file systems, devices, networking, credentials, signals, scheduling, and resource accounting.

A Linux distribution is the usable operating environment around the kernel. It includes a C library such as `glibc` or musl, shells, GNU and non-GNU utilities, init/service manager, package manager, default configuration, kernel patches, security policy, and filesystem layout.

Pitfall: "works on Linux" may hide distro-specific assumptions. The same source or binary may behave differently across glibc vs musl, different kernel versions, enterprise vendor patches, container images, or embedded systems built with Yocto/Buildroot.

Production/debug angle: record `uname -a`, libc version, architecture, container image/base distro, package versions, and kernel config when debugging compatibility bugs.

#### 4. What role does `glibc` play between a C program and the Linux kernel?

`glibc` is a user-space C library used by many Linux distributions. It provides standard C functions, POSIX library APIs, and wrappers around Linux system calls.

The path is often:

```text
application code -> glibc function/wrapper -> Linux syscall -> kernel subsystem
```

Not every library function enters the kernel. `strlen()` is pure user-space work. `printf()` may buffer in user space and only eventually call `write()`. `open()` and `read()` are libc functions that usually wrap syscalls with the same conceptual operation.

Pitfall: debugging only at the C function level may miss the real syscalls; debugging only at syscall level may miss libc buffering, feature-test macros, or wrapper behavior. Production angle: use `strace` to see kernel entries, but inspect libc and headers when the symptom is missing declarations, ABI, buffering, or symbol-version problems.

#### 5. What is the difference between a system call and a C library function?

A system call is a controlled transition from user space into the kernel. It asks the kernel to perform privileged work such as file I/O, process creation, memory mapping, networking, or signal operations.

A C library function is a function provided by libc. It may be a wrapper around a syscall, a pure user-space function, or a higher-level function that performs buffering, conversion, locking, or multiple syscalls.

Examples:

| Function | Category |
|----------|----------|
| `read()` | libc wrapper around a kernel syscall |
| `printf()` | C library function with user-space formatting/buffering; may later call `write()` |
| `pthread_mutex_lock()` | POSIX library API implemented by libc/thread library with kernel help in contended paths |

Pitfall: assuming every C API is a syscall leads to wrong performance and debugging conclusions. Debug angle: `man 2` usually documents system calls, `man 3` documents library functions, and `strace` shows actual syscalls.

#### 6. What does POSIX standardize: source code, binaries, or kernel internals?

POSIX mainly standardizes source-level interfaces and expected behavior visible to applications. It describes headers, functions, shell behavior, utilities, and semantic contracts that portable programs can rely on.

POSIX does not require a particular kernel architecture, file-system implementation, scheduler implementation, or syscall table. It also does not guarantee that one compiled binary runs everywhere.

That distinction matters:

```text
POSIX portability: same source can be recompiled on another conforming system.
Binary compatibility: same compiled binary runs on compatible systems.
```

Pitfall: a POSIX function can still have unspecified or optional details. Production angle: when building portable code, compile and test on target systems, avoid weakly specified behavior, and read `STANDARDS`, `NOTES`, and feature-test macro requirements in man pages.

#### 7. Why can code compile on Linux but fail on macOS, BSD, or musl-based Linux?

The code may accidentally depend on GNU, Linux, or glibc behavior rather than a portable POSIX contract. Common examples include using `_GNU_SOURCE`, Linux-only APIs such as `epoll`, `/proc` details, glibc-only functions, nonportable header exposure, or behavior that POSIX leaves unspecified.

Mechanism: system headers expose different declarations depending on feature-test macros and implementation. libc implementations also differ: `glibc`, musl, BSD libc, and macOS libc do not expose exactly the same extension surface.

Pitfall: "it compiled on my Ubuntu machine" is not portability evidence. Production/debug angle: build in CI against the real target platforms or container images, check `man` page `STANDARDS`, define feature-test macros intentionally, and isolate platform-specific code behind small adapters.

#### 8. When would you choose a Linux-specific API over a POSIX API?

Start with POSIX when the POSIX API solves the problem cleanly and portability matters. Choose a Linux-specific API when you intentionally target Linux and the API gives meaningful operational value.

Examples:

| Need | POSIX-ish choice | Linux-specific choice |
|------|------------------|-----------------------|
| File descriptor readiness | `poll()` | `epoll` for large scalable event loops |
| Timers in event loop | POSIX timers | `timerfd` when FD-based integration is simpler |
| Signals in event loop | POSIX signal APIs | `signalfd` for FD-based handling |
| Process/system introspection | POSIX APIs where available | `/proc` for Linux-specific diagnostics |

Pitfall: Linux-specific APIs can be the right engineering choice, but the dependency must be explicit. Production angle: document it in design/code review, test on the oldest supported kernel/libc, and provide fallback only when the product actually needs cross-platform support.

#### 9. What is a feature-test macro, and why can a missing one break compilation?

A feature-test macro tells libc headers which standard or extension set your source expects. Examples include `_POSIX_C_SOURCE`, `_XOPEN_SOURCE`, and `_GNU_SOURCE`.

Mechanism: headers conditionally expose declarations based on macros defined before any header is included. If you define a macro too late, it may not affect declarations because headers include one another.

Correct placement:

```c
#define _POSIX_C_SOURCE 200809L
#include <time.h>
```

Pitfall: adding `_GNU_SOURCE` everywhere may hide portability bugs by exposing GNU extensions globally. Production/debug angle: when a function or constant is "undeclared" despite the right header, check `man 7 feature_test_macros`, the function's man-page synopsis, compiler flags, and whether any header was included before the macro definition.

#### 10. Why does Linux support both BSD-influenced APIs and System V/POSIX IPC families?

UNIX history split into major lineages. BSD contributed networking and the sockets API. System V contributed commercial UNIX interfaces and System V IPC. Later standards preserved many interfaces for compatibility and portability.

Linux became a practical UNIX-like system by supporting useful APIs from multiple traditions. That is why you see sockets, System V message queues/semaphores/shared memory, and POSIX message queues/semaphores/shared memory in the same OS.

Pitfall: do not treat "old" as automatically irrelevant. Vendor SDKs, embedded systems, and inherited services may still use System V IPC. Production/debug angle: know enough to recognize stale System V IPC objects with `ipcs`, clean them with `ipcrm`, and choose POSIX or Linux-specific alternatives deliberately in new designs.

#### 11. In production, why is "runs on Linux" not precise enough?

Linux deployments differ in more than the kernel name. Real behavior can depend on kernel version, enabled kernel config, architecture, libc, dynamic loader, package versions, vendor patches, init system, filesystem layout, container runtime, and security policy.

Mechanism: applications sit on a stack:

```text
application -> libc/libraries -> syscalls -> kernel -> distro/runtime policy
```

Any layer can change the observable behavior. A binary built against one glibc symbol version may fail on another distro. A Linux-only syscall may be missing on an older kernel. A container may share the host kernel but use a different user-space image.

Pitfall: debugging compatibility as if all Linux machines are equivalent wastes time. Production/debug angle: capture kernel, libc, architecture, distro, container image, compiler, and dependency versions in bug reports and CI matrices.

### Priority B - Trả lời ngắn, đúng trọng tâm

#### 12. Why did rewriting UNIX mostly in C matter for portability?

It moved most OS code away from assembly tied to one CPU. Hardware-specific pieces still existed, but the bulk of the system could be recompiled and ported more realistically. That is one reason C became the natural systems language around UNIX.

#### 13. Why is C still common in Linux system programming?

Linux/POSIX APIs are documented primarily as C interfaces, the kernel ABI is close to C calling conventions, and libc exposes the main system-programming surface. Even higher-level runtimes often end up calling C/POSIX/Linux APIs underneath.

#### 14. What did GNU provide, and what did the Linux kernel provide?

GNU provided much of the user-space environment: compiler, shell, C library, build tools, and common utilities. The Linux kernel supplied the missing kernel. Together they formed a complete UNIX-like system, which is why the term GNU/Linux is sometimes used.

#### 15. What is the difference between POSIX, SUS, and XSI?

POSIX is the portable baseline API standard. SUS is The Open Group's broader Single UNIX Specification, aligned around POSIX base volumes plus UNIX certification context. XSI is a stricter profile/extension set; XSI conformance is tied to UNIX branding requirements in the classic SUS discussion.

#### 16. What does unspecified, weakly specified, obsolete, or legacy mean?

Unspecified means the standard does not define the behavior. Weakly specified means it defines the interface but leaves important details open. Obsolete or legacy means kept for old applications but usually avoided in new code. The interview point is: do not build portable logic on behavior the standard refuses to guarantee.

#### 17. How do you check whether an API is POSIX, Linux-specific, GNU-specific, or version-dependent?

Read the man page like a contract: `SYNOPSIS` for headers/macros, `STANDARDS` for portability, `NOTES` for implementation caveats, `VERSIONS` for kernel/libc dependency, and `HISTORY` for origin. Then check the target platform's docs if you need portability outside Linux.

#### 18. What is the difference between POSIX source portability and Linux binary compatibility?

POSIX source portability means the same source can be compiled on another conforming implementation. Binary compatibility means the already-compiled program runs on a compatible system, usually constrained by architecture, libc/loader, ABI, and distribution policy. LSB tried to address Linux binary compatibility, not general UNIX portability.

### Priority C - Học đến đâu là đủ

- Standards timeline: know that TLPI emphasizes SUSv3/SUSv4, while official current material includes POSIX.1-2024 / SUSv5. Do not memorize every revision for interviews.
- XPG, FIPS, SVID, Spec 1170: recognize these as historical standardization/implementation terms. Look them up when maintaining old portability code.
- Early UNIX/BSD/System V dates: useful context, low interview value.
- Old kernel version numbering: recognize that docs may mention kernel version availability; check `VERSIONS` or release notes instead of relying on memory.
- LSB: know it targeted binary compatibility across Linux distributions. Modern production work usually checks actual distro, libc, container, and ABI constraints directly.

---

## Câu Bổ Sung Nên Thêm Nếu Thiếu

1. How would you design a library that is portable across Linux and macOS but still uses `epoll` on Linux when available?
2. A program builds on glibc but fails on musl with missing declarations. What do you check first?
3. A vendor SDK leaves System V shared memory objects after crashes. How do you recognize and clean them safely?
4. Your container image uses Alpine but production used Debian before. What Linux-system assumptions might break?
5. During code review, how would you document an intentional Linux-only dependency?

---

## One-Minute Review

- Chapter 0 is about judgment, not date memorization.
- UNIX explains the shape: processes, files, pipes, shells, byte streams, and composable tools.
- Linux is strictly the kernel; a distribution is kernel plus libc, tools, services, patches, and policy.
- POSIX is a source-level API contract, not a binary ABI and not a kernel design.
- `glibc` is user-space library code; the kernel is entered only through syscalls.
- A C library function may or may not perform a syscall.
- Use POSIX first when portability matters.
- Use Linux-specific APIs deliberately when their operational value is worth the dependency.
- BSD history explains sockets; System V history explains older IPC.
- Feature-test macros must be defined before headers.
- Read man pages through `SYNOPSIS`, `STANDARDS`, `NOTES`, `VERSIONS`, and `HISTORY`.
- In production, always name kernel, libc, architecture, distro/container, and runtime constraints.
