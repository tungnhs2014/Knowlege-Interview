# Chapter 9.3 - Pseudoterminals (PTY)

> Topics: PTY master/slave model, UNIX 98 PTY APIs, `posix_openpt()`, `grantpt()`, `unlockpt()`, `ptsname()`, `ptyFork()` pattern, relay loops, packet mode, terminal attributes and window size.
> Main sources: TLPI Ch64, `LINUX_SYSTEM_LEARNING_MAP.md`.
> Production context: SSH-like remote shells, terminal emulators, `script(1)`, `expect`, container exec/attach flows, test automation for interactive programs, and tools that need terminal semantics over sockets/pipes.

---

## Problem It Solves

Some programs expect to run on a terminal, not on a plain pipe or socket. A shell, `vi`, `less`, `login`, and many password prompts expect:

- a controlling terminal;
- foreground process group and job-control behavior;
- terminal-driver processing such as echo, canonical mode, special characters, and signals;
- terminal ioctls such as `tcsetattr()` and window-size queries.

A socket can move bytes across a network, but it cannot directly be a terminal. A PTY provides the missing bridge:

```text
driver process <-> PTY master <-> PTY slave <-> terminal-oriented program
```

The slave side behaves like a real terminal. The master side is controlled by a relay/driver program.

## Learning Roadmap

| Level | Learn | Goal |
|---|---|---|
| Must know | PTY master/slave, UNIX 98 allocation flow, `setsid()`, opening the slave as controlling terminal, `dup2()` to stdio | Understand how shells run under SSH, terminal emulators, and `script(1)`. |
| Work useful | Relay loops with `poll()`/`select()`, terminal raw mode on the outer terminal, window-size propagation, close/error handling | Build/debug PTY tools without broken terminal state or hung children. |
| Recognize | Packet mode, BSD PTYs, `openpty()`/`forkpty()`, login accounting, buffering workaround tools | Read mature terminal/remote-login code and know where the tricky parts live. |

## Core Vocabulary

| Term | Meaning | Example / note |
|---|---|---|
| PTY | Pair of virtual devices providing terminal semantics over an IPC channel. | Used by `ssh`, xterm, `script`, `expect`. |
| PTY master | Driver/relay side of the pair. | SSH server reads/writes the master. |
| PTY slave | Terminal-like side of the pair. | Child shell sees this as its terminal. |
| UNIX 98 PTY | Standard System V style PTY interface used by modern Linux. | Based on `/dev/ptmx` and `/dev/pts/N`. |
| `/dev/ptmx` | PTY master clone device. | `posix_openpt()` opens it on Linux. |
| `/dev/pts/N` | PTY slave pathname. | Returned by `ptsname()`. |
| `posix_openpt()` | Opens an unused PTY master. | Usually with `O_RDWR | O_NOCTTY`. |
| `grantpt()` | Adjusts slave ownership/permissions where needed. | Linux often auto-configures, but portable code still calls it. |
| `unlockpt()` | Unlocks slave so it can be opened. | Opening before unlock can fail with `EIO`. |
| `ptsname()` | Returns slave device name for a master. | May return a statically allocated string. |
| Controlling terminal | Terminal associated with a session. | Enables `/dev/tty` and job-control signals. |
| `setsid()` | Creates a new session and detaches from old controlling terminal. | Required before opening slave for child session. |
| Session leader | Process that leads a session. | After `setsid()`, child becomes session leader. |
| Foreground process group | Process group allowed to read from controlling terminal. | Terminal signals target this group. |
| `dup2()` | Duplicates slave FD onto stdin/stdout/stderr. | Makes child program use PTY as stdio. |
| Relay loop | Parent loop forwarding bytes between master and another channel. | Terminal <-> master or socket <-> master. |
| `termios` | Terminal attributes shared with PTY terminal behavior. | Apply to slave for child session. |
| `winsize` | Rows/columns tracked by terminal driver. | Propagate `SIGWINCH` changes. |
| Packet mode | PTY master mode that reports slave state changes. | Enabled with `ioctl(TIOCPKT)`. |
| `SIGHUP` | Hangup signal sent when terminal disappears. | Closing all master FDs can hang up slave session. |
| `EIO` | Common Linux PTY close/error indicator. | `read(master)` may fail with `EIO` after slave closes. |
| BSD PTY | Older pre-created `/dev/ptyXY` / `/dev/ttyXY` style. | Deprecated/legacy on Linux. |

## Concept Overview

The master is not "the terminal"; the slave is the terminal-like endpoint. The master is a control channel into that terminal.

```text
write(master, "abc\n")
    |
    v
terminal driver for slave applies canonical/raw/echo/signal rules
    |
    v
child process reads from stdin connected to slave

child writes to stdout/stderr connected to slave
    |
    v
driver process reads output from master
```

This is why PTYs are different from a bidirectional pipe. A pipe only carries bytes. A PTY slave also has terminal semantics.

## System Context

PTYs connect multiple Linux subsystems:

| Subsystem | PTY interaction |
|---|---|
| Process/session/job control | Child uses `setsid()` and opens the slave as controlling terminal. |
| Terminal driver | Slave side handles canonical mode, echo, special chars, `termios`. |
| Alternative I/O | Parent relay monitors master plus terminal/socket with `select()`/`poll()`/`epoll`. |
| Signals | `Ctrl-C` written through master can become `SIGINT` to slave foreground process group. |
| Window size | Parent may copy real terminal size to slave and propagate later `SIGWINCH`. |
| Sockets | SSH/telnet-like programs bridge network socket <-> PTY master. |
| Security/login | Remote login services add authentication and login accounting around PTY setup. |

If setup order is wrong, the child may run but not have a controlling terminal, causing interactive programs to behave incorrectly.

## Architecture

### UNIX 98 Allocation Pipeline

```text
posix_openpt(O_RDWR | O_NOCTTY)
    |
    v
grantpt(master_fd)
    |
    v
unlockpt(master_fd)
    |
    v
ptsname(master_fd)
    |
    v
open(slave_name, O_RDWR)
```

Linux notes from TLPI:

- `posix_openpt()` is implemented like opening `/dev/ptmx`;
- Linux exposes slave devices under `/dev/pts`;
- PTY limits are visible through `/proc/sys/kernel/pty/max` and `/proc/sys/kernel/pty/nr`;
- Linux often does not need `grantpt()`, but portable code should still call it.

### `ptyFork()` Pattern

Parent:

```text
open PTY master
    |
    v
fork()
    |
    v
keep master FD and relay I/O
```

Child:

```text
setsid()
    |
    v
close inherited master
    |
    v
open PTY slave -> controlling terminal
    |
    v
optional tcsetattr() and TIOCSWINSZ
    |
    v
dup2(slave, STDIN/STDOUT/STDERR)
    |
    v
exec(shell or terminal-oriented program)
```

On some BSD-like systems, an explicit `ioctl(TIOCSCTTY)` may be needed to acquire the controlling terminal.

### Relay Architecture

```text
real terminal or socket
    ^
    |
    v
driver process
    ^
    |
    v
PTY master <-> PTY slave <-> child shell/program
```

The driver usually monitors both directions:

- external channel -> master;
- master -> external channel and maybe log file.

### PTY I/O and Close Semantics

| Event | Linux behavior to remember |
|---|---|
| Write to master | Appears as input to slave, after terminal-driver processing. |
| Write to slave | Can be read from master. |
| Slave in canonical mode | Child may not receive input until newline/EOF condition. |
| All master FDs closed | Slave session may get `SIGHUP`; `read(slave)` returns EOF; `write(slave)` may fail with `EIO`. |
| All slave FDs closed | `read(master)` fails with `EIO` on Linux; other UNIX systems may return EOF. |
| PTY buffer full | Writes can block unless nonblocking. |

Implementation-dependent close behavior means production relays should treat EOF, `EIO`, HUP, and read errors as terminal lifecycle events.

## Execution Flow

### Open a PTY pair

```text
open master
    |
    v
grant and unlock slave
    |
    v
get slave name
    |
    v
open slave when child/session is ready
```

### Spawn an interactive child

```text
parent opens PTY master
    |
    v
fork
    |
    +--> child: setsid -> open slave -> dup2 -> exec shell
    |
    +--> parent: close unused slave path, relay via master
```

### Remote shell relay

```text
network socket readable
    |
    v
read socket -> write PTY master
    |
    v
child shell receives terminal input on slave

PTY master readable
    |
    v
read master -> write network socket
```

### `script(1)`-style recording

```text
capture real terminal termios + winsize
    |
    v
ptyFork child shell using same termios + winsize
    |
    v
put real terminal in raw mode
    |
    v
relay stdin -> master, master -> stdout + log file
    |
    v
restore real terminal on exit
```

### Window-size propagation

```text
real terminal receives resize
    |
    v
parent gets SIGWINCH
    |
    v
ioctl(real_tty, TIOCGWINSZ)
    |
    v
ioctl(pty_master, TIOCSWINSZ)
    |
    v
kernel sends SIGWINCH to foreground process group on slave
```

## 9.3 API / Topic Sections

### 9.3.1 Opening a UNIX 98 PTY

Use:

```text
int mfd = posix_openpt(O_RDWR | O_NOCTTY);
grantpt(mfd);
unlockpt(mfd);
char *slave = ptsname(mfd);
```

Pitfalls:

| Pitfall | Symptom | Fix |
|---|---|---|
| Skip `unlockpt()` | Opening slave may fail with `EIO`. | Always call after `grantpt()`. |
| Trust `ptsname()` pointer forever | Later call may overwrite static buffer. | Copy it if needed. |
| Skip `grantpt()` for portability | Works on Linux, fails elsewhere. | Keep the call. |
| Forget `O_NOCTTY` | Portability surprise around controlling terminal. | Use `O_RDWR | O_NOCTTY` for master open. |

### 9.3.2 Connecting a Child to the Slave

The child must become a session leader before opening the slave so the slave can become the controlling terminal.

Correct order:

```text
setsid()
open(slave_name, O_RDWR)
dup2(slave_fd, 0)
dup2(slave_fd, 1)
dup2(slave_fd, 2)
exec(...)
```

If the child opens the slave before `setsid()`, it may not get the intended controlling-terminal behavior.

### 9.3.3 Relay Loops

Relay loops are ordinary event loops:

| Source ready | Action |
|---|---|
| external input | `read(external)` then `write(master)`. |
| PTY master input | `read(master)` then `write(external)` and optionally log. |
| EOF/HUP/`EIO` | Exit relay, close FDs, wait/reap child if needed. |
| resize signal | Copy window size to PTY. |

Use `poll()` or `select()` for simple relays. Use nonblocking plus `epoll` only when scaling to many PTYs/sockets.

### 9.3.4 Terminal Attributes and Window Size

The master can be used to affect terminal attributes/window size for the slave-side terminal behavior. This is useful when a parent wants the child shell to inherit the user's real terminal profile.

Common setup:

- read real terminal `termios` with `tcgetattr(STDIN_FILENO, &t)`;
- read real window size with `ioctl(STDIN_FILENO, TIOCGWINSZ, &ws)`;
- apply to slave during `ptyFork()` with `tcsetattr()` and `TIOCSWINSZ`;
- handle later `SIGWINCH` changes.

### 9.3.5 Packet Mode

Packet mode is enabled on the master:

```text
ioctl(master_fd, TIOCPKT, &on)
```

It reports slave-side state changes such as flow-control and queue flush events. In readiness APIs:

| API | Packet-mode notification |
|---|---|
| `select()` | master appears in `exceptfds`. |
| `poll()` | master reports `POLLPRI`. |

Packet mode is not standardized by SUSv3. Use it only when implementing terminal/network-login behavior that needs those control events.

## Work-Useful Patterns

| Pattern | Use it when | Notes |
|---|---|---|
| Wrap PTY allocation | Many callers need PTYs | Hide `posix_openpt()`/`grantpt()`/`unlockpt()`/`ptsname()` and copy slave name safely. |
| `setsid()` before opening slave | Spawning terminal-oriented child | Lets slave become controlling terminal. |
| `dup2()` slave to stdio | Child should behave like it is on a terminal | Close extra slave FD after duplication. |
| Put outer terminal in raw mode | Transparent relay like `script` or SSH client side | Avoid double interpretation by real terminal and PTY slave. |
| Relay with bounded writes | PTY or socket can block/partially write | Use nonblocking or robust `write_all()` depending on design. |
| Propagate window size | Full-screen apps under PTY | Handle `SIGWINCH`, then `TIOCSWINSZ` on PTY. |
| Treat `EIO` as lifecycle | Linux PTY master after slave closes | Exit/cleanup instead of logging as mysterious corruption. |
| Reap child | Parent relay may outlive child briefly | Use `waitpid()` or signal-driven cleanup. |
| Keep security separate | Remote login tools | PTY gives terminal semantics; authentication/authorization is a separate layer. |

## Advanced / Recognize First

| Topic | Know this much |
|---|---|
| BSD PTYs | Older `/dev/ptyXY`/`/dev/ttyXY` pairs; legacy/deprecated on Linux. |
| `openpty()` / `forkpty()` | Nonstandard helper APIs provided by glibc/BSDs; convenient wrappers around PTY setup. |
| Login accounting | SSH/telnet-style services may update utmp/wtmp; `script(1)` does not need to. |
| Packet mode | Master receives control bytes for slave state changes; useful in network login implementations. |
| `unbuffer` pattern | PTY can make programs use terminal-style line buffering instead of pipe/file block buffering. |
| PTY limits | `/proc/sys/kernel/pty/max` and `/proc/sys/kernel/pty/nr` show Linux PTY capacity and current use. |
| Security of remote shells | PTY setup is not authentication, sandboxing, or authorization. Combine with process credentials/capabilities/namespace policy as needed. |

## Example

### Example 1: Open a UNIX 98 PTY master

```c
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int mfd = posix_openpt(O_RDWR | O_NOCTTY);
    if (mfd == -1) {
        perror("posix_openpt");
        return 1;
    }
    if (grantpt(mfd) == -1) {
        perror("grantpt");
        close(mfd);
        return 1;
    }
    if (unlockpt(mfd) == -1) {
        perror("unlockpt");
        close(mfd);
        return 1;
    }

    char *slave_name = ptsname(mfd);
    if (slave_name == NULL) {
        perror("ptsname");
        close(mfd);
        return 1;
    }

    printf("master fd=%d slave=%s\n", mfd, slave_name);
    close(mfd);
    return 0;
}
```

What it teaches:

- The UNIX 98 allocation sequence is short but order-sensitive.
- `grantpt()` remains in portable code even if Linux often does the work automatically.
- `ptsname()` gives the slave path, normally under `/dev/pts`.

### Example 2: Child-side attach pattern

```c
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int attach_slave_and_exec(const char *slave_name, const char *program) {
    int sfd;

    if (setsid() == -1) {
        return -1;
    }

    sfd = open(slave_name, O_RDWR);
    if (sfd == -1) {
        return -1;
    }

    if (dup2(sfd, STDIN_FILENO) == -1 ||
        dup2(sfd, STDOUT_FILENO) == -1 ||
        dup2(sfd, STDERR_FILENO) == -1) {
        close(sfd);
        return -1;
    }
    if (sfd > STDERR_FILENO) {
        close(sfd);
    }

    execlp(program, program, (char *) NULL);
    _exit(127);
}
```

What it teaches:

- `setsid()` comes before opening the slave.
- `dup2()` connects the terminal-oriented program to the slave.
- If `exec` fails in a child, use `_exit()` rather than returning into parent-style cleanup.

### Example 3: Minimal relay loop with `poll()`

```c
#include <errno.h>
#include <poll.h>
#include <unistd.h>

static int write_all(int fd, const char *buf, ssize_t len) {
    ssize_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, (size_t) (len - off));
        if (n > 0) {
            off += n;
            continue;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        return -1;
    }
    return 0;
}

int relay_terminal_to_master(int master_fd) {
    struct pollfd fds[2];
    char buf[256];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = master_fd;
    fds[1].events = POLLIN;

    for (;;) {
        int ready = poll(fds, 2, -1);
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (fds[0].revents & POLLIN) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) {
                return 0;
            }
            if (write_all(master_fd, buf, n) == -1) {
                return -1;
            }
        }

        if (fds[1].revents & POLLIN) {
            ssize_t n = read(master_fd, buf, sizeof(buf));
            if (n <= 0) {
                return 0;
            }
            if (write_all(STDOUT_FILENO, buf, n) == -1) {
                return -1;
            }
        }

        if ((fds[0].revents | fds[1].revents) & (POLLHUP | POLLERR | POLLNVAL)) {
            return 0;
        }
    }
}
```

What it teaches:

- A PTY relay is just an event loop with two directions.
- Handle `EINTR` and partial writes.
- Treat HUP/error as lifecycle, not as an impossible state.

## Debugging

Useful commands:

```bash
cat /proc/sys/kernel/pty/max
cat /proc/sys/kernel/pty/nr
ls -l /dev/pts
tty
ps -o pid,ppid,sid,pgid,tpgid,tty,stat,cmd -p <pid>
ls -l /proc/<pid>/fd
strace -f -e trace=openat,ioctl,dup2,setsid,read,write,close,wait4 -p <pid>
```

Common bugs:

| Bug | Symptom | Fix / check |
|---|---|---|
| Open slave before `setsid()` | Child lacks proper controlling terminal behavior. | Call `setsid()` first in child. |
| Skip `unlockpt()` | `open(slave)` fails with `EIO`. | Use full UNIX 98 sequence. |
| Forget to connect stderr | Errors disappear or go to wrong terminal. | `dup2(slave, 0/1/2)`. |
| Outer terminal not raw in transparent relay | Double echo, `Ctrl-C` handled by wrong side, strange line buffering. | Put real terminal in raw mode and restore. |
| No window-size propagation | `vi`/TUI displays wrong after resize. | Handle `SIGWINCH` and call `TIOCSWINSZ`. |
| Treat Linux `read(master) == -1/EIO` as fatal surprise | Relay logs noisy error when child exits. | Treat as slave closed; cleanup. |
| Not reaping child | Zombie process after relay exits. | `waitpid()` or child lifecycle handling. |
| Blocking write in relay | Relay hangs when peer stops reading. | Use nonblocking I/O or robust backpressure design. |

## Real-world Usage

| Scenario | Practical design |
|---|---|
| SSH server session | Authenticated server child owns PTY master; shell/login runs on slave. |
| Terminal emulator | GUI process owns master; shell runs on slave. |
| `script(1)` | Parent relays real terminal <-> master and writes master output to log. |
| `expect`-style automation | Driver writes scripted input to master and reads interactive output. |
| Container exec/attach | Runtime gives process a PTY slave so interactive programs behave normally. |
| Test interactive CLI | Use PTY to trigger terminal-specific behavior not seen with pipes. |
| Unbuffering wrapper | Run program under PTY so stdio treats stdout as terminal and line-buffers. |

## Interview-Relevant Questions

1. Why can a socket not replace a terminal for programs like `bash` or `vi`?
2. What is the difference between a PTY master and PTY slave?
3. Why does the slave side behave differently from a pipe?
4. What is the UNIX 98 PTY allocation sequence?
5. Why should portable code call `grantpt()` on Linux even if it often appears unnecessary?
6. What happens if you open the slave before `unlockpt()`?
7. Why does the child call `setsid()` before opening the slave?
8. How does a child acquire a controlling terminal in the PTY pattern?
9. Why must the slave FD be duplicated onto stdin/stdout/stderr?
10. What does a PTY relay loop monitor?
11. Why does `script(1)` put the user's real terminal in raw mode?
12. How does a PTY help SSH provide an interactive remote shell?
13. What happens when all master FDs close?
14. What Linux behavior is common when reading the master after the slave closes?
15. How do `termios` settings apply to PTYs?
16. How should window-size changes be propagated through a PTY?
17. What is PTY packet mode, and where do its notifications appear in `select()`/`poll()`?
18. What are `openpty()` and `forkpty()`?
19. Why can PTYs change stdio buffering behavior compared with pipes?
20. What would you inspect with `ps` to verify PTY session setup?

## Key Takeaways

1. PTY is the bridge between byte transport and terminal semantics.
2. The master is controlled by the relay; the slave is the terminal-like endpoint.
3. Modern Linux code should use UNIX 98 PTYs: `posix_openpt()`, `grantpt()`, `unlockpt()`, `ptsname()`.
4. Child setup order matters: `setsid()`, open slave, duplicate to stdio, then `exec()`.
5. The slave side uses terminal-driver behavior from Chapter 9.2.
6. Relay loops use Chapter 9.1 readiness APIs to move bytes both ways.
7. Close behavior is not exactly pipe behavior; Linux commonly reports `EIO` on master read after slave close.
8. Window size and terminal attributes must be copied/propagated for full-screen programs.
9. `script`, SSH, terminal emulators, `expect`, and container attach all rely on this pattern.
10. Packet mode is specialized; recognize it, but do not lead with it.
11. PTY setup does not replace authentication, authorization, or sandboxing.
12. Correct PTY tools treat terminal restore, child reaping, HUP/EOF, and resize as first-class lifecycle work.
