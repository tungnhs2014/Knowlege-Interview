# Chapter 7 - System V IPC

> Topics: 7.3 System V IPC intro; 7.4 System V message queues; 7.5 System V semaphores; 7.6 System V shared memory
> Main sources: TLPI Ch45, Ch46, Ch47, Ch48; DevLinux Modules 09, 10, 11
> Related files: [IPC overview](ch07_ipc_overview.md), [Pipes/FIFOs](ch07_ipc_pipes.md), [POSIX IPC](ch07_ipc_posix.md), [Interview](../../interview/ch07_ipc_interview_questions.md)

## Coverage Notes

This file preserves the System V split inside one knowledge file because the learning map routes rows 7.3-7.6 here. POSIX equivalents are compared only where that helps selection; their full lifecycle is moved to `ch07_ipc_posix.md`.

| Coverage Matrix item | Source | Covered here | Moved/out of scope |
|----------------------|--------|--------------|--------------------|
| 7.3 System V IPC intro | Learning map, TLPI Ch45 | `ftok()`, `IPC_PRIVATE`, key/id distinction, permissions, `ipcs`, `ipcrm`, `/proc/sysvipc` | none |
| 7.4 System V message queues | Learning map, TLPI Ch46, DevLinux 09 | `msgget()`, `msgsnd()`, `msgrcv()`, `msgctl()`, `mtype`, queue limits, stale messages | none |
| 7.5 System V semaphores | Learning map, TLPI Ch47, DevLinux 11 | `semget()`, `semop()`, `semctl()`, semaphore sets, atomic multi-op, `sem_otime`, `SEM_UNDO` | POSIX semaphores moved to `ch07_ipc_posix.md` |
| 7.6 System V shared memory | Learning map, TLPI Ch48, DevLinux 10 | `shmget()`, `shmat()`, `shmdt()`, `shmctl()`, attach/detach/removal, offsets, sync protocol | POSIX SHM moved to `ch07_ipc_posix.md` |
| Chapter Must Cover System V | Learning map | keys, ids, permissions, stale objects, init races, debugging and cleanup | none |
| Shared memory protocol | Chapter Must Cover | magic/version/size/state, offsets not pointers, semaphore protection, crash recovery | also summarized in overview/POSIX |
| Synchronization primitives | Chapter Must Cover | System V semaphore counts, lock ownership, deadlock/starvation, `SEM_UNDO` limits | process-shared pthread mutex/condvar is recognize-only in overview/POSIX |
| Embedded constraints | Chapter Must Cover | watchdog restarts, conservative `ipcrm`, kernel limits, namespaces, owner metadata | none |

## Learning Goal

Learn System V IPC as the older key/id-based IPC family: message queues for records, semaphore sets for synchronization, and shared memory segments for fast shared state.

You should be able to:

- explain `key_t -> xxxget() -> integer id -> xxxctl(IPC_RMID)`;
- inspect and clean up objects with `ipcs`, `ipcrm`, and `/proc/sysvipc`;
- use `mtype`, semaphore operations, and `shmat()` correctly;
- avoid stale objects, `ftok()` surprises, semaphore initialization races, and shared-memory corruption;
- reason about legacy and embedded systems that still depend on System V IPC.

## Problem It Solves

System V IPC gives unrelated local processes persistent kernel objects they can find through a key.

It provides three mechanisms:

- **message queues** for message-oriented data transfer;
- **semaphore sets** for process synchronization;
- **shared memory segments** for fast shared pages.

System V IPC is not usually the first choice for new Linux services because POSIX IPC, pipes, and sockets fit the fd/open/unlink model better. It remains important because many deployed databases, industrial systems, embedded stacks, and legacy UNIX applications still use it.

The production problem is lifetime. If a process crashes before cleanup, the queue, semaphore set, or shared memory segment may survive and confuse the next run.

## Mental Model

All three System V IPC mechanisms share one shape.

```text
key_t key
    |
    v
xxxget(key, flags | permissions)
    |
    v
integer IPC identifier
    |
    +-- message queue: msgsnd(), msgrcv()
    +-- semaphore set: semop()
    +-- shared memory: shmat(), direct memory access, shmdt()
    |
    v
xxxctl(id, IPC_RMID, ...)
```

| Mechanism | Carries data? | Preserves records? | Synchronizes? | Main handle |
|-----------|---------------|--------------------|---------------|-------------|
| System V message queue | yes | yes | blocking send/receive only | `msqid` |
| System V semaphore set | no | no | yes | `semid` |
| System V shared memory | yes, through shared pages | application-defined | no, needs external sync | `shmid` plus attached pointer |

System V identifiers are **not file descriptors**. You cannot pass them to `read()`, `write()`, `select()`, `poll()`, or `epoll()`.

## Mechanism

System V IPC objects live in kernel IPC tables. A process finds an object by key, receives an integer identifier, and then uses mechanism-specific syscalls.

### Keys and Identifiers

`key_t` is discovery. The returned id is usage.

```c
key_t key = ftok("/run/myapp/sysv.key", 'Q');
int msqid = msgget(key, IPC_CREAT | 0600);
```

| Key source | Use when | Traps |
|------------|----------|-------|
| `ftok(path, proj_id)` | unrelated processes agree on a stable key file | collisions possible; path must exist; recreated file may produce a different key |
| hard-coded key | tightly controlled legacy deployment | collision with other software; poor isolation |
| `IPC_PRIVATE` | parent creates object before `fork()` or passes id explicitly | name is misleading: it creates a new object, not a private permission mode |

The `xxxget()` flags control create/open behavior:

| Flag | Meaning |
|------|---------|
| `IPC_CREAT` | create object if missing |
| `IPC_EXCL` | with `IPC_CREAT`, fail with `EEXIST` if object exists |
| permission bits | read/write access checks; `umask` is not applied |

### Persistence and Removal

System V IPC is kernel-persistent until explicit removal or reboot.

| Object | Remove call | Removal behavior |
|--------|-------------|------------------|
| Message queue | `msgctl(msqid, IPC_RMID, NULL)` | removed immediately; queued messages lost; waiters wake with `EIDRM` |
| Semaphore set | `semctl(semid, 0, IPC_RMID)` | removed immediately; waiters wake with `EIDRM` |
| Shared memory | `shmctl(shmid, IPC_RMID, NULL)` | marked for deletion; removed after last detach |

System V shared memory removal is closer to unlinking an open file. Message queues and semaphore sets are removed immediately.

### Permissions and Ownership

System V objects store metadata in `ipc_perm`.

- Read/write permission bits matter; execute bits do not carry useful IPC meaning.
- `umask` is not applied at creation.
- `IPC_SET` and `IPC_RMID` require privilege or matching owner/creator effective UID.
- Debugging stale objects requires checking owner, mode, creation time, and last operation time.

## Key APIs And Objects

System V APIs look dense, but each mechanism follows get/use/control.

### Common Intro APIs

| API/object | Purpose | Production note |
|------------|---------|-----------------|
| `ftok(path, proj_id)` | derive a `key_t` from a file identity and project id | collisions and path recreation are real |
| `IPC_PRIVATE` | force creation of a new object | good before `fork()` |
| `IPC_CREAT | IPC_EXCL` | exclusive create | useful for stale-object detection |
| `IPC_RMID` | remove or mark object for removal | must be part of cleanup design |
| `ipcs` / `ipcrm` | inspect/remove objects | mandatory operational tools |

### Message Queue APIs

```c
#include <sys/msg.h>

int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t maxmsgsz,
               long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

Message shape:

```c
struct mymsg {
    long mtype;      /* must be > 0 */
    char body[128];  /* application payload */
};
```

`msgsz` and `maxmsgsz` are the payload size only. They exclude `mtype`.

| `msgtyp` in `msgrcv()` | Selection |
|------------------------|-----------|
| `0` | first message in queue |
| `> 0` | first message with exactly that type |
| `< 0` | first message with the lowest type <= `abs(msgtyp)` |

Useful flags:

- `IPC_NOWAIT`: fail instead of blocking;
- `MSG_NOERROR`: truncate oversized message instead of failing;
- `MSG_EXCEPT`: Linux-specific receive-not-equal behavior.

### Semaphore APIs

```c
#include <sys/sem.h>

int semget(key_t key, int nsems, int semflg);
int semop(int semid, struct sembuf *sops, unsigned int nsops);
int semctl(int semid, int semnum, int cmd, ...);
```

System V semaphores are allocated in sets. A one-semaphore lock is still a set of one.

```c
struct sembuf {
    unsigned short sem_num;
    short sem_op;
    short sem_flg;
};
```

| `sem_op` | Meaning |
|----------|---------|
| positive | add to semaphore value; release/count resource |
| zero | wait until value becomes zero |
| negative | subtract; block if result would be below zero |

`semop()` can apply multiple operations atomically: either all operations are applied, or none are.

Many `semctl()` operations need `union semun`, which applications commonly define themselves on glibc:

```c
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
```

### Shared Memory APIs

```c
#include <sys/shm.h>

int shmget(key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
```

Important rules:

- new segments are initialized to zero;
- prefer `shmat(shmid, NULL, 0)` and let the kernel choose the address;
- `SHM_RDONLY` attaches read-only;
- `fork()` inherits attached segments;
- `exec()` detaches attached segments;
- process exit detaches attached segments;
- store offsets or indexes, not raw process-local pointers.

## Lifecycle / Data Flow

System V code should make creation, initialization, use, and cleanup explicit. The kernel will not guess whether a surviving object is stale or intentional.

### Create/Open with Stale-Object Handling

```text
server starts
  key = ftok(stable_path, project_id)
  id = xxxget(key, IPC_CREAT | IPC_EXCL | perms)
      |
      +-- success:
      |     initialize clean object
      |
      +-- EEXIST:
            object may be live or stale
            prove whether owner is still running
            inspect object metadata/state
            remove only if safe
            recreate
```

Pair stale cleanup with a singleton strategy such as a service manager, lock file, pidfd, or parent-owned startup sequence.

### Message Queue Request/Reply

```text
client
  creates private reply queue or chooses reply mtype
  sends request to server queue

server
  msgrcv(server_queue, request_type)
  processes request
  msgsnd(reply_queue or reply_type)

cleanup
  msgctl(queue, IPC_RMID)
```

Common patterns:

- `mtype` as command class;
- `mtype` as client pid for replies;
- private client queue created with `IPC_PRIVATE` and included in the request.

### Semaphore Initialization

System V semaphore creation and initialization are separate, so unrelated peers can race.

```text
creator:
  semget(key, nsems, IPC_CREAT | IPC_EXCL | perms)
  semctl(SETVAL or SETALL)
  perform a no-op semop() so sem_otime becomes nonzero

peer:
  semget(key, nsems, 0)
  poll semctl(IPC_STAT) until sem_otime != 0
```

This `sem_otime` protocol is unnecessary when one known parent initializes the set before forking children.

### Shared Memory with Semaphore Protection

```text
creator
  semget() and initialize semaphore
  shmget()
  shmat()
  initialize shared header: magic, version, size, state

process A                         process B
  semop(-1) acquire                semop(-1) acquire
  write shared fields              read shared fields
  semop(+1) release                semop(+1) release

cleanup
  shmdt()
  shmctl(IPC_RMID)
  semctl(IPC_RMID)
```

A ready flag alone is not synchronization unless the protocol also defines memory ordering, ownership, and crash recovery.

### `SEM_UNDO`

`SEM_UNDO` asks the kernel to undo a process's semaphore adjustment when that process terminates.

Know the limits:

- undo happens on normal or abnormal process termination;
- `fork()` does not inherit undo adjustments;
- undo adjustments are preserved across `exec()`;
- `SETVAL` and `SETALL` clear undo entries for modified semaphores;
- `SEM_UNDO` can release a count but cannot repair corrupted shared memory.

Use it as a safety layer, not as complete crash recovery.

### Compact Code Patterns

These snippets preserve the important shape of the old examples without hiding the lifecycle rules.

Message queue records always start with a positive `long mtype`; send/receive sizes exclude that field.

```c
struct message {
    long mtype;
    char text[64];
};

int msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);

struct message msg = {
    .mtype = 2,
    .text = "work item",
};

if (msgsnd(msqid, &msg, sizeof(msg.text), 0) == -1)
    die("msgsnd");

if (msgrcv(msqid, &msg, sizeof(msg.text), 2, 0) == -1)
    die("msgrcv");

msgctl(msqid, IPC_RMID, NULL);
```

Shared memory needs a synchronization object beside it. `SEM_UNDO` can reduce abandoned-lock damage, but the shared data still needs validation after a crash.

```c
static void sem_change(int semid, short delta)
{
    struct sembuf op = {
        .sem_num = 0,
        .sem_op = delta,
        .sem_flg = SEM_UNDO,
    };

    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR)
            die("semop");
    }
}

sem_change(semid, -1);
shared->counter++;
sem_change(semid, +1);
```

## Production Bugs And Debugging

System V bugs often survive process exit, so always inspect kernel IPC tables before restarting blindly.

| Symptom | Likely cause | Evidence | Fix pattern |
|---------|--------------|----------|-------------|
| Startup fails with `EEXIST` | stale object or another owner running | `ipcs`, timestamps, owner pid in app state | verify owner, then safe `ipcrm`/recreate |
| `xxxget()` fails with `ENOENT` | missing object and no `IPC_CREAT` | trace flags, key value | create intentionally or fix startup order |
| Peers use different objects | `ftok()` path missing/recreated or collision | compare key file `stat`, `ipcs` keys | stable key file; avoid fragile `ftok()` assumptions |
| `msgsnd()` blocks | queue full | `ipcs -q`, used bytes, receiver health | timeout/backpressure; increase limits only with design |
| `msgrcv()` blocks | wrong `msgtyp` or no producer | protocol logs, queue contents | fix type contract; add timeout/cancel path |
| Old messages processed after restart | queue survived crash | `ipcs -q`, message count | purge/recreate queue during safe startup |
| `semop()` blocks unexpectedly | wrong initial value or unreleased semaphore | `ipcs -s -i`, `sem_otime`, app logs | initialize before use; audit release paths |
| Peer sees uninitialized semaphore | create/init race | `sem_otime == 0` | parent init before fork or `sem_otime` protocol |
| Waiter wakes with `EIDRM` | semaphore/queue removed while waiting | trace return code | handle as shutdown/restart event |
| Shared memory data corrupt | missing lock or partial update | invariants fail, logs, maps | protect invariants, not individual fields only |
| Peer crashes using shared pointer | raw pointer stored in SHM | different attach addresses | store offsets from shared base |
| Embedded restart reuses bad state | watchdog restarted only one process | `ipcs`, shared header magic/state | versioned header and startup repair path |

Useful commands:

```bash
ipcs
ipcs -q
ipcs -s
ipcs -m
ipcs -l

ipcs -q -i <msqid>
ipcs -s -i <semid>
ipcs -m -i <shmid>

ipcrm -q <msqid>
ipcrm -s <semid>
ipcrm -m <shmid>

cat /proc/sysvipc/msg
cat /proc/sysvipc/sem
cat /proc/sysvipc/shm

cat /proc/sys/kernel/msgmax
cat /proc/sys/kernel/msgmnb
cat /proc/sys/kernel/msgmni
cat /proc/sys/kernel/sem
cat /proc/sys/kernel/shmmni
cat /proc/sys/kernel/shmmax
cat /proc/sys/kernel/shmall

strace -f -e trace=msgget,msgsnd,msgrcv,msgctl ./program
strace -f -e trace=semget,semop,semctl ./program
strace -f -e trace=shmget,shmat,shmdt,shmctl ./program
```

On small targets, `ipcs`, `/proc/sysvipc/*`, and app-level object names are often more practical than full tracing.

## DevLinux Practice Bridge

Use DevLinux Modules 09, 10, and 11 as runnable practice after the System V lifecycle is clear.

| DevLinux module | Practice focus | What to verify against TLPI semantics |
|-----------------|----------------|---------------------------------------|
| 09 Message Queues | System V send/receive examples | `mtype` is separate from payload size; wrong type can block forever; queues survive process exit |
| 10 Shared Memory | System V segment creation and attach examples | attaching at different addresses requires offsets, not raw pointers; synchronization is external |
| 11 Semaphores | System V semaphore examples | creation and initialization are separate; `semop()` can block or apply multiple operations atomically |

Good failure drills:

- run a message queue example, kill it before cleanup, and inspect the stale queue with `ipcs -q`;
- start two unrelated semaphore users at once and reason about who initialized the set;
- store a raw pointer in shared memory, attach from another process, and explain why it is invalid;
- remove a queue or semaphore while a process waits and handle `EIDRM` as a restart/shutdown event.

## Embedded Constraints

System V IPC is common in long-lived and legacy embedded deployments, so operational discipline matters as much as API syntax.

- Do not blindly `ipcrm` every object at boot; verify owner, boot instance, service state, and namespace first.
- Store enough owner metadata in application state: pid, boot id if available, magic, version, size, and clean/dirty state.
- Use `IPC_CREAT | IPC_EXCL` for owner-created objects so startup can distinguish clean creation from stale/live state.
- Treat `ftok()` paths as deployment artifacts. Recreated files can change keys, and collisions are possible.
- Budget System V limits (`msgmni`, `msgmnb`, `sem`, `shmmax`, `shmall`) for worst-case restart storms and slow consumers.
- Use timeouts, health checks, or supervisor policy around blocking `msgsnd()`, `msgrcv()`, and `semop()` paths.
- Remember that `SEM_UNDO` can repair a semaphore count, not a half-written shared-memory data structure.

## Work Checklist

Use this when designing or reviewing System V IPC.

- Decide whether System V is required by legacy/platform constraints; prefer fd/POSIX designs for new code when appropriate.
- Use `IPC_PRIVATE` when a parent can create before `fork()` and pass ids naturally.
- If using `ftok()`, create a stable key file and document project ids.
- Use `IPC_CREAT | IPC_EXCL` for owner-created persistent objects.
- Treat `EEXIST` as "maybe live, maybe stale", not as automatic permission to delete.
- Store owner pid, boot id, magic, version, and state in app-controlled metadata where possible.
- Remove queues/semaphore sets explicitly with `IPC_RMID`.
- Mark shared memory for removal when the owner no longer wants new users.
- Add timeout/cancel logic around blocking queue and semaphore operations.
- Initialize semaphores before peers can use them; use `sem_otime` for unrelated concurrent starters.
- Use `SEM_UNDO` only after deciding what data recovery still needs to happen.
- Protect every shared-memory invariant with a synchronization protocol.
- Store offsets, lengths, indexes, and fixed-width fields in shared memory; avoid raw pointers.
- Include `ipcs`/`ipcrm` cleanup guidance in service runbooks and embedded init scripts.

## Recognize / Advanced

These details help when reading mature System V code.

| Topic | Recognize this |
|-------|----------------|
| `MSG_NOERROR` | Allows truncating oversized received messages; dangerous if protocol needs full records |
| `MSG_EXCEPT` | Linux-specific receive-not-matching type |
| `semtimedop()` | Linux timeout variant of `semop()` |
| `SEM_UNDO` limits | Kernel undo lists are finite; behavior varies in impossible-undo cases |
| Identifier reuse | ids contain sequence information on Linux; never depend on exact numeric values |
| `SHM_LOCK` / `SHM_UNLOCK` | Lock SHM pages in RAM subject to privilege/resource limits |
| Huge-page SHM | Linux performance tuning for large segments |
| IPC namespaces | Containers may have separate System V IPC tables |
| `/proc/sys/kernel/*` limits | System-wide limits can explain failures under load |

## Interview Readiness

You should be able to explain System V IPC through object lifetime and failure modes.

Practice answering:

- What are the three System V IPC mechanisms?
- What is the difference between a key and an identifier?
- Why is a System V IPC id not a file descriptor?
- What are the pitfalls of `ftok()`?
- How do `IPC_CREAT`, `IPC_EXCL`, and `IPC_RMID` affect lifecycle?
- How does `msgrcv()` choose messages by `msgtyp`?
- Why do System V semaphores have an initialization race?
- What does `SEM_UNDO` fix, and what does it not fix?
- Why does shared memory need synchronization?
- How would you debug stale IPC after an embedded watchdog restart?

Interview anchor answer:

```text
System V IPC uses a key to find a persistent kernel object and an integer id to use it.
Message queues transfer typed records, semaphores synchronize, and shared memory maps pages.
The biggest production risks are stale objects, fragile keys, semaphore init races,
blocking queues, and shared-memory corruption without a protocol.
```

## Final Coverage Check

- [x] Mapped rows 7.3, 7.4, 7.5, and 7.6 are covered directly.
- [x] System V Must Cover items are present: keys, ids, permissions, `ipcs`/`ipcrm`, message queues, semaphores, shared memory, stale objects, and init races.
- [x] Cross-family Must Cover items are represented where relevant: lifecycle, cleanup, blocking, shared-memory synchronization, production debugging, and Embedded restart behavior.
- [x] Existing useful examples and DevLinux practice drills were preserved; no mapped System V topic is intentionally out of scope.
