# Topic Brief - 07 - Character Device Drivers

## Output Targets
- Knowledge: `knowledge/07-character-device-drivers.md`
- Interview: `interview/07-character-device-drivers.md`
- Example: `examples/07-character-device-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/covered/merged | Core character-device model: `dev_t`, major/minor, `struct cdev`, `file_operations`, read/write/llseek/poll/ioctl. |
| `notion-ch04-part1` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 1 Character Device Registration.md` | read/covered/merged | Registration flow, `/dev`, `/sys/class`, `class_create()`, `device_create()`, multi-device registration, cleanup. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/covered/merged | `file_operations`, `inode` vs `file`, `__user`, copy helpers, `open()`, `release()`, `file->private_data`. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/covered/merged | `read()`, `write()`, `llseek()`, `f_pos`, bounds checks, return values, locking, EEPROM-style examples. |
| `notion-ch04-part4` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | read/covered/merged | `poll()`, wait queues, `O_NONBLOCK`, ioctl command numbers, ioctl validation, ABI best practices. |
| `notion-ch02-part1` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md` | read/covered/merged | Supporting syscall flow and user/kernel memory boundary. |
| `notion-ch03-part6` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 6 Wait Queues and Sleep Wake Mechanisms.md` | read/covered/merged | Supporting wait-queue behavior for blocking reads/writes and readiness notification. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/covered/merged | Supporting platform-driver integration: `probe()` allocates state, registers `cdev`, and creates `/dev/pcdev-*`. |

## Source Files Read
- `knowledge/chapter-04-char-driver.md` and `interview/chapter-04-char-driver.md` were used as high-quality legacy baselines for the learning-path migration.
- V4L2 source material is intentionally not merged into this chapter beyond high-level framework comparison; detailed V4L2 coverage belongs to chapters 32-34.

## Merged Source Notes
- `ldd1-ch04` provides the compact mechanism backbone.
- Notion chapter 4 parts 1-4 provide expanded beginner-friendly explanations and practical examples.
- The final learner-facing docs should keep the legacy clarity and depth, especially around user-copy, short I/O, `poll_wait()`, ioctl ABI, cleanup order, and lifetime.

## Source Differences
- `class_create()` signature is kernel-version-sensitive.
- `.poll` return type is kernel-version-sensitive: older examples may use `unsigned int`; newer kernels commonly use `__poll_t` / `poll_t`.
- Use `_IOWR`, not `_IORW`, for bidirectional ioctl command numbers.

## Gaps / Uncertainties
- New `examples/07-character-device-drivers/` should be reviewed against the target kernel headers before being treated as build-tested.
- Remove-while-open and production-grade lifetime rules require extra care beyond a minimal learning example.

## External Validation
- Prefer `docs.kernel.org/filesystems/vfs.html` for VFS/file operation behavior.
- Prefer `docs.kernel.org/driver-api/ioctl.html` for ioctl ABI design.
- Check target kernel headers for `class_create()` and `.poll` signatures.

## Learning Content Brief
The chapter should teach how `/dev` reaches `file_operations`, how `dev_t`/major/minor/`cdev`/class/device relate, how `inode` differs from `file`, how `file->private_data` carries driver state, how safe user-copy works, how read/write/llseek handle offsets and short I/O, how blocking and `poll()` readiness work, how ioctl ABI should be designed, and how cleanup/lifetime/debugging work in real drivers.
