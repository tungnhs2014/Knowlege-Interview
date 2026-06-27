# 05 - Core Kernel Facilities Example

This is a **learning-only** Linux kernel module. It does not bind to hardware or
expose a stable device ABI. It demonstrates how several core facilities live in
one enclosing object and how their lifetimes interact.

## Goal

Observe this one-shot flow:

```text
module init
  -> allocate and initialize one enclosing object
  -> add its embedded node to an intrusive list
  -> start a waiter kthread
  -> arm a timer
timer callback (atomic context)
  -> recover the owner with from_timer()
  -> queue work
work callback (process context)
  -> recover the owner with container_of()
  -> publish protected state
  -> wake the wait queue
  -> signal the completion
waiter + module init
  -> consume the condition and completion
module exit
  -> stop timer, cancel work, stop waiter, unlink, free
```

The module demonstrates:

- `container_of()` and `from_timer()`;
- embedded `struct list_head` and safe unlinking;
- `timer_setup()`, `mod_timer()`, and `timer_shutdown_sync()`;
- system workqueue use through `schedule_work()`;
- a condition-based wait queue;
- one consumable completion event;
- a spinlock for shared state and a mutex for list membership;
- producer-first, reverse-order teardown;
- an init-timeout error path after asynchronous objects are initialized.

## Kernel Version Assumptions

Build against the headers for the exact kernel that will load the module:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example targets **Linux 6.8 or newer** and uses:

- modern timer callbacks receiving `struct timer_list *`;
- `timer_shutdown_sync()` for final timer teardown;
- concurrency-managed system workqueues.

Some older kernels use `del_timer_sync()` instead of
`timer_shutdown_sync()`. Do not mechanically replace the call in a cyclic
timer/work design: unlike shutdown, deletion alone does not permanently reject
a later timer rearm. Check the target kernel's `include/linux/timer.h`.

## Files

| File | Purpose |
| --- | --- |
| `corefac_demo.c` | One-shot timer, workqueue, wait queue, completion, list, locking, and teardown demonstration. |
| `Makefile` | Out-of-tree Kbuild wrapper. |
| `README.md` | Build, load, test, debug, expected logs, ABI, and cleanup notes. |

No DTS is needed because the module does not bind to hardware. No userspace C
test is needed because loading parameters and observing kernel logs exercise the
entire example.

## Build

From this directory:

```sh
make
```

Equivalent command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For a target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Inspect the result:

```sh
modinfo ./corefac_demo.ko
modinfo -F vermagic ./corefac_demo.ko
modinfo -p ./corefac_demo.ko
```

Expected parameters:

```text
delay_ms:One-shot timer delay in milliseconds (uint)
skip_timer:Skip timer arming to exercise init-timeout cleanup (bool)
```

## Load And Test

Follow logs in one terminal:

```sh
sudo dmesg -C
sudo dmesg -w
```

Build and load in another:

```sh
make
sudo insmod ./corefac_demo.ko delay_ms=200
lsmod | grep corefac_demo
```

Expected log ordering:

```text
corefac_demo: loaded delay_ms=200 skip_timer=0 list_entries=1
corefac_demo: timer fired; queueing process-context work
corefac_demo: work published value=1 list_entries=1
corefac_demo: waiter consumed value=1
corefac_demo: init observed completion with <n> jiffies remaining
```

The final two lines may exchange order. The waiter and module init are released
by different mechanisms after the work callback publishes the event.

Unload:

```sh
sudo rmmod corefac_demo
dmesg | tail -20
```

Expected cleanup logs:

```text
corefac_demo: exit start
corefac_demo: unloaded list_entries=0
```

## Test The Error Path

`skip_timer=1` leaves the timer unarmed. Module init waits two seconds for a
completion that cannot arrive, then executes the same asynchronous teardown used
by normal unload:

```sh
sudo insmod ./corefac_demo.ko skip_timer=1
echo $?
dmesg | tail -30
lsmod | grep corefac_demo
```

Expected behavior:

- `insmod` fails with a timeout.
- The module is not left loaded.
- The waiter wakes because teardown publishes `stopping`.
- The list entry is removed before its enclosing object is freed.

Expected log shape:

```text
corefac_demo: loaded delay_ms=200 skip_timer=1 list_entries=1
corefac_demo: work completion timed out: -110
corefac_demo: waiter observed shutdown
```

`-110` is `-ETIMEDOUT`.

Also test parameter validation:

```sh
sudo insmod ./corefac_demo.ko delay_ms=0
echo $?
lsmod | grep corefac_demo
```

The load fails with `-EINVAL` before allocating asynchronous state.

## Debug

Useful commands:

```sh
dmesg | tail -50
journalctl -k -n 50
lsmod | grep corefac_demo
modinfo -p ./corefac_demo.ko
cat /proc/$(pgrep -f corefac_waiter)/stack 2>/dev/null
```

Trace workqueue activity when tracefs is available:

```sh
sudo mount -t tracefs nodev /sys/kernel/tracing 2>/dev/null || true
echo workqueue:workqueue_queue_work | \
  sudo tee /sys/kernel/tracing/set_event
sudo cat /sys/kernel/tracing/trace_pipe
```

Disable tracing afterward:

```sh
echo nop | sudo tee /sys/kernel/tracing/current_tracer
echo | sudo tee /sys/kernel/tracing/set_event
```

For lifetime and concurrency bugs, use a kernel built with suitable debugging:

- KASAN for use-after-free;
- lockdep for lock ordering and sleep-in-atomic mistakes;
- timer/work debug objects for invalid asynchronous object lifecycle;
- list debugging for double-add, double-delete, and corrupted links.

## Cleanup And Error Paths

`corefac_stop()` centralizes normal and failed-init cleanup:

```text
set stopping under the state lock
  -> timer_shutdown_sync()
  -> cancel_work_sync()
  -> wake condition waiters
  -> kthread_stop()
  -> list_del_init() under the list mutex
  -> caller frees the enclosing object
```

The order is intentional:

- The timer is a producer of work, so it is shut down before work is canceled.
- Synchronous shutdown/cancellation ensures callbacks no longer use the object.
- The waiter receives a terminal condition before `kthread_stop()` waits for it.
- The object is unlinked before `kfree()`.
- No cleanup function holds a lock needed by the timer, work, or waiter while
  waiting synchronously for that callback to finish.

The example uses the shared system workqueue, so it does not call
`destroy_workqueue()`. A driver must destroy only workqueues that it allocated.

## Userspace ABI Impact

There is no `/dev` node, ioctl, sysfs device attribute, procfs file, or debugfs
file. The module has only two load-time parameters and kernel log output.

Loaded module parameters appear under:

```text
/sys/module/corefac_demo/parameters/delay_ms
/sys/module/corefac_demo/parameters/skip_timer
```

They are read-only (`0444`) and are **demonstration controls, not a promised
stable userspace ABI**.

## Why This Is Not Production-Ready

The module is intentionally small and synthetic:

- It has no real device, IRQ, DMA engine, file operation, or subsystem owner.
- It runs one event and one waiter rather than supporting repeated operations.
- It uses global module state and a one-entry list.
- It does not demonstrate reference counting for open handles or external
  readers.
- The completion is not reused, so generation and `reinit_completion()` races
  are intentionally absent.
- The locking is sufficient for this flow, but it is not a reusable device
  state machine.

Production code should use subsystem-managed objects, define ownership for every
external reference, test remove/unbind under active I/O, and match the exact
timer/work APIs of its supported kernel versions.
