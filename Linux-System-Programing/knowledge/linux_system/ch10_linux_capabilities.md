# Linux Capabilities - `CAP_*`, Least Privilege, File Capabilities, Bounding Sets, and `libcap`

## Problem It Solves

The traditional UNIX privilege model is coarse-grained:

- a process with effective user ID 0 is effectively privileged for almost everything;
- a process without user ID 0 is constrained by normal permission checks.

This all-or-nothing model is often too broad. Many programs need only a small subset of privileged operations, such as:

- binding to a privileged port;
- changing the system time;
- bypassing certain file permission checks;
- managing selected network or IPC operations.

Running such a program as root, or making it set-user-ID-root, grants far more power than necessary. If the program is buggy or exploited, that excess privilege becomes a security risk.

Linux capabilities refine the traditional superuser model by splitting privilege into individual capability units. A process can then be granted only the capabilities required for the operations it must perform.

## Concept Overview

In Linux, privileged operations are associated with specific capabilities rather than with a single "superuser" switch.

Examples include:

- `CAP_SYS_TIME` for changing the system time;
- `CAP_NET_BIND_SERVICE` for binding to privileged ports;
- `CAP_DAC_READ_SEARCH` for bypassing certain file read/search permission checks;
- `CAP_KILL` for bypassing signal-sending permission checks;
- `CAP_SETFCAP` for modifying file capabilities.

Capabilities exist for both processes and executable files.

For a process, the kernel maintains three capability sets:

- **permitted**: capabilities the process may use;
- **effective**: capabilities currently used for privilege checks;
- **inheritable**: capabilities that may be preserved across `exec()`.

For an executable file, the kernel likewise maintains:

- **permitted** capabilities;
- **inheritable** capabilities;
- an **effective bit** indicating whether the new process should begin execution with its permitted capabilities also enabled in its effective set.

This model supports least privilege by allowing a program to retain only the capabilities it really needs and to enable them only when required.

## System Context

Capabilities affect:

- kernel privilege checks;
- process creation and `exec()` semantics;
- transitions between root and non-root user IDs;
- executable-file metadata;
- secure process design.

Relevant system components include:

- process capability sets;
- file capability sets stored in the `security.capability` extended attribute;
- the capability bounding set;
- `prctl()` operations related to capabilities and securebits;
- the `libcap` library and its helper commands such as `setcap` and `getcap`;
- `/proc/PID/status`, which exposes capability state through fields such as `CapInh`, `CapPrm`, `CapEff`, and `CapBnd`.

This topic is directly connected to:

- set-user-ID-root program hardening;
- daemon and service security;
- privilege dropping and privilege reacquisition;
- file-based privilege assignment;
- designing applications around least privilege.

## Architecture

## 1) Process capability sets

Each process has three capability sets, implemented as bit masks.

### Permitted set

This is the set of capabilities that the process may employ.

Important properties:

- it is a limiting superset for the effective and inheritable sets;
- a capability dropped from the permitted set cannot be regained by the process, unless a later `exec()` grants it again.

### Effective set

This is the set actually used by the kernel when checking whether the process may perform a privileged operation.

Important properties:

- a capability can be made effective only if it is already in the permitted set;
- a process may temporarily drop a capability from the effective set and later restore it, as long as it remains in the permitted set.

### Inheritable set

This set contains capabilities that may be carried across `exec()`.

It does not directly grant privilege on its own. Instead, it participates in the `exec()` transformation together with the file inheritable set.

## 2) File capability sets

Executable files also have capability metadata.

### File permitted set

This specifies capabilities that may be added to the new process's permitted set during `exec()`, regardless of the process's existing capabilities.

### File inheritable set

This is ANDed with the process inheritable set to determine which inheritable capabilities become permitted after `exec()`.

### File effective set

For files, the effective set is really a single bit, not a full capability mask.

Its purpose is to answer the question:

- should the process begin execution with its newly permitted capabilities already enabled in its effective set?

If the bit is enabled:

```text
P'(effective) = P'(permitted)
```

If the bit is disabled:

```text
P'(effective) = 0
```

This distinction supports two different program styles:

- **capability-dumb** programs, which rely on capabilities being effective immediately;
- **capability-aware** programs, which raise and drop capabilities explicitly as needed.

The file capabilities are stored in the `security.capability` extended attribute. Updating that attribute requires the `CAP_SETFCAP` capability.

## 3) `exec()` capability transformation

During `exec()`, the kernel computes the new process capability sets using the current process sets, the file capability sets, and the capability bounding set.

The transformation rules are:

```text
P'(permitted)   = (P(inheritable) & F(inheritable)) | (F(permitted) & cap_bset)
P'(effective)   = F(effective) ? P'(permitted) : 0
P'(inheritable) = P(inheritable)
```

Where:

- `P` is the process before `exec()`;
- `P'` is the process after `exec()`;
- `F` is the executed file's capability information;
- `cap_bset` is the capability bounding set.

This model means that privilege granted after `exec()` is controlled by:

- what the process chooses to preserve via its inheritable set;
- what the executable allows or forces via its file capabilities;
- what the bounding set permits at all.

## 4) Capability bounding set

The capability bounding set is a security mechanism that limits which capabilities a process may gain during `exec()`.

Its roles are:

- it masks the file permitted set during `exec()`;
- it limits which capabilities may be added to the process inheritable set.

Important properties:

- it is inherited across `fork()`;
- it is preserved across `exec()`;
- if a process has `CAP_SETPCAP`, it can irreversibly drop capabilities from the bounding set using `prctl(PR_CAPBSET_DROP, ...)`;
- it can inspect the set using `prctl(PR_CAPBSET_READ, ...)`.

Dropping a capability from the bounding set does not directly remove it from the process's current permitted or effective sets, but it prevents future gains of that capability through the `exec()` path.

## 5) Relationship with root semantics

Linux maintains special compatibility behavior for root.

When a process with real or effective user ID 0 executes a program, or when a set-user-ID-root program is executed, the kernel preserves traditional root semantics by notionally treating the file capability sets as granting broad privilege.

This means:

- legacy programs can continue to behave as expected when run as root;
- the capabilities system often remains invisible to applications unaware of it.

However, this also means that capabilities and root semantics coexist. A process designer must understand both.

## 6) Effect of changing user IDs

Linux changes capability sets in order to preserve traditional meanings of transitions between root and non-root IDs.

Key rules:

- if one of the real, effective, or saved set-user-ID values was 0, and after the change all three are nonzero, the permitted and effective sets are cleared;
- if the effective user ID changes from 0 to nonzero, the effective set is cleared;
- if the effective user ID changes from nonzero to 0, the permitted set is copied into the effective set;
- changes to the file-system user ID similarly affect some file-related capabilities.

These rules are important when designing programs that drop root privileges but still rely on some capability behavior.

## 7) Programmatic capability changes

Linux provides `capget()` and `capset()`, but TLPI recommends using the `libcap` API instead.

Typical `libcap` workflow:

1. `cap_get_proc()` retrieves a copy of the current capability state;
2. `cap_set_flag()` updates the desired permitted, effective, or inheritable flags in that structure;
3. `cap_set_proc()` pushes the updated state back to the kernel;
4. `cap_free()` releases the user-space structure.

Capability changes are constrained by important rules:

- without `CAP_SETPCAP`, the new inheritable set is restricted;
- the new inheritable set must be limited by the bounding set;
- the new permitted set must be a subset of the current permitted set;
- the new effective set may contain only capabilities that are also in the new permitted set.

The central idea is that a process cannot grant itself new privilege that it does not already possess.

## 8) Shell tools for file capabilities

The `libcap` package provides:

- `setcap` to assign file capabilities;
- `getcap` to display file capabilities;
- `getpcap` to display process capabilities in a human-readable way.

These tools make it possible to replace some set-user-ID-root use cases with capability-based file privilege assignment.

## 9) Securebits and capabilities-only environments

Linux includes the securebits mechanism for disabling the traditional special treatment of root in selected ways.

Important flags include:

- `SECBIT_KEEP_CAPS`
- `SECBIT_NO_SETUID_FIXUP`
- `SECBIT_NOROOT`
- and the corresponding `_LOCKED` variants

These flags are manipulated via `prctl()` and are inherited across `fork()`. Most are preserved across `exec()`.

The securebits mechanism allows an application to create a more purely capability-based execution environment in which privilege is gained only through explicit capability handling and file capabilities, rather than through legacy root semantics.

## Execution Flow

### 1) Typical file-capability flow

1. identify the privileged operation a program needs;
2. determine the corresponding capability;
3. assign that capability to the executable file using `setcap`;
4. execute the program as a non-root user;
5. the kernel computes the new process capability sets during `exec()`.

### 2) Capability-aware program flow

For a program written with least privilege in mind:

1. start with a file capability in the file permitted set;
2. begin with effective capabilities disabled, if appropriate;
3. raise a capability in the effective set only immediately before a privileged operation;
4. drop that capability again immediately after the operation;
5. drop all remaining capabilities once no further privilege is required.

### 3) Bounding-set hardening flow

1. start with the current bounding set;
2. identify capabilities that should never be reacquired through `exec()`;
3. remove them using `prctl(PR_CAPBSET_DROP, ...)`;
4. keep the reduced set across descendant processes and future `exec()` calls.

### 4) Hardening a legacy privileged program

On systems where file capabilities are not available, a traditional approach is:

1. start in a root or set-user-ID-root context;
2. drop all unnecessary capabilities from the effective and permitted sets;
3. optionally use `PR_SET_KEEPCAPS` or securebits;
4. switch all user IDs to nonzero values;
5. raise and drop only the remaining required capabilities as needed.

## Example

## 1) Assign a file capability with `setcap`

```bash
cp /bin/date ./date
sudo setcap "cap_sys_time=pe" ./date
getcap ./date
```

This gives the copied executable the capability needed to change the system time without making it set-user-ID-root.

## 2) Raise one effective capability with `libcap`

```c
#include <sys/capability.h>

int raise_cap(int capability) {
    cap_t caps;
    cap_value_t capList[1];

    caps = cap_get_proc();
    if (caps == NULL)
        return -1;

    capList[0] = capability;
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, capList, CAP_SET) == -1) {
        cap_free(caps);
        return -1;
    }

    if (cap_set_proc(caps) == -1) {
        cap_free(caps);
        return -1;
    }

    if (cap_free(caps) == -1)
        return -1;

    return 0;
}
```

This pattern is appropriate for a capability-aware program that enables privilege only just before a privileged operation.

## 3) Drop all capabilities

```c
#include <sys/capability.h>

int drop_all_caps(void) {
    cap_t empty;
    int s;

    empty = cap_init();
    if (empty == NULL)
        return -1;

    s = cap_set_proc(empty);

    if (cap_free(empty) == -1)
        return -1;

    return s;
}
```

This is useful once a program has finished all privileged work and wants to permanently reduce its attack surface.

## Debugging

Useful debugging and inspection techniques:

- inspect `/proc/PID/status` for `CapInh`, `CapPrm`, `CapEff`, and `CapBnd`;
- use `getcap` to verify file capabilities;
- use `getpcap` to view process capabilities in readable form;
- use `strace` to detect failing privileged operations that return `EPERM`;
- review whether a program is capability-aware or capability-dumb before deciding how to set the file effective bit.

Common pitfalls:

- confusing `permitted` with `effective`;
- assuming a file capability always becomes immediately effective;
- dropping a capability from the permitted set and expecting to regain it later without `exec()`;
- forgetting that the bounding set can prevent a capability from being gained during `exec()`;
- overlooking the legacy effects of root and set-user-ID-root semantics.

## Real-world Usage

Linux capabilities are useful in many practical scenarios:

- network services that need only `CAP_NET_BIND_SERVICE`;
- administrative tools that require one specific privileged operation rather than full root;
- authentication or security programs that need temporary access to protected files;
- hardened services designed around least privilege;
- systems that want to reduce the number of set-user-ID-root executables.

Even when a program still starts from a root context, understanding capabilities allows it to drop privilege more precisely and safely.

## Key Takeaways

- Linux capabilities split traditional root privilege into distinct units.
- Process privilege is determined primarily by the effective capability set.
- The permitted set defines what may become effective; the inheritable set helps preserve selected privilege across `exec()`.
- File capabilities allow privilege to be granted by an executable file without making it set-user-ID-root.
- The capability bounding set limits what can be gained during `exec()`.
- Root compatibility behavior still matters, so capability-based design must account for legacy semantics.
- `libcap`, `setcap`, and `getcap` provide practical tools for building least-privilege programs.

