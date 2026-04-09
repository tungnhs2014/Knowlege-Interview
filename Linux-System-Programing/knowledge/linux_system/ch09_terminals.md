# Terminals and `termios` — Canonical/Noncanonical Input, Raw/Cbreak Modes, and Serial Line Control

## Problem It Solves

A terminal is not a passive byte stream. Between your process and the device, the terminal driver can:

- buffer input by line;
- apply line editing (`ERASE`, `KILL`, `WERASE`);
- interpret special characters (`VINTR`, `VEOF`, `VSUSP`, etc.);
- generate signals for the foreground process group (`SIGINT`, `SIGQUIT`, `SIGTSTP`);
- echo input;
- transform CR/NL on input and output;
- control data flow.

Default behavior is useful for shells, but often wrong for:

- password input;
- key-by-key UI programs (`vi`, `less`, TUIs);
- terminal emulators and PTY-based tools;
- serial communication with external devices.

`termios` is the API used to take control of this behavior safely.

## Concept Overview

The core APIs are:

```c
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
```

`fd` must refer to a terminal (`ENOTTY` otherwise).

The `optional_actions` argument in `tcsetattr()` decides when changes apply:

| Action | Meaning |
|---|---|
| `TCSANOW` | Apply immediately |
| `TCSADRAIN` | Apply after queued output is transmitted |
| `TCSAFLUSH` | Like `TCSADRAIN`, and discard pending input |

Recommended pattern:

1. `tcgetattr()`
2. modify only needed bits/fields
3. `tcsetattr()`
4. restore saved state on exit/signals

`struct termios` groups settings into:

- `c_iflag` (input flags)
- `c_oflag` (output flags)
- `c_cflag` (line control / hardware-related flags)
- `c_lflag` (local modes: canonical, echo, signals, etc.)
- `c_cc[]` (special chars + noncanonical controls `VMIN`/`VTIME`)

Important Linux note: `c_ispeed` and `c_ospeed` fields are nonstandard and unused; speed is managed via `cfsetispeed()` and `cfsetospeed()` with `tcsetattr()`.

## System Context

Terminal handling sits between user-space and kernel terminal code.

Data path:

1. terminal/PTY/serial device -> terminal driver input queue -> process `read()`
2. process `write()` -> terminal driver output queue -> terminal/PTY/serial device

The same terminal concepts apply to:

- virtual consoles (`/dev/ttyN`);
- terminal emulators (via pseudoterminals);
- serial lines and attached devices.

This chapter is especially relevant for:

- Chapter 9.2 itself (terminal control),
- Chapter 9.3 PTY internals (where the slave side uses terminal driver behavior),
- event-driven I/O (Chapter 9.1), where noncanonical character-at-a-time input is often needed.

## Architecture

### 1) Canonical mode (`ICANON` set)

Canonical mode is line-oriented:

- input is gathered into lines;
- `read()` returns when a complete line is available;
- line editing is active (`ERASE`, `KILL`, and with `IEXTEN`, `WERASE`);
- `REPRINT` and `LNEXT` work with `IEXTEN`;
- line delimiters include `NL`, `EOL`, `EOL2`, and `EOF` cases.

### 2) Noncanonical mode (`ICANON` clear)

Noncanonical mode is byte-oriented. `read()` completion is controlled by `VMIN` and `VTIME`:

| `VMIN` | `VTIME` | Behavior |
|---|---|---|
| `0` | `0` | Polling read: returns immediately, `0` if no data |
| `>0` | `0` | Blocking read until `min(requested, VMIN)` bytes |
| `0` | `>0` | Timeout read: return after first byte or timeout (`0`) |
| `>0` | `>0` | Interbyte timeout: timer starts after first byte; return when enough bytes or interbyte gap exceeds timeout |

Notes:

- `VTIME` unit is tenths of a second.
- In all cases, if enough bytes are already queued, `read()` can return immediately.
- `VMIN/VTIME` have no effect in canonical mode.

Portability note:

- SUSv3 allows `VMIN` to share index with `VEOF` and `VTIME` with `VEOL` on some systems.
- Portable code should save original `termios`, switch to noncanonical mode, and restore the saved struct instead of trying to "manually undo" individual fields.

### 3) Cooked, Cbreak, Raw (historical profiles)

Historically:

- cooked: canonical with standard processing;
- cbreak: noncanonical, but signal-generating chars still interpreted;
- raw: noncanonical with broad input/output processing disabled.

Modern UNIX emulates these profiles by explicit `termios` flags.

From TLPI-style helpers:

- Cbreak-like: clear `ICANON` and `ECHO`, keep `ISIG`, set `VMIN=1`, `VTIME=0`, clear `ICRNL`.
- Raw-like: clear `ICANON`, `ISIG`, `IEXTEN`, `ECHO`; disable major input transforms and `OPOST`; set `VMIN=1`, `VTIME=0`.

### 4) Key flags you use most in practice

| Field | Flag | Practical meaning |
|---|---|---|
| `c_lflag` | `ICANON` | line mode on/off |
| `c_lflag` | `ECHO` | input echo on/off |
| `c_lflag` | `ISIG` | interpret INTR/QUIT/SUSP as signals |
| `c_lflag` | `IEXTEN` | extended editing chars (`WERASE`, `REPRINT`, `LNEXT`, etc.) |
| `c_lflag` | `NOFLSH` | prevent queue flush on INTR/QUIT/SUSP |
| `c_iflag` | `ICRNL` | map CR to NL on input |
| `c_iflag` | `IXON` / `IXOFF` | software flow control |
| `c_oflag` | `OPOST` | enable output postprocessing |
| `c_oflag` | `ONLCR` | map NL to CR-NL on output |
| `c_cflag` | `CSIZE`, `PARENB`, `PARODD`, `CSTOPB` | serial framing/parity/stop bits |
| `c_cflag` | `CRTSCTS` | RTS/CTS hardware flow control |

Many legacy delay/case-conversion flags exist for old terminals; modern applications rarely need them.

### 5) Terminal special characters (`c_cc[]`)

Common values:

- `VINTR` (default `^C`) -> `SIGINT` when `ISIG` enabled
- `VQUIT` (default `^\`) -> `SIGQUIT` when `ISIG` enabled
- `VSUSP` (default `^Z`) -> `SIGTSTP` when `ISIG` enabled
- `VEOF` (default `^D`) in canonical mode
- `VERASE` (default `^?`), `VKILL` (default `^U`)
- `VSTART`/`VSTOP` (default `^Q/^S`) with software flow control

Characters can be disabled by setting to `fpathconf(fd, _PC_VDISABLE)`.

### 6) Line speed (bit rate)

Use:

```c
speed_t cfgetispeed(const struct termios *t);
speed_t cfgetospeed(const struct termios *t);
int cfsetispeed(struct termios *t, speed_t speed);
int cfsetospeed(struct termios *t, speed_t speed);
```

Then apply with `tcsetattr()`.

Line speed values are symbolic constants (`B9600`, `B38400`, etc.).

### 7) Line control functions

| Function | Purpose |
|---|---|
| `tcsendbreak(fd, duration)` | transmit BREAK condition |
| `tcdrain(fd)` | wait until output queue fully transmitted |
| `tcflush(fd, selector)` | discard input/output queues |
| `tcflow(fd, action)` | suspend/resume flow in either direction |

`tcflush` selectors:

- `TCIFLUSH` input queue
- `TCOFLUSH` output queue
- `TCIOFLUSH` both queues

`tcflow` actions:

- `TCOOFF`, `TCOON` (output suspend/resume)
- `TCIOFF`, `TCION` (send STOP/START chars)

## Execution Flow

### A) Safe mode-switch flow for interactive programs

1. Verify terminal FD if needed (`isatty(fd)`).
2. `tcgetattr(fd, &saved)`.
3. Copy `saved` to `work`.
4. Modify only required flags/`c_cc`.
5. `tcsetattr(fd, TCSAFLUSH or TCSADRAIN, &work)`.
6. Run I/O loop.
7. Restore `saved` on normal exit.
8. Restore `saved` in signal handlers for termination paths.

### B) Handling suspend/resume safely (`SIGTSTP`)

For raw/cbreak programs:

1. On `SIGTSTP`, save program mode and restore user mode first.
2. Re-raise default `SIGTSTP` so process really stops.
3. On `SIGCONT`, re-fetch user terminal state (it may have changed while stopped).
4. Reapply program mode and continue.

This prevents leaving the shell in broken terminal state.

### C) Serial setup flow

1. `open("/dev/ttyS*", O_RDWR | O_NOCTTY)`
2. `tcgetattr()`
3. set speed with `cfsetispeed/cfsetospeed`
4. adjust framing/flow-control flags
5. `tcsetattr(TCSAFLUSH, ...)`
6. use `tcdrain/tcflush/tcflow` around protocol boundaries

## Example

### Example 1: Disable echo for sensitive input and restore

```c
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct termios t, saved;
    char buf[128];

    if (tcgetattr(STDIN_FILENO, &t) == -1) return 1;
    saved = t;

    t.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) return 1;

    printf("Password: ");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) buf[0] = '\0';

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) return 1;
    printf("\nLength=%zu\n", strcspn(buf, "\n"));
    return 0;
}
```

### Example 2: Noncanonical input with interbyte timeout (`VMIN>0`, `VTIME>0`)

```c
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(void) {
    struct termios t, saved;
    unsigned char buf[32];
    ssize_t n;

    if (tcgetattr(STDIN_FILENO, &t) == -1) return 1;
    saved = t;

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 3;   // want up to 3 bytes
    t.c_cc[VTIME] = 2;  // 0.2s interbyte timeout

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) return 1;

    n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n == -1) {
        perror("read");
    } else {
        printf("read() returned %zd bytes\n", n);
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
    return 0;
}
```

### Example 3: Serial speed + queue control

```c
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(void) {
    int fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    if (fd == -1) return 1;

    struct termios t;
    if (tcgetattr(fd, &t) == -1) return 1;

    if (cfsetispeed(&t, B9600) == -1) return 1;
    if (cfsetospeed(&t, B9600) == -1) return 1;

    // minimal 8N1-style framing baseline
    t.c_cflag |= CREAD;
    t.c_cflag &= ~PARENB;
    t.c_cflag &= ~CSTOPB;
    t.c_cflag &= ~CSIZE;
    t.c_cflag |= CS8;

    if (tcsetattr(fd, TCSAFLUSH, &t) == -1) return 1;

    // before switching protocol phase, ensure output left host
    if (tcdrain(fd) == -1) return 1;

    // drop stale unread input if protocol requires a clean boundary
    if (tcflush(fd, TCIFLUSH) == -1) return 1;

    close(fd);
    return 0;
}
```

## Debugging

### 1) Inspect mode quickly

```bash
stty -a
```

Check:

- `icanon` / `-icanon`
- `echo` / `-echo`
- `isig`
- `min` and `time`
- speed and key mappings (`intr`, `quit`, `erase`, `eof`, etc.)

### 2) Recover broken terminal after crash

If echo/newline handling is broken:

1. press `Ctrl-J`
2. type `stty sane`
3. press `Ctrl-J`

### 3) Common pitfalls

- Not saving/restoring original `termios`.
- Changing to noncanonical mode without explicitly setting `VMIN`/`VTIME`.
- Choosing wrong `tcsetattr` action (`TCSANOW` when `TCSAFLUSH` was required).
- Assuming all flags behave identically across UNIX implementations.
- Forgetting signal paths (especially suspend/resume behavior) in raw/cbreak apps.

### 4) Behavior caveat from `tcsetattr()`

`tcsetattr()` can succeed even if only part of requested changes were applied.  
For strict verification, re-run `tcgetattr()` and compare expected fields.

## Real-world Usage

- Password prompts: disable echo temporarily.
- Terminal UI apps: use noncanonical mode for key-by-key control.
- Full-screen editors/pagers: cbreak/raw profiles with careful restore.
- Serial protocols: speed/framing setup, queue drain/flush between message phases.
- Terminal emulators and PTY-based systems: rely on the same terminal driver semantics on the slave side.
- Resize-aware applications: `SIGWINCH` + `ioctl(TIOCGWINSZ)`.

## Key Takeaways

1. Terminal behavior is policy driven by the kernel terminal driver; `termios` is the control API.
2. Canonical vs noncanonical mode is the primary design choice for input behavior.
3. `VMIN/VTIME` define noncanonical `read()` semantics and must be configured intentionally.
4. Raw/cbreak are explicit flag profiles, not magic one-bit modes in modern POSIX APIs.
5. Safe programs always save and restore terminal state, including on signal paths.
6. Serial communication needs both speed setup (`cfset*speed`) and queue/flow control (`tcdrain`, `tcflush`, `tcflow`, `tcsendbreak`).
7. `stty -a` is the fastest operational tool for understanding and fixing terminal state.
