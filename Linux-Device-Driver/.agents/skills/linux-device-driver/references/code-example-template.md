# <NN> - <Topic> Example

Use this template for `examples/NN-<topic>/`.

Include:

- Goal
- Kernel version assumptions
- Files
- Build command
- Load/unload/test commands
- Kernel module code
- Optional userspace test
- Expected logs
- Cleanup/error-path explanation
- Why this is or is not production-ready

Rules:

- Keep the example minimal but real enough to teach lifetime and error handling.
- Explain user-visible ABI impact when the example creates device nodes, sysfs, ioctl, procfs, or debugfs interfaces.
- Prefer Linux kernel style and realistic cleanup labels.
- If code is simplified, label it learning-only and explain what production code would add.
- State version-sensitive API signatures and validate against target kernel headers or docs when needed.
- Use the Topic Brief internally to avoid missing source details, but do not paste audit metadata into the example README.
