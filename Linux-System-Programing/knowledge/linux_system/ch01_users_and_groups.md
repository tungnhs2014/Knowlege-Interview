# Users, Groups & Process Credentials

> **Source:** TLPI Ch08, Ch09 | DevLinux Module 01
> **Topics:** 1.3 Users & Groups + Process Credentials

---

## Problem It Solves

Without user and group management, any program could:
- Read or overwrite any file on the system
- Kill any process
- Reconfigure system settings

A compromised program would have full access to everything.

Users and groups are Linux's **isolation and permission model** —
they define who can access which resources,
and what a process is allowed to do on behalf of a user.

---

## Concept Overview

Linux identifies every user and group by a **number**, not a name.

```
"tung"    →  UID = 1000
"admin"   →  GID = 1001
"root"    →  UID = 0      ← superuser, special significance
"nobody"  →  UID = 65534  ← used for least-privilege services
```

Names like `tung` or `root` are human-readable aliases.
The kernel only works with numbers internally.

**Important consequence:** If you delete user `tung` (UID=1000) and create
a new user with the same UID=1000, the new user **automatically owns** all
files that belonged to the old user. The kernel sees the same UID — it has
no memory of the name.

---

## System Context

### Where It Sits and How It Interacts

| Question | Answer |
|----------|--------|
| **What problem?** | Without a permission model, any process could read/write any file or kill any process |
| **Where in the system?** | UID/GID permission checks happen inside the kernel at every resource access point (open, read, write, kill, etc.) |
| **Subsystem interactions** | VFS (file permission checks), Process subsystem (credential inheritance on fork/exec), PAM (login authentication), `/proc/PID/status` (credential inspection) |
| **If it fails?** | See Failure Scenarios below |

### Failure Scenarios

| What Fails | What Happens |
|------------|-------------|
| `/etc/passwd` corrupted or missing | Login fails system-wide; `getpwnam()` returns `NULL` |
| `/etc/shadow` unreadable (wrong permissions) | PAM authentication breaks — users cannot log in |
| UID reuse after user deletion | New user with same UID silently inherits old user's files |
| Permission check fails | Syscall returns **EACCES** (errno=13) — operation refused |
| set-UID binary exploited | Attacker inherits elevated Effective UID → privilege escalation |
| `/etc/group` corrupted | Supplementary group lookup fails → unexpected permission denials |

### Three Configuration Files

#### `/etc/passwd` — User definitions

```
root:x:0:0:root:/root:/bin/bash
tung:x:1000:1000:Tung NHS:/home/tung:/bin/bash
nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin
```

Seven fields separated by `:`:
```
login_name : password : UID : GID : comment : home_dir : login_shell
```

| Field | Meaning |
|-------|---------|
| `tung` | Login name |
| `x` | Password placeholder — `x` means "password is in /etc/shadow" |
| `1000` | User ID (UID) |
| `1000` | Primary Group ID (GID) |
| `Tung NHS` | Comment (shown by `ls -l`, `finger`) |
| `/home/tung` | Home directory |
| `/bin/bash` | Shell started at login |

This file must be **world-readable** (so `ls -l` can resolve UIDs to names).
That is why passwords are NOT stored here.

#### `/etc/shadow` — Actual password storage

```
tung:$6$salt$hashedpassword...:19800:0:99999:7:::
```

Passwords are stored as **SHA-512 hashes** — never plain text.
Linux does not store the password — only a hash derived from it.

This file is **readable only by root**. Password-cracking tools cannot
read it without root privilege.

#### `/etc/group` — Group definitions

```
sudo:x:27:tung,admin
plugdev:x:46:tung
docker:x:999:tung
```

Four fields: `group_name : password : GID : user_list`

A user can belong to **multiple groups simultaneously** (supplementary groups).

```bash
$ id tung
uid=1000(tung) gid=1000(tung) groups=1000(tung),27(sudo),46(plugdev),999(docker)
```

---

## Internal Mechanism — Process Credentials

This is the critical part for embedded and security work.

Every process does NOT have just one UID. It has a **set of credential IDs**:

```
┌──────────────────────────────────────────────────────────┐
│  PROCESS CREDENTIALS                                      │
│                                                           │
│  Real UID / Real GID      → "who am I" — the login user  │
│  Effective UID / EGID     → "what privilege am I using    │
│                               right now"                  │
│  Saved set-UID / set-GID  → "privileged UID on standby"  │
│  Supplementary GIDs       → additional group memberships  │
└──────────────────────────────────────────────────────────┘
```

### How the Kernel Checks Permissions

When a process calls `open("file", O_RDONLY)`, the kernel does:

```
1. Get process's Effective UID (e.g., 1000)
2. Read file's inode: owner_uid=1000, owner_gid=1000, perms=rw-r--r--
3. Compare:
   - eUID == owner_uid → apply OWNER permission bits
   - eGID in supplementary groups → apply GROUP bits
   - otherwise → apply OTHER bits
4. Check if the relevant bit is set
5. Allow or return EACCES (Permission denied)
```

**Special case — root (eUID = 0):**
The kernel **completely skips** permission checks. Root can read/write any file.

### Why Three UIDs? — The set-user-ID Mechanism

**The problem:** Regular users need to change their own password.
But `/etc/shadow` is only writable by root.
How can a non-root user update a root-only file?

**Solution: set-user-ID bit on an executable**

```bash
$ ls -la /usr/bin/passwd
-rwsr-xr-x 1 root root 68208 /usr/bin/passwd
#    ^
#    s = set-user-ID bit is set
```

When `tung` (UID=1000) runs `/usr/bin/passwd`:

```
Before exec:   Real=1000   Effective=1000   Saved=1000
After exec:    Real=1000   Effective=0      Saved=0
                                    ↑
                         Effective becomes the file owner's UID (root=0)
```

The process now has **temporary root privilege** — enough to write `/etc/shadow`.
When `passwd` exits, the privilege disappears with the process.

### Saved set-UID — Temporarily Drop Privilege

The saved set-UID lets a process **temporarily lower its privilege** and then
restore it when needed:

```c
// Start with Real=1000, Effective=0, Saved=0  (running as set-uid-root)

setuid(getuid());         // Drop to Real UID → Effective = 1000
// ... do risky operations with low privilege ...

setuid(saved_uid);        // Restore → Effective = 0
// ... do privileged operation ...
```

This is the **least privilege principle**: never use more privilege than needed.
Reduce the attack surface by operating with low privilege by default.

---

## Architecture

```
Login         /etc/passwd  /etc/group
  ↓           sets Real UID/GID from these files
Shell starts
  ↓
User runs program
  ↓
fork() + exec()
  ↓                                         ← Normally Real = Effective
Process inherits parent's Real UID/GID
  ↓
If executable has set-UID bit:
  Effective UID = file owner's UID          ← Effective changes, Real stays
  Saved UID = same as new Effective
  ↓
Kernel uses Effective UID for ALL permission checks
```

---

## Execution Flow — End-to-End Permission Check

```
1. Login:
   PAM reads /etc/shadow → verifies SHA-512 hash
   Shell starts with Real UID/GID from /etc/passwd

2. User runs a program:
   fork()  → child inherits Real/Effective/Saved UID from parent
   exec()  → if binary has set-UID bit:
               Effective UID = binary owner's UID
               Saved UID     = new Effective UID

3. Kernel permission check (on open/read/write/stat...):
   Get process Effective UID
   Get file inode: owner_uid, owner_gid, permission bits (rwxrwxrwx)
   Resolve which permission class applies:
     eUID == owner_uid            → use owner bits (e.g., rw-)
     eGID in supplementary groups → use group bits (e.g., r--)
     otherwise                    → use other bits (e.g., r--)
   Check relevant bit:
     set? → allow
     not set? → return EACCES

4. Privilege management:
   setuid(getuid())   → drop Effective to Real (low privilege)
   ... do unsafe work ...
   setuid(saved_uid)  → restore Effective from Saved (if permitted)
```

---

## Example — Retrieving User/Group Information in C

```c
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

// Look up user by name
struct passwd *pw = getpwnam("tung");
if (pw != NULL) {
    printf("UID:   %d\n", pw->pw_uid);
    printf("GID:   %d\n", pw->pw_gid);
    printf("Home:  %s\n", pw->pw_dir);
    printf("Shell: %s\n", pw->pw_shell);
}

// Look up user by UID
struct passwd *pw = getpwuid(1000);

// Look up group by name
struct group *gr = getgrnam("sudo");
printf("GID: %d\n", gr->gr_gid);

// Get credentials of the current process
uid_t uid  = getuid();    // Real UID
uid_t euid = geteuid();   // Effective UID
gid_t gid  = getgid();    // Real GID
gid_t egid = getegid();   // Effective GID
```

**Thread safety warning:** `getpwnam()` returns a pointer to a **static buffer**
that is overwritten on the next call. In multithreaded code, use the
reentrant versions: `getpwnam_r()`, `getpwuid_r()`, `getgrnam_r()`.

---

## Debugging

```bash
# Show all credentials of current shell
$ id
uid=1000(tung) gid=1000(tung) groups=1000(tung),27(sudo),999(docker)

# Check what UID/GID a running process has
$ cat /proc/1234/status | grep -E "^(Uid|Gid)"
Uid:    1000  1000  1000  1000   # Real Effective Saved FS
Gid:    1000  1000  1000  1000

# Find set-UID binaries (potential security concern)
$ find /usr/bin -perm -4000 -ls
  ... /usr/bin/passwd
  ... /usr/bin/sudo
  ... /usr/bin/mount

# Check file ownership and permissions
$ ls -la /etc/shadow
-rw-r----- 1 root shadow 1234 Jan 1 00:00 /etc/shadow
#                  ↑ only root and shadow group can read
```

---

## Real-world Usage

### Embedded Linux — Common Practice

```
Service runs as:   nobody (UID=65534)  — least privilege
File owned by:     specific service user (e.g., www-data for web server)
Root services:     only when absolutely required (network config, hardware init)
```

**Never run services as root in production.** If a service is exploited,
the attacker gets the privileges of that process.
If it's root — the attacker has full system control.

### Prefer Capabilities over set-UID root

Instead of making a binary run as full root, use Linux capabilities:
```bash
# Give only the network binding capability (port <1024) instead of full root
$ setcap cap_net_bind_service=ep /usr/bin/my_server
```

Covered in detail in Phase 8 — Linux Capabilities (Ch39).

---

## Key Takeaways

```
User        = a number (UID), not a name
Group       = a number (GID); one user belongs to multiple groups
/etc/passwd = user definitions (no real password)
/etc/shadow = hashed passwords, root-only readable
/etc/group  = group definitions

Every process has THREE user credentials:
  Real UID      = who actually launched the process
  Effective UID = what the kernel uses for permission checks
  Saved UID     = backup to restore after temporary privilege drop

set-user-ID bit = allows executable to run with owner's privileges
Least privilege = drop to low privilege, raise only when needed
```
