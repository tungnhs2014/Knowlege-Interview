# Chapter 4 - Timers and Time API

> Topics: 4.4 Timers & Sleeping · 4.5 Time API
> Main sources: TLPI Ch23, Ch10 · Linux man-pages · DevLinux Module 04 Exercise 2 for `SIGALRM` intuition

---

## Coverage Notes

This file covers Chapter 4 mapped rows 4.4 and 4.5:

- 4.4 Timers & Sleeping: `sleep()`, `nanosleep()`, `clock_nanosleep()`, `alarm()`, `setitimer()`, POSIX timers, `timerfd`, timer notification choices, signal interruption, and overrun handling.
- 4.5 Time API: calendar time, elapsed time, process/thread CPU time, `time_t`, `timeval`, `timespec`, `struct tm`, `gettimeofday()`, `clock_gettime()`, `clock_getres()`, time conversion, timezones, and Year 2038 awareness.
- Must-cover production concepts: timeout design, process time versus wall time, `CLOCK_REALTIME` versus `CLOCK_MONOTONIC`, low-drift periodic loops, timer delivery choices, debugging commands, and Embedded scheduling constraints.
- Signal handler mechanics and advanced signal reception are covered in `ch04_signals_core.md`; this file focuses on clock/timer choice and timeout behavior.

---

## Problem It Solves

Linux programs ask several different time questions:

```text
What time is it for a log line?
How long did this operation take?
How can this thread sleep without wasting CPU?
How can a blocking operation have a timeout?
How can a task run every 100 ms?
How can timers fit into an epoll server?
```

The trap:

```text
"time" is not one thing.
```

Calendar timestamps, elapsed duration, CPU time, sleeping, and timer notifications use
different clocks and APIs.

Wrong clock choice creates real bugs:

```text
Use wall-clock time for a timeout
    -> system time jumps backward
    -> timeout waits too long

Use monotonic time for a timestamp
    -> output is not a human date
```

---

## Learning Roadmap

| Priority | Learn this | Why it matters |
|----------|------------|----------------|
| Must know | calendar time vs elapsed time vs CPU time | prevents wrong API choice |
| Must know | `CLOCK_REALTIME` vs `CLOCK_MONOTONIC` | core production distinction |
| Must know | `time_t`, `timeval`, `timespec`, `struct tm` | common time data structures |
| Must know | `clock_gettime()` | modern explicit clock read |
| Must know | `clock_getres()` | know the clock's practical resolution |
| Must know | `sleep()`, `nanosleep()`, signal interruption | sleeping can return early |
| Must know | `alarm()` basics | simple signal-based timeout |
| Work useful | `clock_nanosleep(... TIMER_ABSTIME ...)` | low-drift periodic loops |
| Work useful | `timerfd` | Linux event-loop timers |
| Recognize | `setitimer()`, POSIX timers, timer overruns | real but less common in first-pass app code |
| Recognize | Year 2038, timezone conversion details | important in embedded/logging/data formats |

Decision rule:

```text
human timestamp  -> CLOCK_REALTIME / time()
duration/timeout -> CLOCK_MONOTONIC
CPU profiling    -> CPU-time clocks
event loop timer -> timerfd
```

---

## Core Vocabulary

Read this table before the API sections. Most time bugs come from mixing these terms.

| Term | Meaning | Example / note |
|------|---------|----------------|
| Calendar time | Human wall-clock date/time | `2026-05-05 10:30:00 +07` |
| Elapsed time | Duration between two moments | request took `37 ms` |
| Process CPU time | CPU consumed by a process, not real elapsed time | profiling CPU-heavy code |
| Clock | Source of time values with a specific meaning | `CLOCK_REALTIME`, `CLOCK_MONOTONIC` |
| `CLOCK_REALTIME` | Wall-clock calendar time, affected by system time changes | good for logs |
| `CLOCK_MONOTONIC` | Monotonic elapsed-time clock; not a human date | good for durations and timeouts |
| Time point | A specific reading from a clock | `clock_gettime(..., &ts)` |
| Duration | Difference between two time points from the same clock | `end - start` |
| Sleep | Stop running until time passes or a target time arrives | `nanosleep()` |
| Timeout | Maximum time an operation is allowed to wait | socket read timeout |
| Timer | Object/request that notifies the program later | `alarm()`, POSIX timer, `timerfd` |
| Relative time | Interval from now | sleep for 100 ms |
| Absolute time | Fixed target time on a clock | sleep until `next` |
| Drift | Periodic loop slowly moves away from intended schedule | work time + relative sleeps accumulate |
| Overrun | Timer expired more times than the program consumed notifications | check `timerfd` count or POSIX overrun |
| `time_t` | Calendar time value, usually seconds since Unix Epoch | used by `time()` |
| `struct timespec` | seconds + nanoseconds | modern high-resolution APIs |
| `struct timeval` | seconds + microseconds | legacy APIs |
| `struct tm` | broken-down human calendar fields | year, month, day, hour |
| Timezone | Rules for converting UTC calendar time to local time | `TZ`, `/etc/localtime`, zoneinfo |
| `timerfd` | Linux timer exposed as a readable file descriptor | fits `epoll` event loops |

Main beginner rule:

```text
Timestamp for humans -> realtime/calendar time.
Timeout or latency   -> monotonic elapsed time.
```

---

## Concept Overview

### Time Is Not One Thing

| Need | Meaning | Typical API |
|------|---------|-------------|
| Human timestamp | calendar/wall-clock time | `time()`, `clock_gettime(CLOCK_REALTIME)` |
| Elapsed duration | time passed between two points | `clock_gettime(CLOCK_MONOTONIC)` |
| Sleep | stop running for an interval or until a target time | `nanosleep()`, `clock_nanosleep()` |
| Future notification | wake or notify later | `alarm()`, `timerfd`, POSIX timers |
| CPU usage | CPU time consumed by process/thread | `CLOCK_PROCESS_CPUTIME_ID`, `CLOCK_THREAD_CPUTIME_ID` |

### `CLOCK_REALTIME` vs `CLOCK_MONOTONIC`

| Question | `CLOCK_REALTIME` | `CLOCK_MONOTONIC` |
|----------|------------------|-------------------|
| What does it represent? | wall-clock calendar time | elapsed monotonic time from a fixed point |
| Can it jump? | yes, if system time is changed | no discontinuous wall-clock jumps |
| Good for logs? | yes | no |
| Good for durations/timeouts? | no | yes |
| Example output meaning | date/time since Epoch | not a human date |

Main rule:

```text
If humans read it, use realtime.
If code measures duration, use monotonic.
```

Precision note: `CLOCK_MONOTONIC` is not a calendar clock. It is meant for elapsed time.
Time synchronization may adjust its rate gradually, but it should not jump backward because
an administrator or NTP changed the wall clock.

### Calendar Time vs Elapsed Time

Calendar time:

```text
2026-05-05 10:30:00 +07
```

Elapsed time:

```text
request took 37 ms
```

Do not compute request latency using calendar time unless you have a specific reason and
understand wall-clock jumps.

### Relative vs Absolute Sleep

Relative sleep:

```text
sleep for 100 ms from now
```

Absolute sleep:

```text
sleep until target_time
```

Periodic loops should prefer absolute target time:

```text
next = now + 100 ms
loop:
    do work
    sleep until next
    next += 100 ms
```

That avoids accumulating drift from work time and repeated relative sleep delays.

### Resolution, Accuracy, and Scheduling Delay

These words are related but not the same:

| Term | Meaning |
|------|---------|
| Resolution | smallest clock/timer unit the system reports |
| Accuracy | how close the wakeup/read is to real elapsed time |
| Scheduling delay | extra delay before your process runs after a timer expires |

`clock_getres()` tells the nominal resolution of a clock. It does not promise that your
process will wake exactly at that instant. The scheduler still decides when the process
runs after the timer expires.

---

## System Context

Timers and time APIs touch several Linux subsystems:

```text
hardware clocks
    |
    v
kernel timekeeping
    |
    +-- CLOCK_REALTIME
    +-- CLOCK_MONOTONIC
    +-- CPU-time clocks
    |
    v
user APIs
    time()
    clock_gettime()
    nanosleep()
    alarm()
    timerfd
```

Timers also connect to signals:

```text
alarm()      -> SIGALRM
setitimer()  -> SIGALRM / SIGVTALRM / SIGPROF
POSIX timer  -> signal, thread callback, or no notification
timerfd      -> readable file descriptor
```

Modern Linux event-loop services often prefer `timerfd` because timer expiration becomes
ordinary fd readiness, just like sockets or pipes.

---

## Architecture

### Time Structures

| Structure | Resolution | Common use |
|-----------|------------|------------|
| `time_t` | seconds | calendar time since Unix Epoch |
| `struct timeval` | seconds + microseconds | legacy APIs, `gettimeofday()`, `setitimer()` |
| `struct timespec` | seconds + nanoseconds | `clock_gettime()`, `nanosleep()`, timers |
| `struct itimerval` | `timeval` value + interval | `setitimer()` |
| `struct itimerspec` | `timespec` value + interval | POSIX timers, `timerfd` |
| `struct tm` | calendar fields | year/month/day/hour/min/sec conversions |

Modern high-resolution APIs usually use:

```c
struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};
```

`tv_nsec` must be from `0` to `999999999`.

### Timer Notification Models

| Model | API | Receiver |
|-------|-----|----------|
| signal | `alarm()`, `setitimer()`, POSIX timer with `SIGEV_SIGNAL` | signal handler or `sigwaitinfo()` |
| file descriptor | `timerfd_create()` | `read()` and `select()`/`poll()`/`epoll()` |
| thread callback | POSIX timer with `SIGEV_THREAD` | callback runs as if in a new thread |
| polling | POSIX timer with `SIGEV_NONE` | caller checks timer manually |

Work-oriented selection:

| Need | Good choice |
|------|-------------|
| simple one-shot timeout in small program | `alarm()` |
| elapsed measurement | `CLOCK_MONOTONIC` |
| low-drift periodic loop | `clock_nanosleep()` with `TIMER_ABSTIME` |
| event loop timer | `timerfd` |
| human log timestamp | `time()` or `CLOCK_REALTIME` + conversion |
| CPU profiling | process/thread CPU-time clocks |

---

## Execution Flow

### Flow 1: Measure Duration Correctly

```text
clock_gettime(CLOCK_MONOTONIC, start)
    |
    v
do work
    |
    v
clock_gettime(CLOCK_MONOTONIC, end)
    |
    v
duration = end - start
```

### Flow 2: Sleep Can Be Interrupted

```text
nanosleep(request)
    |
    +-- completes normally
    |
    +-- signal arrives
            |
            v
        returns -1, errno = EINTR
```

Any robust sleep logic needs to decide what to do on `EINTR`.

### Flow 3: Simple Timeout With `SIGALRM`

```text
install SIGALRM handler without SA_RESTART
    |
    v
alarm(5)
    |
    v
read(fd, ...)
    |
    +-- data arrives -> alarm(0)
    |
    +-- SIGALRM arrives -> read() may fail with EINTR
```

This is useful to understand, but `poll()`, `pselect()`, `ppoll()`, or `timerfd` often
compose better in real I/O programs.

### Flow 4: Event Loop Timer With `timerfd`

```text
timerfd_create(CLOCK_MONOTONIC)
    |
    v
timerfd_settime()
    |
    v
epoll_wait()
    |
    v
read(timerfd) -> expiration count
```

This fits naturally with sockets, pipes, serial devices, and other fd-based inputs.

---

## 4.4 Timers and Sleeping

### `sleep()`

```c
#include <unistd.h>

unsigned int sleep(unsigned int seconds);
```

Use for simple, coarse sleeping. It can return early if interrupted by a signal.

For serious timing, prefer `nanosleep()` or `clock_nanosleep()`.

### `nanosleep()`

```c
#define _POSIX_C_SOURCE 199309L
#include <time.h>

int nanosleep(const struct timespec *request, struct timespec *remain);
```

Properties:

- API has nanosecond fields;
- sleep may still be interrupted by a signal;
- on `EINTR`, `remain` can tell how much relative sleep is left.

Restarting relative sleeps is okay for simple waits, but repeated interruptions can
accumulate drift.

### `clock_nanosleep()`

```c
#define _XOPEN_SOURCE 600
#include <time.h>

int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec *request,
                    struct timespec *remain);
```

Most important feature:

```text
TIMER_ABSTIME
```

With `TIMER_ABSTIME`, `request` is an absolute target time. If interrupted, retry the same
target instead of computing a new relative delay.

Use with `CLOCK_MONOTONIC` for low-drift periodic loops.

### `alarm()`

```c
#include <unistd.h>

unsigned int alarm(unsigned int seconds);
```

Behavior:

- after `seconds`, kernel sends `SIGALRM`;
- `alarm(0)` cancels the alarm;
- the alarm is one-shot, not automatically periodic;
- only one such process-wide alarm exists.

Use it for simple demos or simple process-owned timeout guards.

Avoid it in libraries and event loops because it is global process state and signal-based.

DevLinux Exercise 2 uses the classic learning pattern:

```text
alarm(1)
    -> SIGALRM arrives
    -> handler/main path calls alarm(1) again for the next tick
```

Keep the handler minimal. Printing and exiting directly from the handler are useful for a
beginner demo, but they are not the production-safe pattern.

### `setitimer()`

`setitimer()` is a legacy interval-timer API.

```c
#include <sys/time.h>

int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);
int getitimer(int which, struct itimerval *curr_value);
```

| Timer | Measures | Signal |
|-------|----------|--------|
| `ITIMER_REAL` | wall-clock elapsed time | `SIGALRM` |
| `ITIMER_VIRTUAL` | user CPU time | `SIGVTALRM` |
| `ITIMER_PROF` | user + kernel CPU time | `SIGPROF` |

Important:

- only one timer of each type exists per process;
- on Linux, `alarm()` and `setitimer(ITIMER_REAL)` share the same real-time timer;
- do not mix them casually.

For new multi-timer code, prefer POSIX timers or `timerfd` depending on the design.

### POSIX Timers

POSIX timers support:

- multiple timers;
- `struct timespec` resolution;
- signal, thread, or no notification;
- timer overrun reporting.

Core APIs:

```c
int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *value,
                  struct itimerspec *old_value);
int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
int timer_getoverrun(timer_t timerid);
int timer_delete(timer_t timerid);
```

If a signal-notifying POSIX timer expires multiple times before the process receives the
signal, Linux does not queue one signal per expiration. The receiver should use
`timer_getoverrun()` or `siginfo_t` overrun information to detect missed expirations.

First-pass expectation:

```text
Know what problem POSIX timers solve.
Do not memorize every notification mode unless you work on timer-heavy systems code.
```

### `timerfd`

Linux-specific timer as a file descriptor:

```c
#include <sys/timerfd.h>

int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags,
                    const struct itimerspec *new_value,
                    struct itimerspec *old_value);
int timerfd_gettime(int fd, struct itimerspec *curr_value);
```

Read expiration count:

```c
uint64_t expirations;
read(fd, &expirations, sizeof(expirations));
```

Why it matters:

- works naturally with `select()`, `poll()`, `epoll()`;
- no async signal handler;
- expiration count tells if the program fell behind.

Useful flags:

| Flag | Where | Meaning |
|------|-------|---------|
| `TFD_CLOEXEC` | `timerfd_create()` | close fd across `exec()` |
| `TFD_NONBLOCK` | `timerfd_create()` | make `read()` nonblocking |
| `TFD_TIMER_ABSTIME` | `timerfd_settime()` | interpret initial expiration as absolute time |

---

## 4.5 Time API

### Calendar Time and `time_t`

Unix calendar time is seconds since:

```text
1970-01-01 00:00:00 UTC
```

Basic API:

```c
#include <time.h>

time_t time(time_t *timep);
```

`time_t` is the common stored representation for calendar time.

### `gettimeofday()` Legacy Caveat

```c
#include <sys/time.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);
```

Know it because old code uses it.

Do not choose it first for new elapsed-time code:

- it reads wall-clock time;
- wall-clock time can jump;
- the timezone argument is obsolete and should be `NULL`.

Prefer:

```c
clock_gettime(CLOCK_MONOTONIC, ...)
```

for durations and timeouts.

### `clock_gettime()`

```c
#define _POSIX_C_SOURCE 199309L
#include <time.h>

int clock_gettime(clockid_t clockid, struct timespec *tp);
int clock_getres(clockid_t clockid, struct timespec *res);
```

Common clocks:

| Clock | Use |
|-------|-----|
| `CLOCK_REALTIME` | human timestamps |
| `CLOCK_MONOTONIC` | durations, timeouts, scheduling |
| `CLOCK_PROCESS_CPUTIME_ID` | process CPU profiling |
| `CLOCK_THREAD_CPUTIME_ID` | thread CPU profiling |

Use `clock_getres()` when you need to understand the clock's nominal precision, especially
for embedded timing, profiling, or explaining why tiny sleep intervals do not behave like
hard realtime delays.

### Broken-Down Time

`struct tm` is a human calendar breakdown:

```c
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
```

Common traps:

```text
tm_mon is 0-based: January is 0.
tm_year is years since 1900.
```

### Time Conversion

```text
time_t
    |
    +--> gmtime_r()     -> UTC struct tm
    |
    +--> localtime_r()  -> local-time struct tm

struct tm
    |
    +--> mktime()       -> time_t using local timezone rules
    |
    +--> strftime()     -> formatted string
```

Prefer reentrant versions in reusable or threaded code:

| Avoid | Prefer |
|-------|--------|
| `gmtime()` | `gmtime_r()` |
| `localtime()` | `localtime_r()` |
| `ctime()` | `ctime_r()` |
| `asctime()` | `asctime_r()` |

### `mktime()` Details

`mktime()` converts local broken-down time back to `time_t`.

Important behavior:

- it may modify the input `struct tm`;
- it normalizes out-of-range fields such as `tm_hour = 25`;
- it fills `tm_wday` and `tm_yday`;
- `tm_isdst = -1` asks libc to determine daylight-saving behavior.

### Timezones

Calendar conversion uses libc timezone rules:

```text
TZ environment variable
/etc/localtime
zoneinfo files, usually under /usr/share/zoneinfo
```

Changing `TZ` affects `localtime()`, `mktime()`, and `strftime()`. Call `tzset()` after
changing `TZ` inside a process.

### Process Time

Process time measures CPU consumed, not wall-clock duration:

```c
#include <time.h>

clock_t c = clock();
double cpu_seconds = (double)c / CLOCKS_PER_SEC;
```

For explicit clock style:

```c
struct timespec ts;

clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
```

Use CPU time for profiling computation. Use monotonic time for user-visible latency.

### Year 2038

On systems with signed 32-bit `time_t`, calendar time overflows in January 2038.

Most relevant for:

- old 32-bit systems;
- long-lived embedded systems;
- binary protocols and file formats storing 32-bit timestamps.

---

## Example

### Example 1 - Measure Elapsed Time With `CLOCK_MONOTONIC`

```c
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double diff_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec)
         + (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

int main(void)
{
    struct timespec start;
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    sleep(1);

    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    printf("elapsed %.6f seconds\n", diff_seconds(start, end));
    return EXIT_SUCCESS;
}
```

What it teaches:

- elapsed measurement should use `CLOCK_MONOTONIC`;
- duration is the difference between two readings;
- wall-clock jumps should not affect latency measurements.

### Example 2 - Low-Drift Periodic Loop

```c
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void add_ms(struct timespec *ts, long ms)
{
    ts->tv_nsec += ms * 1000000L;
    while (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

int main(void)
{
    struct timespec next;

    if (clock_gettime(CLOCK_MONOTONIC, &next) == -1) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 5; i++) {
        add_ms(&next, 100);

        for (;;) {
            int s = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

            if (s == 0) {
                break;
            }

            if (s != EINTR) {
                errno = s;
                perror("clock_nanosleep");
                return EXIT_FAILURE;
            }
        }

        printf("tick %d\n", i + 1);
    }

    return EXIT_SUCCESS;
}
```

What it teaches:

- periodic work should target absolute times;
- retrying the same absolute target avoids drift after interruption;
- `CLOCK_MONOTONIC` is the correct clock for periodic scheduling.

### Example 3 - Periodic Timer With `timerfd`

```c
#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (fd == -1) {
        perror("timerfd_create");
        return EXIT_FAILURE;
    }

    struct itimerspec spec = {
        .it_value = { .tv_sec = 1, .tv_nsec = 0 },
        .it_interval = { .tv_sec = 1, .tv_nsec = 0 },
    };

    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        perror("timerfd_settime");
        close(fd);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 3; i++) {
        uint64_t expirations;
        ssize_t n = read(fd, &expirations, sizeof(expirations));

        if (n != (ssize_t)sizeof(expirations)) {
            perror("read");
            close(fd);
            return EXIT_FAILURE;
        }

        printf("timer expired %llu time(s)\n",
               (unsigned long long)expirations);
    }

    close(fd);
    return EXIT_SUCCESS;
}
```

What it teaches:

- `timerfd` turns timer expiration into fd input;
- `read()` returns an expiration count;
- this design fits Linux event loops better than signal handlers.

### Example 4 - Format UTC and Local Time Safely

```c
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now = time(NULL);
    struct tm utc_tm;
    struct tm local_tm;
    char utc_buf[64];
    char local_buf[64];

    if (gmtime_r(&now, &utc_tm) == NULL) {
        perror("gmtime_r");
        return EXIT_FAILURE;
    }

    if (localtime_r(&now, &local_tm) == NULL) {
        perror("localtime_r");
        return EXIT_FAILURE;
    }

    strftime(utc_buf, sizeof(utc_buf), "%Y-%m-%d %H:%M:%S %Z", &utc_tm);
    strftime(local_buf, sizeof(local_buf), "%Y-%m-%d %H:%M:%S %Z", &local_tm);

    printf("UTC:   %s\n", utc_buf);
    printf("Local: %s\n", local_buf);
    return EXIT_SUCCESS;
}
```

What it teaches:

- `time_t` is calendar time;
- `gmtime_r()` converts to UTC;
- `localtime_r()` converts through local timezone rules;
- `strftime()` formats the result safely into caller-provided buffers.

---

## Debugging

Inspect system time:

```bash
date
timedatectl
TZ=UTC date
readlink -f /etc/localtime
```

Trace timing APIs:

```bash
strace -e trace=clock_gettime,nanosleep,clock_nanosleep,timerfd_create,timerfd_settime ./program
```

Common bugs:

| Bug | Fix |
|-----|-----|
| using `CLOCK_REALTIME` for timeouts | use `CLOCK_MONOTONIC` |
| assuming sleep always sleeps full duration | handle `EINTR` |
| relative sleep in periodic loop drifts | use absolute `clock_nanosleep()` |
| mixing `alarm()` and `setitimer(ITIMER_REAL)` | choose one owner |
| forgetting `alarm(0)` after success | cancel unused alarm |
| ignoring `timerfd` expiration count | use the `uint64_t` count |
| using `localtime()` in reusable/threaded code | use `localtime_r()` |
| treating `tm_mon` as 1-based | remember January is 0 |

---

## Real-world Usage

| Scenario | Practical choice |
|----------|------------------|
| request latency | `CLOCK_MONOTONIC` before/after |
| log timestamp | `time()` or `CLOCK_REALTIME` + `strftime()` |
| simple CLI timeout | `alarm()` if the process owns it |
| socket timeout | `poll()`/`ppoll()`/`pselect()` or socket timeout options |
| event-loop timeout | `timerfd` |
| embedded periodic polling | absolute `clock_nanosleep()` |
| CPU profiling | CPU-time clocks |
| portable calendar formatting | `localtime_r()` / `gmtime_r()` + `strftime()` |

---

## Work Checklist

Use this checklist when designing timeout or periodic code:

- Use `CLOCK_MONOTONIC` for durations, deadlines, timeouts, and periodic scheduling.
- Use `CLOCK_REALTIME` or `time()` only when the result is a human calendar timestamp.
- Prefer absolute `clock_nanosleep(..., TIMER_ABSTIME, ...)` for fixed-rate Embedded loops.
- Treat `sleep()`, `nanosleep()`, and blocking waits as interruptible; decide the `EINTR` policy.
- Avoid `alarm()` in libraries or multi-timer services because it is process-wide and signal-based.
- Do not mix `alarm()` with `setitimer(ITIMER_REAL)` unless one owner controls both.
- For `epoll`/fd-based services, prefer `timerfd` and always read the `uint64_t` expiration count.
- Check timer overruns when missed periods matter.
- Use reentrant time conversion functions such as `localtime_r()` and `gmtime_r()` in reusable or threaded code.
- On Embedded targets, budget for scheduler latency, clock resolution, no-RTC or RTC-jump scenarios, power-management effects, and 32-bit `time_t` risk.

---

## Interview-Relevant Questions

These questions usually test whether you choose the right clock/API for the job:

1. What is the difference between calendar time, elapsed time, and process CPU time?
2. Why is `CLOCK_MONOTONIC` better than `CLOCK_REALTIME` for durations and timeouts?
3. When should you use `CLOCK_REALTIME`?
4. What is `time_t`, and what is the Year 2038 problem?
5. Why is `gettimeofday()` not preferred for new timeout or latency code?
6. What does `clock_getres()` tell you, and what does it not guarantee?
7. How can `sleep()` or `nanosleep()` return early?
8. What is `EINTR` in sleep/timer code?
9. Why can relative sleeps drift in a periodic loop?
10. How does `clock_nanosleep(..., TIMER_ABSTIME, ...)` reduce drift?
11. What does `alarm()` do, and why is it risky in libraries?
12. How do `alarm()` and `setitimer(ITIMER_REAL)` interact on Linux?
13. What are `ITIMER_REAL`, `ITIMER_VIRTUAL`, and `ITIMER_PROF`?
14. What problem do POSIX timers solve compared with `alarm()`/`setitimer()`?
15. What is a timer overrun?
16. Why is `timerfd` useful in an `epoll`-based service?
17. What does the `uint64_t` read from a `timerfd` represent?
18. Why should reusable/threaded code prefer `localtime_r()` and `gmtime_r()`?
19. What are the common `struct tm` traps with `tm_mon` and `tm_year`?
20. What does `mktime()` do, and why can it modify the input `struct tm`?

---

## Key Takeaways

1. Time APIs solve different problems: timestamp, duration, sleep, timer, CPU profiling.
2. Use `CLOCK_REALTIME` for human calendar timestamps.
3. Use `CLOCK_MONOTONIC` for durations, timeouts, and periodic scheduling.
4. Sleep can be interrupted; handle `EINTR`.
5. Relative sleep can drift; absolute sleep is better for periodic loops.
6. `alarm()` is simple but signal-based and process-global.
7. `setitimer()` is legacy and limited.
8. POSIX timers support multiple timers and overrun reporting, but are more complex.
9. `timerfd` is the clean Linux choice for fd-based event loops.
10. `mktime()` and timezone conversion are local-time operations affected by `TZ`.
11. Use CPU-time clocks for CPU profiling, not request latency.
12. Prefer reentrant time conversion functions in reusable or threaded code.

---

## Final Coverage Check

| Required item | Status |
|---------------|--------|
| 4.4 timers and sleeping | Covered |
| 4.5 time API | Covered |
| Timer lifecycle and notification choices | Covered |
| Process time versus wall/elapsed time | Covered |
| Timeout and low-drift periodic design | Covered |
| Production bugs and debugging commands | Covered |
| Embedded constraints | Covered |
| Interview readiness | Covered |
