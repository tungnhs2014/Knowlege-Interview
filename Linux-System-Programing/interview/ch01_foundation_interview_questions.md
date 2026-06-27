# Chapter 1 Interview - Linux Foundation

> Scope: Linux architecture, user/kernel boundary, syscall and libc behavior, process basics, users/groups/credentials, `/proc`, runtime limits, and first-pass production debugging.
> Interview intent: selected questions that are likely to appear in Linux system, embedded Linux, backend infrastructure, and production debugging interviews.

---

## Review Basis

Chapter 1 mapping was verified from `LINUX_SYSTEM_LEARNING_MAP.md`:

- 1.1 Fundamental Concepts: TLPI Ch02, DevLinux Module 01, `knowledge/ch01_linux_architecture.md`.
- 1.2 System Calls vs Library Functions: TLPI Ch03, DevLinux Module 01, `knowledge/ch01_linux_architecture.md`.
- 1.3 Users and Groups: TLPI Ch08 and Ch09, `knowledge/ch01_users_and_groups.md`.
- 1.4 `/proc` Filesystem: TLPI Ch12, `knowledge/ch01_system_info.md`.
- 1.5 System Limits and Options: TLPI Ch11, `knowledge/ch01_system_info.md`.

Primary correctness sources:

- Repo knowledge files: `ch01_linux_architecture.md`, `ch01_users_and_groups.md`, `ch01_system_info.md`.
- TLPI-derived docs: `ch02_fundamental_concepts.md`, `ch03_system_programming_concepts.md`, `ch08_users_and_groups.md`, `ch09_process_credentials.md`, `ch11_system_limits_and_options.md`, `ch12_system_and_process_information.md`.
- DevLinux Module 01: `INDEX.md`, `README.md`, `01-General-Knowlege/README.md`, and static/shared library exercises for build and runtime context.
- Linux man-pages and official docs: [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html), [`proc_pid_fd(5)`](https://man7.org/linux/man-pages/man5/proc_pid_fd.5.html), [`proc_pid_status(5)`](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html), [`credentials(7)`](https://man7.org/linux/man-pages/man7/credentials.7.html), [`setuid(2)`](https://man7.org/linux/man-pages/man2/setuid.2.html), [`getgroups(2)`](https://man7.org/linux/man-pages/man2/getgroups.2.html), [`sysconf(3)`](https://man7.org/linux/man-pages/man3/sysconf.3.html), [`pathconf(3)`](https://man7.org/linux/man-pages/man3/pathconf.3.html), [`getrlimit(2)`](https://man7.org/linux/man-pages/man2/getrlimit.2.html), [`capabilities(7)`](https://man7.org/linux/man-pages/man7/capabilities.7.html), [`user_namespaces(7)`](https://man7.org/linux/man-pages/man7/user_namespaces.7.html), and the GNU C Library manual on [`syscall`](https://sourceware.org/glibc/manual/2.39/html_node/System-Calls.html) and [`errno`](https://sourceware.org/glibc/manual/latest/html_mono/libc.html).

Interview calibration sources:

- [Amazon Software Development Interview Topics](https://www.amazon.jobs/content/en/how-we-hire/interview-prep/software-development-topics) explicitly includes operating systems and system design, and emphasizes applying knowledge rather than memorizing detail.
- [Microsoft Technical Interviews](https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html) emphasizes problem solving, design, testing, boundaries, error conditions, and security implications.
- [Google Careers Interview Tips](https://www.google.com/about/careers/applications/interview-tips) and [Meta Careers SWE Full Loop Prep](https://www.metacareers.com/careers/SWE-prep-onsite) were used to calibrate communication style: explain assumptions, reason through trade-offs, and handle realistic design/debug follow-ups.
- Recurring OS/Linux interview banks were used only to prioritize common topics such as processes, system calls, permissions, `/proc`, and limits. Technical answers here follow repo docs, TLPI-derived docs, man-pages, and official technical documentation.

---

## Priority Map

### A - Project and production scenarios

Study these deeply. Be ready to explain the mechanism, failure mode, command-line investigation path, and how you would fix or design around the issue.

| Scenario family | Must-cover ideas |
|---|---|
| Service works in a shell but fails under `systemd`, cron, or a container | process state, cwd/root directory, environment, standard FDs, credentials, limits, shell vs kernel |
| `open()` fails with `EACCES`, `EPERM`, `ENOENT`, or `EMFILE` | syscall boundary, `errno`, path traversal, effective IDs, groups, FD limits |
| A program logs late, out of order, or not at all | `printf()` vs `write()`, stdio buffering, FD 1/2, service log capture |
| Production process has leaking FDs or wrong I/O target | FD model, `/proc/<PID>/fd`, `readlink`, resource limits |
| Privileged helper or daemon must do a narrow privileged operation | real/effective/saved IDs, set-user-ID, temporary vs permanent privilege drop, capabilities |
| Live process must be inspected without restarting it | `/proc` as virtual FS, `status`, `cmdline`, `environ`, `fd`, `fdinfo`, `limits`, races |
| Code must run across distros, filesystems, or embedded targets | POSIX vs Linux-specific behavior, `sysconf()`, `pathconf()`, `confstr()`, `getrlimit()`, no hardcoded limits |
| Backend or embedded target has limited RAM/storage/process budget | open files, processes/threads, stack, page size, deployment with static/shared libraries |

### B - Design comparisons and senior follow-ups

Know the distinction and the trade-off. These are common follow-ups after a scenario answer.

| Comparison | Why interviewers ask |
|---|---|
| system call vs libc function | Tests whether you know where work happens and why `strace` sees only kernel-facing calls. |
| `printf()` vs `write()` | Tests buffering, stdout/stderr behavior, and logging reliability. |
| real UID vs effective UID vs saved UID | Tests permission reasoning and privilege-drop design. |
| root vs capabilities | Tests least privilege and modern Linux security awareness. |
| `sysconf()` vs `pathconf()` vs `getrlimit()` | Tests limit-aware production code. |
| `/proc` vs normal files | Tests observability, virtual FS semantics, and race handling. |
| static vs shared libraries | Tests build/runtime deployment awareness, especially on embedded targets. |
| POSIX vs Linux-specific APIs | Tests portability judgment. |

### C - Lower-priority / know enough to recognize

Recognize these names and know when to read the manual. Do not let them dominate first-pass interview prep.

| Topic | Recognition target |
|---|---|
| syscall numbers and entry instructions | Implementation detail behind the syscall boundary. |
| `setfsuid()` / `setfsgid()` | Linux-specific historical filesystem credential APIs; avoid in portable application code. |
| user namespaces | UID/GID and capability meaning can differ inside vs outside a namespace. |
| every `/proc` file | Learn the practical subset first, then read `proc(5)` as needed. |
| every `_SC_*`, `_PC_*`, POSIX option | Know the query pattern, not every constant. |
| `uname()` | Useful diagnostics, weak feature detection. |

---

## Final Interview List

### A - Project and production scenarios

1. A backend service opens a config file successfully from your shell but fails under `systemd` with `ENOENT` or `EACCES`. How do you debug it?
2. A C program calls `open()` and gets `-1`. Walk through what happened from user space into the kernel and how `errno` should be handled.
3. A production process slowly reaches `EMFILE` and stops accepting work. How do you prove it is an FD leak and fix the design?
4. Logs from an embedded daemon appear late or out of order after redirecting stdout to a file or service logger. What is your explanation?
5. A helper needs brief elevated privilege to update a protected file. How would you design it without leaving the whole process privileged?
6. A service runs as the expected user but still gets `Permission denied`. How do real/effective IDs and supplementary groups change your investigation?
7. You need to inspect a live process in production without restarting it. Which `/proc` files do you read and what races or permissions do you handle?
8. Code hardcodes `PATH_MAX`, `OPEN_MAX`, page size, or a fixed FD limit and later fails on another filesystem or embedded target. What should be changed?
9. A binary builds on your machine but fails on the target board with a missing `.so` or different runtime behavior. How do you debug the build/runtime boundary?
10. A containerized process appears to be root but cannot change a host file or kernel setting. What concepts should you bring into the answer?

### B - Design comparisons and senior follow-ups

11. Compare kernel vs shell in the lifecycle of running `cmd >out 2>err`.
12. Compare a program, process, PID, PPID, process tree, command-line arguments, and environment.
13. Compare a file descriptor, open file description, `FILE *`, and standard FDs 0/1/2 at a first-pass level.
14. Compare `EACCES`, `EPERM`, `ENOENT`, and `EMFILE` as debugging signals.
15. Compare `setuid()`, `seteuid()`, and `setresuid()` for privilege management.
16. Why should account lookup use `getpwnam()`, `getpwuid()`, `getgrnam()`, or `getgrgid()` instead of parsing `/etc/passwd` and `/etc/group` directly?
17. What is the static-storage and thread-safety pitfall of the non-`_r` password/group lookup APIs?
18. Why should `/proc` parsers search by field name and handle disappearing processes?
19. How do `ulimit`, `prlimit`, `getrlimit()`, `/proc/<PID>/limits`, and `sysconf(_SC_OPEN_MAX)` relate?
20. Why is `uname()` useful in logs but weak as feature detection?

### C - Lower-priority / know enough to recognize

21. What are syscall numbers, `syscall(2)`, `int 0x80`, `sysenter`, and the `syscall` CPU instruction?
22. What are `setfsuid()` and filesystem UID/GID?
23. What are user namespaces, at a recognition level?
24. What are feature-test macros such as `_POSIX_C_SOURCE`, `_XOPEN_SOURCE`, and `_GNU_SOURCE`?
25. What are `confstr()` and POSIX options?
26. What are `/proc/sys`, sysctl persistence, and why are writable kernel tunables risky?

---

## High-Value Comparisons

| Comparison | Interview-grade answer |
|---|---|
| Kernel vs shell | The kernel is the privileged resource manager. The shell is a user-space command interpreter that sets up argv, environment, redirection, pipes, and starts programs. |
| User mode vs kernel mode | User mode is restricted execution. Kernel mode can access protected memory and privileged operations. A syscall crosses that boundary. |
| Program vs process | A program is a passive executable or script. A process is a running instance with PID, memory, credentials, FDs, cwd, environment, limits, and scheduler state. |
| System call vs libc function | A syscall requests kernel work. A libc function may wrap a syscall, combine several syscalls, add buffering/formatting, or run purely in user space. |
| `errno` vs return value | Return value tells whether the call failed. `errno` explains a documented failure and may contain stale data after success. |
| FD vs `FILE *` | An FD is a process-local integer handle for a kernel I/O object. `FILE *` is a stdio stream that adds user-space buffering and formatting. |
| Real UID vs effective UID vs saved UID | Real is who started the process. Effective is who it acts as for most checks. Saved allows some set-user-ID programs to drop and regain privilege. |
| Supplementary groups vs primary GID | Primary GID is one group identity. Supplementary groups are extra group memberships that can grant or deny access in real deployments. |
| Root vs capabilities | UID 0 traditionally has broad privilege. Capabilities split privilege into smaller units, enabling least-privilege designs when used carefully. |
| `/proc` vs disk file | `/proc` exposes live kernel state through a virtual filesystem. Data can change, disappear, or be restricted while you read it. |
| `sysconf()` vs `pathconf()` | `sysconf()` queries system/process limits. `pathconf()` and `fpathconf()` query path or filesystem-related limits. |
| `getrlimit()` vs `sysconf()` | `getrlimit()` reads soft/hard resource ceilings such as `RLIMIT_NOFILE`; `sysconf()` reports POSIX runtime constants that may be influenced by limits on Linux. |
| POSIX vs Linux-specific | POSIX behavior is portable across conforming systems. `/proc`, capabilities, `setresuid()`, and user namespaces are Linux-specific or nonportable. |
| Static vs shared linking | Static linking copies needed code into the binary. Shared linking records runtime `.so` dependencies loaded by the dynamic linker. |

---

## Common Project Failure Patterns

- Treating a libc call and a syscall as the same abstraction, then misreading `strace` output.
- Checking `errno` without first checking the documented failure return.
- Logging only "Permission denied" without the path, operation, effective UID/GID, groups, cwd, root directory, and mount/container context.
- Assuming FD `1` is always an interactive terminal and FD `2` is always visible to a human.
- Leaking file descriptors across request paths or child processes until `EMFILE` appears far from the bug.
- Using `access()`-style prechecks as if they prove a later operation will succeed.
- Running privileged helper code with a broad environment, unsafe `PATH`, inherited FDs, or no clear privilege-drop point.
- Assuming supplementary group changes apply to already-running processes. They are normally set at login/session creation and inherited.
- Parsing `/proc` as stable normal files, using fixed line numbers, or treating process disappearance as exceptional.
- Hardcoding `PATH_MAX`, `OPEN_MAX`, page size, username length, or stack/open-file/process limits.
- Forgetting `systemd`, cron, containers, and embedded init systems may provide different environment variables, cwd, limits, credentials, namespaces, and stdout/stderr handling.
- Treating root as the only privilege model and missing capabilities, file capabilities, or namespace-specific privilege.

---

## Detailed Answers - Priority A

### 1. A backend service opens a config file successfully from your shell but fails under `systemd` with `ENOENT` or `EACCES`. How do you debug it?

**What the interviewer is testing**

They want to know whether you debug the actual running process instead of assuming the shell environment is the production environment.

**Strong answer**

I would identify the exact failing path and syscall first, then compare the service process state with my shell. The usual differences are current working directory, root directory, environment, effective credentials, supplementary groups, mount namespace, and resource limits. I would avoid changing permissions blindly until I know whether the failure is path resolution, identity, or policy.

**Mechanism**

The shell is a user-space process that starts another process with argv, environment, cwd, redirections, and inherited limits. Under `systemd`, cron, or a container runtime, those inherited attributes can differ. Relative paths are interpreted from the process cwd. Permission checks are made by the kernel during path traversal and final object access, using process credentials, group membership, capabilities, and possibly LSM/container policy.

**Pitfalls**

- Checking only the final file mode and forgetting execute/search permission on parent directories.
- Looking at username strings instead of numeric UID/GID.
- Forgetting that a service may have a reduced environment, different `PATH`, different cwd, or a private filesystem view.
- "Fixing" with `chmod 777` and creating a security incident.

**Debug angle**

Use `journalctl -u <service>`, `systemctl cat <service>`, `strace -f -e trace=file -p <PID>` or a reproduction wrapper, `readlink /proc/<PID>/cwd`, `readlink /proc/<PID>/root`, `tr '\0' '\n' </proc/<PID>/environ`, `cat /proc/<PID>/status`, `cat /proc/<PID>/limits`, `id`, `getent passwd`, `getent group`, `namei -l /path`, `ls -ln`, and `findmnt`.

**Follow-up keywords**

`ENOENT`, `EACCES`, `EPERM`, cwd, root directory, environment, effective UID, supplementary groups, `systemd` `WorkingDirectory=`, `User=`, `Group=`, `Environment=`, mount namespace.

### 2. A C program calls `open()` and gets `-1`. Walk through what happened from user space into the kernel and how `errno` should be handled.

**What the interviewer is testing**

They are testing the syscall boundary, libc wrapper model, return-value discipline, and whether you treat errors as part of normal system programming.

**Strong answer**

In C, `open()` usually means calling a libc wrapper. The wrapper prepares arguments and enters the kernel through the system call interface. The kernel validates the pointer arguments, resolves the pathname, checks permissions and limits, creates an open file object if allowed, installs a file descriptor in the process table, and returns either an FD or an error. In user space, failure is reported as `-1` and `errno` explains the documented reason.

**Mechanism**

A syscall changes CPU execution from user mode to kernel mode. The kernel runs a specific service routine, validates user memory and arguments, performs permission/resource checks, then returns to user mode. Linux kernel internals conventionally return negative error numbers; libc maps these to `errno` and a documented failure return.

**Pitfalls**

- Reading `errno` after success and logging a stale old error.
- Assuming every libc function maps one-to-one to one syscall.
- Losing the original `errno` by calling another library function before logging it.
- Treating `EACCES`, `EPERM`, `ENOENT`, or `EMFILE` as interchangeable.

**Debug angle**

Use `strace -f -e trace=open,openat,close,read,write ./program` or `strace -f -e trace=file`. Log the operation, path, flags, and saved `errno`. Use `man 2 open` and the `ERRORS` section for exact semantics.

**Follow-up keywords**

libc wrapper, `syscall(2)`, `intro(2)`, user mode, kernel mode, privilege boundary, `errno`, `perror()`, `strerror()`, `EINTR`, `EFAULT`, `EMFILE`.

### 3. A production process slowly reaches `EMFILE` and stops accepting work. How do you prove it is an FD leak and fix the design?

**What the interviewer is testing**

They are testing whether you understand the FD model, resource limits, `/proc` inspection, and failure containment under load.

**Strong answer**

I would confirm the failing syscall and `EMFILE`, inspect the process FD table over time, group the FD targets by type, and connect the growth to a code path. The fix is usually to close on all paths, use ownership conventions for FDs, set close-on-exec where appropriate, add tests for error paths, and alert before the process reaches its soft limit.

**Mechanism**

Each process has a file descriptor table. FDs can reference regular files, pipes, sockets, terminals, devices, epoll/event/timer FDs, or anonymous inode objects. `RLIMIT_NOFILE` defines one greater than the maximum FD number a process may open; exceeding it commonly produces `EMFILE`. System-wide exhaustion can produce different failures such as `ENFILE`.

**Pitfalls**

- Assuming only regular files count as FDs.
- Forgetting sockets, pipes, timerfds, epoll fds, inotify fds, and inherited descriptors.
- Missing leak paths in error handling.
- Raising `ulimit -n` without fixing unbounded growth.

**Debug angle**

Use `ls -l /proc/<PID>/fd`, `readlink /proc/<PID>/fd/*`, `cat /proc/<PID>/fdinfo/<N>`, `cat /proc/<PID>/limits`, `prlimit --pid <PID>`, `lsof -p <PID>` if available, `strace -f -e trace=desc -p <PID>`, and application metrics. In code, check `close()`, use `O_CLOEXEC` or `FD_CLOEXEC`, and document FD ownership.

**Follow-up keywords**

FD table, `/proc/<PID>/fd`, `/proc/<PID>/fdinfo`, `RLIMIT_NOFILE`, `ulimit -n`, `prlimit`, `EMFILE`, `ENFILE`, `O_CLOEXEC`, FD inheritance.

### 4. Logs from an embedded daemon appear late or out of order after redirecting stdout to a file or service logger. What is your explanation?

**What the interviewer is testing**

They want to know whether you distinguish stdio buffering from kernel writes and understand standard FDs in noninteractive services.

**Strong answer**

`printf()` writes to a stdio stream, not directly to the final destination every time. Stdio may line-buffer when stdout is a terminal, but fully buffer when stdout is a pipe or file. `write()` sends bytes to an FD through the kernel immediately from the caller's perspective, although storage and downstream logging may still buffer later. Mixing stdio and raw `write()` can reorder output unless carefully flushed.

**Mechanism**

FDs `0`, `1`, and `2` are ordinary inherited descriptors. A shell, service manager, or init system may connect them to a terminal, pipe, file, log collector, or `/dev/null`. `FILE *` streams such as `stdout` and `stderr` sit above FDs and maintain user-space buffers.

**Pitfalls**

- Assuming `printf()` means bytes have reached the log.
- Mixing `printf()` and `write()` to the same FD without `fflush()`.
- Forgetting stderr and stdout may have different buffering.
- On embedded targets, losing buffered logs during crash or power loss.

**Debug angle**

Inspect `/proc/<PID>/fd/1` and `/proc/<PID>/fd/2`, run under `strace -e write`, check service logging configuration, force flush with `fflush()` where appropriate, or configure buffering with `setvbuf()` for controlled tools. For crash-sensitive logs, prefer explicit flushing or a logging mechanism designed for the target.

**Follow-up keywords**

`printf()`, `write()`, `FILE *`, stdio buffering, line-buffered, fully-buffered, `fflush()`, stdout, stderr, service logger, embedded power loss.

### 5. A helper needs brief elevated privilege to update a protected file. How would you design it without leaving the whole process privileged?

**What the interviewer is testing**

They are testing least privilege, set-user-ID recognition, credential semantics, and whether you know privilege dropping is subtle.

**Strong answer**

I would first ask whether set-user-ID is needed at all. Often a dedicated service account, group permission, a small root-owned service, or a file capability is safer. If a privileged helper is required, keep it tiny, validate all inputs, sanitize environment and paths, close unexpected FDs, do only the privileged operation, and drop privilege for all normal work. Temporary privilege drop/regain should use effective-ID APIs deliberately; permanent drop should remove the ability to regain privilege.

**Mechanism**

When executing a set-user-ID file, the kernel sets the effective UID to the executable owner and stores the value in the saved set-user-ID. `seteuid()` can switch effective identity between allowed real/saved values for temporary drop/regain. For a set-user-ID-root program, `setuid(nonroot)` changes real, effective, and saved IDs on Linux and prevents regaining root. That is correct for permanent drop, not temporary drop.

**Pitfalls**

- Trusting `PATH`, cwd, environment variables, inherited FDs, or user-controlled filenames.
- Dropping UID before dropping groups/GID.
- Omitting return-value checks from `setuid()` or `seteuid()`.
- Keeping a large application privileged instead of isolating a tiny privileged boundary.

**Debug angle**

Inspect `Uid:`, `Gid:`, `Groups:`, and `Cap*` in `/proc/<PID>/status`; use `id`, `ps -o pid,user,euid,group,egid,comm`, `getcap`, `setcap`, `capsh`, and `find /path -perm /6000 -type f -ls`. For privileged binaries, `sudo strace -u <user> ...` can help reproduce user-context behavior.

**Follow-up keywords**

set-user-ID, set-group-ID, real UID, effective UID, saved UID, `setuid()`, `seteuid()`, `setresuid()`, capabilities, `CAP_SETUID`, privilege drop, least privilege.

### 6. A service runs as the expected user but still gets `Permission denied`. How do real/effective IDs and supplementary groups change your investigation?

**What the interviewer is testing**

They want to see if you debug credentials as kernel state rather than as a username assumption.

**Strong answer**

I would inspect the credentials of the process that failed, not just the configured service user. Permission checks normally depend on effective credentials and supplementary groups, while Linux filesystem checks use filesystem IDs that usually track effective IDs. I would verify numeric IDs, group membership, parent-directory execute bits, ACLs or LSM policy if present, and whether the service actually inherited the groups I expect.

**Mechanism**

Processes carry real, effective, saved, filesystem, and supplementary group IDs. Names such as `www-data` are user-space mappings from account databases; the kernel uses numeric IDs. A process started before a group membership change will not magically gain the new supplementary group list.

**Pitfalls**

- Running `groups <user>` and assuming the already-running process has those groups.
- Looking only at real UID when effective UID differs.
- Ignoring numeric ownership across containers, NFS, or UID reuse.
- Forgetting parent directory search permission.

**Debug angle**

Use `cat /proc/<PID>/status` for `Uid:`, `Gid:`, and `Groups:`, `id -a`, `id -G`, `getent passwd`, `getent group`, `ls -ln`, `namei -l`, `getfacl`, `ps -o pid,user,euid,group,egid,supgid,comm` if supported, and service manager configuration. Restart the service after group membership changes if needed.

**Follow-up keywords**

real UID, effective UID, saved UID, filesystem UID, supplementary groups, `getuid()`, `geteuid()`, `getgroups()`, `getpwnam()`, `getgrnam()`, ACL, LSM, UID mapping.

### 7. You need to inspect a live process in production without restarting it. Which `/proc` files do you read and what races or permissions do you handle?

**What the interviewer is testing**

They are testing production observability and whether you know `/proc` is a Linux virtual filesystem, not stable disk content.

**Strong answer**

I would use `/proc/<PID>` to inspect live kernel state: `status` for IDs/state/memory/thread count, `cmdline` and `environ` for startup context, `fd` and `fdinfo` for open descriptors, `cwd`, `root`, and `exe` for path context, `limits` for resource ceilings, and `maps` for memory mappings. I would handle process exit, permission restrictions, null-separated fields, and kernel-version format changes.

**Mechanism**

`/proc` is procfs, a pseudo-filesystem generated by the kernel. Reading a file-like path triggers the kernel to expose current state. `/proc/<PID>` appears when a process exists and disappears when it exits. Access may be restricted by ownership, capabilities, ptrace policy, or procfs mount options such as `hidepid`.

**Pitfalls**

- Parsing `status` by line number instead of field names.
- Treating `cmdline` and `environ` as newline-separated text.
- Assuming `/proc/<PID>/fd` is available after the main thread exits in some multithreaded cases.
- Failing a monitoring tool because a process exited during scanning.

**Debug angle**

Use `cat /proc/<PID>/status`, `tr '\0' ' ' </proc/<PID>/cmdline`, `tr '\0' '\n' </proc/<PID>/environ`, `ls -l /proc/<PID>/fd`, `cat /proc/<PID>/fdinfo/<N>`, `cat /proc/<PID>/limits`, `readlink /proc/<PID>/{cwd,root,exe}`, `cat /proc/meminfo`, `cat /proc/cpuinfo`, `cat /proc/mounts`, `ps`, `top`, `htop`, and `readlink`.

**Follow-up keywords**

procfs, `/proc/self`, `/proc/<PID>/status`, `/proc/<PID>/cmdline`, `/proc/<PID>/environ`, `/proc/<PID>/fd`, `/proc/<PID>/fdinfo`, `/proc/<PID>/limits`, `hidepid`, race.

### 8. Code hardcodes `PATH_MAX`, `OPEN_MAX`, page size, or a fixed FD limit and later fails on another filesystem or embedded target. What should be changed?

**What the interviewer is testing**

They are testing portability judgment, limit-aware design, and whether you understand compile-time vs runtime limits.

**Strong answer**

The program should query limits when useful, dynamically allocate or grow buffers when possible, and still handle real operation failures. Use `sysconf()` for system/process limits such as page size, open files, argument size, and supplementary group maximum. Use `pathconf()` or `fpathconf()` for filesystem/path-related limits such as name length or pipe atomic write size. Use `getrlimit()` for soft/hard resource ceilings such as open files, stack, address space, and processes.

**Mechanism**

Some limits are compile-time constants, some are runtime constants, some vary by filesystem, and some are resource limits inherited from the parent process or service manager. For `sysconf()`, `pathconf()`, and `fpathconf()`, `-1` can mean either error or indeterminate. Set `errno = 0` before the call and check `errno` afterward.

**Pitfalls**

- Treating `-1` from `sysconf()` as always fatal.
- Allocating huge buffers just because a reported maximum is large.
- Assuming `sysconf(_SC_OPEN_MAX)` alone explains the current soft `RLIMIT_NOFILE`.
- Forgetting embedded targets may have smaller limits, different page size, and limited RAM.

**Debug angle**

Use `getconf ARG_MAX`, `getconf PAGESIZE`, `getconf NAME_MAX .`, `ulimit -a`, `ulimit -n`, `prlimit --pid <PID>`, `cat /proc/<PID>/limits`, and targeted error logging for `ENAMETOOLONG`, `EMFILE`, `E2BIG`, `ENOMEM`, and `EAGAIN`.

**Follow-up keywords**

compile-time limit, runtime limit, indeterminate limit, `sysconf()`, `pathconf()`, `fpathconf()`, `confstr()`, `getrlimit()`, `setrlimit()`, `RLIMIT_NOFILE`, `RLIMIT_NPROC`, `RLIMIT_STACK`.

### 9. A binary builds on your machine but fails on the target board with a missing `.so` or different runtime behavior. How do you debug the build/runtime boundary?

**What the interviewer is testing**

They are testing whether you understand that compilation, linking, loading, and execution are separate phases, which matters in embedded Linux and deployment work.

**Strong answer**

I would separate compile-time success from runtime dependency resolution. A shared-library build can link successfully but fail on the target if the dynamic linker cannot find the right `.so`, if ABI versions differ, or if environment/search paths differ. I would inspect the executable, its interpreter, its shared dependencies, and the target library paths. For embedded deployment, I would decide deliberately between static linking, shared libraries, RPATH/RUNPATH, package installation, or shipping required `.so` files.

**Mechanism**

Static linking copies selected object code into the executable at build time. Shared linking records dependencies that the dynamic linker resolves at program startup. Environment variables such as `LD_LIBRARY_PATH` may affect lookup, but secure-execution contexts such as set-user-ID programs restrict dangerous environment influence.

**Pitfalls**

- Assuming "it compiled" means "it will run on the target."
- Using `LD_LIBRARY_PATH` as a production-only fix without controlled deployment.
- Forgetting cross-compilation target architecture and libc differences.
- Adding shared-library search paths to privileged programs unsafely.

**Debug angle**

Use `file ./program`, `readelf -l ./program`, `readelf -d ./program`, `ldd ./program` on the right target when safe, `LD_DEBUG=libs ./program` for nonprivileged debugging, `strace -f -e trace=file ./program`, and inspect service environment. In DevLinux-style exercises, static `.a` and shared `.so` builds show this difference clearly.

**Follow-up keywords**

compile, assemble, link, ELF, dynamic linker, static library, shared library, `ldd`, `readelf`, `LD_LIBRARY_PATH`, RPATH, RUNPATH, cross-compilation, embedded rootfs.

### 10. A containerized process appears to be root but cannot change a host file or kernel setting. What concepts should you bring into the answer?

**What the interviewer is testing**

They are testing whether you recognize modern Linux privilege is not just "UID 0 everywhere."

**Strong answer**

I would check the namespace and capability context. In a user namespace, a process can have UID 0 inside the namespace while being unprivileged outside it. Capabilities are also scoped, and container runtimes often drop many capabilities. Access can also be blocked by mount namespace, read-only mounts, LSM policy, seccomp, or procfs/sysctl restrictions.

**Mechanism**

The kernel makes authorization decisions using credentials, capabilities, namespaces, object ownership, and subsystem policy. UID/GID mappings can make a file appear differently inside vs outside a container. Writable `/proc/sys` entries are kernel tunables and often require privilege in the relevant namespace, plus a writable mount and permitted capability.

**Pitfalls**

- Saying "root can do anything" without checking capabilities and namespaces.
- Debugging names instead of numeric ID mappings.
- Assuming `/proc` and `/sys` are fully writable in containers.
- Granting `--privileged` when a narrow capability or mount fix would suffice.

**Debug angle**

Use `id`, `cat /proc/self/uid_map`, `cat /proc/self/gid_map`, `cat /proc/<PID>/status` for `Uid:` and `Cap*`, `capsh --print`, `getcap`, `findmnt`, container runtime inspect commands, and `dmesg` or audit logs for LSM denials when available.

**Follow-up keywords**

user namespaces, UID/GID mapping, capabilities, `CAP_SYS_ADMIN`, `CAP_DAC_OVERRIDE`, `capsh`, `getcap`, `setcap`, procfs, sysctl, seccomp, SELinux, AppArmor.

---

## Short Answers - Priority B

### 11. Compare kernel vs shell in the lifecycle of running `cmd >out 2>err`.

The shell parses the command, opens or creates `out` and `err`, adjusts FDs 1 and 2, then starts the target program. The kernel performs the actual file opens, permission checks, FD table updates, process creation/execution, and I/O. The shell is not a privilege bypass; it is another user-space process.

### 12. Compare a program, process, PID, PPID, process tree, command-line arguments, and environment.

A program is executable content. A process is a running instance. PID identifies the process; PPID identifies the parent that created it. A process tree shows parent-child relationships. Command-line arguments are passed as `argv`; environment variables are inherited key/value strings unless replaced during `exec()`.

### 13. Compare a file descriptor, open file description, `FILE *`, and standard FDs 0/1/2 at a first-pass level.

An FD is a process-local integer handle. An open file description is the kernel-side open object behind one or more FDs, introduced in more detail in file I/O chapters. `FILE *` is stdio's buffered stream abstraction over an FD. FDs 0, 1, and 2 conventionally mean stdin, stdout, and stderr, but their targets are runtime-provided.

### 14. Compare `EACCES`, `EPERM`, `ENOENT`, and `EMFILE` as debugging signals.

`EACCES` usually points to permission failure on the object or path traversal. `EPERM` often means the operation requires privilege or capability. `ENOENT` means a path component or target was not found, but can also come from a wrong cwd/root/mount view. `EMFILE` means the process hit its open-FD limit.

### 15. Compare `setuid()`, `seteuid()`, and `setresuid()` for privilege management.

`setuid()` is commonly used for permanent privilege drop in privileged code; on Linux, set-user-ID-root code that calls `setuid(nonroot)` cannot regain root. `seteuid()` changes only effective identity where permitted and is the usual temporary drop/regain tool. `setresuid()` is Linux/nonportable but directly controls real, effective, and saved IDs.

### 16. Why should account lookup use `getpwnam()`, `getpwuid()`, `getgrnam()`, or `getgrgid()` instead of parsing `/etc/passwd` and `/etc/group` directly?

Account and group data may come from NSS sources such as local files, LDAP, NIS, or other configured backends. The lookup APIs honor that configuration. Direct parsing of one file misses real deployments and creates brittle code.

### 17. What is the static-storage and thread-safety pitfall of the non-`_r` password/group lookup APIs?

Functions such as `getpwnam()` and `getpwuid()` may return pointers to static storage overwritten by later calls. This creates surprises even in single-threaded code if you keep multiple returned pointers. Use `_r` variants or copy data when reentrancy/thread safety matters.

### 18. Why should `/proc` parsers search by field name and handle disappearing processes?

`/proc` exposes live kernel state. Process directories can disappear between listing and opening because the process exited. Human-readable files such as `status` can gain fields across kernel versions, so field-name parsing is more robust than fixed line numbers.

### 19. How do `ulimit`, `prlimit`, `getrlimit()`, `/proc/<PID>/limits`, and `sysconf(_SC_OPEN_MAX)` relate?

`getrlimit()` reads soft/hard resource limits for a process; `setrlimit()` changes them where permitted. Shell `ulimit` sets limits inherited by child processes. `prlimit` can inspect or change another process with permission. `/proc/<PID>/limits` displays current limits. On Linux, `_SC_OPEN_MAX` can reflect the open-file resource limit.

### 20. Why is `uname()` useful in logs but weak as feature detection?

`uname()` gives system identity such as kernel name, release, version, and machine. It is useful diagnostic context. It is weak feature detection because kernels can be patched, backported, containerized, configured differently, or constrained by policy.

---

## Recognition Notes - Priority C

- `syscall(2)` is a generic escape hatch for invoking a syscall by number. It is less portable and less friendly than normal libc wrappers, but useful for very new or special syscalls.
- Syscall numbers and entry instructions are architecture and ABI details. Recognize terms such as `int 0x80`, `sysenter`, and `syscall`, but interview value is usually in explaining the boundary and validation path.
- `setfsuid()` and `setfsgid()` are Linux-specific filesystem credential APIs with historical NFS-related motivation. For normal application design, treat them as recognize-only.
- User namespaces isolate UID/GID mappings and capabilities. A process can be UID 0 inside a namespace while unprivileged outside it.
- Feature-test macros control which declarations headers expose. `_GNU_SOURCE` is a deliberate Linux/glibc extension choice, not a portability badge.
- `confstr()` returns string-valued configuration information. Recognize it beside `sysconf()` and `pathconf()`; it is less likely to be a Chapter 1 interview headline.
- `/proc/sys` exposes kernel tunables. Many writes are immediate, privileged, and nonpersistent unless configured through the system's sysctl mechanism.
- Do not memorize all fields in `/etc/passwd`, `/etc/group`, `/etc/shadow`, or every `/proc/<PID>` file. Know why they exist and which subset helps debug production.

---

## Extra Questions Worth Adding

- How would you debug a program that behaves differently when run from an interactive shell, `systemd`, cron, and a container?
- How would you prove whether missing logs are caused by stdio buffering, service log capture, or process crash?
- How would you audit a set-user-ID helper before allowing it on an embedded target?
- How would you design a service account and group model for shared access to device files or log directories?
- How would you make a process scanner robust while walking `/proc` under heavy process churn?
- How would you handle `EMFILE` in a network service so one request path cannot take down the entire daemon?
- How would you make path and name handling safe without assuming `PATH_MAX` is useful everywhere?
- How would you explain to a teammate why `root` inside a container may still fail to write a host-mounted path?

---

## One-Minute Review

- The kernel manages protected resources; the shell is a user-space command interpreter.
- A syscall crosses the user/kernel boundary; a libc function may wrap syscalls or run in user space.
- Check the documented failure return first; inspect `errno` only when the API says failure happened.
- A process is a running program with PID, PPID, memory, FDs, cwd/root, credentials, environment, and limits.
- FDs are handles to kernel I/O objects: files, pipes, sockets, terminals, devices, and more.
- `printf()` buffers and formats; `write()` sends bytes to an FD.
- Effective credentials and supplementary groups are central to permission debugging.
- `setuid()` is dangerous to misuse; `seteuid()` is the usual temporary drop/regain tool.
- Capabilities and namespaces refine the old "root can do everything" model.
- `/proc` is live kernel-generated state, not stable disk data.
- Start production inspection with `/proc/<PID>/status`, `fd`, `fdinfo`, `cmdline`, `environ`, `cwd`, `root`, `exe`, and `limits`.
- Use `sysconf()`, `pathconf()`, `fpathconf()`, `confstr()`, and `getrlimit()` to avoid hardcoded system assumptions.
- For limit queries, `-1` plus `errno == 0` can mean indeterminate.
- Embedded and backend services often fail at boundaries: environment, credentials, cwd, FDs, limits, libraries, and namespaces.
