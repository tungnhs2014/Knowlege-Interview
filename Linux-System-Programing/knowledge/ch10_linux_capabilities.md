# Chapter 10.3 - Linux Capabilities

> Topics: `CAP_*`, process capability sets, file capabilities, capability bounding set, `exec()` transformation, `setcap`, `getcap`, `getpcap`, `libcap`, `securebits`.
> Main sources: TLPI Ch39; TLPI Ch38 for least-privilege context.
> Production context: used to run services and helpers with narrow privileges instead of full root, especially network daemons, authentication helpers, containerized services, and hardened embedded Linux programs.

---

## Problem It Solves

Traditional UNIX privilege is coarse:

- effective UID 0 gets broad superuser power;
- non-root processes get normal permission checks.

That is too much power for programs that need only one privileged operation. A web server may only need to bind port 80. A diagnostic tool may only need raw sockets. An authentication helper may only need temporary access to protected password data.

Linux capabilities split root privilege into smaller units. Instead of giving a process all root power, you can grant only the capability required for the specific operation.

The production goal is least privilege: if the process is compromised, the attacker gets a smaller set of actions.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | Why capabilities exist, `permitted` vs `effective`, common examples such as `CAP_NET_BIND_SERVICE`, `setcap`, `getcap` | Replace simple root/setuid cases with narrower privilege |
| Work useful | file capabilities, file effective bit, `exec()` transformation, bounding set, `libcap`, `/proc/PID/status`, `strace EPERM` | Design and debug least-privilege services |
| Recognize | securebits, root compatibility rules, inheritable set edge cases, older kernels without file capabilities | Understand deep capability behavior without memorizing every flag |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Capability | One unit of Linux privilege checked by the kernel | `CAP_NET_BIND_SERVICE` |
| Effective UID 0 | Traditional root privilege model | Still has compatibility behavior |
| Permitted set | Capabilities a thread may make effective | Upper bound for effective set |
| Effective set | Capabilities currently used by kernel privilege checks | If not effective, the operation fails |
| Inheritable set | Capabilities that may participate in `exec()` inheritance | Does not directly grant privilege |
| File capability | Capability metadata attached to an executable | Stored in `security.capability` xattr |
| File permitted set | Capabilities a file can grant during `exec()` | Masked by bounding set |
| File inheritable set | Capabilities a file allows from process inheritable set | ANDed with process inheritable |
| File effective bit | Single bit saying whether new permitted caps become effective | Useful for capability-dumb programs |
| Bounding set | Limit on capabilities that may be gained through `exec()` | Can be irreversibly reduced |
| Capability-aware program | Program that explicitly raises/drops capabilities | Safer for least privilege |
| Capability-dumb program | Program expects privilege to already be effective | Needs file effective bit for file caps |
| `libcap` | Preferred library API for changing capabilities | `cap_get_proc()`, `cap_set_proc()` |
| `setcap` / `getcap` | Tools for assigning and viewing file capabilities | From libcap package |
| `getpcap` | Tool to view process capabilities | Easier than raw `/proc` hex |
| `securebits` | Flags controlling root compatibility behavior | Managed with `prctl()` |
| `EPERM` | Common failure when a needed capability is missing | Trace with `strace` |

## Concept Overview

The simple model:

```text
kernel privileged operation
    |
    v
does current thread have required capability in effective set?
    |
    +-- yes -> allow
    |
    +-- no  -> fail, often EPERM
```

The important distinction:

- `permitted` means "this thread is allowed to enable this capability";
- `effective` means "the kernel currently counts this capability";
- `inheritable` means "this capability may survive an `exec()` if the target file allows it".

Capabilities are mostly invisible when a process runs as root because Linux preserves traditional root behavior. The useful design move is to stop thinking "root or not root" and start thinking "which exact capability is needed for this operation, and for how long?"

## System Context

| Area | How capabilities affect it |
|------|----------------------------|
| Kernel permission checks | Many privileged operations check a specific capability |
| Process/thread model | Capability sets are per-thread attributes on Linux |
| `fork()` | Child inherits copies of capability sets |
| `exec()` | New sets are computed from process sets, file sets, and bounding set |
| Filesystem | File capabilities live in `security.capability` extended attribute |
| User ID changes | Transitions to/from UID 0 change capability sets for compatibility |
| Debugging | `/proc`, `getcap`, `getpcap`, and `strace` reveal missing privilege |

Linux capability names and exact coverage are kernel-version dependent. Do not memorize the full list. In production, identify the operation, consult `capabilities(7)` or the relevant syscall man page, then grant the smallest required capability.

## Architecture

### Process Capability Sets

```text
thread
    |
    +-- permitted    # may become effective
    +-- effective    # checked by kernel now
    +-- inheritable  # may participate in exec()
    +-- bounding     # limits future gains through exec()
```

TLPI uses "process" terminology, but on Linux these sets are per-thread. `/proc/PID/status` shows the main thread; `/proc/PID/task/TID/status` shows a specific thread.

### File Capability Sets

```text
executable file
    |
    +-- permitted capability set
    +-- inheritable capability set
    +-- effective bit
```

The file effective set is not a full capability mask. It is a single bit:

- set: new permitted capabilities also become effective after `exec()`;
- clear: new effective set starts empty, and the program must raise capabilities itself.

## Execution Flow

### Flow 1: Kernel Privilege Check

```text
process calls privileged syscall
    |
    v
kernel identifies required CAP_*
    |
    v
check thread effective set
    |
    +-- present -> operation succeeds
    |
    +-- absent  -> operation fails, often EPERM
```

### Flow 2: File Capability Startup

```text
admin sets file capability
    |
    v
user execs file
    |
    v
kernel combines process sets + file sets + bounding set
    |
    v
new program starts with computed capability sets
```

### Flow 3: Capability-Aware Operation

```text
start with capability in permitted set
    |
    v
raise capability into effective set
    |
    v
perform privileged operation
    |
    v
drop capability from effective set
    |
    v
drop all capabilities when no longer needed
```

### Flow 4: Bounding-Set Hardening

```text
service starts with broad possible capabilities
    |
    v
drop capabilities never needed from bounding set
    |
    v
future exec() cannot regain those capabilities
```

## 10.3 API / Topic Sections

### 10.3.1 Why Capabilities Exist

Capabilities reduce the blast radius of privileged programs.

Examples:

| Need | Capability example |
|------|--------------------|
| Bind to ports below 1024 | `CAP_NET_BIND_SERVICE` |
| Use raw sockets | `CAP_NET_RAW` |
| Change system time | `CAP_SYS_TIME` |
| Bypass some file read/search checks | `CAP_DAC_READ_SEARCH` |
| Send signals beyond normal permission rules | `CAP_KILL` |
| Set file capabilities | `CAP_SETFCAP` |

Avoid treating this table as complete. Exact capability coverage should be checked against the running kernel's documentation.

### 10.3.2 Process Permitted, Effective, and Inheritable Sets

The permitted set is the ceiling. If a capability is removed from permitted, the thread cannot make it effective again unless a later `exec()` grants it.

The effective set is what the kernel actually checks.

The inheritable set matters during `exec()`. It does not let the current code perform privileged operations by itself.

### 10.3.3 File Capabilities

File capabilities let an executable grant selected capabilities when it is executed:

```bash
sudo setcap cap_net_bind_service=ep ./server
getcap ./server
```

File capabilities are stored as an extended attribute named `security.capability`. Updating it requires `CAP_SETFCAP`.

Use file capabilities when a program should run as a normal user but needs one narrow privileged operation.

### 10.3.4 `exec()` Transformation

During `exec()`, Linux computes new process capability sets:

```text
P'(permitted)   = (P(inheritable) & F(inheritable)) | (F(permitted) & cap_bset)
P'(effective)   = F(effective) ? P'(permitted) : 0
P'(inheritable) = P(inheritable)
```

Where:

- `P` is the old process capability state;
- `P'` is the new process capability state;
- `F` is file capability state;
- `cap_bset` is the capability bounding set.

Most first-pass debugging focuses on `P'(permitted)`, `P'(effective)`, and whether the file effective bit is set.

### 10.3.5 Root Compatibility and User ID Changes

Linux preserves traditional root semantics. A root process or a set-user-ID-root `exec()` can still receive broad capabilities, which keeps old programs working.

User ID changes also affect capabilities:

- moving from any UID 0 state to all nonzero UIDs clears permitted and effective sets;
- changing effective UID from 0 to nonzero clears effective capabilities;
- changing effective UID from nonzero to 0 copies permitted into effective;
- filesystem UID changes affect some file-related capabilities.

Production pitfall: privilege-dropping code that also manipulates capabilities must verify the final UID and capability state, not just check one return value.

### 10.3.6 `libcap` Workflow

Prefer `libcap` over raw `capget()` and `capset()`.

Typical flow:

```text
cap_get_proc()
    |
    v
cap_set_flag()
    |
    v
cap_set_proc()
    |
    v
cap_free()
```

Rules:

- a process cannot add a capability to permitted if it does not already have it;
- effective capabilities must be a subset of permitted;
- inheritable changes are constrained by current state and the bounding set.

### 10.3.7 Debug and Shell Tools

```bash
getcap ./program
getpcap <pid>
grep '^Cap' /proc/<pid>/status
grep '^Cap' /proc/<pid>/task/<tid>/status
strace -e trace=%file,%process,%network ./program
```

`strace` is useful because missing capability often appears as `EPERM`. The syscall that fails tells you which manual page or kernel capability check to inspect.

### 10.3.8 Securebits and Capability-Only Environments

`securebits` flags allow a process to disable some traditional special treatment of root. Important names include:

- `SECBIT_KEEP_CAPS`
- `SECBIT_NO_SETUID_FIXUP`
- `SECBIT_NOROOT`
- corresponding `_LOCKED` variants

This is advanced. First understand normal capability sets, file capabilities, and bounding sets.

## Work-Useful Patterns

| Pattern | Why it helps |
|---------|--------------|
| Use file capability instead of set-user-ID-root | Grants one operation instead of broad root power |
| Keep capability effective only around the privileged syscall | Reduces exploit window |
| Drop all capabilities after privileged setup | Lowers blast radius for long-lived services |
| Drop unused capabilities from bounding set | Prevents future regain through `exec()` |
| Verify UID and capability state after privilege changes | Avoids silent partial privilege changes |
| Debug `EPERM` with `strace` and `/proc/PID/status` | Finds the missing privilege path |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Inheritable set | Mostly matters for preserving selected capabilities across `exec()` |
| Securebits | Controls root compatibility behavior for specialized hardened designs |
| `PR_SET_KEEPCAPS` | Allows keeping permitted caps while changing all UIDs away from 0 |
| Older kernels without file capabilities | Required set-user-ID-root plus programmatic capability dropping |
| Per-thread capabilities | Threads in one process can have different capability state |
| Capability list growth | New kernels can add capabilities; check `capabilities(7)` |

## Example

### Example 1: Grant a Server Permission to Bind a Privileged Port

```bash
gcc -Wall -Wextra -g -o tiny_server tiny_server.c
sudo setcap cap_net_bind_service=ep ./tiny_server
getcap ./tiny_server
./tiny_server
```

What it teaches:

- the program can bind a low port without running as root;
- the `e` bit makes the capability effective at startup for a capability-dumb program.

### Example 2: Raise and Drop an Effective Capability with `libcap`

```c
#include <sys/capability.h>
#include <stdio.h>
#include <stdlib.h>

static int set_effective_cap(cap_value_t capability, cap_flag_value_t value) {
    cap_t caps;
    cap_value_t list[1];

    caps = cap_get_proc();
    if (caps == NULL)
        return -1;

    list[0] = capability;
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, list, value) == -1) {
        cap_free(caps);
        return -1;
    }

    if (cap_set_proc(caps) == -1) {
        cap_free(caps);
        return -1;
    }

    return cap_free(caps);
}

static int drop_all_caps(void) {
    cap_t empty = cap_init();
    int result;

    if (empty == NULL)
        return -1;

    result = cap_set_proc(empty);
    if (cap_free(empty) == -1)
        return -1;

    return result;
}

int main(void) {
    if (set_effective_cap(CAP_NET_BIND_SERVICE, CAP_SET) == -1) {
        perror("raise capability");
        return EXIT_FAILURE;
    }

    /* Do the privileged operation here. */

    if (set_effective_cap(CAP_NET_BIND_SERVICE, CAP_CLEAR) == -1) {
        perror("drop effective capability");
        return EXIT_FAILURE;
    }

    if (drop_all_caps() == -1) {
        perror("drop all capabilities");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Build:

```bash
gcc -Wall -Wextra -g cap_demo.c -o cap_demo -lcap
```

What it teaches:

- permitted capability is the pool;
- effective capability is enabled only when needed;
- once all privileged work is done, drop everything.

## Debugging

Useful commands:

```bash
getcap ./program
getpcap $(pidof program)
grep '^Cap' /proc/$(pidof program)/status
strace -e trace=%file,%network,%process ./program
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Capability is permitted but not effective | Syscall still fails with `EPERM` | Check `CapEff`; set file effective bit or raise with `libcap` |
| Dropped from permitted too early | Program cannot regain capability | Keep in permitted until all privileged work is done |
| Bounding set removed capability | `exec()` does not grant expected cap | Inspect `CapBnd` |
| Wrong capability chosen | `EPERM` remains | Trace failing syscall and check `capabilities(7)` |
| Running as root hides design bugs | Works in dev, fails as service user | Test as intended non-root user |
| Thread capability mismatch | One thread succeeds, another fails | Inspect `/proc/PID/task/TID/status` |

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| HTTP service on port 80 | Grant `CAP_NET_BIND_SERVICE`, bind socket, then drop capability |
| Packet capture tool | Consider `CAP_NET_RAW` / related network capabilities instead of root |
| Authentication helper | Keep file-read bypass capability effective only during protected read |
| Container/service hardening | Drop unused bounding-set capabilities |
| Legacy privileged helper | Replace set-user-ID-root where file capabilities are available |

## Interview-Relevant Questions

1. Why were Linux capabilities introduced?
2. What is the difference between UID 0 privilege and capabilities?
3. What is the difference between permitted and effective capabilities?
4. Why does a capability in the permitted set not automatically grant privilege?
5. What is the inheritable capability set used for?
6. What are file capabilities, and where are they stored?
7. Why is the file effective capability set only a single bit?
8. What happens to capabilities during `fork()`?
9. What happens to capabilities during `exec()`?
10. What does the capability bounding set protect against?
11. Why is dropping a capability from the permitted set usually irreversible for the current program?
12. How do user ID changes interact with capabilities on Linux?
13. Why should capability-aware programs raise capabilities only briefly?
14. How would you let a non-root program bind to port 80?
15. How would you debug a syscall failing with `EPERM`?
16. Why can root compatibility make capability behavior confusing?
17. What is `libcap`, and why prefer it over raw `capset()`?
18. What are securebits, and why are they advanced?

## Key Takeaways

- Capabilities split root privilege into smaller kernel-checked units.
- The effective set is what the kernel checks; the permitted set is what may become effective.
- File capabilities can replace many set-user-ID-root cases with narrower privilege.
- `exec()` capability results depend on process sets, file sets, and the bounding set.
- Linux preserves root compatibility, so UID changes and capabilities must be understood together.
- Debug missing privilege with `getcap`, `getpcap`, `/proc/PID/status`, and `strace`.
