# 05 - Core Kernel Facilities Interview Questions

Core kernel facilities reveal whether a candidate can connect data structure ownership, execution context, asynchronous work, sleeping, and object lifetime. Strong candidates do more than name APIs: they explain who owns each embedded object, which context executes each callback, what synchronizes shared state, and how teardown prevents late access to freed memory.

## Beginner

### 1. Why Does The Kernel Use `container_of()`?

- **Level:** Beginner
- **Question:** What does `container_of()` do, and why is object embedding common in kernel code?
- **Short Answer:** `container_of()` recovers the address of a containing structure from a pointer to one of its embedded members. It lets generic kernel facilities operate on a standard member while a callback recovers the driver-specific object that owns it.
- **Deep Explanation:** A driver commonly embeds `struct list_head`, `struct work_struct`, `struct timer_list`, or another subsystem object in its private structure. The kernel receives a pointer to that embedded member because it understands the generic type, not the driver's private type. Since the member's offset within the containing type is known at compile time, `container_of()` subtracts that offset and returns the enclosing object. This intrusive design avoids a separate wrapper allocation and gives one object a clear relationship to its callback and lifecycle state.
- **API / Code Anchor:**
  ```c
  struct demo {
          struct work_struct work;
          int value;
  };

  static void demo_work(struct work_struct *work)
  {
          struct demo *d = container_of(work, struct demo, work);

          process_value(d->value);
  }
  ```
- **Production or Debugging Angle:** If the containing type or member name is wrong, the result points at unrelated memory and corruption may appear far from the callback. Prefer typed wrappers such as `from_timer()` and `to_delayed_work()` where appropriate because they make intent clearer.
- **Common Traps:**
  - Passing the pointee stored in a pointer member instead of the address of the embedded member.
  - Naming the wrong containing type or member.
  - Believing `container_of()` validates object lifetime.
  - Assuming embedding supplies synchronization or a reference count.
- **Follow-up Questions:**
  - How does `list_for_each_entry()` use the same idea?
  - What does `from_timer()` add over a raw `container_of()` call?
  - What happens if the containing object has already been freed?

### 2. Why Are Linux Kernel Lists Called Intrusive?

- **Level:** Beginner
- **Question:** How does `struct list_head` differ from a conventional list node that stores a data pointer?
- **Short Answer:** The list link is embedded directly in the object being linked. The list stores `struct list_head` links, and traversal recovers each containing object.
- **Deep Explanation:** A `struct list_head` has forward and backward links and forms a circular doubly linked list. The list head is also a `struct list_head`; an empty list points back to itself. Any structure can join one or more lists by embedding one distinct list member per membership. `list_add()` inserts near the head, while `list_add_tail()` inserts at the tail. The list API manages links only; allocation, object lifetime, ordering policy, and concurrency remain the caller's responsibility.
- **API / Code Anchor:**
  ```c
  struct demo_dev {
          struct list_head node;
          int id;
  };

  LIST_HEAD(device_list);

  INIT_LIST_HEAD(&d->node);
  list_add_tail(&d->node, &device_list);

  list_for_each_entry(d, &device_list, node)
          use_device(d);
  ```
- **Production or Debugging Angle:** Define which lock or ownership rule protects every list. Remove an object from all lists before freeing it, and use `list_for_each_entry_safe()` when the current entry may be deleted during traversal.
- **Common Traps:**
  - Forgetting `INIT_LIST_HEAD()` for a dynamically initialized head or reusable node.
  - Adding the same node twice.
  - Using ordinary traversal while deleting the current entry.
  - Assuming a list protects readers from concurrent deletion.
  - Freeing an object while its node remains linked.
- **Follow-up Questions:**
  - What is the difference between `list_del()` and `list_del_init()`?
  - Why can one object need multiple `list_head` members?
  - Which synchronization strategy would you use for a list touched by IRQ and process context?

### 3. Compare A Timer, Delayed Work, And An Hrtimer

- **Level:** Beginner
- **Question:** When would you choose a standard timer, delayed work, or an hrtimer?
- **Short Answer:** Use a standard timer for jiffies-based deferred callbacks that must not sleep, delayed work when the delayed callback needs process context or may sleep, and an hrtimer only when higher-resolution timing is genuinely required.
- **Deep Explanation:** A standard timer schedules a callback for a future jiffies deadline and runs in atomic timer context. Delayed work uses a timer-like delay to queue a work item, whose function later runs in process context. An hrtimer uses high-resolution time and also has atomic-context callback restrictions. None provides a hard real-time guarantee: scheduling and system load can delay execution. An IRQ or device event is usually preferable to periodic polling when hardware supports it.
- **API / Code Anchor:**
  ```c
  timer_setup(&d->timer, demo_timer_fn, 0);
  mod_timer(&d->timer, jiffies + msecs_to_jiffies(100));

  INIT_DELAYED_WORK(&d->dwork, demo_work_fn);
  schedule_delayed_work(&d->dwork, msecs_to_jiffies(100));

  hrtimer_init(&d->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  hrtimer_start(&d->hrtimer, ms_to_ktime(1), HRTIMER_MODE_REL);
  ```
- **Production or Debugging Angle:** Audit every callback for operations that can sleep. A timer should usually capture minimal state and queue work when slow I/O, a mutex, or a sleeping bus/GPIO operation is needed.
- **Common Traps:**
  - Sleeping or taking a mutex in a timer or hrtimer callback.
  - Assuming an hrtimer makes a driver deterministic.
  - Comparing jiffies with ordinary relational operators instead of wrap-safe helpers.
  - Using obsolete `setup_timer()` or callback data arguments in modern code.
- **Follow-up Questions:**
  - Why is `time_after()` safer than `expires > jiffies`?
  - How would you implement a periodic timer without accumulating drift?
  - Why might a hardware PWM framework be better than an hrtimer callback?

### 4. Compare Wait Queues And Completions

- **Level:** Beginner
- **Question:** What is the conceptual difference between a wait queue and a completion?
- **Short Answer:** A wait queue blocks until a condition becomes true, often repeatedly. A completion signals that an asynchronous operation or lifecycle milestone has occurred.
- **Deep Explanation:** A wait queue is paired with caller-owned state such as `data_ready`. `wait_event*()` checks that condition, sleeps if false, and checks it again after waking. `wake_up*()` makes eligible waiters runnable; it does not evaluate their condition for them. A completion packages an event counter and wait queue for event-style synchronization. Each `complete()` contributes one consumable completion event, while `complete_all()` releases all current and future waiters until the completion is safely reinitialized.
- **API / Code Anchor:**
  ```c
  ret = wait_event_interruptible(d->readq,
                                 READ_ONCE(d->data_ready) ||
                                 READ_ONCE(d->stopping));
  if (ret)
          return ret;

  wait_for_completion(&d->dma_done);

  /* Producer paths */
  wake_up_interruptible(&d->readq);
  complete(&d->dma_done);
  ```
- **Production or Debugging Angle:** Use a wait queue when correctness depends on protected shared state. Use a completion for a discrete handoff such as DMA finished or a worker reached a shutdown point. In both cases, storage and all participants must remain alive until waiting and signaling are finished.
- **Common Traps:**
  - Treating `wake_up()` as saved event state.
  - Assuming one `complete()` permanently leaves a completion signaled.
  - Ignoring the return from an interruptible wait.
  - Calling a sleeping wait API from IRQ or other atomic context.
- **Follow-up Questions:**
  - Can `complete()` run in IRQ context?
  - Can `wait_for_completion()` run in IRQ context?
  - When is `complete_all()` appropriate, and how is it reused safely?

## Mid-Level

### 5. How Do You Delete Entries Safely While Traversing A List?

- **Level:** Mid
- **Question:** Why is `list_for_each_entry_safe()` needed when deleting the current entry?
- **Short Answer:** It saves the next entry before the current node is unlinked or freed, so traversal does not dereference links from an invalid current object.
- **Deep Explanation:** Normal traversal obtains the next link through the current node. After `list_del()` and especially after `kfree()`, those links are no longer valid traversal state. The safe iterator keeps a second cursor for the next object. "Safe" means safe against deletion by that loop, not safe against concurrent access; callers still need locking, exclusion, RCU, or another ownership scheme.
- **API / Code Anchor:**
  ```c
  struct demo_dev *d, *tmp;

  mutex_lock(&device_lock);
  list_for_each_entry_safe(d, tmp, &device_list, node) {
          if (!d->present) {
                  list_del_init(&d->node);
                  kfree(d);
          }
  }
  mutex_unlock(&device_lock);
  ```
- **Production or Debugging Angle:** For corruption, inspect initialization, double insertion, double deletion, concurrent traversal, and whether an object was freed while visible. KASAN, lockdep, and list-debug options can turn distant corruption into a more useful report.
- **Common Traps:**
  - Reading "safe" as thread-safe.
  - Freeing before unlinking.
  - Dropping the protecting lock while retaining an unreferenced cursor.
  - Reusing a node after `list_del()` without reinitializing it.
- **Follow-up Questions:**
  - When would RCU list traversal be appropriate?
  - Why can `list_del_init()` help catch lifecycle mistakes?
  - How would you expose list contents through debugfs without racing removal?

### 6. What Does Queueing Work Actually Guarantee?

- **Level:** Mid
- **Question:** If `queue_work()` is called several times for the same work item, will the function run once per call?
- **Short Answer:** No. A work item cannot represent an unlimited count of pending events. A queue attempt normally succeeds only when that work item was not already pending; callers must store event state or use separate work items if every event matters.
- **Deep Explanation:** Workqueues execute `struct work_struct` callbacks on concurrency-managed worker pools. Queueing means "make this work eligible to run," not "run immediately" and not "append one invocation token." `queue_work()` returns a boolean that helps reveal whether the item was newly queued. Events can coalesce while work is pending. A callback may arrange another execution under supported requeue rules, but the design must explicitly handle state accumulated before and during execution.
- **API / Code Anchor:**
  ```c
  spin_lock_irqsave(&d->lock, flags);
  d->pending_events |= event_bits;
  spin_unlock_irqrestore(&d->lock, flags);

  queue_work(d->wq, &d->work);
  ```
- **Production or Debugging Angle:** When reports say "work was lost," log event-state transitions and the return from `queue_work()`. Often the queue behaved correctly but the driver incorrectly used one work item as an event counter.
- **Common Traps:**
  - Assuming each queue call creates a separate execution.
  - Assuming queued work starts before `queue_work()` returns.
  - Using shared state without synchronization because the callback is deferred.
  - Allocating a custom workqueue without an ordering or isolation requirement.
- **Follow-up Questions:**
  - When is `alloc_ordered_workqueue()` useful?
  - What is the difference between system and driver-owned workqueues?
  - How would you preserve a burst of identical events?

### 7. Compare Canceling, Flushing, And Destroying Work

- **Level:** Mid
- **Question:** What are the differences among canceling work, flushing work, and destroying a workqueue?
- **Short Answer:** Cancellation prevents pending work and, with a `_sync` API, waits for a running callback to finish. Flushing waits for queued work to complete without discarding it. Destroying releases a driver-owned workqueue after its work is quiesced.
- **Deep Explanation:** `cancel_work_sync()` and `cancel_delayed_work_sync()` are common teardown tools because they remove pending execution and wait for an in-progress callback. `flush_work()` waits for a specific work item; `flush_workqueue()` waits for work on an owned queue. `destroy_workqueue()` tears down an allocated queue and must happen only after producers can no longer enqueue new work. Shared system workqueues are not destroyed by a driver, and globally flushing unrelated system work is overbroad.
- **API / Code Anchor:**
  ```c
  WRITE_ONCE(d->stopping, true);
  disable_irq(d->irq);
  cancel_delayed_work_sync(&d->poll_work);
  cancel_work_sync(&d->event_work);
  destroy_workqueue(d->wq);
  ```
- **Production or Debugging Angle:** Synchronous cancellation can deadlock if called from the same work item or while holding a lock the callback needs. Review both the callback's lock dependencies and every path that can requeue it.
- **Common Traps:**
  - Describing `cancel_work_sync()` as asynchronous.
  - Destroying a queue while IRQs or timers can still enqueue work.
  - Flushing when teardown requires pending work to be discarded.
  - Calling synchronous cancellation on the current work item.
  - Flushing the global scheduled-work pool for driver-local cleanup.
- **Follow-up Questions:**
  - Why must producers be stopped before cancellation?
  - When is `flush_work()` preferable to cancellation?
  - What changes if the callback requeues itself?

### 8. How Should An Interruptible Wait Be Handled?

- **Level:** Mid
- **Question:** Why must a driver check the return from `wait_event_interruptible()`?
- **Short Answer:** A signal can end the wait while the readiness condition is still false. The driver must return or handle the negative result instead of continuing as if data were ready.
- **Deep Explanation:** `wait_event_interruptible()` evaluates the condition before sleeping and after wakeups. It returns `0` only when the condition became true; signal interruption returns a negative value such as `-ERESTARTSYS`. Timeout variants add a third outcome, so code must distinguish condition success, timeout, and interruption. The condition itself must be published with synchronization compatible with the producer.
- **API / Code Anchor:**
  ```c
  ret = wait_event_interruptible_timeout(
          d->readq,
          READ_ONCE(d->data_ready) || READ_ONCE(d->stopping),
          msecs_to_jiffies(500));
  if (ret < 0)
          return ret;
  if (ret == 0)
          return -ETIMEDOUT;
  if (READ_ONCE(d->stopping))
          return -ENODEV;
  ```
- **Production or Debugging Angle:** A crash after Ctrl-C often points to ignored interruptible-wait results: the signal woke the task, then the driver accessed an empty buffer or invalid state. Also verify that remove wakes blocked users and gives them a terminal condition.
- **Common Traps:**
  - Treating every wakeup as proof that the condition is true.
  - Updating readiness after calling `wake_up*()`.
  - Reading a multi-field condition without the lock used by the producer.
  - Forgetting that timeout APIs encode success and remaining time in their return.
- **Follow-up Questions:**
  - Why must the condition be changed before wakeup?
  - How would this interact with `O_NONBLOCK`?
  - When are lock-aware wait-event variants useful?

### 9. How Are Completions Used Safely?

- **Level:** Mid
- **Question:** Describe a safe completion lifecycle for an asynchronous DMA transaction.
- **Short Answer:** Initialize the completion before submitting DMA, submit while the transaction state is alive, wait with appropriate timeout or interrupt semantics, let the callback call `complete()`, and ensure cancellation or teardown prevents a late callback from touching expired storage.
- **Deep Explanation:** A completion handles the "DMA operation finished" event but does not own the transaction or cancel hardware. Initialization must precede any possible callback. If the completion is reused, `reinit_completion()` is valid only when no old waiter or completer can race with the new generation. A normal `complete()` contributes one event; multiple waiters need deliberate semantics, potentially `complete_all()`, which itself requires careful reinitialization.
- **API / Code Anchor:**
  ```c
  reinit_completion(&d->dma_done);
  ret = demo_submit_dma(d);
  if (ret)
          return ret;

  ret = wait_for_completion_interruptible_timeout(
          &d->dma_done, msecs_to_jiffies(1000));
  if (ret <= 0) {
          demo_terminate_dma(d);
          return ret < 0 ? ret : -ETIMEDOUT;
  }

  /* DMA completion callback */
  complete(&d->dma_done);
  ```
- **Production or Debugging Angle:** On timeout, do not immediately free transaction state. First stop or synchronize with the DMA engine so a late callback cannot call `complete()` on freed memory. Stack-allocated completions are especially dangerous when asynchronous producers can outlive the stack frame.
- **Common Traps:**
  - Reinitializing after submission, potentially erasing an early completion.
  - Assuming timeout cancels the producer.
  - Reusing a completion while an old callback remains possible.
  - Treating one `complete()` as a permanent signaled state.
- **Follow-up Questions:**
  - What race occurs if DMA completes before the waiter starts waiting?
  - When would `complete_all()` be appropriate?
  - How do you prevent late completion after timeout?

## Senior

### 10. Design A Safe Remove Path For Asynchronous Driver State

- **Level:** Senior
- **Question:** A device has an IRQ, timer, work item, wait queue, completion, and global list node. In what order should removal proceed?
- **Short Answer:** Mark the device as stopping and stop external producers first, then synchronize IRQs, stop timers, cancel or flush work, resolve blocked waiters, unlink externally visible membership, release references, and free memory last.
- **Deep Explanation:** The exact subsystem order varies, but the invariant is stable: no new asynchronous user may be created after quiescing begins, and every existing user must finish before storage disappears. A typical path blocks new file operations, publishes `stopping`, disables hardware interrupt generation, disables and synchronizes the IRQ, synchronously shuts down timers, cancels work, wakes condition waiters, terminates operations associated with completions, removes list/device visibility, drops owned references, and finally frees the containing object. Locks cannot substitute for lifetime: a callback that runs after `kfree()` is already too late to acquire one safely.
- **API / Code Anchor:**
  ```c
  WRITE_ONCE(d->stopping, true);
  demo_mask_device_irqs(d);
  disable_irq(d->irq);              /* synchronize as required */
  timer_shutdown_sync(&d->timer);   /* when available/appropriate */
  cancel_work_sync(&d->work);
  wake_up_all(&d->readq);
  demo_terminate_async_io(d);

  mutex_lock(&device_list_lock);
  list_del_init(&d->node);
  mutex_unlock(&device_list_lock);

  demo_drop_final_refs(d);
  ```
- **Production or Debugging Angle:** Build a producer-consumer teardown table during review: IRQ queues work, timer queues work, userspace waits, DMA completes, and list readers obtain references. For every arrow, identify the stop and synchronization operation. Stress unbind/remove while I/O and interrupts are active under KASAN and lockdep.
- **Common Traps:**
  - Canceling work before disabling the IRQ or timer that requeues it.
  - Freeing private state before waking or terminating waiters.
  - Holding a lock needed by a callback while synchronously canceling it.
  - Assuming device-managed allocation automatically orders asynchronous shutdown.
  - Unlinking an object without handling readers that already found it.
- **Follow-up Questions:**
  - Where should reference counting enter this design?
  - How would open file descriptors affect device removal?
  - What if the timer callback rearms itself?

### 11. How Do You Make Wait/Wake Publication Race-Free?

- **Level:** Senior
- **Question:** Is setting a boolean and calling `wake_up()` always sufficient when producer and waiter run on different CPUs?
- **Short Answer:** Not automatically. The shared condition and associated data need a defined synchronization contract so the waiter cannot observe readiness without the data, or miss a transition because publication and waiting are not coordinated.
- **Deep Explanation:** The strongest ordinary pattern is to protect condition state and associated data with the same lock. The producer updates data and condition while holding the lock, releases it, then wakes waiters. The waiter uses a condition expression whose reads obey that contract and reacquires the lock before consuming data. Specialized lockless patterns may use `READ_ONCE()`, `WRITE_ONCE()`, barriers, and waitqueue-active checks, but they require proof against both compiler and CPU reordering. The wait macros handle task state and rechecking; they do not make arbitrary driver state race-free.
- **API / Code Anchor:**
  ```c
  spin_lock_irqsave(&d->lock, flags);
  d->sample = sample;
  d->data_ready = true;
  spin_unlock_irqrestore(&d->lock, flags);
  wake_up_interruptible(&d->readq);

  ret = wait_event_interruptible(d->readq,
                                 READ_ONCE(d->data_ready) ||
                                 READ_ONCE(d->stopping));
  if (ret)
          return ret;

  spin_lock_irqsave(&d->lock, flags);
  /* Revalidate and consume protected state here. */
  spin_unlock_irqrestore(&d->lock, flags);
  ```
- **Production or Debugging Angle:** Intermittent hangs under SMP load often indicate an incomplete state contract rather than a broken waitqueue API. Trace condition changes, wakeups, waiter returns, and lock ownership; do not add random barriers without identifying the required ordering.
- **Common Traps:**
  - Believing `volatile` provides inter-CPU synchronization.
  - Using `READ_ONCE()` as a replacement for locking multi-field invariants.
  - Waking before publishing the condition.
  - Assuming a wakeup is queued permanently for a future condition wait.
- **Follow-up Questions:**
  - What does the wait macro protect, and what remains the driver's responsibility?
  - When might exclusive waiters reduce a thundering herd?
  - How would you prove a lockless wait/wake design correct?

### 12. What Does A Kobject Contribute To Driver Lifetime?

- **Level:** Senior
- **Question:** What is a kobject, and why should most drivers avoid creating raw kobjects for ordinary devices?
- **Short Answer:** A kobject provides low-level reference-counted identity, hierarchy, type/release behavior, and sysfs integration. Most drivers should use higher-level subsystem objects such as `struct device`, which already embed and manage a kobject under subsystem rules.
- **Deep Explanation:** `struct kobject` is commonly embedded in a larger object. Its parent and kset relationships place it in an object hierarchy, while its reference count controls when the type's release callback may free the container. `kobject_put()` drops a reference; it does not necessarily free immediately. Correctness depends on every externally reachable user holding a reference and on a valid release callback. A raw kobject does not by itself solve driver callback teardown, file-descriptor lifetime, or arbitrary memory ownership.
- **API / Code Anchor:**
  ```c
  struct demo_obj {
          struct kobject kobj;
          /* private state */
  };

  static void demo_release(struct kobject *kobj)
  {
          struct demo_obj *obj =
                  container_of(kobj, struct demo_obj, kobj);

          kfree(obj);
  }

  /* Normal device drivers usually use struct device and get_device()/put_device(). */
  ```
- **Production or Debugging Angle:** A missing or incorrect release callback causes leaks or invalid frees. Debug lifetime by recording who acquires and drops references, and prefer subsystem get/put helpers because they preserve the subsystem's ownership model.
- **Common Traps:**
  - Calling `kobject_put()` and then immediately dereferencing the object.
  - Freeing the container manually while references remain.
  - Copying internal `struct kobject` fields from an old kernel description.
  - Building raw sysfs/kobject infrastructure already supplied by `struct device`.
- **Follow-up Questions:**
  - What is the relationship between `struct device` and `struct kobject`?
  - Why must the release callback free the containing object?
  - Does a kobject reference stop a driver's timer or work item?

## Debugging Scenarios

### 13. A Driver Crashes Shortly After Device Unbind

- **Level:** Mid
- **Question:** KASAN reports a use-after-free in a workqueue callback after device unbind. How do you investigate and fix it?
- **Short Answer:** Identify every producer that can queue the work, stop those producers before cancellation, call the appropriate synchronous cancellation helper, and free private state only after the callback cannot be pending, running, or requeued.
- **Deep Explanation:** The callback site is usually the victim, not the root cause. Build the work item's state timeline: who initializes it, who queues it, whether a timer or IRQ can requeue it, whether the callback self-requeues, and when remove frees its container. If remove calls `cancel_work_sync()` while an enabled IRQ can queue the item afterward, cancellation succeeded only for the old instance. The fix is ordering plus a stopping condition, not a null check in freed memory.
- **API / Code Anchor:**
  ```c
  WRITE_ONCE(d->stopping, true);
  demo_mask_device_irqs(d);
  disable_irq(d->irq);
  timer_shutdown_sync(&d->timer);
  cancel_work_sync(&d->work);
  kfree(d);
  ```
- **Production or Debugging Angle:** Enable KASAN and relevant workqueue/timer tracing, add low-rate logs for `queue`, callback entry/exit, cancellation, and free, and stress repeated bind/unbind under activity. Confirm that callback code checks `stopping` before requeueing.
- **Common Traps:**
  - Adding a callback check after the object is already freed.
  - Canceling work while a live producer can queue it again.
  - Holding a callback-required mutex across `cancel_work_sync()`.
  - Fixing only the normal remove path but not probe-error unwind.
- **Follow-up Questions:**
  - How would the diagnosis change for delayed work?
  - What if cancellation itself hangs?
  - How do device-managed allocations affect this bug?

### 14. A Blocking Read Sometimes Hangs Forever

- **Level:** Mid
- **Question:** A blocking `.read()` occasionally never wakes even though an IRQ reports new data. What evidence do you collect?
- **Short Answer:** Trace the protected readiness state, IRQ producer, wakeup call, waiter condition, signal/timeout result, and teardown state. Verify that readiness is published before wakeup and that producer and consumer use one coherent synchronization rule.
- **Deep Explanation:** Possible causes include the IRQ never running, the producer clearing or updating the wrong instance, wakeup occurring before the condition update, the condition being reset by another consumer, a race across multiple fields, or removal leaving a waiter without a terminal condition. A wakeup that occurs before waiting is not inherently lost if the condition remains true, because `wait_event*()` checks before sleeping. Therefore, a persistent condition should be the primary evidence, not a counted wakeup assumption.
- **API / Code Anchor:**
  ```c
  ret = wait_event_interruptible_timeout(
          d->readq,
          READ_ONCE(d->data_ready) || READ_ONCE(d->stopping),
          msecs_to_jiffies(2000));

  dev_dbg(d->dev, "wait ret=%ld ready=%d stopping=%d\n",
          ret, READ_ONCE(d->data_ready), READ_ONCE(d->stopping));
  ```
- **Production or Debugging Angle:** Inspect sleeping task stacks, IRQ counters, dynamic-debug output, and tracepoints rather than flooding the IRQ path with unconditional prints. Test signal interruption, timeout, concurrent readers, and unbind while blocked.
- **Common Traps:**
  - Calling every early wake a "lost wakeup" without checking condition state.
  - Ignoring a negative interruptible-wait return.
  - Using wakeup as the data itself instead of storing readiness.
  - Forgetting to wake blocked readers when the device is removed.
- **Follow-up Questions:**
  - How would multiple readers change the design?
  - When would `wake_up_interruptible_all()` be justified?
  - How should `.poll()` share the same readiness condition?

### 15. A Periodic Timer Stops Or Corrupts Memory

- **Level:** Senior
- **Question:** A periodic timer occasionally stops after jiffies wrap testing, and unload sometimes corrupts memory. What design errors do you suspect?
- **Short Answer:** Suspect non-wrap-safe deadline comparisons, incorrect rearming, and teardown that deletes a timer while its callback can rearm or access freed state.
- **Deep Explanation:** Jiffies wrap by design, so direct `now > deadline` comparisons are unsafe; use `time_after()`, `time_before()`, and related helpers. A periodic callback should compute the next deadline deliberately and call `mod_timer()` or use the appropriate hrtimer forwarding API. During removal, merely deleting the currently pending instance may not prevent a running callback from rearming itself. Publish a stopping flag, stop producers, and use a synchronous shutdown primitive suitable for the target kernel before freeing the container.
- **API / Code Anchor:**
  ```c
  static void demo_timer(struct timer_list *timer)
  {
          struct demo *d = from_timer(d, timer, timer);

          if (READ_ONCE(d->stopping))
                  return;

          queue_work(d->wq, &d->work);
          mod_timer(&d->timer, jiffies + d->period);
  }

  WRITE_ONCE(d->stopping, true);
  timer_shutdown_sync(&d->timer); /* Check target-kernel availability. */
  ```
- **Production or Debugging Angle:** Record callback entry, rearm, shutdown start, and shutdown completion. Use KASAN and timer debug objects where available. Confirm the exact timer teardown API in the target kernel; older supported trees may require a carefully designed `del_timer_sync()` lifecycle instead of newer shutdown helpers.
- **Common Traps:**
  - Using ordinary integer comparisons for jiffies deadlines.
  - Calling `kfree()` before synchronous timer shutdown.
  - Letting a callback rearm after teardown starts.
  - Using removed `init_timer()`/`setup_timer()` interfaces in current code.
  - Performing sleeping I/O from the timer callback.
- **Follow-up Questions:**
  - How does `timer_shutdown_sync()` differ conceptually from `del_timer_sync()`?
  - How would the design change for an hrtimer?
  - When should periodic polling be replaced by an interrupt?

## Final Interview Checklist

A strong candidate should be able to:

- Explain embedded objects, `container_of()`, and intrusive lists without claiming they provide synchronization.
- Select timer, delayed work, hrtimer, wait queue, or completion from execution-context and event-semantics requirements.
- Check wait and timeout returns and distinguish condition state from wakeup notification.
- Explain workqueue coalescing, cancellation, flushing, and producer-first teardown.
- Design removal so every asynchronous path is quiesced before memory is freed.
- Recognize version-sensitive timer APIs and avoid teaching obsolete structure layouts.
- Treat kobjects as reference-counted infrastructure, while preferring subsystem-managed objects in ordinary drivers.
