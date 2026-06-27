# Chapter 2 Interview - File I/O

> Scope: practical Linux file I/O for backend, Linux system, and Embedded Linux interviews: descriptors, file lifetime, durability, permissions, atomic updates, locking, file watching, and production debugging.
> Primary repo sources: `knowledge/ch02_file_io_core.md`, `knowledge/ch02_file_system.md`, `knowledge/ch02_file_advanced.md`.
> Supporting repo sources: TLPI-derived docs `ch04`, `ch05`, `ch13`, `ch14`, `ch15`, `ch18`, `ch55`, `ch19`, `ch16`, `ch17`, plus DevLinux Module 02 and its exercises.

---

## Review Basis

This interview file is intentionally scenario-first. Real Linux, Embedded Linux, and system-software interviews rarely ask only "What is flag X?". They usually start from a project situation, a production bug, or a design trade-off, then drill into the keywords.

Correctness was checked against:

- Repo knowledge files for Chapter 2: low-level I/O, filesystem metadata, directories, links, locks, `inotify`, xattr, and ACL.
- TLPI-derived docs: universal I/O, open file descriptions, atomic `O_APPEND`, `dup2()`, `pread()`, buffering, `fsync()`, VFS, inodes, permissions, links, `rename()`, `flock()`, `fcntl()` record locks, `inotify`, xattr, and ACL.
- DevLinux Module 02: file operation workflows, page cache intuition, file tables, `O_APPEND` exercise, `stat()` exercise, and locking practice.
- Linux man-pages for API semantics: [`open(2)`](https://man7.org/linux/man-pages/man2/open.2.html), [`read(2)`](https://man7.org/linux/man-pages/man2/read.2.html), [`write(2)`](https://man7.org/linux/man-pages/man2/write.2.html), [`close(2)`](https://man7.org/linux/man-pages/man2/close.2.html), [`lseek(2)`](https://man7.org/linux/man-pages/man2/lseek.2.html), [`dup(2)`](https://man7.org/linux/man-pages/man2/dup.2.html), [`fcntl(2)`](https://man7.org/linux/man-pages/man2/fcntl.2.html), [`pread(2)`](https://man7.org/linux/man-pages/man2/pread.2.html), [`readv(2)`](https://man7.org/linux/man-pages/man2/readv.2.html), [`fsync(2)`](https://man7.org/linux/man-pages/man2/fsync.2.html), [`sync(2)`](https://man7.org/linux/man-pages/man2/sync.2.html), [`stat(2)`](https://man7.org/linux/man-pages/man2/stat.2.html), [`inode(7)`](https://man7.org/linux/man-pages/man7/inode.7.html), [`chmod(2)`](https://man7.org/linux/man-pages/man2/chmod.2.html), [`chown(2)`](https://man7.org/linux/man-pages/man2/chown.2.html), [`umask(2)`](https://man7.org/linux/man-pages/man2/umask.2.html), [`access(2)`](https://man7.org/linux/man-pages/man2/access.2.html), [`rename(2)`](https://man7.org/linux/man-pages/man2/rename.2.html), [`link(2)`](https://man7.org/linux/man-pages/man2/link.2.html), [`unlink(2)`](https://man7.org/linux/man-pages/man2/unlink.2.html), [`symlink(2)`](https://man7.org/linux/man-pages/man2/symlink.2.html), [`readlink(2)`](https://man7.org/linux/man-pages/man2/readlink.2.html), [`mkdir(2)`](https://man7.org/linux/man-pages/man2/mkdir.2.html), [`rmdir(2)`](https://man7.org/linux/man-pages/man2/rmdir.2.html), [`opendir(3)`](https://man7.org/linux/man-pages/man3/opendir.3.html), [`readdir(3)`](https://man7.org/linux/man-pages/man3/readdir.3.html), [`flock(2)`](https://man7.org/linux/man-pages/man2/flock.2.html), [`fcntl_locking(2)`](https://man7.org/linux/man-pages/man2/fcntl_locking.2.html), [`inotify(7)`](https://man7.org/linux/man-pages/man7/inotify.7.html), [`xattr(7)`](https://man7.org/linux/man-pages/man7/xattr.7.html), and [`acl(5)`](https://man7.org/linux/man-pages/man5/acl.5.html).

Interview calibration sources were used only to prioritize realistic interview style and topic frequency, not as technical authority:

- Amazon official SDE prep emphasizes applied coding, system design, operating systems, robust tested code, edge cases, and trade-off discussions rather than memorizing details: [Software development interview topics](https://www.amazon.jobs/content/en-gb/how-we-hire/interview-prep/software-development-topics), [SDE II interview prep](https://www.amazon.jobs/content/es/how-we-hire/sde-ii-interview-prep).
- Microsoft official guidance frames technical interviews around problem solving, technical principles, design, testing, edge cases, and strategic trade-offs: [Technical interviewing](https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html).
- Google official prep resources and role descriptions emphasize technical interview practice plus large-scale software concerns such as storage, networking, security, testing, and software design: [Google Tech Dev Guide interview resources](https://techdevguide.withgoogle.com/paths/interview/), [Google hiring process](https://www.google.com/about/careers/applications/how-we-hire/).
- Meta official SWE full-loop prep was checked for interview-loop framing and technical-skill assessment style: [Preparing for Your Full Loop Interview at Meta](https://www.metacareers.com/careers/SWE-prep-onsite).
- Recurring OS/Linux interview banks were sampled only to detect repeated themes such as file descriptors, inode/link identity, permissions, buffering, and locking: [Baeldung Linux interview questions](https://www.baeldung.com/linux/linux-interview-questions), [GeeksforGeeks File & I/O Management Interview Questions](https://www.geeksforgeeks.org/operating-systems/file-i-o-management-interview-questions/).

Linux/Embedded job-market calibration sources were also used only to prioritize realistic project scenarios, not as technical authority:

- Bosch Vietnam Embedded/Linux roles mention Linux, POSIX/Linux kernel/QNX, C/C++, debugging, `strace`, `gdb`, `valgrind`, system-level components, and embedded firmware work: [Embedded Firmware Engineer (Linux)](https://jobs.smartrecruiters.com/BoschGroup/744000117552417--eta-embedded-firmware-engineer-linux-?trid=7d1dcdfa-96a8-4e55-bb9c-0db211f5a9b3), [Senior Embedded Driver Engineer](https://jobs.smartrecruiters.com/BoschGroup/744000099718095-embedded-software-engineer-posix-os-linux-?oga=true), [Senior Engineer C++ / Linux](https://jobs.smartrecruiters.com/BoschGroup/744000089405734-senior-engineer-c-linux).
- Renesas Vietnam roles emphasize embedded Linux, system programming, integration, drivers, open-source software, and architecture on SoC platforms: [Renesas Staff Software Engineer](https://jobs.renesas.com/job/principal-software-engineer-open-source-software-linux-zephyr-in-ho-chi-minh-city-ho-chi-minh-vietnam-jid-3093).
- Ampere Vietnam firmware roles emphasize robust firmware, SoC production systems, debugging, device driver, RTOS, and Linux software development: [Ampere System Control Firmware Engineer](https://careers.amperecomputing.com/jobs/17547710-technical-intern-system-control-firmware-engineer).
- Viettel High Tech Linux embedded roles emphasize C/C++, Embedded Linux, cross-compilation, `gdb`, logs, `dmesg`, `strace`, POSIX, pthreads, IPC, sockets, shared memory, Yocto, Buildroot, and driver basics: [Viettel High Tech Senior Embedded Software Engineer (Linux)](https://vn.linkedin.com/jobs/view/senior-embedded-software-engineer-linux-at-viettel-high-tech-4393430263).
- Vietnam job-market signals from ITviec show repeated demand for Embedded Linux, C/C++, user-space interaction with Linux drivers, networking, multi-threaded software, and high-performance embedded applications: [Datalogic Vietnam Embedded Linux Engineer](https://itviec.com/it-jobs/embedded-linux-engineer-c-linux-datalogic-viet-nam-4605), [Motorola Solutions Embedded Software Engineer](https://itviec.com/it-jobs/embedded-software-engineer-c-linux-motorola-solutions-1709), [GLOBALTECH Embedded Linux Engineer](https://itviec.com/it-jobs/ky-su-phat-trien-nhung-c-c-linux-embedded-globaltech-1750).

---

## Coverage Trace

This trace is the interview coverage gate for Chapter 2. Every learning-map row and Must Cover concept appears in Priority A, B, or C; important concepts are tested through scenarios or comparisons rather than hidden only in keywords.

| Coverage item | Required interview coverage | Priority coverage | Status |
|---------------|-----------------------------|-------------------|--------|
| 2.1 Universal I/O: `open/read/write/close`, FD, errors, short I/O | Robust loops, EOF/error distinction, binary buffers, delayed `close()` errors, device-style short I/O | A6, B15, C `ioctl()` | Covered |
| 2.2 Further I/O: `fcntl`, OFD, `dup`, `pread/pwrite`, scatter-gather | FD vs OFD, status vs descriptor flags, inheritance, offset-independent I/O, scatter-gather recognition | A4, A5, B16-B18, C `readv/writev`, C `fcntl` | Covered |
| 2.3 Buffering: stdio, page cache, `fsync`, `O_DIRECT`, embedded storage | `fflush`/`fsync`, parent directory sync, stdio controls, mixed stdio/raw I/O, direct/sync tradeoffs | A1-A3, A9, B19, B26, B27 | Covered |
| 2.4 VFS/filesystems/inode/mount/metadata | Inode identity, mount boundaries, `rename()`/`EXDEV`, deleted-open files, timestamp metadata | A7, A9, A13, B15, B22, B31 | Covered |
| 2.5 attributes/permissions: `stat`, ownership, `umask`, special bits | Path permissions, effective credentials, ACL mask, timestamp mutation, SUID/SGID/sticky side effects | A8, A14, B20, B25, B31, B32 | Covered |
| 2.6 directories/links: hard link, symlink, `opendir/readdir`, `chroot` | Link identity, directory traversal, `openat()`/TOCTOU, `chroot()` limits, temp-file and `/dev/fd` patterns | A7, A12, A13, B21, B28-B30, C `/dev/fd` | Covered |
| 2.7 locking: `flock`, `fcntl` record locks, mandatory locking | PID-file lifecycle, advisory model, record-lock failure handling, mandatory locking recognition, stdio+locks | A10, B23, B33, C mandatory locking | Covered |
| 2.8 monitoring file events: `inotify` | Instance/watch/event queue lifecycle, replacement events, self-delete/self-move/unmount/ignored events, overflow recovery | A11, B24, B34, C lifecycle events | Covered |
| 2.9 xattr and ACL | xattr purpose, ACL policy, `ACL_MASK`, deployment/copy metadata loss, debug tools | A14, B25, C ACL C API | Covered |
| Must: production race/debug workflows | `strace`, `/proc`, `lsof`, `lslocks`, `getfacl`, `namei`, `findmnt`, inotify limits, failure-oriented debugging | A1-A14, Common Project Failure Patterns | Covered |
| Must: Embedded constraints | Power loss, slow sync, flash wear, read-only/full media, mount/filesystem behavior, event-loop integration | A3, B26, B34, C `statvfs()` | Covered |

---

## Priority Map

| Priority | How to study | Interview shape |
|----------|--------------|-----------------|
| A - Project and production scenarios | Answer deeply. Start from the real bug or design, then explain the kernel mechanism, pitfalls, and debug commands. | "How would you design/debug this in a project?" |
| B - Design comparisons and senior follow-ups | Know the trade-off and when each option is appropriate. Keep answers compact but precise. | "Compare A vs B. Which one would you use?" |
| C - Recognize only | Know the name, the rough use case, and when to read the manual. Do not memorize every flag/API. | "Have you seen this? When might it matter?" |

---

## Final Interview List

### A - High-Probability Scenario Questions

1. Multiple processes write logs to the same file. How do you avoid corrupted or interleaved output?
2. Your service reports "config saved", but the new config disappears after power loss. What went wrong?
3. An embedded device must update a state file safely without wearing out flash. How would you design the write path?
4. A daemon opens files, then starts child processes. Later a child keeps a socket/log/config file open unexpectedly. How do you prevent and debug this?
5. A parent and child, or two library layers, unexpectedly share file offset. Why does this happen?
6. A file copy or device-read loop sometimes loses bytes or treats binary data as a string. What is the correct I/O pattern?
7. `rm large.log` was executed, but disk usage is still high. How do you explain and debug it?
8. A process gets `Permission denied`, but `ls -l` shows the file is readable. What do you check?
9. A program updates a config by writing a temp file and renaming it. Why is this safer, and what details are easy to miss?
10. A service must guarantee only one instance is running. Would you use a PID file, `flock()`, `fcntl()`, or something else?
11. A config hot-reload watcher misses updates after deployment. What are the common `inotify` traps?
12. A tool scans a user-controlled directory. How do you avoid symlink/path races and wrong-file bugs?
13. Two pathnames appear to refer to the same data. How do you prove whether they are the same file?
14. A shared project directory has strange group permissions. How do `umask`, SGID directories, ACL, and `ACL_MASK` interact?

### B - Design Comparisons and Senior Follow-Ups

15. File descriptor vs open file description vs inode.
16. `open()` vs `dup()` vs `fork()` descriptor inheritance.
17. `O_APPEND` vs `lseek(SEEK_END) + write()`.
18. `pread()`/`pwrite()` vs `lseek() + read()/write()`.
19. `fflush()` vs `fsync()` vs `fdatasync()`.
20. `stat()` vs `lstat()` vs `fstat()`.
21. Hard link vs symbolic link.
22. `rename()` within one filesystem vs cross-filesystem move.
23. `flock()` vs `fcntl()` record locking.
24. `inotify` vs polling.
25. Classic owner/group/other permissions vs ACL.
26. Buffered I/O vs `O_SYNC`/`O_DSYNC`/`O_DIRECT`.
27. stdio buffering controls vs raw syscalls.
28. `openat()`/directory-FD APIs vs path-string APIs.
29. `chroot()` isolation vs real sandboxing.
30. `unlink()` vs `truncate()` vs `ftruncate()`.
31. timestamp metadata reads vs timestamp mutation APIs.
32. SUID/SGID/sticky bits: execution, inheritance, and delete rules.
33. advisory locking vs mandatory locking.
34. `inotify` object-lifecycle events vs content-change events.

### C - Lower-Priority / Know Enough to Recognize

- `readv()` / `writev()`: scatter-gather I/O for multiple buffers.
- `fcntl(F_GETFL/F_SETFL)`: inspect or change file status flags.
- `setvbuf()` / `setbuf()`: choose full, line, or no buffering for a `FILE *` stream before I/O.
- `access()` / `faccessat()`: recognize why pre-checking access can race or disagree with `open()`, especially in privileged code.
- `ioctl()`: device/object-specific operations outside universal I/O.
- `creat()`: historical shorthand for create/truncate/write-only open.
- `truncate()` / `ftruncate()`: resize files, discard data, extend with holes, and mutate metadata.
- `/dev/fd`, `/proc/self/fd`, `mkstemp()`, `tmpfile()`: descriptor naming and temporary-file patterns.
- `posix_fadvise()`: kernel access-pattern hint, not correctness.
- `sync()` / `syncfs()`: broader writeback controls.
- `O_TMPFILE`: advanced unnamed temporary-file workflow; publishing still needs correct link and durability handling.
- mandatory locking: rare Linux feature with special setup; usually avoided.
- `utime()` / `utimes()` / `futimens()`: timestamp mutation APIs with ownership/permission rules.
- `IN_DELETE_SELF`, `IN_MOVE_SELF`, `IN_IGNORED`, `IN_UNMOUNT`: important watch lifecycle events.
- `dnotify` / `fanotify`: older or broader file notification mechanisms.
- ACL C API: useful but verbose; debug first with `getfacl` and `setfacl`.
- `statvfs()`, `nftw()`, FUSE, COW filesystems, large-file macros, filesystem-specific timestamp details.

---

## High-Value Comparisons

| Comparison | Practical interview answer |
|------------|----------------------------|
| FD vs open file description vs inode | FD is the process-local integer handle. Open file description is kernel runtime state such as offset and status flags. Inode is filesystem identity and metadata. |
| `open()` vs `dup()` | `open()` usually creates a new open file description. `dup()` creates another FD pointing to the same open file description. |
| FD flags vs file status flags | `FD_CLOEXEC` lives on one FD entry. `O_APPEND` and `O_NONBLOCK` live in the shared open file description. |
| `O_APPEND` vs `lseek() + write()` | `O_APPEND` makes append positioning and writing one atomic kernel operation. `lseek() + write()` can race. |
| `pread()` vs `lseek() + read()` | `pread()` uses an explicit offset without changing shared file offset. `lseek() + read()` can race in shared-offset code. |
| `fflush()` vs `fsync()` | `fflush()` drains libc buffers to the kernel. `fsync()` pushes kernel-managed file state toward storage. |
| `fsync()` vs `fdatasync()` | `fsync()` covers data and relevant metadata. `fdatasync()` focuses on file data and required metadata. |
| `stat()` vs `lstat()` vs `fstat()` | `stat()` follows symlinks. `lstat()` inspects the symlink itself. `fstat()` inspects an already-open object. |
| Hard link vs symlink | Hard link is another name for the same inode. Symlink is a separate file containing a pathname. |
| `unlink()` vs `rename()` | `unlink()` removes a directory entry. `rename()` atomically changes directory entries within one filesystem. |
| File permissions vs directory permissions | File bits control content access. Directory bits control listing, entry creation/removal, and path traversal. |
| `flock()` vs `fcntl()` | `flock()` is simple whole-file locking tied to the open file description. `fcntl()` gives byte-range locks but has trickier process and close semantics. |
| `inotify` vs polling | `inotify` is event-driven and efficient. Polling repeatedly asks for state and may be slow or incomplete. |
| xattr vs ACL | xattr stores extended metadata. ACL is permission policy, commonly stored using system xattrs on Linux. |

---

## Common Project Failure Patterns

| Failure pattern | Production symptom | Interview-grade fix or debug path |
|-----------------|--------------------|-----------------------------------|
| Treating `write()` as complete | Output is truncated or protocol frames are missing. | Loop on returned byte counts; handle `EINTR`, `EAGAIN`, and fatal errors separately. |
| Treating `write()` as durable | Config or state disappears after crash or power loss. | Use the correct `fflush()`/`fsync()`/`fdatasync()`/`rename()` sequence and check `close()`. |
| Treating binary data as a C string | Logs show garbage, data is cut at `'\0'`, or buffers overrun. | Track explicit byte counts; never assume `read()` null-terminates. |
| Using `lseek(SEEK_END) + write()` for append | Concurrent log writers overwrite or interleave records. | Open with `O_APPEND`; consider locks if a logical record spans multiple writes. |
| Sharing an FD without noticing the shared offset | Parent/child or two threads skip or duplicate data. | Understand open file descriptions; use separate `open()` calls or `pread()`/`pwrite()`. |
| Leaking FDs across `exec()` | Child process keeps logs, sockets, pipes, locks, or deleted files alive. | Use `O_CLOEXEC`, `pipe2(O_CLOEXEC)`, `dup3(O_CLOEXEC)`, and inspect `/proc/<PID>/fd`. |
| Updating config in place | Readers see half-written files. | Write a temp file in the same directory, sync it when needed, then `rename()`. |
| Forgetting parent directory `fsync()` | Data file exists but the name update can be lost after crash. | Sync the file for content and the parent directory for durable directory entry changes. |
| Deleting an active log | `df -h` stays high after `rm large.log`. | Find deleted-open files with `lsof +L1` and make the daemon reopen or close the FD. |
| Trusting path strings in user-controlled directories | Symlink race or wrong-file overwrite. | Prefer directory-FD-relative APIs, `lstat()`, `fstat()` after open, and `O_NOFOLLOW` where appropriate. |
| Confusing `stat()` and `lstat()` | Scanner follows a symlink when it meant to inspect the link. | Use `lstat()`/`readlink()` for link identity; use `stat()` for the target. |
| Watching only one file with `inotify` | Hot reload misses atomic replace deployments. | Watch the parent directory, handle rename/create/delete events, and rescan after overflow. |
| Trusting PID files without locks | A stale PID blocks startup or two instances run. | Use PID file plus nonblocking `flock()` and keep the lock FD open. |
| Ignoring ACL mask | `ls -l` appears permissive but access fails. | Debug with `getfacl`; remember `ACL_MASK` caps group-class effective permissions. |

---

## Detailed Answers - Priority A

### 1. Multiple processes write logs to the same file. How do you avoid corrupted or interleaved output?

**What the interviewer is testing**

- Atomic append.
- Shared file offset.
- Partial writes.
- stdio buffering vs raw `write()`.
- Durability vs throughput.

**Strong answer**

Open the log file with `O_APPEND`, and write each complete log record with one write-like operation when possible. `O_APPEND` makes the kernel choose end-of-file and perform the write as one append operation, which avoids the race in `lseek(fd, 0, SEEK_END) + write()`.

If logs are not crash-critical, do not call `fsync()` after every line. That can destroy throughput, increase latency, and cause unnecessary flash wear on embedded devices. Use batching, periodic sync, or accept best-effort logging depending on product requirements.

**Mechanism**

The file offset lives in the open file description. Without `O_APPEND`, multiple writers can observe the same old end position and overwrite or interleave data. With `O_APPEND`, append positioning is handled inside the kernel for the write.

**Pitfalls**

- `lseek(SEEK_END) + write()` is not atomic.
- `write()` can still be partial, especially for pipes, sockets, nonblocking FDs, or error paths.
- stdio can split or delay output.
- Network filesystems may have weaker behavior than local filesystems.
- `write()` success does not mean data is durable on storage.

**Debug angle**

Use `strace -e openat,write,lseek -p <PID>` to see whether the process opens with append and whether it does unsafe seeks. Use `lsof -p <PID>` or `/proc/<PID>/fd` to inspect open log descriptors.

**Follow-up keywords**

`O_APPEND`, open file description, file offset, partial write, `write()`, stdio buffering, `fsync()`.

### 2. Your service reports "config saved", but the new config disappears after power loss. What went wrong?

**What the interviewer is testing**

- `write()` is not durability.
- stdio buffer vs page cache.
- `fsync()` and parent-directory sync.
- Atomic replacement.
- Crash consistency.

**Strong answer**

A successful `write()` only means the kernel accepted the bytes. It does not prove the data and the directory entry reached stable storage. For an important config update, write a temp file in the same directory, flush libc if used, `fsync()` or `fdatasync()` the temp file, close and check errors, `rename()` it over the old config, then `fsync()` the parent directory if the name update must survive a crash.

**Mechanism**

Regular file writes usually go into the kernel page cache first. Filesystem metadata, file data, and directory entries can be persisted at different times. `rename()` gives atomic name replacement within one filesystem, but the directory update also needs durability when crash recovery matters.

**Pitfalls**

- `fflush()` is not `fsync()`.
- `fsync()` the file but forget the parent directory.
- Write the temp file in another filesystem, causing `rename()` to fail with `EXDEV`.
- Ignore errors from `fsync()` or `close()`.
- Assume journaling means the application data is durable.

**Debug angle**

Use `strace -e openat,write,fsync,fdatasync,rename,close` to inspect the actual sequence. Check mount type and options with `findmnt`. Reproduce with power-fail or crash testing if the product requires this guarantee.

**Follow-up keywords**

page cache, `fflush()`, `fsync()`, `fdatasync()`, `rename()`, directory `fsync()`, journaling, `close()`.

### 3. An embedded device must update a state file safely without wearing out flash. How would you design the write path?

**What the interviewer is testing**

- Embedded Linux constraints.
- Durability policy.
- Flash wear.
- Atomic update.
- Filesystem-dependent behavior.

**Strong answer**

Separate correctness requirements from performance requirements. If state must survive sudden power loss, use the temp-file, `fsync()`, `rename()`, parent-directory `fsync()` pattern. If state changes frequently, batch updates, coalesce changes, use checkpoints, or store only critical state immediately. Avoid `fsync()` on every minor update unless the requirement justifies latency and flash wear.

**Mechanism**

Linux buffers writes for performance. Flash storage has finite program/erase cycles, and embedded filesystems or eMMC/SD behavior can differ from desktop SSDs. Crash-safe updates require explicit synchronization, but synchronization frequency is a product-level trade-off.

**Pitfalls**

- Treating logs and critical state with the same durability policy.
- Using `O_SYNC` everywhere instead of designing update points.
- Forgetting that `rename()` atomicity is same-filesystem.
- Ignoring storage-specific failure modes such as full disk, read-only remount, or delayed I/O errors.

**Debug angle**

Check `dmesg` for I/O errors or read-only remounts. Use `df -h`, `df -i`, `findmnt`, `strace`, and fault-injection tests. On embedded targets, test on the real storage stack, not only on a PC.

**Follow-up keywords**

page cache, `fsync()`, `fdatasync()`, `O_SYNC`, `O_DSYNC`, `rename()`, flash wear, mount options.

### 4. A daemon opens files, then starts child processes. Later a child keeps a socket/log/config file open unexpectedly. How do you prevent and debug this?

**What the interviewer is testing**

- FD inheritance across `exec()`.
- `O_CLOEXEC` and `FD_CLOEXEC`.
- Descriptor leaks.
- Process lifecycle debugging.

**Strong answer**

Use close-on-exec by default for descriptors that should not be inherited. Prefer atomic forms such as `open(..., O_CLOEXEC)`, `pipe2(O_CLOEXEC)`, `dup3(..., O_CLOEXEC)`, and APIs that support close-on-exec at creation time. Setting `FD_CLOEXEC` later with `fcntl()` can race in multithreaded programs.

**Mechanism**

`fork()` copies the file descriptor table. `exec()` replaces the program image but keeps descriptors open unless close-on-exec is set. A leaked FD can keep a socket, pipe, deleted log, or lock alive after the parent thinks it is gone.

**Pitfalls**

- Open first, set `FD_CLOEXEC` later, and race with another thread doing `fork()` plus `exec()`.
- Forget to close unused pipe ends in children.
- Accidentally pass privileged directory FDs or device FDs to helper programs.
- Debug only the parent while the child owns the leaked reference.

**Debug angle**

Inspect `/proc/<PID>/fd` and `/proc/<PID>/fdinfo/<N>`. Use `lsof -p <PID>`. Trace process creation and descriptor flow with `strace -f -e openat,close,fcntl,dup,dup2,dup3,execve`.

**Follow-up keywords**

FD table, `fork()`, `exec()`, `O_CLOEXEC`, `FD_CLOEXEC`, descriptor leak, `/proc/<PID>/fd`.

### 5. A parent and child, or two library layers, unexpectedly share file offset. Why does this happen?

**What the interviewer is testing**

- File descriptor table vs open file description.
- `dup()` and `fork()` sharing.
- Offset races.
- `pread()` / `pwrite()`.

**Strong answer**

They may share the same open file description. `dup()` creates a new FD pointing to the same open file description, and `fork()` copies FDs that still point to the same open file descriptions. Since the offset is stored in the open file description, a seek or sequential read through one FD can affect the other.

**Mechanism**

The FD is only a process-local handle. The open file description is the kernel object containing file offset and status flags. Two separate `open()` calls usually create independent open file descriptions, even for the same pathname.

**Pitfalls**

- Use `lseek()` in one part of the program while another part expects sequential reads.
- Share an FD across `fork()` and then both parent and child read from it.
- Assume `dup()` creates an independent cursor.
- Use `lseek() + read()` in multithreaded random-access code.

**Debug angle**

Inspect `/proc/<PID>/fdinfo/<fd>` for position and flags. Trace `lseek`, `read`, and `write`. Reproduce with a small test that uses `dup()` or `fork()` and prints offsets.

**Follow-up keywords**

FD table, open file description, inode, `dup()`, `fork()`, `lseek()`, `pread()`, `pwrite()`.

### 6. A file copy or device-read loop sometimes loses bytes or treats binary data as a string. What is the correct I/O pattern?

**What the interviewer is testing**

- Partial `read()` and `write()`.
- EOF vs error.
- Binary-safe buffers.
- `EINTR` and nonblocking behavior.

**Strong answer**

Always use the return value. `read()` returns the number of bytes actually read, `0` for EOF on regular files, or `-1` for error. `write()` returns the number of bytes actually accepted, which can be less than requested. A correct loop keeps track of how much remains and continues until complete, EOF, or a real error.

**Mechanism**

Linux system calls move bytes, not C strings. `read()` does not add a null terminator. Pipes, sockets, terminals, devices, and nonblocking descriptors often transfer less than requested.

**Pitfalls**

- Assume one `read()` fills the buffer.
- Assume one `write()` writes all bytes.
- Print binary buffers with `%s`.
- Retry every error blindly without understanding `EINTR`, `EAGAIN`, or fatal errors.
- Forget that `close()` can report delayed write failure on important output.

**Debug angle**

Use `strace -e read,write,close` to see real byte counts. Log the return values. For device or pipe-like behavior, test short transfers deliberately.

**Follow-up keywords**

partial read, partial write, EOF, `errno`, `EINTR`, `EAGAIN`, binary buffer, robust write loop.

### 7. `rm large.log` was executed, but disk usage is still high. How do you explain and debug it?

**What the interviewer is testing**

- `unlink()` semantics.
- Inode link count.
- Open file references.
- Production disk debugging.

**Strong answer**

`rm` removes a directory entry. The file's storage is freed only when the link count is zero and no open file description still references the file. If a process still has the deleted log open, the pathname is gone, but the inode and blocks remain.

**Mechanism**

Files are named by directory entries, but storage belongs to the inode. `unlink()` decrements the link count. Open FDs keep the underlying file object alive until they are closed.

**Pitfalls**

- Restart log rotation without signaling the service to reopen logs.
- Delete a log that a daemon keeps writing.
- Diagnose only with `ls`, which can no longer see the unlinked name.
- Forget inode exhaustion as a separate issue from block exhaustion.

**Debug angle**

Run `lsof +L1` to find deleted-open files. Inspect `/proc/<PID>/fd`. Use `df -h` and `df -i`. Fix by closing/reopening the FD, sending the service reload signal, or restarting the process.

**Follow-up keywords**

`unlink()`, inode, link count, open file description, deleted-open file, `lsof +L1`.

### 8. A process gets `Permission denied`, but `ls -l` shows the file is readable. What do you check?

**What the interviewer is testing**

- Directory permissions.
- Process credentials.
- ACLs.
- Symlinks and mount context.
- Practical debugging.

**Strong answer**

Check every directory in the path, not only the final file. Directory execute permission means search/traverse. Without it, a readable file can still be inaccessible. Then check effective UID/GID, supplementary groups, ACLs, symlink target, mount options, and whether the operation is read, write, create, delete, or execute.

**Mechanism**

Path lookup checks permissions component by component. For files, read/write/execute apply to contents or execution. For directories, read lists names, write creates/removes/renames entries, and execute traverses the directory.

**Pitfalls**

- Think file mode alone decides access.
- Forget supplementary groups.
- Ignore ACL mask.
- Use `access()` in privileged code and assume it predicts `open()`.
- Miss sticky-bit behavior in shared writable directories.

**Debug angle**

Use `namei -l /path/to/file`, `id`, `stat`, `getfacl`, `readlink -f`, `findmnt`, and `strace -e openat` to find the exact failing path component or syscall.

**Follow-up keywords**

directory execute bit, effective UID/GID, supplementary groups, ACL, `ACL_MASK`, sticky bit, `access()`.

### 9. A program updates a config by writing a temp file and renaming it. Why is this safer, and what details are easy to miss?

**What the interviewer is testing**

- Atomic `rename()`.
- Same-filesystem requirement.
- Crash-safe update sequence.
- Reader consistency.

**Strong answer**

Writing a temp file and renaming it over the target prevents readers from seeing a half-written final file. Within one filesystem, `rename()` atomically changes the directory entry, so readers see either the old config or the new config. If crash durability matters, the sequence is temp file write, `fsync()` or `fdatasync()` the temp file, checked `close()`, `rename()`, then `fsync()` the parent directory.

**Mechanism**

`rename()` updates directory entries. It does not copy file data within the same filesystem. Across filesystems it fails with `EXDEV`; user-space `mv` may fall back to copy-and-delete, but the syscall itself does not.

**Pitfalls**

- Temp file is created in `/tmp` while target is in `/etc/app`, causing cross-filesystem issues.
- Forget to `fsync()` the temp file for content durability.
- Forget to sync the parent directory for name durability.
- Preserve wrong mode, owner, xattrs, or ACLs.
- Watch only the old file inode with `inotify` and miss replacement.

**Debug angle**

Trace `openat`, `write`, `fsync`, `rename`, `close`, and the parent-directory open/sync. Use `stat` before and after to see inode changes. Use `findmnt` to verify both paths are on the same filesystem.

**Follow-up keywords**

`rename()`, `EXDEV`, temp file, directory entry, inode, `fsync()`, directory `fsync()`, checked `close()`, mode preservation.

### 10. A service must guarantee only one instance is running. Would you use a PID file, `flock()`, `fcntl()`, or something else?

**What the interviewer is testing**

- Advisory locking.
- PID-file pattern.
- `flock()` vs `fcntl()`.
- Stale PID files.
- Operational behavior.

**Strong answer**

For a simple Linux daemon, use a PID file plus a nonblocking exclusive `flock()` and keep the FD open for the service lifetime. The lock is the authority, not the text PID alone. After acquiring the lock, truncate/rewrite the PID content, flush if operationally useful, and keep clear signal/shutdown cleanup. Stale PID files are normal after crashes, but stale locks disappear when the process exits and the FD closes.

**Mechanism**

`flock()` is whole-file advisory locking associated with the open file description. Cooperating processes must use the same locking protocol. `fcntl()` record locks are useful for byte ranges but have more surprising close semantics.

**Pitfalls**

- Check "file exists" and then create it without an atomic lock.
- Trust a stale PID file without checking the lock.
- Close the lock FD too early.
- Forget to truncate old PID content before writing a shorter PID.
- Treat `EINTR`, `EAGAIN`, or `EACCES` as the same generic lock failure.
- Mix `flock()` and `fcntl()` and expect portable interaction.
- Block forever without a clear error message for CLI tools.

**Debug angle**

Use `lslocks`, `/proc/locks`, and `lsof` on the PID file. Trace `flock` or `fcntl`. Log whether the failure was conflict, interruption, permission, or an unexpected syscall error.

**Follow-up keywords**

advisory lock, `flock()`, `fcntl()`, PID file, stale PID, `EINTR`, `EAGAIN`, `EACCES`, `O_EXCL`, `/proc/locks`.

### 11. A config hot-reload watcher misses updates after deployment. What are the common `inotify` traps?

**What the interviewer is testing**

- `inotify` watches and event queue.
- Directory vs file watch.
- Atomic replacement behavior.
- Overflow recovery.

**Strong answer**

Watch the directory, not only the file, when deployments replace files with temp-file plus `rename()`. Handle `IN_MOVED_TO`, `IN_CREATE`, `IN_DELETE`, `IN_ATTRIB`, and lifecycle events such as `IN_DELETE_SELF`, `IN_MOVE_SELF`, `IN_IGNORED`, and `IN_UNMOUNT` as appropriate. If `IN_Q_OVERFLOW` happens, discard cached assumptions and rescan state.

**Mechanism**

An `inotify` instance has watch descriptors and an event queue read through a file descriptor. File replacement often creates a new inode, so a watch on the old file object may not describe the new config path.

**Pitfalls**

- Assume directory watches are recursive.
- Ignore queue overflow.
- Miss watch invalidation after delete, move, explicit removal, or unmount.
- Count events exactly even though events may be coalesced.
- Fail to re-add watches after directories are created or replaced.
- Assume behavior is identical on all network or unusual filesystems.

**Debug angle**

Check `/proc/sys/fs/inotify/max_user_watches`, `max_user_instances`, and `max_queued_events`. Log raw event masks and watch descriptors. Reproduce deployment with the exact temp-file plus `rename()` pattern.

**Follow-up keywords**

`inotify`, watch descriptor, event queue, `IN_Q_OVERFLOW`, `IN_IGNORED`, `IN_DELETE_SELF`, `IN_MOVE_SELF`, rename cookie, directory watch, rescan.

### 12. A tool scans a user-controlled directory. How do you avoid symlink/path races and wrong-file bugs?

**What the interviewer is testing**

- Pathname lookup races.
- `stat()` vs `lstat()`.
- Directory FDs and `openat()`.
- Symlink safety.

**Strong answer**

Do not trust path strings after a separate check. Use directory-FD-relative APIs such as `openat()` and `fstatat()` where possible. Use `lstat()` when you need to inspect the link itself, `O_NOFOLLOW` when opening must not follow a final symlink, and `fstat()` after opening to verify the object you actually got. Directory FDs also avoid surprises from global current-working-directory changes and can model a safer per-operation working root.

**Mechanism**

Pathnames are names that can be changed by other processes. A check-then-open sequence can race if an attacker replaces a file or directory entry between calls.

**Pitfalls**

- `stat()` follows symlinks when the code intended to inspect the symlink.
- Build paths with strings and rely on current working directory.
- Trust `d_type` from `readdir()` on every filesystem.
- Forget mount points and bind mounts can change what a path means.
- Treat `chroot()` as a complete sandbox while still holding useful FDs or privileges.

**Debug angle**

Use `strace -e openat,newfstatat,readlink` to inspect actual path operations. Use `namei -l` and `findmnt` to understand path and mount traversal.

**Follow-up keywords**

`openat()`, `fstatat()`, `stat()`, `lstat()`, `fstat()`, `O_NOFOLLOW`, symlink race, TOCTOU, directory FD, `chroot()`.

### 13. Two pathnames appear to refer to the same data. How do you prove whether they are the same file?

**What the interviewer is testing**

- Inode identity.
- Hard links.
- Symlinks.
- `stat()` fields.

**Strong answer**

Compare `st_dev` and `st_ino` from `stat()` or `fstat()`. If both match, the two names refer to the same inode on the same filesystem. For symlinks, use `lstat()` to inspect the link itself and `stat()` to inspect the target.

**Mechanism**

The filename is a directory entry. The inode is the file identity within a filesystem. Hard links are multiple directory entries pointing to the same inode.

**Pitfalls**

- Compare only pathname strings.
- Compare only inode number without device ID.
- Confuse a symlink with the target it points to.
- Assume `rename()` means data was copied.

**Debug angle**

Use `ls -li`, `stat`, `find . -samefile file`, and `readlink`. Inspect `st_nlink` for hard-link count.

**Follow-up keywords**

inode, directory entry, hard link, symlink, `st_dev`, `st_ino`, `st_nlink`.

### 14. A shared project directory has strange group permissions. How do `umask`, SGID directories, ACL, and `ACL_MASK` interact?

**What the interviewer is testing**

- File creation permissions.
- Shared directory design.
- ACL effective permissions.
- Debugging `getfacl` output.

**Strong answer**

For classic permissions, new object modes are filtered by the process `umask`. On an SGID directory, new files usually inherit the directory group, which helps team sharing. ACLs add per-user and per-group entries; when extended ACLs exist, `ACL_MASK` caps effective permissions for the group class.

**Mechanism**

`umask` removes requested permission bits at creation time. SGID on directories affects group inheritance. Linux ACLs extend owner/group/other policy and are commonly stored as system xattrs. The ACL mask can make visible group-like permissions lower than an entry appears to request.

**Pitfalls**

- Fix only `chmod` while the real problem is `umask`.
- Ignore SGID on shared directories.
- Forget that ownership changes can clear SUID or SGID bits for security reasons.
- Read `ls -l` group bits without checking `getfacl`.
- Forget that default ACLs affect newly created children, not old files.

**Debug angle**

Use `umask`, `id`, `ls -ld`, `stat`, `getfacl`, and `setfacl`. For new-file behavior, create a test file from the same service user and inspect the result.

**Follow-up keywords**

`umask`, SGID directory, sticky bit, ACL, `ACL_MASK`, default ACL, xattr.

---

## Short Answers - Priority B

### 15. File descriptor vs open file description vs inode

FD is the per-process integer handle. The open file description is kernel runtime state: offset, status flags, and access mode. The inode is filesystem identity and metadata. Most offset-sharing bugs are explained by this model.

### 16. `open()` vs `dup()` vs `fork()` descriptor inheritance

`open()` usually creates a new open file description. `dup()` creates another FD pointing to the same open file description. `fork()` copies FD table entries that still point to the same open file descriptions.

### 17. `O_APPEND` vs `lseek(SEEK_END) + write()`

`O_APPEND` makes append positioning and writing atomic for that write operation. `lseek() + write()` is a two-step sequence and can race between processes.

### 18. `pread()`/`pwrite()` vs `lseek() + read()/write()`

`pread()` and `pwrite()` use explicit offsets without changing the shared current offset. They are the right tool for concurrent random access through a shared open file description.

### 19. `fflush()` vs `fsync()` vs `fdatasync()`

`fflush()` moves data from libc to the kernel. `fsync()` asks the kernel/filesystem to persist file data and metadata. `fdatasync()` focuses on data and required metadata, often with less metadata cost.

### 20. `stat()` vs `lstat()` vs `fstat()`

`stat()` follows a symlink to its target. `lstat()` inspects the symlink itself. `fstat()` inspects the already-open file object and avoids another pathname lookup.

### 21. Hard link vs symbolic link

A hard link is another directory entry for the same inode. A symlink is a separate file containing a pathname. Hard links cannot normally cross filesystems; symlinks can dangle.

### 22. `rename()` within one filesystem vs cross-filesystem move

Same-filesystem `rename()` is atomic and changes directory entries. Cross-filesystem `rename()` fails with `EXDEV`; tools may simulate a move by copy-and-delete, which is not the same atomic operation.

### 23. `flock()` vs `fcntl()` record locking

`flock()` is simple whole-file locking tied to the open file description. `fcntl()` supports byte-range locks but has more surprising process and close behavior. Pick one family per shared file protocol.

### 24. `inotify` vs polling

`inotify` is event-driven and integrates with event loops through an FD. Polling is simpler but slower and often incomplete. `inotify` still needs overflow handling and rescan logic.

### 25. Classic permissions vs ACL

Classic permissions cover owner, group, and other. ACL adds per-user and per-group entries. On ACL files, the mask can cap effective group-class permissions and make `ls -l` misleading without `getfacl`.

### 26. Buffered I/O vs `O_SYNC`/`O_DSYNC`/`O_DIRECT`

Buffered I/O is the normal fast path through page cache. `O_SYNC` and `O_DSYNC` trade throughput for stronger write completion semantics. `O_DIRECT` is specialized and alignment-sensitive; databases may use it, ordinary applications usually should not.

### 27. stdio buffering controls vs raw syscalls

`setvbuf()` and `setbuf()` tune libc buffering for a `FILE *`: full buffering, line buffering, or no buffering. Raw `read()`/`write()` bypass that layer. Mixing stdio and raw syscalls on the same open file description can desynchronize libc's buffer and the kernel file offset unless the code carefully flushes or repositions.

### 28. `openat()`/directory-FD APIs vs path-string APIs

Path-string APIs repeatedly resolve names from cwd or root. `openat()`, `fstatat()`, and related APIs resolve relative to a directory FD, which narrows TOCTOU windows, avoids cwd races, and helps scanners operate on the directory they actually opened.

### 29. `chroot()` isolation vs real sandboxing

`chroot()` changes pathname resolution for absolute paths. It is not a full sandbox: open directory FDs, unchanged cwd, `fchdir()`, privileges, device nodes, mounts, and FD passing can pierce weak setups. Treat it as one containment piece, not the security boundary.

### 30. `unlink()` vs `truncate()` vs `ftruncate()`

`unlink()` removes a name. `truncate()` resizes by pathname. `ftruncate()` resizes an already-open file. Shrinking discards data; extending can create holes. Use the FD form when you already verified the object and want to avoid another pathname lookup.

### 31. timestamp metadata reads vs timestamp mutation APIs

`stat()` reports timestamps such as access, modification, and status-change time. `utime()`, `utimes()`, and `futimens()` modify selected timestamps, subject to ownership, write permission, privilege, and filesystem support rules.

### 32. SUID/SGID/sticky bits: execution, inheritance, and delete rules

SUID changes effective user ID during execution; SGID changes effective group ID for executables and can make directories inherit group ownership; sticky directories restrict who may remove or rename entries. Linux may clear SUID/SGID bits after writes or ownership changes for safety.

### 33. advisory locking vs mandatory locking

Advisory locks coordinate cooperating processes. Mandatory locking is rare Linux behavior requiring special filesystem/mode setup and can interact badly with blocking I/O, `O_NONBLOCK`, truncation, mapping, and denial-of-service risk. Prefer advisory protocols unless a legacy system forces otherwise.

For `fcntl()` record locks, do not use `F_GETLK` as a check-then-lock decision. Try `F_SETLK` or `F_SETLKW` and handle conflict (`EACCES`/`EAGAIN`), interruption (`EINTR`), and possible deadlock detection (`EDEADLK`). If the critical section uses stdio, flush or avoid buffering carefully; holding a lock around `fprintf()` does not prove libc has already issued the kernel writes.

### 34. `inotify` object-lifecycle events vs content-change events

Content events say something happened to data or metadata. Lifecycle events such as `IN_MOVED_FROM`/`IN_MOVED_TO`, `IN_DELETE_SELF`, `IN_MOVE_SELF`, `IN_IGNORED`, and `IN_UNMOUNT` mean the watched name, object, or watch itself changed state. `IN_Q_OVERFLOW` means cached event knowledge is incomplete, so production watchers must rescan and rebuild state.

---

## Recognition Notes - Priority C

- `readv()` / `writev()`: useful when one logical message spans multiple buffers. Still handle partial transfer.
- `fcntl(F_GETFL/F_SETFL)`: use read-modify-write to change status flags such as `O_NONBLOCK`.
- `setvbuf()` / `setbuf()`: choose full, line, or no buffering for a `FILE *` stream before I/O.
- `ioctl()`: device-specific control path. Common near drivers, terminals, and special devices.
- `creat()`: know it maps to an `open()` style create/truncate workflow.
- `truncate()` / `ftruncate()`: resize files; shrinking discards data, extending can create holes, and both mutate metadata.
- `/dev/fd`, `/proc/self/fd`, `mkstemp()`, `tmpfile()`: descriptor naming and temporary-file patterns.
- `posix_fadvise()`: performance hint for expected access pattern.
- `sync()` / `syncfs()`: broad writeback tools, not a substitute for designing per-file durability.
- `O_TMPFILE`: advanced temporary-file pattern where supported; publishing still needs correct link and durability handling.
- mandatory locking: rare Linux feature with special setup; usually avoided.
- `utime()` / `utimes()` / `futimens()`: timestamp mutation APIs with ownership/permission rules.
- `IN_DELETE_SELF`, `IN_MOVE_SELF`, `IN_IGNORED`, `IN_UNMOUNT`: important watch lifecycle events.
- `dnotify` / `fanotify`: older or broader notification mechanisms. `inotify` is the normal first tool for app-level file watching.
- ACL C API: recognize it, but most debugging starts with `getfacl` and `setfacl`.
- `statvfs()`, `nftw()`, FUSE, COW filesystems, large-file macros, timestamp resolution: learn when a project needs them.

---

## Extra Questions Worth Adding

- Your service writes a calibration blob on an embedded device. Which failures must the write path survive?
- A process writes to `/dev/ttyUSB0` or a device file and gets short reads/writes. How does this differ from regular file I/O?
- A firmware update tool replaces a file through a symlinked release directory. What identity and atomicity issues do you check?
- A log rotation script renames the active log, but the daemon keeps writing to the old file. Why?
- A shared file is updated by two unrelated processes. When is `O_APPEND` enough, and when do you need a lock?
- A program says it cannot watch more files with `inotify`. What limit do you inspect?
- A copied deployment loses xattrs or ACLs. Why might this happen?
- A process can read a config manually but systemd service cannot. What environment, credentials, and path checks do you perform?

---

## One-Minute Review

- Real interviews start from bugs and design scenarios, then drill into keywords.
- FD is per process; open file description stores shared offset and status flags; inode stores filesystem identity and metadata.
- `O_APPEND` solves append offset races; it does not replace durability design.
- `read()` and `write()` can be partial. Always use returned byte counts.
- `write()` success is not crash safety. `fflush()` is not `fsync()`.
- Crash-safe replacement is temp file in same directory, file sync, checked close, `rename()`, and parent directory sync when needed.
- `O_CLOEXEC` prevents descriptor leaks across `exec()`.
- `unlink()` removes a name; deleted-open files can still consume disk.
- Directory execute permission is required for path traversal.
- Use `namei`, `stat`, `getfacl`, `id`, `findmnt`, `strace`, `lsof`, and `/proc` for real debugging.
- `flock()` is good for simple whole-file daemon locks; `fcntl()` is for byte ranges but has tricky close semantics.
- `inotify` is not recursive, can overflow, and often requires directory watches plus rescan.
- `umask`, SGID directories, default ACLs, and `ACL_MASK` explain many shared-directory surprises.
