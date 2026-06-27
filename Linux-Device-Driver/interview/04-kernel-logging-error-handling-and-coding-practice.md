# 04 - Kernel Logging, Error Handling, And Coding Practice Interview Questions

Strong candidates can reason about failure paths as carefully as success paths. They should connect return values, cleanup order, kernel logs, userspace `errno`, and code style into one practical driver-development habit.

Use these questions to test whether someone can debug real Embedded Linux driver bring-up, not just recite API names.

## Beginner

### 1. Why Does Kernel Code Return `-ENOMEM` Instead Of `ENOMEM`?

- **Level:** Beginner
- **Question:** Why are Linux kernel errors usually returned as negative errno values?
- **Short Answer:** Kernel functions usually return `0` or a positive useful value on success and a negative errno such as `-ENOMEM` or `-EINVAL` on failure.
- **Deep Explanation:** The negative sign separates failures from successful values. Many APIs need positive values for valid results, such as byte counts from `read()` or object counts from helper functions. Returning `-errno` gives the caller both "this failed" and "why it failed" in one integer.
- **API / Code Anchor:**
  ```c
  buf = kmalloc(size, GFP_KERNEL);
  if (!buf)
          return -ENOMEM;

  if (mode > MAX_MODE)
          return -EINVAL;

  return 0;
  ```
- **Production or Debugging Angle:** If a driver returns the wrong sign or a generic error, userspace and subsystem code may take the wrong recovery path. Always preserve meaningful errors from lower layers unless translation is intentional.
- **Common Traps:**
  - Returning `ENOMEM` instead of `-ENOMEM`.
  - Returning `-1` instead of a specific errno.
  - Using `-EINVAL` for every failure.
  - Returning `0` after a partial initialization failure.
- **Follow-up Questions:**
  - When is a positive return value valid?
  - What should a probe function return on allocation failure?
  - Why is `-EINVAL` a poor catch-all?

### 2. How Does A Kernel Error Reach Userspace?

- **Level:** Beginner
- **Question:** If a driver's `.read()` returns `-EFAULT`, what does userspace see?
- **Short Answer:** The userspace `read()` call returns `-1`, and libc sets `errno` to `EFAULT`.
- **Deep Explanation:** Kernel syscall callbacks return negative errno values internally. The syscall boundary converts that failure into the userspace convention: function returns `-1`, and the positive errno value is stored in `errno`. This is why driver callbacks must choose error codes carefully.
- **API / Code Anchor:**
  ```c
  static ssize_t demo_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
  {
          if (copy_to_user(buf, kernel_buf, count))
                  return -EFAULT;

          return count;
  }
  ```
- **Production or Debugging Angle:** ABI behavior includes error behavior. Scripts and applications may distinguish `EAGAIN`, `EFAULT`, `EINVAL`, and `ENODEV`.
- **Common Traps:**
  - Thinking userspace directly receives `-EFAULT`.
  - Returning `-ENOMEM` for a bad userspace pointer.
  - Forgetting short successful I/O is not the same as failure.
  - Logging every user input mistake as a kernel error.
- **Follow-up Questions:**
  - Which errno should failed `copy_to_user()` normally return?
  - What does non-blocking I/O often return when data is not ready?
  - Why is errno selection part of userspace ABI design?

### 3. What Is The Difference Between `printk()`, `pr_*`, And `dev_*`?

- **Level:** Beginner
- **Question:** Which logging helper should a new device driver usually use?
- **Short Answer:** Use `dev_*` helpers when you have a `struct device *`, use `pr_*` for non-device module/core messages, and understand `printk()` as the lower-level logging function.
- **Deep Explanation:** `printk()` writes a message with an explicit kernel log level. `pr_err()`, `pr_warn()`, and `pr_info()` are convenient wrappers around `printk()` levels. Device drivers usually have a device object, so `dev_err(dev, ...)` is better because the log is tagged with the specific device instance.
- **API / Code Anchor:**
  ```c
  pr_info("module loaded\n");
  dev_err(&pdev->dev, "failed to request IRQ %d: %d\n", irq, ret);
  dev_dbg(&client->dev, "status=0x%02x\n", status);
  ```
- **Production or Debugging Angle:** On boards with multiple instances of the same IP block, `dev_err()` makes the difference between "something failed" and "this exact device failed."
- **Common Traps:**
  - Using `printk("error")` with no level or context.
  - Using `pr_err()` in probe when `dev_err()` is available.
  - Logging normal healthy operation at error level.
  - Forgetting the newline.
- **Follow-up Questions:**
  - When is `pr_info()` appropriate?
  - Why is `dev_dbg()` useful during bring-up?
  - What does `pr_fmt()` customize?

### 4. Why Might A Log Be In `dmesg` But Not On The Console?

- **Level:** Beginner
- **Question:** A driver calls `pr_info()`, but nothing appears on the serial console. What could be happening?
- **Short Answer:** The message may be in the kernel log buffer but filtered from the console by the current console log level.
- **Deep Explanation:** Kernel messages are stored in a ring buffer. Immediate console printing depends on message severity and `console_loglevel`. Lower numeric log levels are higher priority. An info-level message can be visible through `dmesg` while not being printed directly on the console.
- **API / Code Anchor:**
  ```bash
  dmesg | tail
  cat /proc/sys/kernel/printk
  sudo dmesg -n 8
  ```
- **Production or Debugging Angle:** During bring-up, always check `dmesg` or `journalctl -k` before assuming the driver did not log. Console filtering is normal and often desirable.
- **Common Traps:**
  - Assuming no console output means no kernel log exists.
  - Raising console log level permanently in production.
  - Using error severity just to force console visibility.
  - Forgetting the log buffer can wrap and overwrite old messages.
- **Follow-up Questions:**
  - What does `/proc/sys/kernel/printk` show?
  - How can you follow new kernel logs live?
  - Why can excessive logging lose earlier messages?

## Mid-Level

### 5. When Should You Use `ERR_PTR()` Instead Of `NULL`?

- **Level:** Mid
- **Question:** A helper returns a pointer but can fail for multiple reasons. How should it report the exact failure?
- **Short Answer:** Return `ERR_PTR(-errno)` and have callers check with `IS_ERR()` and extract the error with `PTR_ERR()`.
- **Deep Explanation:** Returning `NULL` only says "no pointer." It does not explain whether the problem was no memory, no device, invalid argument, deferred probe, or I/O failure. Error pointers preserve the exact negative errno while keeping the function's return type as a pointer.
- **API / Code Anchor:**
  ```c
  static struct regmap *demo_get_map(struct device *dev)
  {
          if (!device_present(dev))
                  return ERR_PTR(-ENODEV);

          return regmap_init_mmio(dev, base, &cfg);
  }

  map = demo_get_map(dev);
  if (IS_ERR(map))
          return PTR_ERR(map);
  ```
- **Production or Debugging Angle:** Many subsystem APIs return error pointers. Correctly propagating `PTR_ERR()` makes probe failures much easier to diagnose.
- **Common Traps:**
  - Checking an error-pointer API with `if (!ptr)`.
  - Calling `PTR_ERR()` on `NULL`.
  - Dereferencing before `IS_ERR()`.
  - Assuming every pointer-returning API uses error pointers.
- **Follow-up Questions:**
  - What helper checks both `NULL` and error pointers?
  - Why should callers treat encoded error pointers as opaque?
  - What does `PTR_ERR_OR_ZERO()` do?

### 6. Why Does Kernel Code Use `goto` For Cleanup?

- **Level:** Mid
- **Question:** Why is `goto` considered normal in Linux driver error paths?
- **Short Answer:** It keeps the success path linear and centralizes cleanup for resources acquired before a failure.
- **Deep Explanation:** Driver setup usually acquires resources step by step. If step four fails, the driver must release steps three, two, and one in reverse order. `goto` labels avoid deep nesting and duplicated cleanup at every return site. The labels should describe the cleanup action and should only free resources known to exist.
- **API / Code Anchor:**
  ```c
  ret = request_irq(irq, handler, 0, name, data);
  if (ret)
          goto err_unmap;

  ret = register_device(data);
  if (ret)
          goto err_free_irq;

  return 0;

err_free_irq:
  free_irq(irq, data);
err_unmap:
  iounmap(base);
  return ret;
  ```
- **Production or Debugging Angle:** Reviewers look closely at cleanup labels because many driver bugs are leaks or use-after-free issues on rare failure paths.
- **Common Traps:**
  - One generic `err:` label that frees things not allocated on all paths.
  - Jumping backward.
  - Losing the original `ret`.
  - Cleanup order different from acquisition order.
- **Follow-up Questions:**
  - What is a "one err bug"?
  - When is direct `return ret;` better than `goto`?
  - How does `devm_*` change cleanup structure?

### 7. How Do `devm_*` APIs Affect Error Handling?

- **Level:** Mid
- **Question:** If a driver uses `devm_kzalloc()` and `devm_request_irq()`, does it still need cleanup logic?
- **Short Answer:** It needs less manual cleanup, but it still needs lifetime and ordering logic.
- **Deep Explanation:** Device-managed resources are automatically released when the device is detached or probe fails after registration with devres. That simplifies many probe paths. However, not every resource is managed, and callbacks/work/timers/userspace-visible objects may still need explicit shutdown before memory is released.
- **API / Code Anchor:**
  ```c
  priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
  if (!priv)
          return -ENOMEM;

  ret = devm_request_irq(dev, irq, handler, 0, dev_name(dev), priv);
  if (ret)
          return ret;
  ```
- **Production or Debugging Angle:** `devm_*` is excellent for device-bound resources. It is not a license to ignore remove order, active callbacks, or resources whose lifetime is not exactly the device lifetime.
- **Common Traps:**
  - Manually freeing `devm_*` memory in normal remove paths.
  - Mixing managed and unmanaged resources without documenting ownership.
  - Forgetting to stop workqueues/timers before state disappears.
  - Assuming devres solves userspace ABI lifetime.
- **Follow-up Questions:**
  - When might manual cleanup still be required?
  - Why can callback ordering matter with managed memory?
  - What is the benefit of managed resource APIs in probe?

### 8. How Do You Debug Missing `dev_dbg()` Output?

- **Level:** Mid
- **Question:** A driver has `dev_dbg()` messages, but they never appear. What do you check?
- **Short Answer:** Check whether debug callsites are compiled/enabled through `DEBUG` or `CONFIG_DYNAMIC_DEBUG`, then enable the relevant dynamic debug rule if available.
- **Deep Explanation:** `dev_dbg()` and `pr_debug()` are conditional. They are commonly compiled out or inactive unless debug support is enabled. With dynamic debug, callsites can be enabled at runtime by module, file, function, or format.
- **API / Code Anchor:**
  ```bash
  mount -t debugfs none /sys/kernel/debug
  grep my_driver /sys/kernel/debug/dynamic_debug/control
  echo 'module my_driver +p' > /sys/kernel/debug/dynamic_debug/control
  echo 'file drivers/foo/bar.c +p' > /sys/kernel/debug/dynamic_debug/control
  dmesg -w
  ```
- **Production or Debugging Angle:** Dynamic debug is valuable because you can keep production logs quiet but enable detailed driver breadcrumbs during field debugging.
- **Common Traps:**
  - Assuming `dev_dbg()` behaves like `dev_info()`.
  - Forgetting debugfs may not be mounted.
  - Enabling too broad a dynamic debug rule and flooding logs.
  - Using `dev_info()` for permanent debug spam.
- **Follow-up Questions:**
  - How is `pr_debug()` different from `printk(KERN_DEBUG ...)`?
  - Why is debug logging often better than info logging?
  - What alternatives exist for very high-frequency events?

### 9. How Should A Driver Choose Log Severity?

- **Level:** Mid
- **Question:** When should a driver use `dev_err()`, `dev_warn()`, `dev_info()`, and `dev_dbg()`?
- **Short Answer:** Use error for real failures, warning for suspicious recoverable conditions, info sparingly for significant lifecycle events, and debug for noisy details.
- **Deep Explanation:** Log severity is both a human signal and a filtering mechanism. If everything is logged as error, real failures disappear in noise. If probe failures are only debug messages, bring-up becomes painful. Good messages include context such as operation, resource, address, IRQ, or return code.
- **API / Code Anchor:**
  ```c
  if (ret)
          dev_err(dev, "failed to enable regulator: %d\n", ret);

  if (!optional_gpio)
          dev_warn(dev, "reset GPIO missing, using default state\n");

  dev_dbg(dev, "status=0x%08x\n", readl(base + STATUS));
  ```
- **Production or Debugging Angle:** Healthy drivers should be mostly quiet. Logs are for abnormal events, important state transitions, and controlled debug.
- **Common Traps:**
  - Printing every interrupt at info level.
  - Logging expected optional resources as hard errors.
  - Omitting the return code.
  - Using vague messages like `"failed"` with no operation.
- **Follow-up Questions:**
  - Why can info-level logging still be too noisy?
  - When should you use rate-limited logging?
  - What context should a good probe failure message include?

## Senior

### 10. How Can Logging In Hot Paths Break A System?

- **Level:** Senior
- **Question:** Why can `printk()` or `dev_info()` in an interrupt handler or fast data path be dangerous?
- **Short Answer:** Frequent logging can flood the ring buffer, distort timing, increase latency, and on some console paths contribute to stalls or lockup-like behavior.
- **Deep Explanation:** Kernel logging is not free. Messages must be formatted, stored, and sometimes flushed to slow consoles such as serial. In high-frequency paths, that overhead can change the bug being debugged or create a new one. Logs may also overwrite the earlier evidence needed to understand the original failure.
- **API / Code Anchor:**
  ```c
  /* Bad in a frequent IRQ path */
  dev_info(dev, "irq status=0x%x\n", status);

  /* Better */
  dev_dbg(dev, "irq status=0x%x\n", status);
  dev_warn_ratelimited(dev, "unexpected IRQ status=0x%x\n", status);
  ```
- **Production or Debugging Angle:** For frequent events, prefer counters, tracepoints, dynamic debug, ftrace, or rate-limited messages. Keep permanent logs for meaningful abnormal conditions.
- **Common Traps:**
  - Using logs as a substitute for proper tracing.
  - Leaving bring-up prints in production.
  - Logging under locks without considering timing.
  - Forgetting serial consoles are slow.
- **Follow-up Questions:**
  - When would you use tracepoints instead of printk?
  - What does a rate-limited log helper solve?
  - Why can logging hide race conditions?

### 11. How Do You Design A Probe Error Path For Maintainability?

- **Level:** Senior
- **Question:** What makes a probe error path maintainable as a driver grows?
- **Short Answer:** A maintainable probe path preserves exact errors, uses device-scoped logs, has linear acquisition, precise cleanup labels, clear ownership, and avoids mixing managed/unmanaged lifetimes carelessly.
- **Deep Explanation:** Probe functions grow over time: regulators, clocks, resets, pinctrl, IRQs, buffers, registration, sysfs, and runtime PM appear one by one. The error path must scale with that growth. Each new acquisition needs a corresponding cleanup decision. Managed resources help, but registrations and active callbacks still need explicit ordering.
- **API / Code Anchor:**
  ```c
  ret = clk_prepare_enable(priv->clk);
  if (ret)
          return dev_err_probe(dev, ret, "failed to enable clock\n");

  ret = request_irq(irq, handler, 0, dev_name(dev), priv);
  if (ret)
          goto err_disable_clk;

  ret = subsystem_register(priv);
  if (ret)
          goto err_free_irq;

  return 0;

err_free_irq:
  free_irq(irq, priv);
err_disable_clk:
  clk_disable_unprepare(priv->clk);
  return ret;
  ```
- **Production or Debugging Angle:** The best probe paths are easy to audit during code review. A reviewer should be able to answer "what owns this resource?" and "who can still call into this state?" quickly.
- **Common Traps:**
  - Registering with a subsystem before state is fully initialized.
  - Freeing private data before unregistering callbacks.
  - Translating all errors to `-ENODEV`.
  - Adding a resource without updating failure labels.
- **Follow-up Questions:**
  - Where should userspace-visible interfaces be created in probe?
  - Why should remove order usually reverse probe order?
  - How can deferred probe affect error logging?

### 12. What Is The ABI Risk In Error Codes?

- **Level:** Senior
- **Question:** Why can changing error codes in a driver be a userspace ABI issue?
- **Short Answer:** Userspace programs may make decisions based on specific errno values, so changing them can break existing behavior.
- **Deep Explanation:** Driver file operations, ioctl handlers, sysfs stores, and other userspace entry points expose behavior. Returning `-EAGAIN` versus `-EINVAL` versus `-EIO` is not just internal style; applications may retry, report configuration errors, or mark hardware failed based on that errno.
- **API / Code Anchor:**
  ```c
  if (file->f_flags & O_NONBLOCK && !data_ready)
          return -EAGAIN;

  if (cmd_unknown)
          return -ENOTTY;

  if (copy_from_user(&cfg, argp, sizeof(cfg)))
          return -EFAULT;
  ```
- **Production or Debugging Angle:** Before changing a userspace-visible errno, check existing users, tests, tools, and subsystem conventions. Document intentional behavior in ABI-facing code.
- **Common Traps:**
  - Returning `-EINVAL` for unknown ioctl commands instead of the conventional ioctl error.
  - Treating all hardware failures as invalid user input.
  - Changing retryable errors into permanent errors.
  - Logging user mistakes as kernel hardware errors.
- **Follow-up Questions:**
  - Which errno often means non-blocking retry later?
  - Why does failed user copy use `-EFAULT`?
  - What is the difference between invalid command and failed hardware I/O?

### 13. How Do You Review Logging Quality In A Driver Patch?

- **Level:** Senior
- **Question:** What do you look for when reviewing a driver's logging and diagnostics?
- **Short Answer:** Check severity, device context, useful details, noise level, security, hot-path behavior, and whether logs help diagnose real failures.
- **Deep Explanation:** Good logs are specific and sparse. They tell what operation failed, on which device, and often include the return code or relevant resource identifier. Bad logs are vague, too frequent, misleadingly severe, or leak sensitive information such as raw kernel addresses or keys.
- **API / Code Anchor:**
  ```c
  /* Weak */
  pr_err("failed\n");

  /* Better */
  dev_err(dev, "failed to read WHOAMI register: %d\n", ret);
  ```
- **Production or Debugging Angle:** Field logs are often the only evidence available from embedded devices. A clear message can save hours; a noisy driver can hide the root cause.
- **Common Traps:**
  - Printing raw pointers with `%px` unnecessarily.
  - Using all-caps or vague emotional messages.
  - Logging expected probe deferrals as scary errors.
  - Forgetting multi-instance devices need device-scoped logs.
- **Follow-up Questions:**
  - Why are raw kernel addresses sensitive?
  - What makes a log message grep-friendly?
  - When is `dev_err_probe()` useful?

### 14. Debugging Scenario: Probe Fails With `-517`

- **Level:** Senior
- **Question:** A platform driver's probe fails and the log says `failed to get regulator: -517`. How do you reason about it?
- **Short Answer:** `-517` is commonly `-EPROBE_DEFER`; a supplier is not ready yet. The driver should return that error so the core can retry later, ideally without noisy repeated logs.
- **Deep Explanation:** Probe can run before dependencies such as regulators, clocks, GPIO controllers, or PHYs are available. Deferred probe is not necessarily a fatal hardware failure. A good driver preserves the error and lets the driver core retry. Modern probe code often uses helpers that handle deferred-probe logging more cleanly.
- **API / Code Anchor:**
  ```c
  supply = devm_regulator_get(dev, "vdd");
  if (IS_ERR(supply))
          return dev_err_probe(dev, PTR_ERR(supply),
                               "failed to get vdd regulator\n");
  ```
- **Production or Debugging Angle:** Repeated scary logs for deferred probe can confuse bring-up. Keep the exact error and inspect provider drivers, device tree dependencies, and probe ordering.
- **Common Traps:**
  - Converting `-EPROBE_DEFER` to `-ENODEV`.
  - Treating deferred probe as permanent failure.
  - Flooding logs on every retry.
  - Not checking whether the supplier driver loaded.
- **Follow-up Questions:**
  - What kinds of resources commonly cause deferred probe?
  - Why is preserving `PTR_ERR()` important here?
  - How would you inspect whether the supplier device exists?

### 15. Common Trap: `IS_ERR()` On A `NULL` API

- **Level:** Senior
- **Question:** A driver calls an API that returns `NULL` on allocation failure, but checks it with `IS_ERR()`. What can go wrong?
- **Short Answer:** `IS_ERR(NULL)` is false, so the driver may treat `NULL` as a valid pointer and crash later.
- **Deep Explanation:** Error handling must match the API contract. Allocation-style APIs often return `NULL`; many subsystem lookup or init APIs return `ERR_PTR()`. Some APIs can return either, but only their documentation or implementation tells you that. The wrong check is often worse than no check because it looks correct during review.
- **API / Code Anchor:**
  ```c
  priv = kzalloc(sizeof(*priv), GFP_KERNEL);
  if (IS_ERR(priv))          /* wrong */
          return PTR_ERR(priv);

  if (!priv)                 /* correct for kzalloc */
          return -ENOMEM;
  ```
- **Production or Debugging Angle:** When debugging a NULL dereference after an allocation or resource lookup, inspect the API's documented failure return style and all caller checks.
- **Common Traps:**
  - Assuming every pointer failure is `ERR_PTR()`.
  - Using `IS_ERR_OR_NULL()` to paper over not knowing the API.
  - Calling `PTR_ERR()` on `NULL` and returning a misleading value.
  - Copying patterns from a different subsystem blindly.
- **Follow-up Questions:**
  - How does `devm_kzalloc()` report failure?
  - How do many `devm_*_get()` helpers report failure?
  - When is `IS_ERR_OR_NULL()` appropriate?

## Debugging Scenarios And Traps

These quick prompts are useful for mock interviews and code reviews.

| Scenario | What strong candidates should say |
| --- | --- |
| `insmod` fails and userspace only says "Invalid argument" | Check `dmesg`; module init returned `-EINVAL`, but the kernel log should explain which validation failed. |
| Probe fails once, then succeeds later | Suspect deferred probe or supplier ordering; preserve exact errors. |
| Repeated load/unload eventually fails with `-EBUSY` | Suspect leaked registration, IRQ, cdev, sysfs file, work, timer, or reference. |
| `dev_err()` prints too often during normal operation | Wrong severity or expected condition logged as error; use debug/rate-limited/counters. |
| Error label crashes on probe failure | Label frees uninitialized resource; split labels by ownership. |
| `pr_debug()` does not appear | It may be compiled out or dynamically disabled. |
| Pointer log shows hashed value | `%p` is protected; use typed formats and avoid raw addresses. |
| All failures return `-ENODEV` | Driver is destroying useful failure information. |

## Final Interview Checklist

Before an interview, make sure you can explain:

- Negative errno conventions and userspace propagation.
- `NULL` versus `ERR_PTR()` APIs.
- `IS_ERR()`/`PTR_ERR()` misuse patterns.
- Why `goto` improves cleanup when used carefully.
- How `devm_*` helps and where it does not.
- `printk()` levels, console log level, and the ring buffer.
- `pr_*` versus `dev_*`.
- Why `dev_dbg()` may not print.
- Logging risks in hot paths.
- How style, labels, and logs make driver review easier.
