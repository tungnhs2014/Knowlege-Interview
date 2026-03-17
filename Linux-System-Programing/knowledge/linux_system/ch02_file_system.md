# Chapter 2 — File Systems, Attributes & Directory Operations

> Covers: Topic 2.4 (File Systems & Inodes), Topic 2.5 (File Attributes & Permissions), Topic 2.6 (Directories & Links)
> TLPI: Ch14, Ch15, Ch18 | DevLinux: Module 02

---

## Table of Contents

**Topic 2.4 — File Systems & Inodes**
1. [From Disk to Filesystem — The Physical Layout](#1-from-disk-to-filesystem)
2. [Inode — The True Identity of a File](#2-inode)
3. [On-Disk vs In-Memory Inode](#3-on-disk-vs-in-memory-inode)
4. [Block Pointers and ext4 Extents](#4-block-pointers-and-ext4-extents)
5. [VFS — The Abstraction Layer](#5-vfs)
6. [Journaling — Crash Safety Mechanism](#6-journaling)
7. [Mounting and Unmounting Filesystems](#7-mounting-and-unmounting)
8. [Pathname Lookup — Step-by-Step Mechanics](#8-pathname-lookup)

**Topic 2.5 — File Attributes & Permissions**
9. [stat() — Reading File Metadata](#9-stat)
10. [st_mode — The Permission Bit Layout](#10-st_mode)
11. [Permission-Checking Algorithm](#11-permission-checking-algorithm)
12. [Special Bits: SUID, SGID, Sticky](#12-special-bits)
13. [umask — Default Permission Filter](#13-umask)
14. [chmod() and chown() — Changing Attributes](#14-chmod-and-chown)
15. [File Timestamps](#15-file-timestamps)
16. [truncate() and ftruncate()](#16-truncate)

**Topic 2.6 — Directories & Links**
17. [Directory Internals — Not a Folder](#17-directory-internals)
18. [Hard Links — Multiple Names, One Inode](#18-hard-links)
19. [Symbolic Links — Pointers to Paths](#19-symbolic-links)
20. [Hard Link vs Symbolic Link — Comparison](#20-hard-vs-symbolic-link)
21. [rename() — Atomic File Operations](#21-rename)
22. [Directory Operations: mkdir, rmdir, remove](#22-directory-operations)
23. [Directory Traversal: opendir/readdir](#23-directory-traversal)
24. [chroot() — Changing the Root](#24-chroot)
25. [realpath() — Canonical Path Resolution](#25-realpath)
26. [Mental Model: What Delete/Rename/Move Really Means](#26-mental-model)

---

# Topic 2.4 — File Systems & Inodes

---

## 1. From Disk to Filesystem

### Physical Stack

A storage device goes through several layers before the kernel can store files on it:

```
Physical Disk
    └── Partition Table (MBR or GPT)
            ├── Partition 1  →  Formatted with ext4
            ├── Partition 2  →  Formatted with xfs
            └── Partition 3  →  swap
```

### Filesystem On-Disk Layout (ext4 example)

Each partition formatted with a filesystem has a fixed on-disk structure:

```
[ Boot Block | Superblock | Group Descriptor Table | ... | Block Groups ]

Each Block Group:
┌──────────────────────────────────────────────────────────────┐
│ Group Descriptor │ Block Bitmap │ Inode Bitmap │ Inode Table │ Data Blocks │
└──────────────────────────────────────────────────────────────┘
```

| Region | Purpose |
|--------|---------|
| **Superblock** | Records total inodes, free inodes, total blocks, block size, fs type, mount count |
| **Group Descriptor** | Per-group metadata: location of bitmaps and inode table |
| **Block Bitmap** | 1 bit per data block — 0 = free, 1 = in use |
| **Inode Bitmap** | 1 bit per inode slot — tracks which inodes are allocated |
| **Inode Table** | Fixed-size array of inode structures |
| **Data Blocks** | Actual file content, directory entries |

> **Block size** is set at mkfs time. Common: 4096 bytes. Larger blocks = less seeks, more internal fragmentation.

---

## 2. Inode

### What Is an Inode?

An inode (index node) is a fixed-size data structure that stores **all metadata about a file except its name**.

Every file and directory has exactly one inode. The inode is identified by an **inode number** (unique within a filesystem).

### On-Disk Inode Contents

```
struct ext4_inode {
    __le16  i_mode;         // File type + permissions (rwxrwxrwx + SUID/SGID/sticky)
    __le16  i_uid;          // Owner UID (lower 16 bits)
    __le32  i_size_lo;      // File size in bytes (lower 32 bits)
    __le32  i_atime;        // Last access time
    __le32  i_ctime;        // Last inode change time (not creation!)
    __le32  i_mtime;        // Last data modification time
    __le32  i_dtime;        // Deletion time
    __le16  i_gid;          // Owner GID (lower 16 bits)
    __le16  i_links_count;  // Hard link count (nlink)
    __le32  i_blocks_lo;    // Number of 512-byte blocks allocated
    __le32  i_flags;        // ext4 flags (e.g., EXT4_EXTENTS_FL)
    // ...
    __le32  i_block[15];    // Block pointers (or extent tree root)
    // ...
};
```

**Key insight:** A filename is **not** stored in the inode. The filename lives in the directory entry. This is the fundamental reason hard links are possible.

---

## 3. On-Disk vs In-Memory Inode

When a file is opened, the kernel reads the on-disk inode into memory and augments it with runtime fields:

| Field | On-Disk Inode | In-Memory `struct inode` |
|-------|--------------|--------------------------|
| File type & permissions | ✅ | ✅ (copied) |
| UID/GID | ✅ | ✅ (copied) |
| Timestamps (atime/mtime/ctime) | ✅ | ✅ (may be delayed flush) |
| Block pointers / extents | ✅ | ✅ (copied) |
| Open file descriptor count | ❌ | ✅ `i_count` (reference count) |
| Dirty flag (needs write-back) | ❌ | ✅ `I_DIRTY` |
| File locks | ❌ | ✅ `i_flock` list |
| Pending I/O operations | ❌ | ✅ page cache pointers |
| Filesystem-specific data | ❌ | ✅ embedded in `i_private` |

**Lifecycle:**
1. `open()` → kernel checks inode cache (icache) → if miss, reads from disk
2. Reference count (`i_count`) incremented per open FD
3. On last `close()`, `i_count` drops to 0 → inode evicted from cache (if not dirty)
4. Dirty inodes are written back by `pdflush` / `writeback` threads periodically

---

## 4. Block Pointers and ext4 Extents

### ext2/ext3 Block Pointer Scheme (15 slots in i_block[])

```
i_block[0..11]   → Direct pointers (12 × 4096 = 48 KB directly)
i_block[12]      → Single-indirect pointer → block of 1024 pointers → 4 MB
i_block[13]      → Double-indirect → 1024 × 1024 pointers → 4 GB
i_block[14]      → Triple-indirect → 1024³ pointers → theoretical max
```

**Problem:** Large files require multiple disk reads to resolve the pointer chain. A 1 GB sequential file needs a double-indirect read to find every block.

### ext4 Extent Tree

ext4 replaced the pointer scheme with **extents** — contiguous block ranges:

```c
struct ext4_extent {
    __le32  ee_block;    // first logical block in the extent
    __le16  ee_len;      // number of blocks in the extent
    __le16  ee_start_hi; // high 16 bits of physical start block
    __le32  ee_start_lo; // low 32 bits of physical start block
};
```

**Mechanism:**
- The 60-byte `i_block[]` area in the inode now holds the **root of an extent tree**
- Root can hold up to **4 extents inline** (no extra disk reads for small/medium files)
- For larger files, tree grows: root → internal node → leaf nodes storing extents

**Advantage:** A 1 GB file stored contiguously = **1 extent** = 1 disk read to find all blocks. This is why ext4 is dramatically faster than ext2/ext3 for large files.

---

## 5. VFS

### What Problem Does VFS Solve?

Linux supports many filesystem types: ext4, xfs, btrfs, tmpfs, FAT32, NFS, /proc, /sys. Without an abstraction layer, every program would need filesystem-specific code.

**VFS (Virtual File System)** is the kernel layer that presents a **uniform interface** to all filesystems.

### VFS Architecture

```
User Space:   open()  read()  write()  stat()  ...
                   ↓
Kernel:       VFS Layer
              ├── inode operations  (create, lookup, link, unlink, rename, ...)
              ├── file operations   (open, read, write, llseek, mmap, ...)
              └── dentry operations (hash, compare, revalidate, ...)
                   ↓
         ┌─────────┼──────────┐
       ext4       xfs      tmpfs  ...    ← Concrete filesystem drivers
```

### Key VFS Objects

| Object | Kernel Structure | Purpose |
|--------|-----------------|---------|
| Superblock | `struct super_block` | Represents a mounted filesystem |
| Inode | `struct inode` | Represents a file or directory |
| Dentry | `struct dentry` | Maps a pathname component to an inode |
| File | `struct file` | Represents an open file (per open FD) |

**Dentry cache (dcache):** The kernel caches `(name → inode)` mappings. Repeated lookups of `/usr/lib/libm.so.6` reuse cached dentries and never touch disk.

---

## 6. Journaling

### Why Journaling?

Without journaling, a crash mid-write can leave the filesystem in an inconsistent state:
- Inode updated but data blocks not written
- Directory entry added but inode not initialized

Recovery requires a full `fsck` scan — slow on large filesystems.

### How Journaling Works

A journal (also called a write-ahead log) is a dedicated area on disk where the filesystem **logs changes before applying them**:

```
Transaction flow:
1. Journal commit: write metadata (and optionally data) changes to journal
2. Journal checkpoint: apply changes to actual filesystem locations
3. If crash after step 1: replay journal on next mount → consistent state
4. If crash before step 1: transaction is incomplete → discarded → consistent state
```

### Journal Modes in ext4

| Mode | What is journaled | Performance | Safety |
|------|------------------|-------------|--------|
| `journal` | Data + metadata | Slowest | Highest |
| `ordered` (default) | Metadata only, data flushed first | Good | Good |
| `writeback` | Metadata only, data timing not guaranteed | Fastest | Lower |

**Mental model:** The journal is a "undo/redo log" for filesystem metadata. The database world calls this WAL (Write-Ahead Logging).

---

## 7. Mounting and Unmounting Filesystems

### The mount() System Call

```c
#include <sys/mount.h>

int mount(const char *source,   // device path, e.g. "/dev/sdb1"
          const char *target,   // mount point directory
          const char *fstype,   // "ext4", "xfs", "tmpfs", ...
          unsigned long flags,  // MS_RDONLY, MS_NOEXEC, MS_BIND, ...
          const void *data);    // filesystem-specific options
```

### Bind Mounts

```c
mount("/srv/data", "/mnt/data", NULL, MS_BIND, NULL);
```

A bind mount makes a **directory subtree visible at another path** — no filesystem copy happens:
- `/srv/data` and `/mnt/data` point to the **same inode tree**
- Changes at one mount point are instantly visible at the other
- Foundation of Docker container filesystem isolation

### The Mount Table

The kernel maintains a mount table (`/proc/self/mountinfo`). Every mount point is a `struct mount` containing:
- Parent mount (forms a tree)
- Mounted filesystem superblock
- Mount flags (read-only, nosuid, etc.)

```
# /proc/self/mountinfo example
36 35 98:0 /mnt1 /mnt2 rw,nosuid ...
```

### umount()

```c
int umount(const char *target);
int umount2(const char *target, int flags);  // MNT_DETACH, MNT_FORCE
```

`umount()` fails with `EBUSY` if any process has an open file or its current directory inside the mounted filesystem. `MNT_DETACH` performs a lazy unmount: detaches immediately, cleans up when all FDs are closed.

---

## 8. Pathname Lookup

### Step-by-Step: How the Kernel Resolves `/usr/lib/libm.so`

```
1. Start at root inode (inode 2, always) — or current dir for relative paths

2. Lookup "usr" in root directory:
   - Scan root dir entries for name "usr"
   - Found: inode 12345
   - Load inode 12345 → check: is directory? yes → continue

3. Lookup "lib" in inode 12345 (the /usr directory):
   - Scan entries for "lib"
   - Found: inode 23456

4. Lookup "libm.so" in inode 23456 (the /usr/lib directory):
   - Scan entries for "libm.so"
   - Found: inode 34567 → this is the file inode

5. Return inode 34567 → open(), stat(), etc. operate on this inode
```

**Optimization — Dentry Cache (dcache):**
- Each `(parent_inode, name)` → `child_inode` mapping is cached in a dentry
- Repeated lookups of `/usr/lib/libm.so` hit the dcache and never touch disk
- Cache eviction uses LRU when memory is needed

**Symlink handling:**
- When a pathname component resolves to a symlink, the kernel **substitutes** the symlink's target path and restarts that component's resolution
- Kernel tracks nesting depth; aborts at 40 levels to prevent infinite loops

---

# Topic 2.5 — File Attributes & Permissions

---

## 9. stat()

### The Three Variants

```c
#include <sys/stat.h>

int stat(const char *pathname, struct stat *statbuf);   // follows symlinks
int lstat(const char *pathname, struct stat *statbuf);  // does NOT follow symlinks
int fstat(int fd, struct stat *statbuf);                // uses open FD
```

**When to use which:**
- `stat()`: normal metadata about a file (follows symlink to the target)
- `lstat()`: metadata about the symlink itself (use to detect symlinks)
- `fstat()`: when you already have an open FD (no pathname re-lookup, immune to TOCTOU race)

### struct stat Fields

```c
struct stat {
    dev_t     st_dev;     // Device ID of filesystem containing the file
    ino_t     st_ino;     // Inode number
    mode_t    st_mode;    // File type + permissions (see Section 10)
    nlink_t   st_nlink;   // Hard link count
    uid_t     st_uid;     // Owner user ID
    gid_t     st_gid;     // Owner group ID
    dev_t     st_rdev;    // Device ID (for device special files)
    off_t     st_size;    // File size in bytes
    blksize_t st_blksize; // Preferred I/O block size
    blkcnt_t  st_blocks;  // Number of 512-byte blocks allocated
    time_t    st_atime;   // Last access time
    time_t    st_mtime;   // Last data modification time
    time_t    st_ctime;   // Last status change time (inode change)
};
```

**`st_size` vs `st_blocks`:**
- `st_size`: logical size (what `ls -l` shows), includes holes in sparse files
- `st_blocks`: physical storage used (512-byte units — historical, unrelated to `st_blksize`)
- Sparse file: `st_size` >> `st_blocks * 512` — the hole blocks are not actually allocated

**File type macros** (operate on `st_mode`):

```c
S_ISREG(m)   // regular file
S_ISDIR(m)   // directory
S_ISLNK(m)   // symbolic link
S_ISCHR(m)   // character device
S_ISBLK(m)   // block device
S_ISFIFO(m)  // FIFO / named pipe
S_ISSOCK(m)  // socket

// Usage:
struct stat sb;
stat("/dev/sda", &sb);
if (S_ISBLK(sb.st_mode))
    printf("block device\n");
```

---

## 10. st_mode — The Permission Bit Layout

`st_mode` is a 16-bit value encoding both the file type and permissions:

```
Bit positions (octal notation):

  15 14 13 12  |  11 10  9  |  8  7  6  |  5  4  3  |  2  1  0
  ─────────────────────────────────────────────────────────────
  File type    |  S S  S   |  r  w  x  |  r  w  x  |  r  w  x
  (4 bits)     | UID GID T |   owner   |   group   |   other
                            └──────────────────────────────────
                                      9 permission bits
```

| Constant | Octal | Description |
|----------|-------|-------------|
| `S_IRUSR` | `0400` | Owner read |
| `S_IWUSR` | `0200` | Owner write |
| `S_IXUSR` | `0100` | Owner execute |
| `S_IRGRP` | `0040` | Group read |
| `S_IWGRP` | `0020` | Group write |
| `S_IXGRP` | `0010` | Group execute |
| `S_IROTH` | `0004` | Other read |
| `S_IWOTH` | `0002` | Other write |
| `S_IXOTH` | `0001` | Other execute |
| `S_ISUID` | `04000` | Set-user-ID bit |
| `S_ISGID` | `02000` | Set-group-ID bit |
| `S_ISVTX` | `01000` | Sticky bit |

**Example:** `rw-r--r--` = `0644` = `S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH`

---

## 11. Permission-Checking Algorithm

The kernel checks permissions using the **real UID/GID** for `access()` and the **effective UID/GID** for all actual operations.

### The 4-Step Algorithm (Stop on First Match)

```
1. If effective UID == 0 (root):
       GRANT immediately (root bypasses all permission bits)
       Exception: execute requires at least one 'x' bit to be set

2. If effective UID == st_uid (process is the file owner):
       Apply the OWNER bits (rwx in bits 8-6)
       → GRANT or DENY based on owner bits only
       → STOP — do not check group or other bits

3. If effective GID (or any supplementary GID) == st_gid:
       Apply the GROUP bits (rwx in bits 5-3)
       → GRANT or DENY based on group bits only
       → STOP — do not check other bits

4. Apply the OTHER bits (rwx in bits 2-0)
       → GRANT or DENY
```

**Critical insight — "stop on first match":**
If you are the file owner but the owner bits deny access, the kernel does **not** fall through to check group bits. A file with permissions `---rwxrwx` owned by you means **you cannot access it**, even if group/other allow it.

### Directory Permissions Semantics

| Permission | Meaning on a Directory |
|------------|----------------------|
| `r` | Can list directory contents (`ls`) |
| `w` | Can create/delete files in directory (not the files themselves) |
| `x` | Can access (traverse) the directory — required to open any file by path through it |

> You need `x` on every directory in a pathname to access the file at the end. Having `r` on a directory without `x` lets you list names but not access or stat any file.

---

## 12. Special Bits: SUID, SGID, Sticky

### Set-User-ID (SUID) bit — `04000`

When an executable file has the SUID bit set, the process that runs it gets the **file owner's UID** as its effective UID, not the calling process's UID.

```
/usr/bin/passwd  →  -rwsr-xr-x  root root
```

- `s` in owner execute position = SUID set
- Running `passwd` as user `alice`: effective UID = 0 (root), real UID = alice's UID
- `passwd` can now write to `/etc/shadow`, which is owned by root
- Mechanism: kernel sets `euid = st_uid` at `execve()` time when SUID is set

### Set-Group-ID (SGID) bit — `02000`

On **executables:** same as SUID but for GID. The process runs with the file's group as effective GID.

On **directories:**
```bash
mkdir /shared && chmod g+s /shared
```
Files created inside `/shared` **inherit the directory's group** (instead of the creating user's primary group). Useful for shared project directories.

### Sticky bit — `01000`

On **directories:** prevents users from deleting or renaming files they don't own, even if they have write permission on the directory.

```
drwxrwxrwt  /tmp
```
The `t` in other execute position = sticky set. Every user can create files in `/tmp`, but only the file owner (or root) can delete their own files.

On **executables (legacy, mostly ignored):** historically kept text segment in swap; meaningless on modern systems.

---

## 13. umask

### What Is umask?

`umask` is a per-process value (default: `022`) that specifies which permission bits to **remove** when creating a new file or directory.

```
effective permissions = requested permissions & ~umask
```

**Example:**
```c
umask(022);
fd = open("file.txt", O_CREAT | O_RDWR, 0666);
// Actual permissions: 0666 & ~022 = 0666 & 0755 = 0644  (rw-r--r--)

mkdir("mydir", 0777);
// Actual permissions: 0777 & ~022 = 0755  (rwxr-xr-x)
```

**umask `022` breakdown:**
- `0` → no bits removed from owner
- `2` → write bit (`w`) removed from group
- `2` → write bit (`w`) removed from other

### Changing umask

```c
mode_t old = umask(027);  // new umask: owner full, group no-write, other nothing
// ... create files ...
umask(old);               // restore
```

`umask()` always succeeds (no error return). The change is inherited by child processes.

---

## 14. chmod() and chown()

### chmod() — Change File Permissions

```c
#include <sys/stat.h>

int chmod(const char *pathname, mode_t mode);   // follows symlinks
int fchmod(int fd, mode_t mode);                // via open FD
```

Only the file owner or root can change permissions. `mode` is the complete new permission set (not a delta). Example:

```c
chmod("script.sh", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
// equivalent to: chmod 755 script.sh
```

### chown() — Change File Ownership

```c
#include <unistd.h>

int chown(const char *pathname, uid_t owner, gid_t group);
int lchown(const char *pathname, uid_t owner, gid_t group);  // symlink itself
int fchown(int fd, uid_t owner, gid_t group);
```

- Pass `-1` for `owner` or `group` to leave it unchanged
- Only root can change the owning UID
- A non-root user can change the group to any supplementary group they belong to

**Security effect:** When `chown()` changes ownership, the kernel **clears SUID and SGID bits** to prevent privilege escalation via unexpected ownership transfer.

---

## 15. File Timestamps

Every inode stores **three** timestamps. There is no "creation time" in the traditional POSIX model (ext4 adds `crtime` but it's not exposed via standard POSIX APIs).

| Timestamp | Field | Updated by |
|-----------|-------|-----------|
| **atime** — last access | `st_atime` | `read()`, `readdir()` (may be suppressed by `noatime` mount option) |
| **mtime** — last data modification | `st_mtime` | `write()`, `truncate()` |
| **ctime** — last inode change | `st_ctime` | `write()`, `chmod()`, `chown()`, `link()`, `unlink()`, `rename()` |

**Key insight:** `ctime` is NOT "creation time". It records the last time the **inode itself changed** — any metadata change (permissions, ownership, link count) updates ctime, not just data writes.

**Why no creation time in standard POSIX:**
- Early Unix filesystems didn't have space for a 4th timestamp in the inode
- `crtime` was added later in ext4 as a filesystem extension, not standardized

### Updating Timestamps Manually

```c
#include <utime.h>
int utime(const char *pathname, const struct utimbuf *times);

#include <sys/time.h>
int utimes(const char *pathname, const struct timeval tv[2]);

#include <fcntl.h>  // utimensat
int utimensat(int dirfd, const char *pathname,
              const struct timespec times[2], int flags);
```

Use `utimensat` for nanosecond precision. Pass `NULL` for `times` to set both to current time.

---

## 16. truncate() and ftruncate()

```c
#include <unistd.h>

int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
```

### Behavior

- **Shrink:** bytes beyond `length` are discarded. `st_size` decreases. `st_blocks` decreases when entire blocks are freed.
- **Grow (extend):** file grows to `length` bytes. The new bytes read as zeros. May create a **sparse file** — the kernel may not allocate actual blocks for the zero-filled region.

```c
// Create a 1 GB sparse file
int fd = open("sparse.bin", O_CREAT | O_RDWR, 0644);
ftruncate(fd, 1024 * 1024 * 1024);
// st_size = 1 GB, but st_blocks ≈ 0 (no data blocks allocated)
close(fd);
```

**Real-world uses:**
- `ftruncate()` to pre-allocate space in a file (combine with `fallocate()` for guaranteed space)
- Database files often use `ftruncate()` to manage file size without rewriting
- `tmpfile()` pattern: create a file, `unlink()` immediately, use `ftruncate()` to size it

---

# Topic 2.6 — Directories & Links

---

## 17. Directory Internals

### A Directory Is Not a "Folder"

In the filesystem implementation, a directory is a **special file** whose data blocks contain a table of `(filename, inode_number)` pairs.

```
Directory entry table for /home/alice/:

 inode_number │ filename
──────────────┼──────────────
         2    │ .            ← always present: this directory itself
         5    │ ..           ← always present: parent directory
       1047   │ .bashrc
       1048   │ Documents
       1049   │ main.c
```

**Implication:** The filename has no meaning at the filesystem level. It's just a label pointing to an inode. This allows:
- Multiple filenames pointing to the same inode (hard links)
- Renaming a file = changing the label without touching the inode
- Moving a file within the same filesystem = updating two directory tables

---

## 18. Hard Links

### Mechanism

```c
#include <unistd.h>

int link(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
```

`link()` creates a new directory entry pointing to the **same inode**. The inode's `nlink` counter is incremented.

```
After: link("/home/alice/main.c", "/home/alice/backup.c")

 inode_number │ filename
──────────────┼───────────
       1049   │ main.c      ← both point to inode 1049
       1050   │ backup.c    ← (was 1049, now 1049 again)
```

Inode 1049's `nlink` is now **2**.

### unlink() and File Lifetime

`unlink()` removes a directory entry and decrements `nlink`. The inode (and its data blocks) are freed **only when**:
1. `nlink` drops to 0 (no more directory entries), **AND**
2. No process has the file open (open FD count = 0)

```
Scenario: file is open when unlinked

fd = open("temp.txt", O_RDWR);
unlink("temp.txt");
// Directory entry gone — file invisible to other processes
// But the file data is ALIVE because fd still holds a reference
// → data freed only when close(fd) is called
```

This is the basis of the `tmpfile()` pattern: create, immediately unlink, use via FD. The file is automatically cleaned up when the process exits or closes the FD.

### Hard Link Limitations

1. **Cannot cross filesystem boundaries** — directory entries reference inodes by number; inode numbers are only unique within a filesystem
2. **Cannot hard-link directories** — would create cycles in the directory tree, breaking path resolution and tools like `find`

---

## 19. Symbolic Links

```c
#include <unistd.h>

int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
```

A symlink is a special file type (`S_ISLNK`) whose **data content is a path string** (the target path).

```
symlink("/etc/nginx/nginx.conf", "/etc/nginx/conf")

Filesystem:
  inode 5000 (symlink file):
    st_mode = S_IFLNK | 0777
    data = "/etc/nginx/nginx.conf"   ← stored as file content
```

### Symlink Resolution

When the kernel encounters a symlink during pathname lookup, it **substitutes the target path** and continues resolution:

```
open("/etc/nginx/conf/mime.types")

1. Lookup "conf" in /etc/nginx/ → inode 5000
2. Detect: inode 5000 is a symlink
3. Read symlink data: "/etc/nginx/nginx.conf"
4. Substitute and continue: now resolving "/etc/nginx/nginx.conf/mime.types"
```

### Dangling Symlinks

If the target path doesn't exist, the symlink becomes **dangling**:
```bash
ln -s /nonexistent/path mylink
stat(mylink)    # → ENOENT  (follows symlink, target missing)
lstat(mylink)   # → SUCCESS (stat of the symlink itself)
```

Dangle symlinks are not errors at creation time — the target may be created later (e.g., before a USB drive is mounted).

### readlink()

```c
char buf[PATH_MAX];
ssize_t len = readlink("/etc/nginx/conf", buf, sizeof(buf));
buf[len] = '\0';  // readlink does NOT null-terminate
```

`readlink()` returns the raw symlink content (the stored path string) **without resolving it**. Does not follow the link.

---

## 20. Hard Link vs Symbolic Link

| Property | Hard Link | Symbolic Link |
|----------|-----------|---------------|
| Underlying mechanism | Directory entry pointing to inode | Special file containing a path string |
| Cross-filesystem support | ❌ No | ✅ Yes |
| Can link directories | ❌ No | ✅ Yes |
| Survives target deletion | ✅ Yes (owns the inode) | ❌ No (becomes dangling) |
| File disappears if link deleted | Only when last link removed | Symlink itself is removed; target unaffected |
| `ls -l` appearance | Normal file (same inode number) | `lrwxrwxrwx -> target` |
| stat() returns | Target file's metadata | Target file's metadata |
| lstat() returns | Target file's metadata (same inode) | Symlink file's own metadata |
| Inode count change | `nlink++` | Creates new inode for symlink |
| Permissions | Controlled by target | Symlink itself always `777`; target permissions govern access |

---

## 21. rename()

```c
#include <stdio.h>

int rename(const char *oldpath, const char *newpath);
```

### What rename() Does

`rename()` is a **single atomic operation** that:
1. Adds a directory entry for `newpath` pointing to the same inode
2. Removes the old directory entry `oldpath`

These two steps happen atomically from the caller's perspective — no other process can observe an intermediate state.

### Rename Rules

| Case | Behavior |
|------|----------|
| `newpath` doesn't exist | Simple atomic move |
| `newpath` is an existing file | Atomically **replaces** it — old file's `nlink` decremented |
| `newpath` is an existing directory | Replaces it only if directory is **empty**; fails with `ENOTEMPTY` otherwise |
| `oldpath` and `newpath` are same file | No-op, returns 0 |
| Across filesystems | Fails with `EXDEV` — must copy + delete |

### The Atomic Config Update Pattern

This is one of the most important safe-write patterns in systems programming:

```c
// GOAL: safely update /etc/myapp/config.conf without any reader ever
//       seeing a partial or corrupt version

// Step 1: write new content to a temp file in the SAME directory
int fd = open("/etc/myapp/config.conf.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
write(fd, new_config_data, len);
fsync(fd);    // flush to disk before making it visible
close(fd);

// Step 2: atomic rename — replaces old config instantly
rename("/etc/myapp/config.conf.tmp", "/etc/myapp/config.conf");

// Why this works:
// - rename() is atomic: readers see either old or new, never partial
// - fsync() before rename: data is on disk before the name change
// - If process crashes before rename: .tmp file exists, old config intact
// - If process crashes after rename: new config is fully written and synced
```

**Same-filesystem requirement:** `rename()` only works within one filesystem. `/etc/` and `/tmp/` are often on different filesystems — place the temp file in the same directory as the target.

---

## 22. Directory Operations

### mkdir()

```c
#include <sys/stat.h>

int mkdir(const char *pathname, mode_t mode);
```

Creates a new directory. `mode` is filtered by the process `umask`. The new directory always has two initial entries: `.` (itself) and `..` (parent).

```c
mkdir("/tmp/mydir", 0755);   // creates with permissions 755 & ~umask
```

### rmdir()

```c
#include <unistd.h>

int rmdir(const char *pathname);
```

Removes an **empty** directory. Fails with `ENOTEMPTY` if the directory still contains any entries (other than `.` and `..`).

### remove()

```c
#include <stdio.h>

int remove(const char *pathname);
```

Removes either a file or a directory:
- For files: calls `unlink()`
- For empty directories: calls `rmdir()`

It's the portable C library function. For recursive removal, there is no standard system call — tools like `rm -rf` implement it by traversing and calling `unlink()`/`rmdir()` in the correct order.

---

## 23. Directory Traversal

### opendir / readdir / closedir

```c
#include <dirent.h>

DIR *opendir(const char *name);
DIR *fdopendir(int fd);          // from an open FD
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
```

### struct dirent

```c
struct dirent {
    ino_t  d_ino;    // Inode number
    char   d_name[NAME_MAX + 1];  // Null-terminated filename
    // (d_type is also available on Linux, but not POSIX-portable)
};
```

`d_type` values: `DT_REG`, `DT_DIR`, `DT_LNK`, `DT_CHR`, `DT_BLK`, `DT_FIFO`, `DT_SOCK`, `DT_UNKNOWN`.

### Complete Directory Listing Pattern

```c
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

void list_dir(const char *path) {
    DIR *dirp = opendir(path);
    if (dirp == NULL) {
        perror("opendir");
        return;
    }

    errno = 0;  // Must reset errno before readdir loop
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;   // skip self and parent entries

        printf("inode=%lu  name=%s\n", (unsigned long)dp->d_ino, dp->d_name);
        errno = 0;      // reset after each call
    }

    if (errno != 0)     // distinguish end-of-dir from error
        perror("readdir");

    closedir(dirp);
}
```

**Why reset errno:** `readdir()` returns `NULL` both at end-of-directory (success) and on error. The only way to distinguish them is by checking `errno` after the loop — but only if you reset it to 0 before calling `readdir()`.

### Recursive Directory Walk

For recursive traversal, `nftw()` (POSIX) is the standard approach. For more control, use `openat()`-based traversal with directory FDs to avoid TOCTOU races:

```c
// Modern pattern: use openat() + fstatat() + fdopendir() to avoid TOCTOU
int fd = openat(parentfd, entry_name, O_RDONLY | O_DIRECTORY);
DIR *dir = fdopendir(fd);   // fd is now owned by dir; don't close separately
```

---

## 24. chroot()

```c
#include <unistd.h>

int chroot(const char *path);
```

**What it does:** Changes the calling process's notion of the filesystem root `/` to `path`. All subsequent absolute pathnames are resolved relative to this new root.

```
Before chroot("/srv/container"):
  "/" → actual filesystem root
  "/etc/passwd" → /etc/passwd on real system

After chroot("/srv/container"):
  "/" → /srv/container on real filesystem
  "/etc/passwd" → /srv/container/etc/passwd
```

### ChRoot Jail

A process in a chroot jail cannot access files outside the chroot tree using absolute paths. This creates a **sandboxed filesystem environment**.

**Foundation of containers:** Docker and similar tools use `chroot()` (or `pivot_root()`, its more robust successor) combined with Linux namespaces to create isolated container environments.

### Limitations and Escape Vectors

`chroot()` alone is **not a security boundary**:
1. **Needs root privilege** (`CAP_SYS_CHROOT`) to call
2. **A root process inside the jail can escape** by creating a new chroot point above the current jail root, or exploiting devices
3. **Does not affect already-open FDs** — a FD to a directory outside the chroot can still be used via `openat()`
4. **Does not affect `/proc`** — process can read `/proc/self/fd/` to leak paths

**Proper isolation** requires Linux namespaces (`CLONE_NEWNS` mount namespace) in addition to `chroot()` or `pivot_root()`.

---

## 25. realpath()

```c
#include <stdlib.h>

char *realpath(const char *path, char *resolved_path);
```

**What it does:** Resolves the given path to its **canonical absolute path** by:
1. Expanding all symlinks
2. Resolving `.` and `..` components
3. Removing redundant slashes

```c
char *canon = realpath("../../etc/../passwd", NULL);
// Returns: "/etc/passwd" (allocates buffer if resolved_path is NULL)
free(canon);

char *resolved = realpath("/var/run", NULL);
// If /var/run → symlink → /run
// Returns: "/run"
free(resolved);
```

**Memory management:** When `resolved_path` is NULL, `realpath()` allocates a buffer of `PATH_MAX` bytes — caller must `free()` it.

**Failure cases:** Returns NULL if any path component doesn't exist (call resolves the full path by actually traversing the filesystem).

---

## 26. Mental Model: What Delete/Rename/Move Really Means

Understanding operations at the inode level clears up many conceptual confusions:

### "Delete a file"

```
rm file.txt

Reality:
  1. Remove directory entry "file.txt" from the directory's data block
  2. Decrement inode's nlink counter
  3. IF nlink == 0 AND no open FDs:
       Free the inode slot (update inode bitmap)
       Free all data blocks (update block bitmap)
  4. IF nlink == 0 BUT there are open FDs:
       Inode and data survive until last close()
       This is NOT a bug — it's the tmpfile() pattern
  5. IF nlink > 0:
       Other directory entries still reference the inode
       Data is NOT freed — just this one name is removed
```

### "Rename a file"

```
mv file.txt newname.txt   (same directory)

Reality:
  1. Add new directory entry "newname.txt" → inode N
  2. Remove old directory entry "file.txt"
  (atomic; inode N untouched — no data movement)
```

### "Move a file"

```
mv /home/alice/file.txt /home/bob/file.txt   (same filesystem)

Reality:
  1. Add entry "file.txt" → inode N in /home/bob/ directory
  2. Remove entry "file.txt" from /home/alice/ directory
  (atomic rename; no data copied)

mv /home/alice/file.txt /mnt/usb/file.txt    (different filesystem)

Reality:
  1. copy data block-by-block to destination
  2. Create new inode on destination filesystem
  3. unlink source
  (NOT atomic — power failure can result in two partial copies)
```

### Summary Table

| Operation | Inode | Data Blocks | Directory Entry |
|-----------|-------|-------------|-----------------|
| `unlink()` (last link) | Freed (if no open FDs) | Freed | Removed |
| `unlink()` (not last link) | nlink-- only | Untouched | Removed |
| `rename()` (same fs) | Untouched | Untouched | Updated atomically |
| `rename()` (cross fs) | New inode created | Copied | Old removed, new created |
| `link()` | nlink++ | Untouched | New entry added |
| `open() + close()` | i_count changes | Untouched | Untouched |

---

## Summary

### Core Concepts by Topic

**Topic 2.4 — File Systems & Inodes:**
- Disk → Partition → Filesystem; superblock contains fs-level metadata
- Inode = all file metadata except name; identified by inode number
- On-disk inode (persistent) vs in-memory inode (runtime, adds open count, locks, dirty flag)
- ext4 extents replace block pointer chains → 1 extent can describe 1 GB contiguous run
- VFS = kernel abstraction layer presenting uniform API (`open/read/write`) across all filesystem types
- Journaling = WAL (write-ahead log) for metadata; `ordered` mode is default in ext4
- `mount()` attaches a filesystem to the VFS tree; bind mounts share inode trees without copying

**Topic 2.5 — File Attributes & Permissions:**
- `stat()`/`lstat()`/`fstat()` — read file metadata; prefer `fstat()` to avoid TOCTOU
- `st_mode` encodes file type (4 bits) + SUID/SGID/sticky (3 bits) + rwx×3 (9 bits)
- Permission check: root → owner bits → group bits → other bits; stop on first match
- SUID on executable = run with owner's effective UID; SGID on directory = inherited group ownership
- Sticky on directory = only owner can delete their own files (e.g., `/tmp`)
- `umask` filters permissions at creation time; applied as `mode & ~umask`
- Three timestamps: atime (access), mtime (data write), ctime (inode change — not creation)
- No POSIX "creation time"; ctime ≠ creation time

**Topic 2.6 — Directories & Links:**
- Directory = file whose data is `(name, inode#)` table; filename has no meaning at inode level
- Hard link = additional directory entry to same inode; survives original name deletion; same-fs only
- File data freed only when `nlink == 0 AND` no open FDs → basis of `tmpfile()` pattern
- Symlink = special file storing a path string; kernel substitutes target during pathname lookup
- `rename()` is atomic within same filesystem; the gold standard for safe file updates
- `opendir()`/`readdir()` — reset `errno` before loop; `NULL` return can be end-of-dir or error
- `chroot()` changes process's root; incomplete sandbox without namespaces
- "Delete" = remove directory entry + decrement nlink; "Move same-fs" = update two directory entries (no data moved)
