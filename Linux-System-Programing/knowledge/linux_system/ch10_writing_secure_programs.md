# Writing Secure Programs - Least Privilege, Safe `exec()`, TOCTOU, Input Validation, and Defensive Failure

## Problem It Solves

Privileged programs can access resources and perform operations that ordinary users cannot. Such programs include:

- daemons or servers started with privileged user IDs;
- set-user-ID or set-group-ID executables;
- programs that temporarily acquire privilege in order to perform a protected task.

If a privileged program contains bugs, makes unsafe assumptions, or is manipulated by a malicious user, the consequences can be severe:

- unauthorized file access;
- unintended command execution;
- privilege escalation;
- leakage of sensitive information;
- denial of service.

This topic provides the defensive design rules needed to reduce both the chance of compromise and the damage caused if compromise occurs.

## Concept Overview

Secure privileged programming is not about one specific API. It is about applying a consistent set of defensive principles.

The central ideas in this chapter are:

- avoid privileged programs whenever possible;
- operate with the least privilege needed for the current task;
- permanently drop privilege when it will never be needed again;
- be extremely careful when executing another program;
- avoid exposing sensitive information;
- confine the process where possible;
- defend against signals, race conditions, and time-of-check/time-of-use flaws;
- perform file operations in ways that resist malicious interference;
- distrust inputs, environment variables, and run-time assumptions;
- avoid buffer overruns;
- defend against denial-of-service conditions;
- always check results and fail safely.

These ideas apply especially strongly to programs running with elevated privilege, but they also improve the robustness of ordinary software.

## System Context

Secure privileged programming touches many parts of the Linux process model:

- user IDs, group IDs, and saved IDs;
- `exec()` behavior and file-descriptor inheritance;
- file permissions, ownership, and temporary files;
- environment variables;
- signals;
- resource limits and core dumps;
- capabilities and privilege dropping.

Relevant APIs and mechanisms include:

- `seteuid()`, `setuid()`, `setreuid()`, `setresuid()`;
- `setegid()`, `setgid()`, `setregid()`, `setresgid()`;
- `exec()` and close-on-exec file descriptors;
- `setrlimit()` for disabling core dumps;
- `mkstemp()` for safe temporary-file creation;
- `fstat()`, `fchown()`, `fchmod()` for file checks on open descriptors;
- capabilities and `chroot()` as confinement techniques.

This topic is connected to:

- process credentials;
- capabilities and least privilege;
- safe file-system usage;
- defensive server design;
- secure handling of user-controlled input.

## Architecture

## 1) Decide whether privilege is necessary at all

The best privileged program is often the one that was never written.

Before using set-user-ID or set-group-ID:

- ask whether the task can be redesigned to avoid privilege;
- consider moving the privileged action into a small helper program;
- consider using a dedicated group instead of root;
- consider capabilities instead of full superuser privilege.

If a program needs privilege only for one narrow purpose, then designing for narrower privilege reduces the consequences of compromise.

## 2) Least privilege and privilege lifetime

A program should hold privilege only while the current operation requires it.

Practical rules:

- drop privilege immediately at startup if it is not needed yet;
- temporarily reacquire it only around the privileged operation;
- permanently drop it once it will never again be required.

Temporarily dropping privilege typically uses the saved set-user-ID or saved set-group-ID mechanism. Permanently dropping privilege requires changing all relevant IDs so that privilege cannot later be regained.

This distinction matters:

- temporary drop reduces risk during unprivileged work;
- permanent drop removes the possibility of later accidental or malicious privilege reacquisition.

## 3) Changing credentials safely

Credential-changing system calls can be subtle:

- semantics differ across systems;
- behavior may depend on whether the process is currently privileged;
- Linux-specific capabilities can affect whether a requested change actually succeeds.

TLPI recommends care when choosing which credential-changing calls to use. On Linux, `setresuid()` and `setresgid()` often provide clearer behavior than older interfaces.

Good practice is not only to check return values, but also to verify the resulting credential state after the call, using functions such as:

- `geteuid()`
- `getegid()`
- `getresuid()`
- `getresgid()`

When changing multiple IDs:

- drop privileged effective user ID last when dropping privilege;
- raise privileged effective user ID first when regaining privilege.

## 4) Executing another program securely

Executing another program from a privileged process is dangerous.

Main rules:

- permanently drop privilege before `exec()` if the new program does not require it;
- never execute a shell or similar interpreter with privilege if the user has any control over inputs or environment;
- avoid `system()`, `popen()`, `execlp()`, and `execvp()` in privileged contexts;
- close all unnecessary file descriptors before `exec()`, or set them close-on-exec.

Why file descriptors matter:

- a privileged program may hold open descriptors to files or devices an ordinary user cannot access;
- if those descriptors survive across `exec()`, the new program inherits privileged resources.

Linux also ignores set-user-ID and set-group-ID bits on scripts, which avoids a dangerous class of privileged interpreter behavior.

## 5) Protect sensitive information

Sensitive data such as passwords should be erased from memory as soon as possible after use.

Why:

- memory may be swapped to disk;
- a core dump could expose the data;
- the longer data remains in memory, the larger the attack surface.

Secure practices include:

- zeroing secret buffers once no longer needed;
- preventing core dumps using `setrlimit(RLIMIT_CORE, ...)`;
- keeping the lifetime of sensitive data as short as possible.

## 6) Confine the process

If compromise occurs, confinement can reduce the damage.

Options discussed by TLPI:

- use Linux capabilities rather than full root privilege;
- consider a `chroot()` jail where appropriate;
- remember that `chroot()` is not sufficient confinement for a set-user-ID-root program.

The goal is not to make compromise impossible, but to reduce what a compromised process can reach.

## 7) Signals, race conditions, and TOCTOU

Signals can arrive at arbitrary times, including times that matter for security.

A privileged process may:

1. check some property of its environment;
2. get stopped or interrupted;
3. resume later after the environment has changed;
4. continue based on assumptions that are no longer true.

This is a time-of-check/time-of-use problem.

Important consequences:

- signal handlers should remain simple;
- sensitive regions may require blocking, ignoring, or carefully handling signals;
- programs should avoid relying on checks whose validity can disappear before use.

The general lesson is broader than signals: a privileged program should avoid acting on stale verification results.

## 8) Safe file operations

Privileged file handling must avoid windows for malicious manipulation.

Important rules:

- set a restrictive `umask`;
- ensure files are created with safe ownership and mode from the beginning;
- if ownership must later change, create the file safely first, then use `fchown()` and `fchmod()` in the correct order;
- prefer `open()` followed by `fstat()` instead of `stat()` followed by `open()`;
- use `O_EXCL` when the program must ensure it is the creator of a file;
- avoid predictable files in publicly writable directories such as `/tmp`;
- if a temporary file in a public directory is unavoidable, use `mkstemp()`.

The core idea is to avoid path-based checks and avoid giving attackers a chance to replace, redirect, or pre-create files.

## 9) Distrust inputs, environment, and run-time assumptions

Privileged programs must treat untrusted input broadly.

Untrusted sources include:

- command-line arguments;
- interactive input;
- user-controlled files;
- environment variables;
- IPC channels accessible to untrusted users;
- network input.

Validation may include:

- numeric range checks;
- maximum-length checks;
- accepted-character checks;
- structural and semantic validation.

Environment variables deserve special caution:

- `PATH` can redirect program lookup for shell-based execution and path-searching `exec` variants;
- `IFS` can alter shell word splitting;
- in high-risk cases, it may be safest to erase the environment and rebuild only selected safe variables.

Programs should also avoid assumptions about the run-time environment. For example:

- standard descriptors may already be closed;
- resource limits may be exhausted;
- signals may occur unexpectedly;
- files and directories may not remain as previously observed.

## 10) Buffer overruns and denial of service

Buffer overruns remain a major source of compromise.

Rules:

- never use `gets()`;
- use functions such as `scanf()`, `sprintf()`, `strcpy()`, and `strcat()` with extreme caution;
- prefer bounded alternatives such as `snprintf()`, `strncpy()`, and `strncat()`, while still checking for truncation and null-termination issues.

Denial-of-service resilience is also part of secure programming.

Examples of good practice:

- throttle work under overload;
- apply timeouts;
- log overload conditions without allowing logging itself to become an attack vector;
- design data structures that resist algorithmic-complexity attacks;
- rigorously check bounds to avoid crashes under malformed or excessive inputs.

## Execution Flow

### 1) Secure privilege-handling flow

1. determine whether privileged execution is necessary at all;
2. if privilege is required, keep it as narrow as possible;
3. drop privilege immediately when it is not needed;
4. reacquire it only for specific privileged work;
5. permanently drop it once all privileged work is complete;
6. verify that the drop actually occurred.

### 2) Secure `exec()` flow

1. decide whether the target program truly needs privilege;
2. if not, permanently drop privilege first;
3. avoid shell-based execution and path-searching `exec` variants;
4. use absolute pathnames;
5. close or mark unnecessary descriptors close-on-exec;
6. perform the `exec()`.

### 3) Secure file-creation flow

1. establish a safe `umask`;
2. open the file safely, using `O_EXCL` if uniqueness matters;
3. check properties on the open descriptor with `fstat()`;
4. apply ownership and permission changes in a safe order;
5. avoid exposing a writable privileged-owned file even briefly.

### 4) Defensive-failure flow

1. check every important return status;
2. verify result semantics where needed;
3. if behavior is unexpected, terminate or reject the request;
4. avoid "repairing" the situation based on uncertain assumptions.

## Example

## 1) Temporarily drop and reacquire effective privilege

```c
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    uid_t orig_euid = geteuid();

    if (seteuid(getuid()) == -1) {
        perror("seteuid drop");
        exit(EXIT_FAILURE);
    }

    /* Do unprivileged work here */

    if (seteuid(orig_euid) == -1) {
        perror("seteuid regain");
        exit(EXIT_FAILURE);
    }

    /* Do privileged work here */
    return 0;
}
```

## 2) Safe temporary-file creation

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    char tmpl[] = "/tmp/myprog.XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) {
        perror("mkstemp");
        exit(EXIT_FAILURE);
    }

    printf("Created temp file: %s\n", tmpl);

    close(fd);
    return 0;
}
```

## 3) Prevent core dumps when handling secrets

```c
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    struct rlimit rl;

    rl.rlim_cur = 0;
    rl.rlim_max = 0;

    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("setrlimit");
        exit(EXIT_FAILURE);
    }

    /* Handle sensitive data here */
    return 0;
}
```

## Debugging

Useful review questions and checks:

- does the program truly need privilege?
- is privilege dropped immediately when unnecessary?
- is permanent privilege drop implemented correctly and verified?
- does any call to `exec()` retain privilege unintentionally?
- are file descriptors leaked across `exec()`?
- are temporary files created safely?
- is any pathname checked before open instead of validating an open descriptor?
- are environment variables sanitized or ignored appropriately?
- can signals or process suspension invalidate earlier checks?
- are buffer sizes validated and truncation outcomes checked?
- does the program fail safely when it encounters unexpected conditions?

Common pitfalls:

- retaining privilege for convenience;
- using `system()` in privileged code;
- trusting `PATH`, `IFS`, or other environment variables;
- using `stat()` followed by `open()`;
- relying on predictable names in `/tmp`;
- assuming standard descriptors are always open;
- handling unexpected errors by "trying to continue anyway."

## Real-world Usage

These rules apply to:

- set-user-ID and set-group-ID helpers;
- authentication and account-management tools;
- package-management or installation helpers;
- privileged daemons and network services;
- any service that reads untrusted input while holding privilege.

Even ordinary applications benefit from these practices, but privileged programs need them urgently because mistakes have system-wide consequences.

## Key Takeaways

- Avoid privileged programs when possible.
- If privilege is necessary, hold it only as long as required.
- Prefer permanent privilege drop once privileged work is finished.
- Never casually execute a shell or interpreter with privilege.
- Treat pathnames, temporary files, environment variables, and user input as attacker-controlled.
- Design file operations to avoid TOCTOU windows and temporary unsafe states.
- Check results carefully, and fail safely when something unexpected happens.

