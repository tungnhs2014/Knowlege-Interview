# Chapter 2 — File Locking, File Events, and Extended Metadata

> Topics: 2.7 File Locking · 2.8 Monitoring File Events · 2.9 Extended Attributes & ACL
> Main sources: TLPI Ch55, Ch19, Ch16, Ch17 | DevLinux Module 02
> Production context: single-instance services, PID files, shared state files, log/config watchers, hot reload, desktop/file-sync tooling, security labels, ACL-based shared directories, and filesystem metadata debugging.

---

## Problem It Solves

After learning ordinary file I/O and filesystem basics, three practical problems remain:

- what if multiple processes touch the same file concurrently?
- what if an application needs to react when a file or directory changes?
- what if classic owner/group/other permission bits are too coarse?

Those are exactly the problems solved by:

- file locking;
- `inotify`;
- extended attributes and ACLs.

This chapter is where Linux file handling stops being only about bytes and starts becoming about
coordination, observability, and richer policy.

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | advisory locking, shared vs exclusive locks, `flock()`, `fcntl()` record locks, `inotify` instance/watch/event, xattr, ACL, `ACL_MASK` | Coordinate cooperating processes, watch files safely, and debug permission surprises. |
| Work useful | PID-file lock pattern, blocking vs nonblocking locks, lock release rules, `IN_Q_OVERFLOW` recovery, `getfattr/setfattr`, `getfacl/setfacl` | Build operationally reliable daemons, reloaders, file processors, and shared-directory workflows. |
| Recognize | mandatory locking, network filesystem lock quirks, lease-like behavior, `dnotify`, ACL C API, xattr namespaces beyond `user.*` | Identify less common mechanisms without letting them dominate the first pass. |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| advisory lock | Kernel records a lock, but ordinary I/O is blocked only if processes cooperate. | Default Linux file locking model. |
| mandatory lock | Kernel-enforced file locking mode. | Rare, Linux-specific setup details, usually avoided. |
| shared lock | Multiple readers can hold compatible locks. | `LOCK_SH` or `F_RDLCK`. |
| exclusive lock | One writer excludes other shared/exclusive locks. | `LOCK_EX` or `F_WRLCK`. |
| `flock()` | BSD-style whole-file lock API. | Simple for PID files and single-instance daemons. |
| `fcntl()` record lock | POSIX byte-range lock API using `struct flock`. | Use when different file regions can be protected independently. |
| open file description lock association | `flock()` locks follow the shared open file description. | `dup()` and `fork()` can share the same `flock()` lock. |
| process-associated lock | Traditional `fcntl()` locks are associated with a process and file. | Closing any FD for that file in the process can release its record locks. |
| `F_GETLK` | Diagnostic query for conflicting record locks. | Not a safe "check then lock" decision. |
| inotify instance | Kernel watch container exposed as a file descriptor. | Created by `inotify_init1()`. |
| watch descriptor | Small ID for one watched file/directory within an inotify instance. | Map it back to application path state. |
| event queue | Ordered queue read from the inotify FD. | Can overflow; events may be coalesced. |
| xattr | Extended attribute: name-value metadata attached to an inode. | `user.description`, `security.selinux`. |
| xattr namespace | Prefix controlling who may use the attribute and what it means. | `user`, `trusted`, `system`, `security`. |
| ACL | Access Control List extending owner/group/other permissions. | Per-user and per-group entries. |
| `ACL_MASK` | Maximum permission cap for ACL group-class entries. | Common reason `getfacl` shows less effective access than expected. |
| default ACL | Directory ACL template for newly created children. | Used in shared project directories. |

---

## Concept Overview

### Roadmap

```text
shared file access
   |
   +--> flock() for whole-file coordination
   +--> fcntl() for byte-range coordination

file or directory changes
   |
   +--> inotify instance
   +--> watch descriptors
   +--> event queue read through a file descriptor

metadata and permissions beyond 9 bits
   |
   +--> xattr namespaces
   +--> ACL entries and masks
   +--> default ACL inheritance on directories
```

### The Three Big Ideas

| Topic | Main question it answers |
|-------|--------------------------|
| **file locking** | how do cooperating processes avoid corrupting shared file state? |
| **inotify** | how does a process learn that a file or directory changed? |
| **xattr / ACL** | how do we store richer metadata and permissions than classic mode bits provide? |

### What This Chapter Must Make Clear

After this chapter, a learner should be able to explain:

- why `flock()` and `fcntl()` are not interchangeable;
- why advisory locking works only if participants cooperate;
- why `inotify` is better than polling, but not magic;
- why `IN_Q_OVERFLOW` means "rescan" instead of "pretend nothing happened";
- why ACL is a policy layer and xattr is a storage mechanism;
- why `ACL_MASK` is the most important ACL concept to get right.

---

## System Context

### Where These Topics Sit in Linux

```text
User processes
    |
    +--> flock() / fcntl()          -> file coordination
    +--> inotify_*() + read()       -> event notifications
    +--> setxattr() / getxattr()    -> metadata
    +--> ACL tools / APIs           -> fine-grained permissions
    |
    v
VFS and kernel file layer
    |
    +--> inode state
    +--> lock management
    +--> event queueing
    +--> xattr storage
    +--> permission checking
```

### Subsystem Interactions

- **File I/O core** matters because locks, event FDs, and xattr APIs all sit on top of the same
  descriptor and inode model.
- **Process model** matters because lock inheritance differs across `fork()` and `exec()`,
  especially between `flock()` and `fcntl()`.
- **Signals and event loops** matter because blocking lock operations can be interrupted, and
  `inotify` file descriptors integrate naturally with `select()`, `poll()`, and `epoll`.
- **Filesystem support** matters because xattr and ACL depend on filesystem capabilities and
  mount configuration.
- **Permission model** matters because ACL extends classic permission bits rather than replacing
  them with a completely unrelated scheme.

---

## Architecture

### Architecture 1: Locking

Two different locking families exist:

| API | Granularity | Association model |
|-----|-------------|-------------------|
| `flock()` | whole file | open file description |
| `fcntl()` record locking | byte range | process-associated lock on a file region |

That difference explains almost every strange behavior developers hit later.

### Architecture 2: `inotify`

```text
inotify instance
   |
   +--> watch descriptor 1 -> pathname / object A
   +--> watch descriptor 2 -> pathname / object B
   +--> watch descriptor 3 -> pathname / object C
   |
   v
event queue
   |
   v
read() from the inotify file descriptor
```

An `inotify` instance is itself referenced by a file descriptor, which is why it integrates so
well with normal event-driven code.

### Architecture 3: xattr and ACL

```text
inode
   |
   +--> classic metadata (owner, mode bits, timestamps, size)
   +--> extended attributes
   |      +--> user.*
   |      +--> trusted.*
   |      +--> security.*
   |      +--> system.*
   |
   +--> ACL data stored as system.posix_acl_* attributes
```

The important connection is:

> xattr is a general metadata mechanism; ACL uses that mechanism for permission-related data.

---

## Execution Flow

### Flow 1: Acquire a Lock, Update a Shared File, Release

```text
1. Process opens a shared file
2. Process acquires flock() or fcntl() lock
3. Kernel checks compatibility with existing locks
4. Process enters critical section and performs I/O
5. Process unlocks or closes the relevant descriptor
6. Kernel releases the lock according to that API's semantics
```

### Flow 2: Watch a Directory with `inotify`

```text
1. Process creates an inotify instance
2. Process adds a watch on a file or directory
3. Filesystem activity happens
4. Kernel appends events to the inotify queue
5. Process reads one or more inotify_event structures from the fd
6. Process dispatches application logic based on event type and watch descriptor
```

### Flow 3: Access Check with ACL

```text
1. Process requests file access
2. Kernel examines classic ownership and ACL information
3. Kernel applies ACL matching rules and ACL_MASK where needed
4. Kernel grants or denies the operation
```

---

## 2.7 File Locking

### Why File Locking Exists

The core problem is a shared-state race:

```text
Process A reads value 100
Process B reads value 100
Process A writes 101
Process B writes 101
```

One update is lost.

File locking is the coordination mechanism used when the shared state lives in a file.

### Advisory vs Mandatory Locking

By default, Linux locking is **advisory**.

That means:

- the kernel records the locks;
- cooperating processes can respect them;
- a process can still ignore the locking protocol and perform raw I/O anyway.

Mandatory locking exists on Linux, but it is rarely used, has awkward setup rules, and is best
avoided in normal application design.

The practical rule is:

> If you choose advisory locking, every participant must agree to use it correctly.

### `flock()`: Whole-File Locking

```c
#include <sys/file.h>

int flock(int fd, int operation);
```

`flock()` places a lock on the whole file.

Common operations:

| Flag | Meaning |
|------|---------|
| `LOCK_SH` | shared lock |
| `LOCK_EX` | exclusive lock |
| `LOCK_UN` | unlock |
| `LOCK_NB` | do not block |

### `flock()` Mental Model

`flock()` is associated with the **open file description**.

That means:

- `dup()` shares the same lock;
- `fork()` shares the same lock reference;
- a child can release a lock inherited from the parent;
- an independent second `open()` of the same file is treated separately.

This is why one process can lock itself out if it opens the same file twice and then tries to
take incompatible `flock()` locks through each open instance.

### `fcntl()` Record Locking

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ...);
```

For record locking, `fcntl()` uses `struct flock` and supports byte-range locks.

Useful commands:

| Command | Meaning |
|---------|---------|
| `F_SETLK` | set or clear a lock, fail immediately on conflict |
| `F_SETLKW` | same, but wait for the lock |
| `F_GETLK` | ask whether a conflicting lock exists |

Useful lock types:

| Type | Meaning |
|------|---------|
| `F_RDLCK` | shared/read lock |
| `F_WRLCK` | exclusive/write lock |
| `F_UNLCK` | unlock |

### `fcntl()` Mental Model

Classic POSIX record locks are process-associated and tied to a file's inode/region rather than
to a specific FD entry.

Important consequences:

- the child does **not** inherit these locks across `fork()`;
- the locks survive `exec()` unless relevant descriptors are closed by close-on-exec behavior;
- closing **any** descriptor that refers to that file can release the process's record locks on
  that file.

That last rule is one of the nastiest traps in POSIX file locking.

### `flock()` vs `fcntl()`

| Question | `flock()` | `fcntl()` |
|----------|-----------|-----------|
| granularity | whole file | byte range |
| association | open file description | process-associated record lock |
| inherited across `fork()` | yes, shared reference | no |
| child can release parent's lock | yes | not in the same way |
| closing unrelated FD to same file can break lock | no | yes, dangerous trap |
| good fit | single-instance daemons, simple whole-file coordination | record databases, partial-file coordination |

### One More Important Rule

On Linux, `flock()` locks and `fcntl()` locks are generally invisible to one another.

So do **not** assume one family will automatically respect the other.

### `F_GETLK` Is for Diagnostics, Not Lock Decisions

`F_GETLK` can tell you:

- whether a conflicting lock exists;
- which PID currently blocks your request.

But this is not safe:

```text
1. ask with F_GETLK
2. see "no lock"
3. assume it is safe
4. try to lock later
```

Another process may lock the file between steps 1 and 4.

The safe pattern is:

> try `F_SETLK` or `F_SETLKW` directly and handle the result.

### Deadlock Detection

Blocking `fcntl()` record locks can lead to deadlock if processes wait in a cycle.

Linux can detect some of these situations and return `EDEADLK`.

That does not remove the need for good design.
You should still:

- lock resources in a consistent order;
- keep critical sections small;
- avoid unnecessary nested locking.

### Practical Pattern: PID File Lock

A classic use case is preventing multiple daemon instances from running at once:

- open a PID file;
- take an exclusive nonblocking `flock()`;
- keep the descriptor open for the process lifetime.

If another instance fails to obtain the lock, it exits.

---

## 2.8 Monitoring File Events with `inotify`

### Why Polling Is Not Enough

Without `inotify`, a program often falls back to polling:

- call `stat()` every second;
- compare timestamps;
- burn CPU even when nothing changes;
- still miss quick event sequences or react too slowly.

`inotify` changes the model:

> instead of repeatedly asking "did something change?", the process waits for the kernel to
> tell it when something changed.

### Core API

```c
#include <sys/inotify.h>

int inotify_init(void);
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);
```

Modern Linux also provides `inotify_init1()` so you can request:

- close-on-exec;
- nonblocking behavior;

at creation time.

### The `inotify` Mental Model

An `inotify` instance contains:

- a watch list;
- an event queue.

Each watch has:

- a pathname/object;
- an event mask;
- a watch descriptor that identifies it inside the instance.

### Reading Events

Events are read from the `inotify` file descriptor using ordinary `read()`.

Each event is a variable-length `struct inotify_event`:

```c
struct inotify_event {
    int      wd;
    uint32_t mask;
    uint32_t cookie;
    uint32_t len;
    char     name[];
};
```

Important fields:

| Field | Meaning |
|-------|---------|
| `wd` | which watch generated the event |
| `mask` | what happened |
| `cookie` | ties related rename events together |
| `name` | affected filename within a watched directory, when applicable |

### Rename Cookies

When a file moves from one watched directory to another, Linux emits:

- `IN_MOVED_FROM`
- `IN_MOVED_TO`

The `cookie` field lets you correlate the pair.

This is one of the reasons `inotify` is much more precise than naive polling.

### Why Parsing the Read Buffer Carefully Matters

A single `read()` may return:

- one event;
- several events;
- variable-length names with padding.

So event processing must walk the buffer carefully rather than assuming one event per read.

### Important Limitations

#### Not Recursive

Watching a directory does **not** automatically watch all of its subdirectories.

If you want subtree monitoring, you must add watches for subdirectories too.

#### Queue Overflow

If the queue limit is exceeded, the kernel generates `IN_Q_OVERFLOW` and drops excess events.

The correct application response is usually:

> treat cached event knowledge as incomplete and rescan the relevant state.

#### Repeated Events Can Be Coalesced

The kernel may merge repeated similar events at the queue tail.

So `inotify` is good for learning **that something changed**, but not always for counting
exactly how many times it happened.

#### Some Filesystem Contexts Are Unfriendly

Monitoring behavior can be limited or unreliable on some networked or unusual filesystems.

`inotify` is strongest with local Linux filesystem objects.

### Operational Limits

Linux limits `inotify` resource usage through files under `/proc/sys/fs/inotify`, including:

- `max_queued_events`
- `max_user_instances`
- `max_user_watches`

When a system unexpectedly fails to watch enough paths, these limits are often the reason.

### Event Loops

Because an `inotify` instance is exposed as a file descriptor, it integrates naturally with:

- `select()`;
- `poll()`;
- `epoll()`.

That is the production-friendly way to combine file watching with sockets, timers, and signals.

---

## 2.9 Extended Attributes and ACL

### Why Classic 9-Bit Permissions Are Sometimes Not Enough

Classic mode bits are elegant, but limited.

They answer only:

- owner access;
- owning group access;
- everyone else.

Real systems often need more:

- metadata tags for applications;
- security labels;
- per-user and per-group exceptions;
- inherited directory-sharing policy.

That is where xattr and ACL enter.

### Extended Attributes: The General Mechanism

An extended attribute is a named key-value pair attached to a file object.

The naming format is:

```text
namespace.name
```

### The Four Standard Namespaces

| Namespace | Typical use | Access pattern |
|-----------|-------------|----------------|
| `user.*` | application-defined metadata | ordinary user-space use, subject to filesystem support and permissions |
| `trusted.*` | privileged metadata | typically privileged only |
| `security.*` | security modules and labels | security subsystem use |
| `system.*` | kernel-defined attributes | reserved kernel meanings such as ACL storage |

### xattr APIs

Typical APIs include:

```c
setxattr()   lsetxattr()   fsetxattr()
getxattr()   lgetxattr()   fgetxattr()
listxattr()  llistxattr()  flistxattr()
removexattr()
```

The usual naming pattern is the same as with other Linux file APIs:

- pathname form;
- non-dereferencing pathname form for symlinks;
- FD-based form.

### Two-Step Read Pattern

For unknown-size values, the standard pattern is:

1. call `getxattr(..., NULL, 0)` to learn the size;
2. allocate a buffer;
3. call `getxattr()` again to fetch the actual value.

The same idea applies to `listxattr()`.

### ACL: Fine-Grained Permissions

ACL extends the basic owner/group/other model by allowing named user and group entries.

Important ACL entry types:

| Entry type | Meaning |
|------------|---------|
| `ACL_USER_OBJ` | owner entry |
| `ACL_USER` | specific user |
| `ACL_GROUP_OBJ` | owning group entry |
| `ACL_GROUP` | specific group |
| `ACL_MASK` | upper bound for the group class |
| `ACL_OTHER` | everyone else |

### Minimal ACL vs Extended ACL

| Type | Meaning |
|------|---------|
| minimal ACL | equivalent to classic mode bits only |
| extended ACL | includes named user/group entries and therefore needs a mask |

In practice, the interesting behavior starts when the ACL becomes extended.

### `ACL_MASK`: The Most Important ACL Concept

When an ACL contains named user or group entries, `ACL_MASK` limits the effective permissions of
the whole group class.

The group class includes:

- `ACL_GROUP_OBJ`;
- named `ACL_GROUP` entries;
- named `ACL_USER` entries other than the owner.

The owner entry and `ACL_OTHER` are not controlled by the mask in the same way.

A short mental model:

> entry permissions say what is requested; `ACL_MASK` says the maximum that is actually allowed
> for the group class.

### Why `chmod()` Can Surprise You on ACL Files

On files with extended ACLs, changing the traditional group permission bits using `chmod()` can
modify the ACL mask rather than destroying the entire ACL structure.

That is why:

- `ls -l` group bits on such files may reflect the mask;
- effective permissions may differ from what an inexperienced reader expects.

### ACL Permission-Checking Mental Model

A useful simplified order is:

1. owner entry if caller is owner;
2. matching named user entry, if any;
3. matching group-class entries, filtered through `ACL_MASK`;
4. `ACL_OTHER` otherwise.

ACL extends the classic model; it does not replace the need to understand ordinary ownership and
mode bits first.

### Default ACL on Directories

A directory can carry a **default ACL**.

Its purpose is not to govern access to that directory directly.
Its purpose is to influence the ACLs and permissions of newly created children.

Important behavior:

- new subdirectories inherit the default ACL as their own default ACL;
- new files or subdirectories inherit an access ACL derived from that default ACL;
- the ACL entries corresponding to traditional bits are ANDed with the mode argument used during
  creation;
- when a directory has a default ACL, the process `umask` does not determine the resulting ACL
  entries for the new object's inherited access ACL.

### ACL and xattr Relationship

The important relationship is:

> ACL is a policy model; xattr is the storage mechanism underneath.

On Linux, access ACLs and default ACLs are stored as system extended attributes such as:

- `system.posix_acl_access`
- `system.posix_acl_default`

### Practical ACL Tools

For everyday operations, the most practical tools are often shell commands:

```bash
getfacl file
setfacl -m u:alice:rw file
setfacl -d -m g:dev:rwx shared_dir
```

These are often easier to reason about than raw ACL library calls during debugging.

---

## Work-Useful Patterns

| Pattern | Use it when | Production trap |
|---------|-------------|-----------------|
| PID-file single-instance lock | A daemon must prevent multiple active instances. | Lock the file, not just create it; stale PID files are normal after crashes. |
| Lock only the critical section | Updating shared file state. | Holding locks across slow I/O, sleeps, or network calls reduces concurrency and increases deadlock risk. |
| Pick one locking family per file | Multiple processes coordinate on the same path. | Do not mix `flock()` and `fcntl()` and expect portable interaction semantics. |
| Prefer nonblocking lock + clear error path for CLIs | User-facing tools should fail fast when resource is busy. | Blocking forever without diagnostics looks like a hang. |
| Treat `F_GETLK` as informational | Debugging or reporting lock owner. | Check-then-lock races; use `F_SETLK`/`F_SETLKW` as the actual decision point. |
| Rescan after `IN_Q_OVERFLOW` | File watcher queue overflows. | Cached state is incomplete; event-by-event recovery is unsafe. |
| Watch directories, not only files, for replace-style updates | Config reloaders and deploy tools. | Atomic replace often creates temp file then `rename()`; the old watched inode may disappear. |
| Store app metadata in `user.*` xattrs only when filesystem support is known | Local Linux files with side metadata. | Copy, backup, archive, or network filesystems may drop xattrs unless configured. |
| Debug ACL effective permission with `getfacl` | `ls -l` shows confusing group bits or `+`. | `ACL_MASK` may cap named user/group entries. |

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| mandatory locking | Rare Linux feature with awkward requirements; most production code uses advisory locking or a separate synchronization primitive. |
| network filesystem locking | NFS and distributed filesystems can differ by version/configuration; test lock semantics on the deployment filesystem. |
| lock inheritance differences | `flock()` and `fcntl()` have different `fork()`, `dup()`, `exec()`, and close behavior; know the family you chose. |
| `dnotify` | Older Linux file notification mechanism; superseded by `inotify`. |
| `fanotify` | Broader notification/access-control mechanism; useful for security scanners, not normal app file watching. |
| ACL C API | Powerful but verbose; most ops/debug work is easier with `getfacl`/`setfacl`. |
| `trusted.*` xattrs | Require privilege; used for admin/system metadata. |
| `security.*` xattrs | Used by Linux security modules and file capabilities; do not treat them as application scratch space. |

---

## Example

### Example 1 — Single-instance lock with `flock()`

```c
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

int main(void) {
    int fd = open("app.pid", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK)
            fprintf(stderr, "another instance is already running\n");
        else
            perror("flock");
        close(fd);
        return 1;
    }

    puts("lock acquired; keep fd open for process lifetime");
    pause();
    return 0;
}
```

What it teaches:

- `flock()` is simple and effective for whole-file coordination;
- the lock remains meaningful only while the descriptor stays open.

### Example 2 — Byte-range lock with `fcntl()`

```c
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd = open("records.db", O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 100
    };

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl lock");
        close(fd);
        return 1;
    }

    puts("locked bytes 0..99");

    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl unlock");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
```

What it teaches:

- `fcntl()` can coordinate access to only part of a file;
- blocking and unlocking are explicit and structured.

### Example 3 — Minimal `inotify` watch loop

```c
#define _GNU_SOURCE

#include <limits.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>

int main(void) {
    char buf[8 * (sizeof(struct inotify_event) + NAME_MAX + 1)];

    int ifd = inotify_init();
    if (ifd == -1) {
        perror("inotify_init");
        return 1;
    }

    if (inotify_add_watch(ifd, ".", IN_CREATE | IN_DELETE | IN_MODIFY) == -1) {
        perror("inotify_add_watch");
        close(ifd);
        return 1;
    }

    for (;;) {
        ssize_t n = read(ifd, buf, sizeof(buf));
        if (n == -1) {
            perror("read");
            break;
        }

        for (char *p = buf; p < buf + n; ) {
            struct inotify_event *ev = (struct inotify_event *)p;
            printf("mask=%u name=%s\n", ev->mask, ev->len ? ev->name : "(none)");
            p += sizeof(struct inotify_event) + ev->len;
        }
    }

    close(ifd);
    return 0;
}
```

What it teaches:

- `inotify` is consumed through an ordinary file descriptor;
- a watch loop is fundamentally event-driven rather than poll-driven;
- even a minimal example reveals masks and optional names.

### Example 4 — Read an xattr with the two-step pattern

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/xattr.h>

int main(void) {
    ssize_t len = getxattr("file.txt", "user.note", NULL, 0);
    if (len == -1) {
        perror("getxattr size");
        return 1;
    }

    char *buf = malloc((size_t)len + 1);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    len = getxattr("file.txt", "user.note", buf, (size_t)len);
    if (len == -1) {
        perror("getxattr value");
        free(buf);
        return 1;
    }

    buf[len] = '\0';
    printf("user.note = %s\n", buf);
    free(buf);
    return 0;
}
```

What it teaches:

- xattr values are variable-sized metadata;
- size-probe then allocate is the standard pattern.

---

## Debugging

Useful commands:

```bash
# Inspect active file locks
lslocks
cat /proc/locks

# Trace lock-related syscalls
strace -e flock,fcntl ./program

# Inspect inotify limits
cat /proc/sys/fs/inotify/max_queued_events
cat /proc/sys/fs/inotify/max_user_instances
cat /proc/sys/fs/inotify/max_user_watches

# Work with extended attributes
getfattr -d file.txt
setfattr -n user.note -v "hello" file.txt

# Work with ACL
getfacl file.txt
setfacl -m u:alice:rw file.txt

# Check filesystem and mount context
mount | grep /path
```

Common pitfalls:

- assuming advisory locking protects you from uncooperative code;
- forgetting that `fcntl()` and `flock()` follow different inheritance and release rules;
- using `F_GETLK` as if it were a safe pre-check for locking;
- forgetting that `inotify` is not recursive;
- ignoring `IN_Q_OVERFLOW` and continuing as if no events were lost;
- assuming `chmod` on an ACL file only changes the visible 9-bit group field;
- forgetting that xattr and ACL support depend on filesystem capabilities;
- expecting `cp` without preservation options to retain rich metadata automatically.

---

## Real-world Usage

### Where This Knowledge Shows Up

- single-instance daemons and supervisors;
- record-oriented files shared by multiple worker processes;
- hot-reload configuration watchers;
- IDEs, sync tools, and backup tools that react to filesystem changes;
- media or application metadata stored in `user.*` xattrs;
- shared directories that need per-user or per-group exceptions without changing ownership.

### Practical Patterns

| Scenario | Practical design |
|----------|------------------|
| prevent two daemon instances | PID file plus nonblocking `flock()` |
| lock one record range, not the whole file | `fcntl()` byte-range lock |
| react to file changes without polling loops | `inotify` |
| store app metadata on a file | `user.*` xattr |
| give one user extra access without changing owner or group | ACL entry |
| make new files in a shared tree inherit richer permissions | default ACL on the directory |

---

## Coverage Notes

| Coverage item | Status | Notes |
|---------------|--------|-------|
| 2.7 File locking | Covered | advisory model, `flock`, `fcntl` record locks, inheritance/release traps, lock debugging. |
| 2.8 Monitoring file events | Covered | `inotify` instance/watch/event queue, rename handling, overflow, nonrecursive limits. |
| 2.9 Extended metadata | Covered | xattr namespaces/APIs, ACL model, `ACL_MASK`, default ACLs, preservation pitfalls. |
| Must-cover production debug | Covered | `lslocks`, `/proc/locks`, `strace`, inotify limits, `getfattr`, `getfacl`, mount context. |
| Embedded relevance | Covered | lock/watcher/metadata behavior remains filesystem-dependent; preserve rescan and fallback logic. |

No remaining advanced-file coverage gap is known.

---

## Interview-Relevant Questions

- What is the difference between advisory and mandatory locking?
- Why are `flock()` and `fcntl()` not interchangeable?
- Why can closing one descriptor unexpectedly release a `fcntl()` lock?
- Why is `F_GETLK` useful for diagnostics but not for lock decisions?
- Why is `inotify` better than polling?
- Why is `inotify` not sufficient by itself for full recursive subtree monitoring?
- What does `IN_Q_OVERFLOW` mean, and how should an application react?
- What is the difference between xattr and ACL?
- What problem does `ACL_MASK` solve?
- How does a default ACL affect newly created files inside a directory?
- When would you choose `flock()` over `fcntl()` record locking?
- Why can mixing stdio buffering with file locks create incorrect behavior?
- How should a config watcher handle atomic replace via temporary file plus `rename()`?
- Why might xattrs disappear or fail when files are copied or moved across filesystems?
- How would you debug an ACL case where `ls -l` and effective access disagree?

---

## Key Takeaways

- File locking is about coordination, not magical enforcement, unless you enter the rarely used
  world of mandatory locking.
- `flock()` is simple whole-file locking tied to the open file description.
- `fcntl()` record locking is more flexible but has more dangerous semantics.
- `inotify` gives event-driven visibility into file and directory changes through an ordinary
  file descriptor.
- `inotify` is not recursive and can overflow, so production code must plan for rescan logic.
- xattr stores arbitrary metadata in named namespaces.
- ACL extends classic permissions with per-user and per-group entries.
- `ACL_MASK` limits the effective permissions of the group class and explains many ACL surprises.
- Default ACLs on directories shape inherited permissions for newly created children.
- These mechanisms matter in real systems because correctness often depends on coordination,
  observability, and policy, not just raw byte I/O.
