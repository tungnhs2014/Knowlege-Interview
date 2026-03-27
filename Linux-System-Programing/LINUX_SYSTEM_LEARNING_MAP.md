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

## Overall Progress Summary

| Chapter | Số topic | Priority | Status |
|---------|----------|----------|--------|
| 0 — History & Context | 1 | 🔵 context | ✅ |
| 1 — Nền tảng | 5 | 🔴 bắt buộc | ✅ |
| 2 — File I/O | 9 | 🔴 bắt buộc | ✅ |
| 3 — Process | 11 | 🔴 core Embedded | ✅ |
| 4 — Signals | 5 | 🔴 core Embedded | 🔄 |
| 5 — Memory | 3 | 🔴 core Embedded | ✅ |
| 6 — Threads | 5 | 🔴 concurrency | ⬜ |
| 7 — IPC | 10 | 🟡 inter-process | ⬜ |
| 8 — Sockets & Networking | 6 | 🔴 networking | ⬜ |
| 9 — Embedded Special | 3 | 🟡 hardware-level | ⬜ |
| 10 — System & Security | 4 | 🟠 hoàn thiện | ⬜ |
| **Tổng** | **62 topics** | | |

---

## CHAPTER 0 — History & Context

> *Đọc một lần để hiểu WHY — không cần thuộc năm tháng*

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 0.1 | History & Standards — UNIX, GNU, Linux, POSIX | Ch01 | — | ✅ | [ch00_history_and_standards.md](knowledge/linux_system/ch00_history_and_standards.md) |

**3 điều cần hiểu từ lịch sử:**
- UNIX viết bằng C để portable → C trở thành systems language
- BSD thêm TCP/IP stack (1983) → socket API từ đây mà ra
- System V IPC (cũ) và POSIX IPC (mới) tồn tại song song vì tương thích ngược

---

## CHAPTER 1 — Nền tảng hệ thống

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 1.1 | Fundamental Concepts — Kernel, Shell, Process, FD | Ch02 | 01 | ✅ | [ch01_linux_architecture.md](knowledge/linux_system/ch01_linux_architecture.md) |
| 1.2 | System Calls vs Library Functions — user/kernel space | Ch03 | 01 | ✅ | [ch01_linux_architecture.md](knowledge/linux_system/ch01_linux_architecture.md) |
| 1.3 | Users & Groups — UID/GID, credentials, set-user-ID | Ch08, Ch09 | — | ✅ | [ch01_users_and_groups.md](knowledge/linux_system/ch01_users_and_groups.md) |
| 1.4 | /proc Filesystem — runtime process & system info | Ch12 | — | ✅ | [ch01_system_info.md](knowledge/linux_system/ch01_system_info.md) |
| 1.5 | System Limits & Options — `sysconf()`, `pathconf()` | Ch11 | — | ✅ | [ch01_system_info.md](knowledge/linux_system/ch01_system_info.md) |

---

## CHAPTER 2 — File I/O

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 2.1 | File I/O Universal Model — `open/read/write/close`, FD | Ch04 | 02 | ✅ | [ch02_file_io_core.md](knowledge/linux_system/ch02_file_io_core.md) |
| 2.2 | File I/O Further Details — `fcntl()`, scatter-gather, `pread/pwrite` | Ch05 | 02 | ✅ | [ch02_file_io_core.md](knowledge/linux_system/ch02_file_io_core.md) |
| 2.3 | File I/O Buffering — kernel buffer, stdio buffer, `fsync()`, `O_DIRECT` | Ch13 | 02 | ✅ | [ch02_file_io_core.md](knowledge/linux_system/ch02_file_io_core.md) |
| 2.4 | File Systems & Inodes — VFS, ext4, inode, mount | Ch14 | 02 | ✅ | [ch02_file_system.md](knowledge/linux_system/ch02_file_system.md) |
| 2.5 | File Attributes & Permissions — `stat()`, `chmod()`, `umask`, sticky bit | Ch15 | 02 | ✅ | [ch02_file_system.md](knowledge/linux_system/ch02_file_system.md) |
| 2.6 | Directories & Links — hard link, symlink, `opendir/readdir`, `chroot()` | Ch18 | 02 | ✅ | [ch02_file_system.md](knowledge/linux_system/ch02_file_system.md) |
| 2.7 | File Locking — `flock()`, `fcntl()` record locking | Ch55 | 02 | ✅ | [ch02_file_advanced.md](knowledge/linux_system/ch02_file_advanced.md) |
| 2.8 | Monitoring File Events — `inotify` | Ch19 | — | ✅ | [ch02_file_advanced.md](knowledge/linux_system/ch02_file_advanced.md) |
| 2.9 | Extended Attributes & ACL — `setxattr()`, `getfacl/setfacl` | Ch16, Ch17 | — | ✅ | [ch02_file_advanced.md](knowledge/linux_system/ch02_file_advanced.md) |

---

## CHAPTER 3 — Process

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 3.1 | Process Fundamentals — PID, memory layout, environment | Ch06 | 03 | ✅ | [ch03_process_core.md](knowledge/linux_system/ch03_process_core.md) |
| 3.2 | Process Creation — `fork()`, copy-on-write, file sharing | Ch24 | 03 | ✅ | [ch03_process_core.md](knowledge/linux_system/ch03_process_core.md) |
| 3.3 | Process Termination — `exit()`, `_exit()`, `atexit()` | Ch25 | 03 | ✅ | [ch03_process_core.md](knowledge/linux_system/ch03_process_core.md) |
| 3.4 | Monitoring Child Processes — `wait/waitpid`, zombie, orphan | Ch26 | 03 | ✅ | [ch03_process_execution.md](knowledge/linux_system/ch03_process_execution.md) |
| 3.5 | Program Execution — `exec()` family, `system()`, scripts | Ch27 | 03 | ✅ | [ch03_process_execution.md](knowledge/linux_system/ch03_process_execution.md) |
| 3.6 | Process Creation in Detail — `clone()`, fork/exec attributes | Ch28 | — | ✅ | [ch03_process_execution.md](knowledge/linux_system/ch03_process_execution.md) |
| 3.7 | Process Groups, Sessions & Job Control — `setsid()`, `SIGHUP` | Ch34 | — | ✅ | [ch03_process_advanced.md](knowledge/linux_system/ch03_process_advanced.md) |
| 3.8 | Process Credentials — real/effective UID/GID, `setuid()` | Ch09 | — | ✅ | [ch01_users_and_groups.md](knowledge/linux_system/ch01_users_and_groups.md) · [ch03_process_advanced.md](knowledge/linux_system/ch03_process_advanced.md) |
| 3.9 | Daemons — daemon creation, `syslog()` | Ch37 | — | ✅ | [ch03_process_advanced.md](knowledge/linux_system/ch03_process_advanced.md) |
| 3.10 | Process Priorities & Scheduling — `SCHED_FIFO`, `SCHED_RR`, `nice` | Ch35 | — | ✅ | [ch03_process_advanced.md](knowledge/linux_system/ch03_process_advanced.md) |
| 3.11 | Process Resources & Limits — `rlimit`, `getrusage()` | Ch36 | — | ✅ | [ch03_process_advanced.md](knowledge/linux_system/ch03_process_advanced.md) |

---

## CHAPTER 4 — Signals ✅

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 4.1 | Signals Fundamentals — disposition, `sigaction()`, `kill()`, mask | Ch20 | 04 | ✅ | [ch04_signals_core.md](knowledge/linux_system/ch04_signals_core.md) |
| 4.2 | Signal Handlers — async-signal-safe, `SA_SIGINFO`, `sigaltstack()` | Ch21 | 04 | ✅ | [ch04_signals_core.md](knowledge/linux_system/ch04_signals_core.md) |
| 4.3 | Signals Advanced — realtime signals, `signalfd()`, `sigqueue()` | Ch22 | 04 | ✅ | [ch04_signals_core.md](knowledge/linux_system/ch04_signals_core.md) |
| 4.4 | Timers & Sleeping — `setitimer()`, `timer_create()`, `timerfd` | Ch23 | — | ✅ | [ch04_signals_time.md](knowledge/linux_system/ch04_signals_time.md) |
| 4.5 | Time API — `clock_gettime()`, `gettimeofday()`, process time | Ch10 | — | ✅ | [ch04_signals_time.md](knowledge/linux_system/ch04_signals_time.md) |

---

## CHAPTER 5 — Memory

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 5.1 | Memory Allocation — `malloc/free`, heap, `brk()`, memory leak | Ch07 | 03 | ✅ | [ch05_memory.md](knowledge/linux_system/ch05_memory.md) |
| 5.2 | Memory Mappings (mmap) — `mmap()`, file mapping, device memory | Ch49 | — | ✅ | [ch05_memory.md](knowledge/linux_system/ch05_memory.md) |
| 5.3 | Virtual Memory Ops — `mprotect()`, `mlock()`, `madvise()` | Ch50 | — | ✅ | [ch05_memory.md](knowledge/linux_system/ch05_memory.md) |

---

## CHAPTER 6 — Threads

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 6.1 | Threads Introduction — `pthread_create/join/detach`, threads vs processes | Ch29 | 05 | ⬜ | — |
| 6.2 | Thread Synchronization — mutex, condition variables, deadlock | Ch30 | 05 | ⬜ | — |
| 6.3 | Thread Safety & TLS — reentrancy, `pthread_key`, `__thread` | Ch31 | 05 | ⬜ | — |
| 6.4 | Thread Cancellation — `pthread_cancel()`, cleanup handlers | Ch32 | 05 | ⬜ | — |
| 6.5 | Threads Further Details — thread stacks, signals + threads, NPTL | Ch33 | 05 | ⬜ | — |

---

## CHAPTER 7 — IPC

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 7.1 | IPC Overview — taxonomy, pipe vs socket vs shm vs mq | Ch43 | 09 | ⬜ | — |
| 7.2 | Pipes & FIFOs — `pipe()`, `mkfifo()`, named pipes | Ch44 | 08 | ⬜ | — |
| 7.3 | System V IPC Intro — `ftok()`, `ipcs`, `ipcrm` | Ch45 | 09 | ⬜ | — |
| 7.4 | System V Message Queues — `msgget/msgsnd/msgrcv/msgctl()` | Ch46 | 09 | ⬜ | — |
| 7.5 | System V Semaphores — `semget/semop/semctl()` | Ch47 | 11 | ⬜ | — |
| 7.6 | System V Shared Memory — `shmget/shmat/shmdt()` | Ch48 | 10 | ⬜ | — |
| 7.7 | POSIX IPC Intro — so sánh với System V | Ch51 | 09 | ⬜ | — |
| 7.8 | POSIX Message Queues — `mq_open/mq_send/mq_receive()` | Ch52 | 09 | ⬜ | — |
| 7.9 | POSIX Semaphores — `sem_open/sem_wait/sem_post()` | Ch53 | 11 | ⬜ | — |
| 7.10 | POSIX Shared Memory — `shm_open()` + `mmap()` | Ch54 | 10 | ⬜ | — |

---

## CHAPTER 8 — Sockets & Networking

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 8.1 | Sockets Introduction — `socket/bind/listen/accept/connect()` | Ch56 | 06 | ⬜ | — |
| 8.2 | UNIX Domain Sockets — local IPC, `socketpair()`, fd passing | Ch57 | 06 | ⬜ | — |
| 8.3 | TCP/IP Fundamentals — layers, TCP vs UDP, byte order | Ch58 | 06 | ⬜ | — |
| 8.4 | Internet Domain Sockets — `sockaddr_in`, `getaddrinfo()` | Ch59 | 06 | ⬜ | — |
| 8.5 | Socket Server Design — iterative, concurrent, prefork/prethread | Ch60 | 06 | ⬜ | — |
| 8.6 | Advanced Socket Topics — `sendmsg/recvmsg()`, `SO_REUSEADDR` | Ch61 | — | ⬜ | — |

---

## CHAPTER 9 — Embedded Special

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 9.1 | Alternative I/O Models — `select()`, `poll()`, `epoll` | Ch63 | — | ⬜ | — |
| 9.2 | Terminals & termios — serial port, raw mode, `tcsetattr()` | Ch62 | — | ⬜ | — |
| 9.3 | Pseudoterminals (PTY) — `posix_openpt()`, SSH, remote shell | Ch64 | — | ⬜ | — |

---

## CHAPTER 10 — System & Security

| # | Topic | TLPI | DevLinux | Status | Knowledge Doc |
|---|-------|------|----------|--------|---------------|
| 10.1 | Shared Library Fundamentals — `.so`, soname, `ldconfig`, PIC | Ch41 | 01 | ⬜ | — |
| 10.2 | Advanced Shared Libraries — `dlopen/dlsym()`, `LD_PRELOAD` | Ch42 | — | ⬜ | — |
| 10.3 | Linux Capabilities — `CAP_*`, least privilege, `capset()` | Ch39 | — | ⬜ | — |
| 10.4 | Writing Secure Programs — input validation, race conditions | Ch38 | — | ⬜ | — |

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

| Symbol | Meaning |
|--------|---------|
| ⬜ | Not started |
| 🔄 | In progress |
| ✅ | Completed |
| 📝 | Has knowledge doc |
| 🎯 | Has interview questions |
