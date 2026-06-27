# Chapter 9.2 - Terminals and `termios`

> Topics: terminal driver, canonical/noncanonical mode, `termios`, raw/cbreak mode, terminal special characters, serial line control, window size, `stty`.
> Main sources: TLPI Ch62, `LINUX_SYSTEM_LEARNING_MAP.md`.
> Production context: password prompts, terminal UI tools, serial/UART device control, PTY-based programs, remote shells, embedded command consoles, and debugging broken terminal state.

---

## Problem It Solves

A terminal is not just a byte stream. The kernel terminal driver can buffer input by line, echo typed characters, edit the line, translate carriage return/newline, interpret `Ctrl-C` as `SIGINT`, and stop/resume output with flow-control characters.

That default behavior is right for a shell, but wrong for many real programs:

- a password prompt must turn off echo;
- `vi`, `less`, and terminal UIs need key-by-key input;
- serial protocols need exact byte framing, speed, parity, and queue control;
- PTY tools need to understand what the slave side will do to input/output.

`termios` is the POSIX API for reading and changing terminal-driver behavior safely.

## Learning Roadmap

| Level | Learn | Goal |
|---|---|---|
| Must know | `tcgetattr()`, `tcsetattr()`, `ICANON`, `ECHO`, `ISIG`, `VMIN`, `VTIME`, save/restore pattern | Safely switch terminal modes without breaking the user's shell. |
| Work useful | Raw/cbreak profiles, `stty`, serial 8N1 setup, `tcdrain()`, `tcflush()`, `SIGWINCH` | Build reliable CLI/TUI/serial tools and debug production terminal issues. |
| Recognize | line discipline, legacy flags, job-control signal effects, `TIOCGWINSZ`, implementation caveats | Understand PTY/terminal emulator behavior and portability traps. |

## Core Vocabulary

| Term | Meaning | Example / note |
|---|---|---|
| Terminal / tty | Character device with terminal-driver semantics. | `/dev/tty`, `/dev/tty1`, `/dev/pts/3`. |
| Terminal driver | Kernel code that processes terminal input/output queues. | Echo, canonical line buffering, special chars. |
| Line discipline | Kernel processing layer for terminal behavior. | Linux terminal emulators normally use `N_TTY`. |
| `termios` | Structure storing terminal attributes. | Read with `tcgetattr()`, apply with `tcsetattr()`. |
| Canonical mode | Line-oriented input mode. | `read()` returns after newline/EOF delimiter. |
| Noncanonical mode | Byte-oriented input mode. | `read()` completion controlled by `VMIN`/`VTIME`. |
| Cooked mode | Historical "normal shell" profile. | Canonical, echo, signals, editing. |
| Cbreak mode | Character-at-a-time while still allowing signal chars. | Useful for pagers and simple TUIs. |
| Raw mode | Character-at-a-time with most processing disabled. | Useful for transparent relay or exact bytes. |
| `ICANON` | Local flag enabling canonical mode. | Clear it for key-by-key input. |
| `ECHO` | Local flag enabling input echo. | Clear it for passwords. |
| `ISIG` | Local flag enabling signal-generating chars. | `VINTR` -> `SIGINT`, `VSUSP` -> `SIGTSTP`. |
| `IEXTEN` | Enables implementation-defined/extended input processing. | Affects `WERASE`, `LNEXT`, `REPRINT` on Linux. |
| `ICRNL` | Input flag mapping carriage return to newline. | Often disabled for raw/cbreak. |
| `OPOST` | Output postprocessing flag. | Clear for raw output; otherwise NL may become CR-NL. |
| `VMIN` | Minimum bytes for noncanonical read. | `VMIN=1` waits for one byte. |
| `VTIME` | Timeout for noncanonical read, in tenths of a second. | `VTIME=2` means 0.2 seconds. |
| Special characters | `c_cc[]` entries interpreted by terminal driver. | `VINTR`, `VEOF`, `VERASE`, `VSTART`, `VSTOP`. |
| `TCSANOW` | Apply terminal changes immediately. | Good for restore. |
| `TCSADRAIN` | Apply after queued output drains. | Good for output-affecting changes. |
| `TCSAFLUSH` | Drain output, then discard pending input and apply. | Good before password/raw input. |
| `tcdrain()` | Wait until output queue is transmitted. | Serial protocol boundary. |
| `tcflush()` | Discard terminal input/output queue data. | Drop type-ahead or stale serial bytes. |
| `tcflow()` | Suspend/resume data flow. | START/STOP style flow control. |
| Line speed | Bit rate configured with `cfsetispeed()` / `cfsetospeed()`. | `B9600`, `B115200`. |
| Window size | Rows/columns tracked by terminal driver. | `ioctl(TIOCGWINSZ)`, `SIGWINCH`. |
| `isatty()` | Checks whether an FD is terminal-like. | Useful before terminal-only operations. |
| `stty` | Shell command to inspect/change terminal attributes. | `stty -a`, `stty sane`. |

## Concept Overview

The important mental model:

```text
keyboard / serial / PTY peer
    |
    v
terminal driver input queue
    |
    | canonical? echo? signals? CR/NL mapping? VMIN/VTIME?
    v
process read()

process write()
    |
    v
terminal driver output queue
    |
    | OPOST? flow control? drain/flush?
    v
terminal / serial / PTY peer
```

Your process is not always receiving the bytes the user typed. The terminal driver may edit, delay, echo, transform, or convert certain bytes into signals before your `read()` sees anything.

## System Context

Terminal programming connects several earlier chapters:

| Area | Link |
|---|---|
| File descriptors | Terminal devices are operated through normal FDs. |
| Signals | `VINTR`, `VQUIT`, `VSUSP`, and window resize integrate with `SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGWINCH`. |
| Process groups/job control | Terminal-generated signals go to the foreground process group. |
| Alternative I/O | Noncanonical input can be monitored with `select()`, `poll()`, or `epoll`. |
| PTY | The PTY slave behaves like a terminal and uses these same driver rules. |
| Embedded/serial | UART-style devices use `termios` for speed, parity, stop bits, and flow control. |

If a program crashes after changing terminal state, the user's shell may stop echoing, stop translating Enter, or appear frozen. Safe restore paths are production behavior, not polish.

## Architecture

### `struct termios`

| Field | Controls | Common examples |
|---|---|---|
| `c_iflag` | Input processing | `ICRNL`, `IXON`, `BRKINT`, `ISTRIP`. |
| `c_oflag` | Output processing | `OPOST`, `ONLCR`. |
| `c_cflag` | Hardware/line control | `CS8`, `PARENB`, `CSTOPB`, `CREAD`, `CRTSCTS`. |
| `c_lflag` | Local/user-facing terminal behavior | `ICANON`, `ECHO`, `ISIG`, `IEXTEN`. |
| `c_cc[]` | Special chars and noncanonical controls | `VINTR`, `VEOF`, `VMIN`, `VTIME`. |

### Canonical Mode

Canonical mode (`ICANON` set) is line-oriented:

- input is collected until a line delimiter;
- line editing works (`ERASE`, `KILL`, and often `WERASE`);
- `read()` returns at most one line, and may return part of a line if buffer is smaller;
- signal-generating characters can flush queues unless `NOFLSH` is set.

Use it for shell-like line input.

### Noncanonical Mode

Noncanonical mode (`ICANON` clear) is byte-oriented. `VMIN` and `VTIME` decide when `read()` returns:

| `VMIN` | `VTIME` | Behavior | Work use |
|---|---|---|---|
| `0` | `0` | Return immediately; `0` means no data. | Polling without `O_NONBLOCK`. |
| `>0` | `0` | Block until at least `min(requested, VMIN)` bytes. | Key-by-key input with `VMIN=1`. |
| `0` | `>0` | Wait up to timeout for first byte; `0` on timeout. | Serial response timeout. |
| `>0` | `>0` | Wait for first byte, then interbyte timeout. | Escape sequences, packet-ish serial input. |

Portability note: POSIX allows `VMIN`/`VTIME` indexes to overlap with `VEOF`/`VEOL` on some systems. Save the full original `termios` and restore it instead of manually undoing fields.

### Cooked, Cbreak, Raw

| Mode | Input unit | Signals? | Echo? | Typical use |
|---|---|---|---|---|
| Cooked | Line | Yes | Yes | Shell and ordinary prompts. |
| Cbreak | Byte | Usually yes | Usually no | Pagers/TUIs that still allow `Ctrl-C`/`Ctrl-Z`. |
| Raw | Byte | No | No | Full-screen apps, PTY relays, exact serial bytes. |

Modern POSIX does not provide one magic "raw bit"; raw/cbreak are profiles made by changing several flags.

## Execution Flow

### Safe mode switch flow

```text
isatty(fd) if needed
    |
    v
tcgetattr(fd, &saved)
    |
    v
work = saved; modify only needed fields
    |
    v
tcsetattr(fd, TCSAFLUSH, &work)
    |
    v
run terminal I/O
    |
    v
restore saved with tcsetattr(fd, TCSANOW, &saved)
```

### Password prompt flow

```text
save termios
    |
    v
clear ECHO
    |
    v
tcsetattr(TCSAFLUSH)
    |
    v
read password
    |
    v
restore immediately, then print newline
```

### Key-by-key TUI flow

```text
save termios
    |
    v
clear ICANON and usually ECHO
    |
    v
set VMIN=1, VTIME=0
    |
    v
read one byte at a time or combine with poll/select
    |
    v
restore on exit/signals
```

### Serial setup flow

```text
open("/dev/ttyS0" or "/dev/ttyUSB0", O_RDWR | O_NOCTTY)
    |
    v
tcgetattr()
    |
    v
set speed, framing, parity, flow control, VMIN/VTIME
    |
    v
tcsetattr(TCSAFLUSH)
    |
    v
tcdrain/tcflush around protocol boundaries
```

### Resize-aware flow

```text
install SIGWINCH handler
    |
    v
on resize: ioctl(TIOCGWINSZ)
    |
    v
redraw UI or propagate size to PTY with TIOCSWINSZ
```

### Suspend/resume-safe raw mode flow

```text
SIGTSTP received
    |
    v
restore user terminal mode
    |
    v
temporarily set default SIGTSTP and raise it
    |
    v
process stops
    |
    v
SIGCONT: re-read user mode, reapply program mode
```

## 9.2 API / Topic Sections

### 9.2.1 Getting and Setting Attributes

```text
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
```

Use:

- `tcgetattr()` before modifying anything;
- change a copy;
- `tcsetattr()` to apply;
- restore the saved original on all exits.

If `fd` is not a terminal, these calls fail with `ENOTTY`.

`tcsetattr()` caveat from TLPI: it can return success if at least some requested changes were applied. If exact settings matter, call `tcgetattr()` again and compare.

### 9.2.2 Terminal Special Characters

Common entries:

| Entry | Default | Meaning |
|---|---|---|
| `VINTR` | `Ctrl-C` | Generate `SIGINT` if `ISIG` is set. |
| `VQUIT` | `Ctrl-\` | Generate `SIGQUIT` if `ISIG` is set. |
| `VSUSP` | `Ctrl-Z` | Generate `SIGTSTP` if `ISIG` is set. |
| `VEOF` | `Ctrl-D` | EOF marker in canonical mode. |
| `VERASE` | often Backspace/Delete | Erase previous character in canonical mode. |
| `VKILL` | `Ctrl-U` | Erase current input line. |
| `VSTART` / `VSTOP` | `Ctrl-Q` / `Ctrl-S` | Software flow control when enabled. |

Disable a special char portably with the value from:

```text
fpathconf(fd, _PC_VDISABLE)
```

### 9.2.3 Raw and Cbreak Profiles

Cbreak-like profile:

- clear `ICANON`;
- usually clear `ECHO`;
- keep `ISIG`;
- set `VMIN=1`, `VTIME=0`;
- often clear `ICRNL`.

Raw-like profile:

- clear `ICANON`, `ISIG`, `IEXTEN`, `ECHO`;
- disable major input transforms such as `ICRNL`, `INLCR`, `IGNCR`, `IXON`, parity stripping/handling;
- clear `OPOST`;
- set `VMIN=1`, `VTIME=0`.

Use raw mode when you want bytes to pass through with minimal terminal-driver interpretation. Use cbreak when you want character input but still want job-control and interrupt characters to behave normally.

### 9.2.4 Serial Line Speed and Framing

Speed is set through helper functions:

```text
cfsetispeed(&t, B9600);
cfsetospeed(&t, B9600);
tcsetattr(fd, TCSAFLUSH, &t);
```

Common 8N1 baseline:

| Setting | Meaning |
|---|---|
| `CS8` | 8 data bits. |
| clear `PARENB` | no parity. |
| clear `CSTOPB` | 1 stop bit. |
| set `CREAD` | enable receiver. |

Linux note: `c_ispeed` and `c_ospeed` fields are nonstandard and unused; use `cfset*speed()`.

### 9.2.5 Line Control

| API | Purpose | Production example |
|---|---|---|
| `tcdrain(fd)` | Wait until output queue has transmitted. | Ensure command left UART before changing phase. |
| `tcflush(fd, TCIFLUSH)` | Discard unread input. | Drop stale bytes before password/protocol prompt. |
| `tcflush(fd, TCOFLUSH)` | Discard queued output. | Abort pending terminal output. |
| `tcflush(fd, TCIOFLUSH)` | Discard both queues. | Reset protocol state. |
| `tcflow(fd, TCOOFF/TCOON)` | Suspend/resume output. | Flow-control integration. |
| `tcsendbreak(fd, duration)` | Send BREAK condition. | Serial device reset/attention signal. |

### 9.2.6 Window Size and Terminal Identity

| API / command | Use |
|---|---|
| `ioctl(fd, TIOCGWINSZ, &ws)` | Read rows/columns. |
| `ioctl(fd, TIOCSWINSZ, &ws)` | Set driver's notion of size; sends `SIGWINCH` if changed. |
| `isatty(fd)` | Check whether FD is terminal-like. |
| `ttyname(fd)` / `tty` | Get terminal device name. |
| `stty -a` | Inspect current attributes. |
| `stty sane` | Restore a usable terminal profile. |

Window-size ioctls are widely available but not standardized by SUSv3.

## Work-Useful Patterns

| Pattern | Use it when | Notes |
|---|---|---|
| Save-copy-modify-restore | Any terminal mode change | Never build a fresh `struct termios` from zero. |
| Restore on signal paths | Raw/cbreak/password tools | Handle `SIGINT`, `SIGTERM`, and suspend/resume when relevant. |
| Use `TCSAFLUSH` for password/raw entry | Avoid stale type-ahead | It drains output and discards pending input. |
| Set `VMIN/VTIME` intentionally | Noncanonical mode | Defaults may not match your read semantics. |
| Use `stty -a` during debugging | Unknown terminal state | Fastest way to see `icanon`, `echo`, `min`, `time`, speed. |
| Use `O_NOCTTY` for serial opens | Daemons/embedded programs | Avoid accidentally acquiring a controlling terminal. |
| Drain before serial phase change | Request/response protocols | `tcdrain()` ensures queued output left the host. |
| Propagate `SIGWINCH` to PTY | Terminal relays | Keep full-screen apps inside child PTY correctly sized. |

## Advanced / Recognize First

| Topic | Know this much |
|---|---|
| Line discipline | Kernel plugin-like layer for terminal processing. Most app code leaves it alone. |
| Legacy delay/case flags | Exist for old terminals; rarely useful in modern production. |
| `NOFLSH` | Prevents queue flush on signal-generating chars; useful in specialized terminal programs. |
| `IUTF8` | Linux input handling aid for UTF-8 erase behavior; not central for first-pass system programming. |
| `TOSTOP` | Can stop background process groups that write to terminal; job-control detail. |
| Background terminal operations | Some terminal ops from background process groups can trigger `SIGTTOU` or fail with `EIO` if orphaned. |
| Pixel fields in `struct winsize` | Present but Linux generally does not use them for terminal layout. |

## Example

### Example 1: Disable echo and restore it

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_termios;
static int saved_valid = 0;

static void restore_terminal(void) {
    if (saved_valid) {
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
    }
}

int main(void) {
    struct termios t;
    char password[128];

    if (tcgetattr(STDIN_FILENO, &saved_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }
    saved_valid = 1;
    if (atexit(restore_terminal) != 0) {
        return 1;
    }

    t = saved_termios;
    t.c_lflag &= (tcflag_t) ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
        perror("tcsetattr");
        return 1;
    }

    fputs("Password: ", stdout);
    fflush(stdout);
    if (fgets(password, sizeof(password), stdin) == NULL) {
        password[0] = '\0';
    }

    restore_terminal();
    putchar('\n');
    printf("Length: %zu\n", strcspn(password, "\n"));
    return 0;
}
```

What it teaches:

- Always save the original attributes.
- Restore before printing follow-up output.
- `TCSAFLUSH` prevents stale typed input from leaking into the password read.

### Example 2: Noncanonical read with interbyte timeout

```c
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(void) {
    struct termios saved;
    struct termios t;
    unsigned char buf[32];

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return 1;
    }

    t = saved;
    t.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    t.c_cc[VMIN] = 3;
    t.c_cc[VTIME] = 2;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
        perror("tcsetattr");
        return 1;
    }

    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n == -1) {
        perror("read");
    } else {
        printf("read returned %zd byte(s)\n", n);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) {
        perror("restore");
        return 1;
    }
    return n == -1 ? 1 : 0;
}
```

What it teaches:

- `VMIN > 0` and `VTIME > 0` means "wait for first byte, then use interbyte timeout."
- `VMIN/VTIME` only matter after `ICANON` is cleared.
- Restoring the saved struct avoids portability traps.

### Example 3: Configure a serial FD as 9600 8N1

```c
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static int configure_9600_8n1(int fd) {
    struct termios t;

    if (tcgetattr(fd, &t) == -1) {
        return -1;
    }
    if (cfsetispeed(&t, B9600) == -1 || cfsetospeed(&t, B9600) == -1) {
        return -1;
    }

    t.c_cflag |= CREAD;
    t.c_cflag &= (tcflag_t) ~PARENB;
    t.c_cflag &= (tcflag_t) ~CSTOPB;
    t.c_cflag &= (tcflag_t) ~CSIZE;
    t.c_cflag |= CS8;

    t.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ISIG);
    t.c_iflag &= (tcflag_t) ~(IXON | IXOFF | ICRNL);
    t.c_oflag &= (tcflag_t) ~OPOST;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    return tcsetattr(fd, TCSAFLUSH, &t);
}

int main(int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : "/dev/ttyS0";
    int fd = open(path, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    if (configure_9600_8n1(fd) == -1) {
        perror("configure");
        close(fd);
        return 1;
    }
    if (tcdrain(fd) == -1) {
        perror("tcdrain");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
```

What it teaches:

- Open serial devices with `O_NOCTTY`.
- Speed changes happen on the `termios` struct, then apply with `tcsetattr()`.
- Serial protocols usually need explicit `VMIN/VTIME`, framing, and flow-control decisions.

## Debugging

Useful commands:

```bash
stty -a
stty sane
tty
ls -l /proc/<pid>/fd
strace -e trace=ioctl,read,write,tcgetattr,tcsetattr -p <pid>
```

Recover a broken terminal:

```text
Ctrl-J
stty sane
Ctrl-J
```

Use `Ctrl-J` because Enter may no longer be mapped to newline if the terminal was left in a strange mode.

Common bugs:

| Bug | Symptom | Fix / check |
|---|---|---|
| Did not restore saved `termios` | Shell has no echo or broken Enter key. | Use `atexit` plus signal cleanup for interactive tools. |
| Cleared `ICANON` but forgot `VMIN/VTIME` | `read()` returns at surprising times. | Set both fields intentionally. |
| Disabled `ISIG` accidentally | `Ctrl-C` no longer interrupts. | Use cbreak, not raw, if signal chars should work. |
| Used `TCSANOW` before password read | Buffered type-ahead may be consumed unexpectedly. | Prefer `TCSAFLUSH` for sensitive mode switch. |
| Ignored `SIGTSTP`/`SIGCONT` path | Suspended raw-mode program leaves shell broken. | Restore before stop, reapply after continue. |
| Assumed serial speed fields are portable | Speed change does not work. | Use `cfsetispeed()` / `cfsetospeed()`. |
| Forgot `O_NOCTTY` opening serial device | Daemon gets unexpected controlling terminal behavior. | Use `open(path, O_RDWR | O_NOCTTY)`. |
| Did not handle resize | TUI draws wrong after window resize. | Handle `SIGWINCH` and query `TIOCGWINSZ`. |

## Real-world Usage

| Scenario | Practical design |
|---|---|
| Password prompt | Clear `ECHO`, use `TCSAFLUSH`, restore immediately. |
| Pager/editor/TUI | Cbreak/raw mode, `VMIN=1`, handle restore and resize. |
| Serial sensor gateway | `O_NOCTTY`, speed/framing setup, `tcdrain()` and `tcflush()` around protocol boundaries. |
| PTY relay | Put real terminal in raw mode so the PTY slave performs the intended terminal processing once. |
| Remote shell | Propagate `termios` and `winsize` to PTY slave. |
| Debugging broken CLI | `stty -a`, `stty sane`, inspect `/proc/<pid>/fd`. |

## Interview-Relevant Questions

1. Why is a terminal not just a normal byte stream?
2. What is canonical mode, and when does `read()` return in that mode?
3. What changes when `ICANON` is cleared?
4. Explain all four `VMIN`/`VTIME` combinations.
5. Why must terminal programs save and restore the original `termios`?
6. What is the difference between cbreak and raw mode?
7. Which flags would you clear for password input?
8. Why might a raw-mode program break the user's shell after a crash?
9. How do `VINTR`, `VQUIT`, and `VSUSP` interact with signals?
10. What does `ISIG` control?
11. What is `TCSAFLUSH`, and why is it useful before sensitive input?
12. Why should serial programs use `O_NOCTTY`?
13. How do you configure terminal line speed portably?
14. What are `tcdrain()`, `tcflush()`, and `tcflow()` for?
15. How do terminal window-size changes reach applications?
16. Why does a PTY slave use the same terminal-driver rules?
17. How would you debug a terminal stuck with echo disabled?
18. What happens if a background process performs terminal operations?
19. Why do noncanonical reads pair naturally with `poll()` or `select()`?
20. What portability caveat exists around `VMIN`/`VTIME` and `VEOF`/`VEOL`?

## Key Takeaways

1. Terminal behavior is controlled by the kernel terminal driver, not only by your program.
2. `termios` is the main API for inspecting and changing that behavior.
3. Canonical mode is line-based; noncanonical mode is byte-based.
4. `VMIN` and `VTIME` define noncanonical `read()` completion.
5. Cbreak keeps signal characters useful; raw disables most processing.
6. Safe terminal programs restore saved settings on normal and abnormal exits.
7. `stty -a` is the fastest way to inspect terminal state.
8. `stty sane` is the practical recovery command after broken terminal modes.
9. Serial code needs explicit speed, framing, flow-control, and queue decisions.
10. `SIGWINCH` plus `TIOCGWINSZ` is the resize-aware terminal pattern.
11. PTY-based programs rely on the same terminal semantics on the slave side.
12. Treat terminal state restoration as production reliability, not cleanup decoration.
