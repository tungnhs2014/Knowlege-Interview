# Chapter 1 - Users, Groups, and Process Credentials

> Topics: 1.3 Users & Groups - UID/GID, credentials, set-user-ID
> Main sources: TLPI Ch08, Ch09
> Production context: file permissions, service identities, privilege dropping, set-user-ID helpers, containers, embedded devices, and debugging "permission denied" failures.

---

## Problem It Solves

Linux is a multiuser system. Many processes can run at the same time, owned by different users, touching shared files, devices, sockets, IPC objects, and other processes.

The kernel needs a concrete answer to:

> Who is this process acting as, and what is it allowed to do right now?

Human names such as `root` or `alice` are convenient, but the kernel uses numeric IDs. A process carries user and group credentials, and the kernel uses those credentials during access checks.

This chapter is not admin trivia. It is how you debug permissions, design safe services, and understand why a program that "runs as root" should still drop privilege whenever possible.

---

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | UID, GID, `/etc/passwd`, `/etc/group`, real/effective IDs, supplementary groups | Explain how Linux decides ownership and basic access. |
| Work useful | set-user-ID execution, saved IDs, temporary vs permanent privilege drop, `/proc/<PID>/status`, `id`, `ps` | Debug permission problems and avoid unsafe privileged code. |
| Recognize | filesystem IDs, `setresuid()`, capabilities, shadow passwords, reentrant lookup APIs | Recognize advanced credential topics without overloading first-pass learning. |

---

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Login name | Human-readable user name. | `root`, `alice`, `www-data`. |
| UID | Numeric user ID used by the kernel. | UID `0` is traditionally root. |
| Group name | Human-readable group name. | `staff`, `tty`, `docker`. |
| GID | Numeric group ID used by the kernel. | File ownership stores numeric GIDs. |
| `/etc/passwd` | User account metadata. | login name, UID, primary GID, home, shell. |
| `/etc/group` | Group definitions and supplementary members. | Extra group memberships live here. |
| `/etc/shadow` | Restricted password-hash and aging data. | Not specified by POSIX; common on Linux. |
| Real UID/GID | Identity of the user/group that started the process. | Inherited from parent process. |
| Effective UID/GID | Identity usually used for permission decisions. | Changes during set-user-ID execution. |
| Saved set-user-ID/GID | Stored ID that lets some programs drop and regain effective privilege. | Core to set-user-ID design. |
| Supplementary groups | Extra groups attached to a process. | Checked with effective/group identity for access. |
| Privileged process | Traditionally, process with effective UID `0`; on Linux, capabilities refine this. | Can bypass many normal checks. |
| set-user-ID bit | Executable permission bit that changes effective UID at `exec()` time. | `passwd` is the classic example. |
| set-group-ID bit | Executable permission bit that changes effective GID at `exec()` time. | Useful for controlled group access. |
| Filesystem UID/GID | Linux-specific IDs used for filesystem permission checks. | Usually track effective IDs, so first-pass can think effective IDs. |

---

## Concept Overview

The kernel does not ask "what username string did this process type?" It checks numeric credentials attached to the process.

```text
account database
    |
    +-- /etc/passwd: user -> UID + primary GID + home + shell
    +-- /etc/group: group -> GID + extra members
    +-- /etc/shadow: password hashes, restricted
    |
    v
login creates shell process
    |
    v
process tree inherits credentials
    |
    v
kernel checks credentials during protected operations
```

The most important interview/work distinction:

| ID | Mental model |
|----|--------------|
| Real UID | who started me |
| Effective UID | who I am acting as for access checks |
| Saved UID | privilege value I may be able to regain |

For most work discussions, say "effective ID is checked." On Linux, filesystem IDs are actually used for filesystem checks, but they normally follow the effective IDs.

---

## System Context

Credentials affect more than files:

| Subsystem | How credentials matter |
|-----------|------------------------|
| VFS / files | Read/write/execute permission, ownership changes, set-user-ID execution. |
| IPC | Many IPC objects have ownership and permission checks. |
| Signals | Kernel checks whether one process may signal another. |
| `/proc` | Process status exposes real/effective/saved/filesystem IDs and groups. |
| Services | Daemons often start privileged, bind/setup resources, then drop privilege. |
| Containers | User/group mapping changes what IDs mean across host/container boundary. |

Authentication and authorization are separate:

| Question | Answered by |
|----------|-------------|
| Are you really this user? | login/PAM/password/token/authentication system |
| What may this process do now? | kernel checks process credentials and capabilities |

---

## Architecture

### Account Data on Disk

```text
/etc/passwd
    login:x:UID:GID:comment:home:shell

/etc/group
    group:x:GID:member1,member2

/etc/shadow
    login:password-hash:aging-fields...
```

Applications should use lookup APIs such as `getpwnam()` and `getpwuid()` instead of parsing only one local file by hand. TLPI notes that account data may come from mechanisms such as NIS or LDAP, and the lookup functions hide that source from ordinary applications.

### Process Credential Set

```text
process
    |
    +-- real UID / real GID
    +-- effective UID / effective GID
    +-- saved set-user-ID / saved set-group-ID
    +-- supplementary groups
    +-- Linux filesystem UID / filesystem GID
```

### Kernel Permission Check Shape

```text
operation requested
    |
    v
kernel identifies target object
    |
    v
kernel reads process credentials
    |
    v
kernel compares with ownership, permissions, capability rules
    |
    v
allow or fail with errno such as EACCES / EPERM
```

`EACCES` often means access was denied by permissions. `EPERM` often means the operation requires privilege. Always confirm with the target API's manual page.

---

## Execution Flow

### Flow 1 - Login to Normal Process

```text
user authenticates
    |
    v
login/session manager reads account and group data
    |
    v
shell starts with user credentials
    |
    v
shell launches child processes
    |
    v
children inherit credentials
```

### Flow 2 - File Permission Check

```text
process calls open("data", O_RDONLY)
    |
    v
kernel resolves pathname
    |
    v
kernel compares process credentials with file owner/group/mode
    |
    +-- allowed -> fd returned
    |
    +-- denied  -> -1, errno set
```

### Flow 3 - set-user-ID Execution

```text
ordinary user executes set-user-ID file
    |
    v
exec() loads program
    |
    v
kernel sets effective UID to executable owner UID
    |
    v
saved UID records that effective value
    |
    v
program performs narrow privileged work
```

### Flow 4 - Temporary Privilege Drop

```text
start with elevated effective UID
    |
    v
save current effective UID
    |
    v
seteuid(real_uid)          # do unprivileged work
    |
    v
seteuid(saved_effective)   # regain only when needed
```

Use this model for temporary drop/regain. Do not casually use `setuid()` if you intend to regain privilege.

### Flow 5 - Permanent Privilege Drop

```text
privileged setup complete
    |
    v
drop supplementary groups if needed
    |
    v
setgid(target_gid)
    |
    v
setuid(target_uid)
    |
    v
process should not be able to regain root
```

Order matters in real services: after dropping UID, later group changes may fail because privilege is gone.

---

## 1.3 Users, Groups, and Process Credentials

### `/etc/passwd`, `/etc/group`, `/etc/shadow`

| File | Keep in first-pass memory |
|------|---------------------------|
| `/etc/passwd` | public account metadata; not a safe place for password hashes |
| `/etc/group` | group IDs and supplementary memberships |
| `/etc/shadow` | restricted password hashes and password aging data |

Do not overfocus on every field. For system programming, the core is ID mapping and permission behavior.

### Lookup APIs

Useful functions:

```text
getpwnam(name)  -> passwd record by username
getpwuid(uid)   -> passwd record by UID
getgrnam(name)  -> group record by group name
getgrgid(gid)   -> group record by GID
```

Work notes:

- returned structures from non-`_r` functions may use static storage;
- they are not reentrant and can be overwritten by later calls;
- threaded code should prefer the reentrant `_r` variants when needed;
- "not found" vs "error" handling has portability wrinkles, so check docs carefully.

### Credential APIs

Common retrieval:

```text
getuid()   -> real UID
geteuid()  -> effective UID
getgid()   -> real GID
getegid()  -> effective GID
getgroups() -> supplementary groups
```

Common modification:

| API | First-pass use |
|-----|----------------|
| `seteuid()` | temporary effective UID change |
| `setegid()` | temporary effective GID change |
| `setuid()` | permanent UID drop in privileged code, or restricted effective change in unprivileged code |
| `setgid()` | group credential change; often done before `setuid()` in daemons |

### set-user-ID Design Rule

A set-user-ID program should be narrow:

- validate inputs;
- keep privileged code small;
- drop privilege when not needed;
- never trust environment, path, cwd, or user-controlled files blindly;
- prefer a dedicated service account over full root when possible.

This file only introduces the mechanism. Secure privileged programming is expanded later.

---

## Work-Useful Patterns

| Pattern | Why it matters |
|---------|----------------|
| Debug identity with `id`, `ps`, and `/proc/<PID>/status` | Shows actual credentials, not assumptions. |
| Check numeric ownership with `ls -ln` | Names can hide UID/GID reuse or mapping issues. |
| Use `geteuid()` when asking "what privilege am I acting with?" | Real UID alone is often the wrong answer. |
| Use `seteuid()` for temporary drop/regain | Keeps saved ID path available when appropriate. |
| Use `setuid(getuid())` only for intentional permanent drop | A privileged process may lose the ability to regain root. |
| Drop groups before dropping UID in services | After UID drop, group changes may be denied. |
| Avoid parsing account files as the only source of truth | Lookup APIs handle configured account backends better. |
| Treat set-user-ID programs as high-risk | Small bugs become privilege escalation bugs. |

---

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| Capabilities | Linux splits traditional root privilege into units such as `CAP_KILL` and `CAP_SETUID`. |
| Filesystem UID/GID | Linux-specific IDs used for filesystem checks; usually track effective IDs. |
| `setresuid()` / `getresuid()` | Linux/nonportable direct access to real, effective, and saved IDs. |
| `setfsuid()` / `setfsgid()` | Linux-specific, historical, avoid in portable application code. |
| Shadow password authentication | Requires restricted access and careful handling of plaintext passwords. |
| Duplicate numeric UIDs | Possible but unusual; ownership follows numbers, not names. |
| NIS/LDAP-style account sources | Account lookup may come from networked account sources, not only local files. |

---

## Example

### Example 1 - Inspect real and effective IDs

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("ruid=%ld euid=%ld rgid=%ld egid=%ld\n",
           (long)getuid(),
           (long)geteuid(),
           (long)getgid(),
           (long)getegid());
    return 0;
}
```

What it teaches:

- real and effective IDs can differ;
- effective IDs are central to permission reasoning.

### Example 2 - Convert current UID to a username

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    errno = 0;
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        if (errno != 0)
            perror("getpwuid");
        else
            fprintf(stderr, "no passwd entry for current UID\n");
        return 1;
    }

    printf("uid=%ld name=%s home=%s shell=%s\n",
           (long)getuid(), pw->pw_name, pw->pw_dir, pw->pw_shell);
    return 0;
}
```

What it teaches:

- the kernel stores numeric IDs;
- user-space APIs map IDs to names and metadata.

### Example 3 - Temporary effective UID drop/regain

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>

int main(void) {
    uid_t real = getuid();
    uid_t effective = geteuid();

    printf("before: ruid=%ld euid=%ld\n", (long)real, (long)effective);

    if (seteuid(real) == -1) {
        perror("seteuid drop");
        return 1;
    }

    printf("after drop: ruid=%ld euid=%ld\n",
           (long)getuid(), (long)geteuid());

    if (seteuid(effective) == -1) {
        perror("seteuid regain");
        return 1;
    }

    printf("after regain: ruid=%ld euid=%ld\n",
           (long)getuid(), (long)geteuid());
    return 0;
}
```

What it teaches:

- `seteuid()` changes the effective identity;
- regain is meaningful only if the process starts with a saved privileged identity, such as from a set-user-ID executable.

---

## Debugging

Useful commands:

```bash
# Show current shell identity and groups
id

# Show numeric IDs, useful when names are misleading
id -u
id -g
id -G

# Resolve users and groups through configured account lookup
getent passwd <user>
getent group <group>

# Inspect a process's credentials
cat /proc/<PID>/status | sed -n '/^Uid:/p;/^Gid:/p;/^Groups:/p'
ps -o pid,ppid,user,group,euid,egid,comm -p <PID>

# Inspect file owner and mode
ls -l /path/to/file
ls -ln /path/to/file

# Find set-user-ID and set-group-ID files carefully
find /path -perm /6000 -type f -ls
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Checking username, not UID | Ownership looks wrong after account changes | Use `ls -ln` and inspect numeric IDs. |
| Looking only at real UID | Program still has or lacks access unexpectedly | Check effective and saved IDs. |
| Forgetting supplementary groups | Group permission seems ignored | Check `id`, `Groups:` in `/proc/<PID>/status`. |
| Using `setuid()` for temporary drop | Program cannot regain privilege | Use `seteuid()` for temporary changes. |
| Dropping UID before GID/groups | Later group drop fails | Drop groups/GID first, then UID. |
| Trusting set-user-ID environment | Privilege escalation risk | Sanitize environment, paths, inputs. |
| UID reuse | New user appears to own old files | Track numeric IDs in operations and migrations. |

---

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Service starts as root to bind/setup resources | Do privileged setup, then drop to a dedicated user/group. |
| CLI tool gets `Permission denied` | Check effective UID/GID, supplementary groups, path traversal permissions, and target mode. |
| Embedded device uses shared log directory | Use a service group and group permissions instead of making everything root-owned. |
| set-user-ID helper needed | Keep helper tiny, validate input, drop privilege for normal work. |
| Container file ownership mismatch | Compare numeric host/container UID/GID mappings. |
| Audit suspicious binary | Check owner, mode bits, capabilities, and whether it is set-user-ID. |

---

## Interview-Relevant Questions

- Why does Linux use numeric UIDs and GIDs internally?
- What is stored in `/etc/passwd`?
- Why are `/etc/passwd` and `/etc/shadow` separate?
- How does `/etc/group` contribute to supplementary groups?
- What is the difference between real UID and effective UID?
- Which ID is usually most important for permission checks?
- What is a saved set-user-ID?
- What happens when a user executes a set-user-ID root program?
- Why is set-user-ID powerful and dangerous?
- Why is `seteuid()` usually better than `setuid()` for temporary privilege changes?
- How would you permanently drop root privilege in a daemon?
- Why should a service drop groups before dropping UID?
- How would you debug a `Permission denied` from `open()`?
- Why can UID reuse cause confusing file ownership?
- What are Linux capabilities, at a high level?
- What does `/proc/<PID>/status` show about credentials?
- Why are non-`_r` password/group lookup functions risky in threaded code?

---

## Key Takeaways

- The kernel uses numeric UIDs and GIDs, not usernames, for core identity.
- Usernames and group names are user-space mappings around numeric IDs.
- `/etc/passwd`, `/etc/group`, and `/etc/shadow` solve different account problems.
- A process carries real, effective, saved, filesystem, and supplementary credential data.
- Effective IDs are the first-pass model for access checks.
- On Linux, filesystem IDs usually follow effective IDs.
- set-user-ID changes effective UID at `exec()` time.
- Saved IDs let some programs temporarily drop and regain privilege.
- `seteuid()` is the normal temporary drop/regain tool.
- `setuid()` in privileged code is commonly used for permanent privilege drop.
- Supplementary groups are part of access decisions.
- Debug permission problems with actual process credentials and numeric file ownership.
- set-user-ID programs must be small, defensive, and treated as security-sensitive.
