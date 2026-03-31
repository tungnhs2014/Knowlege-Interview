# Pipes & FIFOs — Anonymous Pipes and Named Pipes

## Problem It Solves

Processes run in isolated virtual address spaces — they cannot directly share memory or pass
data to each other. The earliest Unix IPC solution was the **pipe**: a kernel-managed byte
buffer that lets one process write data that another process reads, with automatic flow
control and synchronization built in.

Pipes solve two concrete problems:
- **Parent ↔ child communication**: after `fork()`, both processes inherit the same pipe fds
- **Shell pipelines**: `ls | grep ".c" | wc -l` — output of one program feeds input of next

FIFOs (named pipes) extend this to **unrelated processes**: any process that knows the
filesystem path can open and use the FIFO, without requiring a common ancestor.

---

## Concept Overview

**Anonymous Pipe** (`pipe()`):
- A kernel ring buffer with two file descriptors: read end `pfd[0]` and write end `pfd[1]`
- No filesystem name — exists only while a process holds at least one of its fds
- Unidirectional: data flows `pfd[1] → buffer → pfd[0]`
- Only accessible by related processes (parent, child, siblings via common ancestor)

**FIFO / Named Pipe** (`mkfifo()`):
- A special file in the filesystem (type `p`, shown as `prw-...` by `ls -la`)
- Any process with appropriate permissions can open and use it
- Same byte-stream semantics as anonymous pipe once both ends are open
- Name persists on filesystem; data does not (process persistence)

Key properties shared by both:
- **Byte stream** — no message boundaries, `read()` may coalesce multiple `write()` calls
- **Unidirectional** — one end writes, other reads (bidirectional needs two pipes)
- **Blocking I/O by default** — reader blocks on empty, writer blocks on full
- **Automatic flow control** — kernel handles backpressure transparently

---

## System Context

```
User Space                      Kernel Space
────────────────                ─────────────────────────────────
write(pfd[1], data, n) ──────► copy_from_user()
                                pipe ring buffer [65536 bytes]
read(pfd[0], buf, n)   ◄────── copy_to_user()

Reference counting:
  pipe destroyed only when ALL fds (across ALL processes) are closed
  kernel tracks: struct pipe_inode_info { readers, writers, ... }
```

The pipe buffer is an **in-kernel ring buffer** — not a file on disk, not a memory-mapped
region. Since Linux 2.6.11 the default capacity is **65536 bytes** (previously = page size =
4096). Since Linux 2.6.35 it can be resized with `fcntl(fd, F_SETPIPE_SZ, size)`.

---

## Internal Mechanism

### Kernel Ring Buffer

```
pipe buffer (65536 bytes):
  ┌─────────────────────────────────────────┐
  │ consumed │   data   │     free space    │
  └──────────┴──────────┴───────────────────┘
             ▲ read_pos  ▲ write_pos

write(n): copy n bytes → free space, advance write_pos
          if free < n: writer sleeps (wait queue)
read(n):  copy min(n, available) → user buf, advance read_pos
          if available == 0: reader sleeps (wait queue)
          if available > 0 after write: wake sleeping writers
```

### PIPE_BUF — Atomicity Guarantee

`PIPE_BUF = 4096` bytes on Linux (from `<limits.h>`).

```
write(pfd[1], data, N):
  N ≤ PIPE_BUF (4096):  kernel writes whole chunk atomically
                         → multiple writers cannot interleave
  N > PIPE_BUF:          kernel chunks the write; may be interleaved
                         → data from different writers can mix in buffer
```

This only matters when **multiple processes write to the same pipe/FIFO concurrently**.
For single-writer scenarios, any write size is effectively atomic from the reader's view
(data is ordered, just may span multiple `read()` calls).

### EOF and SIGPIPE Semantics

**Detecting end-of-input (reader side)**:
```
All write-end fds closed → read() returns 0 (EOF)
Still ≥1 write-end fd open → read() blocks (even if writer is sleeping)
```

**Write to broken pipe (writer side)**:
```
All read-end fds closed → kernel sends SIGPIPE to writer process
                           (default action: terminate)
                        → if SIGPIPE ignored/caught: write() returns -1, errno=EPIPE
```

### Why Unused Ends MUST Be Closed After fork()

```
After fork(), both parent and child have copies of all pipe fds.

Bug: reader forgets to close(pfd[1])
──────────────────────────────────────────────────
  writer child    closes pfd[1]   → write refcount: 1 (reader still holds it)
  reader parent   read() blocks forever
  reason: kernel sees write-end refcount > 0 → never sends EOF

Bug: writer forgets to close(pfd[0])
──────────────────────────────────────────────────
  reader child    closes pfd[0]   → read refcount: 1 (writer still holds it)
  writer parent   fills pipe → write() blocks forever
  reason: kernel won't deliver SIGPIPE (read-end refcount > 0)
```

**Rule**: Immediately after `fork()`, each process closes the pipe ends it does not use.

### dup2() — Redirecting pipe to stdin/stdout

Used to implement shell pipelines without modifying the programs being connected:

```c
/* Bind write end of pipe to stdout */
dup2(pfd[1], STDOUT_FILENO);  /* closes fd 1, clones pfd[1] as fd 1 */
close(pfd[1]);                /* fd 1 and old pfd[1] pointed to same thing;
                                  now only fd 1 remains */
/* After exec(), program writes to stdout unaware of the pipe */
```

Safety pattern when `pfd[1]` might already equal `STDOUT_FILENO`:
```c
if (pfd[1] != STDOUT_FILENO) {
    dup2(pfd[1], STDOUT_FILENO);
    close(pfd[1]);
}
```

### FIFO open() Blocking Behaviour

```
Process A: open(path, O_RDONLY)  → blocks in kernel
Process B: open(path, O_WRONLY)  → both unblock atomically

Kernel guarantees no data race at connection time.
```

With `O_NONBLOCK`:
- `O_RDONLY | O_NONBLOCK`: returns immediately; `ENXIO` if no writer yet
- `O_WRONLY | O_NONBLOCK`: returns immediately; `ENXIO` if no reader yet
- `O_RDWR` (non-POSIX but works on Linux): never blocks, never returns ENXIO;
  used in servers to hold one write-end open to prevent constant EOF

---

## Architecture

### Anonymous Pipe Lifecycle

```
pipe(pfd)
    │
    ▼
kernel creates pipe_inode:
  pfd[0] → read file description  (refcount=1)
  pfd[1] → write file description (refcount=1)
    │
fork()
    │
    ├── parent: close(pfd[0])   read  refcount→1
    │           use  pfd[1]     write refcount=2→1 after child closes theirs
    │
    └── child:  close(pfd[1])   write refcount→1
                use  pfd[0]     read  refcount=2→1 after parent closes theirs
    │
    ▼
communication via read/write on remaining fds
    │
    ▼
last fd closed → ring buffer freed, pipe_inode destroyed
```

### FIFO Lifecycle

```
mkfifo("/tmp/myfifo", 0640)
    │
    ▼
filesystem: special file (type=p), inode created
    │
    ├── Process A: open(path, O_WRONLY) ─── blocks
    ├── Process B: open(path, O_RDONLY) ─── blocks
    │                                    ↕ both unblock
    │             kernel connects the two ends
    │
    ▼
read/write identical to anonymous pipe
    │
    ▼
both sides close fds → data gone (process persistence)
    │
FIFO name on filesystem REMAINS until: unlink(path)
```

### Bidirectional Communication (Two Pipes)

```
parent ──pipe_a[1]──────────────────────► child
                   [pipe_a ring buffer]
parent ◄──────────────────────pipe_b[0]── child
                   [pipe_b ring buffer]

Deadlock risk: both processes blocking on read() at same time.
Protocol: always send then receive (never receive then receive).
```

---

## Execution Flow

### flow: pipe() + fork() + dup2() + exec() (shell pipeline)

```
main process                    kernel
    │
    ├──pipe(pipe1)─────────────►│ allocate ring buffer
    │◄──pipe1[0]=3, pipe1[1]=4──┤
    │
    ├──fork() ─────────────────►│
    │                           │─── child1 (ls) ──────────────────────┐
    │                           │                                       │
    │  parent                   │  child1                               │
    │  ──────                   │  ──────                               │
    │                           │  close(pipe1[0])                      │
    │                           │  dup2(pipe1[1], STDOUT) fd1→pipe1[1] │
    │                           │  close(pipe1[1])                      │
    │                           │  execlp("ls", ...)                   │
    │                           │  ls writes to fd1 → pipe1 buffer     │
    │                           │                                       │
    ├──fork() ─────────────────►│─── child2 (wc) ──────────────────────┤
    │                           │  close(pipe1[1])                      │
    │                           │  dup2(pipe1[0], STDIN)  fd0→pipe1[0] │
    │                           │  close(pipe1[0])                      │
    │                           │  execlp("wc", "-l", ...)             │
    │                           │  wc reads from fd0 ← pipe1 buffer    │
    │                           │                                       │
    ├──close(pipe1[0])          │  parent MUST close both ends         │
    ├──close(pipe1[1])──────────►│  so wc sees EOF when ls exits        │
    │                           │                                       │
    ├──waitpid(child1)          │                                       │
    └──waitpid(child2)──────────►│ reap both children                   │
```

### Flow: FIFO open() rendezvous

```
Server process                  Kernel                  Client process
    │                               │                       │
    ├──mkfifo(path, 0666)──────────►│ create special inode  │
    │                               │                       │
    ├──open(path, O_RDWR)──────────►│ open succeeds         │
    │◄──fd=3────────────────────────┤ (O_RDWR never blocks) │
    │                               │                       │
    │                               │◄──mkfifo same path────┤ (EEXIST ok)
    │                               │◄──open(path,O_WRONLY)─┤
    │                               │ connects to server fd │
    │                               │──────────────────────►│ fd=4
    │                               │                       │
    ├──read(3, &req, sizeof(req))──►│ blocks until client   │
    │                               │◄──write(4, &req)──────┤
    │◄──returns sizeof(req)─────────┤ data transferred      │
```

---

## Example (Code)

### Example 1: PIPE_BUF Atomicity — Multiple Writers

```c
/* pipe_atomic.c
 * Demonstrates: write ≤ PIPE_BUF is atomic, write > PIPE_BUF may interleave
 * Build: gcc -o pipe_atomic pipe_atomic.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#define NUM_WRITERS  3
#define MSG_SMALL    64      /* ≤ PIPE_BUF: atomic */
#define MSG_LARGE    8192    /* > PIPE_BUF: NOT atomic */

static void fill_buf(char *buf, int size, char id)
{
    memset(buf, id, size - 1);
    buf[size - 1] = '\n';
}

static void writer_proc(int wfd, char id, int msg_size)
{
    char *buf = malloc(msg_size);
    fill_buf(buf, msg_size, id);
    for (int i = 0; i < 4; i++)
        write(wfd, buf, msg_size);
    free(buf);
    close(wfd);
    _exit(EXIT_SUCCESS);
}

static void check_interleave(int rfd, int msg_size, const char *label)
{
    char *buf = malloc(msg_size);
    int interleaved = 0;
    ssize_t n;

    printf("\n--- %s (msg_size=%d, PIPE_BUF=%d) ---\n",
           label, msg_size, PIPE_BUF);

    while ((n = read(rfd, buf, msg_size)) > 0) {
        char first = buf[0];
        for (ssize_t i = 1; i < n - 1; i++) {
            if (buf[i] != first) {
                printf("  INTERLEAVED: starts '%c', byte[%zd]='%c'\n",
                       first, i, buf[i]);
                interleaved = 1;
                break;
            }
        }
        if (!interleaved)
            printf("  ATOMIC: %zd bytes = '%c'\n", n, first);
        interleaved = 0;
    }
    free(buf);
}

int main(void)
{
    int pfd[2];

    printf("PIPE_BUF on this system = %d bytes\n", PIPE_BUF);

    /* Test 1: small writes (≤ PIPE_BUF) — expect ATOMIC */
    pipe(pfd);
    for (int i = 0; i < NUM_WRITERS; i++)
        if (fork() == 0)
            writer_proc(pfd[1], 'A' + i, MSG_SMALL);
    close(pfd[1]);
    check_interleave(pfd[0], MSG_SMALL, "SMALL writes (atomic expected)");
    close(pfd[0]);
    for (int i = 0; i < NUM_WRITERS; i++) wait(NULL);

    /* Test 2: large writes (> PIPE_BUF) — expect INTERLEAVE */
    pipe(pfd);
    for (int i = 0; i < NUM_WRITERS; i++)
        if (fork() == 0)
            writer_proc(pfd[1], 'A' + i, MSG_LARGE);
    close(pfd[1]);
    check_interleave(pfd[0], MSG_LARGE, "LARGE writes (interleave expected)");
    close(pfd[0]);
    for (int i = 0; i < NUM_WRITERS; i++) wait(NULL);

    return 0;
}
```

### Example 2: Shell Pipeline in C — dup2() + exec()

```c
/* pipe_shell_pipeline.c
 * Implements: ls -la | grep "\.c" | wc -l  in pure C
 * Build: gcc -o pipe_shell_pipeline pipe_shell_pipeline.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static pid_t spawn(int in_fd, int out_fd,
                   const char *prog, char *const argv[])
{
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); exit(EXIT_FAILURE); }

    if (pid == 0) {
        if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        execvp(prog, argv);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }
    return pid;
}

int main(void)
{
    /*
     * ls -la  →  pipe1  →  grep "\.c"  →  pipe2  →  wc -l
     */
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe"); exit(EXIT_FAILURE);
    }

    char *ls_argv[]   = { "ls", "-la", NULL };
    char *grep_argv[] = { "grep", "\\.c", NULL };
    char *wc_argv[]   = { "wc", "-l", NULL };

    pid_t pid_ls   = spawn(STDIN_FILENO,  pipe1[1], "ls",   ls_argv);
    pid_t pid_grep = spawn(pipe1[0],      pipe2[1], "grep", grep_argv);
    pid_t pid_wc   = spawn(pipe2[0], STDOUT_FILENO, "wc",   wc_argv);

    /* Parent MUST close all pipe ends:
     * if pipe1[1] stays open, grep never sees EOF from ls */
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);

    waitpid(pid_ls,   NULL, 0);
    waitpid(pid_grep, NULL, 0);
    waitpid(pid_wc,   NULL, 0);

    return 0;
}
```

### Example 3: FIFO Client/Server — O_NONBLOCK & SIGPIPE Handling

```c
/* fifo_server.c
 * Server receives requests from multiple unrelated clients via FIFO
 * Build: gcc -o fifo_server fifo_server.c
 * Run:   ./fifo_server  (start first)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#define SERVER_FIFO  "/tmp/server_fifo"
#define MSG_SIZE     64

typedef struct {
    pid_t client_pid;
    char  request[MSG_SIZE];
} Request;

static void setup_sigpipe(void)
{
    struct sigaction sa = { .sa_handler = SIG_IGN };
    sigaction(SIGPIPE, &sa, NULL);
}

int main(void)
{
    setup_sigpipe();  /* write() returns EPIPE instead of killing process */

    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo"); exit(EXIT_FAILURE);
    }

    printf("[server] listening on %s (PID=%d)\n", SERVER_FIFO, getpid());

    /* O_RDWR trick: hold one write-end open so read() never returns EOF
     * when no clients are connected — avoids busy-loop on empty FIFO */
    int sfd = open(SERVER_FIFO, O_RDWR);
    if (sfd == -1) { perror("open"); exit(EXIT_FAILURE); }

    Request req;
    ssize_t n;

    while (1) {
        n = read(sfd, &req, sizeof(req));
        if (n <= 0) continue;

        printf("[server] request from PID %d: \"%s\"\n",
               req.client_pid, req.request);

        /* Each client creates its own response FIFO: /tmp/client_<pid> */
        char client_fifo[64];
        snprintf(client_fifo, sizeof(client_fifo),
                 "/tmp/client_%d", req.client_pid);

        /* O_NONBLOCK: don't block if client has already exited */
        int cfd = open(client_fifo, O_WRONLY | O_NONBLOCK);
        if (cfd == -1) {
            fprintf(stderr, "[server] client %d gone: %s\n",
                    req.client_pid, strerror(errno));
            continue;
        }

        char response[MSG_SIZE];
        snprintf(response, sizeof(response),
                 "ACK: processed \"%s\"", req.request);

        if (write(cfd, response, strlen(response) + 1) == -1)
            perror("write to client");

        close(cfd);
    }

    close(sfd);
    unlink(SERVER_FIFO);
    return 0;
}
```

```c
/* fifo_client.c
 * Build: gcc -o fifo_client fifo_client.c
 * Run:   ./fifo_client "my message"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_FIFO  "/tmp/server_fifo"
#define MSG_SIZE     64

typedef struct {
    pid_t client_pid;
    char  request[MSG_SIZE];
} Request;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char my_fifo[64];
    snprintf(my_fifo, sizeof(my_fifo), "/tmp/client_%d", getpid());

    if (mkfifo(my_fifo, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo client"); exit(EXIT_FAILURE);
    }

    /* Send request to server */
    int sfd = open(SERVER_FIFO, O_WRONLY);
    if (sfd == -1) {
        perror("open server (is server running?)");
        unlink(my_fifo);
        exit(EXIT_FAILURE);
    }

    Request req = { .client_pid = getpid() };
    strncpy(req.request, argv[1], MSG_SIZE - 1);

    printf("[client %d] sending: \"%s\"\n", getpid(), req.request);
    write(sfd, &req, sizeof(req));
    close(sfd);

    /* Read response from our personal FIFO */
    int rfd = open(my_fifo, O_RDONLY);
    if (rfd == -1) { perror("open my_fifo"); exit(EXIT_FAILURE); }

    char response[MSG_SIZE];
    ssize_t n = read(rfd, response, sizeof(response));
    if (n > 0)
        printf("[client %d] response: %s\n", getpid(), response);

    close(rfd);
    unlink(my_fifo);
    return 0;
}
```

---

## Debugging

### Inspect pipe file descriptors in a running process
```bash
ls -la /proc/<pid>/fd/          # see all fds: pipe:[inode] entries
cat /proc/<pid>/fdinfo/<fd>     # pos, flags, pipe inode details
```

### Check pipe buffer usage
```bash
# ioctl FIONREAD: bytes currently in pipe buffer
python3 -c "
import fcntl, struct, termios
fd = open('/proc/self/fd/0').fileno()  # example
import array
buf = array.array('i', [0])
fcntl.ioctl(fd, termios.FIONREAD, buf)
print('bytes in pipe:', buf[0])
"
```

### List all FIFOs on filesystem
```bash
find /tmp -type p           # type p = pipe/FIFO
ls -la /tmp/*.fifo 2>/dev/null
```

### Debug SIGPIPE issues
```bash
# Run under strace to see SIGPIPE delivery and EPIPE errors
strace -e trace=write,signal ./your_program 2>&1 | grep -E "EPIPE|SIGPIPE"
```

### Check pipe capacity and resize
```bash
# Default pipe-max-size
cat /proc/sys/fs/pipe-max-size      # typically 1048576 bytes

# In code: get/set pipe capacity
# fcntl(fd, F_GETPIPE_SZ)  → current capacity
# fcntl(fd, F_SETPIPE_SZ, new_size) → resize (unprivileged: up to pipe-max-size)
```

### Diagnose blocked process on pipe
```bash
cat /proc/<pid>/wchan       # shows kernel function where process is sleeping
                            # pipe_wait or pipe_read = blocked on empty pipe
                            # pipe_write = blocked on full pipe
strace -p <pid>             # attach and see which syscall is blocking
```

---

## Real-world Usage

### Unix shell pipelines
Every `cmd1 | cmd2` in bash/sh uses anonymous pipes:
```bash
cat access.log | grep "ERROR" | awk '{print $4}' | sort | uniq -c | sort -rn
```
The shell: `pipe()` × 4, `fork()` × 5, `dup2()` × 8, `exec()` × 5, `close()` × many.

### Subprocess communication in high-level languages
Python `subprocess.Popen(cmd, stdout=PIPE)`, Node.js `child_process.spawn()`, Go
`exec.Cmd.StdoutPipe()` — all use `pipe()` + `dup2()` under the hood.

### `popen()` — convenience wrapper (with caveats)
```c
FILE *fp = popen("date +%s", "r");   /* creates pipe + forks + execs via /bin/sh */
char buf[32];
fgets(buf, sizeof(buf), fp);
pclose(fp);
```
**Security warning**: `popen()` passes command to `/bin/sh -c` → shell injection risk if
command string includes untrusted user input. Prefer `pipe()` + `fork()` + `execvp()` with
explicit `argv[]` when input is not fully controlled.

### FIFO as syslog channel
Classic embedded pattern: applications write log lines to a well-known FIFO
(`/var/run/log` or `/dev/log` on some systems), a logging daemon reads and routes entries.
Multiple writers, single reader. Write size kept ≤ PIPE_BUF (512–4096 bytes) per record to
guarantee atomicity.

### Pipe as parent/child synchronization barrier
```c
/* parent creates pipe, forks N children
 * each child does work then close(pfd[1])
 * parent read() returns 0 (EOF) only after ALL children done
 * — zero-overhead barrier without any mutex or semaphore */
```
Used in test harnesses, batch job coordinators, pre-fork web servers (e.g., older nginx
worker sync patterns).

### `pipe2(pfd, O_CLOEXEC)` — safe in multi-threaded code
Standard `pipe()` + `fcntl(FD_CLOEXEC)` is a TOCTOU race in multi-threaded programs where
another thread may `fork()+exec()` between the two calls. `pipe2()` (Linux 2.6.27) sets
`O_CLOEXEC` atomically — the safe default in modern code.

---

## Key Takeaways

1. **Pipe = kernel ring buffer, not a file.** Capacity: 65536 bytes (Linux 2.6.11+). Lives
   only while at least one fd refers to it. Data is lost when last fd closes.

2. **Always close unused pipe ends immediately after `fork()`.** Failing to do so prevents
   EOF delivery (reader hangs) or prevents SIGPIPE delivery (writer hangs). This is the
   most common pipe bug.

3. **PIPE_BUF = 4096 bytes on Linux.** Writes ≤ PIPE_BUF are atomic when multiple writers
   exist. Writes > PIPE_BUF may be interleaved — use message queues or locking for large
   concurrent writes.

4. **FIFO blocking `open()` is a feature, not a bug.** It provides a free rendezvous/
   synchronization point. Use `O_RDWR` on the server side to avoid spurious EOFs;
   use `O_NONBLOCK` on the client side when the other end may not be present.

5. **`dup2(oldfd, targetfd)` is the mechanism behind shell pipelines.** After `dup2()`,
   the program being exec'd writes/reads to its standard fds unaware of the pipe.

6. **Byte stream semantics require application-level framing.** If message boundaries
   matter, either: keep each write ≤ PIPE_BUF (≤ 4096) and treat each read as one message,
   or use a fixed-size header encoding message length, or switch to a message queue.

7. **`popen()` is convenient but dangerous with untrusted input.** Shell injection via
   `popen()` is an OWASP command injection vulnerability. Use `execvp()` with `argv[]`
   for safe subprocess execution.

8. **Use `pipe2(pfd, O_CLOEXEC)` in multi-threaded programs** to atomically set
   close-on-exec, preventing fd leaks across `fork()+exec()` in sibling threads.
