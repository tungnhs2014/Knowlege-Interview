# IPC Overview — Interprocess Communication Taxonomy

## Problem It Solves

Linux enforces **strict process isolation**: each process has its own virtual address space and
cannot directly read or write another process's memory. This is a fundamental OS security
boundary.

However, cooperating processes need to exchange data and coordinate actions — a web server
spawns workers, a pipeline feeds output of one program as input to another, a sensor daemon
shares readings with a display daemon. IPC is the set of kernel-provided mechanisms that let
processes cross isolation boundaries in a **controlled, safe** manner.

Without IPC:
- Processes are islands — no data sharing, no coordination
- The only alternative would be files, but files have high I/O overhead and no synchronization semantics

---

## Concept Overview

**Interprocess Communication (IPC)** is the umbrella term for all mechanisms that allow
processes (or threads) to communicate and synchronize. IPC facilities are provided by the
kernel and fall into three broad functional categories:

| Category | Purpose | Examples |
|---|---|---|
| **Communication** | Exchange data between processes | Pipes, FIFOs, MQ, Shared Memory |
| **Synchronization** | Coordinate timing / mutual exclusion | Semaphores, File Locks, Mutexes |
| **Signals** | Asynchronous notification | `kill()`, real-time signals |

Key dimensions when choosing an IPC mechanism:
1. **Namespace** — how processes identify/find the IPC object
2. **Handle** — how to refer to the object in code
3. **Data model** — byte stream vs. discrete messages
4. **Persistence** — how long the object lives
5. **Accessibility** — which processes can use it
6. **Network transparency** — local only vs. across hosts

---

## System Context

IPC sits at the intersection of several kernel subsystems:

```
User Space                    Kernel Space
─────────────                 ──────────────────────────────────
Process A ──write(fd)──►      VFS layer
                              └── pipe/FIFO/socket buffer
Process B ──read(fd)──◄       └── copy_to_user / copy_from_user

Process C ─────────────►      VM subsystem
Process D ◄─────────────      └── page table entries → same physical frames
                                   (shared memory — NO copy)

Process E ──msgget()──►       IPC subsystem (ipc/msg.c, ipc/sem.c, ipc/shm.c)
                              └── System V objects (kernel-persistent)
```

The UNIX philosophy of "everything is a file" applies to most IPC facilities:
- Pipes, FIFOs, sockets, POSIX IPC → **file descriptors** → work with `select()`/`poll()`/`epoll()`
- System V IPC → **IPC identifiers** (integers outside fd space) → do NOT work with fd-based I/O multiplexing

---

## Internal Mechanism

### Data Transfer: Two-Copy Model

```
Process A (writer)         Kernel Buffer           Process B (reader)
[user space buf]──copy①──►[kernel space buf]──copy②──►[user space buf]
    write(fd, ...)                                      read(fd, ...)
```

- **Copy ①**: `write()` → syscall → `copy_from_user()` → kernel buffer
- **Copy ②**: `read()` → syscall → `copy_to_user()` → user buffer
- Kernel handles **flow control** (block writer when buffer full) and **synchronization**
  (block reader when buffer empty) automatically

### Shared Memory: Zero-Copy Model

```
Process A virtual space     Physical RAM        Process B virtual space
[vaddr 0xA000]──PTE──►[page frame #N]◄──PTE──[vaddr 0xB000]
```

The kernel sets page table entries (PTEs) in both processes to point to the **same physical
page frames**. There is no data copy — reads/writes go directly to RAM. Consequence: the
kernel provides **no synchronization** — the application must use semaphores or mutexes.

### System V IPC Key Generation: ftok()

```c
key_t ftok(const char *pathname, int proj_id);
```

Combines the inode number of `pathname` and the low 8 bits of `proj_id` into a `key_t`
integer. Two unrelated processes obtain the same key by agreeing on the same `(pathname,
proj_id)` pair — this is the "naming convention" for System V IPC.

Risk: if the file is deleted and recreated, its inode changes → different key → processes
cannot find each other's IPC objects.

### Byte Stream vs. Message Semantics

**Byte stream** (Pipe, FIFO, TCP socket):
```
writer: write("AB")   write("CD")   write("EF")
        ──────────────────────────────────────► kernel buffer: [ABCDEF]
reader: read() might return "ABCDEF" in one call — boundaries are LOST
```

**Message** (System V MQ, POSIX MQ, UDP socket):
```
writer: send("AB")    send("CD")    send("EF")
        ──────────────────────────────────────► queue: [AB][CD][EF]
reader: recv() always returns exactly ONE message — boundaries PRESERVED
```

---

## Architecture

### Full IPC Taxonomy

```
IPC Facilities
├── Communication
│   ├── Data Transfer (kernel acts as intermediary)
│   │   ├── Byte Stream
│   │   │   ├── Pipe (anonymous, related processes only)
│   │   │   ├── FIFO / Named Pipe (filesystem name, any processes)
│   │   │   └── Stream Socket (UNIX domain or TCP/IP)
│   │   └── Message (discrete, boundary-preserving)
│   │       ├── System V Message Queue (integer key, kernel-persistent)
│   │       ├── POSIX Message Queue (pathname, fd-like, priority support)
│   │       └── Datagram Socket (UNIX domain or UDP/IP)
│   │
│   └── Shared Memory (direct RAM access, zero-copy)
│       ├── System V Shared Memory  shmget/shmat/shmdt/shmctl
│       ├── POSIX Shared Memory     shm_open + mmap
│       └── Memory Mapping
│           ├── Anonymous mapping   mmap(MAP_ANONYMOUS) — related processes
│           └── File-backed mapping mmap(file fd) — file-system persistence
│
├── Synchronization
│   ├── Semaphores
│   │   ├── System V Semaphore  semget/semop/semctl (semaphore sets)
│   │   └── POSIX Semaphore
│   │       ├── Named            sem_open/sem_wait/sem_post/sem_unlink
│   │       └── Unnamed          sem_init/sem_wait/sem_post/sem_destroy
│   ├── File Locks
│   │   ├── flock()   whole-file lock
│   │   └── fcntl()   record (byte-range) lock, deadlock detection
│   └── Thread Synchronization (Chapter 6)
│       ├── Mutex        pthread_mutex_*
│       └── Cond Var     pthread_cond_*
│
└── Signals (async notification, limited data)
    ├── Standard signals   kill(), raise()
    └── Real-time signals  sigqueue() — can carry integer or pointer payload
```

### IPC Object Identification & Handles

| Facility | Identifier (naming) | Handle in code |
|---|---|---|
| Pipe | no name | `int fd` |
| FIFO | filesystem pathname | `int fd` |
| UNIX domain socket | filesystem pathname | `int fd` (socket) |
| Internet socket | IP address + port | `int fd` (socket) |
| System V message queue | `key_t` (ftok) | `int msqid` |
| System V semaphore | `key_t` (ftok) | `int semid` |
| System V shared memory | `key_t` (ftok) | `int shmid` |
| POSIX message queue | `/name` pathname | `mqd_t` (fd on Linux) |
| POSIX named semaphore | `/name` pathname | `sem_t *` |
| POSIX unnamed semaphore | no name (in SHM) | `sem_t *` |
| POSIX shared memory | `/name` pathname | `int fd` → mmap |
| Anonymous mapping | no name | pointer from mmap |
| Memory-mapped file | filesystem pathname | `int fd` → mmap |

### Persistence Model

| Type | Facilities | Lifetime |
|---|---|---|
| **Process persistence** | Pipe, FIFO, Socket, Anonymous mapping | Destroyed when last fd closed |
| **Kernel persistence** | System V IPC, POSIX IPC | Until explicit delete or system reboot |
| **File-system persistence** | File-backed mmap | Until file is deleted from filesystem |

---

## Execution Flow

### Flow: Pipe between Parent and Child

```
parent                          kernel                          child
  │                               │                               │
  ├──pipe(pipefd[2])──────────────►│ allocate pipe buffer (64KB)   │
  │◄──[pipefd[0]=3, pipefd[1]=4]──┤                               │
  │                               │                               │
  ├──fork()──────────────────────►│                               │
  │                               │──────── fork() ──────────────►│
  │                               │   child inherits pipefd[0,1]  │
  │                               │                               │
  ├──close(pipefd[0])             │               close(pipefd[1])┤
  │  (close read end)             │               (close write end)│
  │                               │                               │
  │                               │◄──write(pipefd[1], data, n)───┤
  │                               │ copy_from_user → pipe buffer  │
  │                               │                               │
  ├──read(pipefd[0], buf, BUF)───►│                               │
  │                               │ copy_to_user ← pipe buffer    │
  │◄──returns n bytes─────────────┤                               │
  │                               │                               │
  ├──close(pipefd[0])             │               close(pipefd[1])┤
  │                               │ pipe buffer freed (no readers/│
  │                               │ writers remaining)            │
```

### Flow: System V IPC Lifecycle

```
Process A                       Kernel IPC subsystem            Process B
  │                               │                               │
  ├──ftok("/tmp",'A')────────────►│ combine inode + proj_id       │
  │◄──key = 0x41XXXXXX────────────┤                               │
  │                               │                               │
  ├──msgget(key, IPC_CREAT|0640)─►│ create MQ, store in ipc_ids   │
  │◄──msqid = 3───────────────────┤ (kernel-persistent object)    │
  │                               │                               │
  ├──msgsnd(msqid, &msg, ...)────►│ enqueue message               │
  │                               │                               │
  │                          [A exits — object SURVIVES]          │
  │                               │                               │
  │                               │◄──ftok("/tmp",'A')────────────┤
  │                               │◄──msgget(key, 0)──────────────┤
  │                               │   same key → same msqid=3    │
  │                               │──msgrcv(msqid,...) ──────────►│
  │                               │ dequeue message               │
  │                               │                               │
  │                               │◄──msgctl(msqid, IPC_RMID)─────┤
  │                               │ destroy object                │
```

### Flow: Shared Memory Setup

```
Process A                       Kernel VM subsystem             Process B
  │                               │                               │
  ├──shmget(key, size, IPC_CREAT)►│ allocate page frames          │
  │◄──shmid──────────────────────┤                               │
  │                               │                               │
  ├──shmat(shmid, NULL, 0)───────►│ map pages into A's VMA        │
  │◄──ptr_a (e.g. 0x7f000000)────┤ PTE: vaddr→physical frames    │
  │                               │                               │
  │                               │◄─shmat(shmid, NULL, 0)────────┤
  │                               │ map SAME pages into B's VMA   │
  │                               │──►ptr_b (e.g. 0x7e000000)────►│
  │                               │ PTE: different vaddr, SAME    │
  │                               │      physical frames          │
  │                               │                               │
  ├──*(ptr_a) = 42────────────────┤────────────────────────────►B reads 42
  │  (writes to physical RAM)     │ NO COPY — direct page access  │
```

---

## Example (Code)

### Example 1: Pipe — Byte Stream Semantics

```c
/* pipe_demo.c — byte stream: no message boundaries
 * Build: gcc -o pipe_demo pipe_demo.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUF_SIZE 64

int main(void)
{
    int pipefd[2];   /* pipefd[0]=read end, pipefd[1]=write end */

    if (pipe(pipefd) == -1) { perror("pipe"); exit(EXIT_FAILURE); }

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); exit(EXIT_FAILURE); }

    if (pid == 0) {
        /* CHILD: writer */
        close(pipefd[0]);  /* close unused read end */

        const char *msgs[] = { "Hello", " ", "World\n" };
        for (int i = 0; i < 3; i++) {
            write(pipefd[1], msgs[i], strlen(msgs[i]));
            printf("[child]  wrote: \"%s\"\n", msgs[i]);
            usleep(10000);
        }
        close(pipefd[1]);  /* EOF for reader */
        exit(EXIT_SUCCESS);
    }

    /* PARENT: reader */
    close(pipefd[1]);  /* close unused write end */

    char buf[BUF_SIZE];
    ssize_t n;
    printf("[parent] reading (3 writes may arrive as 1 read):\n");
    while ((n = read(pipefd[0], buf, BUF_SIZE)) > 0) {
        printf("[parent] read %zd bytes: \"", n);
        fwrite(buf, 1, n, stdout);
        printf("\"\n");
    }

    close(pipefd[0]);
    wait(NULL);
    return 0;
}
```

### Example 2: FIFO — Named Pipe between Unrelated Processes

```c
/* fifo_writer.c — kernel-side blocking open, process persistence
 * Build: gcc -o fifo_writer fifo_writer.c
 * Run:   ./fifo_writer (blocks until reader opens the other end)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/demo_fifo"

int main(void)
{
    /* mkfifo creates the name in filesystem — process persistence:
     * name survives close, but data is lost when all fds close */
    if (mkfifo(FIFO_PATH, 0640) == -1)
        perror("mkfifo (may already exist)");

    printf("[writer] blocking on open() until reader connects...\n");

    /* O_WRONLY on FIFO blocks until another process opens O_RDONLY */
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) { perror("open"); exit(EXIT_FAILURE); }

    printf("[writer] reader connected, sending messages\n");

    const char *msgs[] = {
        "MSG1: unrelated processes via filesystem name",
        "MSG2: byte stream — boundaries not preserved",
        "MSG3: last message",
    };
    for (int i = 0; i < 3; i++) {
        write(fd, msgs[i], strlen(msgs[i]) + 1);  /* include NUL */
        printf("[writer] sent: %s\n", msgs[i]);
        sleep(1);
    }

    close(fd);
    return 0;
}
```

```c
/* fifo_reader.c
 * Build: gcc -o fifo_reader fifo_reader.c
 * Run:   open second terminal, then ./fifo_reader
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/demo_fifo"

int main(void)
{
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) { perror("open"); exit(EXIT_FAILURE); }

    char buf[256];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        printf("[reader] received (%zd bytes): %s\n", n, buf);

    close(fd);
    unlink(FIFO_PATH);  /* remove the name from filesystem */
    printf("[reader] FIFO name unlinked\n");
    return 0;
}
```

### Example 3: System V vs POSIX IPC — Namespace & Handle Comparison

```c
/* ipc_identify.c — demonstrates naming and handle differences
 * Build: gcc -o ipc_identify ipc_identify.c -lrt
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(void)
{
    /* ---------- System V IPC ---------- */
    printf("=== System V: Integer key + IPC identifier ===\n");

    /* ftok: (pathname, proj_id) → key_t integer
     * Two processes agree on same (file, proj_id) → same key */
    key_t key = ftok("/tmp", 'A');
    if (key == -1) { perror("ftok"); exit(EXIT_FAILURE); }
    printf("ftok(\"/tmp\", 'A') = 0x%x\n", key);

    /* msgget returns an IPC id — NOT a file descriptor */
    int msqid = msgget(key, IPC_CREAT | 0640);
    if (msqid == -1) { perror("msgget"); exit(EXIT_FAILURE); }
    printf("msgget() IPC id = %d  (run 'ipcs -q' to verify)\n", msqid);
    printf("  -> cannot use select()/poll()/epoll() on IPC id\n\n");

    /* Kernel persistence: must EXPLICITLY remove */
    msgctl(msqid, IPC_RMID, NULL);
    printf("Deleted with msgctl(IPC_RMID)\n\n");

    /* ---------- POSIX IPC ---------- */
    printf("=== POSIX: Pathname + file descriptor ===\n");

    const char *name = "/demo_ipc_compare";
    struct mq_attr attr = { .mq_maxmsg = 5, .mq_msgsize = 64 };

    /* mq_open returns mqd_t — on Linux this IS a real fd */
    mqd_t mqd = mq_open(name, O_CREAT | O_RDWR, 0640, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open"); exit(EXIT_FAILURE); }
    printf("mq_open(\"%s\") = fd %d\n", name, (int)mqd);
    printf("  -> visible at /dev/mqueue%s\n", name);
    printf("  -> CAN use epoll() on this fd (Linux)\n\n");

    /* mq_unlink: removes the name but object lives while mqd is open
     * (reference counting — unlike System V which has no refcount) */
    mq_unlink(name);
    printf("mq_unlink() called — object still alive (refcount > 0)\n");
    mq_close(mqd);
    printf("mq_close() — refcount = 0, object destroyed\n\n");

    /* ---------- Summary ---------- */
    printf("%-22s %-18s %-18s\n", "Aspect", "System V", "POSIX");
    printf("%-22s %-18s %-18s\n", "Namespace",    "key_t (ftok)",    "pathname (/name)");
    printf("%-22s %-18s %-18s\n", "Handle",       "int (IPC id)",    "fd / mqd_t");
    printf("%-22s %-18s %-18s\n", "epoll/select", "NO",              "YES (Linux)");
    printf("%-22s %-18s %-18s\n", "Ref count",    "NO",              "YES");
    printf("%-22s %-18s %-18s\n", "Inspect",      "ipcs",            "/dev/mqueue/");
    printf("%-22s %-18s %-18s\n", "Delete",       "msgctl IPC_RMID", "mq_unlink");

    return 0;
}
```

---

## Debugging

### List all live System V IPC objects
```bash
ipcs          # all: MQ, semaphores, shared memory
ipcs -q       # message queues only
ipcs -s       # semaphores only
ipcs -m       # shared memory only
ipcs -l       # system-wide limits (msgmni, semmni, shmmni, etc.)
```

### Remove a specific System V IPC object
```bash
ipcrm -q <msqid>    # remove message queue by id
ipcrm -s <semid>    # remove semaphore set
ipcrm -m <shmid>    # remove shared memory segment
ipcrm -Q <key>      # remove MQ by key
```

### List all POSIX message queues
```bash
ls /dev/mqueue/
cat /dev/mqueue/<name>   # shows attributes: QSIZE, NOTIFY, etc.
```

### Check POSIX shared memory objects
```bash
ls /dev/shm/
```

### Inspect pipe/FIFO file descriptors in a process
```bash
ls -la /proc/<pid>/fd/   # see all open fds including pipes
cat /proc/<pid>/fdinfo/<fd>  # details (pipe inode, pos, etc.)
```

### System V IPC limits (if getting ENOSPC / ENOMEM)
```bash
cat /proc/sys/kernel/msgmni   # max number of message queues
cat /proc/sys/kernel/msgmax   # max message size
cat /proc/sys/kernel/msgmnb   # max bytes per queue
sysctl -a | grep msg          # all msg-related limits
```

### Detect leaked IPC objects (common production issue)
```bash
# Check if IPC objects are accumulating over time
watch -n 5 'ipcs | wc -l'

# Find who created an object (using creator PID in ipcs -c)
ipcs -c -q    # show creator and owner PIDs for message queues
```

---

## Real-world Usage

### Pipe: Unix pipeline (sh, bash)
```bash
ls -la | grep ".c" | wc -l
# Each | creates an anonymous pipe between adjacent processes
# The shell: pipe() → fork() × N → dup2(pipe_fd, STDOUT_FILENO)
```

### FIFO: Simple logging daemon
A syslog-style daemon listens on a named FIFO (`/dev/log` historically used a Unix socket,
but many embedded systems use FIFOs). Multiple application processes write to the same FIFO
path; the daemon reads and routes entries. Process persistence ensures no leftover data
between daemon restarts.

### System V Shared Memory: PostgreSQL buffer pool
PostgreSQL uses System V shared memory (`shmget`/`shmat`) for its shared buffer cache — the
area where all backend processes access cached disk pages. The entire buffer pool is one large
SHM segment. Semaphores protect buffer metadata. This is one of the largest real-world
deployments of System V IPC (though newer PostgreSQL supports POSIX SHM as well).

### POSIX MQ: Real-time task dispatch
In embedded real-time systems (Linux with `PREEMPT_RT`), POSIX message queues with
`mq_notify()` are used to wake a thread when a high-priority task message arrives, without
polling — `mq_notify` can deliver a signal or spawn a thread on message arrival.

### eventfd: Kernel → userspace notification
Since Linux 2.6.22, `eventfd()` creates a counter-based fd that integrates with `epoll`.
Used by KVM for VM exit notification, by io_uring for completion notification, and by
container runtimes for signaling between threads/processes — without the full weight of a
socket or pipe.

---

## Key Takeaways

1. **IPC = crossing process isolation under kernel control.** The kernel enforces boundaries;
   IPC is the controlled gateway, not a bypass.

2. **Two fundamental models:**
   - *Data transfer* → 2 copies through kernel, automatic sync/flow-control
   - *Shared memory* → 0 copies, manual synchronization required

3. **Byte stream vs. message:**
   - Pipes/FIFOs/TCP: boundaries are lost — application must frame its own protocol
   - MQ/UDP: each send/recv is one atomic unit — boundaries are preserved

4. **System V IPC vs POSIX IPC:**
   - System V: integer key (`ftok`), IPC id, no fd → no `epoll`, no refcount, manual cleanup
   - POSIX: pathname, fd-like handle → integrates with `epoll`, refcount-based cleanup

5. **Persistence is a design decision:**
   - Process persistence (Pipe, FIFO): clean lifecycle, no zombie objects
   - Kernel persistence (Sys V, POSIX IPC): survives process crash (useful), but can leak

6. **Choose IPC based on requirements:**
   - Related processes, simple: anonymous pipe (`pipe()`)
   - Unrelated processes, local only: FIFO or POSIX MQ
   - High-throughput large data: Shared Memory + Semaphore
   - Network-capable: Socket (UNIX domain → TCP/IP with minimal changes)
   - I/O multiplexing needed: avoid System V, use POSIX or sockets

7. **Common pitfall — System V resource leak:** Always delete with `msgctl/semctl/shmctl`
   `IPC_RMID` in signal handlers (`SIGTERM`, `SIGINT`) and `atexit()` handlers. On long-running
   production servers, forgotten objects accumulate until `ENOSPC` is hit.
