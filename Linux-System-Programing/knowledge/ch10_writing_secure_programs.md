# Chapter 10.4 - Writing Secure Privileged Programs

> Topics: least privilege, set-user-ID/set-group-ID safety, credential changes, safe `exec()`, file-descriptor leaks, sensitive data, confinement, signals, TOCTOU, safe file operations, environment/input validation, buffer overruns, denial of service, fail-safe behavior.
> Main sources: TLPI Ch38; TLPI Ch9 for credential background; TLPI Ch39 for capabilities context.
> Production context: used when writing daemons, service helpers, installers, authentication tools, embedded maintenance utilities, and any program that handles untrusted input while holding elevated privilege.

---

## Problem It Solves

Privileged programs can do things ordinary programs cannot: read protected files, bind restricted resources, change system state, or run with another user's credentials.

That power is dangerous. A small bug in an ordinary program may crash one process. The same bug in a privileged program can become:

- privilege escalation;
- unauthorized file access;
- arbitrary command execution;
- secret leakage;
- denial of service.

Secure privileged programming is about two goals:

1. reduce the chance that attackers can subvert the program;
2. reduce the damage if subversion happens anyway.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | Avoid privilege, least privilege, temporary vs permanent privilege drop, safe `exec()`, input validation, TOCTOU | Explain and design safe privileged helpers |
| Work useful | verifying credential changes, close-on-exec, safe temp files, `umask`, `fstat()` after `open()`, `setrlimit(RLIMIT_CORE)`, `strace` debugging | Review and debug production security bugs |
| Recognize | chroot limitations, capabilities, securebits, ASLR/NX, algorithmic complexity attacks, setuid script behavior | Understand deeper hardening topics without bloating first-pass learning |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Privileged program | Program with access beyond ordinary user permissions | root daemon, set-user-ID helper |
| set-user-ID | File mode bit causing effective UID to become file owner on `exec()` | `chmod u+s helper` |
| set-group-ID | File mode bit causing effective GID to become file group on `exec()` | safer with dedicated group |
| Real UID | User who started the process | Used for ownership/accountability |
| Effective UID | User ID used for most permission checks | Privilege often comes from this |
| Saved set-user-ID | Stored ID that can allow privilege reacquisition | Key for temporary privilege drop |
| Least privilege | Hold only the privilege needed right now | Drop early, raise briefly |
| Temporary drop | Disable privilege but keep ability to regain it | `seteuid(getuid())` |
| Permanent drop | Remove ability to regain privilege | set all UIDs/GIDs to unprivileged IDs |
| `exec()` | Replace current program image | Inherits open file descriptors by default |
| Close-on-exec | FD flag that closes descriptor during successful `exec()` | `FD_CLOEXEC`, `O_CLOEXEC` |
| TOCTOU | Time-of-check/time-of-use race | `stat()` then `open()` on path |
| `umask` | Process mask limiting permissions of newly created files | Use restrictive defaults |
| `O_EXCL` | `open()` flag requiring new-file creation with `O_CREAT` | Helps avoid preexisting file attacks |
| `mkstemp()` | Creates a unique temporary file safely | Prefer over predictable `/tmp` names |
| Core dump | Memory snapshot after crash | May expose secrets |
| `RLIMIT_CORE` | Resource limit controlling core dump creation | Set to 0 for secret-handling programs |
| Environment list | Variables inherited across `exec()` | `PATH`, `IFS`, `LD_*` can be dangerous |
| Fail closed | Stop or reject request on uncertainty | Safer than guessing a recovery |

## Concept Overview

The core mental model:

```text
privileged program
    |
    v
assume attacker controls inputs, environment, timing, files, and signals
    |
    v
hold privilege for the smallest code region possible
    |
    v
validate using stable objects, not stale assumptions
    |
    v
check every security-relevant result
    |
    v
fail safely when reality differs from expectation
```

Secure code is not just "use safer functions." It is system design:

- narrow the privilege source;
- narrow the privilege lifetime;
- narrow what can cross `exec()`;
- narrow what input is trusted;
- narrow what happens on failure.

## System Context

| Subsystem | Security relevance |
|-----------|--------------------|
| Credentials | Real/effective/saved IDs decide privilege and reacquisition |
| Capabilities | Can replace full root with narrower `CAP_*` privileges |
| File descriptors | Inherited across `exec()` unless closed or marked close-on-exec |
| Filesystem | Pathnames, symlinks, `/tmp`, ownership, and permissions are attack surfaces |
| Signals | User-controlled timing can widen race windows |
| Environment | Influences program lookup, shell behavior, dynamic loading, locale, and libraries |
| Memory | Secrets can remain in buffers, swap, or core dumps |
| Resource limits | Attackers may trigger exhaustion or abnormal failures |

This chapter connects strongly to process credentials, file I/O, signals, capabilities, and program execution.

## Architecture

Think of a privileged program as three zones:

```text
untrusted boundary
    inputs, env, files, IPC, network, signals
        |
        v
defensive core
    validation, safe open, fd control, credential control
        |
        v
privileged operation
    tiny section that actually needs privilege
```

Good architecture keeps the privileged operation small and pushes everything else into unprivileged code.

Security state to track:

| State | Why it matters |
|-------|----------------|
| Real/effective/saved UID/GID | Determines current and future privilege |
| Supplementary groups | May carry extra access that must be dropped |
| Open file descriptors | May expose privileged files or devices after `exec()` |
| Current directory and root directory | Affects path resolution |
| Environment variables | May change program behavior unexpectedly |
| Signal disposition/mask | Determines interruption and race behavior |
| Resource limits | Affects failure paths and core dump behavior |

## Execution Flow

### Flow 1: Secure Privilege Lifecycle

```text
program starts with privilege
    |
    v
drop privilege immediately if not needed
    |
    v
validate untrusted input while unprivileged
    |
    v
raise privilege briefly
    |
    v
perform privileged operation
    |
    v
drop privilege again
    |
    v
permanently drop once no longer needed
```

### Flow 2: Safe `exec()`

```text
need to run another program
    |
    v
decide whether child needs privilege
    |
    v
drop privilege permanently if not needed
    |
    v
sanitize environment
    |
    v
close or mark FDs close-on-exec
    |
    v
exec absolute path
```

### Flow 3: Safe File Handling

```text
need to create/use a file
    |
    v
avoid public writable directories when possible
    |
    v
open safely with restrictive mode and flags
    |
    v
validate using fstat() on the open fd
    |
    v
change owner/mode with fchown()/fchmod() if needed
```

### Flow 4: TOCTOU Failure

```text
stat("/tmp/target") says safe
    |
    v
attacker swaps path before open()
    |
    v
open("/tmp/target") opens attacker-chosen object
```

Fix the design by validating the open file descriptor, not a stale pathname.

### Flow 5: Defensive Failure

```text
unexpected syscall result
    |
    v
log controlled diagnostic
    |
    v
drop request or terminate
    |
    v
do not invent unsafe recovery assumptions
```

## 10.4 API / Topic Sections

### 10.4.1 Avoid Privilege When Possible

Before writing a set-user-ID or root program, ask:

- can the task be done by changing file ownership or group permissions?
- can a small helper isolate the privileged action?
- can Linux capabilities grant only the needed operation?
- can the service start as root only for setup, then drop privilege?

If a dedicated group is enough, prefer set-group-ID with that group over set-user-ID-root.

### 10.4.2 Least Privilege

A privileged program should hold privilege only while the current operation needs it.

Temporary drop:

- useful when later privileged work remains;
- often uses saved set-user-ID/set-group-ID behavior.

Permanent drop:

- required when privilege will never again be needed;
- must remove the saved privileged ID too;
- should be verified after the call.

On Linux, `setresuid()` and `setresgid()` are often clearer for permanent drops because they address real, effective, and saved IDs directly.

### 10.4.3 Credential Changes

Credential-changing APIs have subtle semantics across systems and privilege states.

Work rules:

- check return values;
- verify resulting IDs with `geteuid()`, `getegid()`, `getresuid()`, or `getresgid()` where available;
- drop supplementary groups before dropping root if needed;
- when dropping multiple privileges, drop privileged effective UID last;
- when reacquiring, raise privileged effective UID first.

Capabilities can affect whether UID-changing operations succeed, so privilege-drop code must be tested in the intended runtime environment.

### 10.4.4 Executing Another Program Securely

Avoid shell-based execution in privileged code:

- avoid `system()`;
- avoid `popen()`;
- avoid `execlp()` and `execvp()` when path search is user-influenced;
- use absolute paths with `execve()` or fixed `execv()` where possible.

Before `exec()`:

- permanently drop privilege unless the child truly needs it;
- sanitize or replace the environment;
- close unneeded FDs or set close-on-exec;
- avoid passing privileged descriptors to untrusted code.

Linux ignores set-user-ID and set-group-ID bits on scripts, but do not rely on privileged interpreter scripts as a portable design.

### 10.4.5 Sensitive Information

Passwords, tokens, keys, and decrypted data should have short lifetimes.

Practices:

- erase secret buffers after use;
- avoid unnecessary copies;
- disable core dumps with `setrlimit(RLIMIT_CORE, ...)`;
- consider memory locking only when the threat model justifies it;
- avoid logging secrets or derived sensitive values.

### 10.4.6 Confinement

Confinement reduces damage after compromise.

Options:

- capabilities instead of full root;
- `chroot()` for limited filesystem views;
- service managers, namespaces, containers, or MAC systems where available.

`chroot()` alone is not sufficient confinement for a set-user-ID-root program. Treat it as one layer, not a complete sandbox.

### 10.4.7 Signals and Race Conditions

Users can send signals to set-user-ID programs they started. Signals can arrive during security-sensitive windows.

Rules:

- keep signal handlers simple;
- block or handle signals around critical regions when necessary;
- avoid designs where a checked path or environment assumption must stay true until later use;
- prefer descriptor-based validation.

TOCTOU is the interview-grade mechanism here: checking one object and later using a possibly different object.

### 10.4.8 File Operations

Safe file-operation principles:

- use restrictive `umask`;
- create files with safe mode from the beginning;
- use `open()` then `fstat()` instead of `stat()` then `open()`;
- use `O_CREAT | O_EXCL` when you must create a new file;
- prefer `mkstemp()` for temporary files;
- avoid predictable names in public writable directories;
- use `fchown()` and `fchmod()` on the open file descriptor.

Never create a privileged-owned file that is even briefly writable by untrusted users.

### 10.4.9 Inputs, Environment, and Runtime Assumptions

Treat these as untrusted:

- argv;
- environment variables;
- files users can create or modify;
- IPC messages;
- network packets;
- interactive input;
- current working directory;
- standard file descriptors.

Validation should include length, range, character set, syntax, and semantic checks.

Environment variables such as `PATH` and `IFS` are especially dangerous around shell execution. In high-risk programs, clear the environment and rebuild a small known-safe one.

Also defend against closed standard descriptors. If fd 0, 1, or 2 is closed, a later `open()` may reuse it unexpectedly.

### 10.4.10 Buffer Overruns, DoS, and Fail-Safe Behavior

Avoid classic unsafe APIs such as `gets()`. Use bounded operations, but still check truncation and null termination.

DoS resilience matters:

- use timeouts;
- throttle excessive work;
- enforce limits;
- avoid unbounded memory allocation;
- design data structures to avoid algorithmic worst-case attacks;
- rate-limit logging under load.

Fail-safe rule: when a privileged program sees an unexpected condition, terminate or reject the request. Do not continue based on guesses.

## Work-Useful Patterns

| Pattern | Why it helps |
|---------|--------------|
| Drop privilege immediately after startup setup | Shrinks privileged code region |
| Use helper process for privileged action | Keeps risky parsing and protocol logic unprivileged |
| Set `O_CLOEXEC` at `open()` time | Avoids descriptor leaks even with races |
| Validate open descriptors with `fstat()` | Avoids path replacement races |
| Replace environment before `exec()` | Prevents user-controlled execution behavior |
| Disable core dumps for secret-handling programs | Reduces accidental secret leakage |
| Verify credential state after changes | Catches partial or unexpected privilege transitions |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| `chroot()` escape risks | Not enough for set-user-ID-root confinement |
| set-user-ID scripts | Linux ignores setuid/setgid bits on scripts; other systems may differ |
| ASLR and NX | Mitigations that make exploitation harder, not replacements for safe code |
| `setresuid()` portability | Linux-friendly and clear, but not universal POSIX |
| `mlock()` for secrets | Can reduce swap exposure but requires resource/capability considerations |
| Algorithmic-complexity attacks | Valid-looking inputs can force worst-case CPU/memory behavior |
| capabilities/securebits | Better least-privilege model for some root/setuid designs |

## Example

### Example 1: Temporarily Drop and Reacquire Effective UID

```c
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    uid_t privileged_euid = geteuid();

    if (seteuid(getuid()) == -1)
        die("drop effective uid");

    if (geteuid() != getuid()) {
        fprintf(stderr, "effective uid did not drop\n");
        exit(EXIT_FAILURE);
    }

    /* Do unprivileged parsing or validation here. */

    if (seteuid(privileged_euid) == -1)
        die("reacquire effective uid");

    if (getresuid(&ruid, &euid, &suid) == -1)
        die("getresuid");

    printf("ruid=%ld euid=%ld suid=%ld\n",
           (long) ruid, (long) euid, (long) suid);

    return EXIT_SUCCESS;
}
```

Build:

```bash
gcc -Wall -Wextra -g temp_priv.c -o temp_priv
```

What it teaches:

- temporary drop relies on saved set-user-ID;
- security-relevant credential changes should be verified.

### Example 2: Permanently Drop All UIDs on Linux

```c
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    uid_t target = getuid();
    uid_t ruid;
    uid_t euid;
    uid_t suid;

    if (setresuid(target, target, target) == -1) {
        perror("setresuid");
        return EXIT_FAILURE;
    }

    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresuid");
        return EXIT_FAILURE;
    }

    if (ruid != target || euid != target || suid != target) {
        fprintf(stderr, "privilege drop incomplete\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Build:

```bash
gcc -Wall -Wextra -g drop_priv.c -o drop_priv
```

What it teaches:

- permanent drop must remove the saved privileged ID;
- Linux `setresuid()` makes the intent explicit.

### Example 3: Disable Core Dumps and Create a Safe Temporary File

```c
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    struct rlimit rl = {0, 0};
    char tmpl[] = "/tmp/myhelper.XXXXXX";
    int fd;

    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("setrlimit");
        return EXIT_FAILURE;
    }

    umask(077);

    fd = mkstemp(tmpl);
    if (fd == -1) {
        perror("mkstemp");
        return EXIT_FAILURE;
    }

    puts(tmpl);
    close(fd);
    unlink(tmpl);
    return EXIT_SUCCESS;
}
```

Build:

```bash
gcc -Wall -Wextra -g safe_file.c -o safe_file
```

What it teaches:

- secrets should not end up in core dumps;
- temporary files need unpredictable names and safe permissions.

## Debugging

Useful commands:

```bash
ls -l ./helper
find / -perm -4000 -type f -ls 2>/dev/null
grep '^Uid:\|^Gid:' /proc/<pid>/status
ls -l /proc/<pid>/fd
readlink /proc/<pid>/fd/<n>
strace -f -e trace=execve,setuid,setresuid,setgid,setresgid,openat,fcntl ./helper
cat /proc/<pid>/limits
ulimit -c
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Privilege not permanently dropped | Child process can regain privilege | Verify real/effective/saved IDs |
| Descriptor leak across `exec()` | Child can read privileged file | Use `O_CLOEXEC`, `FD_CLOEXEC`, or close explicitly |
| `system()` used with privilege | User controls command execution | Avoid shell; use fixed absolute path after dropping privilege |
| TOCTOU path race | Attacker swaps symlink/file between check and use | Use `open()` then `fstat()` |
| Unsafe `/tmp` file | Symlink or precreation attack | Use `mkstemp()` or avoid public writable directories |
| Closed stdio fd reused | Program writes secrets to unexpected file | Ensure fds 0, 1, 2 are open to safe targets |
| Secrets in crash dump | Core file exposes password/token | Set `RLIMIT_CORE` to 0 |
| Error path continues unsafely | Unexpected state becomes exploit path | Fail closed |

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Privileged port setup | Start privileged, bind socket, drop privilege permanently |
| Authentication helper | Keep parser unprivileged; raise privilege only to read protected data |
| Installer/updater | Avoid shell; use absolute paths and sanitized environment |
| Embedded maintenance tool | Prefer dedicated group or capability over set-user-ID-root |
| Network daemon | Validate all input, set limits/timeouts, rate-limit logs, drop privilege early |
| File-creating helper | Use restrictive `umask`, `open()`/`fstat()`, `fchown()`/`fchmod()` on fd |

## Interview-Relevant Questions

1. Why should set-user-ID-root programs be avoided when possible?
2. What is least privilege in the context of Linux system programming?
3. What is the difference between temporary and permanent privilege drop?
4. Why can `setuid(getuid())` be insufficient if called after temporarily dropping root?
5. Why are `setresuid()` and `setresgid()` often clearer on Linux?
6. Why must credential changes be verified after the syscall succeeds?
7. Why is `system()` dangerous in privileged programs?
8. Why should privileged programs avoid `execlp()` and `execvp()`?
9. What is a file-descriptor leak across `exec()`, and why is it dangerous?
10. What is close-on-exec, and when should it be used?
11. What is a TOCTOU race? Give a file-system example.
12. Why is `open()` followed by `fstat()` safer than `stat()` followed by `open()`?
13. Why are predictable files in `/tmp` dangerous?
14. Why should sensitive data be erased and core dumps disabled?
15. Why should privileged programs distrust environment variables such as `PATH` and `IFS`?
16. What assumptions about standard file descriptors can be unsafe?
17. How do signals widen race windows in privileged programs?
18. How do capabilities reduce risk compared with full root?
19. What does "fail closed" mean?
20. How would you debug whether a helper leaked privilege or file descriptors?

## Key Takeaways

- Avoid privileged programs when possible; narrow privilege when unavoidable.
- Hold privilege only for the smallest necessary code region.
- Permanent privilege drop must remove the saved privileged ID and should be verified.
- Never casually execute shells or path-searched programs with privilege.
- File descriptors, pathnames, environment variables, signals, and temporary files are attack surfaces.
- Validate open descriptors, not stale path checks.
- Check results and fail safely instead of continuing from uncertain state.
