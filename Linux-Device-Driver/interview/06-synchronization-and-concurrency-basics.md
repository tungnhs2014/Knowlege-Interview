# 06 - Synchronization And Concurrency Basics Interview Questions

Strong candidates do not choose locks from slogans. They identify the protected
invariant, enumerate every execution context, account for sleepability and
lifetime, and can explain how they would prove or debug the result.

## Beginner

### 1. What Is A Race Condition In A Driver?

- **Level:** Beginner
- **Question:** What is a race condition, and why can it be difficult to reproduce?
- **Short Answer:** A race condition exists when correctness depends on an
  uncontrolled execution ordering. It may appear only under a rare interleaving,
  CPU placement, interrupt timing, or teardown window.
- **Deep Explanation:** Two paths may both be individually valid but invalid
  when interleaved. Examples include lost `counter++` updates, checking a flag
  before another path frees the object, or publishing one field before the
  related fields are ready. Extra logging can change scheduling enough to hide
  the failure.
- **API / Code Anchor:**
  ```c
  /* Not safe when concurrent writers exist. */
  d->count++;
  ```
- **Production or Debugging Angle:** Inventory all readers and writers, including
  IRQs, timers, work, file operations, error paths, and remove. KCSAN can help
  detect sampled data races, while KASAN is useful when the race becomes a
  use-after-free.
- **Common Traps:**
  - Assuming a single C statement is atomic.
  - Testing only on one CPU.
  - Forgetting teardown is a concurrent path.
  - Calling every timing bug a data race without identifying conflicting access.
- **Follow-up Questions:**
  - What is the difference between a race condition and a data race?
  - Why can logging make a race disappear?

### 2. How Do You Choose Between A Mutex And A Spinlock?

- **Level:** Beginner
- **Question:** What is the primary decision rule for mutex versus spinlock?
- **Short Answer:** Choose from execution context and sleepability. Use a mutex
  for task-only state when protected work may sleep; use a spinlock for short,
  non-sleeping state reachable from atomic context.
- **Deep Explanation:** Fixed time thresholds are not the decision rule. A
  hard-IRQ handler cannot take a mutex even if the critical section is tiny. A
  process-only operation that performs I2C I/O needs a sleepable design even if
  it is usually fast. Every accessor must fit the selected primitive.
- **API / Code Anchor:**
  ```c
  mutex_lock(&d->config_lock);
  ret = i2c_smbus_write_byte_data(d->client, reg, value);
  mutex_unlock(&d->config_lock);
  ```
- **Production or Debugging Angle:** Build a context table for each protected
  field. Unknown callbacks and subsystem helpers must be checked for sleepability
  before placing them under a spinlock.
- **Common Traps:**
  - "Spinlock for less than 100 microseconds."
  - "Mutex for process context" without checking whether an IRQ also accesses it.
  - Assuming a spinlock makes slow operations acceptable.
- **Follow-up Questions:**
  - Can workqueue callbacks take a mutex?
  - Can process context take a spinlock?

### 3. Why Can You Not Sleep While Holding A Spinlock?

- **Level:** Beginner
- **Question:** Why are sleeping operations forbidden under a conventional spinlock?
- **Short Answer:** The lock represents atomic execution constraints; sleeping
  can leave the lock held while other contexts spin or while scheduling is
  disabled, causing invalid-context reports or deadlock.
- **Deep Explanation:** On a non-`PREEMPT_RT` kernel, a normal spinlock disables
  local preemption. IRQ variants also disable local interrupts. Contenders cannot
  make useful progress if the owner blocks. Operations such as mutex acquisition,
  `GFP_KERNEL` allocation, user copy, and many bus calls may sleep.
- **API / Code Anchor:**
  ```c
  spin_lock(&d->lock);
  snapshot = d->status;
  spin_unlock(&d->lock);

  copy_to_user(buf, &snapshot, sizeof(snapshot));
  ```
- **Production or Debugging Angle:** A `sleeping function called from invalid
  context` report should be followed from the sleep site upward to the lock or
  atomic callback that established the invalid context.
- **Common Traps:**
  - Moving only the explicit `msleep()` while leaving another sleepable helper.
  - Replacing all allocation with `GFP_ATOMIC` instead of moving it outside.
  - Calling `copy_to_user()` under a spinlock.
- **Follow-up Questions:**
  - What changes on `PREEMPT_RT`?
  - Which GPIO APIs may sleep?

### 4. What Are Atomic Operations Good For?

- **Level:** Beginner
- **Question:** When is `atomic_t` appropriate, and what does it not solve?
- **Short Answer:** It is appropriate for an independent scalar operation such
  as an increment or compare-exchange. It does not automatically protect
  surrounding fields, object lifetime, or a multi-field invariant.
- **Deep Explanation:** `atomic_inc()` prevents lost updates to that atomic
  counter. If `head`, `tail`, and `count` must describe one queue state, making
  each atomic does not make the combined transition indivisible. Memory ordering
  is also a separate design concern.
- **API / Code Anchor:**
  ```c
  atomic64_inc(&d->irq_count);
  ```
- **Production or Debugging Angle:** Treat atomics as a deliberate data-model
  choice, not a faster generic lock. For object references, prefer `refcount_t`.
- **Common Traps:**
  - One atomic per related field.
  - Using generic atomics for reference counts.
  - Assuming `atomic_read()` is a complete publication protocol.
- **Follow-up Questions:**
  - Why is `refcount_t` preferred for object lifetime?
  - What is compare-exchange used for?

## Mid-Level

### 5. Why Use `spin_lock_irqsave()` For Process And IRQ Sharing?

- **Level:** Mid
- **Question:** Why can plain `spin_lock()` deadlock when process context and a
  hard-IRQ handler use the same lock?
- **Short Answer:** A local IRQ can preempt the task while it holds the lock and
  then spin forever trying to acquire the same lock. The task cannot resume to
  release it.
- **Deep Explanation:** `spin_lock_irqsave()` saves the caller's local IRQ state,
  disables local hard IRQs, and acquires the lock. The matching restore releases
  the lock and restores the previous state. The lock still provides inter-CPU
  exclusion.
- **API / Code Anchor:**
  ```c
  unsigned long flags;

  spin_lock_irqsave(&d->lock, flags);
  d->pending = false;
  spin_unlock_irqrestore(&d->lock, flags);
  ```
- **Production or Debugging Angle:** Use it only when the data is actually shared
  with hard IRQ context. Unnecessary IRQ disabling increases interrupt latency.
- **Common Traps:**
  - Pairing `_irqsave` with plain `spin_unlock()`.
  - Sharing one `flags` variable across concurrent calls.
  - Believing local IRQ disable alone protects against other CPUs.
- **Follow-up Questions:**
  - When is `spin_lock_bh()` appropriate?
  - What does the IRQ handler itself normally use for the same lock?

### 6. How Should `mutex_lock_interruptible()` Errors Be Handled?

- **Level:** Mid
- **Question:** What must happen after `mutex_lock_interruptible()` returns nonzero?
- **Short Answer:** The lock was not acquired. The path must not enter the
  critical section or unlock the mutex; it should propagate or intentionally
  translate the error.
- **Deep Explanation:** A signal can interrupt lock acquisition. This is useful
  in userspace-facing operations where cancellation matters. Plain
  `mutex_lock()` is still normal for internal paths that must complete and have
  no meaningful signal-return contract.
- **API / Code Anchor:**
  ```c
  ret = mutex_lock_interruptible(&d->lock);
  if (ret)
          return ret;

  ret = update_state(d);
  mutex_unlock(&d->lock);
  ```
- **Production or Debugging Angle:** Review all early returns to prove that each
  successful acquisition has one release and failed acquisition has none.
- **Common Traps:**
  - Always translating the result to `-ERESTARTSYS`.
  - Continuing as though the lock was acquired.
  - Unconditionally unlocking in one cleanup label.
- **Follow-up Questions:**
  - When would `mutex_lock_killable()` be useful?
  - Why might a kernel thread use plain `mutex_lock()`?

### 7. Why Is `mutex_trylock()` Not An IRQ-Safe Shortcut?

- **Level:** Mid
- **Question:** Since `mutex_trylock()` returns immediately, can it be used in a
  hard-IRQ handler?
- **Short Answer:** No. A mutex remains a sleeping-lock primitive with owner and
  debugging semantics; trylock does not turn it into an IRQ-safe lock.
- **Deep Explanation:** Context rules belong to the lock type, not only to whether
  the contended acquisition sleeps in one implementation. Unlock behavior,
  lockdep/debug configurations, and `PREEMPT_RT` rules must also remain valid.
- **API / Code Anchor:**
  ```c
  /* Process context only. */
  if (mutex_trylock(&d->config_lock)) {
          inspect_config(d);
          mutex_unlock(&d->config_lock);
  }
  ```
- **Production or Debugging Angle:** If an IRQ needs the same state, redesign the
  protection around a spinlock or split fast IRQ-owned state from sleepable
  configuration state.
- **Common Traps:**
  - "It does not wait, so it cannot sleep."
  - Mixing mutex and spinlock protection for the same field.
  - Silently dropping required IRQ work when trylock fails.
- **Follow-up Questions:**
  - When is trylock a valid design choice?
  - How would a threaded IRQ change the options?

### 8. How Do You Protect A Multi-Field Invariant?

- **Level:** Mid
- **Question:** Why are three atomic variables not necessarily enough for a
  three-field state?
- **Short Answer:** Each operation may be atomic while readers still observe a
  mixture of old and new fields. Protect the complete transition with one lock
  or a rigorously designed sequence protocol.
- **Deep Explanation:** If `available`, `owner`, and `generation` describe one
  ownership state, a reader must not observe only part of the update. A mutex or
  spinlock serializes both the values and their relationship.
- **API / Code Anchor:**
  ```c
  mutex_lock(&d->state_lock);
  d->owner = new_owner;
  d->generation++;
  d->available = true;
  mutex_unlock(&d->state_lock);
  ```
- **Production or Debugging Angle:** Add an invariant assertion under the same
  lock and document all fields it protects. KCSAN may not detect a logical race
  when every individual access is marked atomic.
- **Common Traps:**
  - Confusing no torn access with coherent state.
  - Reading one field outside the lock as a "fast check."
  - Splitting one update across several lock acquisitions.
- **Follow-up Questions:**
  - When might a seqcount be suitable?
  - What ordering does lock acquisition/release provide?

### 9. What Are `READ_ONCE()` And `WRITE_ONCE()` For?

- **Level:** Mid
- **Question:** Do `READ_ONCE()` and `WRITE_ONCE()` replace a lock?
- **Short Answer:** No. They mark one access and constrain compiler behavior.
  They do not provide mutual exclusion or automatically order surrounding state.
- **Deep Explanation:** They are building blocks for carefully designed
  concurrent protocols and can prevent access merging or tearing where their
  contract applies. A publication protocol may additionally require
  acquire/release operations or locks.
- **API / Code Anchor:**
  ```c
  if (READ_ONCE(d->stopping))
          return;
  ```
- **Production or Debugging Angle:** Require a written explanation of the
  protocol whenever marked accesses replace normal lock coverage. A comment
  should identify the paired writer and ordering rule.
- **Common Traps:**
  - Using `READ_ONCE()` to make object lifetime safe.
  - Assuming `WRITE_ONCE(flag, 1)` publishes prior writes on every architecture.
  - Applying it randomly to silence KCSAN.
- **Follow-up Questions:**
  - What is release/acquire ordering?
  - When is a lock clearer?

### 10. How Do You Prevent ABBA Deadlock?

- **Level:** Mid
- **Question:** What is ABBA deadlock and how do you prevent it?
- **Short Answer:** One path holds A then waits for B while another holds B then
  waits for A. Define and obey one global lock order.
- **Deep Explanation:** Nested locks create dependency edges. Opposite order
  creates a cycle even if each local path looks correct. Helpers should document
  whether the caller holds a lock, and locked/unlocked helper variants can make
  ownership explicit.
- **API / Code Anchor:**
  ```text
  Required order: device_list_lock -> device->state_lock
  Forbidden:      device->state_lock -> device_list_lock
  ```
- **Production or Debugging Angle:** Enable lockdep and inspect the reported
  dependency chain, not just the final acquisition site. Avoid fixing the report
  by disabling validation.
- **Common Traps:**
  - Reviewing functions independently.
  - Hidden lock acquisition in callbacks or subsystem helpers.
  - Dropping and reacquiring locks without revalidating state.
- **Follow-up Questions:**
  - What is recursive deadlock?
  - How can `lockdep_assert_held()` help?

### 11. How Do You Protect A Wait-Queue Predicate?

- **Level:** Mid
- **Question:** A reader sleeps until `d->data_ready` becomes true. What must the
  producer and waiter do to avoid lost wakeups and inconsistent payload?
- **Short Answer:** Treat the predicate and the data it describes as one
  synchronization protocol. Update them under the same lock, wake after
  publishing the state, and have the waiter recheck the predicate in a
  `wait_event*()` loop before consuming the payload under the agreed lock.
- **Deep Explanation:** A wakeup is only a notification; it does not reserve
  data, evaluate the condition for the waiter, or by itself publish a
  multi-field payload. The waiter can wake spuriously, lose a race to another
  consumer, or observe teardown. Its predicate must therefore be safe to
  reevaluate and must include terminal states such as `stopping` when shutdown
  must release sleepers.
- **API / Code Anchor:**
  ```c
  /* Producer */
  spin_lock_irqsave(&d->lock, flags);
  d->sample = sample;
  WRITE_ONCE(d->data_ready, true);
  spin_unlock_irqrestore(&d->lock, flags);
  wake_up_interruptible(&d->readq);

  /* Waiter */
  ret = wait_event_interruptible(d->readq,
          READ_ONCE(d->data_ready) || READ_ONCE(d->stopping));
  if (ret)
          return ret;

  spin_lock_irqsave(&d->lock, flags);
  if (d->stopping) {
          ret = -ENODEV;
  } else if (!d->data_ready) {
          ret = -EAGAIN; /* Another consumer won; retry in the real loop. */
  } else {
          sample = d->sample;
          WRITE_ONCE(d->data_ready, false);
          ret = 0;
  }
  spin_unlock_irqrestore(&d->lock, flags);
  ```
- **Production or Debugging Angle:** When a read occasionally hangs, trace every
  predicate transition and wakeup, then check whether the producer published
  state before waking and whether another consumer can consume it. During
  remove, set the terminal predicate and wake sleepers before waiting for their
  references to drain.
- **Common Traps:**
  - Treating `wake_up()` as data synchronization or data ownership transfer.
  - Checking the condition once and then sleeping manually.
  - Protecting the ready flag and payload with different rules.
  - Forgetting that multiple readers may race after the same wakeup.
  - Freeing the wait queue's containing object while waiters still reference it.
- **Follow-up Questions:**
  - Why must the condition be reevaluated after every wakeup?
  - When would a completion be a better fit than a wait queue?

### 12. How Are Atomicity And Memory Ordering Different?

- **Level:** Mid
- **Question:** If an atomic flag is set after filling a buffer, is a reader that
  sees the flag guaranteed to see the completed buffer?
- **Short Answer:** Not from atomicity alone. The protocol needs ordering, such
  as lock acquire/release or a documented release-store and acquire-load pair.
- **Deep Explanation:** Atomicity prevents one operation from being observed
  halfway through; ordering controls how surrounding accesses become visible.
  A relaxed atomic flag can be indivisible while the payload publication remains
  incorrectly ordered on a weakly ordered architecture. Locks are normally the
  clearest answer for driver control state because lock release publishes prior
  protected writes and lock acquisition orders later protected reads.
- **API / Code Anchor:**
  ```c
  /* Preferred basic-driver design: one lock protects flag and payload. */
  spin_lock(&d->lock);
  d->payload = value;
  d->ready = true;
  spin_unlock(&d->lock);
  ```
- **Production or Debugging Angle:** Review lockless publication as a protocol,
  not as isolated API calls. Require the writer, reader, ordering pair, lifetime
  rule, and retry behavior to be documented; test with KCSAN, but do not treat a
  clean run as a proof of Linux Kernel Memory Model correctness.
- **Common Traps:**
  - Assuming `atomic_set()` publishes unrelated writes.
  - Using `volatile` for inter-CPU ordering.
  - Reasoning only from x86 behavior.
  - Adding barriers without identifying the operation they pair with.
- **Follow-up Questions:**
  - What ordering do lock acquire and release provide?
  - When would `smp_store_release()` and `smp_load_acquire()` be appropriate?

## Senior

### 13. Why Are Synchronization And Lifetime Separate Problems?

- **Level:** Senior
- **Question:** If every field is protected by a lock, why can remove still cause
  use-after-free?
- **Short Answer:** A lock serializes access to live memory; it does not guarantee
  the containing object remains alive before acquisition or after release.
- **Deep Explanation:** A timer, IRQ, worker, file descriptor, or subsystem
  callback may retain a pointer after remove starts. Teardown must stop new
  producers, synchronize existing users, remove visibility, and release
  references before freeing the object and its locks.
- **API / Code Anchor:**
  ```text
  disable producer
    -> synchronize/cancel callbacks
    -> wait for references
    -> free state
  ```
- **Production or Debugging Angle:** KASAN often reports the dereference, but the
  root cause is an earlier missing lifetime barrier. Audit every pointer holder
  and asynchronous source.
- **Common Traps:**
  - Freeing immediately after setting `stopping`.
  - Taking a lock through a pointer that may already be freed.
  - Assuming devm cleanup stops custom asynchronous work.
- **Follow-up Questions:**
  - How do `refcount_t` and completions solve different parts?
  - What producer-first teardown would you use for IRQ plus work?

### 14. How Would You Split Fast And Sleepable State?

- **Level:** Senior
- **Question:** A hard IRQ captures status, while a worker performs I2C and updates
  configuration. How would you structure locking?
- **Short Answer:** Keep a small IRQ-shared state protected by a spinlock, snapshot
  or enqueue work, then use a mutex for sleepable configuration in process
  context. Define the order if both locks are ever needed.
- **Deep Explanation:** Do not stretch one spinlock across I2C, and do not expose
  a mutex to hard IRQ. Often the IRQ acknowledges hardware, records event bits
  under the spinlock, and queues work. The worker drains event state and performs
  sleepable operations under the configuration mutex.
- **API / Code Anchor:**
  ```c
  spin_lock(&d->event_lock);
  d->pending_events |= status;
  spin_unlock(&d->event_lock);
  schedule_work(&d->work);
  ```
- **Production or Debugging Angle:** Account for work coalescing, events arriving
  while work runs, and remove stopping the IRQ before canceling the work.
- **Common Traps:**
  - Worker takes mutex then spinlock while another path takes the reverse order.
  - One work item is treated as an event counter.
  - IRQ requeues work after teardown cancellation.
- **Follow-up Questions:**
  - How would you drain pending event bits without losing new events?
  - Which lock protects the stopping flag?

### 15. When Would You Reject A Lockless Design?

- **Level:** Senior
- **Question:** A patch replaces a lock with atomics and barriers for speed. What
  evidence should be required?
- **Short Answer:** Require a measured bottleneck, a documented state machine and
  memory-ordering proof, lifetime handling, architecture-independent reasoning,
  and stress/sanitizer testing. Otherwise keep the lock.
- **Deep Explanation:** Lockless code transfers complexity from an obvious
  serialization primitive into every access and ordering edge. It can be correct,
  but maintenance and review cost are high. Normal driver control paths rarely
  justify it without profiling.
- **API / Code Anchor:**
  ```text
  benchmark -> protocol -> LKMM reasoning -> tests -> review
  ```
- **Production or Debugging Angle:** Use KCSAN and targeted stress, but remember
  dynamic tools cannot prove all allowed weak-memory executions.
- **Common Traps:**
  - Optimizing before measuring.
  - Equating x86 behavior with the kernel memory model.
  - Using `volatile`.
  - Treating a clean KCSAN run as proof.
- **Follow-up Questions:**
  - When would RCU or seqcount be a better established primitive?
  - What does `cmpxchg` guarantee?

### 16. How Does `PREEMPT_RT` Affect Locking Assumptions?

- **Level:** Senior
- **Question:** Why should portable driver guidance mention `PREEMPT_RT`?
- **Short Answer:** On `PREEMPT_RT`, `spinlock_t` is commonly implemented with
  RT-mutex-based semantics and IRQ suffix behavior differs; `raw_spinlock_t`
  remains the strict low-level spinning primitive.
- **Deep Explanation:** Code must follow documented lock-type nesting and context
  rules rather than depend on one non-RT implementation detail. Drivers should
  not switch to raw spinlocks casually because that harms latency and is intended
  for truly raw contexts and low-level hardware state. Most device interrupts
  are forced-threaded on RT. Code that genuinely remains in hard-IRQ context
  cannot acquire an RT-mutex-backed `spinlock_t` and needs a primitive legal in
  that context, commonly a narrowly scoped `raw_spinlock_t`.
- **API / Code Anchor:**
  ```text
  sleeping locks -> spinlock_t/local_lock class -> raw_spinlock_t
  ```
- **Production or Debugging Angle:** Test latency-sensitive drivers on the actual
  RT configuration and review raw-lock usage closely.
- **Common Traps:**
  - Assuming `spinlock_t` always disables preemption in the same way.
  - Replacing every spinlock with `raw_spinlock_t`.
  - Using an RT-mutex-backed `spinlock_t` from a true hard-IRQ path.
  - Ignoring lock nesting differences.
- **Follow-up Questions:**
  - When is a raw spinlock justified?
  - How can threaded IRQs improve RT behavior?

## Debugging Scenarios

### 17. Diagnose A Sleep-In-Atomic And Teardown Deadlock Report

- **Level:** Senior / Debugging
- **Question:** Remove holds `state_lock`, calls `cancel_work_sync()`, and hangs.
  The worker also takes `state_lock` and performs `copy_to_user()` under a
  spinlock elsewhere. What are the bugs and fixes?
- **Short Answer:** Remove waits for work while holding the lock the worker needs,
  creating deadlock. User copy may sleep and cannot run under a spinlock. Stop
  producers first, call synchronous cancellation without callback-needed locks,
  and snapshot protected data before user copy.
- **Deep Explanation:** Teardown dependency is:
  `remove(state_lock) -> cancel_work_sync() -> worker(state_lock)`. Neither side
  can progress. Separately, the user-copy path enters atomic context through the
  spinlock and may fault. Correct design minimizes the spinlocked state update,
  releases it, then performs the sleepable operation.
- **API / Code Anchor:**
  ```c
  WRITE_ONCE(d->stopping, true);
  disable_irq(d->irq);
  cancel_work_sync(&d->work);

  spin_lock_irqsave(&d->state_lock, flags);
  snapshot = d->value;
  spin_unlock_irqrestore(&d->state_lock, flags);

  if (copy_to_user(buf, &snapshot, sizeof(snapshot)))
          return -EFAULT;
  ```
- **Production or Debugging Angle:** Lockdep can expose the dependency cycle;
  invalid-context diagnostics identify the user copy. Verify that no IRQ can
  requeue work after cancellation and that object lifetime extends through open
  users.
- **Common Traps:**
  - Moving `cancel_work_sync()` but leaving a timer or IRQ producer live.
  - Replacing user copy with an atomic allocation.
  - Reading the snapshot after releasing the lock instead of copying it while
    protected.
- **Follow-up Questions:**
  - Where should the stopping flag be protected?
  - How would the answer change for a threaded IRQ?

### 18. Diagnose Lockdep And KCSAN Reports On The Same Driver

- **Level:** Senior / Debugging
- **Question:** Lockdep reports a possible circular dependency between
  `config_lock` and `event_lock`, while KCSAN reports a race on `d->stopping`.
  How do you investigate without merely suppressing either report?
- **Short Answer:** Treat them as different evidence. Reconstruct the lock-order
  cycle from lockdep's acquisition chains, then inventory every access to
  `stopping` and define one synchronization rule for it. Fix the design and use
  assertions or marked lockless accesses only when they express a real,
  documented protocol.
- **Deep Explanation:** Lockdep reasons about observed lock-class dependency
  edges: one path acquired the mutex before the spinlock and another established
  the reverse edge. KCSAN samples conflicting memory accesses and can reveal a
  missing lock even when no deadlock occurs. The two reports may share a root
  cause, such as an unclear ownership split, but neither proves the other's
  diagnosis. Capture complete stacks, identify callback contexts, and decide
  whether `stopping` belongs under one existing lock or needs a carefully
  ordered lockless publication.
- **API / Code Anchor:**
  ```c
  void update_config_locked(struct mydev *d)
  {
          lockdep_assert_held(&d->config_lock);
          /* Must not acquire event_lock if global order is event -> config. */
  }
  ```
  ```text
  Useful configurations:
    CONFIG_LOCKDEP=y
    CONFIG_PROVE_LOCKING=y
    CONFIG_KCSAN=y
  ```
- **Production or Debugging Angle:** Reproduce with the smallest stress workload
  that exercises IRQ, worker, userspace, and remove paths. Keep the first full
  warning because later reports may be consequences. After the fix, rerun both
  configurations and inspect for invalid-context, hung-task, and use-after-free
  reports as well.
- **Common Traps:**
  - Disabling lockdep or adding `lockdep_off()` to make the warning disappear.
  - Sprinkling `READ_ONCE()` and `WRITE_ONCE()` only to silence KCSAN.
  - Looking only at the final lock acquisition instead of the dependency chain.
  - Assuming KCSAN finds every race or proves a clean run race-free.
  - Changing lock order in one function without auditing callbacks and cleanup.
- **Follow-up Questions:**
  - What is a lock class, and when are separate classes needed?
  - How would KASAN complement KCSAN in a teardown race?

### 19. Diagnose An IRQ-Safe Lock That Still Deadlocks

- **Level:** Senior / Debugging
- **Question:** A PCI driver uses plain `spin_lock()` in two MSI-X handlers that
  share one device lock. Each vector cannot reenter itself, so why can the
  machine still hang, and what is the likely fix?
- **Short Answer:** A different local vector can interrupt the first handler
  while it holds the shared lock and then spin on that same lock. Disable local
  IRQs while holding the shared lock, normally with
  `spin_lock_irqsave()`/`spin_unlock_irqrestore()`, or redesign the vectors not
  to share that lock.
- **Deep Explanation:** Per-vector non-reentrancy does not prevent cross-vector
  nesting on one CPU. If vector A holds the lock and vector B interrupts it,
  vector A cannot resume to release the lock. The `_irqsave` variant also
  preserves callers that may already have interrupts disabled and still
  provides inter-CPU exclusion.
- **API / Code Anchor:**
  ```c
  irqreturn_t my_irq(int irq, void *arg)
  {
          struct mydev *d = arg;
          unsigned long flags;

          spin_lock_irqsave(&d->irq_lock, flags);
          service_vector(d, irq); /* Must not sleep. */
          spin_unlock_irqrestore(&d->irq_lock, flags);
          return IRQ_HANDLED;
  }
  ```
- **Production or Debugging Angle:** Use NMI watchdog or lockup stacks to find
  CPUs spinning in the same lock, then map all interrupt vectors and process
  paths that acquire it. Measure IRQ-off hold time with `irqsoff` tracing after
  the correctness fix; a correct but oversized critical section can still cause
  unacceptable latency.
- **Common Traps:**
  - Assuming one device cannot interrupt itself through another vector.
  - Pairing `spin_lock_irqsave()` with the wrong unlock variant.
  - Calling a sleepable register or bus helper while IRQs are disabled.
  - Using `_irqsave` everywhere without documenting why IRQ exclusion is needed.
  - Forgetting that a device or DMA engine is not serialized by a CPU lock.
- **Follow-up Questions:**
  - When can the IRQ handler use plain `spin_lock()` for shared state?
  - How would threaded IRQ handlers change the locking choices?
