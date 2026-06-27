# 32 - V4L2 Core, Video Device, And VB2 Capture

## Learning Goal

After this chapter, you should be able to explain how a Linux camera or capture bridge driver exposes `/dev/videoX`, how userspace streaming ioctls reach driver callbacks, and how `videobuf2` (`vb2`) manages capture buffers for DMA.

By the end, you should be able to:

- Explain what **V4L2** is and why it is more than a normal character driver.
- Distinguish `v4l2_device`, `video_device`, `vb2_queue`, and `v4l2_subdev`.
- Register a V4L2 bridge device that exposes `/dev/videoX`.
- Connect file operations, ioctl operations, and vb2 helpers.
- Walk through `REQBUFS -> QBUF -> STREAMON -> DQBUF -> STREAMOFF`.
- Explain when the driver owns a buffer and when userspace owns it.
- Implement the key vb2 callbacks at a design level.
- Debug stream start failures, stuck `DQBUF`, corrupted frames, and missing buffers.
- Recognize kernel-version-sensitive V4L2 API details before writing buildable code.

## Why This Matters In Real Work

Video capture drivers are common in embedded systems: MIPI CSI receivers, camera bridge blocks, USB capture devices, PCIe capture cards, HDMI receivers, ISP frontends, and custom FPGA video pipelines. These drivers must expose a stable userspace ABI while moving large frames through DMA with strict ownership rules.

V4L2 solves several hard problems for driver authors:

- Provides the standard Linux video userspace ABI through `/dev/videoX`.
- Handles common ioctl dispatch, file operation wrapping, priorities, controls, and device registration.
- Provides `videobuf2` for buffer allocation, mmap, queueing, dequeueing, streaming state, and memory backends.
- Lets drivers focus on hardware-specific work: format negotiation, DMA programming, IRQ completion, and error recovery.
- Gives userspace tools such as `v4l2-ctl`, `v4l2-compliance`, GStreamer, ffmpeg, OpenCV, and custom apps a consistent API.

Common real systems:

| System | V4L2 role |
| --- | --- |
| SoC CSI receiver | Bridge driver owns `/dev/videoX`, DMA, and vb2 queue. |
| Camera sensor over I2C | Sub-device configures sensor registers and streaming. |
| USB webcam | Often an all-in-one V4L2 driver with internal buffer handling. |
| HDMI capture card | Capture bridge exposes formats, buffers, and streaming. |
| Camera-to-display pipeline | V4L2 capture may export/import DMABUF for zero-copy display. |

**Production rule:** a capture driver is correct only if buffer ownership, stream start/stop, error paths, and userspace ABI behavior are all correct.

## Mental Model

Think of V4L2 capture as a contract between four sides: userspace, the V4L2 core, the bridge driver, and the hardware. Userspace asks for buffers and starts streaming; V4L2/vb2 manages the generic state machine; the bridge driver feeds buffers to DMA; the hardware fills them.

```text
userspace app
  open /dev/video0
  ioctl VIDIOC_* calls
  mmap / DQBUF / QBUF
        |
        v
V4L2 core
  video_device
  file/ioctl dispatch
        |
        v
videobuf2
  vb2_queue
  buffer states
  memory backend
        |
        v
bridge driver
  format rules
  DMA queue
  start/stop streaming
  IRQ completion
        |
        v
hardware
  sensor/receiver/DMA engine
```

The most important idea is **buffer ownership**:

| Buffer state | Owner | Meaning |
| --- | --- | --- |
| Dequeued | Userspace | App can read/process or prepare the buffer. |
| Queued | vb2/driver | Buffer is submitted and must not be touched by userspace. |
| Active | Driver/hardware | DMA may be writing into the buffer. |
| Done | vb2/userspace soon | Driver completed it; userspace can dequeue it. |
| Error | vb2/userspace soon | Driver returns it because capture failed or stopped. |

**Interview trap:** V4L2 is not "just a char driver with read()". Streaming capture is mostly ioctl-driven buffer ownership plus DMA handoff.

## Core Concepts

V4L2 has several objects that are easy to confuse. Keep their jobs separate.

| Object | Simple meaning | Owned by | What it exposes |
| --- | --- | --- | --- |
| `struct v4l2_device` | Root V4L2 container | Bridge/parent driver | Parent for video nodes and sub-devices. |
| `struct video_device` | One userspace video node | Bridge driver | `/dev/videoX`, file ops, ioctl ops. |
| `struct vb2_queue` | Streaming buffer state machine | Bridge driver + vb2 | Buffer allocation, queueing, streaming ioctls. |
| `struct v4l2_subdev` | Sensor/ISP/scaler abstraction | Sub-device driver | Usually kernel-only control path, sometimes `/dev/v4l-subdevX`. |

### Bridge Driver Versus Sub-Device

A **bridge driver** is the driver for the capture interface that connects video data to memory. It usually owns DMA and exposes `/dev/videoX`.

A **sub-device driver** is the driver for a component in the video pipeline, such as a sensor, decoder, ISP block, or scaler. It usually configures registers and streaming but does not own the final userspace capture buffers.

| Question | Bridge driver | Sub-device driver |
| --- | --- | --- |
| Exposes `/dev/videoX`? | Usually yes | No, except optional `/dev/v4l-subdevX` |
| Owns `vb2_queue`? | Yes | Usually no |
| Programs DMA to memory? | Usually yes | Usually no |
| Handles `VIDIOC_QBUF/DQBUF`? | Via vb2 | No |
| Configures sensor registers? | Calls subdev | Usually yes |

Topic 32 focuses on the bridge, video node, and vb2 capture path. Async sub-device discovery and media graph routing are topic 33.

### Buffer Memory Models

V4L2 supports several memory models. The driver advertises and implements them through `vb2_queue.io_modes` and `vb2_queue.mem_ops`.

| Model | Userspace view | Driver/vb2 view | Typical use |
| --- | --- | --- | --- |
| `V4L2_MEMORY_MMAP` | Driver allocates buffers; app maps them with `mmap()` | Common vb2 streaming path | Most capture apps |
| `V4L2_MEMORY_USERPTR` | App allocates memory and passes pointers | Driver must handle user pages safely | Custom userspace memory |
| `V4L2_MEMORY_DMABUF` | App passes DMA buffer file descriptors | Queue imports shared DMA buffers | Zero-copy pipelines |
| `read()` | App reads into a userspace buffer | Optional simpler path | Simple or legacy use |

**Production rule:** do not mix memory models between `REQBUFS`, `QBUF`, and `DQBUF`.

## Kernel Mechanism

The V4L2 core gives every `/dev/videoX` node a generic character-device wrapper. The wrapper checks state, handles common V4L2 behavior, and then calls the driver-specific operations.

### Object Relationships

```text
struct my_cam {
    struct device       *dev;
    struct v4l2_device  v4l2_dev;
    struct video_device vdev;
    struct vb2_queue    queue;

    struct mutex        lock;       // ioctl/queue serialization
    spinlock_t          qlock;      // IRQ-side buffer list
    struct list_head    dma_queue;  // driver-owned buffers
};

v4l2_device
  ^ parent
video_device
  -> fops
  -> ioctl_ops
  -> queue
  -> device_caps
vb2_queue
  -> vb2_ops
  -> mem_ops
  -> drv_priv = my_cam
```

The driver usually embeds these objects in one private state structure so lifetime is easy to reason about.

### Call Dispatch

```text
userspace ioctl(fd, VIDIOC_DQBUF, &buf)
  -> generic V4L2 file operation
  -> video_ioctl2
  -> driver v4l2_ioctl_ops.vidioc_dqbuf
  -> vb2_ioctl_dqbuf
  -> vb2 queue state machine
  -> returns a done buffer if available
```

For many buffer ioctls, the driver does not implement all logic itself. It wires V4L2 ioctls to vb2 helpers:

```c
static const struct v4l2_ioctl_ops cam_ioctl_ops = {
        .vidioc_querycap      = cam_querycap,
        .vidioc_enum_fmt_vid_cap = cam_enum_fmt,
        .vidioc_g_fmt_vid_cap = cam_g_fmt,
        .vidioc_s_fmt_vid_cap = cam_s_fmt,

        .vidioc_reqbufs       = vb2_ioctl_reqbufs,
        .vidioc_querybuf      = vb2_ioctl_querybuf,
        .vidioc_qbuf          = vb2_ioctl_qbuf,
        .vidioc_dqbuf         = vb2_ioctl_dqbuf,
        .vidioc_streamon      = vb2_ioctl_streamon,
        .vidioc_streamoff     = vb2_ioctl_streamoff,
};
```

The driver-specific hardware work appears in `struct vb2_ops`.

## Key Structs And APIs

Important structs and APIs are useful only when you know where each one sits in the flow.

### V4L2 Device Container

`struct v4l2_device` is the root V4L2 object for a physical parent device.

Key APIs:

- `v4l2_device_register(struct device *dev, struct v4l2_device *v4l2_dev)`
- `v4l2_device_unregister(struct v4l2_device *v4l2_dev)`
- `v4l2_device_call_all()` and `v4l2_subdev_call()` when sub-devices are involved

Use it in probe:

```c
ret = v4l2_device_register(&pdev->dev, &cam->v4l2_dev);
if (ret)
        return ret;
```

### Video Device Node

`struct video_device` represents `/dev/videoX`.

Fields you normally set:

| Field | Why it matters |
| --- | --- |
| `v4l2_dev` | Links node to parent V4L2 device. |
| `fops` | Handles open/release/ioctl/mmap/poll at file level. |
| `ioctl_ops` | Handles `VIDIOC_*` commands. |
| `queue` | Connects node to `vb2_queue`. |
| `device_caps` | Reports capture/streaming capabilities. |
| `lock` | Serializes ioctl access. |
| `release` | Required for video-device lifetime. |
| `name` | Human-readable video node name. |

Registration:

```c
ret = video_register_device(&cam->vdev, VFL_TYPE_VIDEO, -1);
if (ret)
        goto err_v4l2;
```

Some older material uses `VFL_TYPE_GRABBER`; modern code commonly uses `VFL_TYPE_VIDEO`. Validate against the target kernel.

### File Operations

`struct v4l2_file_operations` connects system calls to V4L2/vb2 behavior.

Typical bridge file ops:

```c
static const struct v4l2_file_operations cam_fops = {
        .owner          = THIS_MODULE,
        .open           = v4l2_fh_open,
        .release        = vb2_fop_release,
        .unlocked_ioctl = video_ioctl2,
        .poll           = vb2_fop_poll,
        .mmap           = vb2_fop_mmap,
};
```

Use the vb2 helpers when your queue supports that operation. If you support `read()`, also wire `vb2_fop_read` and set `VB2_READ`.

### VB2 Queue

`struct vb2_queue` is the center of streaming capture.

Fields you normally configure:

| Field | Typical capture value |
| --- | --- |
| `type` | `V4L2_BUF_TYPE_VIDEO_CAPTURE` or `_MPLANE` |
| `io_modes` | `VB2_MMAP`, optionally `VB2_USERPTR`, `VB2_DMABUF`, `VB2_READ` |
| `ops` | Driver's `struct vb2_ops` callbacks |
| `mem_ops` | `vb2_dma_contig_memops`, `vb2_dma_sg_memops`, or `vb2_vmalloc_memops` |
| `drv_priv` | Pointer to driver private state |
| `buf_struct_size` | Size of driver-specific buffer wrapper |
| `min_buffers_needed` | Minimum queued buffers before streaming can start |
| `lock` | Mutex for queue/streaming ioctls |
| `dev` | Device used by DMA allocator/mapping |

Initialization:

```c
q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
q->io_modes = VB2_MMAP | VB2_DMABUF;
q->ops = &cam_vb2_ops;
q->mem_ops = &vb2_dma_contig_memops;
q->drv_priv = cam;
q->buf_struct_size = sizeof(struct cam_buffer);
q->min_buffers_needed = 2;
q->lock = &cam->lock;
q->dev = cam->dev;

ret = vb2_queue_init(q);
```

### VB2 Callbacks

`struct vb2_ops` is where the generic buffer state machine calls your hardware-specific code.

| Callback | Called around | Driver responsibility |
| --- | --- | --- |
| `queue_setup` | `VIDIOC_REQBUFS` / `CREATE_BUFS` | Decide buffer count, plane count, plane sizes. |
| `buf_prepare` | `VIDIOC_QBUF` | Validate size, set payload, get DMA address if needed. |
| `buf_queue` | `VIDIOC_QBUF` | Put buffer on driver's DMA-ready list. |
| `start_streaming` | `VIDIOC_STREAMON` | Start sensor/DMA/IRQs after enough buffers are queued. |
| `stop_streaming` | `VIDIOC_STREAMOFF` | Stop hardware and return all outstanding buffers. |
| `buf_finish` | `VIDIOC_DQBUF` | Optional cache/copyback finalization. |
| `buf_cleanup` | Buffer free | Optional unmap/release cleanup. |

**Production rule:** if streaming fails or stops, every buffer still owned by the driver must be returned to vb2.

## Lifecycle / Data Flow

The driver lifecycle and the userspace streaming lifecycle meet inside the vb2 queue.

### Driver Bring-Up

```text
platform/USB/PCI probe
  -> allocate private state
  -> init mutex/spinlock/list
  -> v4l2_device_register()
  -> fill vb2_queue
  -> vb2_queue_init()
  -> fill video_device
  -> video_set_drvdata()
  -> video_register_device()
  -> /dev/videoX exists
```

### Userspace Capture Flow

```text
open("/dev/video0")
  -> VIDIOC_QUERYCAP
  -> VIDIOC_ENUM_FMT / VIDIOC_G_FMT / VIDIOC_S_FMT
  -> VIDIOC_REQBUFS
  -> VIDIOC_QUERYBUF + mmap   (for MMAP)
  -> VIDIOC_QBUF for each buffer
  -> VIDIOC_STREAMON
  -> loop:
       VIDIOC_DQBUF
       process buffer
       VIDIOC_QBUF
  -> VIDIOC_STREAMOFF
  -> munmap / close
```

### Kernel-Side Buffer Flow

```text
VIDIOC_REQBUFS
  -> vb2 alloc/config
  -> queue_setup()
  -> buffers are dequeued/available to userspace

VIDIOC_QBUF
  -> buf_prepare()
  -> vb2 marks buffer queued/active
  -> buf_queue()
  -> driver appends to DMA list

VIDIOC_STREAMON
  -> start_streaming()
  -> driver starts sensor/DMA

DMA IRQ
  -> complete current buffer
  -> set timestamp/sequence/payload
  -> vb2_buffer_done(..., DONE)
  -> program next queued buffer

VIDIOC_DQBUF
  -> userspace receives done buffer

VIDIOC_STREAMOFF
  -> stop_streaming()
  -> driver stops DMA/IRQs
  -> return all queued/active buffers
```

### Buffer State Flow

```text
userspace owns
  DEQUEUED
      |
      | VIDIOC_QBUF
      v
vb2/driver owns
  QUEUED
      |
      | driver starts DMA
      v
hardware owns
  ACTIVE
      |
      | IRQ: vb2_buffer_done()
      v
vb2 done queue
  DONE or ERROR
      |
      | VIDIOC_DQBUF
      v
userspace owns again
  DEQUEUED
```

## Minimal Practical Example

This is a **learning-only pseudo-code skeleton**. It shows how the objects connect; it is not a production-ready driver and must be checked against the target kernel before building.

```c
struct cam_buffer {
        struct vb2_v4l2_buffer vb;
        struct list_head list;
        dma_addr_t dma_addr;
};

struct cam_dev {
        struct device *dev;
        struct v4l2_device v4l2_dev;
        struct video_device vdev;
        struct vb2_queue queue;
        struct mutex lock;
        spinlock_t qlock;
        struct list_head dma_queue;
        unsigned int width;
        unsigned int height;
        unsigned int sequence;
};

static int cam_queue_setup(struct vb2_queue *q,
                           unsigned int *nbufs,
                           unsigned int *nplanes,
                           unsigned int sizes[],
                           struct device *alloc_devs[])
{
        struct cam_dev *cam = vb2_get_drv_priv(q);
        unsigned int size = cam->width * cam->height * 2; /* YUYV */

        if (*nbufs < 3)
                *nbufs = 3;

        *nplanes = 1;
        sizes[0] = size;
        return 0;
}

static int cam_buf_prepare(struct vb2_buffer *vb)
{
        struct cam_dev *cam = vb2_get_drv_priv(vb->vb2_queue);
        unsigned int size = cam->width * cam->height * 2;

        if (vb2_plane_size(vb, 0) < size)
                return -EINVAL;

        vb2_set_plane_payload(vb, 0, size);
        return 0;
}

static void cam_buf_queue(struct vb2_buffer *vb)
{
        struct cam_dev *cam = vb2_get_drv_priv(vb->vb2_queue);
        struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
        struct cam_buffer *buf = container_of(vbuf, struct cam_buffer, vb);
        unsigned long flags;

        spin_lock_irqsave(&cam->qlock, flags);
        list_add_tail(&buf->list, &cam->dma_queue);
        spin_unlock_irqrestore(&cam->qlock, flags);
}

static int cam_start_streaming(struct vb2_queue *q, unsigned int count)
{
        struct cam_dev *cam = vb2_get_drv_priv(q);

        if (count < q->min_buffers_needed)
                return -ENOBUFS;

        /* Program first DMA buffer, enable IRQs, start capture hardware. */
        return 0;
}

static void cam_stop_streaming(struct vb2_queue *q)
{
        struct cam_dev *cam = vb2_get_drv_priv(q);
        struct cam_buffer *buf, *tmp;
        unsigned long flags;

        /* Stop DMA and disable capture IRQs first. */

        spin_lock_irqsave(&cam->qlock, flags);
        list_for_each_entry_safe(buf, tmp, &cam->dma_queue, list) {
                list_del(&buf->list);
                vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
        }
        spin_unlock_irqrestore(&cam->qlock, flags);
}

static const struct vb2_ops cam_vb2_ops = {
        .queue_setup = cam_queue_setup,
        .buf_prepare = cam_buf_prepare,
        .buf_queue = cam_buf_queue,
        .start_streaming = cam_start_streaming,
        .stop_streaming = cam_stop_streaming,
};
```

Important lines:

- `buf_struct_size = sizeof(struct cam_buffer)` lets vb2 allocate driver-specific buffers.
- `queue_setup()` must match the current selected format.
- `buf_prepare()` validates that the provided plane can hold one frame.
- `buf_queue()` transfers ownership from userspace/vb2 into the driver's DMA list.
- `start_streaming()` starts hardware only after enough buffers are available.
- `stop_streaming()` must return every outstanding buffer to vb2.

A real driver must also implement format ioctls, capability reporting, open/release behavior, hardware resource acquisition, IRQ handling, power management, cleanup ordering, and compliance testing.

## Common Bugs And Debugging

Most V4L2 capture failures are visible as userspace ioctl errors or stuck streaming. Start with the observable symptom, then trace which layer owns the state.

| Symptom | Likely causes | What to inspect |
| --- | --- | --- |
| No `/dev/videoX` | `video_register_device()` failed, probe did not run, missing match | `dmesg`, `/sys/class/video4linux`, probe logs |
| `VIDIOC_QUERYCAP` wrong | Missing `device_caps`, bad `querycap` implementation | `v4l2-ctl -D`, driver `querycap` |
| `VIDIOC_REQBUFS` fails | Unsupported memory model, bad queue setup | `q->io_modes`, `q->mem_ops`, `queue_setup()` |
| `STREAMON` fails | Not enough buffers, hardware/subdev start failure, bad DMA setup | `min_buffers_needed`, `start_streaming()` logs |
| `DQBUF` blocks forever | Buffers not primed, DMA IRQ not firing, no `vb2_buffer_done()` | IRQ logs, DMA status, buffer list |
| Corrupted frames | Wrong size/stride/format, bad DMA address, cache issue | `S_FMT`, `queue_setup()`, `buf_prepare()`, DMA config |
| Streamoff hangs or leaks | Driver did not return all buffers | `stop_streaming()`, vb2 debug logs |
| `v4l2-compliance` failures | Missing ioctls, wrong capability bits, invalid state handling | Compliance output, ioctl handlers |

Useful commands:

```bash
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 -D
v4l2-ctl -d /dev/video0 --list-formats-ext
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
v4l2-compliance -d /dev/video0
```

Useful kernel-side checks:

- Add `dev_dbg()` around `queue_setup`, `buf_prepare`, `buf_queue`, `start_streaming`, `stop_streaming`, and IRQ completion.
- Enable dynamic debug for media/vb2 code where available.
- Check whether `videobuf2_v4l2`, `videobuf2_common`, or `videodev` expose debug module parameters on the target kernel.
- Use `/sys/class/video4linux/videoX/dev_debug` if supported to trace ioctl activity.

**Common trap:** if `DQBUF` waits forever, do not look only at userspace. The missing event is often an IRQ, DMA completion path, or missing `vb2_buffer_done()`.

## Production Checklist

Before review or bring-up, verify the capture driver as a full lifecycle, not just as code that compiles.

- Device registration:
  - `v4l2_device_register()` succeeds and is unwound on error.
  - `video_device` has valid `release`, `fops`, `ioctl_ops`, `v4l2_dev`, `queue`, `lock`, and `device_caps`.
  - Driver-private data is reachable through the expected helpers.
- Format and ABI:
  - `QUERYCAP`, `ENUM_FMT`, `G_FMT`, `TRY_FMT`, and `S_FMT` are coherent.
  - Driver clamps or rejects unsupported width/height/pixel formats correctly.
  - Userspace-visible values after `S_FMT` are the actual granted values.
- VB2 queue:
  - `type`, `io_modes`, `ops`, `mem_ops`, `drv_priv`, `dev`, `lock`, and `buf_struct_size` are correct.
  - `queue_setup()` uses the current format, not hardcoded stale dimensions.
  - `buf_prepare()` validates plane size and sets payload.
  - `buf_queue()` uses correct locking for IRQ/shared lists.
- Streaming:
  - `start_streaming()` handles too few buffers and hardware start failure.
  - `stop_streaming()` stops DMA/IRQs and returns all outstanding buffers.
  - IRQ completion sets timestamp/sequence/payload and calls `vb2_buffer_done()`.
  - No sleeping in hard IRQ context.
- Memory and DMA:
  - Memory backend matches hardware capability: contiguous, scatter-gather, or vmalloc.
  - Cache coherency and DMA mapping rules are respected.
  - DMABUF import/export behavior is not confused.
- Teardown:
  - Userspace entry points are quiesced before freeing state.
  - Hardware is stopped before unregistering video nodes.
  - Cleanup ordering handles open files, queued buffers, IRQs, workqueues, subdev streaming, and runtime PM.
- Validation:
  - Test with `v4l2-ctl`.
  - Run `v4l2-compliance`.
  - Test wrong formats, too few buffers, streamon/streamoff loops, open/close loops, and device removal if applicable.

## Interview Readiness

You are ready for interviews when you can explain the whole path from userspace to DMA without memorizing every field name.

You should be able to:

- Explain why V4L2 uses `video_device` and `vb2_queue` instead of a plain `file_operations` character driver.
- Walk through `REQBUFS -> QBUF -> STREAMON -> DQBUF`.
- Explain what `vb2_buffer_done()` means and who calls it.
- Compare MMAP, USERPTR, and DMABUF.
- Explain bridge versus sub-device responsibilities.
- Debug `STREAMON` failure, blocked `DQBUF`, corrupted frames, and missing `/dev/videoX`.

See `interview/32-v4l2-core-video-device-and-vb2-capture.md` for focused questions and traps.

## Kernel Version Notes

V4L2 and media APIs are version-sensitive. The mental model is stable, but buildable code must be checked against the target kernel.

- Some older material uses v4.19-era names or signatures. Validate `struct vb2_ops`, subdev pad APIs, `remove()` signatures, and video unregister helpers.
- Modern V4L2 code commonly reports `device_caps` and sets `V4L2_CAP_DEVICE_CAPS` correctly in `VIDIOC_QUERYCAP`.
- `V4L2_MEMORY_DMABUF` is mainly an import model for queued buffers; `VIDIOC_EXPBUF` exports MMAP-backed buffers. Keep import/export paths distinct.
- If using vb2 file-operation release helpers, check whether the target kernel expects a vb2-aware video unregister helper.
