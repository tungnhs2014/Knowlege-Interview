# Chapter 7 - POSIX IPC

> Topics: 7.7 POSIX IPC intro; 7.8 POSIX message queues; 7.9 POSIX semaphores; 7.10 POSIX shared memory
> Main sources: TLPI Ch51, Ch52, Ch53, Ch54; DevLinux Modules 09, 10, 11
> Related files: [IPC overview](ch07_ipc_overview.md), [Pipes/FIFOs](ch07_ipc_pipes.md), [System V IPC](ch07_ipc_sysv.md), [Interview](../../interview/ch07_ipc_interview_questions.md)

## Coverage Notes

This file covers mapped rows 7.7-7.10. System V IPC is compared for selection but remains covered in `ch07_ipc_sysv.md`; pipes/FIFOs remain in `ch07_ipc_pipes.md`.

| Coverage Matrix item | Source | Covered here | Moved/out of scope |
|----------------------|--------|--------------|--------------------|
| 7.7 POSIX IPC intro | Learning map, TLPI Ch51 | `/name`, `O_CREAT`, `O_EXCL`, permissions, open/close/unlink, System V comparison | none |
| 7.8 POSIX message queues | Learning map, TLPI Ch52, DevLinux 09 | `mq_open()`, `mq_send()`, `mq_receive()`, priority ordering, attributes, notify, timed ops, Linux `/dev/mqueue` | none |
| 7.9 POSIX semaphores | Learning map, TLPI Ch53, DevLinux 11 | named and unnamed semaphores, `sem_open()`, `sem_wait()`, `sem_post()`, `sem_init()`, `pshared` | System V `SEM_UNDO` moved to `ch07_ipc_sysv.md` |
| 7.10 POSIX shared memory | Learning map, TLPI Ch54, DevLinux 10 | `shm_open()`, `ftruncate()`, `mmap()`, `munmap()`, `shm_unlink()`, `/dev/shm` | file-backed mmap details are Chapter 5/49 context |
| POSIX lifecycle Must Cover | Chapter Must Cover | name lifetime vs handle/mapping lifetime, close/unlink cleanup, stale names | none |
| Shared memory protocol | Chapter Must Cover | external synchronization, offsets, header fields, versioning, crash recovery | also summarized in overview/System V |
| Synchronization primitives | Chapter Must Cover | POSIX semaphores and recognize-only process-shared pthread mutex alternative | deep pthread robust mutex rules stay in thread docs |
| Production debugging and Embedded constraints | Chapter Must Cover | `strace`, `/dev/mqueue`, `/dev/shm`, `/proc/sys/fs/mqueue`, namespaces, watchdog cleanup | none |

## Learning Goal

Learn POSIX IPC as the name/open/close/unlink family: message queues for priority records, semaphores for process synchronization, and shared memory objects mapped with `mmap()`.

You should be able to:

- explain how POSIX IPC differs from System V IPC;
- create and clean up named IPC objects with open/unlink semantics;
- use POSIX MQ priority and attributes correctly;
- place process-shared semaphores in shared memory when needed;
- size POSIX SHM with `ftruncate()` before mapping;
- debug stale `/dev/mqueue` and `/dev/shm` entries on Linux.

## Problem It Solves

System V IPC works, but it feels unlike the rest of UNIX: keys, integer ids, special tables, and awkward cleanup.

POSIX IPC keeps similar mechanisms but gives them a more file-like lifecycle:

```text
open/create -> use -> close or unmap -> unlink name
```

The three main mechanisms are:

- **POSIX message queues** for message-oriented data transfer with priority;
- **POSIX semaphores** for synchronization;
- **POSIX shared memory** for mapping shared pages through an fd.

The practical advantage is cleanup clarity. `mq_unlink()`, `sem_unlink()`, and `shm_unlink()` remove the name while existing users may continue through their open handles or mappings.

## Mental Model

POSIX IPC looks like opening a named kernel object, using it, then unlinking the name.

```text
POSIX name "/state"
    |
    v
open/create
    |
    +-- message queue descriptor: mq_send/mq_receive
    +-- semaphore pointer: sem_wait/sem_post
    +-- shared memory fd: ftruncate/mmap/direct access
    |
    v
close/munmap local reference
unlink name when future opens should stop
last reference frees object
```

| Mechanism | Object | Data model | Main ordering/sync rule |
|-----------|--------|------------|--------------------------|
| POSIX MQ | named queue, `mqd_t` | messages | highest priority first, FIFO within same priority |
| POSIX named semaphore | named `sem_t *` | no data | counter gates progress |
| POSIX unnamed semaphore | `sem_t` in memory | no data | `pshared != 0` requires shared memory |
| POSIX SHM | named object opened as fd | shared pages | external synchronization required |

The beginner trap is POSIX SHM size: **new POSIX SHM objects start at size 0**. `shm_open()` alone is not enough; call `ftruncate()` before `mmap()` and use.

## Mechanism

POSIX IPC objects are named and reference-counted. On Linux, many are visible through virtual filesystems, but the API name is still `/name`, not `/dev/.../name`.

### Names and Visibility

Portable POSIX IPC names look like:

```text
/object_name
```

On Linux, common implementation views are:

| POSIX object | Common Linux view | Portability note |
|--------------|-------------------|------------------|
| POSIX MQ `/jobs` | `/dev/mqueue/jobs` | mqueue filesystem must be mounted |
| POSIX SHM `/state` | `/dev/shm/state` | backed by tmpfs |
| POSIX named semaphore `/lock` | often `/dev/shm/sem.lock` | implementation detail |

Use the API name (`/jobs`, `/state`, `/lock`) in code. Use `/dev/mqueue` and `/dev/shm` for debugging Linux systems.

### Reference-Counted Unlink

```text
open/create name
    |
    v
name visible to future openers
    |
    +-- unlink name
    |      future open by same name fails or creates a new object
    |
    +-- existing handles/mappings continue
           |
           v
       last close/munmap
           |
           v
       object destroyed
```

This is why unlink-after-setup is a good pattern for temporary objects inherited by children. It is wrong when unrelated future processes still need to open the object by name.

### System V vs POSIX IPC

| Aspect | System V IPC | POSIX IPC |
|--------|--------------|-----------|
| Discovery | `key_t`, often `ftok()` | `/name` |
| API shape | `get -> use -> ctl(IPC_RMID)` | `open -> use -> close/munmap -> unlink` |
| Handle | integer id | `mqd_t`, `sem_t *`, fd |
| Cleanup model | persistent until remove/reboot | unlink name, object lives while referenced |
| Debugging | `ipcs`, `ipcrm`, `/proc/sysvipc` | `/dev/mqueue`, `/dev/shm`, `strace`, `/proc` |
| Event loop fit | mostly poor | POSIX MQ is fd-backed on Linux; POSIX SHM uses fds before mapping |

## Key APIs And Objects

POSIX IPC APIs are easier to remember when grouped by object lifecycle.

### POSIX IPC Intro

| Flag/object | Meaning | Production note |
|-------------|---------|-----------------|
| `O_CREAT` | create if missing | mode is affected by `umask` |
| `O_EXCL` | with `O_CREAT`, fail if name exists | detects stale names or competing creators |
| `O_NONBLOCK` | nonblocking MQ operations | also settable via `mq_setattr()` |
| `mq_unlink()` / `sem_unlink()` / `shm_unlink()` | remove name | existing references may continue |

### POSIX Message Queue APIs

```c
#include <mqueue.h>

mqd_t mq_open(const char *name, int oflag, ...);
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
            unsigned int msg_prio);
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                   unsigned int *msg_prio);
int mq_notify(mqd_t mqdes, const struct sigevent *sevp);
int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
int mq_setattr(mqd_t mqdes, const struct mq_attr *newattr,
               struct mq_attr *oldattr);
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
                 unsigned int msg_prio,
                 const struct timespec *abs_timeout);
ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                        unsigned int *msg_prio,
                        const struct timespec *abs_timeout);
int mq_close(mqd_t mqdes);
int mq_unlink(const char *name);
```

Queue attributes:

```c
struct mq_attr {
    long mq_flags;    /* 0 or O_NONBLOCK */
    long mq_maxmsg;   /* max messages in queue */
    long mq_msgsize;  /* max bytes per message */
    long mq_curmsgs;  /* current message count */
};
```

Must-know behavior:

- messages larger than `mq_msgsize` fail with `EMSGSIZE`;
- receive buffers smaller than `mq_msgsize` fail with `EMSGSIZE`;
- highest priority message is received first;
- same-priority messages are FIFO;
- full queue blocks sender or returns `EAGAIN` in nonblocking mode;
- empty queue blocks receiver or returns `EAGAIN` in nonblocking mode.
- `mq_getattr()` is the safe way to size receive buffers from `mq_msgsize`;
- `mq_setattr()` mainly changes `O_NONBLOCK`; queue size limits are fixed at creation;
- timed send/receive calls use absolute timeouts, so clock choice and conversion matter.

### POSIX Semaphore APIs

Named semaphore lifecycle:

```c
#include <semaphore.h>

sem_t *sem_open(const char *name, int oflag, ...);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
```

Semaphore operations:

```c
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
```

Unnamed semaphore lifecycle:

```c
int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
```

Rules:

- `sem_wait()` decrements or blocks while value is 0;
- `sem_post()` increments and wakes a waiter if any;
- `sem_wait()` can fail with `EINTR`;
- `sem_getvalue()` is a snapshot, not a synchronization decision;
- named semaphore create+initialize is atomic in `sem_open()`;
- POSIX semaphores do not provide System V `SEM_UNDO`;
- unnamed process-shared semaphores must live in shared memory.

### POSIX Shared Memory APIs

```c
#include <sys/mman.h>

int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);
```

Then use normal fd and mapping APIs:

```c
int ftruncate(int fd, off_t length);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int close(int fd);
```

Rules:

- new object size is 0;
- call `ftruncate()` before mapping or accessing intended bytes;
- use `mmap(..., MAP_SHARED, fd, 0)`;
- closing the fd after successful `mmap()` does not remove the mapping;
- `shm_unlink()` removes the name, not existing mappings;
- shared memory needs external synchronization;
- store offsets, not raw pointers, in shared memory.

## Lifecycle / Data Flow

POSIX IPC lifecycle is clear if you separate name lifetime from handle/mapping lifetime.

### POSIX MQ Producer/Consumer

```text
creator
  mq_open("/jobs", O_CREAT | O_EXCL | O_RDONLY, mode, attr)

producer
  mq_open("/jobs", O_WRONLY)
  mq_send(message, priority)

consumer
  mq_receive()
      -> oldest message with highest priority

cleanup
  mq_close()
  mq_unlink("/jobs")
```

Use explicit `mq_attr` in production. Defaults vary and may be too small or too large for embedded RAM budgets.

### `mq_notify()` Flow

`mq_notify()` is one-shot notification for an empty-to-nonempty transition.

```text
consumer opens queue O_NONBLOCK
consumer registers mq_notify()
queue transitions empty -> nonempty
kernel delivers signal/thread notification
registration is removed
consumer re-registers first
consumer drains queue until EAGAIN
```

Re-register before draining. Otherwise a new message can arrive after the drain and before the new registration, leaving no notification armed.

### Named Semaphore Lock

```text
process A                         process B
  sem_open("/lock", O_CREAT, 0600, 1)
  sem_wait()                       sem_wait() blocks while value is 0
  critical section
  sem_post()  ------------------>  wakes and continues
  sem_close()

owner/manager:
  sem_unlink("/lock")
```

Do not destroy or unlink as a substitute for releasing a lock. `sem_post()` is the release operation.

### Unnamed Semaphore in POSIX SHM

```text
creator
  fd = shm_open("/state", O_CREAT | O_EXCL | O_RDWR, 0600)
  ftruncate(fd, sizeof(struct shared))
  shared = mmap(... MAP_SHARED ...)
  sem_init(&shared->mutex, 1, 1)

processes
  sem_wait(&shared->mutex)
  mutate shared fields
  sem_post(&shared->mutex)

cleanup
  sem_destroy()
  munmap()
  shm_unlink()
```

If `sem_t` is in private heap memory, each process has its own semaphore. `pshared != 0` only works when the object is in shared memory.

### POSIX SHM Temporary Inherited State

```text
creator
  fd = shm_open("/tmp_state", O_CREAT | O_EXCL | O_RDWR, 0600)
  ftruncate(fd, size)
  ptr = mmap(..., fd, 0)
  shm_unlink("/tmp_state")
  close(fd)
  fork workers

workers inherit mapping
last munmap/process exit frees object
```

This is clean for parent/child temporary state. It is not correct when unrelated future processes must open the object by name.

### Compact Code Patterns

These are the practical shapes to keep in muscle memory.

POSIX MQ should be created with explicit attributes and cleaned up with `mq_unlink()`.

```c
struct mq_attr attr = {
    .mq_maxmsg = 8,
    .mq_msgsize = 128,
};

mqd_t mq = mq_open("/jobs", O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
if (mq == (mqd_t)-1)
    die("mq_open");

mq_send(mq, "high", 5, 10);
mq_send(mq, "low", 4, 1);

char buf[128];
unsigned int prio;
mq_receive(mq, buf, sizeof(buf), &prio);  /* receives priority 10 first */

mq_close(mq);
mq_unlink("/jobs");
```

POSIX SHM with an unnamed process-shared semaphore must size the object before mapping and place `sem_t` inside the shared mapping.

```c
struct shared_state {
    sem_t lock;
    int value;
};

int fd = shm_open("/state", O_CREAT | O_EXCL | O_RDWR, 0600);
if (fd == -1)
    die("shm_open");
if (ftruncate(fd, sizeof(struct shared_state)) == -1)
    die("ftruncate");

struct shared_state *state = mmap(NULL, sizeof(*state),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
close(fd);

sem_init(&state->lock, 1, 1);
sem_wait(&state->lock);
state->value++;
sem_post(&state->lock);
```

## Production Bugs And Debugging

POSIX IPC bugs often show up as stale names, blocked queues, one-shot notifications, and SHM sizing mistakes.

| Symptom | Likely cause | Evidence | Fix pattern |
|---------|--------------|----------|-------------|
| `mq_open(O_CREAT | O_EXCL)` fails with `EEXIST` | stale queue or live owner | `ls /dev/mqueue`, app owner state | verify owner, then safe `mq_unlink()` |
| `mq_send()` blocks or returns `EAGAIN` | queue full | `cat /dev/mqueue/<name>`, receiver health | timeout/backpressure; tune attr/limits carefully |
| `mq_receive()` fails with `EMSGSIZE` | receive buffer smaller than `mq_msgsize` | `mq_getattr()` or `/dev/mqueue` | allocate based on queue attributes |
| Messages seem out of order | priority ordering misunderstood | logged priorities | remember highest priority first |
| `mq_notify()` fires once | one-shot registration | code/trace shows no re-register | re-register before draining |
| Named semaphore persists after restart | missing `sem_unlink()` | `/dev/shm/sem.*` | owner cleanup and startup stale check |
| Processes fail to synchronize | unnamed `sem_t` stored in private memory | maps/code review | put `sem_t` in `MAP_SHARED` memory |
| `sem_wait()` returns `EINTR` | signal interrupted wait | logs/strace | retry or handle cancellation explicitly |
| SHM access gets `SIGBUS` | object size 0 or too small | `ls -l /dev/shm`, `strace ftruncate,mmap` | `ftruncate()` before mapping/use |
| Shared state corrupt after watchdog restart | stale mapping/state reused | shared header magic/version/state | add header protocol and repair path |
| Peer cannot open object | name unlinked too early | trace `mq_open`/`shm_open` `ENOENT` | unlink only after all needed openers have handles |
| `/dev/shm` full | SHM leaks or tmpfs limit | `df -h /dev/shm`, `ls -lh /dev/shm` | unlink stale objects; adjust design/limits |

Useful commands:

```bash
# POSIX MQ on Linux
ls -la /dev/mqueue
cat /dev/mqueue/<name>
cat /proc/mounts | grep mqueue
cat /proc/sys/fs/mqueue/msg_max
cat /proc/sys/fs/mqueue/msgsize_max
cat /proc/sys/fs/mqueue/queues_max

# POSIX SHM and named semaphores on Linux
ls -la /dev/shm
df -h /dev/shm
lsof +D /dev/shm
rm /dev/shm/<shm_name>
rm /dev/shm/sem.<sem_name>
cat /proc/<pid>/maps
pmap <pid>

# Trace common paths
strace -f -e trace=mq_open,mq_timedsend,mq_timedreceive,mq_notify,mq_unlink ./program
strace -f -e trace=openat,unlink,mmap,munmap,ftruncate,close ./program
```

On embedded systems, verify namespace and mount setup first. A service in a different namespace may not see the same `/dev/mqueue` or `/dev/shm` view as your shell.

## DevLinux Practice Bridge

Use DevLinux Modules 09, 10, and 11 to connect POSIX IPC APIs to runnable examples.

| DevLinux module | Practice focus | What to verify against production rules |
|-----------------|----------------|-----------------------------------------|
| 09 Message Queues | POSIX MQ creation, send/receive, notify, timed operations | attributes are fixed at creation; receive buffer must match `mq_msgsize`; priority changes ordering |
| 10 Shared Memory | POSIX `shm_open()` and `mmap()` examples | new objects start size 0; `ftruncate()` must happen before use; mapping needs synchronization |
| 11 Semaphores | named and unnamed POSIX semaphore examples | named semaphores need unlink ownership; unnamed process-shared semaphores must live in shared memory |

Good failure drills:

- create a POSIX MQ with small `mq_maxmsg`, fill it, and observe blocking or `EAGAIN`;
- register `mq_notify()` once and confirm it is one-shot;
- omit `ftruncate()` before POSIX SHM access and explain the `SIGBUS` risk;
- place an unnamed `sem_t` in private memory and explain why other processes do not synchronize with it;
- unlink a POSIX object while a process still has it open and observe that existing references continue.

## Embedded Constraints

POSIX IPC is often nicer for new Linux code, but embedded deployment still needs explicit resource and lifecycle policy.

- Mount and namespace setup matters: `/dev/mqueue` and `/dev/shm` may be absent, private, read-only, or size-limited.
- Use explicit `mq_attr`; defaults may consume too much RAM or be too small for real traffic.
- Track ownership of `mq_unlink()`, `sem_unlink()`, and `shm_unlink()` in the service design, not as an afterthought.
- Check `RLIMIT_MSGQUEUE`, `/proc/sys/fs/mqueue/*`, and `/dev/shm` capacity before increasing queue or shared-memory sizes.
- Use timeout or nonblocking paths where a dead peer would otherwise stall boot or watchdog recovery.
- Treat stale `/dev/shm` and `/dev/mqueue` entries like possibly-live state until service ownership proves otherwise.
- Use `lsof +D /dev/shm` sparingly on capable targets to identify live users before cleanup.
- For shared memory, include `magic`, `version`, `size`, `state`, and recovery fields so a restarted process can reject stale layouts.

## Work Checklist

Use this when designing or reviewing POSIX IPC.

- Use `/name` API names; keep Linux `/dev/...` paths for debugging.
- Use `O_CREAT | O_EXCL` for owner-created persistent objects.
- Treat `EEXIST` as a stale-or-live investigation, not an automatic delete.
- Define who owns `mq_unlink()`, `sem_unlink()`, or `shm_unlink()`.
- Use explicit `mq_attr` for max messages and message size.
- Add timeout/backpressure policy for full or empty queues.
- Allocate MQ receive buffers using `mq_msgsize`.
- Re-register `mq_notify()` before draining the queue, or use blocking receiver/epoll on Linux.
- Place unnamed process-shared `sem_t` objects inside shared memory.
- Handle `sem_wait()` `EINTR`.
- Call `ftruncate()` before `mmap()` for POSIX SHM.
- Close SHM fd after successful mapping if no longer needed, but keep/unlink name according to peer needs.
- Put `magic`, `version`, `size`, `state`, and offsets in shared-memory headers.
- Include `/dev/mqueue`, `/dev/shm`, and `/proc/sys/fs/mqueue` checks in runbooks.

## Recognize / Advanced

These details are useful when reading production POSIX IPC code.

| Topic | Recognize this |
|-------|----------------|
| POSIX MQ fd on Linux | `mqd_t` works with `poll()`/`epoll()` on Linux, but POSIX does not require it |
| `mq_timedsend()` / `mq_timedreceive()` | timeout variants using absolute timeouts |
| `RLIMIT_MSGQUEUE` | per-real-UID POSIX MQ memory limit on Linux |
| `/proc/sys/fs/mqueue/*` | Linux system limits for POSIX MQ |
| Older `-lrt` builds | Some older glibc/toolchains require linking with `-lrt` |
| ACLs on `/dev/shm` | Linux may support ACLs for POSIX SHM/named semaphore objects |
| `O_TRUNC` with read-only SHM | Linux behavior exists; POSIX leaves some cases unspecified |
| Robust process-shared pthread mutex | Alternative for shared-memory locking when supported and designed carefully |

## Interview Readiness

You should be ready to compare POSIX IPC with System V IPC and explain the lifecycle rules.

Practice answering:

- What problem does POSIX IPC improve compared with System V IPC?
- What does open/close/unlink mean for named IPC?
- Why can an unlinked POSIX IPC object still be usable?
- How are POSIX MQ messages ordered?
- Why must `mq_receive()` use a big enough buffer?
- Why is `mq_notify()` one-shot?
- What is the difference between named and unnamed semaphores?
- Where must an unnamed process-shared semaphore live?
- Why must POSIX SHM be `ftruncate()`d before use?
- How would you debug stale objects in `/dev/mqueue` or `/dev/shm`?

Interview anchor answer:

```text
POSIX IPC keeps the same broad tools as System V IPC but uses names and open/unlink style lifetime.
Message queues preserve records and prioritize them, semaphores synchronize without data transfer,
and POSIX shared memory is shm_open plus ftruncate plus mmap.
The big production rules are explicit attributes, timeouts, unlink ownership, and SHM synchronization.
```

## Final Coverage Check

- [x] Mapped rows 7.7, 7.8, 7.9, and 7.10 are covered directly.
- [x] POSIX Must Cover items are present: names, open/close/unlink lifecycle, message queues, semaphores, POSIX shared memory via `shm_open()` plus `mmap()`.
- [x] Cross-family Must Cover items are represented where relevant: lifecycle, blocking, cleanup, shared-memory synchronization, production debugging, namespaces, and Embedded restart behavior.
- [x] Existing useful examples and DevLinux practice drills were preserved; no mapped POSIX IPC topic is intentionally out of scope.
