# Linux System Programming — Learning Map

> Roadmap học Linux System Programming theo phương pháp AI cộng sinh.
> Mục tiêu: **Embedded-first, Backend-ready**
> Cập nhật khi hoàn thành từng topic.

---

## Training Sources

| Source | Thư mục | Mô tả |
|--------|---------|-------|
| Linux Programming Interface (TLPI) | `docs/Linux-Programming-Interface/` | Sách Michael Kerrisk — 64 chapters |
| Linux Programming DevLinux | `docs/Linux-Programming-DevLinux/` | Khóa học thực hành — 12 modules |

---

## Coverage Gate

This file is both the source-routing authority and the coverage contract.

For every write, refactor, or review task:
- Read `CODEX.md`, this map, mapped TLPI docs, mapped DevLinux docs when present, and existing outputs.
- Build a Coverage Matrix before writing.
- Cover every mapped topic row and every chapter `Must Cover` concept.
- Preserve correct useful existing content unless it is merged into equivalent coverage, moved with a clear note, or explicitly out of scope.
- Treat missing coverage as a blocker, even when the writing style is good.
- If DevLinux is mapped, read its `INDEX.md`, root `README.md`, mapped module README, and useful exercise/project READMEs for practical coverage.

## Status Model

| Status | Meaning |
|--------|---------|
| Mapped | Topic/source/output route exists, but no trusted output yet. |
| Drafted | Output exists, but has not passed the new coverage gate. |
| Coverage Reviewed | Output was checked against the Coverage Matrix; minor fixes may remain. |
| Final | Output passed source coverage, correctness, work-readiness, interview-readiness, and review blockers. |

---

## Overall Progress Summary

| Chapter | Số topic | Priority | Status |
|---------|----------|----------|--------|
| 0 — History & Context | 1 | 🔵 context | Drafted |
| 1 — Nền tảng | 5 | 🔴 bắt buộc | Drafted |
| 2 — File I/O | 9 | 🔴 bắt buộc | Drafted |
| 3 — Process | 11 | 🔴 core Embedded | Drafted |
| 4 — Signals | 5 | 🔴 core Embedded | Drafted |
| 5 — Memory | 3 | 🔴 core Embedded | Drafted |
| 6 — Threads | 5 | 🔴 concurrency | Drafted |
| 7 — IPC | 10 | 🟡 inter-process | Drafted |
| 8 — Sockets & Networking | 6 | 🔴 networking | Drafted |
| 9 — Embedded Special | 3 | 🟡 hardware-level | Drafted |
| 10 — System & Security | 4 | 🟠 hoàn thiện | Drafted |
| **Tổng** | **62 topics** | | |

---

## CHAPTER 0 — History & Context

> *Đọc một lần để hiểu WHY — không cần thuộc năm tháng*

**Must Cover**
- UNIX/GNU/Linux/POSIX lineage and why it shapes Linux system APIs.
- Portability vs Linux-specific behavior.
- Why old and new API families coexist, especially BSD sockets, System V IPC, and POSIX IPC.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 0.1 | History & Standards — UNIX, GNU, Linux, POSIX | Ch01 | — | Drafted | [ch00_history_and_standards.md](knowledge/ch00_history_and_standards.md) |

**3 điều cần hiểu từ lịch sử:**
- UNIX viết bằng C để portable → C trở thành systems language
- BSD thêm TCP/IP stack (1983) → socket API từ đây mà ra
- System V IPC (cũ) và POSIX IPC (mới) tồn tại song song vì tương thích ngược

---

## CHAPTER 1 — Nền tảng hệ thống

**Must Cover**
- Kernel/user space boundary, shell role, process basics, file descriptors, and the universal "everything is an object handle" intuition.
- System calls vs libc wrappers, errno, blocking behavior, and user/kernel transitions.
- Users, groups, real/effective/saved IDs, permissions, set-user-ID/set-group-ID risk.
- `/proc` as runtime process/system evidence.
- System limits/options via `sysconf()`, `pathconf()`, and related runtime constraints.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 1.1 | Fundamental Concepts — Kernel, Shell, Process, FD | Ch02 | 01 | Drafted | [ch01_linux_architecture.md](knowledge/ch01_linux_architecture.md) |
| 1.2 | System Calls vs Library Functions — user/kernel space | Ch03 | 01 | Drafted | [ch01_linux_architecture.md](knowledge/ch01_linux_architecture.md) |
| 1.3 | Users & Groups — UID/GID, credentials, set-user-ID | Ch08, Ch09 | — | Drafted | [ch01_users_and_groups.md](knowledge/ch01_users_and_groups.md) |
| 1.4 | /proc Filesystem — runtime process & system info | Ch12 | — | Drafted | [ch01_system_info.md](knowledge/ch01_system_info.md) |
| 1.5 | System Limits & Options — `sysconf()`, `pathconf()` | Ch11 | — | Drafted | [ch01_system_info.md](knowledge/ch01_system_info.md) |

---

## CHAPTER 2 — File I/O

**Must Cover**
- Universal I/O model: `open()`, `read()`, `write()`, `close()`, file descriptors, and error/short-I/O handling.
- FD table vs open file description, file offsets, status flags, descriptor flags, duplication, and inheritance.
- Kernel buffering, stdio buffering, `fsync()`, direct I/O tradeoffs, and embedded storage constraints.
- VFS, filesystems, inode/dentry/path model, mounts, and metadata.
- Permissions, ownership, `umask`, special bits, directories, hard links, symlinks, and `chroot()` limits.
- File locking, inotify, extended attributes, ACLs, production race/debug workflows.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 2.1 | File I/O Universal Model — `open/read/write/close`, FD | Ch04 | 02 | Drafted | [ch02_file_io_core.md](knowledge/ch02_file_io_core.md) |
| 2.2 | File I/O Further Details — `fcntl()`, scatter-gather, `pread/pwrite` | Ch05 | 02 | Drafted | [ch02_file_io_core.md](knowledge/ch02_file_io_core.md) |
| 2.3 | File I/O Buffering — kernel buffer, stdio buffer, `fsync()`, `O_DIRECT` | Ch13 | 02 | Drafted | [ch02_file_io_core.md](knowledge/ch02_file_io_core.md) |
| 2.4 | File Systems & Inodes — VFS, ext4, inode, mount | Ch14 | 02 | Drafted | [ch02_file_system.md](knowledge/ch02_file_system.md) |
| 2.5 | File Attributes & Permissions — `stat()`, `chmod()`, `umask`, sticky bit | Ch15 | 02 | Drafted | [ch02_file_system.md](knowledge/ch02_file_system.md) |
| 2.6 | Directories & Links — hard link, symlink, `opendir/readdir`, `chroot()` | Ch18 | 02 | Drafted | [ch02_file_system.md](knowledge/ch02_file_system.md) |
| 2.7 | File Locking — `flock()`, `fcntl()` record locking | Ch55 | 02 | Drafted | [ch02_file_advanced.md](knowledge/ch02_file_advanced.md) |
| 2.8 | Monitoring File Events — `inotify` | Ch19 | — | Drafted | [ch02_file_advanced.md](knowledge/ch02_file_advanced.md) |
| 2.9 | Extended Attributes & ACL — `setxattr()`, `getfacl/setfacl` | Ch16, Ch17 | — | Drafted | [ch02_file_advanced.md](knowledge/ch02_file_advanced.md) |

---

## CHAPTER 3 — Process

**Must Cover**
- Process lifecycle: program -> process -> fork -> child setup -> exec -> exit -> wait/reap.
- Process as ownership boundary: PID, virtual memory, file descriptors, cwd/root, environment, credentials, signals, scheduling/accounting state.
- Multiprocess design: parent/child responsibilities, supervision, error reporting, FD hygiene, close-on-exec.
- `fork()`, copy-on-write, parent/child scheduling nondeterminism, globals are not IPC, process vs thread tradeoffs.
- `execve()` and `exec*()` variants, environment/PATH risk, scripts, `system()` risks.
- `wait()`, `waitpid()`, `waitid()`, status macros, zombies, orphans, subreapers, PID namespaces.
- Process groups, sessions, job control, controlling terminals, foreground/background signal behavior.
- Credentials, privilege drop, set-user-ID behavior, daemon/service-manager lifecycle.
- CPU scheduling, nice values, realtime policies, concurrency vs parallelism, context switches, CPU affinity.
- Resource limits, `getrusage()`, process accounting, core dumps, production debugging with `/proc`, `ps`, `strace`, `perf`, and service logs.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 3.1 | Process Fundamentals — PID, memory layout, environment | Ch06 | 03 | Drafted | [ch03_process_core.md](knowledge/ch03_process_core.md) |
| 3.2 | Process Creation — `fork()`, copy-on-write, file sharing | Ch24 | 03 | Drafted | [ch03_process_core.md](knowledge/ch03_process_core.md) |
| 3.3 | Process Termination — `exit()`, `_exit()`, `atexit()` | Ch25 | 03 | Drafted | [ch03_process_core.md](knowledge/ch03_process_core.md) |
| 3.4 | Monitoring Child Processes — `wait/waitpid`, zombie, orphan | Ch26 | 03 | Drafted | [ch03_process_execution.md](knowledge/ch03_process_execution.md) |
| 3.5 | Program Execution — `exec()` family, `system()`, scripts | Ch27 | 03 | Drafted | [ch03_process_execution.md](knowledge/ch03_process_execution.md) |
| 3.6 | Process Creation in Detail — `clone()`, fork/exec attributes | Ch28 | — | Drafted | [ch03_process_execution.md](knowledge/ch03_process_execution.md) |
| 3.7 | Process Groups, Sessions & Job Control — `setsid()`, `SIGHUP` | Ch34 | — | Drafted | [ch03_process_advanced.md](knowledge/ch03_process_advanced.md) |
| 3.8 | Process Credentials — real/effective UID/GID, `setuid()` | Ch09 | — | Drafted | [ch01_users_and_groups.md](knowledge/ch01_users_and_groups.md) · [ch03_process_advanced.md](knowledge/ch03_process_advanced.md) |
| 3.9 | Daemons — daemon creation, `syslog()` | Ch37 | — | Drafted | [ch03_process_advanced.md](knowledge/ch03_process_advanced.md) |
| 3.10 | Process Priorities & Scheduling — `SCHED_FIFO`, `SCHED_RR`, `nice`, context switches | Ch35 | — | Drafted | [ch03_process_advanced.md](knowledge/ch03_process_advanced.md) |
| 3.11 | Process Resources & Limits — `rlimit`, `getrusage()` | Ch36 | — | Drafted | [ch03_process_advanced.md](knowledge/ch03_process_advanced.md) |

---

## CHAPTER 4 — Signals

**Must Cover**
- Signal lifecycle: generation, pending state, delivery, disposition, default action, ignore, catch.
- Signal masks, per-process vs per-thread behavior, blocking/unblocking, `sigaction()` over `signal()`.
- Async-signal-safety, handler design, `sig_atomic_t`, self-pipe/signalfd patterns, reentrancy traps.
- Standard vs realtime signals, queuing behavior, payloads, ordering, `sigqueue()`.
- Timers/sleeping APIs, process time vs wall time, timer delivery choices, timeout design.
- Production debugging: `strace`, `/proc/<pid>/status`, signal masks, core-dump signal evidence.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 4.1 | Signals Fundamentals — disposition, `sigaction()`, `kill()`, mask | Ch20 | 04 | Drafted | [ch04_signals_core.md](knowledge/ch04_signals_core.md) |
| 4.2 | Signal Handlers — async-signal-safe, `SA_SIGINFO`, `sigaltstack()` | Ch21 | 04 | Drafted | [ch04_signals_core.md](knowledge/ch04_signals_core.md) |
| 4.3 | Signals Advanced — realtime signals, `signalfd()`, `sigqueue()` | Ch22 | 04 | Drafted | [ch04_signals_core.md](knowledge/ch04_signals_core.md) |
| 4.4 | Timers & Sleeping — `setitimer()`, `timer_create()`, `timerfd` | Ch23 | — | Drafted | [ch04_timers_and_time.md](knowledge/ch04_timers_and_time.md) |
| 4.5 | Time API — `clock_gettime()`, `gettimeofday()`, process time | Ch10 | — | Drafted | [ch04_timers_and_time.md](knowledge/ch04_timers_and_time.md) |

---

## CHAPTER 5 — Memory

**Must Cover**
- Heap allocation model: `malloc()`, `free()`, `brk()`, allocator metadata, leaks, fragmentation, ownership.
- Virtual memory fundamentals: address space, pages, mappings, protection, page faults, overcommit, copy-on-write connections to `fork()`.
- `mmap()` use cases: anonymous mapping, file mapping, shared/private mapping, device memory, and cleanup.
- Virtual memory operations: `mprotect()`, `mlock()`, `madvise()`, page residency, and security/performance tradeoffs.
- Embedded constraints: limited RAM/no swap, memory pressure, OOM behavior, deterministic allocation, and debugging with `/proc`, `pmap`, `valgrind`, `asan`, or target-appropriate tools.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 5.1 | Memory Allocation — `malloc/free`, heap, `brk()`, memory leak | Ch07 | 03 | Drafted | [ch05_memory.md](knowledge/ch05_memory.md) |
| 5.2 | Memory Mappings (mmap) — `mmap()`, file mapping, device memory | Ch49 | — | Drafted | [ch05_memory.md](knowledge/ch05_memory.md) |
| 5.3 | Virtual Memory Ops — `mprotect()`, `mlock()`, `madvise()` | Ch50 | — | Drafted | [ch05_memory.md](knowledge/ch05_memory.md) |

---

## CHAPTER 6 — Threads

**Must Cover**
- Thread lifecycle: `pthread_create()`, start routine, join, detach, process exit vs thread exit.
- Process vs thread: shared address space, shared FDs, per-thread stack/TID/signal mask/errno/scheduling details.
- Concurrency vs parallelism, CPU scheduling effects, blocking calls, and context-switch implications.
- Synchronization: mutexes, condition variables, read-write locks, barriers, semaphores when relevant, memory visibility, and invariants.
- Race conditions, deadlock, livelock/starvation, lock ordering, timeout/recovery patterns, and debugging.
- Thread safety, reentrancy, thread-local storage, `pthread_key`, `__thread`/`thread_local`.
- Cancellation, cleanup handlers, cancellation points, resource ownership, and safe shutdown.
- Signals with threads, thread stacks, NPTL/Linux details, embedded stack/RAM constraints.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 6.1 | Threads Introduction — `pthread_create/join/detach`, threads vs processes | Ch29 | 05 | Drafted | [ch06_threads_core.md](knowledge/ch06_threads_core.md) |
| 6.2 | Thread Synchronization — mutex, condition variables, deadlock | Ch30 | 05 | Drafted | [ch06_threads_sync.md](knowledge/ch06_threads_sync.md) |
| 6.3 | Thread Safety & TLS — reentrancy, `pthread_key`, `__thread` | Ch31 | 05 | Drafted | [ch06_threads_tls.md](knowledge/ch06_threads_tls.md) |
| 6.4 | Thread Cancellation — `pthread_cancel()`, cleanup handlers | Ch32 | 05 | Drafted | [ch06_threads_cancel.md](knowledge/ch06_threads_cancel.md) |
| 6.5 | Threads Further Details — thread stacks, signals + threads, NPTL | Ch33 | 05 | Drafted | [ch06_threads_details.md](knowledge/ch06_threads_details.md) |

---

## CHAPTER 7 — IPC

**Must Cover**
- IPC taxonomy: communication vs synchronization; data-transfer IPC vs shared memory IPC.
- Byte-stream IPC vs message-oriented IPC, message boundaries, framing, ordering, backpressure, blocking behavior.
- Peer relationship and lifetime: related/unrelated processes, anonymous/named objects, descriptor inheritance, persistence, cleanup.
- Pipes/FIFOs: unidirectional stream semantics, EOF/SIGPIPE, `PIPE_BUF`, close discipline, deadlocks, multi-client limits.
- System V IPC: keys, ids, permissions, `ipcs`/`ipcrm`, message queues, semaphores, shared memory, stale objects, init races.
- POSIX IPC: names, open/close/unlink lifecycle, message queues, semaphores, shared memory via `shm_open()` + `mmap()`.
- Shared memory protocol: layout, offsets not raw pointers, synchronization, versioning, recovery after crash.
- Synchronization primitives: semaphores, process-shared mutex/condvar when relevant, lock ownership, deadlock/starvation.
- Production debugging: `strace`, `lsof`, `/proc`, `ipcs`, `/proc/sysvipc`, `/dev/mqueue`, `/dev/shm`, permissions, namespaces, embedded watchdog restart cleanup.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 7.1 | IPC Overview — taxonomy, data-transfer vs shared memory, byte stream vs message, synchronization | Ch43 | 08, 09, 10, 11 | Drafted | [ch07_ipc_overview.md](knowledge/ch07_ipc_overview.md) |
| 7.2 | Pipes & FIFOs — `pipe()`, `mkfifo()`, named pipes | Ch44 | 08 | Drafted | [ch07_ipc_pipes.md](knowledge/ch07_ipc_pipes.md) |
| 7.3 | System V IPC Intro — `ftok()`, `ipcs`, `ipcrm` | Ch45 | 09 | Drafted | [ch07_ipc_sysv.md](knowledge/ch07_ipc_sysv.md) |
| 7.4 | System V Message Queues — `msgget/msgsnd/msgrcv/msgctl()` | Ch46 | 09 | Drafted | [ch07_ipc_sysv.md](knowledge/ch07_ipc_sysv.md) |
| 7.5 | System V Semaphores — `semget/semop/semctl()` | Ch47 | 11 | Drafted | [ch07_ipc_sysv.md](knowledge/ch07_ipc_sysv.md) |
| 7.6 | System V Shared Memory — `shmget/shmat/shmdt()` | Ch48 | 10 | Drafted | [ch07_ipc_sysv.md](knowledge/ch07_ipc_sysv.md) |
| 7.7 | POSIX IPC Intro — so sánh với System V | Ch51 | 09 | Drafted | [ch07_ipc_posix.md](knowledge/ch07_ipc_posix.md) |
| 7.8 | POSIX Message Queues — `mq_open/mq_send/mq_receive()` | Ch52 | 09 | Drafted | [ch07_ipc_posix.md](knowledge/ch07_ipc_posix.md) |
| 7.9 | POSIX Semaphores — `sem_open/sem_wait/sem_post()` | Ch53 | 11 | Drafted | [ch07_ipc_posix.md](knowledge/ch07_ipc_posix.md) |
| 7.10 | POSIX Shared Memory — `shm_open()` + `mmap()` | Ch54 | 10 | Drafted | [ch07_ipc_posix.md](knowledge/ch07_ipc_posix.md) |

---

## CHAPTER 8 — Sockets & Networking

**Must Cover**
- Socket model: endpoint, address family, socket type, protocol, bind/listen/accept/connect lifecycle.
- UNIX domain sockets for local IPC, pathname vs abstract namespace, credentials, `socketpair()`, fd passing.
- TCP/IP fundamentals: layers, byte order, TCP stream semantics, UDP datagram semantics, ports, addresses, DNS/name resolution.
- Internet sockets: `sockaddr_in`, `sockaddr_in6`, `getaddrinfo()`, dual-stack and IPv4/IPv6 portability.
- Server designs: iterative, fork-per-connection, threaded, prefork/prethread, event loop, backpressure and overload behavior.
- Advanced topics: `sendmsg()`/`recvmsg()`, ancillary data, `SO_REUSEADDR`, nonblocking I/O, timeouts, keepalive, production debugging with `ss`, `tcpdump`, `strace`, and logs.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 8.1 | Sockets Introduction — `socket/bind/listen/accept/connect()` | Ch56 | 06 | Drafted | [ch08_socket_overview.md](knowledge/ch08_socket_overview.md) |
| 8.2 | UNIX Domain Sockets — local IPC, `socketpair()`, fd passing | Ch57 | 06 | Drafted | [ch08_socket_unix.md](knowledge/ch08_socket_unix.md) |
| 8.3 | TCP/IP Fundamentals — layers, TCP vs UDP, byte order | Ch58 | 06 | Drafted | [ch08_socket_tcp.md](knowledge/ch08_socket_tcp.md) |
| 8.4 | Internet Domain Sockets — `sockaddr_in`, `getaddrinfo()` | Ch59 | 06 | Drafted | [ch08_socket_tcp.md](knowledge/ch08_socket_tcp.md) |
| 8.5 | Socket Server Design — iterative, concurrent, prefork/prethread | Ch60 | 06 | Drafted | [ch08_socket_server.md](knowledge/ch08_socket_server.md) |
| 8.6 | Advanced Socket Topics — `sendmsg/recvmsg()`, `SO_REUSEADDR` | Ch61 | — | Drafted | [ch08_socket_advanced.md](knowledge/ch08_socket_advanced.md) |

---

## CHAPTER 9 — Embedded Special

**Must Cover**
- Alternative I/O models: `select()`, `poll()`, `epoll`, readiness vs completion, fd sets, edge/level triggering, timeout behavior.
- Event-loop design across files, pipes, sockets, terminals, timerfd/signalfd/eventfd where relevant.
- Terminals and serial ports: termios, canonical/raw mode, baud rate, flow control, line discipline, device-file behavior.
- Pseudoterminals: PTY master/slave model, SSH/terminal emulation, process/session/control-terminal interaction.
- Embedded I/O constraints: limited CPU/RAM, device hotplug, driver/user-space boundary, logging constraints, production debugging with `/proc`, `strace`, `lsof`, `stty`, and service logs.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 9.1 | Alternative I/O Models — `select()`, `poll()`, `epoll` | Ch63 | — | Drafted | [ch09_io_multiplexing.md](knowledge/ch09_io_multiplexing.md) |
| 9.2 | Terminals & termios — serial port, raw mode, `tcsetattr()` | Ch62 | — | Drafted | [ch09_terminals.md](knowledge/ch09_terminals.md) |
| 9.3 | Pseudoterminals (PTY) — `posix_openpt()`, SSH, remote shell | Ch64 | — | Drafted | [ch09_pty.md](knowledge/ch09_pty.md) |

---

## CHAPTER 10 — System & Security

**Must Cover**
- Shared library fundamentals: ELF `.so`, soname, ABI compatibility, dynamic linker, search paths, `ldconfig`, PIC.
- Advanced shared library behavior: `dlopen()`, `dlsym()`, `dlclose()`, symbol resolution, `LD_PRELOAD`, plugin risks.
- Capabilities: capability sets, file capabilities, bounding/ambient sets when relevant, least privilege, debugging with `getcap`/`capsh`.
- Secure privileged programs: input validation, environment/PATH risk, race conditions, temporary files, privilege drop, set-user-ID hazards.
- Production/security debugging: loader diagnostics, permissions, capabilities, audit/log evidence, embedded deployment constraints.

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 10.1 | Shared Library Fundamentals — `.so`, soname, `ldconfig`, PIC | Ch41 | 01 | Drafted | [ch10_shared_library_fundamentals.md](knowledge/ch10_shared_library_fundamentals.md) |
| 10.2 | Advanced Shared Libraries — `dlopen/dlsym()`, `LD_PRELOAD` | Ch42 | — | Drafted | [ch10_advanced_shared_libraries.md](knowledge/ch10_advanced_shared_libraries.md) |
| 10.3 | Linux Capabilities — `CAP_*`, least privilege, `capset()` | Ch39 | — | Drafted | [ch10_linux_capabilities.md](knowledge/ch10_linux_capabilities.md) |
| 10.4 | Writing Secure Programs — input validation, race conditions | Ch38 | — | Drafted | [ch10_writing_secure_programs.md](knowledge/ch10_writing_secure_programs.md) |

---

## Learning Path Visualization

```
CHAPTER 0        CHAPTER 1         CHAPTER 2         CHAPTER 3
History    →   Nền tảng      →   File I/O      →    Process
(context)      (bắt buộc)        (bắt buộc)         (core Embedded)
                                                          ↓
                                    CHAPTER 5         CHAPTER 4
                                    Memory        ←   Signals
                                    (Embedded)        (core Embedded)
                                        ↓
                                    CHAPTER 6
                                    Threads
                                    (concurrency)
                                        ↓
                              CHAPTER 7        CHAPTER 8
                              IPC          →   Sockets
                              (inter-proc)     (networking)
                                        ↓
                              CHAPTER 9        CHAPTER 10
                              Embedded     →   Security
                              Special          (hoàn thiện)
```

---

## Status Legend

Use the `Status Model` near the top of this file:

| Status | Meaning |
|--------|---------|
| Mapped | Route exists; trusted output is not confirmed. |
| Drafted | Output exists but has not passed the new coverage gate. |
| Coverage Reviewed | Output was checked against the Coverage Matrix. |
| Final | Output has no correctness, coverage, work-readiness, or interview-readiness blockers. |
