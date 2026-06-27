# Topic Brief - 34 - V4L2 Userspace, Debugging, And Compliance

## Output Targets
- Knowledge: `knowledge/34-v4l2-userspace-debugging-and-compliance.md`
- Interview: `interview/34-v4l2-userspace-debugging-and-compliance.md`
- Example: `examples/34-v4l2-userspace-debugging-and-compliance/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/no direct V4L2 userspace source found | No V4L2 userspace, `v4l2-ctl`, media-controller, or compliance chapter exists in book 1. Generic ioctl, poll, mmap, framebuffer, and IIO capture hits are adjacent concepts, not direct V4L2 userspace/compliance source. |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | searched/mapped/out-of-scope | Contains generic `ioctl()` and `poll()` ABI mechanics. Useful prerequisite for topic 08/07, but not merged as V4L2-specific material. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | searched/mapped/out-of-scope | Framebuffer userspace access through `/dev/fbX`, `mmap()`, and framebuffer ioctls is display-specific and belongs to topic 29, not V4L2 capture. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/related-topic | Driver-side prerequisite for interpreting userspace failures: `video_device`, `v4l2_ioctl_ops`, vb2 callbacks, `STREAMON` failures, DMA timeout, buffer-size mismatch, dynamic debug, and basic `v4l2-ctl` smoke tests. Primary coverage belongs to topic 32. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/covered/merged | Media-controller userspace workflow: `/dev/mediaX` configures pipeline topology, `/dev/videoX` streams, `media-ctl --reset`, `--links`, `--set-v4l2`/`-V`, `--print-topology`/`-p`, `--print-dot`, and board examples before capture with `v4l2-ctl`. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/covered/merged | Primary source for V4L2 userspace API: `open`, `ioctl`, `mmap`, `read`, capture ioctl order, format negotiation, controls, MMAP/USERPTR/DMABUF, prime buffers, DQBUF behavior, `v4l2-ctl`, ffmpeg/GStreamer checks, dev_debug, vb2 debug module parameters, and `v4l2-compliance`. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/covered/merged | General debug support for V4L2 investigations: `pr_*`, `dev_*`, kernel log levels, `/proc/sys/kernel/printk`, `dmesg`, printk timestamps, ftrace/function_graph, event tracing, process filters, oops analysis, and trace dump on oops. Most deep tracing belongs to topic 37. |
| `notion-ch07-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/covered/merged | V4L2/vb2 debugging snippets: dynamic debug commands for V4L2 core, vb2, and a specific media driver; failure table for `STREAMON`, DMA timeout, and image corruption; `v4l2-ctl --all` and `--stream-mmap` smoke tests. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2.md` | read/mapped/covered/merged | Teaching-oriented media-controller userspace commands: `media-ctl -p`, DOT graph generation, reset links, set links, set pad formats, complete i.MX pipeline setup, and boundary with `v4l2-ctl` capture. Primary media graph theory belongs to topic 33. |
| `notion-ch09-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part1.md` | read/mapped/covered/merged | Teaching version of V4L2 userspace API: core syscalls, key ioctls, structures, capture sequence, `xioctl()` retry for `EINTR`, blocking vs non-blocking open, capability checks, format/FPS/control setup, buffer queues, memory model comparison, buffer flags, and key takeaways. |
| `notion-ch09-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md` | read/mapped/covered/merged | Complete MMAP capture flow, cleanup/error paths, `v4l2-ctl` command reference, `v4l2-compliance`, select/poll, multi-planar buffers, DMABUF zero-copy, performance tips, common pitfalls, debugging checklist, errno reference, and tools summary. |

## Source Files Read
- Required project guidance:
  - `Linux-Device-Driver/CODEX.md`
  - `Linux-Device-Driver/LEARNING_PATH.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
  - `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `ldd1-source-root`: searched with V4L2/userspace/tool/debug/compliance terms.
  - Result: no direct V4L2 source in book 1.
  - `ldd1-ch04` has generic ioctl/poll material; mapped as prerequisite only.
  - `ldd1-ch21` has framebuffer userspace access; mapped to topic 29.
- `ldd2-ch07`: relevant sections read.
  - `The videobuf2 Interface and APIs`, `Driver-specific Streaming Callbacks`, and V4L2/vb2 debug snippets around dynamic debug and `v4l2-ctl` tests.
- `ldd2-ch08`: userspace media-controller sections read.
  - `Media controller from user space`
  - `Using media-ctl`
  - UDOO/i.MX6 OV5640 pipeline command example
  - WaRP7/i.MX7 OV2680 media topology and pad format examples
- `ldd2-ch09`: full file read.
  - `Introduction to V4L2 from user space`
  - `The V4L2 user space API`
  - `Querying the device capabilities`
  - `Buffer management`
  - `Image (buffer) format`
  - `Requesting buffers`
  - `Requesting user pointer buffers`
  - `Requesting the memory mappable buffer`
  - `Requesting DMABUF buffers`
  - `Requesting read/write I/O memory`
  - `Enqueueing the buffer and enabling streaming`
  - `The concept of prime buffers`
  - `Enqueuing user pointer buffers`
  - `Enqueuing memory mappable buffers`
  - `Enqueuing DMABUF buffers`
  - `Enabling streaming`
  - `Dequeuing buffers`
  - `Read/write I/O`
  - `V4L2 user space tools`
  - `Using v4l2-ctl`
  - `Listing the video devices and their capabilities`
  - `Changing the device properties`
  - `Setting the pixel format, resolution, and frame rate`
  - `Capturing frames and streaming`
  - `Debugging V4L2 in user space`
  - `V4L2 compliance driver testing`
- `ldd2-ch14`: debug sections read.
  - `Message printing`, `Kernel log levels`, `Kernel log buffer`, `Adding timing information`
  - `Linux kernel tracing and performance analysis`
  - `Using Ftrace to instrument the code`
  - `Available tracers`, `The function tracer`, `The function_graph tracer`
  - `Function filters`, `Tracing events`, `Tracing a specific process with the Ftrace interface`
  - `Oops and panic analysis`, `Trace dump on oops`
- `notion-ch07-extra-part2`: debugging and test snippets read.
  - `7.8.3. Debugging Tips`
  - V4L2/vb2 dynamic debug commands
  - common issue table
  - `v4l2-ctl` quick smoke tests
- `notion-ch08-part2`: relevant media-controller userspace sections read.
  - `8.5. User Space Tools`
  - `media-ctl` topology, links, format, DOT graph, and complete pipeline examples
  - `v4l2-ctl` boundary with media controller
- `notion-ch09-extra-part1`: full file read.
  - `V4L2 User Space API Overview`
  - `Core Functions`
  - `Key ioctl Commands`
  - `V4L2 Data Structures`
  - `Typical Capture Sequence`
  - `ioctl Error Handling`
  - `Video Device Property Management`
  - `Format Negotiation`
  - `Frame Rate Control`
  - `Controls`
  - `Buffer Management`
  - `Buffer Types`
  - `Requesting Buffers`
  - `Query and Map Buffers`
  - `Buffer State and Flags`
- `notion-ch09-extra-part2`: full file read.
  - `Complete Capture Application`
  - `Initialization`, `Set Format`, `Request and Map Buffers`, `Prime Buffers and Start Streaming`, `Capture Loop`, `Stop and Cleanup`
  - `Error Handling Best Practices`
  - `V4L2 User Space Tools`
  - `Debugging with v4l2-ctl`
  - `v4l2-compliance Testing`
  - `SELECT/POLL for Non-blocking I/O`
  - `Multi-planar Buffers`
  - `DMABUF Zero-Copy Pipeline`
  - `Performance Optimization Tips`
  - `Common Pitfalls`
  - `Debugging Checklist`
  - `Quick Reference`
  - appendices for compile commands, FourCC formats, and errno

## Merged Source Notes
- Use `ldd2-ch09` as the technical spine because it walks through the canonical userspace sequence and the real `v4l2-ctl`, debug, and compliance commands.
- Use Notion Chapter 9 as the learner mental model and checklist source. It explains the same flow more clearly: `open -> QUERYCAP -> S_FMT -> REQBUFS -> QUERYBUF/mmap -> QBUF all -> STREAMON -> DQBUF/process/QBUF loop -> STREAMOFF -> cleanup`.
- Preserve the book's warning that V4L2 applications must check the values granted by the driver after `VIDIOC_S_FMT`, `VIDIOC_S_PARM`, and `VIDIOC_REQBUFS`; Notion repeats this as a core pitfall.
- Merge the buffer-ownership model from both sources:
  - userspace owns dequeued buffers;
  - after `VIDIOC_QBUF`, the buffer is locked and owned by the driver/vb2 for hardware access;
  - `VIDIOC_DQBUF` returns a done buffer to userspace;
  - accessing a queued/locked buffer from userspace is undefined.
- Keep the "incoming/outgoing queue" wording from current kernel docs, while translating the book/Notion "INPUT/OUTPUT queue" explanation carefully to avoid confusion with V4L2 `VIDEO_OUTPUT` devices.
- Merge three I/O methods:
  - MMAP: common path where driver allocates buffers and userspace maps them using offsets from `VIDIOC_QUERYBUF`.
  - USERPTR: userspace allocates buffers and passes pointers through `VIDIOC_QBUF`; `VIDIOC_REQBUFS` selects/configures the method.
  - DMABUF: buffers are imported by passing file descriptors to `VIDIOC_QBUF`; exporting MMAP buffers via `VIDIOC_EXPBUF` is a separate operation useful for zero-copy pipelines.
- Merge tools into a practical order:
  - `media-ctl` first when the camera is media-controller based and links/pad formats must be configured.
  - `v4l2-ctl` for device discovery, formats, controls, and simple capture.
  - `ffmpeg` for converting raw captures and checking pixel-format interpretation.
  - `gst-typefind-1.0`/GStreamer for quick media-type checks and prototyping.
  - `v4l2-compliance` for ABI correctness and driver submission confidence.
- Merge debug sources in layers:
  - userspace symptom and command: `v4l2-ctl`, `v4l2-compliance`, `media-ctl`, errno, and capture loop behavior.
  - V4L2 core visibility: `/sys/class/video4linux/videoX/dev_debug`.
  - vb2/core logs: `videobuf2_v4l2` and `videobuf2_common` debug parameters where available.
  - driver logs: `dev_*`, `pr_*`, dynamic debug for `drivers/media/*`, and driver-specific files.
  - deeper timing and crash debugging: ftrace, event tracing, oops analysis, mostly deferred to topic 37.
- Keep `ldd2-ch08` and Notion Chapter 8 as boundary material: topic 34 should teach how to use `media-ctl` enough to make capture work, but topology internals, async matching, and entity/pad/link driver design stay in topic 33.
- Keep `ldd2-ch07` and Notion Chapter 7 as boundary material: topic 34 should explain userspace symptoms in terms of vb2/driver callbacks, but driver implementation belongs to topic 32.

## Source Differences
- `ldd1` has no direct V4L2 userspace/compliance material. Do not substitute framebuffer or generic char-device ioctl content for V4L2 userspace API coverage.
- `ldd2-ch09` targets Linux v4.19.x. Current V4L2 docs preserve the core ioctl sequence, but newer fields/capabilities exist, such as request API handling, buffer capabilities, cache-hint capabilities, orphaned buffer behavior, and `VIDIOC_REMOVE_BUFS`.
- `ldd2-ch09` says `VIDIOC_QUERYBUF` returns a physical address for MMAP. Current teaching should say it returns an offset/metadata used as the `mmap()` offset; userspace should not treat it as a CPU-usable physical address.
- `ldd2-ch09` describes `VIDIOC_REQBUFS` as allocating buffers across memory methods. Current docs clarify that for USERPTR and DMABUF it selects/configures the I/O method and internal structures; the actual memory is provided by userspace or a DMA-buf exporter.
- `ldd2-ch09` and Notion examples blur DMABUF import/export in places. Final learner docs should clearly separate:
  - `V4L2_MEMORY_DMABUF` in `REQBUFS`/`QBUF` means import DMA-buf fds into the queue;
  - `VIDIOC_EXPBUF` exports MMAP buffers from a device that supports export.
- `ldd2-ch09` uses old or typo-prone command spellings in a few places, such as line-wrapped `--set-fmtvideo`. Prefer current `v4l2-ctl --set-fmt-video=...` wording.
- Notion Chapter 9 provides complete code but is educational. It omits some production concerns such as `device_caps` vs `capabilities`, multi-planar buffer array initialization in all paths, robust signal/cleanup handling, and media-controller setup before streaming.
- Notion Chapter 9 suggests `sudo chmod 666 /dev/video0` in a checklist. Final learner docs should prefer proper device permissions, groups, udev rules, or temporary root testing, not broad persistent permissions.
- `ldd2-ch14` focuses on general kernel debugging and does not mention dynamic debug directly. Use it for log levels, `dev_*`, ftrace, and oops background; use Notion V4L2 sources and current media debugging docs for dynamic debug/dev_debug.
- Debug paths and module debug parameters may vary by kernel configuration and module build. Treat `/sys/module/videobuf2_*/*/debug` and dynamic debug paths as "if available" workflows.
- `v4l2-compliance` behavior evolves with v4l-utils. A driver should be tested with a recent v4l-utils build, especially before upstream submission.

## Gaps / Uncertainties
- Need current-kernel validation before creating learner examples:
  - exact `v4l2-ctl` option set for the installed `v4l-utils` version;
  - whether `videobuf2_v4l2` and `videobuf2_common` debug module parameters exist in the target kernel;
  - recommended dynamic debug selectors for current media/vb2 modules;
  - current `v4l2-compliance` options and how to run streaming/media topology tests;
  - exact multi-planar sample code shape with `struct v4l2_plane planes[VIDEO_MAX_PLANES]`.
- Need decide example scope later:
  - A userspace MMAP capture tool is appropriate and can be buildable without kernel modules.
  - A media-controller pipeline setup script can be shown as board-specific/template-only because entity names and pad indexes are hardware-specific.
  - A compliance/debug lab can use a real webcam or `vivid` if available, but `vivid` setup may require kernel config/module availability.
- Need avoid overloading topic 34:
  - Full driver-side vb2 callback implementation belongs to topic 32.
  - Async notifier, fwnode, media entity/pad/link implementation belongs to topic 33.
  - Full ftrace, crash dump, and kernel-debugging theory belongs to topic 37.
  - Topic 34 should still give enough debug workflow to diagnose V4L2 userspace symptoms.
- Need emphasize that passing `v4l2-compliance` does not prove image quality, timing correctness, power management, or board-specific pipeline correctness; it mainly validates API behavior.
- Need external validation for ffmpeg/GStreamer command variants if final docs include distro-specific pipeline examples.

## External Validation
- Used: https://docs.kernel.org/userspace-api/media/v4l/io.html
  - Validates V4L2 I/O method categories, read/write default behavior, and that streaming I/O methods are selected with `VIDIOC_REQBUFS`.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-querycap.html
  - Validates `VIDIOC_QUERYCAP`, `capabilities` versus `device_caps`, `V4L2_CAP_DEVICE_CAPS`, and capture/multi-planar capability flags.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html
  - Validates modern `VIDIOC_REQBUFS` semantics, granted buffer count, `EINVAL` for unsupported I/O method, buffer capabilities, orphaned buffers, and distinction between MMAP allocation and USERPTR/DMABUF method setup.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-querybuf.html
  - Validates `VIDIOC_QUERYBUF`, buffer index range, same buffer type as format/request, multi-planar `m.planes`, and MMAP buffer metadata.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html
  - Validates `VIDIOC_QBUF`/`VIDIOC_DQBUF`, incoming/outgoing queue behavior, buffer locking, `O_NONBLOCK`/`EAGAIN`, `V4L2_BUF_FLAG_ERROR`, DMABUF fd behavior, multi-planar requirements, and request API caveats.
- Used: https://docs.kernel.org/5.10/userspace-api/media/v4l/vidioc-streamon.html
  - Validates `VIDIOC_STREAMON`/`VIDIOC_STREAMOFF`, stream-type argument, DMA abort/finish behavior, queue clearing on streamoff, and restart semantics.
- Used: https://docs.kernel.org/6.18/process/debugging/media_specific_debugging_guide.html
  - Validates media-subsystem debugging workflow: `dev_debug`, `dev_dbg()`/`v4l2_dbg()`, dynamic debug, ftrace/debugfs, `v4l2-compliance`, media-topology compliance options, and `v4l2-ctl --log-status`.
- Used: https://man.archlinux.org/man/extra/v4l-utils/v4l2-ctl.1.en
  - Validates `v4l2-ctl` purpose as a V4L2 control/query tool and common device selection/version/help behavior for current v4l-utils packaging.
- Used: https://man.archlinux.org/man/extra/v4l-utils/v4l2-compliance.1.en
  - Validates `v4l2-compliance` purpose, device/media-device options, streaming tests, verbose/trace options, and the recommendation to use a recent v4l-utils build when testing drivers.

## Learning Content Brief
- Mental model:
  - V4L2 userspace is a strict contract, not just "read frames from a file."
  - `/dev/videoX` is the streaming/control endpoint; `/dev/mediaX` configures topology when the driver exposes a media graph; `/dev/v4l-subdevX` may expose sub-device controls/formats.
  - A capture application negotiates what it wants, checks what the driver granted, supplies or maps buffers, gives those buffers to the driver, then continuously takes filled buffers back and requeues them.
- Core mechanism:
  - `open()` returns the fd used for every V4L2 ioctl.
  - `VIDIOC_QUERYCAP` tells whether the opened node supports capture/output, streaming, read/write, and whether `device_caps` should be used.
  - Format negotiation uses `VIDIOC_ENUM_FMT`, `VIDIOC_G_FMT`, `VIDIOC_TRY_FMT`, `VIDIOC_S_FMT`, and optionally frame interval/stream parameter ioctls.
  - Controls use query/get/set control ioctls or `v4l2-ctl -L`, `--get-ctrl`, and `--set-ctrl`.
  - Streaming setup uses `VIDIOC_REQBUFS`, `VIDIOC_QUERYBUF`/`mmap` for MMAP, `VIDIOC_QBUF`, `VIDIOC_STREAMON`, `VIDIOC_DQBUF`, and `VIDIOC_STREAMOFF`.
  - Non-blocking capture uses `O_NONBLOCK` plus `select()`/`poll()` before `DQBUF`, with `EAGAIN` treated as "no buffer ready yet."
- Important structs/APIs:
  - Syscalls: `open()`, `close()`, `ioctl()`, `mmap()`, `munmap()`, `read()`, `write()`, `select()`, `poll()`.
  - Header: `<linux/videodev2.h>`.
  - Structures: `struct v4l2_capability`, `struct v4l2_fmtdesc`, `struct v4l2_format`, `struct v4l2_pix_format`, `struct v4l2_pix_format_mplane`, `struct v4l2_streamparm`, `struct v4l2_requestbuffers`, `struct v4l2_buffer`, `struct v4l2_plane`, `struct v4l2_exportbuffer`, `struct v4l2_queryctrl`, `struct v4l2_control`.
  - Capability flags: `V4L2_CAP_VIDEO_CAPTURE`, `V4L2_CAP_VIDEO_CAPTURE_MPLANE`, `V4L2_CAP_STREAMING`, `V4L2_CAP_READWRITE`, `V4L2_CAP_DEVICE_CAPS`.
  - Buffer types: `V4L2_BUF_TYPE_VIDEO_CAPTURE`, `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`, `V4L2_BUF_TYPE_VIDEO_OUTPUT`, `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`.
  - Memory types: `V4L2_MEMORY_MMAP`, `V4L2_MEMORY_USERPTR`, `V4L2_MEMORY_DMABUF`.
  - Buffer flags: `V4L2_BUF_FLAG_MAPPED`, `V4L2_BUF_FLAG_QUEUED`, `V4L2_BUF_FLAG_DONE`, `V4L2_BUF_FLAG_ERROR`, timestamp flags.
  - Tools: `v4l2-ctl`, `media-ctl`, `v4l2-compliance`, `yavta`, `ffmpeg`, `gst-typefind-1.0`, GStreamer pipelines, `dmesg`, dynamic debug, ftrace.
- Lifecycle/data flow:
  - Basic MMAP capture:
    - Open `/dev/videoX`.
    - Query capabilities and choose `device_caps` when `V4L2_CAP_DEVICE_CAPS` is set.
    - Enumerate formats and choose pixel format/resolution/FPS.
    - Set or try/set format; read back the granted result.
    - Request buffers with `V4L2_MEMORY_MMAP`.
    - Query each buffer and map it with `mmap()`.
    - Queue all buffers before starting the stream.
    - Call `VIDIOC_STREAMON`.
    - In the loop, `DQBUF`, verify index/flags/bytesused, process data, and `QBUF` promptly.
    - Call `VIDIOC_STREAMOFF`, unmap buffers, close fd.
  - Media-controller pipeline:
    - Inspect topology with `media-ctl -p`.
    - Reset links if appropriate.
    - Enable the sensor-to-capture links.
    - Set compatible pad formats along the path.
    - Verify topology and then stream from `/dev/videoX`.
  - Compliance/debug:
    - First reproduce with `v4l2-ctl --stream-mmap --stream-count=N`.
    - Enable device/core debug only as needed.
    - Run `v4l2-compliance -d /dev/videoX`; for MC drivers also use media-device tests such as `-m /dev/mediaX` or topology-only verbose checks.
- Examples to target later:
  - Small userspace MMAP capture program that saves N frames and handles `EINTR`, `EAGAIN`, cleanup, and format verification.
  - `v4l2-ctl` smoke-test script for list devices, query info, list formats, set format/FPS, capture one frame, and convert raw YUYV with ffmpeg.
  - Media-controller setup template using `media-ctl -p`, `--reset`, `--links`, and `-V`.
  - Compliance/debug checklist using `dev_debug`, dynamic debug, `dmesg`, and `v4l2-compliance`.
- Common bugs:
  - Calling ioctls in the wrong order.
  - Not zero-initializing V4L2 structs before ioctls.
  - Checking `cap.capabilities` incorrectly when `device_caps` is present.
  - Assuming `VIDIOC_S_FMT` accepted exactly the requested width/height/pixelformat.
  - Ignoring granted buffer count after `VIDIOC_REQBUFS`.
  - Forgetting `VIDIOC_QUERYBUF`/`mmap()` for MMAP buffers.
  - Starting streaming without priming enough buffers.
  - Calling `DQBUF` before `STREAMON` or without queued buffers.
  - Treating `EAGAIN` as a fatal error in non-blocking mode.
  - Accessing a buffer after `QBUF` before it is dequeued.
  - Mixing memory types between `REQBUFS`, `QBUF`, and `DQBUF`.
  - Forgetting multi-planar `struct v4l2_plane` arrays.
  - Saving raw frames without recording format/resolution/pixel layout.
  - Running `v4l2-compliance` without valid media links, pad formats, or signal input.
- Debugging notes:
  - Start with simple commands:
    - `v4l2-ctl --list-devices`
    - `v4l2-ctl -d /dev/video0 -D`
    - `v4l2-ctl -d /dev/video0 --all`
    - `v4l2-ctl -d /dev/video0 --list-formats-ext`
    - `v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10`
  - For media-controller pipelines:
    - `media-ctl -d /dev/media0 -p`
    - `media-ctl --print-dot > graph.dot`
    - check links and pad formats before blaming vb2.
  - For V4L2 ioctl tracing:
    - `echo 0x3 > /sys/class/video4linux/video0/dev_debug` if supported.
  - For vb2/core traces:
    - try module debug parameters for `videobuf2_v4l2` and `videobuf2_common` if available.
    - use dynamic debug selectors for `drivers/media/v4l2-core/*`, `drivers/media/common/videobuf2/*`, or a specific driver source file.
  - For kernel-wide timing:
    - use `dmesg`, printk timestamps, ftrace `function`/`function_graph`, filters, and event tracing.
  - For driver correctness:
    - run `v4l2-compliance -d /dev/videoX --verbose`.
    - for MC drivers, include `/dev/mediaX` compliance checks.
- Production concerns:
  - Keep userspace ABI stable and standards-compliant; avoid private controls/ioctls when V4L2 already has a standard interface.
  - Use enough buffers to avoid drops, but do not over-buffer if latency matters.
  - Split capture and heavy processing into separate threads or queues when processing time approaches frame interval.
  - Prefer DMABUF for camera-to-display/encoder zero-copy paths, but validate synchronization, ownership, and importer/exporter support.
  - Treat raw frame files as metadata-poor: always track width, height, pixel format, stride, and frame count.
  - For upstream or production drivers, compliance tests are necessary but not sufficient; also test real streaming, media graph setup, controls, suspend/resume, unplug/remove, error recovery, and performance.
- Interview angles:
  - Walk through the V4L2 userspace capture sequence and explain why each step exists.
  - Why must applications check the format and buffer count returned by the driver?
  - Compare MMAP, USERPTR, and DMABUF from application and driver perspectives.
  - What happens if userspace accesses a queued buffer?
  - Why does `VIDIOC_DQBUF` block, and how do `O_NONBLOCK`, `select()`, and `poll()` change the workflow?
  - How do you debug `DQBUF` blocking forever?
  - How do `media-ctl`, `v4l2-ctl`, and `v4l2-compliance` differ?
  - What does `dev_debug` show, and when would you use dynamic debug or ftrace?
  - What does it mean for a V4L2 driver to pass compliance, and what does compliance not prove?
  - How would you diagnose wrong colors, corrupted frames, dropped frames, `STREAMON` failure, or `v4l2-compliance` control failures?
