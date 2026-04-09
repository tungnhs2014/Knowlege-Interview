# Pseudoterminals (PTY) — `posix_openpt()`, `ptyFork()`, and Terminal-Oriented Process Bridging

## Problem It Solves

A terminal-oriented program (for example `bash`, `vi`, `less`, `login`) expects terminal semantics, not just a byte stream:

- controlling terminal behavior (`/dev/tty`, job control signals);
- terminal driver processing (canonical/noncanonical mode, special characters, echo);
- terminal-specific ioctls and attributes.

Sockets and pipes can transport bytes, but they cannot directly provide full terminal behavior to such programs.  
Pseudoterminals (PTYs) solve this by providing a virtual terminal pair:

- **master** side for the driver/relay program;
- **slave** side that behaves like a real terminal for the terminal-oriented program.

This is the key building block behind remote login services (`ssh`, `telnet`, `rlogin`), terminal emulators, `script(1)`, `expect`, and `screen`.

## Concept Overview

A PTY is a connected pair of virtual devices:

- **PTY master**: controlled by a program that relays/drives I/O.
- **PTY slave**: appears as a terminal device to the child/session program.

Data flow:

- write to master -> appears as input on slave;
- write to slave -> can be read from master.

The critical distinction from a bidirectional pipe:

- the **slave** side is processed by terminal driver rules.

UNIX 98 (System V style) PTY APIs:

- `posix_openpt()` -> open unused master
- `grantpt()` -> adjust slave permissions/ownership (required for portability)
- `unlockpt()` -> unlock slave for opening
- `ptsname()` -> get slave pathname (`/dev/pts/N`)

## System Context

A typical PTY system has three actors:

1. **User-facing channel** (real terminal window, network socket, etc.)
2. **Driver/relay process** (forwards data between channels)
3. **Terminal-oriented process group** attached to PTY slave

In remote login architecture:

- local client and remote server communicate via socket;
- remote server process owns PTY master;
- login shell runs on PTY slave.

The shell and its child jobs treat the slave as their controlling terminal, enabling normal interactive behavior (foreground process group rules, terminal signals, line discipline behavior).

## Architecture

## 1) UNIX 98 PTY allocation pipeline

The standard allocation sequence:

1. `mfd = posix_openpt(O_RDWR | O_NOCTTY)`
2. `grantpt(mfd)`
3. `unlockpt(mfd)`
4. `slaveName = ptsname(mfd)`
5. `sfd = open(slaveName, O_RDWR)`

Linux notes from TLPI:

- `posix_openpt()` is effectively `open("/dev/ptmx", flags)`.
- Linux auto-configures slave ownership/permissions, but portable code still calls `grantpt()`.
- opening locked slave before `unlockpt()` fails with `EIO`.
- PTY count limits are kernel-controlled (`/proc/sys/kernel/pty/max`, `/proc/sys/kernel/pty/nr`).

## 2) `ptyMasterOpen()` abstraction

A helper function usually wraps the 4-step setup (`posix_openpt` + `grantpt` + `unlockpt` + `ptsname`) and returns:

- master FD
- slave pathname

This simplifies caller code and allows swapping implementations (UNIX 98 vs BSD PTY backends).

## 3) `ptyFork()` architecture

`ptyFork()` creates parent-child topology connected by PTY:

- parent keeps master FD;
- child gets slave as stdin/stdout/stderr and controlling terminal.

Child-side sequence:

1. `setsid()` (new session, no controlling terminal)
2. open slave
3. optional `TIOCSCTTY` on BSD-like systems
4. optional apply `termios`/`winsize`
5. `dup2(slaveFd, 0/1/2)`
6. `exec()` terminal-oriented target

This is the canonical foundation for `script`-style and login-service programs.

## 4) PTY I/O semantics and close behavior

General behavior:

- PTY has bounded capacity (Linux: roughly 4 KiB each direction).
- slave side interprets terminal special chars and canonical buffering just like a real terminal.

When **all master FDs are closed**:

- controlling process on slave gets `SIGHUP` (if any);
- `read(slave)` returns EOF (`0`);
- `write(slave)` fails with `EIO` (Linux).

When **all slave FDs are closed**:

- `read(master)` fails with `EIO` on Linux (other UNIX variants may return EOF);
- `write(master)` behavior varies by implementation and queue state.

## 5) Packet mode (`TIOCPKT`)

Packet mode lets master-side process detect slave-side state changes (flow-control and queue events).

- enable via `ioctl(mfd, TIOCPKT, &arg)`
- master reads may return control byte events
- readiness appears as:
  - exceptional condition in `select()` (`exceptfds`)
  - `POLLPRI` in `poll()`

Useful for network login programs handling software flow-control events.

## Execution Flow

### PTY session bootstrap flow

1. Relay process opens PTY master and obtains slave name.
2. Relay process `fork()`.
3. Child:
   - `setsid()`
   - open slave (becomes controlling terminal)
   - optional terminal/window setup
   - connect slave to stdio with `dup2()`
   - `exec()` shell or terminal-oriented program
4. Parent:
   - keeps master FD
   - relays data between master and external channel (terminal/socket/file)

### Relay loop flow (core runtime)

1. Monitor input sources (e.g., user stdin + master FD) using `select()`/`poll()`.
2. Forward user input to master.
3. Forward master output to user channel (and optionally log file).
4. Exit on EOF/error/peer closure conditions.

### `script(1)`-style flow

1. Capture current terminal `termios` and `winsize`.
2. `ptyFork()` child shell with same `termios`/`winsize` on slave.
3. Put real user terminal in raw mode for transparent forwarding.
4. Relay bytes both directions.
5. Copy slave output stream to transcript file.

## Example

### Example 1: Open UNIX 98 PTY master and corresponding slave

```c
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int mfd = posix_openpt(O_RDWR | O_NOCTTY);
    if (mfd == -1) return 1;

    if (grantpt(mfd) == -1) return 1;
    if (unlockpt(mfd) == -1) return 1;

    char *sname = ptsname(mfd);
    if (sname == NULL) return 1;

    printf("master fd=%d, slave=%s\n", mfd, sname);
    close(mfd);
    return 0;
}
```

### Example 2: Minimal child-side PTY attach pattern

```c
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int child_attach_and_exec(const char *slaveName, const char *prog) {
    if (setsid() == -1) return -1;

    int sfd = open(slaveName, O_RDWR);
    if (sfd == -1) return -1;

    if (dup2(sfd, STDIN_FILENO) != STDIN_FILENO) return -1;
    if (dup2(sfd, STDOUT_FILENO) != STDOUT_FILENO) return -1;
    if (dup2(sfd, STDERR_FILENO) != STDERR_FILENO) return -1;

    if (sfd > STDERR_FILENO) close(sfd);
    execlp(prog, prog, (char *) NULL);
    _exit(127);
}
```

### Example 3: Relay loop skeleton (`script`/remote-login core)

```c
#include <sys/select.h>
#include <unistd.h>

int relay_loop(int masterFd) {
    char buf[256];
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(masterFd, &rfds);

        int nfds = (masterFd > STDIN_FILENO ? masterFd : STDIN_FILENO) + 1;
        if (select(nfds, &rfds, NULL, NULL, NULL) == -1) return -1;

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) return 0;
            if (write(masterFd, buf, n) != n) return -1;
        }

        if (FD_ISSET(masterFd, &rfds)) {
            ssize_t n = read(masterFd, buf, sizeof(buf));
            if (n <= 0) return 0;
            if (write(STDOUT_FILENO, buf, n) != n) return -1;
        }
    }
}
```

## Debugging

### 1) Validate PTY resources and limits (Linux)

```bash
cat /proc/sys/kernel/pty/max
cat /proc/sys/kernel/pty/nr
```

### 2) Typical PTY setup errors

- `ENOTTY`: applying terminal ops to wrong FD type
- `EIO` opening locked slave before `unlockpt()`
- `EOVERFLOW`: caller buffer too small for slave name in wrapper functions
- child does not get controlling terminal if session setup/order is wrong

### 3) Runtime pitfalls

- Not handling `EIO`/EOF paths when one side closes
- Forgetting to relay window-size updates (`SIGWINCH` + `TIOCSWINSZ`) in `script`-style programs
- Double terminal processing when real terminal mode is not adjusted for transparent forwarding

### 4) Process/session inspection

Use `ps` with session and TTY columns to verify that child shell has its own session and PTY slave TTY.

## Real-world Usage

- **Remote login stacks**: `ssh` server side connects login shell to PTY slave.
- **Terminal emulators**: each terminal window typically uses a PTY pair.
- **`script(1)`**: records session by relaying between real terminal and PTY master, logging output stream.
- **Automation**: `expect` drives interactive tools through PTY.
- **Multiplexing**: `screen` uses PTYs to host and switch among multiple interactive sessions.
- **Buffering workaround utilities**: PTY can force line-oriented behavior from programs that would otherwise use block buffering when stdout is not a terminal.

## Key Takeaways

1. PTY is the missing bridge between terminal semantics and non-terminal transport channels.
2. The slave side behaves like a terminal; the master side behaves like a control/relay endpoint.
3. Standard UNIX 98 flow is `posix_openpt()` -> `grantpt()` -> `unlockpt()` -> `ptsname()`.
4. `ptyFork()` pattern (`setsid`, open slave, `dup2`, `exec`) is the canonical setup for terminal-oriented child processes.
5. PTY I/O is terminal-semantics-aware, not just raw byte forwarding.
6. Close/error behavior differs from ordinary pipes and must be handled explicitly.
7. PTYs are foundational for ssh-like remote sessions, terminal emulators, scripting/recording tools, and interactive automation.
