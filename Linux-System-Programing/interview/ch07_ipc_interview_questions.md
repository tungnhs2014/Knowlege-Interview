# Chapter 7 Interview - IPC

> Scope: Linux IPC selection, pipes and FIFOs, System V IPC, POSIX IPC, message queues, semaphores, shared memory, lifecycle, cleanup, failure modes, and production debugging.
> Interview style: scenario-first. API names and flags appear as follow-up keywords, not as the main question shape.

---

## Review Basis

This interview set was reviewed against the Chapter 7 learning map and the mapped repository sources:

- Knowledge docs: `knowledge/ch07_ipc_overview.md`, `ch07_ipc_pipes.md`, `ch07_ipc_sysv.md`, and `ch07_ipc_posix.md`.
- TLPI-derived docs: Chapters 43, 44, 45, 46, 47, 48, 51, 52, 53, and 54.
- DevLinux practical docs: `docs/Linux-Programming-DevLinux/INDEX.md`, `README.md`, Module 08 Pipes/FIFOs, Module 09 Message Queues, Module 10 Shared Memory, Module 11 Semaphores, and their relevant exercise READMEs.
- Linux man-pages used for semantic checks: `pipe(2)`, `pipe(7)`, `fifo(7)`, `open(2)`, `dup(2)`, `poll(2)`, `select(2)`, `epoll(7)`, `sysvipc(7)`, `msgget(2)`, `msgsnd(2)`, `msgrcv(2)`, `msgctl(2)`, `semget(2)`, `semop(2)`, `semctl(2)`, `shmget(2)`, `shmat(2)`, `shmdt(2)`, `shmctl(2)`, `mq_overview(7)`, `mq_open(3)`, `mq_send(3)`, `mq_receive(3)`, `mq_notify(3)`, `sem_overview(7)`, `sem_open(3)`, `sem_wait(3)`, `sem_post(3)`, `shm_overview(7)`, `shm_open(3)`, `mmap(2)`, and `munmap(2)`.

External calibration sources were used only to tune interview priority and framing:

- Amazon official software development interview topics include operating systems as a core prep area: <https://www.amazon.jobs/content/en-gb/how-we-hire/interview-prep/software-development-topics>
- Microsoft technical interviewing guidance emphasizes problem solving, technical principles, testing, and boundary conditions: <https://careers.microsoft.com/v2/global/en/hiring-tips/technical-interviewing.html/>
- Google Careers role pages repeatedly emphasize production software, design, testing, debugging, performance, and large-scale systems work: <https://www.google.com/about/careers/applications/jobs/results?q=%22Software+Engineer%22>
- Google virtual interview candidate resources were used as general official interview-process calibration: <https://services.google.com/fh/files/misc/technical_virtual_interviews_candidate_resource.pdf>
- Meta official SWE interview preparation emphasizes communication, reasoning, correctness, trade-offs, verification, and follow-up discussion: <https://www.metacareers.com/careers/SWE-prep-onsite>
- Recurring OS/Linux interview banks such as GeeksforGeeks and InterviewBit were used only to detect common topic recurrence: IPC purpose, pipes, shared memory, message queues, semaphores, synchronization, and deadlock. Technical authority remains repo docs, TLPI-derived docs, and Linux man-pages.

---

## Coverage Trace

Every Chapter 7 learning-map row and Must Cover concept is intentionally placed in Priority A, B, or C. Priority A holds production/debug scenarios; Priority B holds comparisons and trade-offs; Priority C holds recognize-only Linux-specific or rare details.

| Coverage Matrix item | Priority coverage | Interview target |
|---|---|---|
| 7.1 IPC overview: taxonomy, data-transfer vs shared memory, byte stream vs message, synchronization | A, B | A1, B1, B2 |
| 7.2 Pipes & FIFOs: `pipe()`, `mkfifo()`, named pipes | A, B, C | A2, A3, A4, A5, B3, C pipe/FIFO notes |
| 7.3 System V IPC intro: `ftok()`, `ipcs`, `ipcrm` | A, B, C | A6, A13, B5, C System V notes |
| 7.4 System V message queues | A, B, C | A7, B6, C `MSG_*` notes |
| 7.5 System V semaphores | A, B, C | A9, A14, B7, B9, B13, C semaphore notes |
| 7.6 System V shared memory | A, B, C | A8, A13, B8, C SHM notes |
| 7.7 POSIX IPC intro | A, B, C | A10, A13, B5, C namespace/ACL notes |
| 7.8 POSIX message queues | A, B, C | A11, B6, C POSIX MQ fd/limits notes |
| 7.9 POSIX semaphores | A, B, C | A9, A14, B7, B9, B13, C named/unnamed semaphore notes |
| 7.10 POSIX shared memory | A, B, C | A12, B8, C `/dev/shm` notes |
| Must Cover: IPC taxonomy and communication vs synchronization | A, B | A1, B1, B9 |
| Must Cover: byte-stream vs message, boundaries, framing, ordering, backpressure, blocking | A, B | A4, A5, A7, A11, B2, B6 |
| Must Cover: peer relationship and lifetime | A, B | A1, A2, A3, A4, A6, A10, A12, B3, B5 |
| Must Cover: pipes/FIFOs traps: EOF/SIGPIPE, `PIPE_BUF`, close discipline, deadlocks, multi-client limits | A, B, C | A2, A3, A4, A5, B3, C FIFO notes |
| Must Cover: System V keys, ids, permissions, tools, stale objects, init races | A, B, C | A6, A7, A9, A14, B5, B11, C System V notes |
| Must Cover: POSIX names, open/close/unlink, MQ/sem/SHM lifecycle | A, B, C | A10, A11, A12, B5, B6, B7, B8 |
| Must Cover: shared-memory protocol: layout, offsets, sync, versioning, crash recovery | A, B | A8, A12, A13, B4, B8, B10 |
| Must Cover: synchronization primitives, ownership, deadlock/starvation | A, B, C | A9, A14, B7, B9, B13, C semaphore notes |
| Must Cover: production debugging and Embedded constraints | A, B, C | A2-A14, B11, B12, C namespace/limits notes |

---

## Priority Map

### A - Project and production scenarios

Study these deeply. Be ready to explain mechanism, failure behavior, debugging commands, and design trade-offs.

- Selecting an IPC mechanism for a service or embedded multi-process pipeline.
- Debugging pipe/FIFO hangs, EOF bugs, `SIGPIPE`, fd leaks across `exec()`, and backpressure.
- Designing FIFO or message-queue client-server protocols for unrelated processes, including crashed or malicious clients.
- Handling System V IPC keys, stale objects, permissions, and cleanup after crashes.
- Designing message queue protocols with type/priority, queue limits, timeouts, and per-client response paths.
- Using semaphores for inter-process synchronization without creating deadlocks, starvation, stale locks, or initialization races.
- Using shared memory for high-throughput data while adding synchronization, offsets, lifecycle cleanup, and corruption recovery.
- Choosing POSIX IPC for new Linux code and avoiding `mq_notify()`, `shm_open()` sizing, and unlink lifecycle traps.
- Debugging production IPC on constrained embedded targets with limited RAM, watchdog restarts, stripped binaries, and init/systemd cleanup.

### B - Design comparisons and senior follow-ups

Know the trade-offs and be able to justify a choice. Do not memorize every flag.

- Pipe vs FIFO vs UNIX domain socket.
- Pipe/FIFO vs message queue.
- System V IPC vs POSIX IPC.
- System V MQ vs POSIX MQ.
- System V semaphore vs POSIX semaphore vs process-shared pthread mutex/condition variable.
- Semaphore vs mutex vs file lock.
- Semaphore vs process-shared mutex/condition variable for shared-memory synchronization.
- Message queue vs shared memory.
- Shared memory plus semaphore vs socket.
- POSIX SHM vs System V SHM vs shared file mapping.
- `select()`/`poll()`/`epoll()` integration and why System V IPC is awkward here.
- Cleanup, permissions, resource limits, namespaces, and restart behavior.

### C - Lower-priority / know enough to recognize

Recognize these in code or follow-up discussion, then look up exact details when needed.

- `pipe2()`, `F_GETPIPE_SZ`, `F_SETPIPE_SZ`, `FIONREAD`, `splice()`, `tee()`, and `vmsplice()`.
- FIFO `O_RDWR` behavior on Linux and why it can hide EOF.
- System V `MSG_EXCEPT`, `MSG_NOERROR`, `semtimedop()`, `SHM_LOCK`, `SHM_HUGETLB`, and `SHM_RDONLY`.
- POSIX MQ Linux fd behavior, `/proc/sys/fs/mqueue/*`, and `RLIMIT_MSGQUEUE`.
- POSIX IPC ACLs, IPC namespaces, `memfd_create()`, `eventfd()`, realtime signals with payloads, and UNIX domain fd passing.

---

## Final Interview List

### A - Project and production scenarios

1. A product has several local processes: one captures sensor data, one filters it, one logs it, and one exposes status. How would you choose the IPC mechanisms?
2. A parent-child pipeline sometimes hangs forever after the writer exits. How would you debug and fix the pipe design?
3. A helper process is launched with `exec()`, and a later pipeline never sees EOF. What IPC bug do you suspect?
4. A FIFO-based control service blocks during startup or under multiple clients. How would you redesign it?
5. A stream over pipe/FIFO corrupts request boundaries under load. How would you preserve records and handle backpressure?
6. A legacy System V service fails after a crash because old queues, semaphores, or shared memory segments remain. How would you recover and prevent recurrence?
7. A System V message queue client-server app occasionally blocks forever or sends replies to the wrong client. How would you debug the protocol?
8. A multi-process shared-memory data path is fast but sometimes produces corrupt structures. How would you make it safe?
9. A process dies while holding a semaphore-like lock. What can System V and POSIX semaphores do, and what can they not fix?
10. You are writing new Linux IPC code. When would POSIX IPC be preferable to System V IPC, and what lifecycle traps remain?
11. A POSIX message queue consumer misses notifications or sees messages out of expected order. How would you reason about it?
12. A POSIX shared-memory program crashes with `SIGBUS` or leaves stale objects in `/dev/shm`. What would you check?
13. On an embedded target, an IPC-heavy service hangs after watchdog restart. What production debug workflow would you use?
14. Several processes use semaphores around shared memory, but the system sometimes deadlocks or one process starves forever. How would you redesign the synchronization?

### B - Design comparisons and senior follow-ups

1. Compare data-transfer IPC with shared-memory IPC.
2. Compare byte-stream IPC with message-oriented IPC.
3. Compare pipe, FIFO, and UNIX domain socket for local IPC.
4. Compare message queue and shared memory for throughput, latency, and complexity.
5. Compare System V IPC and POSIX IPC for new Linux code.
6. Compare System V message queues and POSIX message queues.
7. Compare System V semaphores and POSIX semaphores.
8. Compare System V SHM, POSIX SHM, and shared file mappings.
9. Compare semaphore, mutex, and file lock.
10. Explain when not to use shared memory.
11. Explain how IPC permissions and `umask` differ across FIFO, POSIX IPC, and System V IPC.
12. Explain how `select()`, `poll()`, and `epoll()` change IPC design.
13. Compare semaphores with process-shared mutexes and condition variables for shared-memory synchronization.

### C - Lower-priority / know enough to recognize

- Linux-only pipe helpers and capacity tuning.
- Linux-specific System V and POSIX IPC limits under `/proc`.
- IPC namespaces and container visibility surprises.
- POSIX MQ fd behavior on Linux versus portable POSIX.
- Advanced shared memory tuning such as huge pages and memory locking.
- `eventfd()`, `memfd_create()`, realtime signals, and UNIX domain fd passing as adjacent IPC tools.

---

## High-Value Comparisons

| Comparison | Interview answer |
|---|---|
| Data transfer vs shared memory | Data transfer moves bytes/messages through kernel buffers and gives natural blocking/backpressure. Shared memory maps the same pages into multiple processes and needs an explicit synchronization protocol. |
| Synchronization vs communication | Semaphores, mutexes, and file locks coordinate access. Pipes, FIFOs, sockets, message queues, and shared memory carry data. Shared memory carries data but does not synchronize itself. |
| Byte stream vs message | Pipes/FIFOs/stream sockets preserve byte order but not records. Message queues preserve one send as one receive and add type or priority selection. |
| Pipe vs FIFO | A pipe is anonymous and normally shared by related processes after `fork()`. A FIFO has a filesystem pathname and can be opened by unrelated local processes. |
| Pipe/FIFO vs UNIX domain socket | Pipe/FIFO is simpler for one-way local streams. UNIX domain sockets support bidirectional communication, datagrams or streams, connection handling, credentials, and fd passing. |
| Pipe/FIFO vs message queue | Pipes/FIFOs are simple streams. Message queues fit bounded records, requests, priorities, or type-based selection. |
| Message queue vs shared memory | MQ is simpler and safer for small commands. SHM is faster for large data but requires locking, layout, versioning, and recovery. |
| System V IPC vs POSIX IPC | System V uses keys and integer identifiers with special tools. POSIX uses names plus open/close/unlink style APIs and clearer reference-counted deletion. |
| System V MQ vs POSIX MQ | System V selects by `mtype`. POSIX receives highest-priority messages first and supports notification. Linux makes POSIX MQ descriptors pollable, but that is not portable. |
| System V semaphore vs POSIX semaphore | System V uses semaphore sets, atomic multi-operation `semop()`, and `SEM_UNDO`. POSIX semaphores are simpler individual counters but lack `SEM_UNDO`. |
| Semaphore vs mutex vs file lock | A semaphore is a counter/signaling primitive. A mutex is ownership-oriented mutual exclusion. A file lock is best when the shared resource is a file or byte range. |
| Semaphore vs process-shared mutex/condvar | Semaphores count resources or signal events. A process-shared mutex protects ownership of shared invariants. A process-shared condition variable lets peers sleep until a state predicate changes, but it still needs the mutex and shared predicate. |
| System V SHM vs POSIX SHM | System V uses `shmget()` and `shmat()`. POSIX uses `shm_open()`, `ftruncate()`, and `mmap()`, so normal fd operations such as `fstat()` and `fchmod()` fit naturally. |
| POSIX SHM vs shared file mapping | POSIX SHM is temporary memory-backed shared storage. A shared file mapping is better when data must survive reboot or be inspected as a durable file. |
| Shared memory plus semaphore vs socket | SHM plus semaphore can be lower-copy and higher-throughput for local large data. Sockets are simpler for request/response protocols, peer failure, networking, and tooling. |

---

## Common Project Failure Patterns

| Failure | Symptom | Usual fix |
|---|---|---|
| Unused pipe ends not closed | Reader never sees EOF or writer never gets `EPIPE` | Close unused read/write ends in every process after `fork()` |
| Missing close-on-exec | Execed helper accidentally keeps pipe/FIFO fd open | Use `pipe2(O_CLOEXEC)` or `fcntl(FD_CLOEXEC)` before `exec()` |
| FIFO open order wrong | Startup hangs in `open()` | Use known peer startup order, `O_NONBLOCK`, timeout/retry, or socket/MQ |
| FIFO client never opens its reply FIFO | Server blocks opening client FIFO; other clients stall | Open reply FIFO with `O_NONBLOCK`, use timeout/worker isolation, reject stale client path |
| Multi-writer FIFO records too large | Requests interleave | Keep records <= `PIPE_BUF`, add framing, or use MQ/socket |
| Pipe or queue full | Producer stalls | Add backpressure policy, timeout, nonblocking mode, monitoring |
| `SIGPIPE` not handled intentionally | Writer dies unexpectedly | Ignore/catch `SIGPIPE` and handle `EPIPE` as peer shutdown where appropriate |
| System V object left after crash | Next run sees stale messages/state or `EEXIST` | Use `IPC_CREAT | IPC_EXCL`, inspect ownership, then safe `IPC_RMID` |
| `ftok()` mismatch or collision | Processes attach different or wrong object | Use stable key files, avoid recreated paths, validate object metadata |
| Wrong message type/priority | Receiver blocks or processes wrong work | Define protocol types/priorities and inspect queued state |
| Semaphore initialization race | Random blocking or wrong initial value | Parent-init-before-fork or `sem_otime` protocol for unrelated peers |
| Semaphore wait-order/starvation bug | One waiter never proceeds even though semaphore values change | Avoid mixed-size waits, use fixed lock ordering, add timeout/recovery and simpler ownership protocol |
| Process dies while holding lock | Other processes wait forever or data is inconsistent | Consider `SEM_UNDO`, robust mutexes, timeouts, and state validation |
| SHM lacks synchronization | Corrupted headers, torn state, impossible counters | Protect invariants with semaphore/mutex/futex protocol |
| Raw pointer stored in SHM | Peer crashes or dereferences invalid address | Store offsets or indexes relative to shared-memory base |
| POSIX SHM not sized | `mmap()` failure or `SIGBUS` on access | `ftruncate()` to intended size before mapping/use |
| POSIX object not unlinked | Stale `/dev/mqueue` or `/dev/shm` entry | `mq_unlink()`, `sem_unlink()`, `shm_unlink()` by owner/manager |
| POSIX MQ notification assumptions wrong | Missed notification, `EBUSY`, or a blocked receiver drains messages | Register one owner, handle already-nonempty queues, re-register after notification, coordinate blocking receivers |
| Permission mismatch | Peer gets `EACCES`/`ENOENT` | Check mode, owner, group, `umask`, mount namespace, and service user |

---

## Detailed Answers - Priority A

### 1. A product has several local processes: one captures sensor data, one filters it, one logs it, and one exposes status. How would you choose the IPC mechanisms?

**What the interviewer is testing**

Whether you choose IPC from data model, lifetime, throughput, failure behavior, and debuggability instead of naming APIs at random.

**Strong answer**

I would split the problem by data type. For large sensor frames or high-rate samples, shared memory is attractive because copying every payload through a pipe or queue can waste CPU and memory bandwidth. For control commands, status updates, and small work items, a message queue, UNIX domain socket, FIFO, or pipe is usually simpler. For coordination around the shared memory ring, I would use semaphores, process-shared mutexes, futex-based code, or eventfd-style notification depending on the codebase.

I would also decide whether peers are related or unrelated. Parent-child helpers can use anonymous pipes or inherited mappings. Unrelated daemons need named objects: FIFO path, POSIX IPC name, System V key, UNIX socket path, or a supervised fd-passing setup. On embedded systems I would keep cleanup and restart behavior explicit because watchdog restarts can leave stale named/keyed IPC.

**Mechanism**

Data-transfer IPC stores bytes or messages in kernel buffers. Shared memory maps the same physical pages into multiple address spaces. Semaphores synchronize access but do not carry data. Pipes and FIFOs are byte streams. Message queues preserve message boundaries. System V IPC uses keys and persistent kernel objects. POSIX IPC uses names and open/close/unlink style lifetime.

**Pitfalls**

Do not use shared memory just because it is fast. It adds protocol complexity: ownership, synchronization, memory layout, versioning, cleanup, and corruption recovery. Do not use pipes or FIFOs for multi-client structured records without framing and backpressure. Do not ignore permissions and stale object cleanup.

**Debug angle**

Use `strace` to see blocking calls, `/proc/<PID>/fd` for descriptor ownership, `ipcs` and `/proc/sysvipc/*` for System V objects, `ls -l /dev/mqueue`, `cat /dev/mqueue/<name>`, and `ls -l /dev/shm` for POSIX objects. On embedded targets also check service restart order, init/systemd cleanup, watchdog logs, RAM pressure, and target kernel config.

**Follow-up keywords**

Data-transfer IPC, shared memory, synchronization, pipe, FIFO, System V IPC, POSIX IPC, UNIX domain socket, backpressure, persistence, `ipcs`, `/dev/mqueue`, `/dev/shm`.

### 2. A parent-child pipeline sometimes hangs forever after the writer exits. How would you debug and fix the pipe design?

**What the interviewer is testing**

Whether you understand pipe EOF and broken-pipe semantics across duplicated file descriptors after `fork()`.

**Strong answer**

I would suspect that some process still has an unintended copy of the pipe write end open. `read()` returns 0 only after all write descriptors for that pipe are closed and buffered data has been drained. After `fork()`, both parent and child inherit both pipe ends, so every process must close the ends it does not use. In a shell-like pipeline, the parent also has to close both pipe fds after forking the children.

The symmetric bug happens on the writer side. If an unintended read end remains open, a writer may not see `SIGPIPE` or `EPIPE` when the real reader exits; it may block later when the pipe buffer fills.

**Mechanism**

A pipe is a kernel byte-stream buffer with a read end and write end. Descriptor copies refer to the same pipe object. EOF depends on the count of open write ends, not on whether the intended writer has exited. Broken-pipe behavior depends on all read ends being closed.

**Pitfalls**

Closing only in the obvious parent/child path is not enough when there are multiple children, error branches, or `exec()` paths. A process can also deadlock if both sides write large data first and neither side drains the peer pipe.

**Debug angle**

Inspect `ls -l /proc/<PID>/fd` and `readlink /proc/<PID>/fd/<fd>` for unexpected pipe fds. Use `strace -f -e trace=pipe,pipe2,dup2,close,read,write,execve,wait4` to verify close order. Use `lsof -p <PID>` when available.

**Follow-up keywords**

`pipe()`, `fork()`, `dup2()`, `close()`, EOF, `SIGPIPE`, `EPIPE`, `/proc/<PID>/fd`, `strace`.

### 3. A helper process is launched with `exec()`, and a later pipeline never sees EOF. What IPC bug do you suspect?

**What the interviewer is testing**

Whether you connect IPC correctness with file descriptor inheritance and close-on-exec hygiene.

**Strong answer**

I would suspect a descriptor leak across `exec()`. If a helper process inherits a pipe write end it never uses, the real reader will not see EOF after the intended writer exits. The fix is to make descriptors close-on-exec unless the child intentionally needs them. On Linux, `pipe2(O_CLOEXEC)` is the cleanest way to create pipe fds with close-on-exec atomically. Otherwise use `fcntl(F_SETFD, FD_CLOEXEC)` immediately after creation.

If the child intentionally needs stdin/stdout wired to the pipe, I would `dup2()` onto `STDIN_FILENO` or `STDOUT_FILENO`, then close the original pipe fds. The standard fd remains open only because it is the intended channel.

**Mechanism**

`exec()` replaces the program image but preserves open file descriptors unless `FD_CLOEXEC` is set. A descriptor inherited by an unrelated helper still counts for pipe EOF and `SIGPIPE` decisions.

**Pitfalls**

Using `fcntl()` in multithreaded code can race with another thread doing `fork()`/`exec()`. Atomic creation flags such as `O_CLOEXEC` reduce that risk. Also avoid assuming libraries will close descriptors they did not open.

**Debug angle**

Compare `/proc/<PID>/fd` before and after `exec()`. Trace with `strace -f -e trace=pipe,pipe2,fcntl,dup2,close,execve`. In production, check service supervisors and child process launch helpers.

**Follow-up keywords**

`pipe2(O_CLOEXEC | O_NONBLOCK)`, `FD_CLOEXEC`, `dup2()`, `execve()`, fd leak, shell pipeline.

### 4. A FIFO-based control service blocks during startup or under multiple clients. How would you redesign it?

**What the interviewer is testing**

Whether you understand FIFO rendezvous rules, unrelated-process IPC, and multi-client protocol design.

**Strong answer**

A FIFO has a filesystem name, so it is useful when unrelated local processes need a simple byte-stream rendezvous. The default `open()` is also a synchronization point: opening read-only blocks until a writer opens, and opening write-only blocks until a reader opens. With `O_NONBLOCK`, read-only open can succeed without a writer, while write-only open fails with `ENXIO` if no reader exists.

For a control service, I would usually create a well-known request FIFO under `/run/<service>/`, set restrictive permissions, and use per-client response FIFOs or another reply channel. A single shared response FIFO lets clients consume each other's replies unless the protocol prevents it. If startup blocking is harmful, I would use nonblocking open with retry, `poll()`, a supervisor-managed startup order, or switch to a UNIX domain socket.

For multiple clients, I would also protect the server from a bad client. If the request contains a client FIFO path but the client never opens it for reading, the server can block forever in `open(O_WRONLY)`. The server should use `O_NONBLOCK` plus timeout/retry, hand replies to worker processes/threads so one client cannot stop the accept loop, validate that client FIFO paths live under an owned directory, and drop requests whose reply channel cannot be opened safely.

**Mechanism**

Once opened, a FIFO behaves like a pipe: byte stream, finite kernel buffering, EOF when all writers close, and `SIGPIPE`/`EPIPE` when writing with no readers. The FIFO pathname persists until `unlink()`, but unread data does not persist after all descriptors close.

**Pitfalls**

Public `/tmp` FIFO names can have race and permission problems. Multiple writers can interleave records larger than `PIPE_BUF`. Keeping a dummy write fd open can prevent server EOF between clients, but it must be intentional because it changes EOF behavior. A FIFO server that opens each client reply FIFO in blocking mode lets one crashed or malicious client delay every later client.

**Debug angle**

Use `ls -l` to verify FIFO type and permissions, `find /run /tmp -type p`, `/proc/<PID>/fd` to see who has it open, and `strace -e trace=openat,read,write,close` to see whether the server is blocked opening a client FIFO or reading the request FIFO. Use `lsof` if available.

**Follow-up keywords**

`mkfifo()`, `fifo(7)`, `O_NONBLOCK`, `ENXIO`, `PIPE_BUF`, per-client FIFO, denial of service, permissions, cleanup.

### 5. A stream over pipe/FIFO corrupts request boundaries under load. How would you preserve records and handle backpressure?

**What the interviewer is testing**

Whether you know that pipes/FIFOs are byte streams, not message queues, and can reason about atomic writes and flow control.

**Strong answer**

I would not assume one `write()` equals one `read()`. Pipes and FIFOs preserve byte order, not application records. If record boundaries matter, I would add framing: fixed-size records, delimiter protocol, or length-prefix plus payload. With multiple writers, I would keep each complete request at or below `PIPE_BUF` if using a FIFO and relying on atomic writes. Larger records need framing and read/write loops, or a better mechanism such as message queue or socket.

Backpressure is normal. If the pipe buffer or queue fills, a blocking writer waits; in nonblocking mode it gets `EAGAIN` or a partial write depending on size and state. Production code needs timeouts, retry policy, cancellation, and metrics.

**Mechanism**

`PIPE_BUF` is the maximum size POSIX guarantees to be atomic with respect to other writers. On Linux it is commonly 4096 bytes. It prevents interleaving for writes up to that size, but it does not force a reader to receive exactly one record per `read()`.

**Pitfalls**

Large nonblocking writes can partially succeed. Stdio buffering over pipes can delay output. A slow reader can stall a producer. A writer that ignores short writes or `EINTR` loses data.

**Debug angle**

Trace `read()` and `write()` sizes with `strace`. Check `/proc/<PID>/fdinfo/<fd>`, blocked stack traces with `gdb`, and throughput with `perf` when CPU overhead matters. Inspect application framing logs, not just system calls.

**Follow-up keywords**

Byte stream, framing, `PIPE_BUF`, partial write, `EAGAIN`, `EINTR`, pipe capacity, `poll()`, `select()`, `epoll()`.

### 6. A legacy System V service fails after a crash because old queues, semaphores, or shared memory segments remain. How would you recover and prevent recurrence?

**What the interviewer is testing**

Whether you understand System V key-based lookup, kernel persistence, `ftok()` limitations, and safe cleanup.

**Strong answer**

I would first inspect the objects with `ipcs`, `ipcs -q`, `ipcs -s`, `ipcs -m`, and the detailed `ipcs -i` views. System V IPC objects have kernel persistence: they remain until explicitly removed or the system reboots. A crashed service can leave a queue full of old messages, a semaphore set with stale values, or a shared memory segment with old state.

For startup, I would use `IPC_CREAT | IPC_EXCL` when the service should own a clean object. If I get `EEXIST`, I would not blindly remove it. I would verify whether another valid instance is running, inspect owner/perms/timestamps/attach counts, then remove only when safe with `msgctl(IPC_RMID)`, `semctl(IPC_RMID)`, or `shmctl(IPC_RMID)`.

**Mechanism**

System V processes find objects using a `key_t`, often generated by `ftok()`, and then operate using an integer identifier. `ftok()` depends on file identity and low project-id bits; collisions and path recreation are possible. Message queues and semaphore sets are removed immediately by `IPC_RMID`; shared memory is marked for deletion and removed after the last detach.

**Pitfalls**

Hard-coded keys can collide. Recreated key files can produce different keys. `IPC_RMID` on a queue destroys messages immediately. Removing a live object's semaphore set can wake waiters with `EIDRM`. Permissions are not affected by `umask` on System V creation.

**Debug angle**

Use `ipcs -l` for limits, `/proc/sysvipc/msg`, `/proc/sysvipc/sem`, `/proc/sysvipc/shm`, `/proc/sys/kernel/msg*`, `/proc/sys/kernel/sem`, and `/proc/sys/kernel/shm*`. Check service user IDs and container IPC namespace.

**Follow-up keywords**

`key_t`, `ftok()`, `IPC_PRIVATE`, `IPC_CREAT`, `IPC_EXCL`, `IPC_RMID`, `ipcs`, `ipcrm`, `/proc/sysvipc`.

### 7. A System V message queue client-server app occasionally blocks forever or sends replies to the wrong client. How would you debug the protocol?

**What the interviewer is testing**

Whether you know message boundaries, `mtype` selection, queue limits, and client-server reply patterns.

**Strong answer**

I would inspect the message type protocol first. System V messages have a positive `mtype`, and `msgrcv()` can receive the first message, the first exact type, or the lowest type up to a threshold for negative `msgtyp`. A wrong `msgtyp` can make the receiver wait forever while other messages sit in the queue. For replies, a common design is a server queue plus per-client queue ID or per-client type. The protocol must prevent one client from consuming another client's response.

I would also check whether the queue is full. `msgsnd()` blocks when the queue has no room unless `IPC_NOWAIT` is used. Long-running services should include timeout/cancellation behavior instead of trusting every peer to stay alive.

**Mechanism**

System V MQ preserves message boundaries. `msgsz` excludes the `long mtype`. Reads are destructive. Queues are kernel-persistent and not fd-based, so they do not integrate naturally with `select()`, `poll()`, or `epoll()`.

**Pitfalls**

Wrong type selection, missing per-client reply path, queue full, stale queue from previous run, zero-length messages used accidentally, oversized receive buffer handling, and missing cleanup.

**Debug angle**

Use `ipcs -q`, `ipcs -q -i <msqid>`, `/proc/sysvipc/msg`, and `/proc/sys/kernel/msgmax`, `msgmnb`, `msgmni`. Trace `msgsnd()` and `msgrcv()` with `strace`; check signal interruptions and `EINTR`.

**Follow-up keywords**

`msgget()`, `msgsnd()`, `msgrcv()`, `msgctl()`, `mtype`, `IPC_NOWAIT`, `MSG_NOERROR`, `EIDRM`, queue limits.

### 8. A multi-process shared-memory data path is fast but sometimes produces corrupt structures. How would you make it safe?

**What the interviewer is testing**

Whether you understand that shared memory is storage, not a complete communication protocol.

**Strong answer**

I would treat shared memory as the data plane and design a separate synchronization/control protocol. Every shared invariant needs protection, not just individual fields. For a simple shared structure, a semaphore or process-shared mutex can protect updates. If a process needs to wait until a shared state predicate changes, a process-shared condition variable can fit, but only together with the mutex and predicate. For a ring buffer, I would define ownership of slots, producer/consumer indexes, memory visibility rules, and recovery after a process dies.

I would also audit layout. Shared memory may attach or map at different virtual addresses in each process, so data structures inside it should store offsets or indexes, not raw pointers. I would include a header with size, version, magic, state, and possibly generation counters so restarted processes can validate the segment before trusting it.

**Mechanism**

System V SHM uses `shmget()` and `shmat()`. POSIX SHM uses `shm_open()`, `ftruncate()`, and `mmap(MAP_SHARED)`. Once mapped, normal loads and stores access shared physical pages. The kernel does not serialize application-level updates.

**Pitfalls**

Missing synchronization, raw pointers, layout/version mismatch, process dies mid-update, stale segment reused after crash, permissions blocking a peer, deadlock from inconsistent lock ordering, starvation from unfair handoff, and assuming a ready flag is enough without memory ordering.

**Debug angle**

Use `ipcs -m`, `ipcs -m -i <shmid>`, `/proc/<PID>/maps`, `pmap`, `gdb`, `/dev/shm`, and application invariant checks. For corruption, capture writer/reader sequencing and lock ownership rather than only checking object existence.

**Follow-up keywords**

`shmget()`, `shmat()`, `shmdt()`, `shmctl(IPC_RMID)`, `shm_open()`, `ftruncate()`, `mmap()`, offsets, semaphore, process-shared mutex, futex.

### 9. A process dies while holding a semaphore-like lock. What can System V and POSIX semaphores do, and what can they not fix?

**What the interviewer is testing**

Whether you distinguish semaphore counter recovery from protected-data recovery.

**Strong answer**

System V semaphores have `SEM_UNDO`, which asks the kernel to reverse a process's semaphore adjustment when that process terminates. That can reduce abandoned-lock damage for simple lock-like usage. But it is not full crash recovery: if the process died after partially updating shared memory, undoing the counter does not repair the shared data.

POSIX semaphores are simpler and do not have a `SEM_UNDO` equivalent. With POSIX semaphores, I would use timeouts, owner/heartbeat metadata in shared memory, robust process-shared mutexes where appropriate, and a recovery path that validates and repairs state. A condition variable can wake peers waiting for a state change, but it does not repair an abandoned lock or corrupted shared state by itself.

**Mechanism**

A semaphore is a nonnegative kernel-maintained counter. Wait/decrement blocks if the value cannot be reduced. Post/increment wakes a waiter if one exists. Unlike a mutex, a semaphore does not inherently record an owner; any cooperating process can post if it has the handle and permission. System V `semop()` can apply multiple operations atomically across a semaphore set. POSIX semaphores operate as individual counters with `sem_wait()` and `sem_post()`.

**Pitfalls**

`SEM_UNDO` can hide some abandoned lock symptoms but cannot make complex state consistent. `sem_getvalue()` and System V `GETVAL` are snapshots, not safe decisions. System V semaphore creation and initialization are separate and can race. Because semaphores do not enforce ownership, a mistaken extra `sem_post()` can let multiple writers enter a critical section.

**Debug angle**

Use `ipcs -s`, `ipcs -s -i <semid>`, `/proc/sysvipc/sem`, `strace -e trace=semop,semctl`, and `gdb` to find waiters. For POSIX named semaphores, inspect `/dev/shm/sem.*` on Linux.

**Follow-up keywords**

Binary semaphore, counting semaphore, `semop()` atomic multi-operation, `SEM_UNDO`, `sem_otime`, `sem_wait()`, `sem_post()`, robust mutex.

### 10. You are writing new Linux IPC code. When would POSIX IPC be preferable to System V IPC, and what lifecycle traps remain?

**What the interviewer is testing**

Whether you can choose modern APIs without ignoring portability and cleanup details.

**Strong answer**

For new Linux code, POSIX IPC is often easier because it follows a file-like model: open or create by name, use the object, close or unmap local references, and unlink the name. POSIX objects are reference-counted after unlink, so deleting a name does not immediately break existing users. POSIX SHM returns a file descriptor, which works naturally with `fstat()`, `ftruncate()`, `fchmod()`, and `mmap()`.

System V remains important for legacy code, older UNIX portability, and features such as System V semaphore sets with atomic multi-operation `semop()` and `SEM_UNDO`.

**Mechanism**

POSIX MQ uses `mq_open()` and `mq_unlink()`. POSIX named semaphores use `sem_open()` and `sem_unlink()`. POSIX SHM uses `shm_open()`, `ftruncate()`, `mmap()`, `munmap()`, and `shm_unlink()`. On Linux, POSIX MQ appears under `/dev/mqueue`, while POSIX SHM and named semaphores commonly appear under `/dev/shm`.

**Pitfalls**

POSIX IPC can still be stale if not unlinked. Object names and filesystem visibility are not fully portable. `mq_notify()` is one-shot. POSIX SHM starts at size 0. Linux POSIX MQ descriptors are pollable fds, but POSIX does not require that.

**Debug angle**

Use `ls -l /dev/mqueue`, `cat /dev/mqueue/<name>`, `mount | grep mqueue`, `ls -l /dev/shm`, `df -h /dev/shm`, and `strace` around `mq_*`, `sem_*`, `shm_open`, `ftruncate`, `mmap`, and `shm_unlink`.

**Follow-up keywords**

POSIX IPC names, open/close/unlink, `mq_unlink()`, `sem_unlink()`, `shm_unlink()`, `/dev/mqueue`, `/dev/shm`, reference counting.

### 11. A POSIX message queue consumer misses notifications or sees messages out of expected order. How would you reason about it?

**What the interviewer is testing**

Whether you know POSIX MQ priority ordering, queue attributes, and `mq_notify()` edge cases.

**Strong answer**

First I would check ordering assumptions. POSIX MQ receives the highest-priority message first; FIFO applies only among messages with the same priority. If the application expects send order across different priorities, it is using the wrong model.

For missed notification, I would check `mq_notify()` usage. Notification occurs only when a queue transitions from empty to nonempty, only one process can be registered, and the registration is one-shot. A common safe pattern is to re-register first, then drain the queue in nonblocking mode until `EAGAIN`. If another thread is blocked in `mq_receive()`, it may consume the message instead of notification being delivered.

I would also check whether the queue was already nonempty when notification was registered. In that case, no notification is sent until the queue becomes empty and later receives a new message. If another process is already registered, `mq_notify()` fails with `EBUSY`; the program must treat that as a coordination bug or explicit ownership policy, not as a harmless retry detail.

**Mechanism**

`mq_attr` sets queue capacity and max message size at creation. `mq_receive()` requires a buffer at least `mq_msgsize`. Full queues block senders or return `EAGAIN` in nonblocking mode. Timed operations use absolute deadlines. `mq_notify()` registers one process for an empty-to-nonempty transition and removes the registration after one notification; a blocked receiver can take the message instead of the notifier being awakened.

**Pitfalls**

Relying on implementation defaults, too-small receive buffer, one-shot notification not re-registered, signal-based notification complexity, queue full, assuming registration works when another process owns it, assuming an already-nonempty queue triggers notification, and Linux-only `poll()`/`epoll()` assumptions.

**Debug angle**

Check `mq_getattr()`, `cat /dev/mqueue/<name>`, `/proc/sys/fs/mqueue/msg_max`, `msgsize_max`, `queues_max`, and `RLIMIT_MSGQUEUE`. Trace `mq_open`, `mq_timedsend`, `mq_timedreceive`, and `mq_notify`, and log `EBUSY`, `EAGAIN`, queue depth, and whether another consumer is blocked in receive.

**Follow-up keywords**

`mq_open()`, `mq_send()`, `mq_receive()`, `mq_notify()`, `EBUSY`, empty-to-nonempty transition, `mq_attr`, priority, `O_NONBLOCK`, `mq_timedreceive()`, `/dev/mqueue`.

### 12. A POSIX shared-memory program crashes with `SIGBUS` or leaves stale objects in `/dev/shm`. What would you check?

**What the interviewer is testing**

Whether you know the POSIX SHM create-size-map-use-unlink flow.

**Strong answer**

For `SIGBUS`, I would check whether the object was sized before use. A new POSIX SHM object has size 0. The creator normally calls `ftruncate(fd, size)` before mapping or at least before accessing the intended range. Mapping or accessing beyond the object size can crash. I would also check whether another process truncated the object while it was mapped.

For stale objects, I would check ownership of `shm_unlink()`. `close(fd)` does not remove the object, and `munmap()` only removes this process's mapping. `shm_unlink()` removes the name; existing mappings remain valid until unmapped or processes exit.

**Mechanism**

POSIX SHM uses a file descriptor from `shm_open()`, then `mmap(MAP_SHARED)`. After `mmap()`, the fd can be closed without invalidating the mapping. On Linux, objects are usually visible under `/dev/shm`.

**Pitfalls**

Forgetting `ftruncate()`, using wrong size on `mmap()`/`munmap()`, unlinking before unrelated peers have opened by name, not unlinking on owner shutdown, raw pointers in shared memory, and missing synchronization.

**Debug angle**

Use `ls -l /dev/shm`, `stat /dev/shm/<name>`, `df -h /dev/shm`, `/proc/<PID>/maps`, `pmap`, and `strace -e trace=shm_open,ftruncate,mmap,munmap,close,unlink`. In `gdb`, inspect the fault address and mapped range.

**Follow-up keywords**

`shm_open()`, `ftruncate()`, `mmap(MAP_SHARED)`, `munmap()`, `shm_unlink()`, `SIGBUS`, `/dev/shm`.

### 13. On an embedded target, an IPC-heavy service hangs after watchdog restart. What production debug workflow would you use?

**What the interviewer is testing**

Whether you can turn IPC knowledge into an on-device debugging plan under constraints.

**Strong answer**

I would start by classifying the hang: blocked open/read/write, full queue, semaphore wait, stale shared memory state, or permission failure after restart. Then I would collect low-impact evidence: process list, fd table, System V objects, POSIX object directories, resource limits, and recent logs. On embedded targets I would be careful with `strace` and `gdb` overhead, but they are still useful in short targeted runs.

I would also inspect restart cleanup. Watchdog restarts can kill the process before normal cleanup runs. Persistent IPC objects may survive and be reused with bad state. Init scripts or systemd units should remove owned FIFOs/POSIX names/System V objects only when safe, and the service should validate state on startup.

**Mechanism**

IPC hangs are usually wait-queue symptoms: pipe/FIFO peer missing, buffer full, message queue full or wrong type, semaphore value unavailable, or a process waiting for a shared-memory state transition that never happens.

**Pitfalls**

Assuming reboot-like cleanup after watchdog restart, ignoring IPC namespaces, target/host kernel differences, stripped binaries without symbols, no swap/limited RAM, and service user permission drift.

**Debug angle**

Use `strace`, `gdb`, `pmap`, `/proc/<PID>/fd`, `/proc/<PID>/stack` if available, `ipcs`, `ipcrm`, `lsipc`, `/proc/sysvipc/*`, `/proc/sys/kernel/msg*`, `/proc/sys/kernel/sem`, `/dev/mqueue`, `/dev/shm`, `lsof`, `dmesg`, service logs, and watchdog/init/systemd configuration.

**Follow-up keywords**

Embedded Linux, watchdog restart, stale IPC, limited RAM, no swap, systemd cleanup, init scripts, stripped binaries, target/host mismatch.

### 14. Several processes use semaphores around shared memory, but the system sometimes deadlocks or one process starves forever. How would you redesign the synchronization?

**What the interviewer is testing**

Whether you understand semaphore ownership limits, lock ordering, deadlock conditions, starvation, and why synchronization must protect shared-memory invariants rather than just individual fields.

**Strong answer**

I would first model the shared state and the critical sections. If this is really mutual exclusion around a shared structure, I would consider a process-shared mutex or robust mutex where supported because a mutex expresses ownership more directly. If peers need to sleep until a shared state predicate changes, I would use a process-shared condition variable with the mutex and predicate in shared memory. If semaphores are required, I would define a strict protocol: which semaphore protects which invariant, which process may decrement or post it, what order multiple semaphores must be acquired in, and what timeout/recovery path exists if a peer dies.

For deadlock, I would look for hold-and-wait cycles: process A holds semaphore 1 and waits for semaphore 2 while process B holds semaphore 2 and waits for semaphore 1. The fix is a single global lock order, fewer locks, try/timed waits with rollback, or a message/owner process that serializes updates. For starvation, I would avoid mixed-size semaphore waits where small requests keep succeeding and a larger request never becomes possible. If fairness matters, I would add an explicit queueing protocol or choose a primitive that gives the needed scheduling behavior.

**Mechanism**

A semaphore is a counter, not an ownership-tracking mutex. `sem_wait()` or negative `semop()` waits until the counter can be decremented; `sem_post()` or positive `semop()` increments it. System V `semop()` can apply multiple operations atomically, but TLPI notes that waiters are not simply served by arrival order in all cases. Requests that become possible first may proceed first, so some designs can starve larger or multi-semaphore operations.

**Pitfalls**

Do not treat `sem_getvalue()` or System V `GETVAL` as a safe synchronization decision; they are snapshots. Do not assume `SEM_UNDO` repairs protected data after a crash; it only adjusts semaphore counts. Do not let arbitrary processes post a semaphore unless the protocol says that is safe. Do not use busy waiting on shared-memory flags when a semaphore, condition variable, futex-style protocol, or eventfd would express readiness better.

**Debug angle**

For System V semaphores, use `ipcs -s`, `ipcs -s -i <semid>`, `/proc/sysvipc/sem`, and `strace -f -e trace=semop,semctl` to see blocked operations and current values. For POSIX named semaphores, inspect `/dev/shm/sem.*` on Linux and trace `sem_wait`, `sem_timedwait`, and `sem_post`. For shared memory, inspect `/proc/<PID>/maps`, app-level owner/heartbeat fields, and logs around acquire/release order.

**Follow-up keywords**

Semaphore ownership, mutex ownership, `semop()` atomic multi-operation, `SEM_UNDO`, `sem_timedwait()`, `semtimedop()`, robust mutex, process-shared condition variable, lock ordering, starvation, deadlock, shared-memory invariant.

---

## Short Answers - Priority B

**B1. Compare data-transfer IPC with shared-memory IPC.**

Data-transfer IPC copies bytes/messages through kernel buffers and gives natural blocking, destructive reads, and backpressure. Shared memory maps the same pages into multiple processes, avoiding per-message copies but requiring synchronization and a data-layout protocol.

**B2. Compare byte-stream IPC with message-oriented IPC.**

A byte stream has no record boundaries; the application must frame records. Message IPC preserves one send as one receive and may support type or priority selection.

**B3. Compare pipe, FIFO, and UNIX domain socket.**

Use a pipe for simple related-process streams, a FIFO for simple unrelated-process streams, and a UNIX domain socket when you need bidirectional communication, connection handling, datagrams, credentials, fd passing, or easier event-loop integration.

**B4. Compare message queue and shared memory.**

Use MQ for small bounded records and simpler correctness. Use SHM for high-volume shared data when the cost of copying matters enough to justify synchronization and recovery complexity.

**B5. Compare System V IPC and POSIX IPC.**

System V uses keys, integer identifiers, kernel persistence, and special tools. POSIX uses `/name` style objects, open/close/unlink APIs, and clearer reference-counted deletion. System V remains common in legacy code and has special semaphore features.

**B6. Compare System V MQ and POSIX MQ.**

System V MQ selects by message type and is not fd-based. POSIX MQ receives highest-priority first, supports `mq_notify()`, and on Linux is fd-backed, though that fd behavior is not portable.

**B7. Compare System V and POSIX semaphores.**

System V semaphores are sets and support atomic multi-operation `semop()` plus `SEM_UNDO`. POSIX semaphores are simpler named or unnamed counters, but have no `SEM_UNDO`.

**B8. Compare System V SHM, POSIX SHM, and shared file mappings.**

All share pages and require synchronization. System V uses `shmget()`/`shmat()`. POSIX SHM uses fd-style `shm_open()` plus `mmap()`. Shared file mappings are best when data should persist as a real file.

**B9. Compare semaphore, mutex, and file lock.**

A semaphore is a counter and can be used for resource counts or signaling; it does not inherently know which process "owns" the protected state. A mutex is ownership-oriented mutual exclusion, and a robust process-shared mutex can help detect owner death when supported. A file lock coordinates access to file content or file-associated resources and is often easier to inspect when the shared resource is already a file.

**B10. When should you not use shared memory?**

Avoid SHM when messages are small, protocol simplicity matters more than copy cost, peers may crash often, data must cross machines, or the team cannot maintain synchronization, layout, and recovery rules.

**B11. How do permissions differ across FIFO, POSIX IPC, and System V IPC?**

FIFOs use filesystem permissions and `umask`. POSIX IPC uses file-like permissions and `umask`. System V IPC stores `ipc_perm` ownership/mode and does not apply process `umask` at creation.

**B12. How do `select()`, `poll()`, and `epoll()` affect IPC design?**

Fd-based IPC such as pipes, FIFOs, sockets, POSIX SHM fds, and Linux POSIX MQ descriptors can fit event loops. System V IPC identifiers are not file descriptors, so they do not plug directly into these mechanisms.

**B13. Compare semaphores with process-shared mutexes and condition variables for shared-memory synchronization.**

Use semaphores when the protocol is naturally a counter, resource pool, or signal between processes. Use a process-shared mutex when the main need is ownership-oriented mutual exclusion around shared-memory invariants. Use a process-shared condition variable when processes need to sleep until a shared state predicate changes; the condition variable still needs a process-shared mutex and a checked predicate. Robust mutexes can help detect owner death, but recovery still needs application-level validation of the shared data.

---

## Recognition Notes - Priority C

- `pipe2()` is Linux-specific and can set `O_CLOEXEC` or `O_NONBLOCK` atomically.
- Pipe capacity can be queried/tuned on Linux, but correctness must not depend on exact capacity.
- `splice()`, `tee()`, and `vmsplice()` are advanced Linux pipe data-path helpers.
- `ioctl(FIONREAD)` can query unread pipe/FIFO bytes on many systems but is not portable POSIX.
- FIFO `O_RDWR` works on Linux but is unspecified by POSIX and can hide missing peer bugs.
- `MSG_EXCEPT` is Linux-specific for System V MQ receive filtering.
- `semtimedop()` is a Linux timeout variant of `semop()`.
- `SHM_LOCK`, `SHM_UNLOCK`, and huge-page SHM are specialized resource/performance features.
- POSIX MQ limits live under `/proc/sys/fs/mqueue/*`; System V limits live under `/proc/sys/kernel/msg*`, `/proc/sys/kernel/sem`, and `/proc/sys/kernel/shm*`.
- `mq_notify()` is one-shot, one process at a time, fails with `EBUSY` if already registered, and only notifies on an empty-to-nonempty transition.
- POSIX named semaphores often appear under `/dev/shm/sem.*` on Linux, but portable code should use `sem_open("/name", ...)`.
- Unnamed POSIX semaphores, process-shared pthread mutexes, and process-shared condition variables must live in shared memory when used across processes.
- Semaphore starvation can happen when different waiters require different counter changes or multiple semaphore operations become possible at different times.
- `eventfd()` is useful for fd-based counter notifications, especially event loops.
- `memfd_create()` creates anonymous memory-backed files that can be shared by fd passing.
- Realtime signals can carry small payloads but are not a general replacement for queues.
- IPC namespaces can make `ipcs`, `/dev/mqueue`, and object visibility differ across containers.

---

## Extra Questions Worth Adding

1. How would you design a shared-memory ring buffer for one producer and one consumer, including shutdown and restart behavior?
2. How would you make a FIFO client-server protocol resistant to a client that opens a request FIFO but never opens its response FIFO?
3. How would you migrate a legacy System V MQ service to POSIX MQ or UNIX domain sockets without breaking clients?
4. How would you decide between `eventfd()`, POSIX semaphore, and pipe for waking an event loop?
5. How would you design IPC cleanup for a systemd service that may be killed with `SIGKILL`?
6. How would you verify that a POSIX SHM object and an unnamed process-shared semaphore are initialized exactly once?
7. How would you debug an IPC issue that happens only inside a container but not on the host?
8. How would you protect a public FIFO path against permission and race issues?

---

## One-Minute Review

- IPC exists because processes are isolated but real systems need cooperation.
- First choose the data model: bytes, messages, shared pages, or synchronization.
- Pipes and FIFOs are byte streams; they need framing for records.
- Pipe EOF and `SIGPIPE` depend on all duplicated pipe ends, not just the intended peer.
- Use `O_CLOEXEC` to prevent execed helpers from keeping IPC fds alive accidentally.
- FIFOs allow unrelated processes to rendezvous by pathname, but default `open()` can block.
- `PIPE_BUF` gives atomic write protection against interleaving, not message semantics.
- System V IPC uses keys and integer identifiers; objects can become stale after crashes.
- `ftok()` is convenient but collision-prone and tied to file identity.
- System V MQ preserves boundaries and selects by message type.
- System V semaphores are sets; `semop()` can apply multiple operations atomically.
- `SEM_UNDO` helps with semaphore counts, not corrupted shared state.
- Shared memory is fast because processes access the same pages, but it needs synchronization.
- Store offsets, not raw pointers, inside shared memory.
- POSIX IPC uses names and open/close/unlink lifecycle.
- POSIX MQ receives highest-priority messages first; `mq_notify()` is one-shot.
- `mq_notify()` has one registered process per queue and notifies only on empty-to-nonempty transitions.
- POSIX SHM is `shm_open()` plus `ftruncate()` plus `mmap()`.
- Semaphores are counters, not ownership-tracking mutexes; deadlock/starvation still require protocol design.
- Debug System V IPC with `ipcs`, `ipcrm`, `lsipc`, and `/proc/sysvipc/*`.
- Debug POSIX IPC with `/dev/mqueue`, `/dev/shm`, `mount`, and `strace`.
- Production IPC design must include cleanup, permissions, timeouts, restart behavior, and embedded constraints.

---

## Final Coverage Check

- [x] Learning-map rows 7.1 through 7.10 appear in Priority A, B, or C coverage.
- [x] Chapter Must Cover concepts are covered by scenario, comparison, or recognize-only material.
- [x] Important concepts are scenario or comparison questions, including FIFO multi-client failure, POSIX MQ notification semantics, semaphore ownership/deadlock/starvation, shared-memory protocol, stale cleanup, permissions, namespaces, and embedded watchdog restart workflow.
- [x] Priority A answers include testing intent, strong answer, mechanism, pitfalls, debug angle, and follow-up keywords.
- Remaining coverage gaps: none found.
