# Topic Brief - 32 - V4L2 Core, Video Device, And VB2 Capture

## Output Targets
- Knowledge: `knowledge/32-v4l2-core-video-device-and-vb2-capture.md`
- Interview: `interview/32-v4l2-core-video-device-and-vb2-capture.md`
- Example: `examples/32-v4l2-core-video-device-and-vb2-capture/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/no relevant V4L2 source found | No V4L2, `video_device`, `v4l2_device`, or `vb2` chapter found. Search hits for "capture" were IIO buffered capture and framebuffer/display material, which belong to topics 25 and 29 rather than V4L2 capture. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | sampled/mapped/out-of-scope | Confirmed this is legacy framebuffer/display memory access, not V4L2 capture. Keep separate under topic 29. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/covered/merged | Primary source for V4L2 architecture, `struct v4l2_device`, `struct video_device`, bridge driver role, file operations, ioctl dispatch, `struct vb2_queue`, `struct vb2_buffer`, buffer states, planes, vb2 memory backends, `struct vb2_ops`, queue init, streaming callbacks, DMA completion, sub-device basics, and controls. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/related-topic | Mostly belongs to topic 33. Only the media-controller relation to `v4l2_device`, `video_device`, and `v4l2_subdev` was read and mapped here as boundary context. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/covered/merged | Supporting source for the userspace side of the same vb2 lifecycle: ioctl order, format negotiation, `VIDIOC_REQBUFS`, `VIDIOC_QUERYBUF`, `VIDIOC_QBUF`, `VIDIOC_DQBUF`, `VIDIOC_STREAMON/OFF`, MMAP/USERPTR/DMABUF, prime buffers, blocking behavior, V4L2 debug knobs, and `v4l2-compliance`. Much of this belongs to topic 34, but the buffer lifecycle validates topic 32. |
| `notion-ch07-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part1.md` | read/mapped/covered/merged | Teaching-oriented restatement of V4L2 core architecture: four core structs, bridge-vs-subdev split, file ops, `video_ioctl2`, vb2 ioctl helpers, and a minimal `querycap`/format/ioctl skeleton. |
| `notion-ch07-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/covered/merged | Detailed vb2 callback walkthrough with concrete examples for `queue_setup`, `buf_prepare`, `buf_queue`, `start_streaming`, `stop_streaming`, DMA IRQ completion, custom buffer struct, common pitfalls, performance notes, debugging commands, and a minimal bridge-driver appendix. |
| `notion-ch08-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md` | mapped/related-topic | Not read in full for topic 32. Search and headings show it covers V4L2 async, fwnode endpoint parsing, and bridge/sub-device binding. Defer to topic 33, with only "register video device after async completion" as a boundary note. |
| `notion-ch08-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2.md` | mapped/related-topic | Not read in full for topic 32. Search and headings show media controller entities/pads/links and userspace media graph setup. Defer to topic 33. |
| `notion-ch09-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part1.md` | read/mapped/covered/merged | Supporting source for userspace view of V4L2 buffer queues, capture sequence, ioctl error handling, format negotiation, controls, memory models, `REQBUFS`, `QUERYBUF`, and buffer flags. |
| `notion-ch09-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md` | read/mapped/covered/merged | Supporting source for complete MMAP capture flow, prime-buffer sequence, capture loop, cleanup, tools, vb2 debug commands, compliance testing, poll/select, multi-planar buffers, DMABUF zero-copy, performance pitfalls, and errno reference. Much of this belongs to topic 34. |

## Source Files Read
- Required project guidance:
  - `Linux-Device-Driver/CODEX.md`
  - `Linux-Device-Driver/LEARNING_PATH.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
  - `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `ldd1-source-root`: searched with V4L2/video/vb2/capture terms.
  - Result: no direct V4L2 source. `ldd1-ch10` contains IIO triggered-buffer capture, not V4L2. `ldd1-ch21` is framebuffer/display access, not V4L2 capture.
- `ldd1-ch21`: sampled first framebuffer sections.
  - Relevant only as an out-of-scope contrast: framebuffer exposes `/dev/fbX` display memory, while V4L2 capture exposes `/dev/videoX` and streaming buffer ioctls.
- `ldd2-ch07`: full file read.
  - Relevant sections: `Framework Architecture and the Main Data Structures`, `Initializing and Registering a V4L2 Device`, `Introducing Video Device Drivers - The Bridge Driver`, `Initializing and Registering the Video Device`, `Video Device File Operations`, `V4L2 ioctl Handling`, `The videobuf2 Interface and APIs`, `Concept of Buffers`, `The Concept of Planes`, `The Concept of Queue`, `Driver-specific Streaming Callbacks`, `Initializing and Releasing the vb2 Queue`.
  - Boundary sections read but mostly deferred to topic 33: `The Concept of Sub-devices`, `Sub-device Initialization`, `Sub-device Operations`, `Calling Sub-device Operations`, `Traditional Sub-device (Un)registration`, `V4L2 Controls Infrastructure`.
- `ldd2-ch08`: relevant media-controller excerpt read.
  - Relevant sections: `The Linux media controller framework`, `The media controller abstraction model`, `V4L2 device abstraction`.
  - Defer full async/fwnode/media graph detail to topic 33.
- `ldd2-ch09`: full file read.
  - Relevant sections: `The V4L2 user space API`, `Buffer management`, `Image (buffer) format`, `Requesting buffers`, `Requesting user pointer buffers`, `Requesting the memory mappable buffer`, `Requesting DMABUF buffers`, `Enqueueing the buffer and enabling streaming`, `The concept of prime buffers`, `Enabling streaming`, `Dequeuing buffers`, `Debugging V4L2 in user space`, `V4L2 compliance driver testing`.
- `notion-ch07-extra-part1`: full file read.
  - Relevant sections: `7.1. Giới Thiệu V4L2 Framework`, `7.2. Bốn Cấu Trúc Dữ Liệu Cốt Lõi`, `7.3. Bridge Driver - File Operations`, `7.4. Bridge Driver - IOCTL Operations`.
- `notion-ch07-extra-part2`: full file read.
  - Relevant sections: `7.5. videobuf2 Framework - Chi Tiết`, `Buffer Concept và Lifecycle`, `struct vb2_ops - Driver Callbacks`, `DMA Completion Handling`, `Custom Buffer Structure`, `Debugging Tips`, `Checklist viết V4L2 driver`, `Common Pitfalls`, `Performance Tips`, `PHỤ LỤC: Complete Minimal Bridge Driver`.
  - Boundary sections read but mostly deferred to topic 33: `Sub-Device Drivers`, `V4L2 Controls Infrastructure`.
- `notion-ch09-extra-part1`: full file read.
  - Relevant sections: `V4L2 User Space API Overview`, `Typical Capture Sequence`, `ioctl Error Handling`, `Video Device Property Management`, `Buffer Management`, `Buffer Types`, `Requesting Buffers`, `Query and Map Buffers`, `Buffer State và Flags`.
- `notion-ch09-extra-part2`: full file read.
  - Relevant sections: `Complete Capture Application`, `Prime Buffers and Start Streaming`, `Capture Loop`, `Stop and Cleanup`, `Error Handling Best Practices`, `V4L2 User Space Tools`, `Debugging with v4l2-ctl`, `v4l2-compliance Testing`, `SELECT/POLL`, `Multi-planar Buffers`, `DMABUF Zero-Copy Pipeline`, `Common Pitfalls`, `Debugging Checklist`.

## Merged Source Notes
- The core topic should be built from `ldd2-ch07`, using Notion chapter 7 as the clearer beginner explanation and chapter 9 sources as lifecycle validation from userspace.
- `ldd2-ch07` gives the authoritative structure: `v4l2_device` as the top-level V4L2 container, `video_device` as the `/dev/videoX` abstraction, and `vb2_queue` as the central streaming/DMA queue owned by the bridge driver.
- `notion-ch07-extra-part1` preserves the same structure but improves the mental model: V4L2 core handles registration/ioctl dispatch, the bridge driver owns DMA and `/dev/videoX`, the sub-device owns sensor/control logic, and vb2 owns buffer state transitions.
- `notion-ch07-extra-part2` adds practical callback and failure-path details missing from `ldd2-ch07`: validate plane size in `buf_prepare`, keep a private driver DMA list in `buf_queue`, return all queued buffers with `VB2_BUF_STATE_ERROR` on `start_streaming` failure or `stop_streaming`, and use IRQ completion to set timestamp/sequence and call `vb2_buffer_done()`.
- `ldd2-ch09` and Notion chapter 9 are not primary driver-writing sources, but they are essential for understanding what the driver must satisfy. Merge only the parts that explain how userspace drives the same kernel path: `REQBUFS -> QUERYBUF/mmap -> QBUF -> STREAMON -> DQBUF/QBUF loop -> STREAMOFF`.
- Topic 32 should include enough userspace ioctl context to make vb2 driver callbacks meaningful, but the full userspace application, `v4l2-ctl`, `v4l2-compliance`, ffmpeg/GStreamer, and debugging workflow belong mostly to topic 34.
- Sub-devices and controls appear in `ldd2-ch07` and both Notion V4L2 chapter files. For topic 32, merge only what the bridge driver must know: `start_streaming` may call `v4l2_subdev_call(..., video, s_stream, 1)`, and control handlers may be attached to `v4l2_device`/`video_device`. Async discovery, media pads, fwnode endpoints, and control-depth are topic 33.
- The `ldd2-ch08` media-controller excerpt clarifies that `video_device` and `v4l2_subdev` embed `media_entity` under `CONFIG_MEDIA_CONTROLLER`, but that topology model should stay as related context here.

## Source Differences
- `ldd1` has no direct V4L2 chapter. Do not infer coverage from framebuffer or IIO "capture" material. Framebuffer is display memory access through `/dev/fbX`; IIO buffers are sensor data buffers through IIO, not V4L2 video capture.
- `ldd2-ch07` is based on Linux kernel v4.19.x. Some API names and signatures have changed since then, especially around sub-device pad state/config APIs and modern media-controller helpers. Validate before producing buildable examples.
- `ldd2-ch07` emphasizes `video_register_device()`/`video_unregister_device()`. Current kernel docs note a vb2-specific unregister helper when a driver uses vb2 file-operation release helpers. This should be checked before final example code.
- Internal docs sometimes simplify `DMABUF`. For current V4L2, `V4L2_MEMORY_DMABUF` generally configures a queue to import buffers allocated elsewhere, while `VIDIOC_EXPBUF` exports MMAP buffers. Preserve the distinction in final learner docs.
- `ldd2-ch09` describes `VIDIOC_REQBUFS` as allocating buffers for all memory models. Current docs clarify that USERPTR and DMABUF configure the queue/method and internal structures; actual memory is allocated by userspace or another exporter.
- `ldd2-ch07` states `VB2_MEMORY_DMABUF` as indicating memory allocated by the driver and exported as a DMABUF file handle. Treat this as imprecise for current teaching; validate against current docs and distinguish export/import paths.
- Notion examples are intentionally educational and concise. They may omit current kernel details such as `device_caps`, `V4L2_CAP_DEVICE_CAPS`, `video_set_drvdata()`, current `remove` signatures, media-device cleanup, runtime PM, and exact subdev API signatures.
- Notion `Chapter 7-v4l2_part2.md` states `struct vb2_v4l2_buffer vb` "must be first" in a custom buffer struct. Current docs say the first field should be the subsystem-specific buffer struct; keep this rule but verify exact wording and helper use.
- Chapter 9 sources use "INPUT queue" and "OUTPUT queue" to explain capture buffers. This is useful for learners but can conflict with V4L2 `VIDEO_OUTPUT` terminology. In final docs, call them "driver-owned incoming/queued buffers" and "done buffers ready for userspace" to avoid confusion.

## Gaps / Uncertainties
- Need current kernel validation before writing examples:
  - `struct vb2_ops` callback signatures and required callbacks.
  - `struct v4l2_file_operations` and `video_ioctl2` usage.
  - `vb2_fop_release()` versus `_vb2_fop_release()` and `vb2_video_unregister_device()`.
  - Whether `q->dev`, `q->lock`, `q->timestamp_flags`, `q->buf_struct_size`, and `q->min_buffers_needed` should be mandatory in the example for the target kernel.
  - Correct helper for driver-private data: `video_set_drvdata()`, `video_drvdata()`, and `vb2_get_drv_priv()`.
- Need final boundary decision:
  - Topic 32 should teach `v4l2_device`, `video_device`, file/ioctl ops, vb2 queue, buffers, streaming, DMA handoff.
  - Topic 33 should teach async notifier, subdev registration, fwnode endpoints, media entities/pads/links, and controls in depth.
  - Topic 34 should teach userspace tools, capture apps, compliance, and debug workflows in depth.
- Need decide example scope later:
  - A fake/minimal V4L2 bridge driver may not produce real frames without hardware or a timer/workqueue frame generator.
  - A production-style CSI/bridge example would require hardware-specific registers and is too large.
  - A learner example should be explicitly marked learning-only and validated against local kernel headers.
- Need external validation for current V4L2 control inheritance if controls are included in topic 32 at all. It may be cleaner to defer detailed controls to topic 33.
- Need confirm current dynamic debug paths/module parameters for `videobuf2_v4l2`, `videobuf2_common`, and `videodev`; module names and paths may vary by kernel config.

## External Validation
- Used: https://docs.kernel.org/driver-api/media/v4l2-videobuf2.html
  - Validates current videobuf2 concepts, `struct vb2_queue`, `struct vb2_ops`, memory operations, driver-private buffer struct rule, and the note that drivers using vb2 file-op release helpers should use the vb2-aware video unregister helper.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html
  - Validates `VIDIOC_REQBUFS`, memory models, granted buffer count semantics, unsupported method returning `EINVAL`, buffer reallocation/freeing behavior, and current `v4l2_requestbuffers.capabilities`.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html
  - Validates `VIDIOC_QBUF`/`VIDIOC_DQBUF`, valid buffer index range, multi-planar `struct v4l2_plane` use, and queue/dequeue behavior.
- Used: https://docs.kernel.org/userspace-api/media/v4l/vidioc-streamon.html
  - Validates `VIDIOC_STREAMON`/`VIDIOC_STREAMOFF`, queued buffer behavior, stream-type argument, DMA abort/finish semantics, and removal of queued/done buffers on streamoff.
- Used: https://docs.kernel.org/userspace-api/media/v4l/io.html
  - Validates the broader userspace I/O methods: read/write, streaming I/O with MMAP/USERPTR/DMABUF, and why streaming setup starts with `VIDIOC_REQBUFS`.
- Used: https://docs.kernel.org/driver-api/media/v4l2-intro.html
  - Validates the high-level V4L2 model: bridge drivers, connected sensor ICs, sub-device framework, and why video nodes plus buffer handling need shared framework code.

## Learning Content Brief
- Mental model:
  - V4L2 capture is a contract between userspace, V4L2 core, a bridge driver, optional sub-devices, and hardware DMA.
  - Userspace sees `/dev/videoX` and uses `VIDIOC_*` ioctls.
  - The bridge driver owns the capture interface and DMA engine.
  - `vb2` owns the generic buffer state machine, memory models, mmap/poll/read helpers, and queue/dequeue/streaming ioctl plumbing.
  - The driver mostly fills in hardware-specific callbacks: format negotiation, buffer sizing, DMA queueing, start/stop streaming, and IRQ completion.
- Core mechanism:
  - `v4l2_device` is the root container registered against the parent `struct device`.
  - `video_device` creates the userspace video node and connects file operations, ioctl operations, `device_caps`, queue pointer, locks, release callback, and V4L2 parent.
  - `v4l2_file_operations` routes system calls. Typical bridge drivers use `video_ioctl2`, `vb2_fop_mmap`, `vb2_fop_poll`, and a release helper.
  - `v4l2_ioctl_ops` exposes V4L2 ioctls. Format/capability ioctls are driver-specific; buffer/streaming ioctls often delegate to `vb2_ioctl_*`.
  - `vb2_queue` connects V4L2 buffer ioctls to driver-owned DMA callbacks through `vb2_ops`.
- Important structs/APIs:
  - Core registration: `struct v4l2_device`, `v4l2_device_register()`, `v4l2_device_unregister()`.
  - Video node: `struct video_device`, `video_device_alloc()`, `video_register_device()`, `video_unregister_device()`, `video_device_release()`, `video_device_release_empty()`.
  - File/ioctl ops: `struct v4l2_file_operations`, `struct v4l2_ioctl_ops`, `video_ioctl2`, `vb2_fop_mmap`, `vb2_fop_poll`, `vb2_fop_read`, `vb2_fop_release`, `vb2_ioctl_reqbufs`, `vb2_ioctl_querybuf`, `vb2_ioctl_qbuf`, `vb2_ioctl_dqbuf`, `vb2_ioctl_streamon`, `vb2_ioctl_streamoff`.
  - Buffers/queues: `struct vb2_queue`, `struct vb2_buffer`, `struct vb2_v4l2_buffer`, `struct vb2_ops`, `vb2_queue_init()`, `vb2_get_drv_priv()`, `to_vb2_v4l2_buffer()`, `vb2_buffer_done()`, `vb2_set_plane_payload()`, `vb2_plane_size()`.
  - Memory backends: `vb2_dma_contig_memops`, `vb2_dma_sg_memops`, `vb2_vmalloc_memops`; user-visible `V4L2_MEMORY_MMAP`, `V4L2_MEMORY_USERPTR`, `V4L2_MEMORY_DMABUF`; driver-visible `VB2_MMAP`, `VB2_USERPTR`, `VB2_DMABUF`, `VB2_READ`.
  - Buffer types and capabilities: `V4L2_BUF_TYPE_VIDEO_CAPTURE`, `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`, `V4L2_CAP_VIDEO_CAPTURE`, `V4L2_CAP_STREAMING`, `V4L2_CAP_READWRITE`, `V4L2_CAP_DEVICE_CAPS`.
- Lifecycle/data flow:
  - Driver probe allocates state, registers `v4l2_device`, initializes `vb2_queue`, initializes/fills `video_device`, and registers `/dev/videoX`.
  - Userspace opens `/dev/videoX`, queries capabilities, negotiates format, requests buffers, maps or supplies buffers, queues empty buffers, and starts streaming.
  - `VIDIOC_REQBUFS` calls into vb2 and driver `queue_setup` to decide buffer count, plane count, and plane sizes.
  - `VIDIOC_QBUF` prepares a buffer, marks it active, and calls driver `buf_queue` so the driver can add it to its DMA list.
  - `VIDIOC_STREAMON` calls driver `start_streaming`; the driver enables sensor/subdev stream if needed, configures DMA with the first queued buffer, enables IRQs, and starts hardware.
  - DMA completion IRQ removes the completed buffer from the driver list, sets timestamp/sequence/payload, calls `vb2_buffer_done(..., VB2_BUF_STATE_DONE)`, and programs the next buffer if available.
  - Userspace `VIDIOC_DQBUF` receives a done buffer, processes it, then `VIDIOC_QBUF` returns it to the driver.
  - `VIDIOC_STREAMOFF` calls driver `stop_streaming`; the driver stops DMA/IRQs/subdev streaming and returns every outstanding buffer to vb2, usually with `VB2_BUF_STATE_ERROR`.
- Examples to target later:
  - Minimal learning-only V4L2 bridge skeleton with `v4l2_device`, embedded `video_device`, embedded `vb2_queue`, `querycap`, one fixed format, vb2 MMAP queue, and no real hardware.
  - Optional timer/workqueue-generated fake frames to make `DQBUF` observable without camera hardware.
  - Debug workflow using `v4l2-ctl --stream-mmap --stream-count=N`, dynamic debug, and `v4l2-compliance`.
- Common bugs:
  - Registering `video_device` without a valid `release` callback.
  - Forgetting to set `vdev->v4l2_dev`, `vdev->fops`, `vdev->ioctl_ops`, or `vdev->queue`.
  - Returning success from `start_streaming` before hardware and buffers are ready.
  - Not returning all queued buffers on `start_streaming` failure or `stop_streaming`.
  - Wrong buffer size in `queue_setup` or failing to validate payload in `buf_prepare`.
  - Accessing a userspace buffer while it is queued/owned by the driver.
  - Using MMAP/USERPTR/DMABUF inconsistently between `REQBUFS`, `QBUF`, and `DQBUF`.
  - Starting stream before priming enough buffers.
  - Doing heavy processing in IRQ context.
  - Confusing capture/output direction with explanatory "input/output queue" wording.
- Debugging notes:
  - Start from userspace: `v4l2-ctl --list-devices`, `v4l2-ctl -d /dev/video0 -D`, `v4l2-ctl --list-formats-ext`, and a short `--stream-mmap` capture.
  - Enable vb2/core traces where available: module debug parameters for `videobuf2_v4l2` and `videobuf2_common`, or dynamic debug on `drivers/media/*`.
  - Enable per-video-node ioctl tracing through `/sys/class/video4linux/videoX/dev_debug` where supported.
  - Run `v4l2-compliance -d /dev/videoX` before considering a driver correct.
  - Log state transitions in driver callbacks: queue setup, buffer prepare/queue, start/stop streaming, IRQ completion, and error returns.
- Production concerns:
  - Validate current kernel APIs before copying examples; the source material is v4.19-era and some APIs moved.
  - Keep buffer ownership strict: user owns dequeued buffers; driver/vb2 owns queued and active buffers.
  - Use the correct memory backend for hardware: contiguous DMA for simple DMA engines, scatter-gather for SG-capable hardware, vmalloc usually for non-DMA/USB-like paths.
  - Use locks carefully: queue/device mutexes for ioctl serialization, spinlocks for IRQ-side driver buffer lists, no sleeping in hard IRQ context.
  - Stop hardware before unregistering video nodes or freeing queues; prevent userspace from racing with teardown.
  - Handle sub-device streaming and runtime PM ordering if the capture path depends on sensors/clocks/regulators.
- Interview angles:
  - Explain why V4L2 uses `v4l2_device`, `video_device`, and `vb2_queue` instead of a plain character driver.
  - Walk through `VIDIOC_REQBUFS -> QBUF -> STREAMON -> DQBUF` from userspace down to driver callbacks.
  - Explain what `vb2_buffer_done()` does and when the driver calls it.
  - Compare MMAP, USERPTR, and DMABUF from both userspace and driver perspectives.
  - Explain what must happen on `stop_streaming` and why returning all buffers matters.
  - Explain bridge driver versus sub-device driver responsibilities.
  - Diagnose `STREAMON` failure, `DQBUF` blocking forever, corrupted frames, dropped frames, and `v4l2-compliance` failures.
