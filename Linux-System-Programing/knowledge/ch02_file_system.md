# Chapter 2 — File Systems, Attributes, and Directories

> Topics: 2.4 File Systems & Inodes · 2.5 File Attributes & Permissions · 2.6 Directories & Links
> Main sources: TLPI Ch14, Ch15, Ch18 | DevLinux Module 02
> Production context: backend storage paths, config files, log rotation, deploy-safe replacement, container mount views, embedded flash filesystems, permission debugging, and file cleanup in long-running services.

---

## Problem It Solves

File I/O is not only about moving bytes.
Programs also need answers to deeper questions:

- how does a pathname become an actual file object?
- what exactly is a file's identity?
- why can one file have more than one name?
- why does deleting a filename not always free disk space immediately?
- why can a file show `0644` and still fail with "Permission denied"?
- why is `rename()` such an important safety tool in production systems?

This chapter solves the naming, identity, metadata, and permission side of Linux file handling.

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | inode vs filename, directory entry, `stat()/lstat()/fstat()`, permissions, `umask`, hard link, symlink, `unlink()`, `rename()` | Explain what a file is, why deletion can be delayed, and why permissions fail. |
| Work useful | pathname lookup, directory permissions, sticky bit, safe update with temp file + `fsync()` + `rename()`, `opendir()/readdir()` | Build robust config/log/data file workflows and debug "disk still full" or "permission denied". |
| Recognize | mount points, VFS, dentry cache, journaling, extents, `openat()` family, `chroot()`, `realpath()` | Understand production behavior across filesystems, containers, and security boundaries. |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| filesystem | Organized collection of directories, files, metadata, and data blocks. | ext4, XFS, tmpfs, NFS. |
| VFS | Kernel abstraction presenting one API across different filesystem implementations. | `open()` can target ext4, tmpfs, `/proc`, or a device. |
| mount point | Directory where another filesystem is attached into the single Linux tree. | `/home`, `/mnt/data`, container bind mounts. |
| superblock | Metadata describing one mounted filesystem instance. | Size, block size, filesystem state. |
| inode | File identity and metadata, not the filename. | Stores type, owner, mode, timestamps, size, block mapping. |
| directory entry / dentry | Mapping from a name to an inode number; kernel may cache lookups as dentries. | `report.txt -> inode 12345`. |
| hard link | Another directory entry pointing to the same inode. | Same `st_dev` + `st_ino`; link count increases. |
| symbolic link | File whose content is a pathname to another file. | Can cross filesystems; can dangle. |
| link count | Number of hard links to an inode. | Data is freed only when link count is zero and no FD still references it. |
| `struct stat` | Metadata returned by `stat()`, `lstat()`, or `fstat()`. | `st_mode`, `st_ino`, `st_nlink`, `st_size`, timestamps. |
| `st_mode` | Bit field containing file type and permission bits. | Use `S_ISREG()` and permission masks. |
| permission bits | Classic owner/group/other read-write-execute model. | `0644`, `0755`, `0700`. |
| directory execute bit | Search/traverse permission on a directory. | Needed to access `/a/b/file`, not to execute the directory. |
| `umask` | Per-process mask that removes permissions from newly created files. | Requested `0666` with `umask 022` becomes `0644`. |
| sticky bit | Directory rule allowing only file owner, directory owner, or privileged user to remove/rename entries. | `/tmp` usually has it. |
| atomic rename | Same-filesystem `rename()` updates directory entries atomically. | Foundation of safe config replacement. |

---

## Concept Overview

### Roadmap

```text
disk / partition
   |
   v
mounted filesystem
   |
   +--> directories map names to inode numbers
   +--> inodes store metadata and block mapping
   |
   v
VFS presents a uniform interface
   |
   +--> pathname lookup
   +--> permission checks
   +--> mount traversal
   |
   v
process gets access to the target file object
```

### The Core Mental Model

The most important mental model in this file is:

> A filename is not the file itself.

More precisely:

- the **directory entry** stores the mapping from a name to an inode number;
- the **inode** stores metadata and block mapping for the file;
- the **data blocks** store the actual file contents.

That is why:

- hard links are possible;
- `rename()` often changes names without moving file data;
- `unlink()` removes a name first, not necessarily the data immediately.

### Common Beginner Confusions to Fix Early

Three confusions cause many real bugs:

1. Thinking "delete a file" means "immediately erase the bytes from disk".
2. Thinking file permissions alone determine access.
3. Thinking symlinks are just "another kind of hard link".

This chapter should make all three points clear.

---

## System Context

### Where This Chapter Sits in Linux

```text
User program
    |
    +--> open(), stat(), chmod(), link(), rename(), opendir(), ...
    |
    v
VFS
    |
    +--> pathname lookup and dentry cache
    +--> mount table traversal
    +--> inode objects
    +--> permission checks using process credentials
    |
    v
filesystem driver
    |
    +--> superblock and filesystem metadata
    +--> inode table / extents / directory blocks
    +--> journaling and writeback
    |
    v
block layer and storage device
```

### Subsystems That Interact with These Topics

- **File I/O core** matters because `open()`, `read()`, and `write()` ultimately act on the
  inode and mount structures described here.
- **Process credentials** matter because the kernel checks effective UID, effective GID, and
  supplementary groups during permission checks.
- **VFS** matters because Linux must present one interface across ext4, xfs, tmpfs, NFS,
  `/proc`, and other filesystems.
- **Page cache and writeback** matter because file metadata and data are cached in memory and
  written back later.
- **Containers and namespaces** matter because mount namespaces can give different processes
  different views of the mount tree.

---

## Architecture

### From Name to Data

```text
pathname component
   |
   v
directory entry
   |
   v
inode number
   |
   v
inode
   |
   +--> file type
   +--> owner / group
   +--> permission bits
   +--> timestamps
   +--> size
   +--> block mapping / extents
   |
   v
data blocks
```

### A Minimal Filesystem Layout

Different filesystems implement details differently, but the classic mental model is:

```text
filesystem
   |
   +--> superblock
   +--> inode metadata structures
   +--> directory data blocks
   +--> file data blocks
   +--> allocation metadata such as free-space tracking
```

For learning Linux file semantics, you do not need to memorize every ext4 structure.
What matters first is:

- names live in directories;
- file identity lives in inodes;
- bytes live in data blocks;
- mounts connect one filesystem tree into another.

### Four Important Kernel Objects

| Object | Role |
|--------|------|
| **superblock / mount** | represents one mounted filesystem instance |
| **dentry** | caches a name-to-inode lookup result |
| **inode** | represents a filesystem object's identity and metadata |
| **open file description** | runtime I/O state such as current offset |

This chapter focuses mostly on the first three.

---

## Execution Flow

### Flow 1: Opening `/home/alice/report.txt`

```text
1. Start from root directory or current working directory
2. Resolve "home" in the current directory
3. Resolve "alice" inside /home
4. Resolve "report.txt" inside /home/alice
5. For each directory in the path, check search/traverse permission
6. For the final inode, check the requested access mode
7. If allowed, create an open file description and return an FD
```

### Flow 2: Creating a New File

```text
1. Resolve the parent directory
2. Check write + execute permission on that directory
3. Allocate a new inode
4. Apply the requested mode filtered by the process umask
5. Add a new directory entry mapping name -> inode
6. Return a descriptor if creation was part of open()
```

### Flow 3: Deleting a File

```text
1. Remove one directory entry
2. Decrement the inode's link count
3. If link count becomes 0 and no process still has it open, free the inode and data blocks
4. Otherwise, the underlying file object survives for now
```

### Flow 4: Renaming a File Within One Filesystem

```text
1. Update directory entries so the name changes from oldpath to newpath
2. Keep the same inode
3. Do not move file contents
4. Make the change appear atomically to other processes
```

That last property is why `rename()` is so valuable in real systems.

---

## 2.4 File Systems and Inodes

### File Identity: The Inode, Not the Name

An inode is the persistent identity of a file within one filesystem.

The inode stores metadata such as:

- file type;
- owner UID and group GID;
- permission bits;
- file size;
- timestamps;
- link count;
- mapping to data blocks or extents.

The inode does **not** store the filename.

That single fact explains much of UNIX filesystem behavior.

### Directory Entries Store Names

A directory is a special file whose contents act like a table of name-to-inode mappings.

```text
directory /home/alice

name        -> inode
.           -> 1050
..          -> 1002
notes.txt   -> 2088
report.txt  -> 2091
```

So when you rename a file in the same filesystem, Linux usually updates directory entries and
keeps the inode.

### On-Disk vs In-Memory Inode

There are two useful views of an inode:

| View | Purpose |
|------|---------|
| on-disk inode | persistent metadata stored by the filesystem |
| in-memory inode | kernel runtime object used while the file is active |

The in-memory inode may also be connected to:

- page-cache state;
- locks;
- reference counts;
- dentry cache relationships.

### VFS: Why Applications Do Not Care Which Filesystem Is Underneath

Linux supports many filesystem types:

- ext4;
- xfs;
- btrfs;
- tmpfs;
- NFS;
- `/proc`;
- `/sys`.

Without an abstraction layer, applications would need filesystem-specific logic for common
operations.

VFS solves that by presenting one interface for path lookup, metadata access, and file
operations.

### Mounting

Mounting connects one filesystem into the global pathname tree.

Conceptually:

```text
existing tree + mount point + mounted filesystem -> one larger visible tree
```

Examples:

- mount a disk partition at `/data`;
- mount `procfs` at `/proc`;
- mount `tmpfs` at `/run`.

Once mounted, path lookup crossing that mount point enters a different filesystem instance.

### Practical Admin Flow: Device to Mounted Filesystem

Application code usually starts at a pathname, but production debugging often starts one layer
lower: "what storage and filesystem is this path really using?"

The DevLinux practical workflow is useful as a recognition model:

```text
block device
   |
   v
partition table / partition
   |
   v
mkfs creates filesystem metadata
   |
   v
mount attaches filesystem at a directory
   |
   v
application opens path under that mount point
```

Typical commands map to different lifecycle stages:

| Stage | Command family | What it tells you |
|-------|----------------|-------------------|
| discover devices | `lsblk`, `blkid` | which block devices, partitions, labels, UUIDs, and filesystem types exist |
| partition storage | `fdisk`, `parted`, `gdisk` | whether the disk has usable partition layout |
| create filesystem | `mkfs.ext4`, `mkfs.xfs`, `mkfs.vfat` | initializes filesystem structures on a partition or device |
| attach filesystem | `mount`, `findmnt`, `/etc/fstab` | where the filesystem appears in the pathname tree |
| check/repair | `fsck`, `xfs_repair` | filesystem consistency workflow, usually offline or during boot/recovery |

The application-facing rule is:

> `open("/data/app/state")` does not care how `/data` was prepared, but debugging must.

If `/data` is the wrong mount, read-only, full, out of inodes, or backed by a failing device,
the symptom may still appear as ordinary `open()`, `write()`, `fsync()`, or `rename()` failure.

### Bind Mounts

A bind mount makes an existing subtree visible at another location in the tree.

This matters in practice for:

- containers;
- chroot-like environments;
- service sandboxing;
- exposing one directory in multiple places.

### Pathname Lookup

Path resolution is more than string parsing.

For each pathname component, the kernel:

1. looks up the name in the current directory;
2. finds the next inode;
3. follows mount points when needed;
4. usually dereferences symlinks in intermediate components;
5. checks directory search permission as it goes.

### Dentry Cache

The kernel caches many pathname lookup results.

That cache matters because repeated path resolution would be expensive otherwise.

A useful mental model is:

> dentry cache remembers "this name in this directory led to that inode".

### Journaling

Journaling exists to reduce filesystem corruption after crashes or power loss.

It does **not** mean your application data is magically durable after every `write()`.

What journaling primarily helps with is:

- metadata consistency;
- faster recovery after crashes;
- fewer broken filesystem structures after power loss.

Application-level durability still requires careful use of `fsync()`, `fdatasync()`, and safe
update patterns.

### Deep Note: ext4 Extents

For interview and system understanding, it is useful to know one modern detail:

> ext4 often uses extents, which describe contiguous block ranges, instead of relying only on
> older direct/indirect block-pointer schemes.

Why this matters:

- large files can be described more efficiently;
- sequential access can be more efficient;
- fragmentation handling improves.

For most application programmers, this is background knowledge, not something used directly in
normal API code.

---

## 2.5 File Attributes and Permissions

### `stat()`, `lstat()`, and `fstat()`

```c
#include <sys/stat.h>

int stat(const char *pathname, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
```

Use them like this:

| Call | Typical use |
|------|-------------|
| `stat()` | get metadata for the target of a pathname |
| `lstat()` | get metadata for the symlink itself |
| `fstat()` | get metadata for an already-open file without another pathname lookup |

### Important `struct stat` Fields

| Field | Meaning |
|-------|---------|
| `st_dev` | device ID of containing filesystem |
| `st_ino` | inode number |
| `st_mode` | file type and permission bits |
| `st_nlink` | hard-link count |
| `st_uid` / `st_gid` | owner IDs |
| `st_size` | logical file size |
| `st_blocks` | allocated 512-byte blocks |
| `st_atime` | last access time |
| `st_mtime` | last data modification time |
| `st_ctime` | last inode metadata change time |

### `st_mode`: File Type and Permissions Together

`st_mode` encodes both:

- what kind of object this is;
- what permission bits are set.

Useful file-type macros:

```c
S_ISREG(mode)
S_ISDIR(mode)
S_ISLNK(mode)
S_ISCHR(mode)
S_ISBLK(mode)
S_ISFIFO(mode)
S_ISSOCK(mode)
```

### The Classic Permission Model

Linux starts with three classes:

- owner;
- group;
- other.

Each class has:

- read;
- write;
- execute.

This gives the familiar 9 permission bits.

### Permission-Checking Algorithm

For ordinary permission checks without ACL complications, the mental model is:

```text
1. If effective UID is root, many checks bypass ordinary permission rules
2. Else if effective UID matches file owner, use owner bits
3. Else if effective GID or supplementary groups match, use group bits
4. Else use other bits
```

Important consequence:

> The kernel does not combine owner, group, and other permissions.

If you are the owner, owner bits decide the result.
It does not "fall through" to use group bits just because they are more permissive.

### `access()` Is a Special Case

`access()` is about the calling user's real identity, not the effective identity normally used
for actual operations.

That is why `access()` can disagree with what an `open()` call would do in a privileged process.

### Directory Permissions Are Different from File Permissions

This is one of the most important practical topics in the chapter.

| Bit | Meaning on a regular file | Meaning on a directory |
|-----|----------------------------|------------------------|
| `r` | read file contents | list entries |
| `w` | modify file contents | create/remove/rename entries in that directory |
| `x` | execute file | traverse/search through that directory |

The key beginner lesson:

> To access `/a/b/c.txt`, you need execute/search permission on every directory in the path.

This is why a file can have readable permission bits and still fail with `EACCES`.

### Special Permission Bits

#### SUID

On an executable file, SUID means the new process gets the file owner's UID as its effective
UID at `exec()` time.

This is how programs such as `passwd` can do privileged work without giving the user a root
shell.

#### SGID

On executables, SGID behaves similarly for group identity.

On directories, SGID causes newly created files and subdirectories to inherit the directory's
group.

This is very useful for shared project directories.

#### Sticky Bit

On directories such as `/tmp`, the sticky bit prevents ordinary users from deleting or renaming
files that they do not own, even when the directory is otherwise writable.

### `umask`

`umask` is a per-process filter applied during file and directory creation.

The rough rule is:

```text
effective mode = requested mode & ~umask
```

Examples:

- request `0666` with `umask 022` -> actual file mode `0644`
- request `0777` with `umask 022` -> actual directory mode `0755`

`umask` matters during creation.
It does not retroactively change existing files.

### `chmod()` and `chown()`

`chmod()` changes permission bits.
`chown()` changes ownership.

Important practical detail:

> Ownership changes can clear SUID and SGID bits for security reasons.

That prevents an unexpected ownership transfer from silently leaving behind a privileged binary.

### File Timestamps

Three timestamps matter in classic POSIX file metadata:

| Time | Meaning |
|------|---------|
| `atime` | last access time |
| `mtime` | last content modification time |
| `ctime` | last inode metadata change time |

One of the most common interview mistakes is:

> `ctime` is **not** creation time.

It means change time for inode metadata.

Modern filesystems may also record creation time, but that is not the classic portable POSIX
mental model.

---

## 2.6 Directories and Links

### Directories Are Naming Structures

A directory is not just a "folder" in the GUI sense.
At the filesystem level, it is the naming structure that maps names to inode numbers.

That is why:

- renaming a file often means changing directory entries, not moving bytes;
- deleting a file name removes one mapping first;
- hard links are possible.

### Hard Links

`link(oldpath, newpath)` creates another directory entry that points to the same inode.

Consequences:

- both names refer to the same underlying file;
- both names share the same inode number;
- the file's link count increases.

#### Hard Link Limits

Hard links normally:

- cannot cross filesystem boundaries;
- cannot be made to directories in ordinary use.

### `unlink()` and File Lifetime

`unlink()` removes one directory entry.

The file's storage is freed only when:

1. the link count reaches zero, and
2. no process still has the file open.

This explains a famous UNIX behavior:

> A file can be deleted from the directory tree and still remain alive through an open file
> descriptor.

That is not a bug.
It is a feature used by temporary-file patterns.

### Symbolic Links

A symbolic link is a special file whose contents are a path string.

Unlike a hard link:

- it has its own inode;
- it points to a pathname, not directly to the target inode;
- it can cross filesystems;
- it can refer to directories;
- it can become dangling if the target disappears.

### How Symlink Resolution Works

During pathname resolution:

- intermediate symlink components are generally followed;
- whether the final component is followed depends on the system call;
- `stat()` follows the link to the target;
- `lstat()` reports the link itself;
- `readlink()` returns the stored path string without following it.

One more useful rule:

> `rename()` does not dereference symbolic links in its arguments; it operates on directory
> entries.

### Hard Link vs Symbolic Link

| Property | Hard link | Symbolic link |
|----------|-----------|---------------|
| points to | same inode | pathname string |
| crosses filesystems | no | yes |
| can refer to directories | normally no | yes |
| survives target deletion | yes, if another link still exists | no, becomes dangling |
| has its own inode | no | yes |

### `rename()`

Within one filesystem, `rename()` is the gold-standard safe name-change operation.

Key properties:

- it usually keeps the same inode;
- it updates directory entries atomically;
- it does not copy file contents;
- across filesystems it fails with `EXDEV`.

That last point matters because shell `mv` may fall back to copy-and-delete, but the syscall
`rename()` itself does not.

### The Safe Update Pattern

One of the most important real-world patterns is:

1. write new content to a temp file in the same directory;
2. flush as required;
3. `rename()` the temp file over the old file.

Why this matters:

- readers see either old or new content;
- they do not observe a half-written file under the final pathname.

### Directory Operations and Traversal

Typical directory APIs:

```c
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
```

These APIs let programs:

- list names in a directory;
- inspect entries one by one;
- combine names with `stat()` or `lstat()` for deeper inspection.

### `mkdir()`, `rmdir()`, and `remove()`

- `mkdir()` creates a new directory;
- `rmdir()` removes an empty directory;
- `remove()` is a convenience interface that removes a file or empty directory.

Creation still obeys permission and `umask` rules.

### `chroot()`

`chroot()` changes the process root directory for absolute-path resolution.

It is useful for:

- restricted environments;
- testing;
- old-style service isolation.

But the most important practical warning is:

> `chroot()` is not a full security boundary by itself.

It changes pathname interpretation, not the whole security model.

### `realpath()`

`realpath()` resolves:

- symbolic links;
- `.` components;
- `..` components;

to produce a canonical absolute pathname.

It is useful for diagnostics, tooling, and path normalization.

### Mental Model: Delete, Rename, and Move

This is the short version worth remembering:

| Operation | What really happens |
|-----------|---------------------|
| delete file | remove one directory entry; maybe free inode later |
| rename in same filesystem | change directory entries; keep inode |
| move in same filesystem | same as rename |
| move across filesystems | copy to new inode on destination, then delete source if user-space tool chooses to do so |

---

## Work-Useful Patterns

| Pattern | Use it when | Production trap |
|---------|-------------|-----------------|
| Identify files by `st_dev + st_ino` | You need to know whether two names refer to the same file. | Pathnames are not stable identities; hard links and renames break pathname assumptions. |
| Use `lstat()` before following links | Tools scan user-controlled directories. | `stat()` follows symlinks and may inspect the target instead of the link. |
| Safe file replacement | Updating config, state, cache, or checkpoints. | Write temp file, flush it, `rename()` over old path, then sync parent directory when durability matters. |
| Diagnose deleted-open files | Disk usage stays high after `rm`. | `unlink()` removed the name, but a process still holds the inode open. |
| Check directory permissions separately | "Permission denied" happens despite file mode looking correct. | Need execute/search on each parent directory. |
| Keep `/tmp` sticky | Shared writable directories. | Without sticky bit, users may remove or rename each other's files. |
| Avoid trusting `d_type` alone | Portable directory walkers. | Some filesystems return `DT_UNKNOWN`; fall back to `lstat()`. |
| Treat `chroot()` as containment helper, not complete sandbox | Legacy FTP-style isolation or test roots. | Open directory FDs, privileges, device files, and FD passing can bypass weak setups. |

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| VFS dentry/inode caches | They make pathname lookup fast; correctness still comes from permission and inode rules. |
| journaling modes | Journaling protects filesystem metadata consistency; it does not replace application-level `fsync()` discipline. |
| ext4 extents | Modern block mapping for efficient large files; useful background, rarely direct app code. |
| bind mounts and mount namespaces | Common in containers; two processes may see different path trees. |
| `openat()` family | Avoids current-working-directory races and helps write safer directory-relative code. |
| `nftw()` | Useful for whole-tree traversal; be careful with symlink behavior and mount boundaries. |
| filesystem-specific timestamp resolution | Linux supports nanosecond fields, but real support depends on filesystem. |
| cross-filesystem moves | `rename()` cannot move across filesystems; user-space tools must copy and remove. |
| partitioning and `mkfs` | Admin workflow before a filesystem can be mounted; application code usually only sees the mounted result. |
| `fsck` | Offline or boot-time repair/check tool; not an application-level consistency substitute. |
| `statvfs()` | Programmatic way to inspect filesystem capacity and flags; useful for preflight checks, not a guarantee against later ENOSPC. |

---

## Example

### Example 1 — Inspect metadata with `lstat()`

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    struct stat sb;

    if (argc != 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

    if (lstat(argv[1], &sb) == -1) {
        perror("lstat");
        return 1;
    }

    printf("inode=%lu nlink=%lu size=%ld\n",
           (unsigned long)sb.st_ino,
           (unsigned long)sb.st_nlink,
           (long)sb.st_size);

    if (S_ISREG(sb.st_mode))
        puts("regular file");
    else if (S_ISDIR(sb.st_mode))
        puts("directory");
    else if (S_ISLNK(sb.st_mode))
        puts("symbolic link");

    return 0;
}
```

What it teaches:

- metadata inspection is a first-class programming task;
- `lstat()` is how you inspect the link itself;
- inode number and link count are concrete runtime facts, not abstract theory.

### Example 2 — List directory entries

```c
#include <dirent.h>
#include <errno.h>
#include <stdio.h>

int main(void) {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    for (;;) {
        errno = 0;
        struct dirent *de = readdir(dir);
        if (de == NULL) {
            if (errno != 0) {
                perror("readdir");
                closedir(dir);
                return 1;
            }
            break;
        }

        printf("%s\n", de->d_name);
    }

    if (closedir(dir) == -1) {
        perror("closedir");
        return 1;
    }

    return 0;
}
```

What it teaches:

- directories are traversed through dedicated directory APIs;
- directory contents are names first, not automatically full metadata records.

### Example 3 — Unlink after open

```c
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd = open("temp.data", O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (unlink("temp.data") == -1) {
        perror("unlink");
        close(fd);
        return 1;
    }

    if (write(fd, "abc\n", 4) == -1) {
        perror("write");
        close(fd);
        return 1;
    }

    puts("name removed, file still alive through fd");

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    return 0;
}
```

What it teaches:

- deleting a pathname does not necessarily destroy the underlying file immediately;
- open file descriptors keep the underlying file alive.

### Example 4 — Atomic replace with `rename()`

```c
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd = open("config.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    ssize_t n = write(fd, "mode=stable\n", 12);
    if (n == -1) {
        perror("write");
        close(fd);
        return 1;
    }

    if (n != 12) {
        fprintf(stderr, "short write\n");
        close(fd);
        return 1;
    }

    if (fsync(fd) == -1) {
        perror("fsync");
        close(fd);
        return 1;
    }

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    if (rename("config.tmp", "config.conf") == -1) {
        perror("rename");
        return 1;
    }

    return 0;
}
```

What it teaches:

- `rename()` is the core of safe same-filesystem replacement;
- the final pathname can change atomically even though file creation and writing happened
  earlier.

---

## Debugging

Useful commands:

```bash
# Show inode numbers and link counts
ls -li
stat file.txt

# Trace pathname resolution component by component
namei -l /path/to/file

# Show mount layout
mount
findmnt

# Check inode exhaustion as well as disk space
df -h
df -i

# Inspect block devices and filesystem type
lsblk -f
findmnt -T /path/to/file

# Check or repair an unmounted filesystem from admin/debug workflow
fsck -n /dev/<device>

# Resolve symlinks and canonical paths
readlink linkname
realpath path

# Find all names that refer to the same inode
find . -inum <inode_number>
find . -samefile file.txt

# Find deleted files still held open by processes
lsof +L1
```

Common pitfalls:

- thinking a filename is the file's identity;
- forgetting that directory execute/search permission is required for path traversal;
- confusing `ctime` with creation time;
- assuming `rename()` across filesystems is still atomic;
- mixing up hard links and symlinks;
- forgetting that `stat()` and `lstat()` answer different questions;
- assuming `unlink()` immediately frees disk space;
- treating `chroot()` as a complete security sandbox;
- treating `df -h` as enough when inode exhaustion, read-only mounts, or wrong mount targets are possible;
- trying to use `fsck` on a mounted production filesystem as if it were a normal application
  recovery step.

---

## Real-world Usage

### Where This Knowledge Shows Up

- safe config-file replacement with temp file plus `rename()`;
- debugging "Permission denied" when the real issue is a directory bit in the path;
- detecting why disk space is still consumed after a file was deleted;
- shared team directories using SGID and controlled `umask`;
- deployment systems that rely on symlinks for releases and rollbacks;
- mount-based isolation and container filesystem layouts.

### Practical Patterns

| Scenario | Practical design |
|----------|------------------|
| replace a config file safely | write temp file in same directory, then `rename()` |
| create shared project tree | SGID directory plus suitable `umask` or ACL |
| keep temporary data private and auto-cleaned | open then `unlink()` |
| inspect a symlink itself | use `lstat()` or `readlink()` |
| understand why access fails | inspect directory bits, ownership, group membership, and mount context |
| explain "disk full" despite space | check `df -h`, `df -i`, deleted-open files, quotas, and the actual mount |
| prepare storage manually | partition if needed, create filesystem with `mkfs`, mount it, then verify with `findmnt` |

---

## Coverage Notes

| Coverage item | Status | Notes |
|---------------|--------|-------|
| 2.4 Filesystems and inodes | Covered | VFS, inode/dentry/path model, mount points, metadata, cache context. |
| 2.5 Attributes and permissions | Covered | `stat/lstat/fstat`, ownership, mode bits, `chmod/chown`, `umask`, special bits. |
| 2.6 Directories and links | Covered | hard links, symlinks, `opendir/readdir`, `unlink`, `rename`, `chroot` limits. |
| Must-cover production debug | Covered | `namei`, `stat`, `findmnt`, `df -h`, `df -i`, `lsof +L1`, same-inode checks. |
| DevLinux filesystem admin workflow | Covered | Recognize partitioning, `mkfs`, mount verification, `fsck`, and filesystem-capacity checks. |

No remaining filesystem-file coverage gap is known.

---

## Interview-Relevant Questions

- What is the difference between a filename and an inode?
- Why can one file have multiple hard links?
- Why does deleting an open file not always free disk space immediately?
- What is the difference between `stat()` and `lstat()`?
- How do directory permissions differ from file permissions?
- Why can `rename()` be used for atomic updates?
- Why does `rename()` fail with `EXDEV` across filesystems?
- What is the difference between a hard link and a symbolic link?
- What is `umask`, and when is it applied?
- Why is `ctime` not the same as creation time?
- Why can `ls -l` show readable permissions while `open()` still fails?
- What permissions are required to create or delete a file inside a directory?
- How would you debug disk space that remains used after deleting a large log file?
- Why should safe replacement write the temp file in the same directory as the target?
- What are the security limitations of using `chroot()` as a jail?

---

## Key Takeaways

- Names live in directory entries; file identity lives in inodes.
- A directory is fundamentally a naming structure, not just a GUI folder.
- VFS gives Linux one interface across many filesystem types.
- Mounting connects filesystem instances into one visible pathname tree.
- `stat()` inspects metadata; `lstat()` is how you inspect the symlink itself.
- Permission checks depend on process credentials and the correct class of permission bits.
- Directory execute/search permission is essential for pathname traversal.
- Hard links create multiple names for one inode; symlinks store a path string.
- `unlink()` removes a name first; the file survives while links or open references remain.
- `rename()` within one filesystem is atomic and is central to safe file replacement patterns.
