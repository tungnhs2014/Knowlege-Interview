# Socket Server Design — Iterative, Concurrent, Prefork, Prethread

## Problem It Solves

A basic TCP server (socket/bind/listen/accept loop) handles one client at a time. In the real
world, multiple clients arrive simultaneously and each may take an unpredictable amount of
time to service. The question is: **how should a server be structured so that one slow client
does not block all others?**

There is no single answer — each design pattern trades off complexity, memory, throughput,
latency, and fault isolation differently. Understanding all models is essential for both
system programming interviews and production server design decisions.

---

## Concept Overview

### The Six Server Models

| Model | Concurrency unit | Connection cost | Memory / conn | Best for |
|-------|-----------------|-----------------|---------------|---------|
| **Iterative** | None | Zero | Minimal | Low traffic, UDP stateless |
| **Fork per client** | Process | High (`fork()`) | ~MB | Simple concurrent, prototype |
| **Thread per client** | Thread | Medium | ~8 MB (stack) | Moderate traffic |
| **Preforked pool** | Process pool | Low (at startup) | MB × pool size | Apache-style, fault isolation |
| **Prethreaded pool** | Thread pool | Low (at startup) | MB × pool size | Database backends |
| **I/O multiplexing** | Single event loop | Very low | ~KB | nginx, Redis, Node.js |

> I/O multiplexing (select/poll/epoll) is covered in Chapter 9 (topic 9.1). It is listed here
> for comparison but not detailed in this file.

---

## System Context

### Where Server Design Decisions Interact with the Kernel

```
accept queue (kernel)
  [conn1][conn2][conn3]...
        │
        ▼
  accept() — who calls it and how many times determines the model

Iterative:    one accept() at a time, sequential
Fork:         accept() in parent, fork() child for each conn
Thread:       accept() in main thread, spawn thread per conn
Preforked:    N children all block on same accept()
Prethreaded:  main thread accept(), push to worker queue
Epoll:        one thread monitors N fds simultaneously
```

### Process vs Thread — Isolation Trade-off

```
Fork (process per client):           Thread per client:
─────────────────────────────────    ──────────────────────────────────────
separate virtual address space       shared address space
child crash → only child dies        thread crash → whole process dies
CoW memory pages (safe shared data)  must use mutex for shared data
OS scheduler manages each process    OS scheduler manages each thread
~MB overhead for page tables         ~8 MB overhead for default stack
SIGCHLD + waitpid() to reap          pthread_join() or detach to cleanup
```

---

## Architecture

### Model 1 — Iterative Server

```
while (1) {
    connfd = accept(listenfd)      ← block until connection arrives
    handle_client(connfd)          ← BLOCK: no other accept() until done
    close(connfd)
}
```

Suitable only when `handle_client()` completes in microseconds. Any slow client
(network delay, slow read, long computation) freezes all waiting clients.

### Model 2 — Fork per Client

```
while (1) {
    connfd = accept(listenfd)
    pid = fork()
    ┌─ child (pid==0):
    │    close(listenfd)           ← child doesn't need listening socket
    │    handle_client(connfd)
    │    _exit(0)
    └─ parent (pid>0):
         close(connfd)             ← CRITICAL: must close to allow EOF delivery
         (loop back to accept)
}
```

**Why parent must `close(connfd)` immediately:**
`fork()` duplicates all file descriptors. After fork, both parent and child hold a
reference to `connfd` (reference count = 2). The socket closes only when count reaches 0.
If parent keeps `connfd` open, the child's `close()` only drops count to 1 — the client
**never receives EOF**. This is the most common bug in concurrent fork servers.

**Zombie prevention — mandatory SIGCHLD handler:**
When a child terminates it becomes a zombie until the parent calls `wait()`. Without
cleanup, zombies accumulate until the process table is full and `fork()` fails with `EAGAIN`.

```c
static void reap_children(int sig) {
    int saved_errno = errno;
    /* Loop: multiple children may terminate near-simultaneously but
     * SIGCHLD is only queued once. WNOHANG = non-blocking. */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;   /* restore: handler runs inside interrupted syscalls */
}
/* Install with SA_RESTART so accept() is not permanently interrupted */
```

### Model 3 — Thread per Client

```
while (1) {
    connfd = accept(listenfd)
    int *arg = malloc(sizeof(int));
    *arg = connfd;
    pthread_create(&tid, NULL, client_thread, arg)
    pthread_detach(tid)            ← auto-cleanup; no join needed
}

void *client_thread(void *arg) {
    int connfd = *(int *)arg;
    free(arg);
    handle_client(connfd);
    close(connfd);
    return NULL;
}
```

**Stack size concern:** default stack = 8 MB per thread. 1000 concurrent
threads → 8 GB stack alone. Reduce with:
```c
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setstacksize(&attr, 512 * 1024);   /* 512 KB stack */
pthread_create(&tid, &attr, client_thread, arg);
pthread_attr_destroy(&attr);
```

### Model 4 — Preforked Server Pool

Master creates N worker processes **before** any connections arrive. All workers
block on the same listening socket's `accept()`.

```
Master:
  socket() → bind() → listen()
  fork() × N  ← workers inherit listenfd

Each worker:
  while (1) {
      connfd = accept(listenfd)   ← multiple workers block here simultaneously
      handle_client(connfd)
      close(connfd)
  }
```

**Thundering herd:** when a connection arrives, the kernel wakes all N blocked workers.
Only one succeeds at `accept()`; the others return to sleep. On Linux ≥ 2.6, `accept()`
on a shared socket is serialized by the kernel — only **one** process is woken per
connection. The classic thundering herd problem no longer applies to `accept()` on Linux.

**`SO_REUSEPORT` (Linux 3.9+) — better approach:**
Each worker binds its **own** listening socket to the same port. The kernel distributes
incoming connections across all sockets (load-balance at kernel level, no shared lock):
```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
bind(sockfd, ...);   /* each worker calls this independently */
```
This is how **nginx** operates when `reuseport` is enabled.

### Model 5 — Prethreaded Pool with Accept Queue

Main thread calls `accept()` and dispatches to workers via a shared queue:

```
Main thread → accept() → push(connfd) → queue
                                            │
                         ┌──────────────────┘
Worker threads (pool): pop(connfd) → handle_client(connfd) → close()
```

Advantages over "thread per client": thread creation cost is paid once at startup;
pool size is bounded → predictable memory usage.

### inetd — Internet Superserver (Legacy)

`inetd` solved the problem of hundreds of low-traffic services each running their own
idle daemon (wasting RAM and process table slots):

```
/etc/inetd.conf:
  ftp   stream tcp nowait root /usr/sbin/in.ftpd in.ftpd
  echo  stream tcp nowait root internal

inetd operation:
  For each configured service: socket() → bind() → listen()
  select() on all sockets
  On activity:
    accept() (TCP) or recvfrom() (UDP)
    fork()
    child: dup(connfd → stdin/stdout/stderr), exec(server)
    parent: close(connfd), return to select()
```

Server programs launched by `inetd` simply read/write `stdin`/`stdout` — zero socket
init code needed. **Modern replacement**: systemd socket activation, which passes
pre-bound fds to the service via `SD_LISTEN_FDS_START`.

---

## Execution Flow

### Fork-per-client Full Lifecycle

```
PARENT (server)                         CHILD (per-client handler)
─────────────────────────────────────────────────────────────────────
socket(AF_INET, SOCK_STREAM, 0)
bind(listenfd, &addr, addrlen)
listen(listenfd, backlog)
install SIGCHLD handler (SA_RESTART)

accept(listenfd, &clientaddr, ...)      ← blocks
  │  client connects
  │  returns connfd

fork()
  │  ─────────────────────────────────► (child process starts)
  │                                     close(listenfd)  ← don't need
  │                                     handle_client(connfd)
  │                                       read/write loop
  │                                     close(connfd)
  │                                     _exit(0)
  │  ◄──── SIGCHLD ────────────────────
  │
close(connfd)   ← MUST close — drops refcount to 0
(loop: accept next connection)

SIGCHLD handler fires:
  waitpid(-1, NULL, WNOHANG) loop → reap zombie child
```

### Prethreaded Pool Full Lifecycle

```
main()
  create pool of N worker threads (all block on queue_pop)
  socket() → bind() → listen()

  loop:
    connfd = accept(listenfd)
    queue_push(connfd)          ← hand off immediately, no blocking

worker thread:
  loop:
    connfd = queue_pop()        ← blocks on condition variable if queue empty
    handle_client(connfd)       ← echo/process data
    close(connfd)
```

---

## Example (Code)

### Example 1 — Iterative TCP Echo Server

```c
/* iterative_server.c — Process one client at a time (simple, no concurrency)
 * Compile: gcc iterative_server.c -o iterative_server
 * Run:     ./iterative_server 8080
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

static void handle_client(int connfd) {
    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(connfd, buf, sizeof(buf))) > 0)
        write(connfd, buf, n);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(atoi(argv[1])),
    };
    bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listenfd, 10);
    printf("[iterative] Listening on port %s\n", argv[1]);

    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        if (connfd == -1) { perror("accept"); continue; }

        printf("Client %s:%d connected\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        handle_client(connfd);   /* blocks here — no other clients served */
        close(connfd);
    }

    close(listenfd);
    return 0;
}
```

### Example 2 — Concurrent Fork-per-client Server

```c
/* concurrent_fork_server.c — One child process per client
 * Compile: gcc concurrent_fork_server.c -o fork_server
 * Run:     ./fork_server 8080
 * Test:    for i in $(seq 5); do nc 127.0.0.1 8080 & done
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

/* Reap all terminated children without blocking.
 * Loop because multiple children may exit near-simultaneously but
 * the kernel only delivers one SIGCHLD. */
static void reap_children(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

static void handle_client(int connfd) {
    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(connfd, buf, sizeof(buf))) > 0)
        write(connfd, buf, n);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    /* SA_RESTART: automatically restart accept() if interrupted by SIGCHLD */
    struct sigaction sa = { .sa_handler = reap_children, .sa_flags = SA_RESTART };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(atoi(argv[1])),
    };
    bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listenfd, 50);
    printf("[fork] Listening on port %s\n", argv[1]);

    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        if (connfd == -1) {
            if (errno == EINTR) continue;   /* SIGCHLD interrupted accept — retry */
            perror("accept"); break;
        }

        printf("Client %s:%d → fork\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        switch (fork()) {
        case -1:
            perror("fork");
            close(connfd);   /* can't spawn child; drop this client */
            break;

        case 0:              /* CHILD */
            close(listenfd); /* child never calls accept() */
            handle_client(connfd);
            close(connfd);
            _exit(0);        /* _exit: no atexit(), no stdio flush */

        default:             /* PARENT */
            /* MUST close connfd: after fork both parent+child hold a reference.
             * Parent's copy keeps refcount at 1 even after child closes.
             * Client will never see EOF unless parent drops its reference. */
            close(connfd);
            break;
        }
    }

    close(listenfd);
    return 0;
}
```

### Example 3 — Prethreaded Server Pool with Accept Queue

```c
/* prethreaded_server.c — Fixed thread pool + bounded accept queue
 * Compile: gcc prethreaded_server.c -o prethreaded_server -lpthread
 * Run:     ./prethreaded_server 8080 4      (port, thread pool size)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE  1024
#define QUEUE_MAX 128

/* Thread-safe bounded queue for connected fds */
static int    q_buf[QUEUE_MAX];
static int    q_head = 0, q_tail = 0, q_count = 0;
static pthread_mutex_t q_mtx       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  q_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  q_not_full  = PTHREAD_COND_INITIALIZER;

static void q_push(int fd) {
    pthread_mutex_lock(&q_mtx);
    while (q_count == QUEUE_MAX)
        pthread_cond_wait(&q_not_full, &q_mtx);   /* block if queue full */
    q_buf[q_tail] = fd;
    q_tail = (q_tail + 1) % QUEUE_MAX;
    q_count++;
    pthread_cond_signal(&q_not_empty);
    pthread_mutex_unlock(&q_mtx);
}

static int q_pop(void) {
    pthread_mutex_lock(&q_mtx);
    while (q_count == 0)
        pthread_cond_wait(&q_not_empty, &q_mtx);  /* block if queue empty */
    int fd = q_buf[q_head];
    q_head = (q_head + 1) % QUEUE_MAX;
    q_count--;
    pthread_cond_signal(&q_not_full);
    pthread_mutex_unlock(&q_mtx);
    return fd;
}

static void handle_client(int connfd) {
    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(connfd, buf, sizeof(buf))) > 0)
        write(connfd, buf, n);
}

static void *worker_thread(void *arg) {
    (void)arg;
    for (;;) {
        int connfd = q_pop();       /* blocks until work available */
        handle_client(connfd);
        close(connfd);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <threads>\n", argv[0]);
        exit(1);
    }
    int port      = atoi(argv[1]);
    int pool_size = atoi(argv[2]);

    /* Create pool BEFORE accepting connections */
    for (int i = 0; i < pool_size; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, worker_thread, NULL);
        pthread_detach(tid);   /* auto-cleanup on thread exit */
    }
    printf("[prethreaded] %d workers started\n", pool_size);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(port),
    };
    bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listenfd, 50);
    printf("[prethreaded] Listening on port %d\n", port);

    /* Main thread: accept() and dispatch — never handles clients directly */
    while (1) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd == -1) { perror("accept"); continue; }
        q_push(connfd);   /* hand off to pool immediately */
    }

    close(listenfd);
    return 0;
}
```

---

## Debugging

### Common Pitfalls

| Pitfall | Cause | Symptom | Fix |
|---------|-------|---------|-----|
| Client never gets EOF | Parent forgot `close(connfd)` after `fork()` | `read()` on client side hangs forever | Always `close(connfd)` in parent immediately after `fork()` |
| Zombie accumulation | No `SIGCHLD` handler | `ps aux` shows many `<defunct>` processes; `fork()` eventually fails with `EAGAIN` | Install handler: `waitpid(-1, NULL, WNOHANG)` loop |
| `accept()` fails with `EINTR` | `SIGCHLD` delivered during `accept()` | Server loop exits unexpectedly | Check `errno == EINTR` and `continue`; or use `SA_RESTART` |
| Thread memory exhaustion | Too many threads × 8 MB stack | OOM kill | Use thread pool; reduce stack size with `pthread_attr_setstacksize()` |
| Thundering herd (old kernels) | N workers wake on one connection | CPU spike, most workers immediately sleep again | Linux 2.6+ serializes `accept()`; use `SO_REUSEPORT` for best result |
| FD leak in worker thread | Thread forgets `close(connfd)` | `EMFILE` on `accept()` after FD limit | Always `close(connfd)` in worker before returning |

### Inspection Commands

```bash
# Count processes/threads of a server
ps aux | grep server_name
ls /proc/<pid>/task/ | wc -l        # thread count

# Monitor accept queue depth (listen backlog overflow)
ss -tlnp | grep <port>
netstat -s | grep "listen"          # SYN drops if accept queue full

# Check open fd count per process
ls /proc/<pid>/fd | wc -l
cat /proc/sys/fs/file-max           # system-wide fd limit

# Check zombie processes
ps aux | grep Z

# Watch active connections
ss -tanp | grep <port>
watch -n1 'ss -tan | grep ESTABLISHED | wc -l'

# Verify SO_REUSEPORT (multiple sockets on same port)
ss -tlnp | grep <port>
# Should show multiple entries with different pids

# strace to trace accept/fork calls
strace -p <pid> -e trace=accept,fork,clone,waitpid
```

---

## Real-world Usage

| System | Model | Rationale |
|--------|-------|-----------|
| **Apache (MPM Prefork)** | Preforked process pool | Max fault isolation; each request in separate process |
| **Apache (MPM Worker)** | Prethreaded pool | Better performance than prefork; moderate thread count |
| **nginx** | 1 process per CPU + epoll | Event-driven; handles 10k+ connections per worker |
| **PostgreSQL** | Fork per client (postmaster) | Strong fault isolation; crash in one backend doesn't affect others |
| **Redis** | Single-threaded event loop | All data in RAM → no blocking I/O; one thread is enough |
| **Node.js** | Single event loop + libuv thread pool | JS is single-threaded; blocking ops offloaded to thread pool |
| **sshd** | Fork per connection | Privilege separation; each session isolated |
| **systemd socket activation** | inetd successor | Pre-bind sockets, pass fd to service on demand |

**Choosing between models in practice:**
- If clients do **blocking I/O** (DB queries, file reads): use thread/process pool to avoid one client blocking others
- If most connections are **idle most of the time** (WebSocket, long-poll): use I/O multiplexing (epoll) — topic 9.1
- If **crash isolation** is critical (PostgreSQL, sshd): use process-per-client or preforked
- If **extreme concurrency** (>10k connections) with low per-connection work: I/O multiplexing is the only viable option

---

## Key Takeaways

| Concept | Core point |
|---------|-----------|
| Iterative | One client at a time — simplest, only for fast/rare requests |
| Fork per client | `fork()` after `accept()`; parent **must** `close(connfd)` immediately |
| `SIGCHLD` handler | Required for fork model — `waitpid(-1, NULL, WNOHANG)` loop |
| `SA_RESTART` | Prevents `accept()` from returning `EINTR` when SIGCHLD fires |
| `_exit()` in child | Skips `atexit()` and stdio flush — child should not run parent's cleanup |
| Thread per client | Cheaper than fork; shared state needs mutex; stack size is the limit |
| `pthread_detach()` | Avoids memory leak for threads that are not `join()`ed |
| Preforked pool | Workers created at startup — no per-connection fork cost |
| `SO_REUSEPORT` | Each worker binds own socket on same port — kernel-level load balancing |
| Prethreaded + queue | Main thread accepts, queue decouples acceptance from handling |
| inetd / systemd | Superserver pattern — start service on demand, pass pre-bound fd |
| Thundering herd | Linux ≥ 2.6 serializes `accept()` — one wake per connection |
