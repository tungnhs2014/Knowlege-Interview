# Chapter 4 — Timers & Time API
## Topics: 4.4 Timers & Sleeping · 4.5 Time API
> Sources: TLPI Ch23, Ch10

---

## 4.4 Timers & Sleeping

### What Problem Timers Solve

A process needs to:
- Schedule tasks to run after a fixed delay
- Measure elapsed time (profiling, benchmarks)
- Timeout blocking syscalls (read/write on slow devices)
- Monitor multiple timers simultaneously in an event loop

Linux provides three generations of timer API: **classical** (`setitimer`), **POSIX** (`timer_create`), and **file-descriptor-based** (`timerfd`).

---

### 1. Classical Timers — `setitimer()` / `alarm()`

The kernel maintains **3 per-process timers**, each tracking a different type of time:

| Timer type | Measures | Signal on expiry |
|---|---|---|
| `ITIMER_REAL` | Wall-clock time | `SIGALRM` |
| `ITIMER_VIRTUAL` | Process CPU time (user mode) | `SIGVTALRM` |
| `ITIMER_PROF` | CPU time user + kernel (profiling) | `SIGPROF` |

```c
#include <sys/time.h>

struct itimerval {
    struct timeval it_interval;  /* repeat interval (0 = one-shot) */
    struct timeval it_value;     /* initial expiry (0 = disarm) */
};

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
int getitimer(int which, struct itimerval *curr_value);
```

`alarm(n)` is a simplified interface for `ITIMER_REAL`: one-shot, second granularity.
`alarm()` and `setitimer(ITIMER_REAL, ...)` **share the same per-process timer** — do not mix them.

**Limitation:** Only one timer of each type per process. For multiple concurrent timers, use POSIX timers.

---

### 2. Timer Accuracy

Traditionally, the kernel uses a **jiffy** (typically 10ms at `CONFIG_HZ=100`, 4ms at `CONFIG_HZ=250`) as the minimum time unit. Timers are rounded up to the next jiffy boundary.

```
setitimer(ITIMER_REAL, 100ms) → actual expiry at 100–110ms (jiffy rounding)
```

From kernel **2.6.21** with `CONFIG_HIGH_RES_TIMERS`: timers operate at nanosecond precision on supporting hardware (HPET, TSC).

```bash
grep HIGH_RES_TIMERS /boot/config-$(uname -r)
# CONFIG_HIGH_RES_TIMERS=y
```

---

### 3. Timeout Pattern on Blocking Syscalls

Classic pattern: timeout a `read()` after N seconds if no data arrives.

```c
volatile sig_atomic_t timeout_flag = 0;

void alarm_handler(int sig) { timeout_flag = 1; }

/* Install handler WITHOUT SA_RESTART — we want EINTR to interrupt read() */
struct sigaction sa = { .sa_handler = alarm_handler };
sigemptyset(&sa.sa_mask);
/* no SA_RESTART */
sigaction(SIGALRM, &sa, NULL);

alarm(5);

ssize_t n = read(fd, buf, sizeof(buf));

alarm(0);  /* cancel timer if read completed early */

if (n == -1 && errno == EINTR && timeout_flag) {
    /* timed out */
}
```

**Race condition:** If `SIGALRM` fires before `read()` blocks, the signal is delivered and the handler runs, then `read()` blocks forever. Fix: use `sigprocmask()` to block `SIGALRM`, then atomically unblock with `pselect()` or `ppoll()`.

---

### 4. `sleep()` and `nanosleep()`

```c
unsigned int sleep(unsigned int seconds);
/* Returns remaining seconds if interrupted by a signal */

int nanosleep(const struct timespec *req, struct timespec *rem);
/* rem receives remaining time on EINTR */
```

`nanosleep()` advantages over `sleep()`:
- Nanosecond precision (rounded up to kernel resolution in practice)
- No interaction with `SIGALRM` / `setitimer`
- Returns remaining time via `rem` on `EINTR`

**Restart loop pattern:**

```c
struct timespec req = { .tv_sec = 2, .tv_nsec = 500000000 }; /* 2.5s */
struct timespec rem;

while (nanosleep(&req, &rem) == -1) {
    if (errno == EINTR) {
        req = rem; /* continue with remaining time */
    } else {
        break;
    }
}
```

**Problem with restart loop:** Each restart incurs rounding, accumulating error.
For example: requested 100ms, actual 102ms → after 10 restarts → 120ms instead of 1000ms.

**Fix — `clock_nanosleep()` with `TIMER_ABSTIME`:**

```c
struct timespec abs_target;
clock_gettime(CLOCK_MONOTONIC, &abs_target);
abs_target.tv_sec += 2;  /* fixed absolute target */

/* Restarts never accumulate error — always sleep to the same absolute target */
while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &abs_target, NULL) != 0) {
    if (errno != EINTR) break;
}
```

---

### 5. POSIX Clocks — `clock_gettime()`

Replaces `gettimeofday()` (microsecond), provides nanosecond precision:

```c
#include <time.h>

int clock_gettime(clockid_t clockid, struct timespec *tp);
int clock_getres(clockid_t clockid, struct timespec *res);    /* actual resolution */
int clock_settime(clockid_t clockid, const struct timespec *tp); /* requires CAP_SYS_TIME */
```

| Clock ID | Measures | Adjustable? |
|---|---|---|
| `CLOCK_REALTIME` | Wall-clock time (Epoch) | Yes — can jump on NTP adjustment |
| `CLOCK_MONOTONIC` | Time since boot | No — never decreases |
| `CLOCK_PROCESS_CPUTIME_ID` | CPU time consumed by process | No |
| `CLOCK_THREAD_CPUTIME_ID` | CPU time consumed by current thread | No |

**Rule:** Measuring elapsed time → always use `CLOCK_MONOTONIC` (immune to NTP, time adjustments). Logging a timestamp → `CLOCK_REALTIME`.

---

### 6. POSIX Interval Timers — `timer_create()`

Solves the limitations of `setitimer`: **multiple timers per process**, **nanosecond precision**, **flexible notification**.

```c
#include <signal.h>
#include <time.h>

/* Step 1: Create timer */
timer_t tid;
struct sigevent sev;

sev.sigev_notify          = SIGEV_SIGNAL;   /* notify via signal */
sev.sigev_signo           = SIGRTMIN;       /* use a realtime signal */
sev.sigev_value.sival_ptr = &tid;           /* data attached to signal */

timer_create(CLOCK_REALTIME, &sev, &tid);

/* Step 2: Arm timer */
struct itimerspec ts = {
    .it_value    = { .tv_sec = 2 },  /* first fire after 2s */
    .it_interval = { .tv_sec = 5 },  /* repeat every 5s */
};
timer_settime(tid, 0, &ts, NULL);
/* timer_settime(tid, TIMER_ABSTIME, &ts, NULL) for absolute time */

/* Step 3: Inspect or disarm */
struct itimerspec curr;
timer_gettime(tid, &curr);   /* remaining time */
timer_delete(tid);           /* cleanup */
```

**4 notification modes (`sigev_notify`):**

| Mode | Behavior |
|---|---|
| `SIGEV_SIGNAL` | Send signal to process |
| `SIGEV_THREAD` | Invoke function in a new thread on each expiry |
| `SIGEV_THREAD_ID` | Send signal to a specific thread (Linux-specific) |
| `SIGEV_NONE` | No notification — poll via `timer_gettime()` |

**Timer Overrun:** If the timer expires multiple times before the signal is delivered (process preempted, signal blocked), extra expirations are **NOT queued** — they are counted as overruns:

```c
/* Inside signal handler (async-signal-safe): */
int overrun = timer_getoverrun(tid);
/* Or: siginfo_t.si_overrun (Linux extension, avoids syscall overhead) */
```

If overrun count is 2, the timer expired 3 times (1 signaled + 2 overruns) since the last delivery.

---

### 7. `timerfd` — Timer as File Descriptor

Linux-specific (kernel 2.6.25+). Timer expiration notifications can be **read** from a file descriptor → integrates naturally with `epoll`/`select`/`poll` in an event loop.

```c
#include <sys/timerfd.h>

/* Create timer fd */
int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

/* Arm timer (analogous to timer_settime) */
struct itimerspec ts = {
    .it_value    = { .tv_sec = 1 },
    .it_interval = { .tv_sec = 1 },
};
timerfd_settime(fd, 0, &ts, NULL);
/* timerfd_settime(fd, TFD_TIMER_ABSTIME, &ts, NULL) for absolute time */

/* Read expiration count (blocks until at least one expiry with blocking fd) */
uint64_t num_exp;
ssize_t s = read(fd, &num_exp, sizeof(num_exp));
/* num_exp = number of expirations since last read() */

close(fd); /* disarm + cleanup */
```

**Pattern with epoll:**

```c
int epfd = epoll_create1(0);
struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

/* Event loop handles timer alongside network sockets, pipes, etc. */
while (1) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        if (events[i].data.fd == fd) {
            uint64_t num_exp;
            read(fd, &num_exp, sizeof(num_exp));
            handle_timer_expiry(num_exp);
        }
        /* handle other fds ... */
    }
}
```

**`fork()` / `exec()` behavior:**
- `fork()`: child inherits the fd, sharing the **same timer object** as parent — either process can read expirations.
- `exec()`: fd is preserved (unless `TFD_CLOEXEC` was set) — armed timer continues after exec.

---

### API Comparison

| | `setitimer` | `timer_create` | `timerfd` |
|---|---|---|---|
| Timers per process | 3 (fixed types) | Multiple | Multiple |
| Precision | Jiffy / high-res | Nanosecond | Nanosecond |
| Notification | Fixed signal | Signal / Thread / None | `read()` on fd |
| Event loop integration | Difficult | Difficult | Native (`epoll`) |
| Portability | POSIX | POSIX | Linux-only |
| Overrun detection | No | `timer_getoverrun()` | Count returned by `read()` |

---

### System Context — Timers in the Linux Architecture

```
Hardware timer chip (HPET / TSC / APIC local timer)
               │
               ▼
      Kernel clocksource layer
      (selects best available: TSC > HPET > ACPI PM > PIT)
               │
       ┌───────┴────────────────────┐
       │                            │
  tick subsystem              hrtimer subsystem
  (CONFIG_HZ jiffies)         (CONFIG_HIGH_RES_TIMERS)
  setitimer(), alarm()         nanosleep(), clock_nanosleep()
  sleep()                      timer_create(), timerfd_create()
```

**Subsystem interactions:**
- **Scheduler:** timer expiry wakes a process sleeping in `TASK_INTERRUPTIBLE` (e.g., `nanosleep()`); the scheduler then dispatches the process.
- **Signal subsystem:** `ITIMER_REAL` expiry → `SIGALRM`; `timer_create(SIGEV_SIGNAL)` → delivers the configured signal via the signal subsystem.
- **epoll/VFS:** `timerfd` expiry marks the fd as readable → `epoll_wait()` returns it alongside socket/pipe events — unified event loop.
- **NPTL thread library:** `SIGEV_THREAD` timer expiry → NPTL spawns a new thread internally to invoke the notification function.

**Failure scenarios:**
- `setitimer()` and `alarm()` used together → **one silently cancels the other** (they share the same `ITIMER_REAL` slot).
- `nanosleep()` restart loop without `TIMER_ABSTIME` → rounding error accumulates across restarts → total sleep shorter than requested.
- `timerfd` fd not `close()`d after use → **timer continues running in kernel, fd leaks**.
- `SIGEV_THREAD` timer: if thread creation fails (resource exhaustion) → expiration **silently missed**, no error returned.
- `timer_create()` with standard (non-realtime) signal: multiple expirations while signal pending → **overruns silently accumulated**; always call `timer_getoverrun()`.

---

### Debugging

```bash
# List all active kernel timers with expiry times and owners
cat /proc/timer_list | head -80

# Verify HIGH_RES_TIMERS is enabled on this kernel
cat /boot/config-$(uname -r) | grep HIGH_RES_TIMERS

# Check which clocksource the kernel selected
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
dmesg | grep -i "clocksource\|hpet\|tsc"
```

**Common timer bugs:**

```c
/* BUG: mixing alarm() and setitimer() — they share the same slot */
alarm(5);
setitimer(ITIMER_REAL, &tv, NULL);  /* CANCELS the alarm() silently */

/* BUG: nanosleep restart loop accumulates rounding error */
while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    req = rem;  /* each restart rounds up again — total > intended */

/* FIX: sleep to absolute target — restarts have zero accumulated error */
struct timespec target;
clock_gettime(CLOCK_MONOTONIC, &target);
target.tv_nsec += 100000000;  /* +100ms */
if (target.tv_nsec >= 1000000000) { target.tv_sec++; target.tv_nsec -= 1000000000; }
while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, NULL) != 0)
    if (errno != EINTR) break;

/* BUG: not closing timerfd — timer continues running in kernel */
int fd = timerfd_create(CLOCK_MONOTONIC, 0);
timerfd_settime(fd, 0, &ts, NULL);
/* ... forgot close(fd) */
```

---

### Real-world Usage

| Use case | API | Notes |
|---|---|---|
| Per-connection network timeout | `timerfd` + `epoll` | One `timerfd` per connection; unified with socket events |
| Periodic heartbeat / sensor poll | `timer_create(SIGEV_SIGNAL)` | Embedded systems; use realtime signal to avoid loss |
| CPU profiling | `ITIMER_PROF` → `SIGPROF` | Used internally by `gprof` |
| RPC / socket call timeout | `alarm()` + `EINTR` + `alarm(0)` | Cancel immediately after call returns |
| Jitter-free periodic RT task | `clock_nanosleep(TIMER_ABSTIME)` + `CLOCK_MONOTONIC` | No error accumulation in tight loops |
| Watchdog timer | `timerfd` in event loop | Reset on activity; fire → trigger recovery action |

---

### Key Takeaways — 4.4

1. `ITIMER_REAL/VIRTUAL/PROF` → 3 per-process timers only; `alarm()` and `setitimer(ITIMER_REAL)` share the same timer slot.
2. Timer accuracy is jiffy-bounded by default; `CONFIG_HIGH_RES_TIMERS` enables nanosecond precision.
3. Timeout pattern: `alarm()` + handler without `SA_RESTART` + check `EINTR`. Beware race between `alarm()` call and actual `read()` block.
4. `nanosleep()` restart loop accumulates rounding error → use `clock_nanosleep(TIMER_ABSTIME)` to sleep to an absolute target.
5. `CLOCK_MONOTONIC` for elapsed time; `CLOCK_REALTIME` for wall-clock; never use `CLOCK_REALTIME` to measure intervals.
6. `timer_create()` supports multiple timers per process with flexible notification (`SIGEV_SIGNAL`, `SIGEV_THREAD`, `SIGEV_THREAD_ID`, `SIGEV_NONE`).
7. Timer overruns: extra expirations are not queued; retrieve via `timer_getoverrun()` or `siginfo_t.si_overrun`.
8. `timerfd` exposes timer as readable fd → native `epoll` integration. Use `TFD_CLOEXEC` and `TFD_NONBLOCK` at creation.
9. `timerfd` + `fork()`: child inherits fd sharing the same underlying timer object with parent.

---

## 4.5 Time API

### What Problem the Time API Solves

A process needs to:
- Obtain the current wall-clock time for logging and timestamping
- Accurately measure elapsed CPU or real time
- Convert between numeric, broken-down, and string representations of time
- Handle timezone and locale when displaying time to users

---

### 1. Two Types of "Time" in Linux

| Type | Meaning | APIs |
|---|---|---|
| **Real time** (calendar) | Wall-clock time measured from the Epoch | `time()`, `gettimeofday()`, `clock_gettime()` |
| **Process time** (CPU) | CPU time consumed by a process | `clock()`, `times()`, `CLOCK_PROCESS_CPUTIME_ID` |

**Epoch** = 00:00:00 UTC, January 1, 1970. All calendar time is stored as seconds elapsed since this reference point.

**Year 2038 problem:** On 32-bit systems, `time_t` is a `signed 32-bit int` → overflows at 03:14:07 UTC on January 19, 2038. On 64-bit systems, `time_t` is `int64_t` → no issue.

---

### 2. Getting Calendar Time

**`time()`** — simplest, second precision:

```c
#include <time.h>
time_t t = time(NULL);  /* seconds since Epoch */
```

**`gettimeofday()`** — microsecond precision (deprecated in POSIX.1-2008):

```c
#include <sys/time.h>
struct timeval tv;
gettimeofday(&tv, NULL);
/* tv.tv_sec  = seconds since Epoch */
/* tv.tv_usec = microseconds (0–999999) */
```

**`clock_gettime(CLOCK_REALTIME)`** — preferred modern API, nanosecond precision:

```c
#include <time.h>
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
/* ts.tv_sec  = seconds since Epoch */
/* ts.tv_nsec = nanoseconds (0–999999999) */
```

**Rule:** Use `clock_gettime()` in new code. `gettimeofday()` remains widespread in existing codebases.

---

### 3. Process Time

```c
#include <time.h>
clock_t used = clock();
double cpu_secs = (double)used / CLOCKS_PER_SEC;
```

More detail via `times()`:

```c
#include <sys/times.h>
struct tms t;
times(&t);
/* t.tms_utime  = user CPU ticks (process) */
/* t.tms_stime  = kernel CPU ticks (process) */
/* t.tms_cutime = user CPU ticks (waited-for children) */
/* t.tms_cstime = kernel CPU ticks (waited-for children) */
/* Unit: clock ticks = sysconf(_SC_CLK_TCK) */
```

Or via POSIX clocks (preferred):

```c
struct timespec ts;
clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);  /* total CPU of process */
clock_gettime(CLOCK_THREAD_CPUTIME_ID,  &ts);  /* CPU of current thread */
```

---

### 4. Time Conversion Chain

```
time_t ──gmtime()────► struct tm  (UTC)
       ──localtime()─► struct tm  (local timezone)
                            │
                            ├── asctime()  ──► fixed-format string
                            ├── strftime() ──► custom-format string
                            └── mktime()   ──► time_t  (reverse conversion)

string ──strptime()──► struct tm
```

**`struct tm`** — broken-down time representation:

```c
struct tm {
    int tm_sec;    /* 0–60 (60 allows for leap second) */
    int tm_min;    /* 0–59 */
    int tm_hour;   /* 0–23 */
    int tm_mday;   /* 1–31 */
    int tm_mon;    /* 0–11  ← January = 0, not 1 */
    int tm_year;   /* years since 1900 */
    int tm_wday;   /* 0 = Sunday ... 6 = Saturday */
    int tm_yday;   /* 0–365 */
    int tm_isdst;  /* >0 DST active, 0 not active, <0 unknown */
};
```

**Conversion functions:**

```c
/* time_t → struct tm */
struct tm *gmt = gmtime(&t);       /* UTC — uses internal static buffer */
struct tm *loc = localtime(&t);    /* local timezone — uses internal static buffer */

/* Thread-safe variants (use these in multi-threaded code) */
struct tm result;
gmtime_r(&t, &result);
localtime_r(&t, &result);

/* struct tm → time_t */
time_t t2 = mktime(loc);   /* normalizes struct tm; fills tm_wday, tm_yday;
                               respects tm_isdst=-1 to auto-determine DST */

/* struct tm → string */
char buf[64];
strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", loc);
/* → "2026-03-26 14:30:00" */

/* string → struct tm */
struct tm parsed = {0};
strptime("2026-03-26", "%Y-%m-%d", &parsed);
```

**Common `strftime` format specifiers:**

| Specifier | Output |
|---|---|
| `%Y` | 4-digit year (2026) |
| `%m` | Month `01`–`12` |
| `%d` | Day `01`–`31` |
| `%H` | Hour `00`–`23` |
| `%M` | Minute `00`–`59` |
| `%S` | Second `00`–`60` |
| `%Z` | Timezone name (`UTC`, `ICT`) |
| `%s` | Seconds since Epoch (non-standard, widely supported) |
| `%F` | Equivalent to `%Y-%m-%d` |
| `%T` | Equivalent to `%H:%M:%S` |

**Thread safety:** `gmtime()`, `localtime()`, `ctime()` use **internal static buffers** → not thread-safe. Always use `gmtime_r()`, `localtime_r()`, `ctime_r()` in multi-threaded code.

---

### 5. Timezone Handling

Timezone resolution order:
1. `TZ` environment variable (overrides everything for the process)
2. `/etc/localtime` (system default, usually a symlink into `/usr/share/zoneinfo/`)

```bash
TZ="America/New_York" date   # override timezone for a single command
TZ="UTC" date
```

Changing timezone at runtime in C:

```c
setenv("TZ", "Asia/Ho_Chi_Minh", 1);
tzset();        /* re-reads TZ, updates global variables: timezone, daylight, tzname[] */
localtime(&t);  /* now uses the new timezone */
```

**DST handling via `mktime()`:**

| `tm_isdst` | Behavior |
|---|---|
| `-1` | Let `mktime()` determine DST automatically (recommended) |
| `0`  | Force standard time |
| `1`  | Force daylight saving time |

`mktime()` also **normalizes out-of-range values** (e.g., `tm_hour = 25` → advances `tm_mday` by 1) and fills in `tm_wday` and `tm_yday`.

---

### 6. Measuring Elapsed Time Correctly

```c
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);

/* ... code under measurement ... */

clock_gettime(CLOCK_MONOTONIC, &end);

double elapsed = (end.tv_sec  - start.tv_sec)
               + (end.tv_nsec - start.tv_nsec) / 1e9;
printf("Elapsed: %.6f seconds\n", elapsed);
```

**Why `CLOCK_MONOTONIC` and not `CLOCK_REALTIME`:**
`CLOCK_REALTIME` can be stepped backward by NTP or `clock_settime()` during measurement → `elapsed` can be negative or wrong. `CLOCK_MONOTONIC` only moves forward and is immune to time adjustments.

---

## System Context — Time API in the Linux Architecture

```
RTC chip (battery-backed, survives power-off)
               │ set at boot
               ▼
  Kernel clocksource (TSC / HPET / APIC)
               │
       ┌───────┴──────────────────────┐
       │                              │
  CLOCK_REALTIME                CLOCK_MONOTONIC
  (wall-clock, NTP-adjusted)    (since boot, never decreases)
       │                              │
       └──────────────┬───────────────┘
                      │
           POSIX Time API (libc)
           time(), gettimeofday(), clock_gettime()
                      │
           Conversion layer (libc)
           gmtime_r(), localtime_r(), strftime(), mktime()
                      │
           Timezone data: /usr/share/zoneinfo/ + TZ env var
```

**Subsystem interactions:**
- **NTP daemon (`ntpd`/`chronyd`):** continuously adjusts `CLOCK_REALTIME` — can step it backward; `CLOCK_MONOTONIC` is immune.
- **VFS (filesystem):** file `mtime`, `atime`, `ctime` are set from `CLOCK_REALTIME` via `current_time()` on each syscall.
- **Process scheduler:** `CLOCK_PROCESS_CPUTIME_ID` and `CLOCK_THREAD_CPUTIME_ID` are accumulated by the scheduler on every context switch.
- **Hibernation/suspend:** `CLOCK_MONOTONIC` stops during suspend; `CLOCK_BOOTTIME` (Linux extension) includes suspend time.

**Failure scenarios:**
- `CLOCK_REALTIME` used for elapsed time measurement + NTP steps backward mid-measurement → **elapsed is negative or inflated**.
- Changing `TZ` without calling `tzset()` → `localtime()` still uses old timezone rule.
- Calling `gmtime()` / `localtime()` from two threads simultaneously → **race condition on static buffer** → corrupted `struct tm`.
- `time_t` on 32-bit system after January 19, 2038 → overflow → dates appear as 1901.
- `mktime()` on uninitialized `struct tm` → garbage fields produce wrong result; always zero-initialize.

---

## Debugging

```bash
# Check system clock and NTP sync status
timedatectl status              # local time, NTP sync state, timezone

# Compare system clock vs hardware clock
hwclock --show                  # RTC time (may differ from system clock)
date                            # system clock

# Diagnose NTP
chronyc tracking                # offset, drift, reference source
timedatectl show-timesync

# Debug timezone
ls -la /etc/localtime           # shows linked zoneinfo file
TZ="UTC" date                   # override timezone for one command
TZ="Asia/Ho_Chi_Minh" date
```

**Common time API bugs:**

```c
/* BUG: not thread-safe */
struct tm *t = localtime(&epoch);   /* static buffer — race in multithreaded code */

/* FIX */
struct tm t;
localtime_r(&epoch, &t);            /* caller-supplied buffer */

/* BUG: uninitialized struct tm — garbage in unset fields */
struct tm t;
t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 1;
mktime(&t);  /* tm_hour/min/sec/isdst are garbage → wrong result */

/* FIX */
struct tm t = {0};              /* zero-initialize first */
t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 1; t.tm_isdst = -1;
mktime(&t);

/* BUG: elapsed time with CLOCK_REALTIME — unsafe under NTP */
clock_gettime(CLOCK_REALTIME, &start);
do_work();
clock_gettime(CLOCK_REALTIME, &end);
/* end < start if NTP stepped clock backward → negative elapsed */

/* FIX */
clock_gettime(CLOCK_MONOTONIC, &start);  /* immune to NTP adjustments */
```

---

## Real-world Usage

| Use case | API | Notes |
|---|---|---|
| Log timestamps | `clock_gettime(CLOCK_REALTIME)` + `strftime()` | ISO 8601: `%Y-%m-%dT%H:%M:%S` |
| Performance profiling | `clock_gettime(CLOCK_PROCESS_CPUTIME_ID)` | CPU time consumed, not wall time |
| Session / token expiry | `time()` + compare stored timestamp | Difference in seconds |
| Schedule next run (cron-style) | `mktime()` to compute next target | Always set `tm_isdst = -1` |
| Sub-second RTT measurement | `clock_gettime(CLOCK_MONOTONIC)` | Nanosecond precision, immune to NTP |
| Reproducible builds | `SOURCE_DATE_EPOCH` env var | Tools read this instead of live `time()` |

---

### Key Takeaways — 4.5

1. `time_t` = seconds since Epoch (Jan 1 1970 UTC). Year 2038 overflow only affects 32-bit `time_t`.
2. Precision hierarchy: `clock_gettime()` (ns) > `gettimeofday()` (µs) > `time()` (s). Use `clock_gettime()` in new code.
3. `struct tm` pitfalls: `tm_mon` is 0-indexed (January = 0); `tm_year` = actual year − 1900.
4. `gmtime()` / `localtime()` / `ctime()` use static internal buffers — **not thread-safe**. Use `_r` variants in multi-threaded code.
5. `mktime()` normalizes out-of-range `struct tm` fields and auto-fills `tm_wday` / `tm_yday`. Set `tm_isdst = -1` to let it determine DST.
6. Measuring elapsed time → `CLOCK_MONOTONIC`. Timestamping events → `CLOCK_REALTIME`. Never use `CLOCK_REALTIME` for duration measurement.
7. Timezone: `/etc/localtime` system default; `TZ` env var overrides per-process; call `tzset()` after changing `TZ` at runtime.
