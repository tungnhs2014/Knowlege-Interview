# Chapter 9 Interview - Embedded Special: Event Loops, Terminals, Serial, and PTYs

> Scope: Linux readiness-based I/O with `select()`, `poll()`, and `epoll`; terminal programming with `termios`; serial/UART behavior in Embedded Linux; pseudoterminals for interactive tools, remote shells, and container/session attach flows.
> Primary repo sources: `knowledge/ch09_io_multiplexing.md`, `knowledge/ch09_terminals.md`, `knowledge/ch09_pty.md`.
> Mapped TLPI-derived docs: `docs/Linux-Programming-Interface/ch63_alternative_io_models.md`, `ch62_terminals.md`, `ch64_pseudoterminals.md`.
> DevLinux mapping: none for Chapter 9. This file does not invent a DevLinux source for this chapter.

---

## Review Basis

This interview file was reviewed against the Chapter 9 learning map, the three Chapter 9 knowledge files, and TLPI-derived chapters 62, 63, and 64.

Correctness sources:

- Repo knowledge: readiness semantics, event-loop production patterns, terminal/serial debug workflow, PTY lifecycle and relay behavior.
- TLPI Chapter 63: alternative I/O models, readiness vs actual I/O, `select()`, `poll()`, `epoll`, level-triggered vs edge-triggered notification, `pselect()`, and self-pipe.
- TLPI Chapter 62: terminal driver model, `termios`, canonical and noncanonical mode, `VMIN`/`VTIME`, raw/cbreak mode, line speed, `tcdrain()`, `tcflush()`, `SIGWINCH`, and `stty`.
- TLPI Chapter 64: PTY master/slave model, UNIX 98 PTY flow, `ptyFork()` pattern, relay loops, close behavior, window-size propagation, and packet mode.
- Linux man-pages: `select(2)`, `poll(2)`, `epoll(7)`, `epoll_create(2)`, `epoll_ctl(2)`, `epoll_wait(2)`, `fcntl(2)`, `read(2)`, `write(2)`, `close(2)`, `termios(3)`, `tty(4)`, `pty(7)`, `posix_openpt(3)`, `grantpt(3)`, `unlockpt(3)`, `ptsname(3)`, `openpty(3)`, `forkpty(3)`, `signal(7)`, `signalfd(2)`, `timerfd_create(2)`, and `eventfd(2)`.

Interview-priority calibration:

- Official company prep pages from Amazon, Microsoft, Google, and Meta were used to calibrate for applied problem solving, design, testing, and communication rather than memorization.
- Linux man-pages and official technical docs were used as technical authority.
- Public recurring OS/system interview banks were considered only as weak signals that `select()` vs `poll()` vs `epoll`, blocking vs nonblocking I/O, and terminal/PTY basics recur often.
- Embedded Linux context was calibrated with official Linux/man-page behavior plus vendor-style Linux documentation showing UART exposed through standard Linux device files such as `/dev/ttyS*`, with baud, parity, stop-bit, and flow-control concerns.

---

## Priority Map

### A - Project and production scenarios

Study these deeply. The interviewer expects a design/debug answer, not a definition dump.

| Scenario | Core topics |
|---|---|
| Event-driven TCP service works in staging but spins at 100% CPU in production. | readiness vs actual I/O, always-ready FDs, `EPOLLOUT` storms, HUP/ERR, regular files, busy loops |
| Epoll server randomly stops receiving data after switching to edge-triggered mode. | level vs edge trigger, `O_NONBLOCK`, drain-until-`EAGAIN`, starvation, partial reads |
| Service using `select()` breaks after enough clients or files are opened. | `FD_SETSIZE`, `nfds`, value-result `fd_set`, `poll()` and `epoll` migration |
| Event loop handles many idle clients plus slow clients on an embedded gateway. | thread-per-client vs event loop, backpressure, partial writes, protocol framing, accepted socket nonblocking |
| Serial device on `/dev/ttyUSB0` blocks forever or reads partial packets. | `termios`, raw mode, `VMIN`/`VTIME`, `O_NONBLOCK`, framing, checksums, permissions, hotplug |
| Embedded app reads binary UART frames but canonical terminal mode corrupts behavior. | terminal driver, `ICANON`, `ECHO`, `ISIG`, `IEXTEN`, CR/NL mapping, flow control |
| CLI/password/TUI tool leaves the user's terminal with no echo after a crash. | save-copy-modify-restore `termios`, raw/cbreak, signal cleanup, `stty sane` |
| Local interactive command works in a terminal but fails through a pipe. | terminal vs byte stream, `isatty()`, `/dev/tty`, job control, line discipline |
| SSH/container/terminal-emulator style tool needs a PTY rather than a pipe. | PTY master/slave, controlling terminal, `setsid()`, `dup2()`, `SIGHUP`, relay loop |
| PTY app does not resize correctly or behaves differently after the slave closes. | `SIGWINCH`, `TIOCGWINSZ`, `TIOCSWINSZ`, PTY close semantics, Linux `EIO`, child reaping |

### B - Design comparisons and senior follow-ups

Know these well enough to answer trade-offs and follow-up probes.

| Topic | Expected depth |
|---|---|
| `select()` vs `poll()` vs `epoll` | Explain API shape, scaling behavior, portability, and failure modes. |
| Readiness vs completion | Distinguish `select`/`poll`/`epoll` from POSIX AIO or completion queues. |
| Blocking vs nonblocking | Explain why nonblocking is still needed after readiness notification. |
| `pselect()` / `ppoll()` / `epoll_pwait()` | Recognize signal-mask race avoidance. |
| `signalfd`, `timerfd`, `eventfd` | Recognize event-loop integration on Linux. |
| `EPOLLONESHOT` | Recognize worker ownership and rearm pattern. |
| `EPOLLERR`, `EPOLLHUP`, `EPOLLRDHUP` | Treat as normal lifecycle, not optional extras. |
| `tcdrain()` vs `tcflush()` | Explain serial protocol boundaries and stale input handling. |
| `openpty()` / `forkpty()` | Recognize convenient wrappers over lower-level PTY setup. |
| PTY vs pipe vs socket | Explain terminal semantics, transport semantics, and security boundaries. |

### C - Lower-priority / know enough to recognize

Recognize these names and know where to look them up. Do not spend first-pass interview time memorizing every bit value.

- Signal-driven I/O with `O_ASYNC` and `SIGIO`.
- POSIX AIO as completion-based I/O.
- BSD `kqueue` and Solaris `/dev/poll`.
- Every `termios` flag, legacy delay flags, obscure line disciplines, and packet-mode byte values.
- BSD PTYs and login accounting.
- `EPOLLWAKEUP`, `EPOLLEXCLUSIVE`, nested epoll instances, and detailed `/proc/sys/fs/epoll` tuning.

---

## Final Interview List

### Priority A

1. An event-driven TCP service works under low traffic but spins at 100% CPU in production. How do you debug and fix it?
2. An epoll-based server randomly stops receiving data after switching from level-triggered to edge-triggered mode. What went wrong?
3. A service using `select()` breaks after enough clients or unrelated files are opened. How do you explain the failure and migrate it safely?
4. You are designing an embedded gateway with many idle TCP clients, one UART, timers, and a shutdown signal. Would you use thread-per-client or an event loop?
5. A serial device on `/dev/ttyUSB0` sometimes blocks forever and sometimes returns partial packets. How do you debug the serial configuration and read strategy?
6. An embedded app reads binary UART frames, but bytes are echoed, line endings change, and `Ctrl-S` appears to freeze output. What terminal mode mistake is likely?
7. A CLI/TUI/password tool crashes and leaves the user's terminal with no echo. What should the program have done?
8. A command works interactively but fails when run through a pipe in automation. How do you decide whether it needs a terminal or a PTY?
9. You are building an SSH/container attach/remote shell feature. Why is a PTY required, and what setup order matters?
10. A PTY-based terminal emulator or relay does not resize correctly and sometimes logs `EIO` after the child exits. What should the lifecycle design be?

### Priority B

11. Compare `select()`, `poll()`, and `epoll` for small tools, portable code, and high-FD Linux services.
12. Explain readiness-based I/O vs completion-based I/O.
13. Why does readiness not mean a complete protocol message is available?
14. Why should accepted sockets be made nonblocking explicitly or via `accept4()` in an event-loop server?
15. How should an event loop handle partial writes and backpressure for slow clients?
16. What problem do `pselect()`, `ppoll()`, and `epoll_pwait()` solve?
17. When do `signalfd`, `timerfd`, and `eventfd` improve an event-loop design?
18. What do `tcdrain()`, `tcflush()`, baud rate, parity, stop bits, and flow control mean for serial protocols?
19. How do window-size changes propagate through a PTY?
20. What are `openpty()` and `forkpty()`, and why should you still understand `posix_openpt()` flow?

### Priority C

- Recognize signal-driven I/O, POSIX AIO, `kqueue`, `/dev/poll`, BSD PTYs, packet mode, login accounting, and detailed `termios` flag tables.

---

## High-Value Comparisons

| Comparison | Strong interview answer |
|---|---|
| Readiness vs actual I/O | Readiness means an operation should not block; it may still return EOF, error, or only part of the data. |
| Readiness vs completion | `select()`, `poll()`, and `epoll` tell you when to try I/O. Completion APIs notify after an I/O operation has completed. |
| Blocking I/O vs event loop | Blocking on one FD is simple but can freeze progress on others. An event loop waits for any watched FD to make progress. |
| Nonblocking polling vs `poll()` | Repeated nonblocking reads can burn CPU. `poll()` lets the kernel sleep the process until readiness or timeout. |
| `select()` vs `poll()` | `select()` uses fixed-size value-result bit sets and `nfds`. `poll()` uses an array with stable `events` and returned `revents`. |
| `poll()` vs `epoll` | `poll()` passes and scans the full array each wait. `epoll` keeps interest in the kernel and returns ready events. |
| Level-triggered vs edge-triggered | Level-triggered repeats notification while the condition remains true. Edge-triggered reports changes and requires draining until `EAGAIN`. |
| `EPOLLIN` vs complete request | `EPOLLIN` means read can make progress, not that a full application frame or HTTP request is present. |
| `EPOLLOUT` always on vs demand-driven | Write readiness is often true; enable it only while output is queued to avoid wakeup storms. |
| Canonical vs noncanonical | Canonical mode is line-oriented with editing. Noncanonical mode is byte-oriented and controlled by `VMIN`/`VTIME`. |
| Cbreak vs raw | Cbreak keeps useful signal characters; raw disables most terminal processing and transformations. |
| Terminal vs pipe/socket | A terminal has line discipline, special characters, signals, job control, `termios`, and window size. A pipe/socket moves bytes. |
| PTY master vs slave | The master is controlled by the relay. The slave behaves like a terminal for the child process. |
| PTY vs pipe | A PTY provides terminal semantics on the slave side; a pipe cannot satisfy terminal-only operations. |
| `posix_openpt()` flow vs `openpty()`/`forkpty()` | `posix_openpt()` flow is the portable mental model; `openpty()`/`forkpty()` are convenient nonstandard wrappers. |

---

## Common Project Failure Patterns

- Using blocking sockets, UART FDs, pipes, or PTY masters inside an event loop.
- Switching to edge-triggered epoll but reading or accepting only once per event.
- Forgetting to set accepted sockets nonblocking.
- Assuming readiness means a full protocol message, newline, UART frame, or response is available.
- Ignoring partial `read()` and `write()` results.
- Keeping `EPOLLOUT` enabled for every connection.
- Ignoring `EPOLLERR`, `EPOLLHUP`, `EPOLLRDHUP`, `POLLERR`, `POLLHUP`, or `POLLNVAL`.
- Closing or duplicating FDs without clear epoll interest-list ownership.
- Using `select()` with fd numbers at or above `FD_SETSIZE`.
- Reusing `select()` `fd_set` or timeout structures incorrectly.
- Treating `EINTR` as fatal in wait loops.
- Leaving terminal state in raw/no-echo mode after normal exit, error, `SIGINT`, or suspend/resume.
- Using canonical mode for binary serial protocols.
- Misconfiguring baud rate, parity, stop bits, hardware/software flow control, `VMIN`, or `VTIME`.
- Assuming `/dev/ttyUSB0` is stable across hotplug.
- Forgetting serial permissions, `dialout` group membership, udev rules, or another process holding the device open.
- Using a pipe where a PTY is required for an interactive program.
- Forgetting PTY window-size propagation and `SIGWINCH`.
- Treating Linux PTY master `read()` returning `-1/EIO` after slave close as data corruption instead of lifecycle.

---

## Detailed Answers - Priority A

### 1. An event-driven TCP service works under low traffic but spins at 100% CPU in production. How do you debug and fix it?

**What the interviewer is testing**

Whether you understand readiness as a lifecycle signal, can identify busy-loop causes, and can debug event-loop behavior in production.

**Strong answer**

I would first find which FD or event type is waking the loop repeatedly. Common causes are always-enabled `EPOLLOUT`, ignored HUP/ERR events, a regular file or invalid FD in a readiness loop, a timer with zero timeout, or code that retries immediately after `EAGAIN`. The fix is to make readiness interest state-driven: register read interest normally, enable write interest only when output is queued, remove or close dead FDs, and apply backoff or correct timeout logic for timers.

**Mechanism**

Readiness APIs wake when an operation should not block. Sockets are often writable, so `EPOLLOUT` can be almost permanently true. HUP and ERR are returned even if not explicitly requested. Regular files are usually always ready for `select()`/`poll()`, and Linux `epoll_ctl()` rejects many regular files with `EPERM`.

**Pitfalls**

Spinning can hide real work, drain battery on embedded systems, starve lower-priority tasks, and make latency worse. Ignoring `read() == 0`, `EPOLLHUP`, or `EPOLLERR` leaks dead connections in the interest list.

**Debug angle**

Use `top` or `perf top` to confirm CPU spin, then `strace -tt -e trace=select,pselect6,poll,ppoll,epoll_wait,epoll_ctl,read,write,close -p <pid>`. Inspect `/proc/<pid>/fd`, `/proc/<pid>/fdinfo/<fd>`, `lsof -p <pid>`, `ss -tanp`, and event-loop counters for wakeups by event type.

**Follow-up keywords**

`EPOLLOUT`, `EPOLLERR`, `EPOLLHUP`, `EPOLLRDHUP`, `POLLNVAL`, regular-file readiness, timeout `0`, `EAGAIN`, `/proc/<pid>/fdinfo`.

### 2. An epoll-based server randomly stops receiving data after switching from level-triggered to edge-triggered mode. What went wrong?

**What the interviewer is testing**

Whether you understand edge-triggered semantics and why it changes the required I/O loop.

**Strong answer**

The usual bug is handling an `EPOLLET` event with a single `read()`, `recv()`, or `accept()` and then returning to `epoll_wait()`. With edge-triggered epoll, the kernel may not notify again just because unread data remains. Each watched FD must be nonblocking, and the handler must drain reads, writes, or accepts until the operation fails with `EAGAIN` or `EWOULDBLOCK`.

**Mechanism**

Level-triggered mode reports readiness while the condition remains true. Edge-triggered mode reports a change since the last wait. If data arrives, one event is delivered. If the application reads only part of the buffer, the remaining bytes may not generate another event until new data arrives.

**Pitfalls**

Blocking FDs can hang the entire event loop during a drain. Unlimited drain loops can starve other FDs. Correct designs use nonblocking FDs, per-connection buffers, bounded work, and a user-space ready queue if a connection is continuously active.

**Debug angle**

Use `strace` to check whether reads stop before `EAGAIN`. Inspect `fcntl(F_GETFL)` behavior in code or logs to verify `O_NONBLOCK`. Add counters for bytes drained per event, `EAGAIN` stops, and per-FD service time.

**Follow-up keywords**

`EPOLLET`, `O_NONBLOCK`, `EAGAIN`, `EWOULDBLOCK`, level-triggered, edge-triggered, starvation, accept drain loop.

### 3. A service using `select()` breaks after enough clients or unrelated files are opened. How do you explain the failure and migrate it safely?

**What the interviewer is testing**

Whether you know `select()` limitations and can migrate without changing application semantics accidentally.

**Strong answer**

`select()` is fragile when fd numbers grow. User-space `fd_set` is limited by `FD_SETSIZE`, commonly 1024 in glibc, and using `FD_SET()` on a larger fd is undefined. `select()` also modifies the sets in place, so the loop must rebuild them every time, and `nfds` must be highest watched fd plus one. I would move small portable code to `poll()` and high-FD Linux services to `epoll`, while preserving EOF, HUP/ERR, timeout, and partial I/O handling.

**Mechanism**

`select()` takes bit sets for read, write, and exceptional conditions. The kernel overwrites those sets with the ready result. `poll()` uses an array of `struct pollfd`, so it avoids `FD_SETSIZE` and separates `events` from `revents`. `epoll` keeps an in-kernel interest list and returns ready events.

**Pitfalls**

Increasing `ulimit -n` can make fd numbers exceed `FD_SETSIZE` sooner. Passing `nfds` too low silently misses high FDs. Treating `exceptfds` as generic error handling is wrong; use normal read/write plus HUP/ERR handling.

**Debug angle**

Check `ulimit -n`, `/proc/<pid>/limits`, `/proc/<pid>/fd`, and logs around the highest fd. Use `strace` to see `select()` returning `EBADF`, `EINVAL`, timeout, or repeatedly missing expected descriptors.

**Follow-up keywords**

`FD_SETSIZE`, `fd_set`, `FD_ZERO`, `FD_SET`, `FD_ISSET`, `nfds`, `pollfd`, `POLLNVAL`, `epoll_ctl`.

### 4. You are designing an embedded gateway with many idle TCP clients, one UART, timers, and a shutdown signal. Would you use thread-per-client or an event loop?

**What the interviewer is testing**

Whether you can choose a concurrency model based on workload, resource limits, and failure isolation.

**Strong answer**

For many mostly idle connections, I would usually choose a single event loop or a small number of event-loop threads. The loop can monitor the listening socket, client sockets, UART FD, `timerfd`, `eventfd`, and possibly `signalfd`. Thread-per-client is simpler for low connection counts or blocking third-party libraries, but it scales poorly in memory, scheduling overhead, and synchronization complexity.

**Mechanism**

The event loop waits for readiness, then performs bounded nonblocking I/O. Protocol parsing remains separate: readiness only means bytes can be read or written, not that a complete frame or request exists. Each connection needs input and output buffers, framing state, and backpressure.

**Pitfalls**

An accepted socket inherits blocking behavior unless you set `O_NONBLOCK` or use `accept4()` with `SOCK_NONBLOCK`. Slow clients require output queues and write-interest toggling. UART data also arrives as a stream, so frame boundaries and checksums must be handled above `read()`.

**Debug angle**

Track per-FD state, queue length, bytes in/out, parse errors, and time since last progress. Use `ss`, `strace`, `/proc/<pid>/fd`, `/proc/<pid>/fdinfo`, `lsof`, and logs for queue growth and stuck FDs.

**Follow-up keywords**

Thread-per-client, event loop, `epoll`, `accept4()`, `SOCK_NONBLOCK`, `timerfd`, `eventfd`, `signalfd`, backpressure, framing.

### 5. A serial device on `/dev/ttyUSB0` sometimes blocks forever and sometimes returns partial packets. How do you debug the serial configuration and read strategy?

**What the interviewer is testing**

Whether you understand that serial devices use terminal semantics and stream framing, not message semantics.

**Strong answer**

I would verify device identity, permissions, whether another process owns the port, then inspect `termios`. Serial reads return bytes from a stream, not full packets. The application must set raw mode, correct baud/parity/stop bits/flow control, choose `VMIN`/`VTIME` or nonblocking/event-loop behavior intentionally, and implement protocol framing and checksums in user space.

**Mechanism**

Serial devices such as `/dev/ttyS*`, `/dev/ttyUSB*`, and `/dev/ttyACM*` are terminal-like character devices. In noncanonical mode, `VMIN` and `VTIME` control when `read()` returns. With `O_NONBLOCK`, `read()` can return `-1/EAGAIN` regardless of `VMIN`/`VTIME` expectations when no data is available.

**Pitfalls**

Canonical mode waits for newline and can transform bytes. `IXON`/`IXOFF` can interpret `Ctrl-S`/`Ctrl-Q` as flow control. Wrong baud, parity, or stop bits cause garbage. `/dev/ttyUSB0` can change after hotplug. Permissions may depend on `dialout`, udev rules, or device ownership.

**Debug angle**

Use `dmesg -w`, `lsusb`, `udevadm info`, `udevadm monitor`, `stty -F /dev/ttyUSB0 -a`, `lsof /dev/ttyUSB0`, `fuser /dev/ttyUSB0`, and `strace -e trace=openat,ioctl,read,write,poll,select`. Use `picocom`, `minicom`, `screen`, `socat`, or a logic analyzer when needed.

**Follow-up keywords**

`/dev/ttyS*`, `/dev/ttyUSB*`, `/dev/ttyACM*`, raw mode, `VMIN`, `VTIME`, `O_NONBLOCK`, baud, parity, stop bits, `CRTSCTS`, `IXON`, udev.

### 6. An embedded app reads binary UART frames, but bytes are echoed, line endings change, and `Ctrl-S` appears to freeze output. What terminal mode mistake is likely?

**What the interviewer is testing**

Whether you can connect symptoms to terminal driver flags.

**Strong answer**

The program likely left the serial FD in a cooked or partially cooked terminal mode. For binary UART protocols, I would start from the existing `termios`, clear canonical processing and echo, disable signal and extended processing if exact bytes are required, disable CR/NL translation and output postprocessing, and set flow control according to the device spec.

**Mechanism**

`ICANON` makes input line-oriented. `ECHO` copies input to output. `ISIG` turns special characters into signals. `IEXTEN` enables implementation-defined line editing features. `ICRNL`, `INLCR`, `IGNCR`, and `OPOST` can transform line endings. `IXON` and `IXOFF` enable software flow control.

**Pitfalls**

Raw mode is not one flag. Building a zeroed `struct termios` destroys unrelated settings. Clearing `ISIG` on an interactive terminal disables expected `Ctrl-C` behavior, so use cbreak rather than raw for many user-facing TUIs.

**Debug angle**

Compare `stty -F <device> -a` against the protocol requirements. Use `strace -e ioctl` to verify `tcgetattr()` and `tcsetattr()` calls. Capture raw bytes with a known-good serial tool or logic analyzer to separate software translation from electrical/protocol problems.

**Follow-up keywords**

`ICANON`, `ECHO`, `ISIG`, `IEXTEN`, `ICRNL`, `INLCR`, `IGNCR`, `OPOST`, `IXON`, `IXOFF`, raw mode, cbreak.

### 7. A CLI/TUI/password tool crashes and leaves the user's terminal with no echo. What should the program have done?

**What the interviewer is testing**

Whether you treat terminal attributes as persistent device state and design cleanup paths.

**Strong answer**

The program should save the original `termios`, modify a copy, apply the new mode, and restore the saved struct on normal exit, error paths, and catchable signals. For suspend/resume, restore before stopping and reapply after `SIGCONT`. A password prompt should restore immediately after reading before printing follow-up output.

**Mechanism**

Terminal attributes belong to the terminal device, not only the process. If a process disables `ECHO` or enters raw mode and exits without restoring, the shell continues using the modified settings. `tcsetattr()` can apply immediately, after output drains, or after drain plus input flush.

**Pitfalls**

`SIGKILL` cannot be caught, so no program can guarantee cleanup after it. Still, handling `SIGINT`, `SIGTERM`, `SIGQUIT`, `SIGHUP`, and job-control suspend/resume covers common real incidents. A zeroed `termios` restore is not a restore.

**Debug angle**

Recover with `stty sane`; if Enter is broken, use `Ctrl-J stty sane Ctrl-J`. Inspect with `stty -a`, `tty`, and `strace -e trace=ioctl,read,write`. In tests, run the tool under a PTY and assert terminal state is restored.

**Follow-up keywords**

`tcgetattr()`, `tcsetattr()`, `TCSANOW`, `TCSADRAIN`, `TCSAFLUSH`, `atexit()`, `SIGINT`, `SIGTERM`, `SIGTSTP`, `SIGCONT`, `stty sane`.

### 8. A command works interactively but fails when run through a pipe in automation. How do you decide whether it needs a terminal or a PTY?

**What the interviewer is testing**

Whether you can distinguish byte transport from terminal semantics.

**Strong answer**

I would check whether the program calls terminal-only APIs or depends on interactive behavior: `isatty()`, `/dev/tty`, `tcgetattr()`, `tcsetattr()`, job control, signal-generating characters, password prompts, cursor control, or window size. If yes, a pipe may not be enough; use a PTY for automation or test harnesses that must mimic an interactive terminal.

**Mechanism**

A pipe gives ordered bytes. A terminal adds line discipline, canonical/noncanonical mode, echo, special characters, foreground process groups, terminal-generated signals, and window-size ioctls. Many programs change behavior when stdout is not a terminal, including stdio buffering and color/progress output.

**Pitfalls**

Forcing a PTY can also change buffering and signal behavior, so it should be used deliberately. Do not use a PTY as a security boundary; it only provides terminal semantics.

**Debug angle**

Compare `strace -f -e trace=ioctl,openat,read,write` with pipe vs terminal runs. Check `isatty()` behavior, `TERM`, `/dev/tty` access, and `stty -a`. Use `script`, `socat`, `expect`, or a PTY-based test harness.

**Follow-up keywords**

`isatty()`, `/dev/tty`, `ttyname()`, pipe, socket, PTY, stdio line buffering, `script`, `expect`, `socat`.

### 9. You are building an SSH/container attach/remote shell feature. Why is a PTY required, and what setup order matters?

**What the interviewer is testing**

Whether you understand PTY master/slave setup, controlling terminals, and child process session behavior.

**Strong answer**

Interactive shells and full-screen programs need a terminal, not just a socket. The server or runtime should allocate a PTY master, prepare the slave, fork, have the child call `setsid()`, open the slave so it becomes the controlling terminal, duplicate the slave onto stdin/stdout/stderr, and then `exec()` the shell or command. The parent keeps the master and relays between it and the network or outer terminal.

**Mechanism**

UNIX 98 PTY allocation is `posix_openpt(O_RDWR | O_NOCTTY)`, `grantpt()`, `unlockpt()`, `ptsname()`, then open the slave at the right point. The PTY slave behaves like a real terminal: `Ctrl-C` written through the master can become `SIGINT` for the foreground process group on the slave.

**Pitfalls**

Opening the slave before `setsid()` can prevent correct controlling-terminal behavior. Forgetting stderr redirection loses errors. Not setting close-on-exec on unrelated FDs leaks descriptors. Not reaping the child leaves zombies.

**Debug angle**

Use `ps -o pid,ppid,sid,pgid,tpgid,tty,stat,cmd`, `ls -l /proc/<pid>/fd`, `ls -l /dev/pts`, and `strace -f -e trace=openat,ioctl,setsid,dup2,execve,read,write,close,wait4`.

**Follow-up keywords**

PTY master/slave, `/dev/ptmx`, `/dev/pts/N`, `posix_openpt()`, `grantpt()`, `unlockpt()`, `ptsname()`, `setsid()`, controlling terminal, `dup2()`, `SIGHUP`.

### 10. A PTY-based terminal emulator or relay does not resize correctly and sometimes logs `EIO` after the child exits. What should the lifecycle design be?

**What the interviewer is testing**

Whether you can design a robust PTY relay beyond the happy path.

**Strong answer**

The relay should copy initial `termios` and window size to the PTY slave, put the outer terminal in raw mode for transparent relays, propagate later window-size changes, relay both directions with partial I/O handling, treat EOF/HUP/`EIO` as lifecycle, restore the outer terminal, and reap the child.

**Mechanism**

Window resize is delivered as `SIGWINCH` to the foreground process group of a terminal. A relay reads the real terminal size with `TIOCGWINSZ` and sets the PTY size with `TIOCSWINSZ`; the kernel can then notify the child foreground process group. On Linux, reading the PTY master after all slave FDs close commonly fails with `EIO`.

**Pitfalls**

Without resize propagation, `vi`, shells, and TUIs draw with stale dimensions. Without raw outer terminal mode, input can be processed twice. Without backpressure handling, a slow network can block the PTY relay. Without terminal restore, the user's shell is left broken.

**Debug angle**

Use `stty -a`, `tty`, `ps` session fields, `/proc/<pid>/fd`, `strace -f -e ioctl,read,write,poll,select,wait4`, and tools such as `script`, `scriptreplay`, `socat`, `tmux`, or `screen` to reproduce terminal behavior.

**Follow-up keywords**

`SIGWINCH`, `TIOCGWINSZ`, `TIOCSWINSZ`, `SIGHUP`, Linux PTY `EIO`, raw outer terminal, relay loop, `waitpid()`.

---

## Short Answers - Priority B

### 11. Compare `select()`, `poll()`, and `epoll`.

`select()` is portable but limited by `FD_SETSIZE`, value-result `fd_set`, and `nfds` scanning. `poll()` is also portable and has a cleaner array model, but still scans all entries on every wait. `epoll` is Linux-specific, stores interest in the kernel, and is better for many mostly idle FDs.

### 12. Explain readiness-based I/O vs completion-based I/O.

Readiness APIs tell the program when an I/O call should not block. The program still calls `read()`, `write()`, `accept()`, or `recv()`. Completion APIs notify after an operation completes. POSIX AIO is a completion-style API, not a `select()` replacement.

### 13. Why does readiness not mean a complete protocol message is available?

TCP, PTYs, pipes, and serial devices are streams. Readiness only says at least some progress is possible, or EOF/error can be observed. Message framing, length fields, delimiters, checksums, and parse state belong in the application.

### 14. Why make accepted sockets nonblocking?

A readiness notification can become stale before the application calls I/O, and edge-triggered loops require nonblocking drain loops. Use `accept4(..., SOCK_NONBLOCK | SOCK_CLOEXEC)` where available, or call `fcntl()` after `accept()`.

### 15. How should an event loop handle slow clients?

Keep per-connection output buffers. Attempt bounded nonblocking writes. Enable `EPOLLOUT` only while data remains queued. Apply queue limits, timeouts, or disconnect policy so one slow client cannot consume unbounded memory.

### 16. What race do `pselect()`, `ppoll()`, and `epoll_pwait()` solve?

They avoid the race where a signal arrives after the program checks a flag but before it enters a blocking wait. The wait operation can atomically use a signal mask. Self-pipe and `signalfd` are alternative event-loop patterns.

### 17. When are `signalfd`, `timerfd`, and `eventfd` useful?

They turn signals, timers, and application notifications into readable FDs. That lets an event loop handle shutdown, periodic work, and cross-thread wakeups through the same readiness path as sockets, UARTs, and PTYs.

### 18. What do serial line settings mean in practice?

Baud controls bit rate. Parity and stop bits must match the device. Flow control can be software (`IXON`/`IXOFF`) or hardware (`CRTSCTS`). `tcdrain()` waits for queued output to transmit; `tcflush()` discards queued input/output.

### 19. How do window-size changes propagate through a PTY?

The relay receives or observes `SIGWINCH`, reads the outer terminal size with `TIOCGWINSZ`, sets the PTY size with `TIOCSWINSZ`, and the kernel sends `SIGWINCH` to the foreground process group on the PTY slave if the size changed.

### 20. What are `openpty()` and `forkpty()`?

They are convenient nonstandard helpers, available on glibc/BSD systems, that wrap common PTY setup. A strong candidate still understands the underlying UNIX 98 flow: open master, grant, unlock, get slave name, `setsid()`, open slave, `dup2()`, `exec()`.

---

## Recognition Notes - Priority C

| Topic | Recognize this much |
|---|---|
| Signal-driven I/O | Uses `O_ASYNC` and signals such as `SIGIO`; edge-like and tricky. Modern Linux services usually prefer `epoll` or an event library. |
| POSIX AIO | Completion-based I/O. Do not confuse it with readiness APIs. |
| `kqueue` and `/dev/poll` | Non-Linux readiness/event mechanisms encountered in portable event libraries. |
| `libevent` / `libev` / similar libraries | Abstract over `select`, `poll`, `epoll`, and platform backends. |
| `EPOLLONESHOT` | Disables a watched FD after one event until rearmed with `EPOLL_CTL_MOD`; useful for worker handoff. |
| `EPOLLEXCLUSIVE` | Linux flag for reducing thundering-herd wakeups in some multi-waiter designs. |
| `EPOLLWAKEUP` | Power-management related; recognize for embedded autosleep designs, but do not lead with it. |
| Packet mode | PTY master can receive slave-side control notifications; `select()` exceptional condition or `poll()` `POLLPRI`. |
| BSD PTYs | Legacy precreated `/dev/ptyXY` and `/dev/ttyXY` pairs. New Linux code should use UNIX 98 PTYs. |
| Login accounting | Relevant for login services, not every PTY relay or test harness. |
| Full `termios` flag table | Know common flags first; look up rare flags when needed. |

---

## Extra Questions Worth Adding

1. How would you debug an `epoll_wait()` loop that sleeps forever even though clients claim they sent data?
2. What metrics would you add to an event loop before shipping it in an embedded gateway?
3. How would you design a bounded output queue and disconnect policy for slow clients?
4. How would you test UART protocol parsing when the kernel returns frames split across multiple reads?
5. How would you make serial device names stable across hotplug?
6. How would you recover a field device when another process keeps `/dev/ttyUSB0` busy?
7. How would you write an automated test for a CLI that requires a terminal?
8. What should a PTY relay do when the network side is slow but the child keeps writing?
9. How would you prove a child process has the intended controlling terminal?
10. How would you design shutdown so signals, timers, sockets, and PTY lifecycle events all go through one event loop?

---

## One-Minute Review

1. Readiness means "an I/O call should not block", not "useful data is guaranteed".
2. `select()` is portable but has `FD_SETSIZE`, `nfds`, value-result set, and timeout traps.
3. `poll()` avoids fixed bit sets but still scans the watched array.
4. `epoll` keeps interest in the kernel and works well for many mostly idle FDs.
5. Level-triggered is easier; edge-triggered requires nonblocking drain loops until `EAGAIN`.
6. Treat EOF, HUP, ERR, `EINTR`, partial reads, and partial writes as normal paths.
7. Do not keep write readiness enabled unless output is queued.
8. Event-loop protocols still need buffers, framing, and backpressure.
9. Terminals are devices with line discipline, echo, signals, flow control, and window size.
10. Canonical mode is line-based; noncanonical mode uses `VMIN` and `VTIME`.
11. Serial protocols usually need raw mode plus explicit baud, parity, stop bits, and flow-control settings.
12. Always save and restore `termios`; broken terminal state is a production bug.
13. A pipe or socket cannot replace a PTY for terminal-oriented programs.
14. PTY child setup order is `setsid()`, open slave, `dup2()` to stdio, then `exec()`.
15. Robust PTY relays handle resize, raw outer terminal mode, HUP/EOF/`EIO`, partial writes, terminal restore, and child reaping.
