# 34 - V4L2 Userspace, Debugging, And Compliance Interview Questions

Strong candidates should reason from a userspace symptom down to V4L2 ioctl state, vb2 buffer ownership, media-controller topology, and driver behavior. Good answers are concrete: commands, structs, error codes, ownership, and debug evidence.

## Beginner

### 1. What Is The V4L2 Userspace API?

- **Level:** Beginner
- **Question:** What is the V4L2 userspace API, and why does it use ioctls?
- **Short Answer:** It is the standard Linux userspace ABI for video devices. It uses `/dev/videoX` plus ioctls so applications can query capabilities, configure formats and controls, manage buffers, and start/stop streaming.
- **Deep Explanation:** Video capture is more structured than reading bytes from a file. Userspace must know what formats are supported, choose resolution and frame rate, request buffers, hand buffers to the driver, wait for completed frames, and return buffers. V4L2 standardizes this contract so tools like `v4l2-ctl`, GStreamer, ffmpeg, OpenCV, and custom apps can work with many drivers.
- **API / Code Anchor:**
  ```text
  open("/dev/video0")
  VIDIOC_QUERYCAP
  VIDIOC_S_FMT
  VIDIOC_REQBUFS
  VIDIOC_QBUF / VIDIOC_DQBUF
  VIDIOC_STREAMON / VIDIOC_STREAMOFF
  ```
- **Production or Debugging Angle:** Start bring-up with `v4l2-ctl -d /dev/video0 -D` and `v4l2-ctl -d /dev/video0 --list-formats-ext` before writing custom userspace code.
- **Common Traps:**
  - Saying V4L2 is just `read()` from `/dev/video0`.
  - Ignoring that driver behavior is part of a stable userspace ABI.
  - Confusing V4L2 capture with framebuffer display.
  - Assuming every `/dev/videoX` node is a capture node.
- **Follow-up Questions:**
  - What does `VIDIOC_QUERYCAP` return?
  - Why is `VIDIOC_S_FMT` needed before streaming?
  - What is `/dev/mediaX` used for?

### 2. Walk Through A Basic Capture Sequence

- **Level:** Beginner
- **Question:** What is the normal V4L2 MMAP capture sequence?
- **Short Answer:** Open the device, query capabilities, set format, request buffers, query and map buffers, queue all buffers, start streaming, loop over dequeue/process/requeue, then stop streaming and clean up.
- **Deep Explanation:** Each step prepares the next state. `QUERYCAP` confirms the node supports capture and streaming. `S_FMT` selects a format, but the driver may adjust it. `REQBUFS` creates/configures the buffer pool. `QUERYBUF` and `mmap()` map driver-allocated buffers. `QBUF` gives buffers to the driver. `STREAMON` starts capture. `DQBUF` returns completed frames. Requeueing keeps the driver supplied with buffers.
- **API / Code Anchor:**
  ```text
  QUERYCAP -> S_FMT -> REQBUFS -> QUERYBUF/mmap
  -> QBUF x N -> STREAMON
  -> DQBUF -> process -> QBUF
  -> STREAMOFF
  ```
- **Production or Debugging Angle:** If `DQBUF` blocks forever, first check whether userspace queued buffers before `STREAMON`.
- **Common Traps:**
  - Calling `DQBUF` before `STREAMON`.
  - Forgetting to queue buffers before streaming.
  - Not requeueing after processing.
  - Thinking `REQBUFS` starts streaming.
- **Follow-up Questions:**
  - What does `VIDIOC_QUERYBUF` do?
  - What happens when `STREAMOFF` is called?
  - Why should buffers be primed?

### 3. What Is Buffer Ownership In V4L2?

- **Level:** Beginner
- **Question:** Who owns a buffer after `VIDIOC_QBUF`, and when can userspace safely access it?
- **Short Answer:** After `VIDIOC_QBUF`, the buffer is owned by vb2/driver/hardware and userspace must not access it. Userspace can safely access the buffer only after `VIDIOC_DQBUF` returns it.
- **Deep Explanation:** `QBUF` queues an empty capture buffer to the driver. The driver or hardware may DMA into it at any time. `DQBUF` returns a completed or errored buffer to userspace. Accessing the memory while queued is undefined because userspace may race with hardware writes or cache synchronization.
- **API / Code Anchor:**
  ```text
  QBUF  -> buffer locked / driver-owned
  DQBUF -> buffer userspace-owned
  ```
- **Production or Debugging Angle:** Corrupted frames can come from userspace reading or writing a queued buffer, especially in multi-threaded applications.
- **Common Traps:**
  - Treating `QBUF` as a copy.
  - Processing a buffer after requeueing it.
  - Keeping a pointer to a queued buffer in a worker thread.
  - Ignoring `V4L2_BUF_FLAG_ERROR`.
- **Follow-up Questions:**
  - What does `V4L2_BUF_FLAG_QUEUED` mean?
  - What should an app do after processing a dequeued buffer?
  - Can a dequeued buffer contain bad data?

### 4. What Are `v4l2-ctl`, `media-ctl`, And `v4l2-compliance`?

- **Level:** Beginner
- **Question:** How do `v4l2-ctl`, `media-ctl`, and `v4l2-compliance` differ?
- **Short Answer:** `v4l2-ctl` queries/configures/streams from V4L2 nodes, `media-ctl` inspects and configures media-controller topology, and `v4l2-compliance` tests whether the driver follows the V4L2 API.
- **Deep Explanation:** `v4l2-ctl` works mostly on `/dev/videoX` and optional subdev nodes for controls, formats, and capture tests. `media-ctl` works on `/dev/mediaX`, where it shows entities, pads, links, and pad formats. `v4l2-compliance` runs many ioctl checks and reports ABI failures. They answer different questions: "what can this node do?", "is the pipeline connected?", and "does this driver obey the API?"
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --all
  media-ctl -d /dev/media0 -p
  v4l2-compliance -d /dev/video0 --verbose
  ```
- **Production or Debugging Angle:** For an embedded camera, use `media-ctl` before streaming if links and pad formats are not fixed by the driver.
- **Common Traps:**
  - Running only `v4l2-ctl` on a media-controller pipeline with inactive links.
  - Treating compliance as an image-quality test.
  - Ignoring `/dev/videoX` node type.
  - Assuming `media-ctl` captures frames.
- **Follow-up Questions:**
  - Which tool lists supported pixel formats?
  - Which tool shows pads and links?
  - Which tool should run before upstream submission?

## Mid-level

### 5. Why Must Userspace Check The Returned Format?

- **Level:** Mid-level
- **Question:** Why should userspace inspect `struct v4l2_format` after `VIDIOC_S_FMT`?
- **Short Answer:** The driver may adjust unsupported width, height, pixel format, stride, field, or `sizeimage`. The returned struct is the actual format userspace must handle.
- **Deep Explanation:** V4L2 format negotiation is not "set exactly or fail" in every case. Drivers may clamp sizes, choose aligned strides, or select the closest supported format. If userspace assumes the requested values were accepted, it may allocate wrong buffers, convert raw frames with the wrong pixel format, or misinterpret stride.
- **API / Code Anchor:**
  ```c
  xioctl(fd, VIDIOC_S_FMT, &fmt);
  printf("%ux%u fourcc=%.4s sizeimage=%u\n",
         fmt.fmt.pix.width, fmt.fmt.pix.height,
         (char *)&fmt.fmt.pix.pixelformat,
         fmt.fmt.pix.sizeimage);
  ```
- **Production or Debugging Angle:** Wrong colors or truncated frames often begin with ignoring the granted format, `bytesperline`, or `sizeimage`.
- **Common Traps:**
  - Allocating buffers from requested, not returned, dimensions.
  - Ignoring stride.
  - Treating Bayer data as YUYV.
  - Forgetting multi-planar formats use `fmt.pix_mp`.
- **Follow-up Questions:**
  - What does `VIDIOC_TRY_FMT` do?
  - How does `bytesperline` affect raw frame conversion?
  - Why can `S_FMT` affect buffer size?

### 6. Compare MMAP, USERPTR, And DMABUF

- **Level:** Mid-level
- **Question:** Compare the three main V4L2 streaming memory models.
- **Short Answer:** MMAP uses driver/vb2-allocated buffers mapped into userspace. USERPTR uses userspace-allocated buffers passed to the driver. DMABUF imports shared DMA buffer file descriptors, typically for zero-copy pipelines.
- **Deep Explanation:** MMAP is the common default for capture because the driver controls buffer allocation and userspace maps it. USERPTR lets the app manage memory but the driver must pin/handle user pages. DMABUF is for sharing buffers across devices, such as camera to DRM display or hardware encoder. `VIDIOC_REQBUFS` allocates MMAP buffers, but for USERPTR and DMABUF it mainly selects/configures that I/O mode.
- **API / Code Anchor:**
  ```text
  MMAP:    REQBUFS -> QUERYBUF -> mmap -> QBUF(index)
  USERPTR: REQBUFS -> QBUF(m.userptr, length)
  DMABUF:  REQBUFS -> QBUF(m.fd)
  ```
- **Production or Debugging Angle:** Use MMAP first during bring-up. Move to DMABUF when you need zero-copy display/encode and have verified exporter/importer support.
- **Common Traps:**
  - Saying DMABUF always means the capture device allocated the buffer.
  - Confusing `V4L2_MEMORY_DMABUF` import with `VIDIOC_EXPBUF` export.
  - Mixing memory types between `REQBUFS` and `QBUF`.
  - Assuming USERPTR is always faster.
- **Follow-up Questions:**
  - What does `VIDIOC_EXPBUF` do?
  - Which model uses `m.offset`?
  - Which model uses `m.fd`?

### 7. How Do You Use Non-Blocking Capture?

- **Level:** Mid-level
- **Question:** How do `O_NONBLOCK`, `select()`, and `poll()` change V4L2 capture?
- **Short Answer:** With `O_NONBLOCK`, `DQBUF` returns `EAGAIN` instead of sleeping when no frame is ready. `select()` or `poll()` lets userspace wait for readiness with a timeout before calling `DQBUF`.
- **Deep Explanation:** Blocking `DQBUF` is simple but can hang an application if the pipeline stops producing frames. Non-blocking mode is better for event loops, multiple devices, and timeout-based diagnostics. A timeout means no completed buffer became available; that pushes debugging toward media links, sensor streaming, DMA IRQs, or `vb2_buffer_done()`.
- **API / Code Anchor:**
  ```c
  fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
  pfd.fd = fd;
  pfd.events = POLLIN;
  ret = poll(&pfd, 1, 5000);
  if (ret > 0)
          xioctl(fd, VIDIOC_DQBUF, &buf);
  ```
- **Production or Debugging Angle:** A five-second `poll()` timeout during capture is strong evidence that the driver or pipeline is not completing buffers.
- **Common Traps:**
  - Treating `EAGAIN` as a fatal device error.
  - Calling `DQBUF` in a busy loop without `poll()`.
  - Forgetting to requeue after processing.
  - Ignoring timeout as diagnostic evidence.
- **Follow-up Questions:**
  - What does `POLLIN` mean for capture?
  - What should you inspect after repeated timeouts?
  - How does this interact with multiple cameras?

### 8. Debug `DQBUF` Blocking Forever

- **Level:** Mid-level
- **Question:** A V4L2 app reaches `VIDIOC_DQBUF` and blocks forever. How do you debug it?
- **Short Answer:** Verify the ioctl sequence, queued buffers, media links/pad formats, `STREAMON`, sensor streaming, DMA IRQs, and whether the driver calls `vb2_buffer_done()`.
- **Deep Explanation:** `DQBUF` waits for a buffer on the outgoing/done queue. If no buffer appears, either userspace never gave the driver buffers, streaming never really started, the upstream pipeline is not sending pixels, DMA/IRQ completion is broken, or the driver forgot to return buffers to vb2. Use tools to narrow the layer.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
  media-ctl -d /dev/media0 -p
  echo 0x3 > /sys/class/video4linux/video0/dev_debug
  dmesg -w
  ```
- **Production or Debugging Angle:** Add logs around `buf_queue`, `start_streaming`, IRQ completion, and `vb2_buffer_done()`.
- **Common Traps:**
  - Blaming userspace before checking media-controller links.
  - Forgetting sensor `s_stream`.
  - Not checking whether IRQs fire.
  - Returning success from `start_streaming` with no active buffer.
- **Follow-up Questions:**
  - What would `poll()` show?
  - What does `STREAMOFF` do to queued buffers?
  - How can dynamic debug help?

### 9. What Does `dev_debug` Show?

- **Level:** Mid-level
- **Question:** What is `/sys/class/video4linux/videoX/dev_debug`, and when is it useful?
- **Short Answer:** It is a per-video-node debug knob, when supported, that logs V4L2 ioctl activity and parameters to the kernel log.
- **Deep Explanation:** `dev_debug` helps bridge the gap between userspace calls and kernel-side V4L2 handling. After enabling it, running a `v4l2-ctl` capture can show `QUERYCAP`, control queries, `QUERYBUF`, `QBUF`, and `STREAMON` activity. It is useful when you need to prove which ioctl userspace actually sent and what the V4L2 layer saw.
- **API / Code Anchor:**
  ```bash
  echo 0x3 > /sys/class/video4linux/video0/dev_debug
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
  dmesg | grep video0
  ```
- **Production or Debugging Angle:** Use it for short reproductions. Do not leave noisy debug enabled during performance measurements.
- **Common Traps:**
  - Assuming the sysfs knob exists on every kernel/config.
  - Confusing V4L2 core ioctl logs with driver-specific hardware logs.
  - Forgetting to check `dmesg`.
  - Leaving debug enabled and changing timing.
- **Follow-up Questions:**
  - How is dynamic debug different?
  - What would you log in a driver?
  - When would ftrace be more useful?

### 10. What Does `v4l2-compliance` Test?

- **Level:** Mid-level
- **Question:** What does `v4l2-compliance` test, and how should you interpret failures?
- **Short Answer:** It tests whether a V4L2 device follows the API across many ioctls. Failures usually point to invalid capability reporting, control behavior, format handling, buffer handling, or unsupported ioctl semantics.
- **Deep Explanation:** Compliance is not a video quality tool. It checks ABI behavior: required ioctls, open behavior, controls, formats, priority, debug/log status ioctls, streaming behavior when requested, and media-controller behavior when run against `/dev/mediaX`. A failure gives a test area and often source-line context in the tool. The next step is to inspect the relevant ioctl implementation or V4L2/vb2 helper wiring.
- **API / Code Anchor:**
  ```bash
  v4l2-compliance -d /dev/video0 --verbose
  v4l2-compliance -m /dev/media0
  ```
- **Production or Debugging Angle:** Run a recent v4l-utils build for serious driver validation, especially before upstream submission.
- **Common Traps:**
  - Claiming compliance proves image quality.
  - Running compliance with an invalid media pipeline and misreading streaming failures.
  - Ignoring warnings.
  - Testing only `/dev/videoX` for a media-controller driver.
- **Follow-up Questions:**
  - What does compliance not prove?
  - Why might a control test fail?
  - What is the value of `--verbose`?

## Senior

### 11. Design A Robust Userspace Capture Application

- **Level:** Senior
- **Question:** What design choices matter in a production V4L2 capture application?
- **Short Answer:** It should negotiate and verify format, handle buffer ownership strictly, use poll/timeouts, manage cleanup on every error path, record raw-frame metadata, avoid blocking heavy processing in the capture loop, and choose the right memory model.
- **Deep Explanation:** Production capture apps fail when they assume the driver accepted requested parameters, process too slowly, access queued buffers, ignore `EAGAIN`/`EIO`, or leak mappings on error. A robust app treats V4L2 as a state machine: every resource has a cleanup step, every ioctl return is checked, and every buffer has one owner at a time. Heavy processing should usually happen in another thread or use a queue that does not violate buffer ownership.
- **API / Code Anchor:**
  ```text
  init -> set format -> req/map buffers -> start
  capture thread: poll -> DQBUF -> hand off safely -> QBUF
  error path: STREAMOFF -> munmap -> close
  ```
- **Production or Debugging Angle:** For latency-sensitive systems, more buffers reduce drops but increase latency. Tune buffer count based on frame interval and processing time.
- **Common Traps:**
  - Processing on a buffer after it was requeued.
  - No `STREAMOFF` on errors.
  - Saving raw files without FourCC/stride metadata.
  - Using broad `/dev/video0` permissions instead of proper group/udev setup.
- **Follow-up Questions:**
  - How would you handle device unplug?
  - How many buffers would you start with?
  - How would you integrate with an encoder?

### 12. Explain DMABUF Import Versus Export

- **Level:** Senior
- **Question:** Explain DMABUF import versus export in V4L2 and why the distinction matters.
- **Short Answer:** Import means userspace queues an existing DMA-buf fd to a V4L2 queue using `V4L2_MEMORY_DMABUF`. Export means a V4L2 device exposes one of its MMAP buffers as a DMA-buf fd using `VIDIOC_EXPBUF`.
- **Deep Explanation:** In a zero-copy pipeline, one device allocates or exports a buffer and another imports it. The capture driver may import buffers from another allocator, or it may export its own MMAP buffers for display/encode. These are different operations with different ownership and synchronization concerns. Confusing them causes invalid queue setup and broken zero-copy assumptions.
- **API / Code Anchor:**
  ```text
  Import:
    req.memory = V4L2_MEMORY_DMABUF
    buf.memory = V4L2_MEMORY_DMABUF
    buf.m.fd = dmabuf_fd
    VIDIOC_QBUF

  Export:
    req.memory = V4L2_MEMORY_MMAP
    VIDIOC_EXPBUF -> expbuf.fd
  ```
- **Production or Debugging Angle:** Validate both exporter and importer support, and test cache/synchronization behavior under real throughput.
- **Common Traps:**
  - Calling `VIDIOC_EXPBUF` after configuring the queue as DMABUF import.
  - Assuming every driver can export buffers.
  - Ignoring multi-planar per-plane fds.
  - Forgetting fd lifetime and ownership.
- **Follow-up Questions:**
  - How does this help camera-to-display?
  - Which field carries the fd in `struct v4l2_buffer`?
  - What can go wrong with cache coherency?

### 13. Debug Wrong Colors Or Corrupted Frames

- **Level:** Senior
- **Question:** A camera streams, but colors are wrong or frames are corrupted. How do you debug it?
- **Short Answer:** Verify the whole format chain: media pad format, video-node format, FourCC, width/height, stride, `sizeimage`, multi-planar layout, conversion command, DMA length, and buffer size.
- **Deep Explanation:** Successful streaming only proves buffers are completing. It does not prove the bytes are interpreted correctly. Wrong colors often mean a pixel format mismatch, such as Bayer treated as YUYV, RGB/BGR swap, or UV plane order mismatch. Corruption often means a stride, alignment, `sizeimage`, plane size, or DMA programming mismatch. Check what the driver returned, not what userspace requested.
- **API / Code Anchor:**
  ```bash
  media-ctl -d /dev/media0 -p
  v4l2-ctl -d /dev/video0 --get-fmt-video
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=frame.raw
  ffmpeg -f rawvideo -s 640x480 -pix_fmt yuyv422 -i frame.raw frame.png
  ```
- **Production or Debugging Angle:** Compare `fmt.fmt.pix.sizeimage` with vb2 queue plane size and hardware DMA transfer size.
- **Common Traps:**
  - Assuming raw files self-describe their format.
  - Ignoring `bytesperline`.
  - Forgetting pad format may differ from video node format in ISP/scaler pipelines.
  - Using single-planar code for multi-planar formats.
- **Follow-up Questions:**
  - What is the difference between pad format and video node format?
  - How would you identify Bayer data?
  - Where does `queue_setup()` fit?

### 14. How Would You Validate A Driver Before Upstream Submission?

- **Level:** Senior
- **Question:** What would you run and inspect before saying a V4L2 driver is ready for review?
- **Short Answer:** Run `v4l2-ctl` smoke tests, media-controller topology tests if applicable, `v4l2-compliance`, real streaming tests across formats, controls tests, error-path tests, suspend/resume or remove tests, and performance checks.
- **Deep Explanation:** Upstream readiness is more than "I saw a frame." The ABI must be correct and stable, controls must follow V4L2 conventions, unsupported ioctls must fail properly, buffers must be returned on stop/errors, media topology must be coherent, and the driver must handle teardown and runtime failures. Compliance is a key gate, but real-world streaming and board-specific validation are also necessary.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --all
  v4l2-ctl -d /dev/video0 --list-formats-ext
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=300
  media-ctl -d /dev/media0 -p
  v4l2-compliance -d /dev/video0 --verbose
  v4l2-compliance -m /dev/media0
  ```
- **Production or Debugging Angle:** Also test invalid parameters, too few buffers, stream stop during capture, repeated open/close, and unplug/remove if relevant.
- **Common Traps:**
  - Treating a single successful frame as validation.
  - Skipping compliance.
  - Skipping media-device compliance for MC drivers.
  - Not testing error paths.
- **Follow-up Questions:**
  - What failures can compliance miss?
  - How do you test frame drops?
  - What logs would you keep in the driver?

### 15. How Do You Separate Userspace Bugs From Driver Bugs?

- **Level:** Senior
- **Question:** A custom app fails to capture, but the driver author says the driver works. How do you separate userspace bugs from driver bugs?
- **Short Answer:** Reproduce with standard tools, compare the custom app's ioctl sequence and parameters, enable V4L2 ioctl logging, and inspect whether the driver behaves correctly for standard `v4l2-ctl` and compliance tests.
- **Deep Explanation:** Standard tools give a baseline. If `v4l2-ctl --stream-mmap` works and the app fails, compare format selection, buffer memory type, buffer count, `QBUF` order, non-blocking behavior, and cleanup. If standard tools fail too, the issue likely lies in driver, media graph, or hardware setup. `dev_debug` can show the actual ioctls and parameters the app sent.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
  echo 0x3 > /sys/class/video4linux/video0/dev_debug
  dmesg -w
  v4l2-compliance -d /dev/video0 --verbose
  ```
- **Production or Debugging Angle:** Ask the app to log every ioctl return, `errno`, requested/granted format, buffer count, and memory model.
- **Common Traps:**
  - Debugging only the custom app.
  - Ignoring the returned format.
  - Not checking media graph state.
  - Assuming `errno` values without printing them.
- **Follow-up Questions:**
  - What would you compare between two apps?
  - How can `strace` help?
  - What if MMAP works but DMABUF fails?

### 16. What Are The Limits Of `v4l2-compliance`?

- **Level:** Senior
- **Question:** A driver passes `v4l2-compliance`. Is it production-ready?
- **Short Answer:** Not necessarily. Compliance validates a large part of API behavior, but production readiness also requires image correctness, media-route validation, performance, power management, teardown, error recovery, and real workload testing.
- **Deep Explanation:** `v4l2-compliance` is essential because it catches ABI mistakes that manual tests miss. But it cannot know whether your sensor tuning is good, whether the image has correct colors, whether frame timing meets product requirements, whether runtime PM works, or whether the driver survives suspend/resume and remove races. Treat compliance as a gate, not the finish line.
- **API / Code Anchor:**
  ```text
  Compliance proves: many ioctl semantics and ABI rules.
  It does not prove: product behavior under all real conditions.
  ```
- **Production or Debugging Angle:** Pair compliance with long streaming tests, format matrix tests, control tests, media graph tests, power-cycle tests, and stress tests.
- **Common Traps:**
  - Equating compliance pass with no bugs.
  - Ignoring warnings.
  - Running an old compliance tool.
  - Not testing streaming mode with valid signal/pipeline setup.
- **Follow-up Questions:**
  - Why use recent v4l-utils?
  - What product tests would you add?
  - What does image quality depend on?

## Debugging Scenarios

### Scenario 1: `VIDIOC_STREAMON` Fails With `EINVAL`

- **Level:** Mid-level
- **Question:** `v4l2-ctl --stream-mmap --stream-count=1` fails at `STREAMON` with `EINVAL`. What do you check?
- **Short Answer:** Check buffer type, queued buffer count, media links/pad formats, selected video format, and driver `start_streaming` validation.
- **Deep Explanation:** `STREAMON` requires a valid stream type and enough queued buffers for the queue/driver. On MC pipelines, the video node may be valid but the media graph may not be configured. The driver may reject stream start if format, buffer size, sensor state, or DMA setup is invalid.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --get-fmt-video
  media-ctl -d /dev/media0 -p
  dmesg -w
  ```
- **Production or Debugging Angle:** Add `dev_err()` in the driver at each `start_streaming` failure branch and return queued buffers correctly on failure.
- **Common Traps:**
  - Not checking the media graph.
  - Assuming one queued buffer is enough.
  - Ignoring driver logs.
  - Returning `EINVAL` without a useful log.
- **Follow-up Questions:**
  - What should `stop_streaming` do after a failed start?
  - What does vb2 require on start failure?
  - How does `min_buffers_needed` affect this?

### Scenario 2: Compliance Fails Control Tests

- **Level:** Mid-level
- **Question:** `v4l2-compliance` reports control failures. What are likely causes?
- **Short Answer:** Missing control classes, invalid ranges/defaults, bad menu handling, incorrect get/set behavior, inactive controls not flagged correctly, or private controls exposed incorrectly.
- **Deep Explanation:** V4L2 controls have standardized metadata and behavior. Compliance expects controls to report valid ranges, steps, defaults, menus, flags, and read/write semantics. Sensor drivers often fail when auto/manual controls are not linked correctly or when volatile/inactive controls are mishandled.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 -L
  v4l2-compliance -d /dev/video0 --verbose
  ```
- **Production or Debugging Angle:** Prefer standard V4L2 control IDs over private ABI when possible.
- **Common Traps:**
  - Returning success for unsupported controls.
  - Wrong menu ranges.
  - Not updating inactive flags.
  - Assuming controls are cosmetic and not ABI.
- **Follow-up Questions:**
  - What is a volatile control?
  - When should a control be inactive?
  - Why avoid private controls?

### Scenario 3: Raw Capture Converts To A Purple Image

- **Level:** Senior
- **Question:** A raw frame captured by `v4l2-ctl` converts to a purple image. What do you do?
- **Short Answer:** Verify the actual FourCC, width, height, stride, media pad format, and ffmpeg `-pix_fmt`. The conversion likely uses the wrong pixel format or ignores stride/layout.
- **Deep Explanation:** Raw files contain no self-describing header. A purple image often means YUV plane/order mismatch, treating Bayer as YUV, or using the wrong conversion command. If the media graph has an ISP/scaler, pad formats may differ from the video-node format. Inspect both the media topology and `/dev/videoX` format.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --get-fmt-video
  media-ctl -d /dev/media0 -p
  ffmpeg -f rawvideo -s 640x480 -pix_fmt yuyv422 -i frame.raw frame.png
  ```
- **Production or Debugging Angle:** Save raw captures with names that include width, height, FourCC, and whether the data is single- or multi-planar.
- **Common Traps:**
  - Trusting requested format instead of returned format.
  - Forgetting Bayer is not directly viewable as RGB/YUV.
  - Ignoring padding/stride.
  - Assuming compressed MJPEG and raw YUYV behave the same.
- **Follow-up Questions:**
  - How would you detect stride mismatch?
  - What command lists formats?
  - What is the role of pad formats?

### Scenario 4: Frame Drops Under Load

- **Level:** Senior
- **Question:** Capture works, but frames drop when processing is enabled. How do you approach it?
- **Short Answer:** Measure processing time versus frame interval, requeue buffers promptly, increase buffer count carefully, move heavy work off the capture thread, and consider DMABUF/hardware acceleration.
- **Deep Explanation:** If processing holds a dequeued buffer too long, the driver may run out of queued buffers and drop frames. More buffers can absorb jitter but add latency. A better architecture keeps capture fast and moves processing to another stage while respecting buffer ownership. For display/encode pipelines, DMABUF can avoid copies.
- **API / Code Anchor:**
  ```text
  DQBUF -> quick handoff -> QBUF
  heavy processing in separate worker or zero-copy pipeline
  ```
- **Production or Debugging Angle:** Use timestamps and sequence numbers to detect gaps. Tune buffer count based on actual workload.
- **Common Traps:**
  - Increasing buffers endlessly and creating latency.
  - Processing after requeueing the same buffer.
  - Copying every frame unnecessarily.
  - Ignoring CPU scheduling and memory bandwidth.
- **Follow-up Questions:**
  - How does DMABUF reduce CPU load?
  - What is the latency tradeoff of more buffers?
  - How would you prove drops are happening?
