# 32 - V4L2 Core, Video Device, And VB2 Capture Example

This is a **learning-only** V4L2/vb2 example. It does not ship a new kernel
module. Instead, it uses the kernel's virtual video driver, `vivid`, to exercise
the same userspace ABI that a real bridge driver must satisfy, then maps each
userspace command back to the V4L2/vb2 callbacks a driver would implement.

The goal is to learn the lifecycle without pretending that a tiny fake camera
driver is production-ready. A real capture driver depends on target kernel
headers, DMA constraints, interrupts, clocks, reset lines, power management,
sensor/sub-device ordering, and hardware registers.

## Goal

Use this example to see how `/dev/videoX` streaming maps onto a V4L2 bridge
driver:

```text
userspace
  -> VIDIOC_QUERYCAP / S_FMT / REQBUFS / QBUF / STREAMON / DQBUF
V4L2 core
  -> struct video_device
  -> video_ioctl2
videobuf2
  -> struct vb2_queue
  -> queue_setup / buf_prepare / buf_queue / start_streaming
driver
  -> DMA queue
  -> IRQ completion
  -> vb2_buffer_done()
```

The lab demonstrates:

- finding a V4L2 capture node;
- querying driver and device capabilities;
- negotiating one fixed capture format;
- running a short MMAP streaming capture;
- observing how `REQBUFS`, `QBUF`, `STREAMON`, `DQBUF`, and `STREAMOFF` relate
  to vb2;
- debugging common failures from the userspace side;
- understanding the cleanup rules a real bridge driver must follow.

## Kernel Version Assumptions

Run the commands on the same kernel whose modules and headers are installed:

```sh
uname -r
ls /lib/modules/$(uname -r)/kernel/drivers/media/test-drivers/vivid/ 2>/dev/null
```

This example assumes:

- the kernel has `CONFIG_VIDEO_VIVID` built in or available as `vivid.ko`;
- `v4l2-ctl` is installed from the `v4l-utils` package;
- the capture node supports streaming MMAP buffers;
- the bridge-driver skeleton below is checked against the target kernel headers
  before being converted into buildable code.

V4L2 and vb2 APIs change over time. Before writing a real module, validate
these details against your target kernel:

- `struct vb2_ops` callback signatures;
- `struct v4l2_file_operations` fields;
- `vb2_fop_release()` and vb2-aware unregister helper usage;
- `vb2_queue` fields such as `dev`, `lock`, `timestamp_flags`,
  `buf_struct_size`, and `min_buffers_needed`;
- the chosen memory backend: `vb2_dma_contig_memops`,
  `vb2_dma_sg_memops`, or `vb2_vmalloc_memops`.

## Files

| File | Purpose |
| --- | --- |
| `README.md` | Learning-only V4L2/vb2 lab, command flow, callback mapping, debug notes, and cleanup rules. |

There is no `Makefile` because this directory does not include a standalone
kernel module. If you later add a buildable `.c` file, add an out-of-tree
Kbuild `Makefile` and test it against the exact kernel you will load it into.

## Build

No local build is required for this example.

If `vivid` is available as a module, inspect it:

```sh
modinfo vivid | sed -n '1,40p'
```

If `modinfo` cannot find it, check whether the driver is built into the kernel
or absent from the kernel configuration:

```sh
zgrep CONFIG_VIDEO_VIVID /proc/config.gz 2>/dev/null || \
grep CONFIG_VIDEO_VIVID /boot/config-$(uname -r) 2>/dev/null
```

Expected configuration shape:

```text
CONFIG_VIDEO_VIVID=m
```

or:

```text
CONFIG_VIDEO_VIVID=y
```

## Load

Load the virtual video driver if it is a module:

```sh
sudo modprobe vivid
dmesg | tail -30
```

Expected log shape varies by kernel, but it should mention `vivid` and one or
more created video nodes.

List V4L2 devices:

```sh
v4l2-ctl --list-devices
ls -l /dev/video*
```

Choose the `vivid` capture node. The examples below use `/dev/video0`; replace
it with the actual node on your system.

```sh
DEV=/dev/video0
```

## Test

Query the V4L2 device identity and capabilities:

```sh
v4l2-ctl -d "$DEV" -D
```

Expected output shape:

```text
Driver name      : vivid
Card type        : vivid
Bus info         : platform:vivid
Device Caps      : Video Capture
Device Caps      : Streaming
```

List capture formats:

```sh
v4l2-ctl -d "$DEV" --list-formats-ext
```

Select a small YUYV format:

```sh
v4l2-ctl -d "$DEV" \
  --set-fmt-video=width=640,height=480,pixelformat=YUYV
```

Read back the granted format:

```sh
v4l2-ctl -d "$DEV" --get-fmt-video
```

Expected output shape:

```text
Width/Height      : 640/480
Pixel Format      : 'YUYV'
Bytes per Line    : 1280
Size Image        : 614400
```

Run a short MMAP streaming capture:

```sh
v4l2-ctl -d "$DEV" --stream-mmap=3 --stream-count=5 --stream-to=/tmp/vivid-yuyv.raw
ls -lh /tmp/vivid-yuyv.raw
```

Expected output shape:

```text
<<<<<
```

The raw file should contain five frames. For 640x480 YUYV, one frame is:

```text
640 * 480 * 2 = 614400 bytes
```

Five frames should be about:

```text
3072000 bytes
```

## ABI Impact

This example does not create a new userspace ABI. It uses the existing V4L2 ABI
exposed by the loaded video driver:

- device node: `/dev/videoX`;
- ioctl commands: `VIDIOC_QUERYCAP`, format ioctls, buffer ioctls, and stream
  on/off ioctls;
- MMAP streaming buffers owned alternately by userspace and vb2/driver;
- optional sysfs class entries under `/sys/class/video4linux/videoX`.

A real driver that registers a `struct video_device` creates a long-lived
userspace contract. Applications may depend on capabilities, formats, frame
sizes, buffer behavior, blocking behavior, timestamps, sequence numbers, and
error codes. Treat those as ABI, not as internal implementation details.

## Command To Callback Map

| Userspace command | Kernel path | Driver responsibility |
| --- | --- | --- |
| `v4l2-ctl -D` | `VIDIOC_QUERYCAP` | Fill driver/card/bus strings and capability bits. |
| `--set-fmt-video` | `VIDIOC_S_FMT` | Validate and store the granted format, stride, and `sizeimage`. |
| `--stream-mmap=N` | `VIDIOC_REQBUFS` | `queue_setup()` chooses buffer count, plane count, and plane sizes. |
| implicit buffer queueing | `VIDIOC_QBUF` | `buf_prepare()` validates size; `buf_queue()` adds buffer to DMA list. |
| stream start | `VIDIOC_STREAMON` | `start_streaming()` arms hardware only after enough buffers exist. |
| frame receive | `VIDIOC_DQBUF` | Driver IRQ/work completion calls `vb2_buffer_done()`. |
| stream stop | `VIDIOC_STREAMOFF` | `stop_streaming()` stops DMA and returns all outstanding buffers. |

## Minimal Driver Skeleton

This skeleton is **learning-only pseudo-code**. It is here to connect the lab to
driver internals, not to be copied into a product tree unchanged.

```c
struct lld_vb2_buffer {
        struct vb2_v4l2_buffer vb;
        struct list_head list;
        dma_addr_t dma_addr;
};

struct lld_video {
        struct device *dev;
        struct v4l2_device v4l2_dev;
        struct video_device vdev;
        struct vb2_queue queue;
        struct mutex lock;
        spinlock_t qlock;
        struct list_head queued;
        unsigned int width;
        unsigned int height;
        unsigned int sequence;
};

static int lld_queue_setup(struct vb2_queue *q,
                           unsigned int *nbufs,
                           unsigned int *nplanes,
                           unsigned int sizes[],
                           struct device *alloc_devs[])
{
        struct lld_video *vid = vb2_get_drv_priv(q);
        unsigned int size = vid->width * vid->height * 2;

        if (*nbufs < 3)
                *nbufs = 3;

        *nplanes = 1;
        sizes[0] = size;
        return 0;
}

static int lld_buf_prepare(struct vb2_buffer *vb)
{
        struct lld_video *vid = vb2_get_drv_priv(vb->vb2_queue);
        unsigned int size = vid->width * vid->height * 2;

        if (vb2_plane_size(vb, 0) < size)
                return -EINVAL;

        vb2_set_plane_payload(vb, 0, size);
        return 0;
}

static void lld_buf_queue(struct vb2_buffer *vb)
{
        struct lld_video *vid = vb2_get_drv_priv(vb->vb2_queue);
        struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
        struct lld_vb2_buffer *buf = container_of(vbuf, struct lld_vb2_buffer, vb);
        unsigned long flags;

        spin_lock_irqsave(&vid->qlock, flags);
        list_add_tail(&buf->list, &vid->queued);
        spin_unlock_irqrestore(&vid->qlock, flags);
}

static int lld_start_streaming(struct vb2_queue *q, unsigned int count)
{
        struct lld_video *vid = vb2_get_drv_priv(q);

        if (count < q->min_buffers_needed)
                return -ENOBUFS;

        /* Start upstream source if needed, program DMA, enable IRQs. */
        return 0;
}

static void lld_stop_streaming(struct vb2_queue *q)
{
        struct lld_video *vid = vb2_get_drv_priv(q);
        struct lld_vb2_buffer *buf, *tmp;
        unsigned long flags;

        /* Stop capture hardware and disable IRQs before returning buffers. */

        spin_lock_irqsave(&vid->qlock, flags);
        list_for_each_entry_safe(buf, tmp, &vid->queued, list) {
                list_del(&buf->list);
                vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
        }
        spin_unlock_irqrestore(&vid->qlock, flags);
}

static const struct vb2_ops lld_vb2_ops = {
        .queue_setup = lld_queue_setup,
        .buf_prepare = lld_buf_prepare,
        .buf_queue = lld_buf_queue,
        .start_streaming = lld_start_streaming,
        .stop_streaming = lld_stop_streaming,
};
```

Important production additions:

- `VIDIOC_QUERYCAP`, enum/get/try/set format ioctls;
- `struct v4l2_file_operations` using `video_ioctl2`, `vb2_fop_mmap`,
  `vb2_fop_poll`, and an appropriate release helper;
- `struct video_device` registration with `device_caps`, `queue`, `lock`,
  `release`, and driver data;
- `vb2_queue_init()` with the correct buffer type, memory operations, locks,
  DMA device, timestamp flags, and private buffer size;
- IRQ or worker completion that sets timestamp, sequence, payload, and calls
  `vb2_buffer_done(..., VB2_BUF_STATE_DONE)`;
- reverse-order unregister and resource cleanup.

## Debug

Start from userspace:

```sh
v4l2-ctl --list-devices
v4l2-ctl -d "$DEV" -D
v4l2-ctl -d "$DEV" --list-formats-ext
v4l2-ctl -d "$DEV" --get-fmt-video
v4l2-ctl -d "$DEV" --stream-mmap=3 --stream-count=3
dmesg | tail -80
```

If the node exists but streaming fails:

- check that `Device Caps` includes `Video Capture` and `Streaming`;
- check that the selected pixel format and size are accepted after `S_FMT`;
- check that `--stream-mmap` uses a supported memory model;
- try a smaller frame size;
- inspect kernel logs for vb2, media, DMA, or driver errors.

If `DQBUF` blocks forever in a real driver:

- confirm userspace queued buffers before `STREAMON`;
- log `queue_setup()`, `buf_prepare()`, `buf_queue()`, and `start_streaming()`;
- confirm interrupts or completion work run;
- confirm the driver calls `vb2_buffer_done()` for completed or failed frames;
- confirm `stop_streaming()` returns every queued or active buffer.

If supported by your kernel, enable dynamic debug for media drivers:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
sudo sh -c 'echo "file drivers/media/* +p" > /sys/kernel/debug/dynamic_debug/control'
dmesg | tail -80
```

Some kernels expose per-node V4L2 ioctl tracing:

```sh
VIDEO=$(basename "$DEV")
cat /sys/class/video4linux/$VIDEO/dev_debug 2>/dev/null
sudo sh -c "echo 0x1f > /sys/class/video4linux/$VIDEO/dev_debug" 2>/dev/null
```

Run compliance checks when the tool is available:

```sh
v4l2-compliance -d "$DEV"
```

`v4l2-compliance` failures are not cosmetic. They usually indicate ABI behavior
that real applications can trip over.

## Cleanup

Remove the raw capture file:

```sh
rm -f /tmp/vivid-yuyv.raw
```

Unload the virtual driver if it was loaded as a module:

```sh
sudo modprobe -r vivid
dmesg | tail -30
```

If `modprobe -r vivid` fails with "in use", close applications that still have
`/dev/videoX` open:

```sh
fuser -v /dev/video* 2>/dev/null
```

## Cleanup And Error Paths In A Real Driver

Successful probe normally acquires resources in this order:

```text
allocate private state
  -> initialize locks and buffer lists
  -> v4l2_device_register()
  -> vb2_queue_init()
  -> initialize struct video_device
  -> video_register_device()
```

Failure paths unwind in reverse order:

```text
video registration failed
  -> v4l2_device_unregister()
  -> release private state

stream start failed
  -> stop partial hardware setup
  -> disable IRQs/DMA
  -> return all queued buffers with VB2_BUF_STATE_ERROR

stream stop or device removal
  -> stop hardware first
  -> prevent new queueing
  -> return queued/active buffers
  -> unregister video node
  -> unregister v4l2_device
```

The key rule is simple: once userspace queues a buffer, the driver must either
complete it with `VB2_BUF_STATE_DONE` or return it with `VB2_BUF_STATE_ERROR`.
Never leave buffers stranded in a private DMA list.

## Why This Is Not Production-Ready

This example is intentionally small:

- it uses `vivid`, not real hardware;
- it does not include build-tested kernel module source;
- it does not program DMA, handle IRQs, or manage clocks, resets, regulators,
  runtime PM, or sub-devices;
- it does not define a stable product-specific format table;
- it does not test every teardown race such as unplug/remove while streaming;
- it does not validate cache coherency or DMABUF import/export behavior.

Use it to understand the ABI, object relationships, buffer ownership, and
debugging flow. Treat a production V4L2 capture driver as a hardware driver and
a userspace ABI contract at the same time.
