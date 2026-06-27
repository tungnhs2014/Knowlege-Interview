# 06 - Synchronization And Concurrency Basics Example

This is a **learning-only** kernel module. It creates no device node, sysfs file,
ioctl, procfs/debugfs entry, or stable userspace ABI. Module parameters only
control the demonstration.

## Goal

Two kthreads perform the same number of updates using three synchronization
models:

- a mutex protects `pair_left` and `pair_right` as one invariant;
- a spinlock protects a short, non-sleeping scalar update;
- `atomic64_t` protects one independent scalar counter.

```text
module init
  -> allocate and initialize all synchronization objects
  -> start two workers
workers
  -> update the mutex-protected pair
  -> update the spinlock-protected counter
  -> update the independent atomic counter
  -> signal when both loops finish
module init
  -> snapshot each protected state correctly
  -> verify every result against the expected total
module exit
  -> stop both parked workers
  -> free the containing object
```

The pair invariant is:

```text
pair_right == pair_left * 2
```

One lock covers both fields because separate atomic updates would not make the
pair visible as one coherent state transition.

## Kernel Version Assumptions

The example targets conventional Linux `6.8` kernel APIs and was designed for
the repository's available `6.8.0-124-generic` headers.

It uses:

- `mutex_init()`, `mutex_lock()`, and `mutex_unlock()`;
- `spin_lock_init()`, `spin_lock()`, and `spin_lock_irqsave()`;
- `atomic64_*()` and `atomic_dec_and_test()`;
- kthreads and a completion for the learning flow.

The example has no hard IRQ. `spin_lock_irqsave()` in the verification path is
included to show correct save/restore pairing, not because this module requires
IRQ exclusion. Real drivers should choose the narrowest correct spinlock variant
from the contexts that access the data.

## Files

| File | Purpose |
| --- | --- |
| `sync_demo.c` | Two-worker synchronization and invariant example |
| `Makefile` | Out-of-tree Kbuild wrapper |
| `README.md` | Build, test, debugging, and design notes |

## Build

```sh
make
```

Equivalent:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For another target:

```sh
make KDIR=/path/to/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Inspect the result:

```sh
modinfo ./sync_demo.ko
modinfo -p ./sync_demo.ko
```

Expected parameters:

```text
iterations:Updates performed by each worker (uint)
timeout_ms:Maximum wait for both workers in milliseconds (uint)
```

## Load And Test

Follow logs:

```sh
sudo dmesg -C
sudo dmesg -w
```

Load with defaults:

```sh
sudo insmod ./sync_demo.ko
lsmod | grep sync_demo
```

Expected log shape:

```text
sync_demo: expected=200000 mutex_pair=200000/400000 spin=200000 atomic=200000
sync_demo: loaded: workers=2 iterations=100000 timeout_ms=5000
```

Unload:

```sh
sudo rmmod sync_demo
dmesg | tail -20
```

Expected final line:

```text
sync_demo: unloaded
```

Run a smaller case:

```sh
sudo insmod ./sync_demo.ko iterations=1000 timeout_ms=1000
sudo rmmod sync_demo
```

The expected count is `2 * iterations`.

## Test Validation And Error Cleanup

Zero and over-limit iteration counts are rejected before allocation:

```sh
sudo insmod ./sync_demo.ko iterations=0
echo $?

sudo insmod ./sync_demo.ko iterations=1000001
echo $?
```

A very short timeout can exercise the init-time teardown path on a sufficiently
busy system:

```sh
sudo insmod ./sync_demo.ko iterations=1000000 timeout_ms=1
echo $?
lsmod | grep sync_demo
```

If the workers do not finish in time:

- module insertion returns `-ETIMEDOUT`;
- both worker threads are stopped;
- the allocated state is freed;
- the module is not left loaded.

Scheduling is nondeterministic, so a one-millisecond timeout is a stress control,
not a guaranteed failure on every machine.

## What To Observe

### Mutex-Protected Invariant

Both pair fields are updated while holding `pair_lock`. The critical section may
be used only from sleepable task context because mutex contention can sleep.

### Spinlock-Protected Counter

The spinlock section contains only one in-memory update. There is no allocation,
sleep, user copy, bus transaction, or logging while the lock is held.

### Atomic Counter

`atomic64_inc()` prevents lost updates to one scalar. It does not imply that an
atomic counter could replace `pair_lock`.

### Lifetime

Workers remain parked after finishing their loops. Module exit calls
`kthread_stop()` before freeing the state containing the mutex and spinlock.

## Debug

Basic inspection:

```sh
dmesg | tail -50
journalctl -k -n 50
ps -e -o pid,comm | grep sync_demo
```

Useful debug kernel options:

- `CONFIG_PROVE_LOCKING` and lockdep for invalid lock dependencies;
- `CONFIG_DEBUG_MUTEXES` and spinlock debugging;
- KCSAN for sampled data races;
- KASAN for lifetime mistakes;
- hung-task and soft-lockup detectors for deadlock or excessive atomic work.

This correct example should not produce lockdep or KCSAN reports. To learn from a
real report, test driver changes on a disposable debug kernel rather than adding
undefined unlocked concurrent accesses to this module.

## Why This Is Not Production-Ready

- It has no hardware, driver-model binding, or userspace purpose.
- Global module state is sufficient only because exactly one demo instance exists.
- It uses kthreads solely to create controlled contention.
- It does not benchmark lock performance.
- It does not demonstrate hard-IRQ sharing; topic 15 applies the same reasoning
  to real interrupt paths.
- Production code would define lock scope beside real device fields, integrate
  subsystem lifetime rules, and test actual error/remove races.

## Cleanup Rule

The important order is:

```text
stop every thread that can touch the object
  -> free the object containing its synchronization primitives
```

A mutex or spinlock cannot protect storage after that storage has been freed.
