# 32 - V4L2 Core, Video Device, And VB2 Capture Interview Questions

Strong candidates should be able to reason from userspace ioctls down to V4L2 core dispatch, vb2 queue state, driver-owned DMA buffers, IRQ completion, and teardown. Good answers explain ownership and lifecycle, not just struct names.

## Beginner

### 1. What Is V4L2?

- **Level:** Beginner
- **Question:** What is V4L2, and why does Linux use it for video capture drivers?
- **Short Answer:** V4L2 is the Linux kernel framework and userspace ABI for video capture/output devices. It standardizes `/dev/videoX`, video ioctls, formats, controls, streaming buffers, and common driver infrastructure.
- **Deep Explanation:** A camera driver is not just a byte stream. Userspace must query capabilities, choose pixel formats and frame sizes, allocate or share buffers, queue buffers, start streaming, dequeue frames, and stop streaming. V4L2 gives applications a stable ABI and gives drivers common infrastructure through `video_device`, V4L2 ioctl dispatch, and `videobuf2`.
- **API / Code Anchor:**
  ```text
  /dev/video0
    -> VIDIOC_QUERYCAP
    -> VIDIOC_S_FMT
    -> VIDIOC_REQBUFS
    -> VIDIOC_QBUF / VIDIOC_DQBUF
    -> VIDIOC_STREAMON / VIDIOC_STREAMOFF
  ```
- **Production or Debugging Angle:** Use `v4l2-ctl -d /dev/video0 -D` to check whether the device exposes the expected driver name, card name, and capabilities.
- **Common Traps:**
  - Treating V4L2 as only `read()` from a character device.
  - Forgetting that userspace ABI behavior matters as much as hardware programming.
  - Confusing V4L2 capture with framebuffer display.
  - Assuming every video pipeline is a single hardware block.
- **Follow-up Questions:**
  - What is `/dev/videoX`?
  - Why are ioctls central in V4L2?
  - What does `VIDIOC_QUERYCAP` check?

### 2. What Are The Core Objects In A Basic V4L2 Capture Driver?

- **Level:** Beginner
- **Question:** What are `v4l2_device`, `video_device`, and `vb2_queue` used for?
- **Short Answer:** `v4l2_device` is the parent V4L2 container, `video_device` represents the `/dev/videoX` node, and `vb2_queue` manages streaming buffers and connects V4L2 buffer ioctls to driver callbacks.
- **Deep Explanation:** A V4L2 bridge driver usually registers one parent `v4l2_device`, one or more `video_device` nodes, and a `vb2_queue` for each streaming node. The `video_device` holds file operations, ioctl operations, capabilities, and a pointer to the queue. The queue uses vb2 callbacks to ask the driver how to size buffers, queue buffers, and start/stop DMA.
- **API / Code Anchor:**
  ```c
  struct cam_dev {
          struct v4l2_device v4l2_dev;
          struct video_device vdev;
          struct vb2_queue queue;
  };
  ```
- **Production or Debugging Angle:** If `/dev/video0` exists but streaming fails, registration probably worked; inspect `vb2_queue` setup, ioctl ops, and streaming callbacks next.
- **Common Traps:**
  - Thinking `v4l2_device` creates `/dev/videoX` by itself.
  - Forgetting `video_device->queue`.
  - Confusing `video_device` with `v4l2_subdev`.
  - Treating `vb2_queue` as optional for streaming capture.
- **Follow-up Questions:**
  - Which object creates `/dev/videoX`?
  - Which object owns buffer state?
  - Where do file operations live?

### 3. What Is A Bridge Driver?

- **Level:** Beginner
- **Question:** In V4L2, what is a bridge driver, and how is it different from a sensor sub-device driver?
- **Short Answer:** The bridge driver exposes the capture video node and usually owns DMA to memory. A sensor sub-device driver configures a camera sensor or pipeline component, often through I2C/SPI, and usually does not own the final userspace buffers.
- **Deep Explanation:** The bridge is the endpoint that captures frames into memory, so it owns `/dev/videoX`, `video_device`, and `vb2_queue`. A sensor sub-device controls upstream hardware: power, mode registers, frame interval, and stream enable. During stream start, the bridge may call the sensor sub-device to start sending pixels, then the bridge starts DMA.
- **API / Code Anchor:**
  ```c
  /* In bridge start_streaming() */
  ret = v4l2_subdev_call(sensor, video, s_stream, 1);
  ```
- **Production or Debugging Angle:** If DMA runs but no frames arrive, check whether the upstream sensor or receiver was actually put into streaming mode.
- **Common Traps:**
  - Putting capture buffer management in the sensor driver.
  - Expecting a sensor sub-device to expose `/dev/videoX`.
  - Ignoring bridge-to-subdev ordering.
  - Mixing topic 32 bridge basics with media-controller routing details.
- **Follow-up Questions:**
  - Which driver owns `vb2_queue`?
  - What does `s_stream` usually do?
  - When does media controller become important?

### 4. What Is Videobuf2?

- **Level:** Beginner
- **Question:** What is `videobuf2` (`vb2`)?
- **Short Answer:** `videobuf2` is the V4L2 buffer management framework. It handles buffer allocation, mmap, queue/dequeue state, streaming ioctls, and memory backends while calling driver callbacks for hardware-specific work.
- **Deep Explanation:** Without vb2, each capture driver would need to implement the same complex buffer state machine. vb2 standardizes MMAP, USERPTR, DMABUF, buffer states, waiting, mmap/poll helpers, and streaming transitions. The driver supplies callbacks such as `queue_setup`, `buf_prepare`, `buf_queue`, `start_streaming`, and `stop_streaming`.
- **API / Code Anchor:**
  ```c
  static const struct vb2_ops cam_vb2_ops = {
          .queue_setup = cam_queue_setup,
          .buf_prepare = cam_buf_prepare,
          .buf_queue = cam_buf_queue,
          .start_streaming = cam_start_streaming,
          .stop_streaming = cam_stop_streaming,
  };
  ```
- **Production or Debugging Angle:** A stuck capture loop often means the driver never returned a filled buffer to vb2 with `vb2_buffer_done()`.
- **Common Traps:**
  - Saying vb2 performs hardware DMA by itself.
  - Forgetting the driver still owns DMA programming.
  - Ignoring buffer ownership while queued.
  - Not returning buffers on stream stop.
- **Follow-up Questions:**
  - What is `struct vb2_queue`?
  - Which callback sizes buffers?
  - Which function returns a completed buffer?

## Mid-level

### 5. Walk Through The Normal Capture Sequence

- **Level:** Mid-level
- **Question:** Walk through a normal MMAP V4L2 capture sequence from userspace and explain what happens in the driver.
- **Short Answer:** Userspace opens `/dev/videoX`, queries capabilities, sets format, requests buffers, maps them, queues them, calls `STREAMON`, loops over `DQBUF/process/QBUF`, then calls `STREAMOFF`. In the driver, vb2 calls queue setup, buffer prepare/queue, start streaming, IRQ completion, and stop streaming callbacks.
- **Deep Explanation:** `VIDIOC_REQBUFS` establishes the buffer pool and calls `queue_setup`. `VIDIOC_QUERYBUF` lets userspace map MMAP buffers. Each `VIDIOC_QBUF` transfers a buffer to vb2/driver ownership and calls `buf_prepare` and `buf_queue`. `VIDIOC_STREAMON` calls `start_streaming`, where the driver starts sensor/DMA. On each DMA completion, the driver calls `vb2_buffer_done()`. `VIDIOC_DQBUF` returns done buffers to userspace. `VIDIOC_STREAMOFF` stops hardware and flushes queued buffers.
- **API / Code Anchor:**
  ```text
  REQBUFS -> QUERYBUF/mmap -> QBUF x N -> STREAMON
  loop: DQBUF -> process -> QBUF
  STREAMOFF
  ```
- **Production or Debugging Angle:** If `DQBUF` blocks forever, check whether userspace primed buffers, whether `start_streaming` ran, whether IRQs fire, and whether `vb2_buffer_done()` is called.
- **Common Traps:**
  - Calling `DQBUF` before `STREAMON`.
  - Starting stream without queuing buffers.
  - Forgetting to requeue buffers after processing.
  - Treating `REQBUFS` as "streaming has started."
- **Follow-up Questions:**
  - What happens if `O_NONBLOCK` is used?
  - What does `STREAMOFF` do to queued buffers?
  - Why should userspace check granted buffer count?

### 6. How Should `queue_setup()` Work?

- **Level:** Mid-level
- **Question:** What should a driver's `queue_setup()` callback do?
- **Short Answer:** It tells vb2 how many buffers, how many planes per buffer, and how large each plane must be for the current selected format.
- **Deep Explanation:** `queue_setup()` is called during buffer request/create paths. Userspace may request a number of buffers, but the driver can adjust it based on hardware minimums. The driver must set the plane count and sizes consistently with the currently selected pixel format, width, height, stride, and multi-planar layout. Hardcoding stale dimensions causes corrupted frames or failed queues.
- **API / Code Anchor:**
  ```c
  static int cam_queue_setup(struct vb2_queue *q,
                             unsigned int *nbufs,
                             unsigned int *nplanes,
                             unsigned int sizes[],
                             struct device *alloc_devs[])
  {
          if (*nbufs < 3)
                  *nbufs = 3;
          *nplanes = 1;
          sizes[0] = cam->width * cam->height * 2;
          return 0;
  }
  ```
- **Production or Debugging Angle:** For corrupted frames, compare `S_FMT` result, `bytesperline`, `sizeimage`, `queue_setup()` size, and DMA programming.
- **Common Traps:**
  - Using requested format instead of granted/current format.
  - Forgetting multi-planar formats need per-plane sizes.
  - Returning fewer buffers than hardware can tolerate.
  - Not considering alignment or stride.
- **Follow-up Questions:**
  - What is a plane?
  - Why might a driver increase buffer count?
  - How does multi-planar capture change this callback?

### 7. What Should `buf_prepare()` And `buf_queue()` Do?

- **Level:** Mid-level
- **Question:** Explain the difference between `buf_prepare()` and `buf_queue()`.
- **Short Answer:** `buf_prepare()` validates and prepares a buffer before queueing, while `buf_queue()` hands the prepared buffer to the driver's internal DMA queue.
- **Deep Explanation:** `buf_prepare()` is the right place to check plane size, set payload, and prepare address-related state. `buf_queue()` is where ownership moves toward the driver/hardware path. Many drivers wrap `vb2_v4l2_buffer` in a private buffer struct and link it into a driver list protected by a spinlock if IRQ completion also touches it.
- **API / Code Anchor:**
  ```c
  if (vb2_plane_size(vb, 0) < frame_size)
          return -EINVAL;

  vb2_set_plane_payload(vb, 0, frame_size);
  list_add_tail(&buf->list, &cam->dma_queue);
  ```
- **Production or Debugging Angle:** If `QBUF` returns `EINVAL`, inspect `buf_prepare()`. If `STREAMON` starts but no DMA buffer is available, inspect `buf_queue()` and the driver list.
- **Common Traps:**
  - Doing heavy hardware work in `buf_prepare()`.
  - Queueing buffers without locking a shared list.
  - Not setting payload for capture buffers.
  - Touching queued buffers from userspace.
- **Follow-up Questions:**
  - Why do drivers use custom buffer structs?
  - Which lock type is suitable for an IRQ-shared list?
  - When does userspace lose ownership of the buffer?

### 8. What Happens In `start_streaming()`?

- **Level:** Mid-level
- **Question:** What should happen when vb2 calls `start_streaming()`?
- **Short Answer:** The driver should verify enough buffers are queued, start upstream components if needed, program DMA with a queued buffer, enable interrupts, and start capture hardware. If it fails, it must return queued buffers to vb2.
- **Deep Explanation:** `start_streaming()` is the transition from configured buffers to live hardware. The driver must not return success until the capture path can actually produce frames. If the sensor, receiver, DMA engine, or IRQ setup fails, queued buffers cannot remain stuck in driver ownership; they must be returned, usually with `VB2_BUF_STATE_ERROR`.
- **API / Code Anchor:**
  ```c
  static int cam_start_streaming(struct vb2_queue *q, unsigned int count)
  {
          if (count < q->min_buffers_needed)
                  return -ENOBUFS;

          ret = v4l2_subdev_call(sensor, video, s_stream, 1);
          if (ret)
                  goto return_buffers;

          /* program DMA and enable IRQs */
          return 0;
  }
  ```
- **Production or Debugging Angle:** For `STREAMON` failure, log each step: buffer count, subdev stream start, DMA address, IRQ enable, hardware status.
- **Common Traps:**
  - Returning success before DMA is armed.
  - Ignoring `min_buffers_needed`.
  - Forgetting to start the sensor/sub-device.
  - Leaving buffers stuck on failure.
- **Follow-up Questions:**
  - What state should buffers get on failure?
  - Should `-ENOIOCTLCMD` from a subdev always be fatal?
  - What is the difference between queued and active buffers?

### 9. What Must `stop_streaming()` Guarantee?

- **Level:** Mid-level
- **Question:** What must a V4L2 driver guarantee in `stop_streaming()`?
- **Short Answer:** It must stop capture hardware, disable/settle DMA and IRQ activity, stop upstream streaming if needed, and return all outstanding driver-owned buffers to vb2.
- **Deep Explanation:** `STREAMOFF` means userspace wants streaming to end and buffer ownership to reset. Any buffers still on the driver's DMA list must be removed and passed back with `vb2_buffer_done(..., VB2_BUF_STATE_ERROR)` unless they completed normally before stop. If buffers remain in the driver list, vb2 state and userspace can hang or leak resources.
- **API / Code Anchor:**
  ```c
  list_for_each_entry_safe(buf, tmp, &cam->dma_queue, list) {
          list_del(&buf->list);
          vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
  }
  ```
- **Production or Debugging Angle:** If close or streamoff hangs, inspect whether every queued/active buffer is returned exactly once and whether IRQs can race with stop.
- **Common Traps:**
  - Stopping DMA but not returning buffers.
  - Returning a buffer twice in an IRQ/stop race.
  - Sleeping while holding a spinlock.
  - Freeing state before userspace callbacks are quiesced.
- **Follow-up Questions:**
  - How do you avoid racing with the IRQ handler?
  - Why use `list_for_each_entry_safe()`?
  - Should you call sensor `s_stream(0)`?

### 10. Compare MMAP, USERPTR, And DMABUF

- **Level:** Mid-level
- **Question:** Compare MMAP, USERPTR, and DMABUF in V4L2 capture.
- **Short Answer:** MMAP uses driver/vb2-allocated buffers mapped into userspace, USERPTR uses application-provided memory, and DMABUF uses file descriptors for shared DMA buffers, usually for zero-copy pipelines.
- **Deep Explanation:** MMAP is the common default because the driver controls allocation and userspace maps the buffers. USERPTR gives the application memory allocation control, but the driver must safely handle user pages. DMABUF lets one device share buffers with another device, such as camera-to-display or camera-to-encoder. In current teaching, keep import and export distinct: `V4L2_MEMORY_DMABUF` queues external DMA buffer fds, while `VIDIOC_EXPBUF` exports suitable MMAP buffers.
- **API / Code Anchor:**
  ```c
  req.memory = V4L2_MEMORY_MMAP;
  req.memory = V4L2_MEMORY_USERPTR;
  req.memory = V4L2_MEMORY_DMABUF;
  ```
- **Production or Debugging Angle:** If `REQBUFS` returns `EINVAL`, the driver may not support that memory model. Check `q->io_modes` and `q->mem_ops`.
- **Common Traps:**
  - Mixing memory type between `REQBUFS` and `QBUF`.
  - Saying DMABUF always means the capture driver allocated the buffer.
  - Assuming USERPTR works on hardware that cannot handle the memory layout.
  - Ignoring cache coherency and DMA constraints.
- **Follow-up Questions:**
  - Which model is most common for simple capture?
  - When is DMABUF worth the complexity?
  - What does `VIDIOC_EXPBUF` do?

## Senior

### 11. How Do You Handle Buffer Ownership And Races?

- **Level:** Senior
- **Question:** A driver has a DMA completion IRQ and a `stop_streaming()` path that both touch the same buffer list. How do you design this safely?
- **Short Answer:** Use a clear ownership model, protect the driver buffer list with IRQ-safe locking, disable or synchronize DMA/IRQs during stop, remove each buffer exactly once, and call `vb2_buffer_done()` outside unsafe contexts if required by the specific path.
- **Deep Explanation:** The IRQ path completes the active buffer and may start the next one. The stop path aborts the same queue. Without careful locking, one buffer can be completed twice, lost, or returned after state is freed. The driver needs a state flag such as `streaming`, an IRQ-safe list lock, ordered hardware stop, and synchronization so no completion path runs after teardown.
- **API / Code Anchor:**
  ```c
  spin_lock_irqsave(&cam->qlock, flags);
  if (!cam->streaming) {
          spin_unlock_irqrestore(&cam->qlock, flags);
          return IRQ_HANDLED;
  }
  /* remove current buffer from list */
  spin_unlock_irqrestore(&cam->qlock, flags);
  ```
- **Production or Debugging Angle:** Race bugs show up as intermittent streamoff hangs, use-after-free, double completion warnings, or rare corrupted frames during stop/start loops.
- **Common Traps:**
  - Assuming `stop_streaming()` cannot race with IRQ.
  - Freeing buffers while hardware can still DMA.
  - Holding a spinlock around sleeping hardware calls.
  - Completing a buffer twice.
- **Follow-up Questions:**
  - Where would you place `synchronize_irq()`?
  - What state should aborted buffers receive?
  - How would you test this race?

### 12. How Would You Debug `DQBUF` Blocking Forever?

- **Level:** Senior
- **Question:** Userspace calls `STREAMON`, then `DQBUF` blocks forever. How do you debug it?
- **Short Answer:** Verify userspace primed buffers, then trace the driver path: `QBUF` callbacks, `start_streaming`, sensor stream enable, DMA programming, IRQ firing, and `vb2_buffer_done()`.
- **Deep Explanation:** A blocked `DQBUF` means no buffer reached vb2's done queue. The cause can be userspace ordering, no queued buffers, `start_streaming()` silently not starting hardware, sensor not producing data, DMA not completing, interrupt not handled, or the IRQ handler failing to call `vb2_buffer_done()`. Debug from userspace state down to hardware state.
- **API / Code Anchor:**
  ```bash
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
  ```
  ```c
  vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
  ```
- **Production or Debugging Angle:** Add logs in `buf_queue`, `start_streaming`, IRQ handler, and `stop_streaming`. Check DMA status registers and interrupt counters.
- **Common Traps:**
  - Only debugging userspace.
  - Forgetting that `DQBUF` waits for a done buffer.
  - Ignoring sensor/sub-device stream state.
  - Not checking whether buffers were queued before `STREAMON`.
- **Follow-up Questions:**
  - What changes if the file was opened with `O_NONBLOCK`?
  - What would `EAGAIN` mean?
  - How can `v4l2-ctl` simplify reproduction?

### 13. How Do You Design Capability And Format Ioctls Correctly?

- **Level:** Senior
- **Question:** What makes `QUERYCAP`, format enumeration, and `S_FMT` correct in a production V4L2 driver?
- **Short Answer:** They must report accurate device capabilities, enumerate only supported formats, validate or clamp requested formats, store the granted current format, and keep buffer sizing consistent with that format.
- **Deep Explanation:** Userspace depends on these ioctls to choose a valid capture mode. If `S_FMT` accepts impossible dimensions or `queue_setup()` uses a different size, streaming will fail or corrupt frames. The driver should implement `TRY_FMT` logic that normalizes format requests, then use the same rules for `S_FMT`. Capability bits must reflect what the driver implements, not what the hardware might theoretically support.
- **API / Code Anchor:**
  ```c
  cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
  cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
  ```
- **Production or Debugging Angle:** `v4l2-compliance` is very good at catching bad capability bits, missing required ioctls, and inconsistent format behavior.
- **Common Traps:**
  - Reporting capabilities the driver does not implement.
  - Not checking the actual values returned by `S_FMT`.
  - Forgetting multi-planar capability differences.
  - Letting format change while buffers are allocated or streaming.
- **Follow-up Questions:**
  - Why should userspace check granted format?
  - What is the relation between `S_FMT` and `queue_setup()`?
  - When should format changes be rejected?

### 14. What Are The Teardown And Lifetime Risks?

- **Level:** Senior
- **Question:** What lifetime issues matter when unregistering a V4L2 capture device?
- **Short Answer:** The driver must stop userspace-visible streaming, stop hardware and IRQs, return or invalidate outstanding buffers, unregister the video node safely, and only then release state according to V4L2/vb2 lifetime rules.
- **Deep Explanation:** `/dev/videoX` may be open while remove or module unload begins. The driver must prevent new operations, quiesce active capture, and avoid freeing data reachable from file handles or vb2 callbacks. `video_device` has a release callback because the node's lifetime can outlive simple stack scope. vb2 release/unregister helpers can be version-sensitive, so target-kernel rules matter.
- **API / Code Anchor:**
  ```c
  video_unregister_device(&cam->vdev);
  v4l2_device_unregister(&cam->v4l2_dev);
  ```
- **Production or Debugging Angle:** Stress with open/close loops, streamon/streamoff loops, unplug/remove if applicable, and `rmmod` while userspace is misbehaving.
- **Common Traps:**
  - Freeing private state immediately after unregister without considering open files.
  - Not stopping DMA before state release.
  - Forgetting vb2-specific release/unregister expectations.
  - Missing cleanup on probe failure.
- **Follow-up Questions:**
  - Why does `video_device` need a `release` callback?
  - What should happen to queued buffers on remove?
  - How do managed resources interact with explicit unregister calls?

### 15. How Do You Choose A VB2 Memory Backend?

- **Level:** Senior
- **Question:** How do you choose between `vb2_dma_contig_memops`, `vb2_dma_sg_memops`, and `vb2_vmalloc_memops`?
- **Short Answer:** Choose based on what the hardware can DMA to. Use contiguous DMA for hardware that needs physically contiguous buffers, scatter-gather for hardware that supports SG DMA, and vmalloc mainly for non-DMA or USB-style paths where physical contiguity is not needed.
- **Deep Explanation:** The memory backend defines how vb2 allocates/maps buffer memory. Picking the wrong backend can produce DMA failures, corruption, or poor performance. Simple SoC capture DMA often needs contiguous memory. More capable hardware may use scatter-gather. vmalloc is virtually contiguous but not physically contiguous, so it is not suitable for simple DMA engines that require bus-contiguous memory.
- **API / Code Anchor:**
  ```c
  q->mem_ops = &vb2_dma_contig_memops;
  q->mem_ops = &vb2_dma_sg_memops;
  q->mem_ops = &vb2_vmalloc_memops;
  ```
- **Production or Debugging Angle:** If frames are corrupted or DMA faults occur, verify physical address requirements, IOMMU behavior, cache coherency, and the chosen vb2 memory backend.
- **Common Traps:**
  - Choosing vmalloc because it is easy while hardware requires DMA-contiguous memory.
  - Enabling USERPTR without considering hardware mapping constraints.
  - Ignoring cache synchronization.
  - Assuming every platform has enough contiguous memory for large frames.
- **Follow-up Questions:**
  - How does an IOMMU affect this choice?
  - Why might high-resolution capture fail with contiguous allocation?
  - When is DMABUF useful here?

### 16. How Would You Review A New V4L2 Bridge Driver?

- **Level:** Senior
- **Question:** What would you check in a code review for a new V4L2 bridge capture driver?
- **Short Answer:** Review object lifetime, format ABI, vb2 queue setup, buffer ownership, stream start/stop error paths, IRQ locking, DMA safety, sub-device ordering, compliance results, and kernel-version-specific API usage.
- **Deep Explanation:** V4L2 bugs often hide in state transitions rather than obvious syntax. A driver can register `/dev/video0` and still be broken if `S_FMT` lies, `queue_setup()` sizes buffers incorrectly, `stop_streaming()` leaks buffers, or IRQ completion races with teardown. A serious review follows the full path from probe to userspace ioctls to DMA completion and remove.
- **API / Code Anchor:**
  ```text
  probe -> v4l2_device_register -> vb2_queue_init
        -> video_register_device
        -> REQBUFS/QBUF/STREAMON/DQBUF
        -> STREAMOFF/remove
  ```
- **Production or Debugging Angle:** Require `v4l2-compliance`, at least one real streaming test, failure-path tests, and repeated streamon/streamoff testing before considering the driver reliable.
- **Common Traps:**
  - Reviewing only probe and register paths.
  - Not testing userspace-visible behavior.
  - Ignoring rare but important stop/remove races.
  - Copying old V4L2 examples without checking current APIs.
- **Follow-up Questions:**
  - What compliance failures would worry you most?
  - How would you test buffer leak paths?
  - What kernel-version notes should be documented?

## Debugging Scenarios And Traps

### Scenario 1: `/dev/video0` Does Not Appear

- **Level:** Mid-level
- **Question:** The driver module loads, but `/dev/video0` does not appear. What do you check?
- **Short Answer:** Check whether probe ran, whether `v4l2_device_register()` and `video_register_device()` succeeded, and whether device/driver matching exists.
- **Deep Explanation:** Module load only registers code. A video node appears only after a matched device probes and the driver registers a `video_device`. Missing `/dev/video0` can be a platform/USB/PCI match issue, a probe failure, or a video registration failure.
- **API / Code Anchor:**
  ```c
  ret = video_register_device(&cam->vdev, VFL_TYPE_VIDEO, -1);
  ```
- **Production or Debugging Angle:** Inspect `dmesg`, `/sys/class/video4linux`, and the relevant bus device/driver directories.
- **Common Traps:**
  - Assuming `insmod` means probe happened.
  - Not logging register failures.
  - Forgetting `video_device->release`.
  - Missing match table/module alias.
- **Follow-up Questions:**
  - What return value should probe use on registration failure?
  - What does `-1` mean as the video node number?

### Scenario 2: `STREAMON` Returns `EINVAL`

- **Level:** Mid-level
- **Question:** `VIDIOC_STREAMON` returns `EINVAL`. What are likely causes?
- **Short Answer:** The stream type may be wrong, buffers may not be requested/queued correctly, the queue may not support the selected mode, or the driver/vb2 state is invalid.
- **Deep Explanation:** `STREAMON` needs a valid buffer type matching the queue and a valid streaming setup. If userspace passes the wrong `enum v4l2_buf_type`, skips `REQBUFS`, mixes memory models, or the driver failed to initialize the queue correctly, vb2 can reject the request.
- **API / Code Anchor:**
  ```c
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctl(fd, VIDIOC_STREAMON, &type);
  ```
- **Production or Debugging Angle:** Reproduce with `v4l2-ctl --stream-mmap --stream-count=1` and compare with the custom app's ioctl order.
- **Common Traps:**
  - Passing `_MPLANE` type to a single-planar queue or the reverse.
  - Forgetting to queue buffers first.
  - Mixing `V4L2_MEMORY_MMAP` and `USERPTR`.
  - Not checking return values before `STREAMON`.
- **Follow-up Questions:**
  - How do you distinguish app bug from driver bug?
  - Which debug logs would you add?

### Scenario 3: Frames Are Corrupted

- **Level:** Senior
- **Question:** Capture works but frames are corrupted. How do you approach it?
- **Short Answer:** Verify format negotiation, bytes per line, size image, buffer plane sizes, DMA address programming, cache coherency, and whether the hardware writes the format userspace expects.
- **Deep Explanation:** Corruption often comes from a mismatch between userspace format, driver current format, vb2 buffer size, and hardware DMA setup. If userspace requests YUYV but hardware writes NV12, or if stride is wrong, frames appear scrambled. If DMA writes beyond the plane size, memory corruption can follow.
- **API / Code Anchor:**
  ```c
  vb2_set_plane_payload(vb, 0, size);
  sizes[0] = current_format.sizeimage;
  ```
- **Production or Debugging Angle:** Dump the granted format with `v4l2-ctl --get-fmt-video`, capture one raw frame, and verify expected pixel format/stride before chasing complex bugs.
- **Common Traps:**
  - Trusting requested format instead of granted format.
  - Ignoring `bytesperline`.
  - Hardcoding 640x480 while userspace selected another resolution.
  - Forgetting cache/DMA synchronization constraints.
- **Follow-up Questions:**
  - What would `v4l2-compliance` catch here?
  - How would multi-planar formats change your checks?
