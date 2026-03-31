# POSIX IPC — Message Queues, Semaphores, Shared Memory

## Problem It Solves

System V IPC (Chapter ch07_ipc_sysv.md) suffers three core design deficiencies:
1. **Integer key namespace** (`ftok`) — inconsistent with Unix file model
2. **IPC identifier ≠ file descriptor** — cannot use `select()`/`poll()`/`epoll()`
3. **No reference counting** — impossible to safely determine when to delete objects

POSIX IPC (POSIX.1b realtime extensions, 1993) was designed specifically to fix all three:
- **Pathname namespace** (`/name`) — same convention as files
- **fd-like handles** — integrate with existing Unix I/O model and multiplexing
- **Reference counting** — safe unlink-then-close pattern (like `unlink()` on files)

Additionally, POSIX semaphores fix the System V race between semaphore creation and
initialization by making `sem_open()` atomic (create + initialize in one call).

---

## Concept Overview

POSIX IPC provides three mechanisms under a unified design:

| Mechanism | Header | Handle | Create | Delete |
|---|---|---|---|---|
| **Message Queue** | `<mqueue.h>` | `mqd_t` | `mq_open()` | `mq_unlink()` |
| **Named Semaphore** | `<semaphore.h>` | `sem_t *` | `sem_open()` | `sem_unlink()` |
| **Unnamed Semaphore** | `<semaphore.h>` | `sem_t *` | `sem_init()` | `sem_destroy()` |
| **Shared Memory** | `<sys/mman.h>` | `int fd` | `shm_open()` + `mmap()` | `shm_unlink()` |

**Common pattern** (mirrors standard Unix file I/O):
```
open  → use  → close  → unlink
```

**Compile flag**: all POSIX IPC programs must link with `-lrt` (realtime library) on Linux.

**Filesystem visibility**:
```
/dev/mqueue/    ← POSIX message queue objects
/dev/shm/       ← POSIX shared memory objects + named semaphores (sem.name)
```

---

## System Context

```
User Space                      Kernel / tmpfs
────────────────                ───────────────────────────────────────
mq_open("/myqueue")────────────►/dev/mqueue/myqueue (tmpfs inode)
  returns: mqd_t = fd 5         mqd_t IS a real Linux fd
                                 → usable with epoll/select/poll

sem_open("/mysem")─────────────►/dev/shm/sem.mysem (tmpfs inode)
  returns: sem_t *               kernel manages sem_t storage

shm_open("/myshm")─────────────►/dev/shm/myshm (tmpfs inode, starts at 0 bytes)
  returns: int fd = 6
ftruncate(fd, size)─────────────►allocates pages (zero-initialized)
mmap(NULL,size,PROT_RW,         maps physical pages into process VMA
     MAP_SHARED, fd, 0)─────────►ptr valid in this process

Reference counting:
  Each open() increments refcount
  Each close()/munmap() decrements refcount
  unlink() removes name; object freed when refcount → 0
```

On Linux, POSIX IPC objects are implemented as inodes in **tmpfs** filesystems:
- `tmpfs` = virtual filesystem backed by RAM + swap (no disk I/O)
- Kernel-persistent: survives process exit, lost on system reboot
- Size of `/dev/shm` is bounded by `/proc/sys/kernel/shmall` (for SHM) and mount options

---

## Internal Mechanism

### POSIX Message Queue: Priority-Ordered Queue

```c
struct mq_attr {
    long mq_flags;    /* 0 or O_NONBLOCK */
    long mq_maxmsg;   /* max messages in queue  (set at creation) */
    long mq_msgsize;  /* max bytes per message  (set at creation) */
    long mq_curmsgs;  /* current message count  (read-only) */
};
```

Internal kernel structure: a **priority-sorted list** of messages:
```
Queue state after sending prio=1, prio=10, prio=5:
  [prio=10 | data] → [prio=5 | data] → [prio=1 | data] → NULL

mq_receive() ALWAYS returns highest-priority message first.
Same-priority messages are FIFO ordered.
```

`mq_send(mqd, buf, len, priority)`: `priority` is an unsigned int (0 = lowest).
`mq_receive(mqd, buf, msgsize, &prio)`: returns highest-priority available message.

**`mq_notify()` — one-shot asynchronous notification**:
```c
struct sigevent sev = {
    .sigev_notify = SIGEV_SIGNAL,  /* or SIGEV_THREAD */
    .sigev_signo  = SIGUSR1,
};
mq_notify(mqd, &sev);
/* When queue transitions from EMPTY → non-empty: deliver SIGUSR1
   Registration is ONE-SHOT — must re-register in handler */
```

**Linux-specific**: `mqd_t` is implemented as a real file descriptor.
```c
/* epoll works on mqd_t on Linux (non-portable but practical) */
epoll_ctl(epfd, EPOLL_CTL_ADD, (int)mqd, &ev);
```

### Named Semaphore: Atomic Create + Init

```c
sem_t *sem = sem_open("/name", O_CREAT, 0640, initial_value);
/* Create AND initialize atomically — no System V two-step race */
```

Stored as `/dev/shm/sem.name` (note: kernel prepends `sem.`).

Operations (always by exactly 1):
```c
sem_wait(sem);         /* P: decrement; block if value == 0 */
sem_trywait(sem);      /* P non-blocking: EAGAIN if value == 0 */
sem_timedwait(sem, &abs_timeout); /* P with deadline: ETIMEDOUT */
sem_post(sem);         /* V: increment; wake one blocked waiter */
sem_getvalue(sem, &v); /* read current value (may be stale) */
```

Lifecycle:
```
sem_open(O_CREAT)  → name in /dev/shm/sem.*, refcount=1
sem_open(no flags) → open existing, refcount++
sem_close()        → refcount--
sem_unlink()       → remove name; destroyed when refcount==0
```

### Unnamed Semaphore: Lives in Shared Memory

```c
sem_t sem;
sem_init(&sem, pshared, initial_value);
```

- `pshared = 0`: shared between **threads** of the same process (sem can be anywhere)
- `pshared = 1`: shared between **processes** — sem MUST be in shared memory

```
pshared=1 requires shared memory because:

Process A heap:  vaddr 0x600000 → physical frame #10
Process B heap:  vaddr 0x600000 → physical frame #55  ← DIFFERENT!

sem_t at A's 0x600000 would point to frame #10;
B reading 0x600000 gets frame #55 → operating on different data → broken

Solution: put sem_t in MAP_SHARED region:
  vaddr 0x700000 (A) → physical frame #42 ← same frame
  vaddr 0x700000 (B) → physical frame #42 ←
```

Three valid ways to place unnamed semaphore for inter-process use:
```c
/* Option 1: MAP_SHARED | MAP_ANONYMOUS (for parent-child via fork) */
sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE,
                  MAP_SHARED|MAP_ANONYMOUS, -1, 0);

/* Option 2: Inside POSIX SHM */
int fd = shm_open("/myshm", O_CREAT|O_RDWR, 0640);
ftruncate(fd, sizeof(sem_t) + data_size);
sem_t *sem = mmap(NULL, sizeof(sem_t)+data_size, PROT_READ|PROT_WRITE,
                  MAP_SHARED, fd, 0);

/* Option 3: Inside System V SHM */
sem_t *sem = shmat(shmid, NULL, 0);
```

### POSIX Shared Memory: Two-Step by Design

```c
/* Step 1: Create name + fd in tmpfs (initially 0 bytes) */
int fd = shm_open("/name", O_CREAT|O_RDWR, 0640);

/* Step 2: Set size — allocates physical pages (zero-initialized) */
ftruncate(fd, size);

/* Step 3: Map into virtual address space */
void *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

/* Step 4: Close fd — mapping is independent of fd */
close(fd);

/* Step 5: Use ptr directly — zero kernel involvement per access */
((MyStruct*)ptr)->field = value;

/* Step 6: Unmap when done */
munmap(ptr, size);

/* Step 7: Destroy name */
shm_unlink("/name");
```

**Why two-step (shm_open + mmap)?**
`mmap()` already existed in POSIX. The committee reused it rather than creating a new
syscall. This gives SHM access to all mmap flags: `PROT_READ`, `PROT_EXEC`, `MAP_PRIVATE`
(for copy-on-write semantics), `off_t offset` (map subregion), and `ftruncate()` for
dynamic resize.

**Unlink-then-close pattern** (the key advantage over System V):
```c
fd = shm_open("/name", O_CREAT|O_RDWR, 0640);
ftruncate(fd, size);
ptr = mmap(..., fd, 0);
shm_unlink("/name");   /* name removed NOW — no new process can find it */
close(fd);             /* fd no longer needed — mapping survives */
/* ptr remains valid until munmap() */
```
This prevents name collisions and resource leaks without requiring explicit cleanup at exit.

---

## Architecture

### Unified POSIX IPC Pattern

```
┌──────────────────────────────────────────────────────────────────────┐
│                POSIX IPC — UNIFIED OPEN/CLOSE/UNLINK MODEL           │
│                                                                        │
│  Namespace: /name (pathname, consistent with Unix file model)          │
│  Visibility: /dev/mqueue/ (MQ)  /dev/shm/ (SHM + named sem)          │
│                                                                        │
│  mq_open("/q",O_CREAT,mode,&attr) ──► mqd_t (= real fd on Linux)    │
│  mq_send / mq_receive / mq_notify                                     │
│  mq_close(mqd) / mq_unlink("/q")                                     │
│                                                                        │
│  sem_open("/s",O_CREAT,mode,val)  ──► sem_t * (managed by kernel)   │
│  sem_wait / sem_post / sem_trywait / sem_timedwait                   │
│  sem_close(sem) / sem_unlink("/s")                                   │
│                                                                        │
│  sem_init(&sem, pshared, val)     ──► sem_t * (in shared memory)    │
│  sem_wait / sem_post                                                  │
│  sem_destroy(&sem)                                                   │
│                                                                        │
│  shm_open("/m",O_CREAT|O_RDWR,mode) ──► int fd                      │
│  ftruncate(fd, size)                                                  │
│  mmap(NULL,size,PROT_RW,MAP_SHARED,fd,0) ──► void *ptr              │
│  close(fd)                                                            │
│  munmap(ptr,size) / shm_unlink("/m")                                 │
│                                                                        │
│  All: kernel persistence (survive process exit, lost on reboot)       │
│  All: reference counted (unlink removes name, object lives until      │
│       refcount == 0)                                                   │
│  Link: -lrt                                                           │
└──────────────────────────────────────────────────────────────────────┘
```

### System V vs POSIX Comparison

| Aspect | System V | POSIX |
|---|---|---|
| Namespace | `key_t` (ftok) | `/name` pathname |
| Handle | `int` IPC id | fd / `mqd_t` / `sem_t *` |
| `epoll`/`select` | NO | YES (MQ = real fd on Linux) |
| Reference counting | NO | YES |
| Unlink-then-close | NO | YES |
| Semaphore init race | YES (two steps) | NO (atomic in `sem_open`) |
| Semaphore granularity | Set of N semaphores | Single semaphore |
| Atomic multi-sem op | YES (`semop`) | NO (one at a time) |
| Message priority | `mtype` (any long > 0) | unsigned int (0=lowest) |
| MQ notification | NO | YES (`mq_notify`) |
| Visibility | `ipcs` command | `ls /dev/shm/`, `ls /dev/mqueue/` |
| Delete method | `ipcrm` / `xxxctl(IPC_RMID)` | `xxx_unlink()` / `rm /dev/shm/` |
| Portability | High (XSI POSIX) | Medium (Linux 2.6+, NPTL) |
| Link flags | none | `-lrt` |

### POSIX MQ Priority Queue Layout

```
Queue: mq_maxmsg=10, mq_msgsize=128

After mq_send with priorities 1, 10, 5, 10, 3:

  ┌──────────────┐
  │ prio=10 data │ ← mq_receive returns this first
  ├──────────────┤
  │ prio=10 data │
  ├──────────────┤
  │ prio= 5 data │
  ├──────────────┤
  │ prio= 3 data │
  ├──────────────┤
  │ prio= 1 data │ ← last to be returned
  └──────────────┘

Same-priority messages maintain FIFO order within their priority level.
```

### POSIX SHM Lifecycle

```
shm_open(O_CREAT)
    │ creates inode in /dev/shm/ tmpfs
    │ file size = 0, refcount = 1
    ▼
ftruncate(fd, size)
    │ kernel allocates zero-initialized pages
    │ file size = size
    ▼
mmap(MAP_SHARED, fd)
    │ kernel maps pages into process VMA
    │ mmap_count++
    ▼
close(fd)           ← fd closed, mapping UNAFFECTED
    │ refcount-- (but mmap_count > 0)
    ▼
shm_unlink("/name") ← name removed from /dev/shm/
    │ refcount -- (if only one opener)
    │ if refcount==0 AND mmap_count==0: pages freed
    │ if mmap_count > 0: pages remain (mapping still valid)
    ▼
munmap(ptr, size)   ← process unmaps
    │ mmap_count--
    │ if mmap_count==0 AND refcount==0: pages freed NOW
    ▼
  [object destroyed]
```

---

## Execution Flow

### Flow: POSIX MQ — send/receive with priority + epoll

```
main process                    Kernel MQ subsystem          producer child
    │                               │                               │
    ├──mq_open(O_CREAT|O_RDONLY)──►│ create tmpfs inode            │
    │◄──mqd=3────────────────────── │ /dev/mqueue/demo              │
    │                               │                               │
    ├──fork()──────────────────────►│─────────────────────────────►│
    │                               │                               │
    │  epoll_create1()              │◄──mq_open(name,O_WRONLY)──────┤
    │  epoll_ctl(ADD, mqd)          │◄──mq_send(mqd,"HIGH",len,10)──┤ prio=10
    │                               │ insert at head of prio queue  │
    │  epoll_wait() ───────────────►│◄──mq_send(mqd,"LOW",len,1)───┤ prio=1
    │  unblocks when msg arrives    │ insert after prio=10          │
    │                               │                               │
    ├──mq_receive(O_NONBLOCK)──────►│ dequeue highest prio (10)     │
    │◄──"HIGH",prio=10──────────────┤                               │
    ├──mq_receive(O_NONBLOCK)──────►│ dequeue next (prio=1)         │
    │◄──"LOW",prio=1────────────────┤                               │
    ├──mq_receive(O_NONBLOCK)──────►│ queue empty → EAGAIN          │
    │                               │                               │
    ├──mq_close(mqd)───────────────►│ refcount--                    │
    ├──mq_unlink(name)─────────────►│ name removed, freed (rc==0)   │
```

### Flow: POSIX SHM — unlink-then-close pattern

```
Process A (creator)             Kernel tmpfs             Process B (consumer)
    │                               │                           │
    ├──shm_open(O_CREAT|O_RDWR)───►│ create /dev/shm/myshm    │
    │◄──fd=4────────────────────────┤ size=0, refcount=1        │
    │                               │                           │
    ├──ftruncate(fd, 4096)─────────►│ allocate 1 page (zeroed)  │
    ├──mmap(MAP_SHARED,fd)─────────►│ map page into A's VMA     │
    │◄──ptr_a=0x7f000000────────────┤ mmap_count=1              │
    ├──shm_unlink("/myshm")────────►│ name removed from /dev/shm│
    │                               │ refcount=0, mmap_count=1  │
    │                               │ (pages NOT freed yet)     │
    ├──close(fd)────────────────────┤ fd closed (no effect)     │
    │                               │                           │
    │  /* ptr_a still valid! */     │                           │
    ├──*(ptr_a) = 42────────────────►│ write to physical page    │
    │                               │                           │
    │  /* pass ptr via fork/pipe */ │◄──fork() or mmap before unlink
    │                               │──────────────────────────►│
    │                               │ map same page into B's VMA│
    │                               │──ptr_b=0x7e000000────────►│
    │                               │ B reads *(ptr_b) == 42    │
    │                               │                           │
    ├──munmap(ptr_a, 4096)─────────►│ mmap_count--: 1→0         │
    │                               │ refcount==0 AND mmap_count==0
    │                               │ → physical page freed     │
```

### Flow: Unnamed Semaphore in MAP_SHARED

```
parent                          Kernel VM                      child
    │                               │                               │
    ├──mmap(MAP_SHARED|MAP_ANON)───►│ allocate anonymous page       │
    │◄──shd=0x7f000000──────────────┤ page NOT backed by any file   │
    │                               │                               │
    ├──sem_init(&shd->mutex,1,1)───►│ initialize sem_t in page      │
    │  (pshared=1: cross-process)   │                               │
    │                               │                               │
    ├──fork()──────────────────────►│─────────────────────────────►│
    │                               │ child inherits mapping:       │
    │                               │ shd points to same page       │
    │                               │                               │
    ├──sem_wait(&shd->mutex)───────►│ futex syscall on page         │
    │  [critical section]           │◄──sem_wait(&shd->mutex)───────┤ BLOCKS
    ├──sem_post(&shd->mutex)───────►│ futex wake                    │
    │                               │──────────────────────────────►│ unblocks
```

---

## Example (Code)

### Example 1: POSIX Message Queue — Priority Queue + epoll

```c
/* posix_mq_demo.c
 * mq_open, mq_send/receive with priority, epoll on mqd_t (Linux)
 * Build: gcc -o posix_mq_demo posix_mq_demo.c -lrt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <errno.h>

#define MQ_NAME  "/demo_posix_mq"
#define MSG_SIZE 128
#define MAX_MSGS 10

static void producer(void)
{
    mqd_t mqd = mq_open(MQ_NAME, O_WRONLY);
    if (mqd == (mqd_t)-1) { perror("mq_open producer"); _exit(EXIT_FAILURE); }

    struct { unsigned int prio; const char *text; } tasks[] = {
        {  1, "priority-1  LOW:  cleanup logs" },
        { 10, "priority-10 HIGH: payment transaction" },
        {  5, "priority-5  MED:  refresh cache" },
        { 10, "priority-10 HIGH: user authentication" },
        {  3, "priority-3  LOW:  send report" },
    };

    for (int i = 0; i < 5; i++) {
        mq_send(mqd, tasks[i].text, strlen(tasks[i].text) + 1, tasks[i].prio);
        printf("[producer] sent (prio=%2u): %s\n", tasks[i].prio, tasks[i].text);
        usleep(50000);
    }
    mq_close(mqd);
    _exit(EXIT_SUCCESS);
}

int main(void)
{
    mq_unlink(MQ_NAME);  /* remove stale object */

    struct mq_attr attr = {
        .mq_maxmsg  = MAX_MSGS,
        .mq_msgsize = MSG_SIZE,
    };

    /* O_NONBLOCK: mq_receive returns EAGAIN when empty (for drain loop) */
    mqd_t mqd = mq_open(MQ_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0640, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open"); exit(EXIT_FAILURE); }

    printf("[main] MQ '%s' created, mqd=%d\n", MQ_NAME, (int)mqd);
    printf("[main] visible: ls /dev/mqueue/\n\n");

    pid_t pid = fork();
    if (pid == 0) producer();

    sleep(1);  /* wait for producer to finish */

    /* mqd_t is a real fd on Linux → epoll works */
    int epfd = epoll_create1(0);
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = (int)mqd };
    epoll_ctl(epfd, EPOLL_CTL_ADD, (int)mqd, &ev);

    printf("[consumer] waiting via epoll on mqd=%d\n\n", (int)mqd);

    struct epoll_event events[1];
    int received = 0;
    while (received < 5) {
        if (epoll_wait(epfd, events, 1, 3000) <= 0) break;

        char buf[MSG_SIZE];
        unsigned int prio;
        ssize_t n;
        /* Drain all messages — highest priority first */
        while ((n = mq_receive(mqd, buf, MSG_SIZE, &prio)) > 0) {
            printf("[consumer] recv (prio=%2u): %s\n", prio, buf);
            received++;
        }
        /* errno == EAGAIN is normal after draining */
    }

    close(epfd);
    waitpid(pid, NULL, 0);
    mq_close(mqd);
    mq_unlink(MQ_NAME);
    printf("\n[main] MQ deleted\n");
    return 0;
}
```

### Example 2: POSIX Semaphores — Named + Unnamed (pshared=1)

```c
/* posix_sem_demo.c
 * Part A: named semaphore (any process, /dev/shm/sem.*)
 * Part B: unnamed semaphore (pshared=1 in MAP_SHARED|MAP_ANONYMOUS)
 * Build: gcc -o posix_sem_demo posix_sem_demo.c -lrt -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SEM_NAME  "/demo_posix_sem"
#define N_WORKERS 4
#define N_ITERS   100

/* ---- Part A: Named semaphore ---- */

static void named_worker(void)
{
    sem_t *sem = sem_open(SEM_NAME, 0);  /* open existing */
    int shm_fd = shm_open("/demo_counter", O_RDWR, 0);
    long *ctr = mmap(NULL, sizeof(long), PROT_READ|PROT_WRITE,
                     MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    for (int i = 0; i < N_ITERS; i++) {
        sem_wait(ctr ? sem : sem);    /* P */
        long v = *ctr; usleep(1); *ctr = v + 1;
        sem_post(sem);               /* V */
    }
    munmap(ctr, sizeof(long));
    sem_close(sem);
    _exit(EXIT_SUCCESS);
}

static void demo_named(void)
{
    printf("=== Part A: Named Semaphore (/dev/shm/sem%s) ===\n\n", SEM_NAME);

    int fd = shm_open("/demo_counter", O_CREAT|O_RDWR, 0640);
    ftruncate(fd, sizeof(long));
    long *ctr = mmap(NULL, sizeof(long), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    *ctr = 0; close(fd);

    /* Atomic create + init (no System V two-step race) */
    sem_t *sem = sem_open(SEM_NAME, O_CREAT|O_EXCL, 0640, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); exit(EXIT_FAILURE); }

    int v; sem_getvalue(sem, &v);
    printf("[main] sem created, value=%d (1=unlocked)\n", v);

    pid_t pids[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) named_worker();
    }
    for (int i = 0; i < N_WORKERS; i++) waitpid(pids[i], NULL, 0);

    printf("[main] final counter=%ld  expected=%d\n\n",
           *ctr, N_WORKERS * N_ITERS);

    munmap(ctr, sizeof(long));
    sem_close(sem);
    sem_unlink(SEM_NAME);      /* unlink: name gone, freed when refcount==0 */
    shm_unlink("/demo_counter");
}

/* ---- Part B: Unnamed semaphore (pshared=1) ---- */

typedef struct { sem_t mutex; long counter; } Shared;

static void unnamed_worker(Shared *shd)
{
    for (int i = 0; i < N_ITERS; i++) {
        sem_wait(&shd->mutex);
        long v = shd->counter; usleep(1); shd->counter = v + 1;
        sem_post(&shd->mutex);
    }
    _exit(EXIT_SUCCESS);
}

static void demo_unnamed(void)
{
    printf("=== Part B: Unnamed Semaphore (pshared=1, MAP_SHARED|ANON) ===\n\n");

    /* sem_t must live in shared memory when pshared=1
     * MAP_ANONYMOUS: not backed by any file, zero-initialized */
    Shared *shd = mmap(NULL, sizeof(Shared),
                       PROT_READ|PROT_WRITE,
                       MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (shd == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    shd->counter = 0;
    /* sem_init pshared=1: usable across fork() because same physical page */
    sem_init(&shd->mutex, 1, 1);

    printf("[main] unnamed sem at %p inside MAP_SHARED|MAP_ANONYMOUS\n\n",
           (void *)&shd->mutex);

    pid_t pids[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) unnamed_worker(shd);
    }
    for (int i = 0; i < N_WORKERS; i++) waitpid(pids[i], NULL, 0);

    printf("[main] final counter=%ld  expected=%d\n",
           shd->counter, N_WORKERS * N_ITERS);

    sem_destroy(&shd->mutex);
    munmap(shd, sizeof(Shared));
}

int main(void)
{
    sem_unlink(SEM_NAME);
    shm_unlink("/demo_counter");
    demo_named();
    demo_unnamed();
    return 0;
}
```

### Example 3: POSIX Shared Memory — Producer/Consumer + unlink-then-close

```c
/* posix_shm_prodcon.c
 * shm_open + ftruncate + mmap, unlink-then-close pattern,
 * unnamed semaphores inside the SHM segment for synchronization
 * Build: gcc -o posix_shm_prodcon posix_shm_prodcon.c -lrt
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME  "/demo_posix_shm"
#define BUF_SIZE  8
#define N_ITEMS   20

/* Entire shared state lives inside one SHM segment */
typedef struct {
    sem_t empty;            /* empty slot count (init=BUF_SIZE) */
    sem_t full;             /* filled slot count (init=0) */
    sem_t mutex;            /* mutual exclusion (init=1) */
    int   buffer[BUF_SIZE];
    int   in, out;
} SharedBuf;

static void producer(SharedBuf *shm)
{
    for (int i = 1; i <= N_ITEMS; i++) {
        sem_wait(&shm->empty);
        sem_wait(&shm->mutex);

        shm->buffer[shm->in] = i;
        shm->in = (shm->in + 1) % BUF_SIZE;
        printf("[producer] wrote  item %2d   in=%d\n", i, shm->in);

        sem_post(&shm->mutex);
        sem_post(&shm->full);
        usleep(60000);
    }
    _exit(EXIT_SUCCESS);
}

static void consumer(SharedBuf *shm)
{
    for (int i = 1; i <= N_ITEMS; i++) {
        sem_wait(&shm->full);
        sem_wait(&shm->mutex);

        int item = shm->buffer[shm->out];
        shm->out = (shm->out + 1) % BUF_SIZE;
        printf("           [consumer] read  item %2d  out=%d\n",
               item, shm->out);

        sem_post(&shm->mutex);
        sem_post(&shm->empty);
        usleep(90000);
    }
    _exit(EXIT_SUCCESS);
}

int main(void)
{
    shm_unlink(SHM_NAME);

    /* Step 1: create SHM object — size 0, tmpfs inode in /dev/shm/ */
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0640);
    if (fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }

    printf("[main] '%s' created  (ls /dev/shm/ to see it)\n", SHM_NAME);

    /* Step 2: set size — kernel allocates zero-initialized pages */
    ftruncate(fd, sizeof(SharedBuf));

    /* Step 3: map into virtual address space */
    SharedBuf *shm = mmap(NULL, sizeof(SharedBuf),
                          PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    /* Step 4: close fd — mapping is independent of fd */
    close(fd);
    printf("[main] fd closed — mapping still valid\n");

    /* Step 5: unlink-then-close pattern
     * Name removed NOW; object lives while mapping exists (refcount>0) */
    shm_unlink(SHM_NAME);
    printf("[main] shm_unlink() — name gone, object alive until munmap()\n\n");

    /* Initialize semaphores inside SHM (pshared=1: for forked children) */
    shm->in = shm->out = 0;
    sem_init(&shm->empty, 1, BUF_SIZE);
    sem_init(&shm->full,  1, 0);
    sem_init(&shm->mutex, 1, 1);

    printf("[main] buf_size=%d n_items=%d\n\n", BUF_SIZE, N_ITEMS);

    pid_t ppid = fork(); if (ppid == 0) producer(shm);
    pid_t cpid = fork(); if (cpid == 0) consumer(shm);

    waitpid(ppid, NULL, 0);
    waitpid(cpid, NULL, 0);

    printf("\n[main] done. destroying.\n");
    sem_destroy(&shm->empty);
    sem_destroy(&shm->full);
    sem_destroy(&shm->mutex);
    munmap(shm, sizeof(SharedBuf));
    /* refcount → 0: physical pages freed here */
    printf("[main] munmap() — object destroyed\n");
    return 0;
}
```

---

## Debugging

### List POSIX IPC objects
```bash
ls -la /dev/mqueue/         # POSIX message queues
ls -la /dev/shm/            # POSIX SHM + named semaphores (sem.name prefix)
cat /proc/mounts | grep mqueue   # verify mqueue fs is mounted
cat /proc/mounts | grep tmpfs    # verify /dev/shm tmpfs
```

### Inspect a POSIX message queue
```bash
cat /dev/mqueue/<name>      # shows: QSIZE bytes, NOTIFY pid, SIGNO, NOTIFY_PID
# Example output:
# QSIZE:128        NOTIFY:0     SIGNO:0    NOTIFY_PID:0
```

### Remove POSIX IPC objects manually
```bash
rm /dev/mqueue/myqueue      # delete POSIX MQ
rm /dev/shm/mysem_obj       # delete POSIX SHM or named semaphore
# (regular rm works because these are real files in tmpfs)
```

### Check MQ limits
```bash
cat /proc/sys/fs/mqueue/msg_max       # max mq_maxmsg per queue
cat /proc/sys/fs/mqueue/msgsize_max   # max mq_msgsize per message
cat /proc/sys/fs/mqueue/queues_max    # max number of queues system-wide

# Increase temporarily:
sysctl -w fs.mqueue.msg_max=100
```

### Check /dev/shm size limit
```bash
df -h /dev/shm              # current usage and total size
# Resize if needed (until reboot):
mount -o remount,size=512m /dev/shm
```

### Trace POSIX IPC syscalls
```bash
strace -e trace=mq_open,mq_send,mq_receive,mq_unlink,\
            shm_open,shm_unlink,semopen,semunlink \
       ./your_program

# On Linux, POSIX MQ uses mq_* syscalls directly
# shm_open/shm_unlink are glibc wrapper around open()/unlink() on /dev/shm/
strace -e trace=openat,unlink,mmap,ftruncate ./posix_shm_prodcon
```

### Verify semaphore state
```bash
# Named semaphore file in /dev/shm/ has 16 bytes on Linux
# Cannot read value directly from file; use sem_getvalue() in code

# For debugging: write a small probe program
cat > /tmp/sem_probe.c << 'EOF'
#include <semaphore.h>
#include <stdio.h>
int main(int argc, char **argv) {
    sem_t *s = sem_open(argv[1], 0);
    int v; sem_getvalue(s, &v);
    printf("sem %s = %d\n", argv[1], v);
    sem_close(s);
}
EOF
gcc -o /tmp/sem_probe /tmp/sem_probe.c -lrt
/tmp/sem_probe /myname
```

---

## Real-world Usage

### POSIX MQ for real-time task dispatch (embedded Linux)
POSIX MQ with priorities is used in `PREEMPT_RT` Linux systems for task queues where
high-priority work (sensor interrupt handling) must preempt lower-priority work (logging).
`mq_notify(SIGEV_THREAD)` spawns a thread directly on message arrival — zero polling latency.

### POSIX SHM for zero-copy video frame sharing
Computer vision pipelines (ROS2, GStreamer, OpenCV) use POSIX SHM to share raw frame
buffers (10–100 MB) between capture, processing, and display processes. Copying 4K frames
through pipes would be prohibitive; SHM makes it zero-copy. A POSIX named semaphore
coordinates producer-consumer access to the frame buffer.

### mq_open for service discovery without a broker
Microservices on the same host use POSIX MQ as a lightweight message bus: each service
owns a queue `/myapp/service-name`, other services send requests by name. No broker
daemon needed — the kernel is the broker.

### POSIX SHM unlink-then-close for ephemeral shared state
A process creates SHM, unlinks immediately, then forks workers. Workers inherit the
mapping. After all exit, the SHM is automatically freed. No cleanup needed in signal
handlers. This pattern is used by multiprocessing libraries (Python's `multiprocessing`,
Rust's `shared_memory` crate) to avoid SHM leaks.

### Named semaphore as system-wide lock (replacing file locks)
```c
/* Any process that calls this owns the "singleton lock" */
sem_t *lock = sem_open("/myapp_singleton", O_CREAT, 0640, 1);
if (sem_trywait(lock) == -1) {
    fprintf(stderr, "Another instance is running\n");
    exit(1);
}
/* sem_unlink in atexit() */
```
Simpler than `flock()` because no file inode needed; cleaner than `fcntl()` locks.

---

## Key Takeaways

1. **POSIX IPC = System V IPC done right.** Same three mechanisms, but with pathname
   namespace, fd handles, reference counting, and atomic semaphore initialization.

2. **The unified pattern is `open → use → close → unlink`** — mirrors standard Unix file
   I/O. Once learned, it applies identically to MQ, semaphores, and SHM.

3. **`mqd_t` is a real file descriptor on Linux.** This means POSIX MQ integrates with
   `epoll`, `select`, and `poll` — a critical advantage over System V MQ which cannot.

4. **Message priority replaces `mtype` tricks.** `mq_receive()` always returns the
   highest-priority message first. No need for the System V `mtype < 0` workaround.

5. **`mq_notify()` is one-shot.** Must re-register in the handler BEFORE reading
   messages to avoid a race. Safer alternative: dedicated blocking thread or epoll.

6. **Named semaphore init is atomic** (`sem_open(O_CREAT, mode, value)` creates and
   initializes in one call). This eliminates the System V create-then-init race condition.

7. **Unnamed semaphore with `pshared=1` MUST live in shared memory.** Placing `sem_t`
   in a private heap variable and passing a pointer to a child process is undefined
   behavior — the pointer addresses different physical memory in each process.

8. **POSIX SHM requires two steps: `shm_open` then `ftruncate`.** Newly created objects
   have zero size. Forgetting `ftruncate` before `mmap` results in `SIGBUS` on access.

9. **The unlink-then-close pattern prevents leaks.** Call `shm_unlink()`/`mq_unlink()`
   immediately after mapping/opening. The object lives until refcount reaches zero.
   This means no cleanup logic needed in signal handlers or `atexit()`.

10. **Always link with `-lrt`.** POSIX MQ and named semaphore functions are in `librt`.
    Forgetting `-lrt` gives linker errors or silent runtime failures depending on glibc
    version and symbol visibility.
