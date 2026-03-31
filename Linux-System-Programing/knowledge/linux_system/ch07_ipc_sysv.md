# System V IPC — Message Queues, Semaphores, Shared Memory

## Problem It Solves

Unix processes are isolated by design — they cannot directly read or write each other's
memory. System V IPC (introduced in Columbus UNIX, 1978, mainstreamed in System V, 1983)
was created to support **database transaction processing** in telephone company systems, where
multiple cooperating processes need:

- **Message Queues**: structured, typed data exchange without a persistent file
- **Semaphores**: synchronize access to shared resources across processes
- **Shared Memory**: share data at memory speed, bypassing the kernel copy overhead

All three mechanisms were designed together and share a common design pattern, which is why
they are grouped and learned together.

---

## Concept Overview

System V IPC provides three distinct mechanisms under one design umbrella:

| Mechanism | Purpose | Key syscalls |
|---|---|---|
| **Message Queue** | Typed message exchange, priority ordering | `msgget`, `msgsnd`, `msgrcv`, `msgctl` |
| **Semaphore Set** | Process synchronization, mutual exclusion | `semget`, `semop`, `semctl` |
| **Shared Memory** | Zero-copy data sharing via shared RAM pages | `shmget`, `shmat`, `shmdt`, `shmctl` |

**Common design pattern** shared by all three:

```
Step 1 — Key:    key_t key = ftok(pathname, proj_id)
Step 2 — Get:    int id = xxxget(key, flags)   → IPC identifier (NOT a fd)
Step 3 — Use:    msgsnd/msgrcv | semop | shmat/shmdt
Step 4 — Delete: xxxctl(id, IPC_RMID, NULL)    (MUST be explicit: kernel persistence)
```

**Critical distinction from file I/O**:
- The IPC identifier (`id`) is **not** a file descriptor
- It is a **system-wide** integer visible to all processes with access
- Cannot be used with `select()`, `poll()`, or `epoll()`

---

## System Context

```
User Space                          Kernel IPC Subsystem
                                    (ipc/msg.c, ipc/sem.c, ipc/shm.c)
                                    ┌─────────────────────────────────┐
Process A ──msgget(key)────────────►│  ipc_ids hash table (per type)  │
           ◄──msqid=5──────────────┤  key → struct kern_ipc_perm     │
                                    │       + msqid_ds / semid_ds     │
Process B ──msgget(key)────────────►│         / shmid_ds              │
           ◄──msqid=5──────────────┤  same key → same object         │
                                    │                                  │
Process C ──msgsnd(5, &msg)────────►│  linked list of messages        │
Process D ──msgrcv(5, &msg, type)──►│  type-filtered dequeue          │
                                    └─────────────────────────────────┘

IPC identifier encoding:
  id = (sequence_number << sequence_bits) | slot_index
  sequence_number: incremented each time slot is reused → detects stale IDs
```

---

## Internal Mechanism

### ftok() — Key Generation

```c
key_t ftok(const char *pathname, int proj_id);
```

Combines:
- Low 16 bits of the **inode number** of `pathname`
- Low 8 bits of the **minor device number** of the filesystem
- Low 8 bits of `proj_id`

→ produces a 32-bit `key_t` integer

Two processes calling `ftok()` with the same `(pathname, proj_id)` get the same key → find
the same IPC object.

**Critical pitfall**:
```
ftok("/tmp/app", 'A')  uses inode of /tmp/app

If /tmp/app is deleted and recreated:
  old inode = 12345 → key = 0x41_03039
  new inode = 99999 → key = 0x41_0270F  ← DIFFERENT KEY

→ Other processes using old key cannot find the new object.
   Use IPC_PRIVATE for related processes, or a hardcoded key in a shared header.
```

### IPC_CREAT, IPC_EXCL, IPC_PRIVATE

```c
msgget(key, IPC_CREAT | 0640)          // create if not exists, open if exists
msgget(key, IPC_CREAT | IPC_EXCL | 0640) // create only; EEXIST if already exists
msgget(IPC_PRIVATE, 0640)              // always creates new, guaranteed unique key
```

### Message Queue: Typed Linked List

Internally, a System V MQ is a **kernel-maintained linked list** of messages, each tagged
with a `mtype` (positive long integer).

```
msqid_ds:
  ├── msg_qbytes (max total bytes in queue, default 16384)
  ├── msg_qnum   (current message count)
  └── linked list:  [mtype=2, data] → [mtype=1, data] → [mtype=3, data] → NULL

msgrcv retrieval semantics:
  mtype > 0:   return first msg with exactly that type
  mtype == 0:  return first msg regardless of type (FIFO order)
  mtype < 0:   return first msg with smallest type ≤ |mtype|  (priority queue!)
```

Writer blocks when `msg_qbytes` would be exceeded. Reader blocks when queue is empty
(unless `IPC_NOWAIT` is set → `ENOMSG`).

**System limits** (tunable via `sysctl`):
```
/proc/sys/kernel/msgmax   = 8192   bytes  (max single message size)
/proc/sys/kernel/msgmnb   = 16384  bytes  (max total bytes per queue)
/proc/sys/kernel/msgmni   = 32000  queues (max queues system-wide)
```

### Semaphore Set: Atomic Multi-Semaphore Operations

Unlike POSIX semaphores (which are single integers), System V creates **sets** of semaphores.
`semop()` operates on one or more semaphores **atomically**:

```c
struct sembuf {
    unsigned short sem_num;  /* index within the set */
    short          sem_op;   /* +n: increment | -n: decrement/wait | 0: wait for zero */
    short          sem_flg;  /* IPC_NOWAIT, SEM_UNDO */
};

/* Lock two resources atomically — prevents partial-lock deadlock */
struct sembuf ops[2] = {
    { .sem_num=0, .sem_op=-1, .sem_flg=SEM_UNDO },
    { .sem_num=1, .sem_op=-1, .sem_flg=SEM_UNDO },
};
semop(semid, ops, 2);
/* Kernel: either BOTH succeed, or the whole operation blocks.
   No process holds sem[0] while waiting for sem[1]. → No deadlock. */
```

**SEM_UNDO mechanism**:
```
Per-process semadj table tracks all SEM_UNDO operations.
On process exit (normal or crash):
  kernel applies inverse of all accumulated adjustments
  → locked semaphores are automatically released

Caveats:
  1. semadj overflow: too many operations may not all be undone
  2. IPC_RMID before exit: undo target no longer exists → adjustment dropped
  3. exec() clears semadj table → SEM_UNDO does not survive exec()
```

**`union semun`** — must be defined manually (not in glibc headers per SUSv3):
```c
union semun {
    int              val;    /* SETVAL */
    struct semid_ds *buf;    /* IPC_STAT, IPC_SET */
    unsigned short  *array;  /* GETALL, SETALL */
};
```

### Shared Memory: Zero-Copy via Page Table Mapping

```c
int shmid = shmget(key, size, IPC_CREAT | 0640);
void *ptr = shmat(shmid, NULL, 0);  /* NULL: kernel chooses virtual address */
```

The kernel sets page table entries (PTEs) in the calling process to point to the
**same physical page frames** as other attached processes:

```
Process A page table:   vaddr 0x7f000000 → PTE → physical frame #42
Process B page table:   vaddr 0x7e000000 → PTE → physical frame #42
                                           ↕
                              (same physical RAM — zero copy)
```

`shmat()` returns a pointer valid within this process only. After `fork()`, child inherits
the mapping (same pointer is valid). After `exec()`, all attachments are automatically
detached.

**shmdt() vs shmctl(IPC_RMID)**:
```
shmdt(ptr):           detach from THIS process's virtual space
                      segment still exists in kernel (kernel persistence)

shmctl(id, IPC_RMID): mark segment for deletion
                      actually destroyed only after last shmdt() call
                      (only SHM has this reference-count-like behavior among Sys V IPC)
```

---

## Architecture

### Unified Get/Use/Control Pattern

```
┌─────────────────────────────────────────────────────────────────────┐
│              SYSTEM V IPC — COMMON ARCHITECTURE                      │
│                                                                       │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────────────────┐   │
│  │  ftok()  │    │IPC_PRIVATE│   │  Hardcoded key constant      │   │
│  └────┬─────┘    └────┬──────┘   └──────────────┬───────────────┘   │
│       └───────────────┴──────────────────────────┘                   │
│                         key_t key                                     │
│                              │                                        │
│              ┌───────────────┼───────────────┐                       │
│              ▼               ▼               ▼                        │
│         msgget()         semget()        shmget()                    │
│              │               │               │                        │
│         IPC id (int)    IPC id (int)    IPC id (int)                 │
│              │               │               │                        │
│     ┌────────┴──┐     ┌──────┴─────┐  ┌─────┴──────────┐           │
│     │ msgsnd()  │     │  semop()   │  │  shmat()       │           │
│     │ msgrcv()  │     │  semctl()  │  │  shmdt()       │           │
│     └────────┬──┘     └──────┬─────┘  └─────┬──────────┘           │
│              │               │               │                        │
│         msgctl()         semctl()        shmctl()                   │
│         IPC_RMID         IPC_RMID        IPC_RMID                   │
└─────────────────────────────────────────────────────────────────────┘
  All IPC objects: kernel-persistent, permissions mask (like files),
  visible via ipcs, removable via ipcrm
```

### Message Queue Internal Layout

```
msqid_ds
├── msg_perm   (permissions, uid, gid)
├── msg_qbytes = 16384  (total byte capacity)
├── msg_qnum   = 3      (current message count)
├── msg_first ──► [mtype=2 | "medium task"] ──►
│                [mtype=1 | "low task"]    ──►
│                [mtype=3 | "high task"]   ──► NULL
└── msg_last  ──────────────────────────────────►(last message)

msgrcv(id, buf, size, mtype=-3, 0):
  → scans list for smallest mtype ≤ 3
  → returns mtype=1 first, then mtype=2, then mtype=3
```

### Semaphore Set Internal Layout

```
semid_ds
├── sem_perm    (permissions)
├── sem_nsems = 3
└── sem_base ──► sem[0]: value=1  (mutex)
                 sem[1]: value=8  (empty slots)
                 sem[2]: value=0  (full slots)

semop() with multiple operations:
  kernel checks ALL operations can proceed before applying ANY
  if any would block: entire call blocks, NO partial state change
```

### Shared Memory Lifecycle

```
shmget()  →  kernel allocates physical page frames (zeroed)
             creates shmid_ds entry in kernel IPC table

shmat()   →  kernel maps pages into process virtual address space
             increments shm_nattch counter

shmdt()   →  kernel unmaps pages from process virtual address space
             decrements shm_nattch counter
             if IPC_RMID already called AND shm_nattch == 0: pages freed

shmctl(IPC_RMID) →  marks segment for deletion (SHM_DEST flag set)
                    if shm_nattch > 0: waits until all detach
                    if shm_nattch == 0: immediately frees pages
```

---

## Execution Flow

### Flow: System V Message Queue — Send and Priority Receive

```
Process A (producer)        Kernel MQ subsystem          Process B (consumer)
    │                               │                           │
    ├──msgget(key, IPC_CREAT)──────►│ create msqid_ds           │
    │◄──msqid=5─────────────────────┤                           │
    │                               │                           │
    ├──msgsnd(5, {type=3,"HIGH"})──►│ append to linked list     │
    ├──msgsnd(5, {type=1,"LOW"})───►│ append                    │
    ├──msgsnd(5, {type=2,"MED"})───►│ append                    │
    │                               │ list: [3]→[1]→[2]         │
    │                               │                           │
    │                               │◄──msgrcv(5,buf,sz,3,0)────┤ type==3
    │                               │ returns "HIGH"            │
    │                               │──────────────────────────►│
    │                               │                           │
    │                               │◄──msgrcv(5,buf,sz,-2,0)───┤ type ≤ 2
    │                               │ returns "LOW" (type=1)    │ (smallest first)
    │                               │──────────────────────────►│
    │                               │                           │
    ├──msgctl(5, IPC_RMID)─────────►│ destroy queue             │
```

### Flow: Semaphore — Binary Mutex Pattern

```
Process A                   Kernel semaphore             Process B
    │                               │                           │
    ├──semget(IPC_PRIVATE,1)───────►│ create set, sem[0]        │
    ├──semctl(SETVAL, 1)───────────►│ sem[0] = 1 (unlocked)     │
    │                               │                           │
    ├──semop({sem[0],-1,SEM_UNDO})─►│ sem[0]: 1→0 (locked)      │
    │  [enters critical section]    │                           │
    │                               │◄──semop({sem[0],-1})──────┤
    │                               │ sem[0]==0: BLOCK Process B │
    │                               │                           │
    ├──semop({sem[0],+1,SEM_UNDO})─►│ sem[0]: 0→1 (unlocked)    │
    │  [exits critical section]     │ wake up Process B         │
    │                               │──────────────────────────►│
    │                               │ sem[0]: 1→0 (locked by B) │
    │                               │                           │
```

### Flow: Shared Memory — Zero-Copy Data Transfer

```
Process A                   Kernel VM + IPC              Process B
    │                               │                           │
    ├──shmget(key,4096,IPC_CREAT)──►│ allocate page frame #N    │
    │◄──shmid=1─────────────────────┤ shmid_ds: nattch=0        │
    │                               │                           │
    ├──shmat(1, NULL, 0)───────────►│ map frame #N into A's PTE │
    │◄──ptr_a=0x7f000000────────────┤ nattch: 0→1               │
    │                               │                           │
    │                               │◄──shmat(1, NULL, 0)───────┤
    │                               │ map frame #N into B's PTE │
    │                               │──ptr_b=0x7e000000────────►│
    │                               │ nattch: 1→2               │
    │                               │                           │
    ├──*(ptr_a+0) = 42──────────────┤────── physical RAM ───────►B reads 42
    │  (write to physical frame #N) │    NO COPY, NO SYSCALL    │
    │                               │                           │
    ├──shmdt(ptr_a)─────────────────►│ nattch: 2→1               │
    ├──shmctl(1,IPC_RMID)──────────►│ mark SHM_DEST             │
    │                               │ (not freed yet: nattch=1)  │
    │                               │◄──shmdt(ptr_b)────────────┤
    │                               │ nattch: 1→0 + SHM_DEST    │
    │                               │ → physical page freed     │
```

---

## Example (Code)

### Example 1: System V Message Queue — Priority-Based Task Dispatch

```c
/* sysv_mq_demo.c
 * msgget, msgsnd, msgrcv with type filtering, IPC_NOWAIT
 * Build: gcc -o sysv_mq_demo sysv_mq_demo.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MSG_SIZE  128

typedef struct {
    long mtype;           /* 1=LOW, 2=MEDIUM, 3=HIGH priority */
    char text[MSG_SIZE];
} Message;

static void producer(int msqid)
{
    Message msg;
    struct { long type; const char *text; } tasks[] = {
        { 1, "low:   cleanup temp files" },
        { 3, "HIGH:  process payment transaction" },
        { 1, "low:   send weekly report" },
        { 2, "med:   refresh cache" },
        { 3, "HIGH:  handle user login" },
        { 2, "med:   update search index" },
    };

    for (int i = 0; i < 6; i++) {
        msg.mtype = tasks[i].type;
        strncpy(msg.text, tasks[i].text, MSG_SIZE - 1);
        msg.text[MSG_SIZE - 1] = '\0';
        if (msgsnd(msqid, &msg, sizeof(msg.text), 0) == -1) {
            perror("msgsnd"); exit(EXIT_FAILURE);
        }
        printf("[producer] queued (type=%ld): %s\n", msg.mtype, msg.text);
    }
    exit(EXIT_SUCCESS);
}

static void consumer_priority(int msqid)
{
    Message msg;
    printf("\n[consumer] draining by priority (HIGH first):\n");

    /* Drain HIGH (type=3) first, then MEDIUM (2), then LOW (1) */
    for (long t = 3; t >= 1; t--)
        while (msgrcv(msqid, &msg, sizeof(msg.text), t, IPC_NOWAIT) > 0)
            printf("[consumer] type=%ld: %s\n", t, msg.text);
}

int main(void)
{
    /* IPC_PRIVATE: guaranteed unique, suitable for parent-child */
    int msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0640);
    if (msqid == -1) { perror("msgget"); exit(EXIT_FAILURE); }

    printf("[main] MQ id=%d  (run 'ipcs -q' to see it)\n\n", msqid);

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid == 0) producer(msqid);

    waitpid(pid, NULL, 0);
    consumer_priority(msqid);

    /* Kernel persistence: MUST delete explicitly */
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
        perror("msgctl IPC_RMID");
    else
        printf("\n[main] MQ deleted\n");

    return 0;
}
```

### Example 2: System V Semaphore — Mutex with SEM_UNDO, Protecting Shared Counter

```c
/* sysv_sem_mutex.c
 * Binary semaphore mutex, SEM_UNDO, union semun, shared counter via SHM
 * Build: gcc -o sysv_sem_mutex sysv_sem_mutex.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

/* MUST define union semun manually — not provided by glibc per SUSv3 */
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

static int sem_create_mutex(void)
{
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0640);
    if (semid == -1) { perror("semget"); exit(EXIT_FAILURE); }
    union semun arg = { .val = 1 };  /* start unlocked */
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl SETVAL"); exit(EXIT_FAILURE);
    }
    return semid;
}

static void sem_lock(int semid)
{
    struct sembuf op = { .sem_num=0, .sem_op=-1, .sem_flg=SEM_UNDO };
    if (semop(semid, &op, 1) == -1) { perror("semop lock"); exit(EXIT_FAILURE); }
}

static void sem_unlock(int semid)
{
    struct sembuf op = { .sem_num=0, .sem_op=+1, .sem_flg=SEM_UNDO };
    if (semop(semid, &op, 1) == -1) { perror("semop unlock"); exit(EXIT_FAILURE); }
}

static long *shared_counter;

static void worker(int semid, int n_iters, const char *name)
{
    for (int i = 0; i < n_iters; i++) {
        sem_lock(semid);
        long val = *shared_counter;  /* read */
        usleep(1);                   /* widen race window */
        *shared_counter = val + 1;   /* write */
        sem_unlock(semid);
    }
    printf("[%s] done, final counter=%ld\n", name, *shared_counter);
    exit(EXIT_SUCCESS);
}

int main(void)
{
#define N_WORKERS  4
#define N_ITERS    250

    /* Shared memory for the counter */
    int shmid = shmget(IPC_PRIVATE, sizeof(long), IPC_CREAT | 0640);
    if (shmid == -1) { perror("shmget"); exit(EXIT_FAILURE); }
    shared_counter = shmat(shmid, NULL, 0);
    if (shared_counter == (void *)-1) { perror("shmat"); exit(EXIT_FAILURE); }
    *shared_counter = 0;

    int semid = sem_create_mutex();

    int cur = semctl(semid, 0, GETVAL);
    printf("[main] semid=%d shmid=%d initial_sem_value=%d\n\n",
           semid, shmid, cur);

    pid_t pids[N_WORKERS];
    const char *names[] = { "worker-A", "worker-B", "worker-C", "worker-D" };
    for (int i = 0; i < N_WORKERS; i++) {
        pids[i] = fork();
        if (pids[i] == -1) { perror("fork"); exit(EXIT_FAILURE); }
        if (pids[i] == 0) worker(semid, N_ITERS, names[i]);
    }

    for (int i = 0; i < N_WORKERS; i++) waitpid(pids[i], NULL, 0);

    printf("\n[main] final counter=%ld  expected=%d\n",
           *shared_counter, N_WORKERS * N_ITERS);

    /* Cleanup: detach SHM first, then delete both objects */
    shmdt(shared_counter);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    printf("[main] all IPC objects deleted\n");
    return 0;
}
```

### Example 3: Shared Memory + Semaphore Set — Producer/Consumer Circular Buffer

```c
/* sysv_shm_prodcon.c
 * Classic pattern: SHM for data, 3-semaphore set for sync (empty/full/mutex)
 * Build: gcc -o sysv_shm_prodcon sysv_shm_prodcon.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun { int val; struct semid_ds *buf; unsigned short *array; };

/* Semaphore indices */
#define SEM_EMPTY  0   /* count of empty slots (init = BUF_SIZE) */
#define SEM_FULL   1   /* count of filled slots (init = 0)       */
#define SEM_MUTEX  2   /* mutual exclusion on buffer (init = 1)  */

#define BUF_SIZE  8
#define N_ITEMS   20

typedef struct {
    int buffer[BUF_SIZE];
    int in, out;         /* write/read positions */
} SharedBuf;

static void P(int semid, int n)  /* wait (decrement) */
{
    struct sembuf sb = { .sem_num=n, .sem_op=-1, .sem_flg=SEM_UNDO };
    if (semop(semid, &sb, 1) == -1) { perror("P"); exit(EXIT_FAILURE); }
}

static void V(int semid, int n)  /* signal (increment) */
{
    struct sembuf sb = { .sem_num=n, .sem_op=+1, .sem_flg=SEM_UNDO };
    if (semop(semid, &sb, 1) == -1) { perror("V"); exit(EXIT_FAILURE); }
}

static void producer(SharedBuf *shm, int semid)
{
    for (int i = 1; i <= N_ITEMS; i++) {
        P(semid, SEM_EMPTY);             /* wait for empty slot */
        P(semid, SEM_MUTEX);             /* lock buffer */

        shm->buffer[shm->in] = i;
        shm->in = (shm->in + 1) % BUF_SIZE;
        printf("[producer] wrote  item %2d  in=%d\n", i, shm->in);

        V(semid, SEM_MUTEX);             /* unlock */
        V(semid, SEM_FULL);              /* signal new item available */
        usleep(50000);
    }
    exit(EXIT_SUCCESS);
}

static void consumer(SharedBuf *shm, int semid)
{
    for (int i = 1; i <= N_ITEMS; i++) {
        P(semid, SEM_FULL);              /* wait for item */
        P(semid, SEM_MUTEX);             /* lock buffer */

        int item = shm->buffer[shm->out];
        shm->out = (shm->out + 1) % BUF_SIZE;
        printf("           [consumer] read  item %2d  out=%d\n",
               item, shm->out);

        V(semid, SEM_MUTEX);             /* unlock */
        V(semid, SEM_EMPTY);             /* signal slot now free */
        usleep(80000);
    }
    exit(EXIT_SUCCESS);
}

int main(void)
{
    /* Create shared memory */
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedBuf), IPC_CREAT | 0640);
    if (shmid == -1) { perror("shmget"); exit(EXIT_FAILURE); }

    SharedBuf *shm = shmat(shmid, NULL, 0);
    if (shm == (void *)-1) { perror("shmat"); exit(EXIT_FAILURE); }
    shm->in = shm->out = 0;

    /* Create 3-semaphore set and initialize all at once via SETALL */
    int semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0640);
    if (semid == -1) { perror("semget"); exit(EXIT_FAILURE); }

    union semun arg;
    unsigned short init_vals[3] = { BUF_SIZE, 0, 1 };
    arg.array = init_vals;
    if (semctl(semid, 0, SETALL, arg) == -1) {
        perror("semctl SETALL"); exit(EXIT_FAILURE);
    }

    printf("[main] shmid=%d semid=%d buf_size=%d n_items=%d\n\n",
           shmid, semid, BUF_SIZE, N_ITEMS);

    pid_t ppid = fork(); if (ppid == 0) producer(shm, semid);
    pid_t cpid = fork(); if (cpid == 0) consumer(shm, semid);

    waitpid(ppid, NULL, 0);
    waitpid(cpid, NULL, 0);

    printf("\n[main] transfer complete. cleaning up.\n");
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}
```

---

## Debugging

### List all live System V IPC objects
```bash
ipcs             # all: message queues, semaphores, shared memory
ipcs -q          # message queues only
ipcs -s          # semaphore sets only
ipcs -m          # shared memory segments only
ipcs -l          # system-wide limits (msgmni, semmni, shmmni, ...)
ipcs -c          # show creator/owner PID — useful to trace leaked objects
ipcs -t          # show access times
```

### Remove a specific object
```bash
ipcrm -q <msqid>    # delete message queue by id
ipcrm -s <semid>    # delete semaphore set by id
ipcrm -m <shmid>    # delete shared memory by id
ipcrm -Q <key>      # delete MQ by key
ipcrm -S <key>      # delete semaphore by key
ipcrm -M <key>      # delete SHM by key
```

### Remove ALL user-owned IPC objects (emergency cleanup)
```bash
ipcs -q | awk 'NR>2 {print $2}' | xargs -I{} ipcrm -q {}
ipcs -s | awk 'NR>2 {print $2}' | xargs -I{} ipcrm -s {}
ipcs -m | awk 'NR>2 {print $2}' | xargs -I{} ipcrm -m {}
```

### Check shared memory attachment count
```bash
ipcs -m -i <shmid>   # detailed info including nattch (attached processes)
cat /proc/sysvipc/shm  # raw kernel view: key, shmid, size, nattch, ...
cat /proc/sysvipc/msg  # raw kernel view of all message queues
cat /proc/sysvipc/sem  # raw kernel view of all semaphore sets
```

### Check current semaphore values
```bash
ipcs -s -i <semid>   # shows value of each semaphore in the set
# In code:
# semctl(semid, sem_num, GETVAL)        → value of one semaphore
# semctl(semid, 0, GETALL, arg.array)   → all values at once
```

### Tuning system limits
```bash
cat /proc/sys/kernel/msgmax    # max message size
cat /proc/sys/kernel/msgmnb    # max bytes per queue
cat /proc/sys/kernel/msgmni    # max number of queues system-wide
cat /proc/sys/kernel/semmni    # max semaphore sets
cat /proc/sys/kernel/semmsl    # max semaphores per set
cat /proc/sys/kernel/shmmni    # max shared memory segments
cat /proc/sys/kernel/shmmax    # max size of single SHM segment
cat /proc/sys/kernel/shmall    # max total pages of shared memory

# Increase limits (not persistent across reboot):
sysctl -w kernel.msgmni=64000

# Persistent: add to /etc/sysctl.conf
```

### Detect leaked IPC objects accumulating over time
```bash
watch -n 5 'ipcs | wc -l'       # watch count grow
# If processes respawn without cleanup, count climbs until ENOSPC
```

---

## Real-world Usage

### PostgreSQL shared buffer pool (System V SHM)
PostgreSQL uses a single large System V shared memory segment for its **shared_buffers**
— the cache of disk pages shared across all backend processes. All backends `shmat()` the
same segment; semaphores and spinlocks protect individual buffer headers. On Linux,
PostgreSQL 9.3+ defaults to POSIX SHM (`shm_open`) but still supports System V for
compatibility.

### Apache prefork module (SHM for scoreboard)
Apache's prefork MPM uses a shared memory scoreboard where each worker process records its
state (idle, reading, writing, keepalive). The parent reads the scoreboard to make
scheduling decisions. Classic System V SHM use: many readers, occasional writers, mutex
via semaphore.

### System V semaphores as distributed mutex (same machine)
Any number of unrelated processes (e.g., multiple instances of a CLI tool, cron jobs) can
use a System V semaphore as a system-wide mutex — just agree on the same `ftok()` call in
a shared header. No daemon required.
```c
/* In shared header: */
#define LOCK_FILE  "/var/run/myapp"
#define LOCK_PROJ  'L'
/* Any process: */
key_t key = ftok(LOCK_FILE, LOCK_PROJ);
int semid = semget(key, 1, IPC_CREAT | 0640);
```

### Message queue for crash-resilient task dispatch
Because System V MQ has kernel persistence, messages survive the death of both producer
and consumer. A job submitter writes tasks to the queue; workers read them. If a worker
crashes, messages stay in the queue for the next worker. This is the original use case
from Columbus UNIX telephone billing systems.

---

## Key Takeaways

1. **System V IPC = three mechanisms, one design pattern**. Learn `get/use/ctl` once —
   applies to MQ, Semaphore, and SHM identically. The only difference is the "use" step.

2. **IPC identifier ≠ file descriptor**. It cannot be used with `read()`/`write()`,
   `select()`/`poll()`/`epoll()`. It is a system-wide integer, not per-process.

3. **Kernel persistence is both a feature and a trap**. Objects survive process exit and
   crash — great for crash recovery, dangerous for resource leaks. Always clean up with
   `xxxctl(IPC_RMID)` in signal handlers (`SIGTERM`, `SIGINT`) and `atexit()`.

4. **ftok() is not collision-free**. If the referenced file is deleted/recreated (new
   inode), the key changes silently. Prefer `IPC_PRIVATE` for parent-child communication,
   or hardcode keys in a shared header for unrelated processes.

5. **Message type is System V MQ's killer feature**. `msgrcv()` with `mtype < 0` gives
   a priority queue. Multiple consumers can selectively receive different message classes
   from the same queue — impossible with pipes or FIFOs.

6. **System V Semaphores operate on sets, not singles**. `semop()` on multiple semaphores
   is atomic — either all operations complete or none does. This eliminates the partial-lock
   deadlock class of bugs. The cost: a more complex API requiring `struct sembuf[]` arrays.

7. **`union semun` must be defined manually**. Per SUSv3, `glibc` does not define it. A
   missing definition causes silent UB or compile errors. Always include the union definition
   in a project-wide header.

8. **`SEM_UNDO` is not a complete safety net**. It helps on clean exit but fails after
   `exec()`, when the semaphore set is deleted before the process exits, or on `semadj`
   overflow. `SEM_UNDO` is an extra layer, not a replacement for explicit cleanup.

9. **SHM is fast only for large or complex data**. For small structs or high-contention
   scenarios, cache-line coherence protocol (MESI) overhead can exceed the 2× memcpy cost
   of pipes. Profile before assuming SHM is faster.

10. **Cleanup order matters for SHM**: call `shmdt()` before `shmctl(IPC_RMID)`. The segment
    is physically freed only after the last `shmdt()`. Calling `IPC_RMID` while processes are
    still attached marks it for deletion but does not crash them.
