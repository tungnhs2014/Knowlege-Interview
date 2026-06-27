# 04 - Kernel Logging, Error Handling, And Coding Practice

## Learning Goal

After this chapter, you should be able to write driver code whose failures are easy to understand, easy to unwind, and easy to debug from kernel logs.

By the end, you should be able to:

- Choose the right negative `errno` value for common driver failures.
- Explain how a kernel `return -EFAULT` becomes userspace `errno == EFAULT`.
- Use `ERR_PTR()`, `IS_ERR()`, and `PTR_ERR()` correctly for pointer-returning helpers.
- Structure cleanup paths with `goto` labels without hiding lifetime bugs.
- Choose between `printk()`, `pr_*`, and `dev_*` logging helpers.
- Use `dmesg`, console log levels, and dynamic debug to inspect messages.
- Follow kernel coding style in a way that improves review and reliability.

## Why This Matters In Real Work

Driver bring-up is mostly failure handling. Clocks are missing, GPIOs are described wrong, I2C transfers timeout, DMA memory cannot be allocated, userspace passes bad pointers, and probe may run before a supplier device is ready.

Good driver code makes these failures boring:

- The caller receives the **exact error code**.
- The log says which device and operation failed.
- Partially acquired resources are released in the right order.
- Healthy runtime paths stay quiet.
- Reviewers can follow the function without reverse-engineering indentation.

Bad driver code usually fails in noisier ways:

| Bad habit | Real consequence |
| --- | --- |
| Returning `EINVAL` for everything | Userspace and kernel callers cannot make useful decisions. |
| Missing one cleanup path | Memory/resource leak, reload failure, or crash later. |
| Logging without device context | Impossible to know which hardware instance failed. |
| Printing in hot paths | Timing changes, log flooding, or console stalls. |
| Ignoring style | Reviewers spend time on formatting instead of logic. |

## Mental Model

Think of this topic as three contracts that work together: **return values tell the caller what happened**, **cleanup code restores the system after partial success**, and **logs leave breadcrumbs for the human debugging the failure**.

The simple picture:

```text
driver operation starts
  -> acquire resource A
  -> acquire resource B
  -> operation C fails
  -> log useful context
  -> release B
  -> release A
  -> return exact negative errno
caller receives error
  -> kernel caller sees -errno
  -> syscall userspace sees -1 and errno
```

The kernel does not have process isolation inside driver code. A sloppy error path can destabilize the whole system, so style and structure are part of correctness.

## Core Concepts

Error handling in Linux drivers follows conventions. Once you know them, most probe functions, file operations, and subsystem callbacks start to look familiar.

| Concept | Meaning | Driver rule |
| --- | --- | --- |
| `errno` | Numeric error identity such as `ENOMEM` or `EINVAL`. | Return it as a **negative** value from kernel code. |
| Negative error return | `-ENOMEM`, `-EIO`, `-ENODEV`, etc. | Use for integer-returning failures. |
| Error pointer | Pointer value that encodes a negative errno. | Use when a pointer-returning function must preserve the error reason. |
| Cleanup label | A target used to unwind already-acquired resources. | Labels should describe what they release. |
| Log level | Message severity such as error, warning, info, debug. | Use severity intentionally; do not spam normal paths. |
| Device-scoped log | `dev_err(dev, ...)`, `dev_info(dev, ...)`, etc. | Prefer in drivers when a `struct device *` exists. |
| Kernel style | Shared formatting and naming rules. | Helps humans verify logic and lifetime. |

### Common Error Codes

Choose the error that tells the caller what kind of failure happened.

| Error | Typical driver meaning |
| --- | --- |
| `-ENOMEM` | Allocation failed. |
| `-EINVAL` | Invalid argument, invalid configuration, bad mode, bad offset. |
| `-EIO` | Hardware or bus I/O failed. |
| `-ENODEV` | Device is not present or cannot be matched. |
| `-EBUSY` | Resource or device is already in use. |
| `-EFAULT` | Bad userspace pointer or failed user copy. |
| `-EAGAIN` | Non-blocking operation would need to retry later. |
| `-ERESTARTSYS` | Sleep was interrupted by a signal and syscall restart handling may apply. |
| `-EOPNOTSUPP` | Operation is not supported by this device/driver. |

### `NULL` vs `ERR_PTR()`

Pointer-returning functions have two common failure styles.

| Return style | Use when | Caller checks with |
| --- | --- | --- |
| `NULL` | Failure reason is obvious or API contract says allocation-style failure. | `if (!ptr)` |
| `ERR_PTR(-errno)` | Caller needs the exact failure reason. | `if (IS_ERR(ptr))` then `PTR_ERR(ptr)` |

**Production rule:** check the API contract. Some kernel APIs return `NULL`, some return `ERR_PTR()`, and some can return either. Guessing here causes subtle bugs.

### `pr_*` vs `dev_*`

Kernel logs should tell both severity and origin.

| Helper | Best use |
| --- | --- |
| `printk(KERN_ERR "...")` | Low-level/raw form; useful to understand, uncommon in new driver code. |
| `pr_err()`, `pr_warn()`, `pr_info()` | Module or core code not tied to one device instance. |
| `dev_err()`, `dev_warn()`, `dev_info()` | Device drivers with a `struct device *`. |
| `pr_debug()`, `dev_dbg()` | Debug messages that are compiled out or dynamically controlled by default. |
| `*_once()` / `*_ratelimited()` | Repeated paths where normal logging could flood output. |

For most real drivers, prefer `dev_err(&pdev->dev, ...)`, `dev_warn(&client->dev, ...)`, or `dev_dbg(dev, ...)`.

## Kernel Mechanism

The kernel uses a small set of conventions rather than exceptions. Functions report status through return values, logs go into a kernel ring buffer, and cleanup is explicit unless a managed API owns the resource.

### Error Return Flow

For integer-returning functions:

```text
0                 success
positive value    success with useful value, when API defines it
-ERRNO            failure
```

For syscall-facing callbacks:

```text
driver read() returns -EFAULT
  -> syscall layer returns failure to libc
  -> libc function returns -1
  -> userspace sees errno = EFAULT
```

This is why a character driver must not invent random negative numbers. Userspace behavior depends on meaningful `errno` values.

### Error Pointers

Some kernel helpers return pointers on success but need to preserve a failure code. They encode `-errno` into a special pointer value.

Use the API as an opaque contract:

```c
ptr = helper_get_resource(dev);
if (IS_ERR(ptr))
        return PTR_ERR(ptr);
```

Do not dereference an error pointer. Do not call `PTR_ERR()` on a plain `NULL` pointer unless the API explicitly allows the combined style and you handle it correctly.

### Cleanup With `goto`

Kernel style uses `goto` for centralized cleanup because driver setup is usually a sequence of acquisitions.

Good cleanup has these properties:

- Normal success path is mostly linear.
- Error labels move downward.
- Cleanup happens in reverse acquisition order.
- Label names describe the cleanup action.
- Each label releases only resources that are known to exist.

This avoids nested code like:

```text
if step1 succeeds:
  if step2 succeeds:
    if step3 succeeds:
      do operation
```

### Managed Resources

Device-managed APIs such as `devm_kzalloc()` and `devm_request_irq()` attach cleanup to the device lifetime.

Use them to simplify probe paths:

- `devm_kzalloc(dev, size, GFP_KERNEL)` frees memory when the device detaches.
- `devm_ioremap_resource(dev, res)` unmaps automatically.
- `devm_request_irq(dev, irq, handler, flags, name, data)` frees the IRQ automatically.

**Important:** `devm_*` does not mean "no lifetime thinking." You still need correct ordering for things that can call back into freed state, and not every resource is device-managed.

### Logging Flow

Every kernel log message goes to the kernel log buffer. Whether it appears immediately on the console depends on its level and the console log level.

```text
driver calls dev_err()
  -> printk backend stores message in kernel ring buffer
  -> console_loglevel decides immediate console visibility
  -> userspace can read with dmesg, /dev/kmsg, or journal tools
```

The log buffer is circular. If too much is printed, old messages are overwritten.

## Key Structs And APIs

These APIs matter because they encode the conventions you will use in almost every driver.

### Error Handling APIs

| API | Purpose | Example |
| --- | --- | --- |
| `ERR_PTR(err)` | Convert negative errno to error pointer. | `return ERR_PTR(-ENOMEM);` |
| `IS_ERR(ptr)` | Check whether pointer is an error pointer. | `if (IS_ERR(map))` |
| `PTR_ERR(ptr)` | Extract negative errno from error pointer. | `return PTR_ERR(map);` |
| `IS_ERR_OR_NULL(ptr)` | Check error pointer or `NULL`. | Use only when API may return either. |
| `PTR_ERR_OR_ZERO(ptr)` | Return encoded error or `0`. | Useful after APIs returning error pointer. |

### Logging APIs

| API | Purpose |
| --- | --- |
| `printk(KERN_<LEVEL> "...")` | Base kernel logging function with explicit level prefix. |
| `pr_err()`, `pr_warn()`, `pr_info()` | General log helpers. |
| `pr_debug()` | Conditional debug helper; often controlled by `DEBUG` or dynamic debug. |
| `dev_err()`, `dev_warn()`, `dev_info()` | Device-scoped log helpers. |
| `dev_dbg()` | Device-scoped conditional debug helper. |
| `pr_fmt(fmt)` | Prefix all `pr_*` messages in a source file. |

Common log levels:

| Level | Meaning | Typical driver use |
| --- | --- | --- |
| `KERN_ERR` / `pr_err()` / `dev_err()` | Error condition | Probe failed, hardware transfer failed. |
| `KERN_WARNING` / `pr_warn()` / `dev_warn()` | Suspicious but not fatal | Optional property missing, fallback used. |
| `KERN_NOTICE` / `pr_notice()` | Significant normal event | Rare in drivers. |
| `KERN_INFO` / `pr_info()` / `dev_info()` | Informational | Module loaded, device found; use sparingly. |
| `KERN_DEBUG` / `pr_debug()` / `dev_dbg()` | Debug detail | Register values, state transitions. |

### Debug Commands

| Command / file | Use |
| --- | --- |
| `dmesg` | Read kernel messages. |
| `dmesg -w` | Follow new kernel messages live. |
| `dmesg -l err,warn` | Filter by level. |
| `cat /proc/sys/kernel/printk` | Show console log-level settings. |
| `echo 8 > /proc/sys/kernel/printk` | Print all levels to console temporarily. |
| `dmesg -n 5` | Set console log level through `dmesg`. |
| `/sys/kernel/debug/dynamic_debug/control` | Enable/disable dynamic debug callsites. |
| `/sys/module/printk/parameters/time` | Runtime control for printk timestamp display when supported. |

## Lifecycle / Data Flow

A reliable probe or init path has a clear success path and a matching failure path.

```text
probe/init
  -> validate inputs
  -> allocate private state
  -> acquire hardware resources
  -> initialize hardware/software state
  -> register with subsystem
  -> publish userspace-visible interface
  -> return 0

failure at any step
  -> preserve original ret
  -> log with useful context when helpful
  -> undo only completed steps
  -> return ret

remove/exit
  -> stop new users
  -> unregister callbacks/interfaces
  -> stop work/timers/IRQs
  -> release resources not handled by devm
```

For file operations, the data flow extends to userspace:

```text
userspace read(fd, user_buf, len)
  -> driver .read callback
  -> copy_to_user() fails
  -> driver returns -EFAULT
  -> libc read() returns -1
  -> userspace checks errno
```

For logs:

```text
dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret)
  -> message stored in kernel ring buffer
  -> maybe printed on console
  -> engineer inspects with dmesg or journalctl -k
```

## Minimal Practical Example

This is **learning-only pseudo-code**. It demonstrates error style, cleanup labels, error pointers, and logging; it is not a complete production driver.

```c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct demo_priv {
        void __iomem *base;
        int irq;
};

static void __iomem *demo_map_regs(struct platform_device *pdev)
{
        struct resource *res;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!res)
                return ERR_PTR(-ENODEV);

        /*
         * In real code, prefer devm_ioremap_resource() when the mapping
         * should be tied to the device lifetime.
         */
        return ioremap(res->start, resource_size(res)) ?:
               ERR_PTR(-ENOMEM);
}

static int demo_probe(struct platform_device *pdev)
{
        struct device *dev = &pdev->dev;
        struct demo_priv *priv;
        int ret;

        priv = kzalloc(sizeof(*priv), GFP_KERNEL);
        if (!priv)
                return -ENOMEM;

        priv->base = demo_map_regs(pdev);
        if (IS_ERR(priv->base)) {
                ret = PTR_ERR(priv->base);
                dev_err(dev, "failed to map registers: %d\n", ret);
                goto err_free_priv;
        }

        priv->irq = platform_get_irq(pdev, 0);
        if (priv->irq < 0) {
                ret = priv->irq;
                dev_err(dev, "failed to get IRQ: %d\n", ret);
                goto err_unmap;
        }

        platform_set_drvdata(pdev, priv);
        dev_info(dev, "device initialized\n");
        return 0;

err_unmap:
        iounmap(priv->base);
err_free_priv:
        kfree(priv);
        return ret;
}
```

Important lines:

- `return -ENOMEM;` is correct because nothing has been acquired yet.
- `demo_map_regs()` returns an error pointer so the caller knows whether the failure was missing resource or allocation failure.
- `ret = PTR_ERR(priv->base);` preserves the real error.
- Cleanup labels release resources in reverse order.
- `dev_err()` includes device identity in the log.

With managed resources, the same idea can be shorter:

```c
priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
        return -ENOMEM;

priv->base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(priv->base))
        return dev_err_probe(dev, PTR_ERR(priv->base),
                             "failed to map registers\n");
```

`dev_err_probe()` is useful in modern probe paths, especially with deferred probe, but check your target kernel before using it.

## Common Bugs And Debugging

Start from the symptom, then inspect return values and logs. Most driver failure bugs leave evidence if the code preserves errors and logs context.

| Symptom | Likely cause | Evidence | Fix pattern |
| --- | --- | --- | --- |
| Userspace sees `EINVAL` for many unrelated failures | Driver overwrites exact error with generic `-EINVAL`. | `dmesg` lacks specific failing operation. | Return original `ret`; use specific errno. |
| Module loads once but reload fails | Cleanup path leaked device node, IRQ, memory, class, or cdev. | `dmesg`, sysfs leftovers, registration returns `-EBUSY`/`-EEXIST`. | Mirror init/remove order; test repeated load/unload. |
| Kernel oops after probe failure | Error label freed resource that was never allocated. | Oops near cleanup label. | Split generic `err:` into precise labels. |
| `PTR_ERR()` returns nonsense | Caller used error-pointer handling on `NULL` API or unchecked pointer. | API docs/code show `NULL` failure contract. | Match check style to API: `!ptr`, `IS_ERR()`, or `IS_ERR_OR_NULL()`. |
| `dev_dbg()` never appears | Dynamic debug disabled or callsite not enabled. | No debug lines in `dmesg`; dynamic debug control lacks enabled `+p`. | Enable `DEBUG`, `CONFIG_DYNAMIC_DEBUG`, or dynamic debug rule. |
| Important message not on console | Console log level filters it. | Message is visible in `dmesg` but not console. | Adjust `/proc/sys/kernel/printk` or use correct severity. |
| Logs flood under load | Printing in interrupt/timer/high-frequency path. | Huge `dmesg`, lost old messages, timing changes. | Use `dev_dbg()`, rate-limited logs, tracepoints, or counters. |
| Address prints as hashed value | `%p` hides raw kernel pointers by default. | `(ptrval)` or hashed-looking value. | Use typed formats; use raw addresses only for controlled debugging. |

Useful debugging commands:

```bash
dmesg -w
dmesg -l err,warn
cat /proc/sys/kernel/printk
sudo dmesg -n 8
sudo sh -c "echo 'module my_driver +p' > /sys/kernel/debug/dynamic_debug/control"
sudo sh -c "echo 'file drivers/foo/bar.c +p' > /sys/kernel/debug/dynamic_debug/control"
```

Debugging checklist:

- What exact errno was returned?
- Was the original `ret` preserved?
- Which resource was acquired immediately before failure?
- Does every error label release only resources acquired before it?
- Did the log include device, operation, and useful identifiers?
- Is the message visible in `dmesg` even if not on console?
- Is the path repeated enough to need rate limiting or dynamic debug?

## Production Checklist

Before review or board bring-up, error handling and logging should be boringly consistent.

Error returns:

- Use negative errno values from kernel code.
- Preserve subsystem return values unless there is a strong reason to translate them.
- Use `-EFAULT` for failed userspace copy helpers.
- Use `-ENOMEM` only for allocation failures.
- Use `-EINVAL` for invalid input/configuration, not as a catch-all.
- Return `0` only after all required setup has succeeded.

Cleanup:

- Keep the main success path linear.
- Use meaningful labels such as `err_free_irq`, `err_unmap`, `err_disable_clk`.
- Unwind in reverse acquisition order.
- Avoid one generic `err:` label that assumes every resource exists.
- Test failure paths where practical, not only the success path.
- Repeatedly load/unload learning modules and remove/reprobe real drivers during bring-up.

Logging:

- Prefer `dev_*` in device drivers.
- Include the failed operation and return code.
- Use `dev_dbg()`/`pr_debug()` for noisy details.
- Avoid logging normal high-frequency events.
- End messages with `\n`.
- Do not print secrets, keys, or casual raw kernel addresses.
- Keep healthy drivers mostly quiet.

Style:

- Use tabs with kernel indentation.
- Keep functions focused and split overly deep nesting.
- Use `static` for file-local helpers.
- Keep comments for "why," hardware constraints, and tricky ordering.
- Run `scripts/checkpatch.pl`, but still read similar in-tree drivers.

## Interview Readiness

You are ready for interviews when you can explain failure paths without memorizing tables.

Be able to answer:

- Why does kernel code return `-ENOMEM` instead of `ENOMEM`?
- How does a driver error become userspace `errno`?
- When should a pointer-returning function use `NULL` versus `ERR_PTR()`?
- Why does kernel code use `goto` for cleanup?
- What is a "one err bug"?
- Why should drivers prefer `dev_err()` over `pr_err()` when possible?
- Why might `pr_info()` appear in `dmesg` but not on the console?
- Why does `dev_dbg()` not always print?
- Why can logging in interrupt or fast paths be dangerous?

See `interview/04-kernel-logging-error-handling-and-coding-practice.md` for structured practice questions.

## Kernel Version Notes

Logging and error-handling conventions are stable, but a few helpers and behaviors are version-sensitive.

- `dev_err_probe()` is common in modern probe paths, especially for deferred probe handling. Check target kernel headers before using it in examples.
- Pointer printing has become more restrictive over time. `%p` is generally hashed; avoid `%px` unless you have a controlled debug reason.
- `pr_debug()` and `dev_dbg()` behavior depends on build configuration such as `DEBUG` and `CONFIG_DYNAMIC_DEBUG`.
- `printk()` can be called from any context, but excessive printing in hot paths can still cause serious latency or console-related problems. Prefer debug, rate-limited, or tracing mechanisms for frequent events.
