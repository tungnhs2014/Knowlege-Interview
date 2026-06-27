# Chapter 7 - IPC Overview

> Topics: 7.1 IPC overview, taxonomy, data transfer vs synchronization, pipe vs socket vs shared memory vs message queue
> Main sources: TLPI Ch43; DevLinux Modules 08, 09, 10, 11
> Related files: [Pipes/FIFOs](ch07_ipc_pipes.md), [System V IPC](ch07_ipc_sysv.md), [POSIX IPC](ch07_ipc_posix.md), [Interview](../../interview/ch07_ipc_interview_questions.md)

## Coverage Notes

This file is the chapter-level map. It covers row 7.1 and points each mechanism family to the dedicated file where implementation detail lives.

| Coverage Matrix item | Source | Covered here | Detail target |
|----------------------|--------|--------------|---------------|
| 7.1 IPC taxonomy | Learning map, TLPI Ch43, DevLinux 08-11 | communication vs synchronization, data-transfer vs shared memory | this file |
| Byte stream vs message IPC | Learning map, TLPI Ch43 | boundaries, framing, ordering, backpressure, blocking | this file, pipes, SysV/POSIX MQ files |
| Peer relationship and lifetime | Learning map, TLPI Ch43 | related/unrelated peers, anonymous/named objects, fd inheritance, persistence | this file plus family files |
| Pipes/FIFOs | Learning map row 7.2 | overview and selection rules only | moved to `ch07_ipc_pipes.md` |
| System V IPC | Learning map rows 7.3-7.6 | key/id model, persistence, debug surface | moved to `ch07_ipc_sysv.md` |
| POSIX IPC | Learning map rows 7.7-7.10 | name/open/unlink model, debug surface | moved to `ch07_ipc_posix.md` |
| Shared memory protocol | Chapter Must Cover | header, offsets, synchronization, versioning, recovery | this file plus SysV/POSIX SHM sections |
| Synchronization primitives | Chapter Must Cover | semaphore/process-shared lock selection | this file plus SysV/POSIX semaphore sections |
| Production debugging and Embedded constraints | Chapter Must Cover | tools, stale cleanup, namespaces, watchdog restart recovery | this file plus family files |

## Learning Goal

Understand how isolated Linux processes cooperate, and learn to choose an IPC mechanism by data shape, peer relationship, lifetime, and failure behavior.

After this chapter, you should be able to:

- explain why IPC exists even though processes are isolated;
- distinguish byte streams, messages, shared memory, and synchronization objects;
- choose between pipe/FIFO, message queue, shared memory, semaphore, and socket;
- predict common hang, stale-object, and corruption bugs;
- know which Linux tools expose IPC state in production.

## Problem It Solves

Linux isolates processes on purpose. Each process has its own virtual address space, file descriptor table, credentials, signal state, and scheduler-visible execution context.

Real systems still need cooperation:

- a shell connects `ls` to `wc`;
- a service supervisor captures child output;
- a daemon sends work to helper processes;
- a camera or sensor pipeline shares large frames;
- multiple processes coordinate access to shared state;
- an embedded watchdog restarts one process while another still owns IPC state.

IPC is the controlled way to cross the isolation boundary without making every process share everything.

## Mental Model

Think of IPC as choosing what crosses the process boundary: bytes, records, memory pages, or permission to proceed.

```text
Process A                 Kernel / shared object                 Process B
---------                 ----------------------                 ---------
write bytes       --->    pipe/socket buffer       --->          read bytes
send message      --->    message queue            --->          receive record
store to mapping  --->    shared physical pages     <---         load from mapping
sem_post/wait     --->    semaphore counter         --->         proceed/block
```

| Need | Good first choice | Why |
|------|-------------------|-----|
| Parent sends a stream to child | Pipe | Created before `fork()`, inherited as fds |
| Unrelated local tools exchange a stream | FIFO | Filesystem pathname is a rendezvous |
| Preserve records | Message queue | One receive returns one message |
| Large shared buffers | Shared memory + synchronization | Avoids per-message copying |
| Only coordinate access | Semaphore, file lock, process-shared mutex | Synchronizes without transferring data |
| Local request/response with fd passing | UNIX domain socket | Fd-based and flexible |
| Cross-machine communication | Socket | IPC boundary becomes network boundary |

The interview trap is simple: **shared memory is not a complete protocol**. It gives storage, not mutual exclusion, readiness, ownership, recovery, or versioning.

## Mechanism

Linux IPC mechanisms differ in where state lives and who owns lifetime. Copy-based IPC stores bytes or messages in kernel buffers; shared memory maps the same physical pages into multiple address spaces; synchronization objects decide when a process may continue.

### Why Multiple IPC Families Coexist

Linux carries old and new IPC families because UNIX systems evolved in parallel and production software values compatibility. The right question is not "which API is newest", but "which lifetime, peer relationship, and deployment constraint does this system need".

| Family | Why it exists | Why it still appears |
|--------|---------------|----------------------|
| Pipes/FIFOs | early UNIX stream composition | shell pipelines, supervisors, simple local data flow |
| BSD sockets | local and network communication with fd semantics | UNIX domain sockets, TCP/UDP, fd passing, event loops |
| System V IPC | persistent kernel IPC for unrelated UNIX processes | legacy systems, databases, industrial/embedded deployments |
| POSIX IPC | standardized name/open/unlink APIs | clearer cleanup, POSIX MQ/semaphore/SHM portability |
| `mmap()` and process-shared pthread objects | shared address-space style coordination | high-throughput shared state and richer locking |

Do not mix families casually. A mixed design needs an explicit ownership rule: which object carries data, which object synchronizes readiness, and which process is responsible for cleanup after crash or restart.

### Data Transfer vs Shared Memory

| Model | Data path | Kernel role | Main risk |
|-------|-----------|-------------|-----------|
| Pipe/FIFO/socket stream | user buffer -> kernel buffer -> user buffer | buffering, blocking, EOF/broken-pipe behavior | missing framing, leaked fds |
| Message queue | user message -> kernel queue -> user message | message boundaries, queue limits, selection/order | full queue, wrong type/priority |
| Shared memory | process virtual address -> shared physical page | mapping and permissions | races, corruption, invalid pointers |
| Semaphore | no application data transfer | atomic counter/wait queue | deadlock, abandoned state |

### Lifetime Models

| Lifetime | Meaning | Examples |
|----------|---------|----------|
| Descriptor/reference lifetime | Kernel data disappears after last reference closes | pipe data, socket connection state |
| Pathname plus descriptor lifetime | Name may persist, unread data does not | FIFO pathname |
| Kernel persistence | Object survives process exit until explicit remove/unlink or reboot | System V IPC, POSIX named IPC |
| Filesystem persistence | Backing file can survive reboot | file-backed `mmap()` |

Persistent IPC is useful for unrelated processes, but it creates stale-object bugs after crashes and watchdog restarts.

### FD-Based vs Non-FD IPC

| Facility | Handle | Works naturally with `/proc/<pid>/fd` and event loops? |
|----------|--------|--------------------------------------------------------|
| Pipe | file descriptor | yes |
| FIFO | file descriptor | yes |
| POSIX SHM | fd before `mmap()` | yes for the object fd |
| POSIX MQ on Linux | `mqd_t` implemented as fd | yes on Linux, not portable POSIX |
| System V MQ | `msqid` integer | no |
| System V semaphore | `semid` integer | no |
| System V SHM | `shmid`, then `shmat()` pointer | no fd after attach |
| POSIX named semaphore | `sem_t *` | no portable fd |

Fd-based IPC is easier to inspect with `/proc`, trace with file syscalls, and integrate into `poll()`/`epoll()`. System V IPC uses separate kernel tables and separate tooling.

## Key APIs And Objects

Use this table to orient yourself before diving into the dedicated files.

| Family | Main objects | Core APIs | Debug surface |
|--------|--------------|-----------|---------------|
| Pipes/FIFOs | pipe buffer, read end, write end, FIFO inode | `pipe()`, `pipe2()`, `mkfifo()`, `open()`, `read()`, `write()`, `close()`, `dup2()`, `popen()` | `/proc/<pid>/fd`, `/proc/<pid>/fdinfo`, `find -type p`, `strace` |
| System V MQ | key, `msqid`, `mtype`, kernel queue | `ftok()`, `msgget()`, `msgsnd()`, `msgrcv()`, `msgctl(IPC_RMID)` | `ipcs -q`, `ipcrm -q`, `/proc/sysvipc/msg`, `/proc/sys/kernel/msg*` |
| System V semaphore | key, `semid`, semaphore set, `SEM_UNDO` | `semget()`, `semop()`, `semctl()`, `IPC_RMID` | `ipcs -s`, `/proc/sysvipc/sem`, `/proc/sys/kernel/sem` |
| System V SHM | key, `shmid`, attached segment | `shmget()`, `shmat()`, `shmdt()`, `shmctl(IPC_RMID)` | `ipcs -m`, `/proc/sysvipc/shm`, `/proc/<pid>/maps`, `pmap` |
| POSIX MQ | `/name`, `mqd_t`, priority, `mq_attr` | `mq_open()`, `mq_send()`, `mq_receive()`, `mq_notify()`, `mq_unlink()` | `/dev/mqueue`, `/proc/sys/fs/mqueue` |
| POSIX semaphore | `/name` or `sem_t` in shared memory | `sem_open()`, `sem_wait()`, `sem_post()`, `sem_close()`, `sem_unlink()`, `sem_init()` | `/dev/shm/sem.*` on Linux, process logs, `strace` |
| POSIX SHM | `/name`, fd, mapping | `shm_open()`, `ftruncate()`, `mmap()`, `munmap()`, `shm_unlink()` | `/dev/shm`, `df -h /dev/shm`, `/proc/<pid>/maps` |

System V uses `key -> id -> ctl(IPC_RMID)`. POSIX uses `name -> open -> close/unmap -> unlink`.

## Lifecycle / Data Flow

Choose IPC by lifecycle as much as by API. Most production failures come from one side exiting, one reference being leaked, or one persistent object being reused unexpectedly.

### Pipe

```text
pipe(pfd)
fork()
parent/child close unused ends
write bytes -> kernel pipe buffer -> read bytes
all writers close
reader drains buffer
read() returns 0
```

EOF depends on **all** write descriptors being closed, including inherited descriptors in grandchildren after `exec()`.

### FIFO

```text
mkfifo(path, mode)
reader open(path, O_RDONLY)
writer open(path, O_WRONLY)
read/write like pipe
all fds close -> unread data disappears
unlink(path) removes name
```

The pathname persists; queued data does not.

### System V IPC

```text
choose key: ftok(), hard-coded key, or IPC_PRIVATE
xxxget(key, IPC_CREAT | perms) -> integer id
use object: msgsnd/msgrcv, semop, shmat/direct memory
xxxctl(id, IPC_RMID, ...) removes or marks for removal
```

`ftok()` keys can collide, and deleting/recreating the key file can change the generated key.

### POSIX IPC

```text
open name: mq_open(), sem_open(), shm_open()
use object
close local handle or munmap mapping
unlink name: mq_unlink(), sem_unlink(), shm_unlink()
existing references continue after unlink
last reference frees object
```

POSIX SHM starts with size 0. Call `ftruncate()` before `mmap()` and before any process can touch the intended range.

### Shared Memory Protocol

```text
shared struct header:
  magic
  version
  size
  state
  producer index / consumer index
  offsets, not raw pointers
  payload

writer:
  acquire semaphore/mutex
  validate header
  update payload and state
  release semaphore/mutex

reader:
  acquire semaphore/mutex
  validate header
  read stable state
  release semaphore/mutex
```

Embedded systems benefit from explicit `magic`, `version`, and `state` fields because restarts can leave memory initialized but semantically stale.

## Production Bugs And Debugging

Start from the symptom. IPC bugs usually look like hangs, unexpected EOF, broken pipes, stale data, or corrupted shared state.

| Symptom | Likely cause | Evidence | Fix pattern |
|---------|--------------|----------|-------------|
| Reader never sees EOF | leaked write fd after `fork()`/`exec()` | `ls -l /proc/<pid>/fd`, `readlink`, `strace close,execve` | close unused ends; set `O_CLOEXEC`/`FD_CLOEXEC` |
| Writer never gets `SIGPIPE`/`EPIPE` | leaked read fd | `/proc/<pid>/fd`, process tree | close read ends in writers/supervisors |
| FIFO startup hangs | peer did not open opposite end | `strace -e openat`, process logs | define startup order, use `O_NONBLOCK` plus retry/timeout |
| Stream records interleave | multiple writers write records larger than `PIPE_BUF` | protocol logs, `strace -e write` sizes | keep atomic records <= `PIPE_BUF` or use MQ |
| Producer blocks forever | pipe/MQ full, receiver dead or slow | queue depth, `ipcs -q`, `/dev/mqueue/<name>` | timeout, backpressure policy, health checks |
| Receiver blocks forever | wrong MQ type or no matching priority/producer | inspect message protocol and queue contents | validate type/priority contract |
| Restart sees old state | persistent IPC survived crash | `ipcs`, `/dev/mqueue`, `/dev/shm` | exclusive create, ownership check, safe cleanup |
| System V peers disagree on key | `ftok()` collision or key file recreated | compare keys, `stat` key file, `ipcs` | stable key file or explicit id sharing |
| Semaphore users hang at boot | init race or abandoned count | `ipcs -s -i`, `sem_otime`, traces | parent initializes before fork or wait for `sem_otime` |
| Shared memory corrupt | missing synchronization or bad layout | `/proc/<pid>/maps`, logs, invariant checks | lock every invariant; store offsets |
| POSIX SHM crashes with `SIGBUS` | object not `ftruncate()`d or wrong size | `ls -l /dev/shm`, `strace ftruncate,mmap` | size object before mapping/use |
| `mq_notify()` fires once only | one-shot registration misunderstood | code inspection, `strace mq_notify` | re-register before draining |

Useful commands:

```bash
# fd-based IPC
ls -la /proc/<pid>/fd
cat /proc/<pid>/fdinfo/<fd>
readlink /proc/<pid>/fd/<fd>
lsof -p <pid>
find /run /tmp -type p 2>/dev/null
strace -f -e trace=pipe,pipe2,openat,read,write,close,dup2,execve ./program

# System V IPC
ipcs
ipcs -q
ipcs -s
ipcs -m
ipcs -l
ipcrm -q <msqid>
ipcrm -s <semid>
ipcrm -m <shmid>
cat /proc/sysvipc/msg
cat /proc/sysvipc/sem
cat /proc/sysvipc/shm
cat /proc/sys/kernel/msgmax
cat /proc/sys/kernel/sem
cat /proc/sys/kernel/shmmax

# POSIX IPC on Linux
ls -la /dev/mqueue
cat /dev/mqueue/<name>
cat /proc/sys/fs/mqueue/msg_max
ls -la /dev/shm
df -h /dev/shm
cat /proc/<pid>/maps
pmap <pid>
```

On stripped embedded targets, prefer low-impact evidence first: `/proc`, `ipcs`, mount tables, object names, queue depths, fd links, and short `strace` windows.

## DevLinux Practice Bridge

Use the DevLinux modules as practice lanes after the mental model is clear.

| DevLinux module | Practice value | Connect it back to this chapter |
|-----------------|----------------|---------------------------------|
| 08 Pipes/FIFOs | Build and run one-way pipes, bidirectional examples, FIFO client-server examples, framing patterns | Check fd close discipline, EOF behavior, `PIPE_BUF`, and startup blocking |
| 09 Message Queues | Compare System V and POSIX message queues with send/receive examples | Check message boundaries, type/priority selection, queue limits, and cleanup |
| 10 Shared Memory | Create System V and POSIX shared memory examples | Add synchronization, header/version fields, and offset-based layout |
| 11 Semaphores | Run System V and POSIX semaphore examples | Explain who initializes, who releases, and what happens if a process dies |

When practicing, do not stop at "the example works". Change one failure condition at a time: kill a peer, leave a FIFO without a reader, fill a queue, remove an IPC object while a process waits, skip `ftruncate()`, or restart only one process. Those experiments turn API memory into production intuition.

## Embedded Constraints

IPC designs on embedded Linux fail differently because restarts, memory pressure, and limited tooling are normal operating conditions.

- Prefer object names and paths under service-owned directories such as `/run/<service>/`, not world-writable ad hoc paths.
- Treat watchdog restart as a partial failure: one process may restart while another still owns fds, queues, semaphores, or mappings.
- Keep cleanup conservative. Remove persistent IPC only after proving the owner is dead or the object belongs to this boot/service instance.
- Budget kernel memory for pipe buffers, message queues, POSIX MQ limits, `/dev/shm`, and System V limits.
- Include low-impact diagnostics in runbooks: `/proc/<pid>/fd`, `ipcs`, `/proc/sysvipc/*`, `/dev/mqueue`, `/dev/shm`, and service logs.
- Use `lsof -p <pid>` when available to cross-check fd ownership, deleted FIFO paths, and tmpfs-backed SHM files.
- Check namespaces and mount setup before assuming an object is missing; a shell and a service may not see the same IPC namespace or tmpfs/mqueue mount.

## Work Checklist

Use this checklist before choosing or reviewing an IPC design.

- Define the data shape: bytes, records, shared struct, or synchronization only.
- Define peer relationship: parent-child, unrelated local processes, many clients, future network need.
- Define object lifetime: per process, pathname, kernel-persistent, or reboot-persistent.
- Define failure behavior: peer exit, crash, watchdog restart, receiver slow, queue full.
- Add close-on-exec for fds that must not leak into `exec()`ed programs.
- Add timeouts or nonblocking mode where a dead peer would otherwise hang forever.
- For streams, define framing and maximum record size.
- For queues, define capacity, overflow policy, type/priority rules, and stale-message handling.
- For shared memory, define synchronization, layout versioning, offsets, ownership, and recovery.
- For persistent IPC, use exclusive create where possible and clean up only after proving the owner is dead.
- For embedded systems, include init scripts/service hooks that remove stale objects without deleting live IPC.
- Document the debug commands operators should use on the target.

## Recognize / Advanced

These topics appear in production code, but they are not the first mental model.

| Topic | Recognize this |
|-------|----------------|
| `eventfd()` | Linux fd-based counter often used for event-loop wakeups |
| UNIX domain socket fd passing | Lets one process send an open fd to another |
| Process-shared pthread mutex/condvar | Alternative to semaphores when robust/process-shared support is available |
| `F_SETPIPE_SZ` | Linux pipe capacity tuning; do not depend on exact capacity for correctness |
| Huge-page SHM | Performance tuning for large shared-memory regions |
| Lock-free SHM protocol | Requires explicit memory-ordering and crash-recovery design |
| IPC namespaces | Containers/services may not see the same System V/POSIX IPC objects |
| Mount setup | `/dev/mqueue` and `/dev/shm` may be missing or mounted differently |

## Interview Readiness

You should be able to explain IPC as a design choice, not as an API list.

Practice answering:

- Why do isolated processes need IPC?
- How do pipes/FIFOs differ from message queues?
- Why does shared memory need synchronization?
- What is the difference between System V key/id objects and POSIX name/open/unlink objects?
- How can leaked fds break EOF or `SIGPIPE`?
- How would you debug an IPC program that hangs after a restart?
- What cleanup policy would you use on an embedded device with watchdog restarts?

Interview anchor answer:

```text
I choose IPC by data shape, peer relationship, lifetime, and failure behavior.
Streams need framing and close discipline. Queues preserve records but can fill.
Shared memory is fastest for large data, but it needs synchronization and a layout protocol.
Persistent IPC must include stale-object detection and cleanup.
```

## Final Coverage Check

- [x] Row 7.1 IPC overview is covered directly.
- [x] Rows 7.2-7.10 are represented here and moved to their dedicated knowledge files for mechanism detail.
- [x] Chapter Must Cover concepts are covered or explicitly routed: taxonomy, stream/message behavior, peer/lifetime, pipes/FIFOs, System V IPC, POSIX IPC, SHM protocol, synchronization, debugging, and Embedded restart constraints.
- [x] Existing useful content was preserved and expanded; no mapped topic is intentionally out of scope.
