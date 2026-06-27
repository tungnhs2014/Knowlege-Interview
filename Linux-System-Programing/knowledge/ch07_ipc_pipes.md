# Chapter 7 - Pipes and FIFOs

> Topics: 7.2 Pipes and FIFOs, `pipe()`, `pipe2()`, `mkfifo()`, named pipes, `dup2()`, `popen()`, blocking, EOF, `PIPE_BUF`
> Main sources: TLPI Ch44; DevLinux Module 08 and Exercises 1-3
> Related files: [IPC overview](ch07_ipc_overview.md), [System V IPC](ch07_ipc_sysv.md), [POSIX IPC](ch07_ipc_posix.md), [Interview](../../interview/ch07_ipc_interview_questions.md)

## Coverage Notes

This file covers mapped row 7.2. Broader IPC taxonomy stays in `ch07_ipc_overview.md`; message queues, semaphores, and shared memory are intentionally covered in the System V and POSIX files.

| Coverage Matrix item | Source | Covered here | Moved/out of scope |
|----------------------|--------|--------------|--------------------|
| 7.2 Pipes and FIFOs | Learning map, TLPI Ch44, DevLinux 08 | `pipe()`, `pipe2()`, `mkfifo()`, named pipes, shell pipelines, FIFO client-server shape | none |
| Byte-stream behavior | Chapter Must Cover, TLPI Ch44 | no message boundaries, ordering, framing, partial reads/writes | message-oriented alternatives moved to SysV/POSIX MQ files |
| Peer relationship/lifetime | Chapter Must Cover | related pipes, unrelated FIFO rendezvous, descriptor inheritance, FIFO pathname lifetime | socket fd passing moved to Chapter 8 |
| EOF/SIGPIPE/close discipline | Chapter Must Cover, TLPI Ch44 | all-writers/all-readers rules, close-on-exec, leaked fd debugging | none |
| `PIPE_BUF`, blocking, backpressure | Chapter Must Cover, TLPI Ch44 | atomicity, capacity, nonblocking read/write behavior, deadlock patterns | exact pipe capacity treated as Linux-specific advanced detail |
| Multi-client FIFO limits | TLPI Ch44, DevLinux 08 | one request FIFO plus per-client response FIFO, record interleaving limits | full server architecture out of scope |
| Production debugging and Embedded constraints | Chapter Must Cover | `/proc/<pid>/fd`, `strace`, FIFO paths, boot hangs, watchdog cleanup | none |

## Learning Goal

Learn pipes and FIFOs as Linux byte-stream IPC: how bytes flow, how descriptors control EOF and broken-pipe behavior, and how to build shell-style pipelines without hidden hangs.

You should leave this topic able to:

- create a one-way parent-child channel with `pipe()`;
- connect a pipe to standard input/output with `dup2()` before `exec()`;
- use a FIFO when unrelated processes need a named rendezvous;
- explain `read()` returning 0, `SIGPIPE`, `EPIPE`, and `PIPE_BUF`;
- debug fd leaks and FIFO startup hangs with `/proc` and `strace`.

## Problem It Solves

Sometimes one local process only needs to stream bytes to another process.

```text
producer writes bytes -> kernel pipe buffer -> consumer reads bytes
```

That is the mechanism behind:

```bash
ls | wc -l
```

The shell creates a pipe, forks two children, wires one child's stdout to the write end, wires the other child's stdin to the read end, closes unused descriptors, and then `exec()`s both programs.

FIFOs solve the same byte-stream problem for unrelated processes. A FIFO has a filesystem pathname, so programs that do not share a parent can still meet at a known path.

## Mental Model

A pipe is a kernel buffer with a read end and a write end. A FIFO is a named pipe: it has a pathname for opening, but I/O behaves like a pipe once opened.

```text
Process A fd table                 Kernel pipe/FIFO buffer              Process B fd table
  fd 4: write end   ------------>  ordered bytes  ------------------>    fd 3: read end
```

| Question | Pipe | FIFO |
|----------|------|------|
| How is it created? | `pipe(pfd)` | `mkfifo(path, mode)` |
| How do peers find it? | inherited or passed fd | filesystem pathname |
| Best for | related processes | unrelated local processes |
| Data model | byte stream | byte stream |
| Message boundaries | not preserved | not preserved |
| Data lifetime | until all fds close | until all fds close |
| Name lifetime | none | until `unlink(path)` |

The key beginner rule is: **a pipe does not know your records**. If records matter, add framing or choose a message queue.

## Mechanism

Pipes and FIFOs are fd-based IPC. They use the normal file-descriptor operations `read()`, `write()`, `close()`, `fcntl()`, `select()`, `poll()`, and `epoll()`.

### Descriptor Copies Matter

After `fork()`, both parent and child inherit both pipe fds unless each process closes the unused end.

```text
pipe(pfd)
fork()

parent has: pfd[0], pfd[1]
child has:  pfd[0], pfd[1]
```

EOF and broken-pipe behavior depend on all descriptor copies in all processes:

- `read()` returns 0 only after **all write ends** are closed and buffered data is drained;
- writing with **no read ends** open raises `SIGPIPE`;
- if `SIGPIPE` is ignored or caught, `write()` fails with `EPIPE`;
- a leaked fd in a grandchild after `exec()` can keep the pipe alive invisibly.

Use `pipe2(O_CLOEXEC)` on Linux or set `FD_CLOEXEC` with `fcntl()` when execed children should not inherit the pipe.

### Byte Stream Behavior

Pipes preserve byte order, not application write boundaries.

```text
writer: write("AB")
writer: write("CD")
reader: read(4) may return "ABCD"
reader: read(1) may return "A"
```

`PIPE_BUF` gives only an atomicity guarantee against interleaving from multiple writers:

- POSIX requires `PIPE_BUF >= 512`;
- Linux commonly defines `PIPE_BUF` as 4096;
- writes of `PIPE_BUF` bytes or less are atomic relative to other writers;
- larger writes may interleave;
- atomic write does not turn a pipe into a message queue.

### FIFO Open Rendezvous

Opening a FIFO can itself block because the kernel waits for the other side.

| Open mode | Other end already open | Other end not open |
|-----------|------------------------|--------------------|
| `O_RDONLY` | succeeds | blocks |
| `O_RDONLY | O_NONBLOCK` | succeeds | succeeds |
| `O_WRONLY` | succeeds | blocks |
| `O_WRONLY | O_NONBLOCK` | succeeds | fails with `ENXIO` |

This is useful as simple synchronization, but it is also a common startup hang.

### Nonblocking I/O Semantics

`O_NONBLOCK` changes both startup and data-flow behavior. It is a tool for timeouts and event loops, not a substitute for a protocol.

| Operation | Blocking mode | Nonblocking mode |
|-----------|---------------|------------------|
| Empty pipe/FIFO read, writer still open | block | fail with `EAGAIN` |
| Empty pipe/FIFO read, no writers | return `0` EOF | return `0` EOF |
| Write `<= PIPE_BUF`, enough space | atomic success | atomic success |
| Write `<= PIPE_BUF`, not enough space | block until the whole write can fit | fail with `EAGAIN` |
| Write `> PIPE_BUF`, some space | may partially write and/or block; interleaving possible | may partially write; interleaving possible |
| Write with no readers | `SIGPIPE`, or `EPIPE` if signal ignored/caught | same |

Handle partial writes even when the common case looks small. Signals, nonblocking mode, large writes, and peer shutdown all change the control flow.

## Key APIs And Objects

The APIs are small; correctness comes from the lifecycle rules.

| API/object | Purpose | Production note |
|------------|---------|-----------------|
| `pipe(int pfd[2])` | create anonymous read/write fds | create before `fork()` |
| `pipe2(pfd, O_CLOEXEC | O_NONBLOCK)` | Linux atomic flag-setting pipe creation | avoids fd-leak race in threaded programs |
| `mkfifo(path, mode)` | create FIFO filesystem entry | `mode` is affected by `umask`; unlink is explicit |
| `open(path, O_RDONLY/O_WRONLY)` | open FIFO endpoint | may block until peer opens other end |
| `read(fd, buf, n)` | consume bytes | `0` means EOF only when no writers remain |
| `write(fd, buf, n)` | append bytes to pipe/FIFO | handle partial writes, `EINTR`, `EPIPE` |
| `close(fd)` | drop descriptor reference | required in every process |
| `dup2(oldfd, targetfd)` | wire pipe to stdin/stdout/stderr | used before `exec()` in pipelines |
| `popen(command, mode)` | run shell command with one pipe | shell-based; unsafe with untrusted command strings |
| `pclose(stream)` | close pipe and wait for shell child | use instead of `fclose()` for `popen()` |
| `O_NONBLOCK` | avoid indefinite blocking | changes both FIFO open and I/O behavior |
| `O_CLOEXEC` / `FD_CLOEXEC` | close fd during `exec()` | prevents hidden EOF/SIGPIPE bugs |

`popen()` is convenient for trusted one-off tool integration. For untrusted input, use `fork()` plus `execve()`/`execvp()` with explicit `argv`.

## Lifecycle / Data Flow

Most pipe/FIFO bugs are lifecycle bugs. Draw the fd ownership before writing code.

### Parent Writes to Child

```text
parent                               child
  pipe(pfd)
  fork()  -------------------------> inherits pfd[0], pfd[1]
  close(pfd[0])                      close(pfd[1])
  write loop on pfd[1]  ---------->  read loop on pfd[0]
  close(pfd[1])                      read() drains, then returns 0
  waitpid(child)
```

Write loops are necessary because `write()` can complete partially or fail with `EINTR`.

### Shell Pipeline

```text
shell
  pipe(pfd)
  fork child 1
      dup2(pfd[1], STDOUT_FILENO)
      close unused descriptors
      exec("ls")
  fork child 2
      dup2(pfd[0], STDIN_FILENO)
      close unused descriptors
      exec("wc", "-l")
  parent closes both pipe descriptors
  wait for children
```

`ls` and `wc` do not know about the pipe. They just use standard output and standard input.

### FIFO Client-Server

```text
server creates /run/myapp/request.fifo

client
  creates /run/myapp/client.<pid>.fifo
  writes request containing response FIFO path

server
  reads request from well-known FIFO
  opens client FIFO
  writes response

client
  reads response
  unlinks private FIFO
```

Use per-client response channels. One shared response FIFO can let clients consume each other's replies.

### Framing a Stream

For structured records, define a protocol:

```text
fixed-size records:
  [struct request][struct request]

length-prefixed records:
  [uint32 length][payload][uint32 length][payload]

line protocol:
  command args...\n
```

Validate lengths before allocation and keep multi-writer FIFO request records at or below `PIPE_BUF` when atomicity matters.

### Minimal Pipe Pattern

This is the small shape to recognize from the old parent-child examples: create before `fork()`, close the unused ends, read until EOF, and wait for the child.

```c
int pfd[2];

if (pipe2(pfd, O_CLOEXEC) == -1)
    die("pipe2");

pid_t pid = fork();
if (pid == -1)
    die("fork");

if (pid == 0) {
    close(pfd[1]);              /* child reads only */
    for (;;) {
        char buf[4096];
        ssize_t n = read(pfd[0], buf, sizeof(buf));
        if (n == 0)
            break;              /* all writers closed */
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1)
            _exit(1);
        write_all(STDOUT_FILENO, buf, (size_t)n);
    }
    _exit(0);
}

close(pfd[0]);                  /* parent writes only */
write_all(pfd[1], msg, msg_len);
close(pfd[1]);                  /* lets child see EOF */
waitpid(pid, NULL, 0);
```

For portable code without Linux `pipe2()`, use `pipe()` and then set `FD_CLOEXEC` with `fcntl()` before any `exec()` path can run.

## Production Bugs And Debugging

Start with fds. Pipes and FIFOs are visible through `/proc/<pid>/fd`, which makes them much easier to debug than many IPC objects.

| Symptom | Likely cause | Evidence | Fix pattern |
|---------|--------------|----------|-------------|
| Reader blocks forever after writer exits | leaked write end in reader, parent, or execed child | `ls -l /proc/<pid>/fd`; same pipe inode still open for write | close unused ends; use close-on-exec |
| Writer blocks forever instead of failing | leaked read end | `/proc/<pid>/fd`, process tree | close read ends in writers/supervisors |
| Writer dies with `Broken pipe` | no readers; default `SIGPIPE` action | shell status, `strace -e write,signal` | handle peer close intentionally |
| FIFO `open(O_WRONLY)` hangs | no reader opened FIFO | `strace -e openat` | start reader first or use `O_NONBLOCK` with retry |
| FIFO server exits between clients | `read()` saw EOF when no writers remained | logs, trace read returning 0 | reopen FIFO or keep a dummy write fd |
| Records are merged/split | stream read boundaries misunderstood | trace read/write sizes | implement framing |
| Multiple writers interleave records | records larger than `PIPE_BUF` | trace write sizes | keep records small or use MQ |
| Receiver appears stuck with stdio | block buffering over pipe | output appears only at process exit | `fflush()`, unbuffered stream, or PTY |
| Command injection through `popen()` | command string includes untrusted input | code review | use `execve()`/`execvp()` with argv |

Useful commands:

```bash
ls -la /proc/<pid>/fd
cat /proc/<pid>/fdinfo/<fd>
readlink /proc/<pid>/fd/<fd>
lsof -p <pid>

find /run /tmp -type p 2>/dev/null
ls -l /path/to/fifo

strace -f -e trace=pipe,pipe2,openat,read,write,close,dup2,execve ./program
strace -f -e trace=write,signal ./program
```

For embedded targets, a good low-impact check is to capture `/proc/<pid>/fd` before attaching heavy tracing.

## DevLinux Practice Bridge

DevLinux Module 08 is the practical companion for this file. Use it to make pipe/FIFO behavior visible instead of treating the APIs as trivia.

| Practice | What to observe | What it proves |
|----------|-----------------|----------------|
| One-way pipe example | Parent/child fd ownership before and after `close()` | EOF depends on every write end closing |
| Bidirectional example | Two pipes, one for each direction | Pipes are unidirectional; two-way protocols need two channels or sockets |
| FIFO writer/reader examples | Blocking in `open()` before the peer exists | FIFO open is a rendezvous point |
| FIFO client-server example | One request FIFO plus per-client response channel | A shared stream is not enough for safe replies |
| Framing examples | delimiter, length-prefix, fixed-size records | Pipes/FIFOs preserve bytes, not message boundaries |

Small experiments worth running after the examples:

- start the FIFO writer before the reader and watch `open()` block or fail with `ENXIO` under `O_NONBLOCK`;
- leave an inherited write fd open in a child and confirm the reader never sees EOF;
- write records larger than `PIPE_BUF` from multiple writers and inspect interleaving risk;
- replace `write()` loops with a single unchecked call and reason about partial writes.

## Embedded Constraints

Pipes and FIFOs are simple, but embedded systems make lifecycle bugs sharper.

- Use `pipe2(O_CLOEXEC)` where available so helper programs do not keep hidden fds alive after `exec()`.
- Prefer service-owned FIFO directories under `/run`; avoid public `/tmp` names unless ownership, permissions, and race handling are designed.
- Add timeouts or nonblocking retry around FIFO startup so one missing peer does not stall boot.
- Keep record sizes bounded; large multi-writer FIFO records can interleave and large pipe buffers consume scarce kernel memory.
- Capture `/proc/<pid>/fd` and `strace` short windows before attaching heavier tools on slow targets.
- Use `lsof -p <pid>` when it exists on the image to confirm which process still owns a pipe or FIFO endpoint.
- After watchdog restart, remove stale FIFO pathnames only if they belong to this service instance; an active peer may still have an open fd to an unlinked object.

## Work Checklist

Use this checklist when writing or reviewing pipe/FIFO code.

- Create anonymous pipes before `fork()`.
- Close every unused pipe end in every process immediately after `fork()`.
- Set `O_CLOEXEC` or `FD_CLOEXEC` for fds that must not survive `exec()`.
- Use `dup2()` only after checking whether the source fd already equals the target fd.
- Read in a loop until EOF or protocol-complete record.
- Write in a loop for buffers that may be partially written.
- Handle `EINTR`, `EAGAIN`, `SIGPIPE`, and `EPIPE` intentionally.
- Define framing for records; never assume one `write()` equals one `read()`.
- Keep multi-writer FIFO records `<= PIPE_BUF` or use a message queue.
- Add timeout/nonblocking behavior to avoid permanent startup hangs.
- Avoid public `/tmp` FIFO names unless ownership, permissions, and race handling are designed.
- Clean up FIFO pathnames during normal shutdown and safe startup recovery.

## Recognize / Advanced

These are useful to recognize in code reviews and production debugging.

| Topic | Recognize this |
|-------|----------------|
| `pipe2()` | Linux-specific atomic creation with flags such as `O_CLOEXEC` |
| `F_GETPIPE_SZ` / `F_SETPIPE_SZ` | Linux pipe capacity query/tuning; not a correctness contract |
| FIFO `O_RDWR` | Works on Linux, unspecified by POSIX, and can hide EOF |
| `ioctl(FIONREAD)` | Nonstandard unread-byte query |
| Pipe as readiness/synchronization | Parent waits for byte or EOF from child |
| `splice()` / `tee()` | Linux pipe helpers for optimized data movement |
| PTY instead of pipe | Needed when a child behaves differently unless connected to a terminal |

## Interview Readiness

You should be ready to explain pipe behavior from fd lifetime, not from memorized slogans.

Practice answering:

- Why are pipes and FIFOs byte streams rather than message queues?
- Why must unused ends be closed after `fork()`?
- When does `read()` return 0 on a pipe?
- What happens when writing with no readers?
- What does `PIPE_BUF` guarantee, and what does it not guarantee?
- How does a shell implement `ls | wc -l`?
- Why can FIFO open order hang?
- How would you debug a process stuck reading from a pipe?

Interview anchor answer:

```text
A pipe is a kernel byte stream with read and write descriptor references.
EOF appears only after all write ends close; broken pipe appears only after all read ends close.
Because it is a stream, records need framing or PIPE_BUF-sized atomic writes.
Most production bugs are leaked fds, missing close-on-exec, FIFO open-order hangs, or bad framing.
```

## Final Coverage Check

- [x] Mapped row 7.2 is covered directly.
- [x] The pipe/FIFO Must Cover items are covered here: unidirectional stream semantics, EOF/SIGPIPE, `PIPE_BUF`, close discipline, deadlocks, multi-client limits, blocking behavior, and production debugging.
- [x] System V/POSIX IPC family details are intentionally moved to their dedicated files.
- [x] Embedded constraints are covered through boot ordering, `/run` paths, close-on-exec, bounded records, and watchdog cleanup.
- [x] Existing useful examples and DevLinux practice links were preserved; no mapped pipe/FIFO topic is intentionally out of scope.
