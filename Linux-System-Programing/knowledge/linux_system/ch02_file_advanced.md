# Chapter 2 — File Locking, Event Monitoring & Extended Attributes

> Covers: Topic 2.7 (File Locking), Topic 2.8 (Monitoring File Events), Topic 2.9 (Extended Attributes & ACL)
> TLPI: Ch55, Ch19, Ch16, Ch17 | DevLinux: Module 02

---

## Table of Contents

**Topic 2.7 — File Locking**
1. [Why File Locking?](#1-why-file-locking)
2. [Advisory vs Mandatory Locking](#2-advisory-vs-mandatory-locking)
3. [flock() — Whole-File Locking](#3-flock)
4. [flock() Semantics: Lock Tied to Open File Description](#4-flock-semantics)
5. [fcntl() Record Locking — Byte-Range Locking](#5-fcntl-record-locking)
6. [Deadlock Detection](#6-deadlock-detection)
7. [fcntl() Lock Semantics vs flock()](#7-fcntl-vs-flock-semantics)
8. [Real-World Pattern: PID File Lock](#8-pid-file-lock)

**Topic 2.8 — Monitoring File Events**
9. [Why inotify? The Problem with Polling](#9-why-inotify)
10. [inotify Architecture](#10-inotify-architecture)
11. [The Three inotify System Calls](#11-inotify-system-calls)
12. [inotify Event Types](#12-inotify-event-types)
13. [Reading Events: struct inotify_event](#13-reading-events)
14. [Key Limitations](#14-key-limitations)
15. [inotify vs Polling](#15-inotify-vs-polling)

**Topic 2.9 — Extended Attributes & ACL**
16. [Why Go Beyond 9-bit Permissions?](#16-why-beyond-9-bit)
17. [Extended Attributes (xattr)](#17-extended-attributes)
18. [xattr System Calls](#18-xattr-system-calls)
19. [Access Control Lists (ACL)](#19-access-control-lists)
20. [ACL Entry Types and ACL_MASK](#20-acl-entry-types-and-mask)
21. [ACL Permission-Checking Algorithm](#21-acl-permission-checking-algorithm)
22. [getfacl / setfacl Shell Commands](#22-getfacl-setfacl)
23. [Default ACL — Permission Inheritance](#23-default-acl)
24. [xattr vs ACL Relationship](#24-xattr-vs-acl)
25. [Summary](#25-summary)

---

# Topic 2.7 — File Locking

---

## 1. Why File Locking?

The classic read-modify-write race condition on files:

```
Process A: read()  → seq = 1000
Process A: [preempted]
Process B: read()  → seq = 1000  (stale!)
Process B: write() → seq = 1001
Process A: write() → seq = 1001  (WRONG — should be 1002)
```

File locking enforces mutual exclusion: only one process (or one writer) can access the critical section at a time.

---

## 2. Advisory vs Mandatory Locking

| Type | Behavior |
|------|----------|
| **Advisory** (default) | Lock only works if **all** cooperating processes respect it. Kernel does not prevent a process from ignoring locks. |
| **Mandatory** | Kernel automatically blocks incompatible I/O. Rarely used on Linux; requires special filesystem mount option + file permission configuration. |

**In practice:** Nearly all Linux file locking is advisory. Mandatory locking has too many edge cases and is not recommended on modern Linux.

---

## 3. flock()

```c
#include <sys/file.h>

int flock(int fd, int operation);
// Returns 0 on success, -1 on error
```

Places a lock on an **entire file**.

| Flag | Description |
|------|-------------|
| `LOCK_SH` | Shared lock (read) — multiple processes can hold simultaneously |
| `LOCK_EX` | Exclusive lock (write) — only one process at a time |
| `LOCK_UN` | Unlock |
| `LOCK_NB` | Non-blocking: fail immediately with `EWOULDBLOCK` instead of blocking |

### Lock compatibility

| A holds | B requests LOCK_SH | B requests LOCK_EX |
|---------|--------------------|--------------------|
| LOCK_SH | ✅ OK | ❌ Block |
| LOCK_EX | ❌ Block | ❌ Block |

### Basic usage pattern

```c
int fd = open("database.seq", O_RDWR);

flock(fd, LOCK_EX);          // block until exclusive lock acquired
// --- critical section ---
long seq = read_sequence(fd);
seq++;
write_sequence(fd, seq);
// --- end critical section ---
flock(fd, LOCK_UN);          // release

close(fd);
```

---

## 4. flock() Semantics: Lock Tied to Open File Description

Recall the 3-table kernel structure (Topic 2.1): FD Table → Open File Description (OFD) → inode.

`flock()` locks are associated with the **Open File Description**, not the FD or inode.

**Consequence 1 — dup/fork share the same lock:**

```c
flock(fd, LOCK_EX);
newfd = dup(fd);        // newfd → same OFD
flock(newfd, LOCK_UN);  // RELEASES lock — even though acquired via fd!

// Same happens with fork(): child inherits reference to parent's OFD
// child can inadvertently release parent's lock
```

**Consequence 2 — two open() calls = two independent locks:**

```c
fd1 = open("a.txt", O_RDWR);
fd2 = open("a.txt", O_RDWR);  // creates a NEW OFD

flock(fd1, LOCK_EX);
flock(fd2, LOCK_EX);  // BLOCKS — process locks itself out!
```

`fd1` and `fd2` have different OFDs, so the kernel treats them as if from different processes. A single process can deadlock itself using `flock()`.

### Limitations of flock()

1. Locks only the **entire file** — no byte-range granularity
2. Unreliable on NFS
3. Reduces concurrency when only parts of a file need protection

---

## 5. fcntl() Record Locking

`fcntl()` locks any **byte range** within a file — from one byte to the entire file.

### struct flock

```c
struct flock {
    short  l_type;    // F_RDLCK, F_WRLCK, F_UNLCK
    short  l_whence;  // SEEK_SET, SEEK_CUR, SEEK_END
    off_t  l_start;   // offset from l_whence
    off_t  l_len;     // bytes to lock; 0 = from l_start to EOF (and beyond)
    pid_t  l_pid;     // PID of conflicting lock holder (F_GETLK only)
};
```

### Three fcntl() commands for locking

| Command | Behavior |
|---------|----------|
| `F_SETLK` | Set/remove lock. If conflict → fail immediately with `EAGAIN` or `EACCES` |
| `F_SETLKW` | Same as F_SETLK but **blocks** if conflict (W = Wait) |
| `F_GETLK` | Test if lock can be placed — does NOT actually place it |

### Lock types

| l_type | flock() equivalent | Requirement |
|--------|-------------------|-------------|
| `F_RDLCK` | LOCK_SH | fd must be open for reading |
| `F_WRLCK` | LOCK_EX | fd must be open for writing |
| `F_UNLCK` | LOCK_UN | — |

### Basic pattern

```c
#include <fcntl.h>

int fd = open("records.db", O_RDWR);

// Lock bytes 0–99 with a write lock (blocking)
struct flock fl = {
    .l_type   = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start  = 0,
    .l_len    = 100
};
fcntl(fd, F_SETLKW, &fl);

// --- critical section ---
// ... read/modify bytes 0-99 ...

// Unlock
fl.l_type = F_UNLCK;
fcntl(fd, F_SETLK, &fl);
```

### Lock entire file

```c
struct flock fl = {
    .l_type   = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start  = 0,
    .l_len    = 0    // 0 = lock from start to EOF and all future bytes
};
```

### F_GETLK — diagnostic use only

```c
fcntl(fd, F_GETLK, &fl);
if (fl.l_type != F_UNLCK)
    printf("Locked by PID %d\n", fl.l_pid);
```

**Warning:** Never use `F_GETLK` to decide whether to proceed with `F_SETLK`. This is a TOCTOU race — the lock state may change between the two calls. Always call `F_SETLK` directly and handle errors.

The useful application of `F_GETLK`: obtaining the **PID of the conflicting process** for diagnostics or monitoring tools.

---

## 6. Deadlock Detection

When two processes each hold a lock the other needs:

```
Process A holds bytes 10–19, waiting for bytes 50–59 (held by B)
Process B holds bytes 50–59, waiting for bytes 10–19 (held by A)
→ Deadlock — both block forever without intervention
```

The kernel maintains a **wait-for graph** and detects cycles when a new `F_SETLKW` request is made. If a cycle is detected, the kernel returns `EDEADLK` to the most recently blocking process:

```c
if (fcntl(fd, F_SETLKW, &fl) == -1) {
    if (errno == EDEADLK) {
        // Kernel detected deadlock — must handle:
        // 1. Release all held locks
        // 2. Back off briefly
        // 3. Retry from scratch
    }
}
```

Deadlock detection works across circular chains involving 3+ processes and multiple files.

---

## 7. fcntl() Lock Semantics vs flock()

| Property | flock() | fcntl() |
|----------|---------|---------|
| Lock associated with | Open File Description | **Process + inode** |
| Inherited via fork() | ✅ Child inherits lock | ❌ Child does NOT inherit |
| Preserved via exec() | ✅ | ✅ |
| Threads in same process | Share (same OFD) | Share same lock set |
| Close **any** FD to same file | No effect if other FDs open | ⚠️ **Releases ALL process locks on that file** |
| Process can lock itself out | Yes (via two open() calls) | No |

### The critical fcntl() trap

```c
fd1 = open("testfile", O_RDWR);
fd2 = open("testfile", O_RDWR);

struct flock fl = { F_WRLCK, SEEK_SET, 0, 100 };
fcntl(fd1, F_SETLKW, &fl);   // acquire lock via fd1

close(fd2);   // LOCK IS RELEASED — even though acquired via fd1!
```

fcntl() locks are keyed by `(PID, inode)`. Closing **any** FD pointing to the same inode triggers removal of **all** locks held by this process on that file.

This is an acknowledged architectural flaw in POSIX record locking (TLPI refers to it as "an architectural blemish"), making `fcntl()` locking particularly error-prone inside library code.

---

## 8. PID File Lock

A common pattern for ensuring only one instance of a daemon runs:

```c
// Ensure single instance using flock() on a PID file
int fd = open("/var/run/myapp.pid", O_RDWR | O_CREAT, 0644);
if (fd < 0) { perror("open"); exit(1); }

if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    if (errno == EWOULDBLOCK) {
        fprintf(stderr, "Another instance is already running\n");
        exit(1);
    }
    perror("flock"); exit(1);
}

// Write current PID for diagnostic purposes
char buf[32];
ftruncate(fd, 0);
snprintf(buf, sizeof(buf), "%d\n", getpid());
write(fd, buf, strlen(buf));

// Keep fd open for the lifetime of the process
// Lock is automatically released when process exits — no stale lock problem
```

The lock is automatically released when the daemon exits (or crashes), so a new instance can start cleanly without manual cleanup.

---

# Topic 2.8 — Monitoring File Events

---

## 9. Why inotify? The Problem with Polling

Without inotify, detecting file changes requires **polling**:

```c
// Old way — poll every second
while (1) {
    stat("config.json", &sb);
    if (sb.st_mtime != last_mtime) {
        reload_config();
        last_mtime = sb.st_mtime;
    }
    sleep(1);   // wastes CPU, detects changes up to 1 second late
}
```

Problems:
- Wastes CPU even when nothing changes
- Latency vs CPU tradeoff — poll fast = more CPU, poll slow = late detection
- Does not scale: 10,000 files = 10,000 `stat()` calls per poll interval

**inotify solution:** The kernel proactively delivers notifications when events occur. The application simply waits (blocking `read()`), consuming zero CPU between events.

---

## 10. inotify Architecture

```
Kernel:
  File/Dir A —→ [watch item, wd=1] ─┐
  File/Dir B —→ [watch item, wd=2] ─┤
  File/Dir C —→ [watch item, wd=3] ─┘
                                     ↓
                          [inotify event queue]
                                     ↓
User Space:        read(inotify_fd)   ← blocks until events arrive
```

The inotify instance is a **file descriptor** — consistent with "everything is a file":
- `select()`/`poll()`/`epoll()` work on an inotify fd (useful when combining with other event sources in a single event loop — covered in Chapter 9)
- `close(inotify_fd)` automatically removes all watch items

---

## 11. The Three inotify System Calls

```c
#include <sys/inotify.h>

// 1. Create inotify instance → returns a file descriptor
int inotify_init(void);
int inotify_init1(int flags);   // flags: IN_CLOEXEC, IN_NONBLOCK

// 2. Add or modify a watch item — returns watch descriptor (wd)
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);

// 3. Remove a watch item
int inotify_rm_watch(int fd, uint32_t wd);
```

**Watch descriptor (wd):** Each `inotify_add_watch()` returns a unique positive integer. Events carry this `wd` to identify which watched path triggered them. The application must maintain a `wd → pathname` mapping.

---

## 12. inotify Event Types

**File content events:**

| Event | Triggered by |
|-------|-------------|
| `IN_ACCESS` | `read()` on file |
| `IN_MODIFY` | `write()` on file |
| `IN_OPEN` | File opened |
| `IN_CLOSE_WRITE` | File opened for write, then closed |
| `IN_CLOSE_NOWRITE` | File opened read-only, then closed |
| `IN_CLOSE` | Shorthand = `IN_CLOSE_WRITE \| IN_CLOSE_NOWRITE` |
| `IN_ATTRIB` | Metadata changed (chmod, chown, link count) |

**Directory and rename events:**

| Event | Triggered by |
|-------|-------------|
| `IN_CREATE` | File/dir created inside watched dir |
| `IN_DELETE` | File/dir deleted from inside watched dir |
| `IN_DELETE_SELF` | The watched file/dir itself was deleted |
| `IN_MOVED_FROM` | File moved out of watched dir |
| `IN_MOVED_TO` | File moved into watched dir |
| `IN_MOVE_SELF` | The watched file/dir itself was moved/renamed |
| `IN_MOVE` | Shorthand = `IN_MOVED_FROM \| IN_MOVED_TO` |

**Output-only flags (kernel sets these, do not use in mask):**

| Event | Meaning |
|-------|---------|
| `IN_IGNORED` | Watch was removed (explicit or file deleted) |
| `IN_ISDIR` | ORed into mask when event subject is a directory |
| `IN_Q_OVERFLOW` | Event queue overflowed — events were lost |
| `IN_UNMOUNT` | Filesystem containing watched object was unmounted |

**Convenience:** `IN_ALL_EVENTS` — watch for all possible events.

**Watch control flags (input only):**

| Flag | Effect |
|------|--------|
| `IN_ONESHOT` | Auto-remove watch after first event |
| `IN_ONLYDIR` | Fail with `ENOTDIR` if pathname is not a directory |

---

## 13. Reading Events: struct inotify_event

```c
struct inotify_event {
    int      wd;       // Watch descriptor — identifies which path triggered event
    uint32_t mask;     // Bitmask of event type(s)
    uint32_t cookie;   // Links related events (used for rename pairs)
    uint32_t len;      // Bytes allocated for name field (may include padding)
    char     name[];   // Null-terminated filename (only when event is inside watched dir)
};
```

### name field

- Event on the **watched file/dir itself** (e.g., watching `/etc/config.conf` directly): `len = 0`, `name` is empty.
- Event on a **file inside a watched directory**: `len > 0`, `name = "config.conf"`.

### cookie — linking rename pairs

When a file is renamed, the kernel generates two events:

```
IN_MOVED_FROM  wd=1  name="old.txt"  cookie=548
IN_MOVED_TO    wd=2  name="new.txt"  cookie=548
                                     ^^^^^^^^^
                                     Same cookie → these are a pair
```

If `IN_MOVED_FROM` has no matching `IN_MOVED_TO` → file was moved to an unwatched directory.
If `IN_MOVED_TO` has no matching `IN_MOVED_FROM` → file was moved in from an unwatched directory.

### Buffer parsing — the critical detail

Events in the read buffer are **variable-length** because `name` has padding bytes. Must advance by `sizeof(struct inotify_event) + ev->len`, not just `sizeof(struct inotify_event)`:

```c
#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

char buf[BUF_LEN];
ssize_t n = read(ifd, buf, BUF_LEN);

for (char *p = buf; p < buf + n; ) {
    struct inotify_event *ev = (struct inotify_event *) p;

    if (ev->mask & IN_MODIFY && ev->len > 0)
        printf("[MODIFIED] %s\n", ev->name);

    if (ev->mask & IN_CREATE)
        printf("[CREATED] %s%s\n", ev->name,
               (ev->mask & IN_ISDIR) ? "/" : "");

    if (ev->mask & IN_Q_OVERFLOW)
        fprintf(stderr, "WARNING: event queue overflow — events lost\n");

    p += sizeof(struct inotify_event) + ev->len;  // correct advancement
}
```

Buffer size should be **at least** `sizeof(struct inotify_event) + NAME_MAX + 1` to hold one event. A larger buffer retrieves multiple events per `read()` call.

---

## 14. Key Limitations

### inotify is NOT recursive

```c
inotify_add_watch(ifd, "/home/alice", IN_CREATE);
```

Only monitors files created **directly** inside `/home/alice/`. Files created in subdirectories do **not** trigger events.

To watch an entire directory tree, call `inotify_add_watch()` for every subdirectory. When a new subdirectory is created (`IN_CREATE | IN_ISDIR`), immediately add a watch for it.

### Kernel memory limits

Configurable via `/proc/sys/fs/inotify/`:

| File | Typical default | Meaning |
|------|----------------|---------|
| `max_queued_events` | 16,384 | Max events queued per inotify instance |
| `max_user_instances` | 128 | Max inotify instances per user |
| `max_user_watches` | 8,192 | Max watch items per user |

When the queue is full, `IN_Q_OVERFLOW` is generated and subsequent events are **silently discarded**. Production code must handle overflow by rescanning directories.

### Event coalescing

If the same file is written repeatedly in rapid succession, the kernel **merges multiple `IN_MODIFY` events into one** to save memory. inotify cannot reliably count how many times an event occurred — only that it occurred at least once.

---

## 15. inotify vs Polling

| Criterion | Polling stat() | inotify |
|-----------|---------------|---------|
| CPU when idle | Always consumed | Zero (blocking read) |
| Detection latency | Up to poll interval | Near-instant |
| 10,000 files | 10,000 stat()/interval | 1 read() per event batch |
| Recursive watch | Self-implemented | Self-implemented |
| POSIX standard | Yes | Linux-specific |
| NFS remote files | Reliable | Unreliable |

**Real-world users:** systemd, inotifywait, webpack/nodemon hot-reload, antivirus real-time scanning, desktop file managers.

---

# Topic 2.9 — Extended Attributes & ACL

---

## 16. Why Go Beyond 9-bit Permissions?

The traditional 9-bit `rwxrwxrwx` model has a hard constraint: only three categories — owner, group, other. This is often insufficient:

```
/srv/project/ owned by group "devops"

Alice (developer) → needs READ only
Bob   (developer) → needs READ + WRITE
Carol (ci-bot)    → needs WRITE only
Dave  (manager)   → needs READ only

→ Impossible to express with 3 categories only
```

Two mechanisms address this:
- **Extended Attributes (xattr):** Attach arbitrary metadata key-value pairs to inodes
- **ACL:** Fine-grained permissions per individual user or group

---

## 17. Extended Attributes (xattr)

### What is xattr?

A key-value pair stored **inside the inode** (or in an extra disk block associated with the inode). The key is a named string in the format `namespace.name`; the value is arbitrary binary data.

```
File: /srv/data/report.pdf  (inode 5042)

    inode standard fields:  size, uid, gid, timestamps, permissions...
    extended attributes:
        user.author          = "Alice"
        user.version         = "2.1"
        security.selinux     = "system_u:object_r:httpd_sys_content_t:s0"
        system.posix_acl_access = <binary ACL data>
```

### The four namespaces

| Namespace | Example name | Who can access | Purpose |
|-----------|-------------|----------------|---------|
| `user` | `user.author` | Unprivileged (need r/w permission on file) | Application-defined metadata |
| `trusted` | `trusted.overlay.opaque` | Root only (`CAP_SYS_ADMIN`) | Kernel/system tools |
| `system` | `system.posix_acl_access` | Kernel | ACLs, capabilities |
| `security` | `security.selinux` | SELinux / kernel | Security labels |

**Important restriction:** `user` namespace is only supported on **regular files and directories**. Not on symlinks, device files, sockets, or FIFOs — because their permission bits serve different purposes and cannot reliably guard xattr access.

### Limits

- Name length: max 255 characters
- Value size: max 64 KB
- Total xattr data per file on ext2/3/4: max one disk block (typically 4 KB)

---

## 18. xattr System Calls

All three groups follow the same `stat/lstat/fstat` pattern: pathname (follows symlinks), `l`-variant (does not follow), `f`-variant (via open FD).

```c
#include <sys/xattr.h>

// SET (create or replace)
int setxattr(const char *path, const char *name,
             const void *value, size_t size, int flags);
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags);

// GET
ssize_t getxattr(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size);
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size);

// LIST (returns null-separated name list)
ssize_t listxattr(const char *path, char *list, size_t size);

// REMOVE
int removexattr(const char *path, const char *name);
```

**flags for setxattr:**

| Flag | Behavior |
|------|----------|
| `0` | Create if absent, replace if exists |
| `XATTR_CREATE` | Fail with `EEXIST` if already exists |
| `XATTR_REPLACE` | Fail with `ENODATA` if does not exist |

### Reading with unknown size (two-step pattern)

```c
// Step 1: query the required buffer size
ssize_t len = getxattr("file.txt", "user.comment", NULL, 0);
if (len == -1) { perror("getxattr"); return; }

// Step 2: allocate and read
char *value = malloc(len + 1);
getxattr("file.txt", "user.comment", value, len);
value[len] = '\0';
printf("comment: %s\n", value);
free(value);
```

### Listing and iterating all attribute names

```c
// listxattr returns: "user.author\0user.version\0security.selinux\0"
ssize_t size = listxattr("file.txt", NULL, 0);
char *buf = malloc(size);
listxattr("file.txt", buf, size);

for (char *name = buf; name < buf + size; name += strlen(name) + 1)
    printf("attr: %s\n", name);

free(buf);
```

---

## 19. Access Control Lists (ACL)

### What is ACL?

An ACL is a **list of entries**, each granting specific permissions to an individual user or group — overcoming the three-category limit of traditional permissions.

```
File: /srv/project/data.txt

ACL:
┌──────────────┬──────────────┬─────────────┐
│  Entry type  │  Qualifier   │ Permissions │
├──────────────┼──────────────┼─────────────┤
│ ACL_USER_OBJ │      —       │   rwx       │  ← owner (always present)
│ ACL_USER     │  alice(1001) │   r--       │  ← Alice: read only
│ ACL_USER     │  bob  (1002) │   rw-       │  ← Bob: read + write
│ ACL_GROUP_OBJ│      —       │   r-x       │  ← file's owning group
│ ACL_GROUP    │ devops(200)  │   rwx       │  ← group devops: full access
│ ACL_MASK     │      —       │   rw-       │  ← upper limit for group class
│ ACL_OTHER    │      —       │   ---       │  ← everyone else
└──────────────┴──────────────┴─────────────┘
```

**Where is ACL stored?** As the `system.posix_acl_access` extended attribute — this is why xattr is the storage foundation for ACL.

### Minimal vs Extended ACL

| Type | Entries | Description |
|------|---------|-------------|
| **Minimal** | USER_OBJ, GROUP_OBJ, OTHER | Semantically equivalent to 9-bit permissions |
| **Extended** | + ACL_USER, ACL_GROUP, ACL_MASK | Adds per-user and per-group entries |

When a file has an extended ACL, `ls -l` appends `+`:
```
-rwxr-x--x+  1 alice devops  0 Mar 19 10:00 data.txt
```

---

## 20. ACL Entry Types and ACL_MASK

### Entry types

| Type | Qualifier | Meaning |
|------|-----------|---------|
| `ACL_USER_OBJ` | — | File owner's permissions |
| `ACL_USER` | UID | Permissions for a specific user |
| `ACL_GROUP_OBJ` | — | File group's permissions |
| `ACL_GROUP` | GID | Permissions for a specific group |
| `ACL_MASK` | — | Upper bound for the group class |
| `ACL_OTHER` | — | Everyone not matched above |

### ACL_MASK — the most important concept

When a file has `ACL_USER` or `ACL_GROUP` entries, an `ACL_MASK` entry is **mandatory**.

**Group class** = `ACL_USER` + `ACL_GROUP_OBJ` + `ACL_GROUP` entries.

`ACL_MASK` is the **upper permission limit for the entire group class**:

```
mask = rw-

group devops entry = rwx  → effective: rw-  (x blocked by mask)
user bob entry     = rw-  → effective: rw-  (unaffected)

ACL_USER_OBJ (owner) → NOT affected by mask
ACL_OTHER            → NOT affected by mask
```

**Why does ACL_MASK exist?** ACL-unaware tools like `chmod` do not understand individual ACL_USER/ACL_GROUP entries. When `chmod g=rx file` is called on a file with an extended ACL, Linux modifies the **ACL_MASK** (not ACL_GROUP_OBJ). This preserves the individual ACL entries while still honoring the intent of the chmod operation.

```bash
$ ls -l tfile
-rwxr-x--x+  ...          # group class bits reflect ACL_MASK, not ACL_GROUP_OBJ
```

---

## 21. ACL Permission-Checking Algorithm

Five steps, stop at the first match:

```
1. Effective UID == 0 (root)?
       → GRANT all access
       → Exception: execute requires at least one entry with 'x'

2. Effective UID == file owner (st_uid)?
       → Apply ACL_USER_OBJ permissions
       → STOP

3. Effective UID matches a ACL_USER entry?
       → Apply that entry's permissions, AND with ACL_MASK
       → STOP

4. Any GID (effective or supplementary) matches ACL_GROUP_OBJ or any ACL_GROUP entry?
       a. If ACL_GROUP_OBJ matches AND grants requested permissions → AND with ACL_MASK
       b. If an ACL_GROUP entry matches AND grants requested permissions → AND with ACL_MASK
       c. If matched but insufficient permissions → DENY
       → STOP

5. Apply ACL_OTHER permissions
       → GRANT or DENY
```

**Difference vs traditional permission check (Topic 2.5):**
Traditional uses "stop on first category match" — if you own the file, only owner bits are checked. ACL step 4 checks **all matching group entries** before denying.

---

## 22. getfacl / setfacl Shell Commands

```bash
# View ACL
$ getfacl data.txt
# file: data.txt
# owner: alice
# group: devops
user::rwx
user:bob:rw-
group::r-x
group:devops:rwx
mask::rwx
other::---

# Add/modify ACL entry
$ setfacl -m u:bob:rw data.txt
$ setfacl -m g:devops:rwx data.txt

# Remove a specific entry
$ setfacl -x u:bob data.txt

# Remove all extended entries (revert to minimal ACL)
$ setfacl -b data.txt

# Apply recursively to directory tree
$ setfacl -R -m u:bob:rx /srv/project/
```

---

## 23. Default ACL — Permission Inheritance

A **default ACL** set on a directory causes all files and subdirectories created inside it to **inherit** the ACL automatically.

```bash
# Set default ACL on /srv/project/
$ setfacl -d -m u::rwx,u:bob:rwx,g::r-x,g:devops:rwx,o::- /srv/project/

# View default ACL
$ getfacl -d /srv/project/
default:user::rwx
default:user:bob:rwx
default:group::r-x
default:group:devops:rwx
default:mask::rwx
default:other::---
```

Any **new file or subdirectory** created inside `/srv/project/` automatically receives this ACL as its access ACL.

**When default ACL is present:** the process `umask` is **not used** to set permissions on new files. Instead, ACL_USER_OBJ, ACL_MASK, and ACL_OTHER from the default ACL are ANDed with the `mode` argument of `open()`/`mkdir()`.

Default ACLs **propagate**: new subdirectories inherit the parent's default ACL as their own default ACL, cascading down the tree.

---

## 24. xattr vs ACL Relationship

| | xattr | ACL |
|--|-------|-----|
| Purpose | Store arbitrary metadata | Control file access permissions |
| Storage | In inode / extra disk block | As `system.posix_acl_access` xattr |
| Kernel enforcement | None — application reads manually | Kernel enforces during open/read/write |
| Who manages | Application code | Kernel permission subsystem |
| Dependency | Independent | **Built on top of xattr** |

**Key insight:** ACL is a specialization of xattr. The `system` namespace xattr `system.posix_acl_access` is written and read by the kernel's ACL subsystem automatically whenever you use `setfacl`/`getfacl` or call `acl_set_file()` library functions.

---

## 25. Summary

### Topic 2.7 — File Locking

- Race conditions on shared files require synchronization — file locking is preferred over semaphores because locks are **automatically associated with files**
- **All Linux file locking is advisory by default** — only works if all cooperating processes participate
- `flock()`: whole-file granularity; lock tied to Open File Description; `dup()`/`fork()` share the lock; two `open()` calls = two independent locks
- `fcntl()` record locking: byte-range granularity; POSIX standard; lock keyed by `(PID, inode)`; closing **any** FD to the same inode releases all locks — an architectural flaw
- `F_SETLKW` blocks until lock available; kernel detects deadlock cycles and returns `EDEADLK`
- `F_GETLK` use case: diagnostics to find blocking PID, not for conditional locking (TOCTOU)
- PID file pattern: `flock(LOCK_EX|LOCK_NB)` → fail fast if another instance running; lock auto-released on exit

### Topic 2.8 — Monitoring File Events

- inotify replaces polling: zero CPU when idle, near-instant notification, scales to thousands of files
- Core workflow: `inotify_init1()` → `inotify_add_watch()` → `read()` loop → `close()`
- Events carry `wd` (which path), `mask` (what happened), `cookie` (rename linking), `name` (filename if inside watched dir)
- Buffer parsing: advance by `sizeof(inotify_event) + ev->len` — variable-length entries
- `IN_MOVED_FROM` and `IN_MOVED_TO` share the same `cookie` to identify rename pairs; they may span multiple `read()` calls
- inotify is **not recursive** — must add watch per subdirectory
- `IN_Q_OVERFLOW` means events were lost — must trigger a full rescan
- inotify fd is a normal fd: works with `select()`/`poll()`/`epoll()` (covered in Chapter 9)

### Topic 2.9 — Extended Attributes & ACL

- xattr = key-value metadata stored on inodes; four namespaces: `user`, `trusted`, `system`, `security`
- `user` namespace requires read permission to get, write permission to set; only on regular files and directories
- ACL extends the traditional 3-category (owner/group/other) permission model to per-user and per-group entries
- ACL is stored as `system.posix_acl_access` xattr — ACL is built on top of xattr
- `ACL_MASK` is the upper permission limit for the group class; `chmod` on extended-ACL files modifies `ACL_MASK`, not `ACL_GROUP_OBJ`
- ACL permission check: root → owner → matching ACL_USER → matching group entries → ACL_OTHER
- Default ACL on directories: new files/subdirs inherit as their access ACL; umask is bypassed
- Extended ACL is visible as `+` suffix in `ls -l` output
