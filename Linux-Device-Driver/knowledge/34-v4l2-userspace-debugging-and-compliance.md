# 34 - V4L2 Userspace, Debugging, And Compliance

## Learning Goal

After this chapter, you should be able to use V4L2 from userspace, test a video device with standard tools, debug common streaming failures, and understand what `v4l2-compliance` does and does not prove.

By the end, you should be able to:

- Explain why V4L2 userspace is an ioctl-driven contract, not just `read()` from `/dev/video0`.
- Walk through a normal capture sequence from `open()` to `STREAMOFF`.
- Compare MMAP, USERPTR, and DMABUF buffer models.
- Use `v4l2-ctl` to inspect devices, formats, controls, and capture frames.
- Use `media-ctl` when a camera pipeline must be configured through `/dev/mediaX`.
- Debug `STREAMON` failures, stuck `DQBUF`, wrong colors, corrupted frames, and frame drops.
- Run `v4l2-compliance` and interpret it as an ABI test, not a full product-quality test.
- Recognize kernel-version-sensitive behavior around buffer capabilities, debug hooks, and v4l-utils.

## Why This Matters In Real Work

Most V4L2 bugs first appear in userspace: an application cannot open the camera, `v4l2-ctl` cannot stream, `DQBUF` blocks forever, the image is purple, or compliance fails before an upstream review. A driver engineer needs to connect those symptoms back to the V4L2 ABI, vb2 state, media graph setup, and driver callbacks.

Common real work:

| Situation | What you need |
| --- | --- |
| Bring up a new camera sensor | `media-ctl -p`, pad formats, `v4l2-ctl --stream-mmap`, `dmesg` |
| Validate a bridge driver | `v4l2-compliance`, buffer ioctl sequence, streaming tests |
| Debug corrupt frames | format, stride, `sizeimage`, buffer size, DMA programming |
| Debug stuck capture | queued buffers, `STREAMON`, sensor stream, IRQ/DMA completion |
| Integrate camera to display/encoder | DMABUF import/export and buffer ownership |
| Prepare upstream driver | standards-compliant V4L2 controls, ioctls, media topology, compliance |

**Production rule:** if a device only works in one custom app but fails `v4l2-ctl` or `v4l2-compliance`, the driver or pipeline is not ready.

## Mental Model

Think of V4L2 userspace as a state machine. The application negotiates what it wants, checks what the driver granted, gives buffers to the driver, starts streaming, then repeatedly receives filled buffers and gives them back.

```text
userspace app
  open /dev/video0
  query capabilities
  choose format / controls / FPS
  request buffers
  map or provide buffers
  queue empty buffers
  start streaming
  loop: dequeue filled -> process -> requeue empty
  stop streaming
  cleanup

kernel side
  video_device receives ioctl
  V4L2 core dispatches
  vb2 owns buffer state machine
  driver owns hardware-specific DMA and stream control
```

The most important rule is **buffer ownership**:

| State | Owner | Userspace may touch data? |
| --- | --- | --- |
| Dequeued | Userspace | Yes |
| Queued | vb2/driver | No |
| Active DMA | Hardware/driver | No |
| Done, not yet dequeued | vb2 | No |
| Dequeued with error flag | Userspace | Yes, but data may be bad |

**Interview trap:** `VIDIOC_QBUF` is not a copy operation. It gives the buffer to the driver for hardware access.

## Core Concepts

V4L2 userspace uses a small set of system calls and many standardized ioctls. The exact ioctl sequence matters because each step changes kernel-side state.

### V4L2 Nodes

| Node | Meaning | Typical tool |
| --- | --- | --- |
| `/dev/videoX` | Capture/output streaming and controls | `v4l2-ctl`, custom app |
| `/dev/mediaX` | Media-controller topology | `media-ctl`, `v4l2-compliance -m` |
| `/dev/v4l-subdevX` | Optional sub-device controls/formats/events | `v4l2-ctl --help-subdev` |

For simple USB webcams, `/dev/videoX` may be enough. For many embedded camera pipelines, `/dev/mediaX` must be configured before `/dev/videoX` can stream.

### Capture Sequence

```text
open()
VIDIOC_QUERYCAP
VIDIOC_ENUM_FMT / VIDIOC_G_FMT / VIDIOC_TRY_FMT / VIDIOC_S_FMT
VIDIOC_G_PARM / VIDIOC_S_PARM       optional FPS path
VIDIOC_REQBUFS
VIDIOC_QUERYBUF + mmap()            MMAP only
VIDIOC_QBUF x N                     prime buffers
VIDIOC_STREAMON
loop:
  VIDIOC_DQBUF
  process frame
  VIDIOC_QBUF
VIDIOC_STREAMOFF
munmap()
close()
```

### Buffer Models

| Model | Who allocates memory? | Userspace setup | Best for | Common trap |
| --- | --- | --- | --- | --- |
| MMAP | Driver/vb2 | `REQBUFS`, `QUERYBUF`, `mmap` | Most capture apps | Treating `m.offset` as a physical address |
| USERPTR | Application | allocate memory, pass `m.userptr` | Custom app-managed memory | Passing invalid/unaligned/too-small buffers |
| DMABUF | Another exporter/device | pass DMA-buf fd in `m.fd` | Zero-copy camera to display/encoder | Confusing import with `VIDIOC_EXPBUF` export |
| read/write | Application buffer | `read()` or `write()` | Simple/legacy path | Assuming every driver supports it |

**Production rule:** keep the same buffer type and memory model across `REQBUFS`, `QUERYBUF`, `QBUF`, and `DQBUF`.

### Tool Roles

| Tool | Use it for |
| --- | --- |
| `v4l2-ctl` | Device info, formats, controls, simple streaming |
| `media-ctl` | Media graph links and pad formats |
| `v4l2-compliance` | V4L2 ABI and ioctl behavior validation |
| `yavta` | Low-level V4L2 streaming tests |
| `ffmpeg` | Convert raw captures or test decode/display paths |
| GStreamer | Rapid pipeline prototyping |
| `dmesg` | Kernel-side evidence |
| dynamic debug / `dev_debug` | V4L2 ioctl/core/driver trace visibility |
| ftrace | Timing, call graph, IRQ/latency investigation |

## Kernel Mechanism

Userspace talks to `/dev/videoX`, but the kernel path usually goes through the V4L2 core and vb2 before hardware-specific driver code runs.

```text
ioctl(fd, VIDIOC_DQBUF, &buf)
  -> video device file operation
  -> video_ioctl2
  -> v4l2_ioctl_ops.vidioc_dqbuf
  -> vb2_ioctl_dqbuf
  -> vb2 queue waits for outgoing/done buffer
  -> returns buffer metadata to userspace
```

The driver implementation from topic 32 provides:

- `struct video_device` for `/dev/videoX`.
- `struct v4l2_ioctl_ops` for format, control, and buffer ioctls.
- `struct vb2_queue` for streaming buffer state.
- `struct vb2_ops` callbacks such as `queue_setup`, `buf_prepare`, `buf_queue`, `start_streaming`, and `stop_streaming`.
- IRQ/DMA completion logic that eventually calls `vb2_buffer_done()`.

### Incoming And Outgoing Queues

Kernel docs describe two conceptual queues:

| Queue | Contains | How buffers enter | How buffers leave |
| --- | --- | --- | --- |
| Incoming queue | Buffers submitted to the driver | `VIDIOC_QBUF` | Driver fills/displays them |
| Outgoing queue | Completed buffers ready for userspace | Driver completion | `VIDIOC_DQBUF` |

For capture:

```text
QBUF empty buffer -> incoming queue
hardware DMA fills it
driver marks buffer done
buffer moves to outgoing queue
DQBUF returns it to userspace
```

Do not confuse this explanatory "incoming/outgoing" wording with `V4L2_BUF_TYPE_VIDEO_OUTPUT`, which is a different device direction.

## Key Structs And APIs

These are the important userspace structs and calls. Learn where they appear in the flow rather than memorizing every field.

### System Calls

| API | Purpose |
| --- | --- |
| `open()` | Open `/dev/videoX`; may use `O_NONBLOCK`. |
| `close()` | Release fd and associated streaming state. |
| `ioctl()` | Send V4L2 commands. |
| `mmap()` / `munmap()` | Map/unmap MMAP buffers. |
| `read()` / `write()` | Optional classic I/O path. |
| `select()` / `poll()` | Wait for frame readiness without blocking forever. |

Use an `xioctl()` wrapper to retry interrupted ioctls:

```c
static int xioctl(int fd, unsigned long request, void *arg)
{
        int ret;

        do {
                ret = ioctl(fd, request, arg);
        } while (ret == -1 && errno == EINTR);

        return ret;
}
```

### Capability And Format Structs

| Struct | Used by | Why it matters |
| --- | --- | --- |
| `struct v4l2_capability` | `VIDIOC_QUERYCAP` | Driver/card/bus info and supported capabilities. |
| `struct v4l2_fmtdesc` | `VIDIOC_ENUM_FMT` | Supported pixel formats. |
| `struct v4l2_format` | `G_FMT`, `TRY_FMT`, `S_FMT` | Current/requested format. |
| `struct v4l2_pix_format` | single-planar video | Width, height, FourCC, stride, `sizeimage`. |
| `struct v4l2_pix_format_mplane` | multi-planar video | Per-plane layout for formats such as NV12 on some devices. |
| `struct v4l2_streamparm` | `G_PARM`, `S_PARM` | Frame interval/FPS when supported. |

Capability flags to recognize:

- `V4L2_CAP_VIDEO_CAPTURE`
- `V4L2_CAP_VIDEO_CAPTURE_MPLANE`
- `V4L2_CAP_STREAMING`
- `V4L2_CAP_READWRITE`
- `V4L2_CAP_DEVICE_CAPS`

**Production rule:** if `V4L2_CAP_DEVICE_CAPS` is set in `capabilities`, use `device_caps` for the opened node's capabilities.

### Buffer Structs

| Struct | Used by | Key fields |
| --- | --- | --- |
| `struct v4l2_requestbuffers` | `VIDIOC_REQBUFS` | `count`, `type`, `memory` |
| `struct v4l2_buffer` | `QUERYBUF`, `QBUF`, `DQBUF` | `index`, `type`, `memory`, `bytesused`, `flags`, `m.offset/userptr/fd`, `length` |
| `struct v4l2_plane` | multi-planar buffers | per-plane fd/userptr/offset, length, bytesused |
| `struct v4l2_exportbuffer` | `VIDIOC_EXPBUF` | export a supported MMAP buffer as DMA-buf fd |

Important buffer flags:

- `V4L2_BUF_FLAG_MAPPED`: mapped into userspace.
- `V4L2_BUF_FLAG_QUEUED`: currently queued to the driver.
- `V4L2_BUF_FLAG_DONE`: ready for dequeue.
- `V4L2_BUF_FLAG_ERROR`: recoverable streaming error; data may be corrupt.

### Controls

| API | Purpose |
| --- | --- |
| `VIDIOC_QUERYCTRL` / `VIDIOC_QUERY_EXT_CTRL` | Discover control type, range, menu, flags. |
| `VIDIOC_G_CTRL` / `VIDIOC_S_CTRL` | Get/set simple controls. |
| `VIDIOC_G_EXT_CTRLS` / `VIDIOC_S_EXT_CTRLS` | Batch and extended controls. |
| `v4l2-ctl -L` | List controls. |
| `v4l2-ctl --get-ctrl name` | Read a control. |
| `v4l2-ctl --set-ctrl name=value` | Set a control. |

Common controls include brightness, contrast, saturation, exposure, gain, white balance, and power line frequency.

## Lifecycle / Data Flow

The safest way to learn V4L2 userspace is to tie each ioctl to the state it changes.

### Basic MMAP Capture Flow

```text
1. Open device
   fd = open("/dev/video0", O_RDWR);

2. Query capability
   VIDIOC_QUERYCAP
   check capture + streaming support

3. Negotiate format
   VIDIOC_ENUM_FMT
   VIDIOC_TRY_FMT or VIDIOC_S_FMT
   read back granted width/height/pixelformat/sizeimage

4. Request buffers
   VIDIOC_REQBUFS with V4L2_MEMORY_MMAP
   check granted count

5. Map buffers
   for each index:
     VIDIOC_QUERYBUF
     mmap(fd, buf.m.offset)

6. Prime buffers
   VIDIOC_QBUF for every buffer

7. Start streaming
   VIDIOC_STREAMON with V4L2_BUF_TYPE_VIDEO_CAPTURE

8. Capture loop
   VIDIOC_DQBUF
   process buffers[buf.index], buf.bytesused
   VIDIOC_QBUF

9. Stop and cleanup
   VIDIOC_STREAMOFF
   munmap()
   close()
```

### Non-Blocking Capture Flow

Open with `O_NONBLOCK` when the application has an event loop or timeout requirement.

```text
open(..., O_RDWR | O_NONBLOCK)
start stream normally
loop:
  poll(fd, POLLIN, timeout)
  if ready:
    DQBUF should not block
  if timeout:
    inspect pipeline/IRQ/DMA or keep waiting
  if DQBUF returns EAGAIN:
    no completed buffer is ready yet
```

### Media-Controller Pipeline Flow

Many embedded camera pipelines need media graph setup before V4L2 capture works.

```bash
# Inspect topology
media-ctl -d /dev/media0 -p

# Optional: reset dynamic links
media-ctl -d /dev/media0 --reset

# Enable links. Entity names and pads are board-specific.
media-ctl -d /dev/media0 --links "'sensor 1-0036':0 -> 'csi-rx':0[1]"
media-ctl -d /dev/media0 --links "'csi-rx':1 -> 'csi capture':0[1]"

# Set compatible pad formats along the route.
media-ctl -d /dev/media0 -V "'sensor 1-0036':0[fmt:SBGGR10_1X10/800x600]"
media-ctl -d /dev/media0 -V "'csi-rx':0[fmt:SBGGR10_1X10/800x600]"

# Then test the video node.
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

**Production rule:** if a media-controller camera fails capture, verify links and pad formats before blaming vb2.

## Minimal Practical Example

This is a learning-only MMAP capture skeleton. It shows the order and ownership rules; production code needs stronger cleanup, signal handling, multi-planar support, configurable device/format, and better error reporting.

```c
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define BUF_COUNT 4

struct buffer {
        void *start;
        size_t length;
};

static int xioctl(int fd, unsigned long request, void *arg)
{
        int ret;

        do {
                ret = ioctl(fd, request, arg);
        } while (ret == -1 && errno == EINTR);

        return ret;
}

int main(void)
{
        int fd = open("/dev/video0", O_RDWR);
        struct v4l2_capability cap;
        struct v4l2_format fmt;
        struct v4l2_requestbuffers req;
        struct buffer buffers[BUF_COUNT];
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        unsigned int i;

        if (fd < 0)
                return 1;

        CLEAR(cap);
        if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
                return 1;

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
                return 1;

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = 640;
        fmt.fmt.pix.height = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;

        if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
                return 1;

        /* Always inspect fmt after S_FMT: the driver may adjust it. */

        CLEAR(req);
        req.count = BUF_COUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
                return 1;
        if (req.count < 2)
                return 1;

        for (i = 0; i < req.count; i++) {
                struct v4l2_buffer buf;

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
                        return 1;

                buffers[i].length = buf.length;
                buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd, buf.m.offset);
                if (buffers[i].start == MAP_FAILED)
                        return 1;

                if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                        return 1;
        }

        if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
                return 1;

        for (i = 0; i < 10; i++) {
                struct v4l2_buffer buf;

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
                        return 1;
                if (buf.index >= req.count)
                        return 1;

                printf("frame %u: buffer=%u bytes=%u flags=0x%x\n",
                       i, buf.index, buf.bytesused, buf.flags);

                /* Process only while dequeued. Requeue promptly. */
                if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                        return 1;
        }

        xioctl(fd, VIDIOC_STREAMOFF, &type);
        for (i = 0; i < req.count; i++)
                munmap(buffers[i].start, buffers[i].length);
        close(fd);
        return 0;
}
```

Important lines:

- `VIDIOC_S_FMT` may change width, height, pixel format, stride, or `sizeimage`.
- `VIDIOC_REQBUFS` may grant a different buffer count.
- `VIDIOC_QUERYBUF` gives MMAP metadata, including an offset for `mmap()`.
- `VIDIOC_QBUF` transfers buffer ownership to the driver.
- `VIDIOC_DQBUF` transfers a completed buffer back to userspace.

## Useful Tool Workflows

These commands are the first layer of practical V4L2 debugging. They are often faster than writing a custom app.

### Device And Capability Inspection

```bash
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 -D
v4l2-ctl -d /dev/video0 --all
```

Look for:

- Driver name and card name.
- `Video Capture` or `Video Capture Multiplanar`.
- `Streaming`.
- `Device Capabilities`.
- Unexpected metadata-only or output-only nodes.

### Formats, FPS, And Controls

```bash
v4l2-ctl -d /dev/video0 --list-formats-ext
v4l2-ctl -d /dev/video0 --get-fmt-video
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=480,pixelformat=YUYV
v4l2-ctl -d /dev/video0 --set-parm=30

v4l2-ctl -d /dev/video0 -L
v4l2-ctl -d /dev/video0 --get-ctrl brightness
v4l2-ctl -d /dev/video0 --set-ctrl brightness=128
```

### Capture And Convert

```bash
# Capture one raw YUYV frame.
v4l2-ctl -d /dev/video0 \
  --set-fmt-video=width=640,height=480,pixelformat=YUYV \
  --stream-mmap --stream-count=1 --stream-to=frame-640x480-yuyv.raw

# Convert raw frame. The format and size must match the capture.
ffmpeg -f rawvideo -s 640x480 -pix_fmt yuyv422 \
  -i frame-640x480-yuyv.raw frame.png

# Capture one MJPEG frame.
v4l2-ctl -d /dev/video0 \
  --set-fmt-video=width=640,height=480,pixelformat=MJPG \
  --stream-mmap --stream-count=1 --stream-to=frame.jpg
```

Raw files have no header. Always record width, height, pixel format, stride, and frame count.

### Compliance

```bash
v4l2-compliance -d /dev/video0 --verbose

# For media-controller devices, test the media device too.
v4l2-compliance -m /dev/media0
v4l2-compliance -M /dev/media0 --verbose
```

`v4l2-compliance` checks V4L2 API behavior across many ioctls. Passing it is necessary for serious driver work, but it does not prove:

- image quality,
- correct sensor tuning,
- correct colors,
- no frame drops under load,
- suspend/resume correctness,
- remove/unplug race safety,
- product-level latency,
- all media routes on a board.

## Common Bugs And Debugging

Start from the observable symptom. Then check the userspace sequence, media graph, V4L2/vb2 logs, and driver-side completion path.

### Quick Debug Checklist

- Does the node exist?
  - `ls -l /dev/video* /dev/media* /dev/v4l-subdev*`
- Is it the right node?
  - `v4l2-ctl --list-devices`
  - `v4l2-ctl -d /dev/video0 -D`
- Does it support capture and streaming?
  - `v4l2-ctl -d /dev/video0 --all`
- Are formats available?
  - `v4l2-ctl -d /dev/video0 --list-formats-ext`
- Is the media graph configured?
  - `media-ctl -d /dev/media0 -p`
- Can a simple stream run?
  - `v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10`
- What does compliance say?
  - `v4l2-compliance -d /dev/video0 --verbose`
- What does the kernel log say?
  - `dmesg -w`

### Symptom Table

| Symptom | Likely causes | Evidence | Fix pattern |
| --- | --- | --- | --- |
| `VIDIOC_QUERYCAP` fails with `EINVAL` | Not a V4L2 node, wrong device path | `v4l2-ctl -D` fails | Pick correct `/dev/videoX` |
| `S_FMT` returns different format | Driver adjusted unsupported request | print `fmt` after ioctl | Accept granted format or choose supported one |
| `REQBUFS` returns `EINVAL` | Unsupported memory model or buffer type | try MMAP, inspect capabilities | Use supported model |
| `STREAMON` fails | Not enough buffers, bad media graph, sensor not ready, driver error | `dmesg`, dynamic debug | Prime buffers, fix links/formats, inspect driver start |
| `DQBUF` blocks forever | No buffer completed | `poll()` timeout, no IRQ, no `vb2_buffer_done()` | Check media pipeline, sensor stream, DMA IRQ |
| `DQBUF` returns `EAGAIN` | Non-blocking fd and no done buffer | open flags | Use `poll()`/`select()` or retry |
| Image wrong colors | wrong FourCC, Bayer treated as YUV, UV swap | raw conversion command, format list | Use correct pix_fmt, configure pad/video formats |
| Corrupted frames | stride/size mismatch, DMA length wrong, buffer too small | `bytesperline`, `sizeimage`, queue size | fix format negotiation and queue setup |
| Frame drops | too few buffers, slow processing, no requeue | sequence gaps, timestamps | requeue quickly, add buffers, split processing thread |
| Compliance control failure | missing control class/menu/ranges, bad get/set behavior | `v4l2-compliance --verbose` | implement standard controls correctly |

### Debug Visibility

Use lightweight tools first, then increase detail.

```bash
# Kernel logs with timestamps if useful.
dmesg -w
echo 1 > /sys/module/printk/parameters/time

# Print all console log levels temporarily.
echo 8 > /proc/sys/kernel/printk

# Per-video-node ioctl debug, if supported.
echo 0x3 > /sys/class/video4linux/video0/dev_debug

# vb2 debug module parameters, if available in this kernel/config.
echo 0x3 > /sys/module/videobuf2_v4l2/parameters/debug
echo 0x3 > /sys/module/videobuf2_common/parameters/debug

# Dynamic debug, if enabled.
echo 'file drivers/media/v4l2-core/* +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file drivers/media/common/videobuf2/* +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file drivers/media/platform/my-driver.c +p' > /sys/kernel/debug/dynamic_debug/control
```

Use ftrace when logs are not enough:

```bash
cd /sys/kernel/debug/tracing
echo function_graph > current_tracer
echo my_driver_start_streaming > set_ftrace_filter
echo 1 > tracing_on
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
echo 0 > tracing_on
cat trace
```

**Production rule:** enable heavy tracing only while reproducing the issue; it can affect timing.

## Production Checklist

Before considering a V4L2 userspace path ready, verify the API, the data, and the failure paths.

### Userspace ABI

- [ ] `v4l2-ctl -D` shows correct driver/card/bus info.
- [ ] `capabilities` and `device_caps` are correct.
- [ ] `--list-formats-ext` shows valid formats, sizes, and intervals.
- [ ] `S_FMT` returns coherent `bytesperline` and `sizeimage`.
- [ ] Controls have valid types, ranges, defaults, menus, and inactive flags.
- [ ] Unsupported ioctls return correct errors, not random success.

### Streaming

- [ ] MMAP capture works with `v4l2-ctl --stream-mmap`.
- [ ] Buffer count returned by `REQBUFS` is sane.
- [ ] All buffers are returned on `STREAMOFF`.
- [ ] `DQBUF` does not block forever when the pipeline is valid.
- [ ] `O_NONBLOCK`, `EAGAIN`, `select()`, and `poll()` behavior is correct.
- [ ] Multi-planar formats work if advertised.
- [ ] `V4L2_BUF_FLAG_ERROR` is used for recoverable frame errors.

### Media Controller

- [ ] `media-ctl -p` topology is understandable.
- [ ] Links and pad formats are valid before streaming.
- [ ] `/dev/videoX` format matches the active media pipeline.
- [ ] Board-specific setup scripts document entity names and pad indexes.

### Debug And Compliance

- [ ] `v4l2-compliance -d /dev/videoX --verbose` passes or failures are understood.
- [ ] Media-device compliance is run for media-controller drivers.
- [ ] Dynamic debug/dev_debug paths are documented if available.
- [ ] Logs identify the device with `dev_*` style messages.
- [ ] Error paths are tested: invalid format, insufficient buffers, stream stop, timeout, unplug/remove where relevant.

### Performance

- [ ] Buffer count balances frame drops and latency.
- [ ] Capture loop requeues promptly.
- [ ] Heavy processing does not block capture.
- [ ] DMABUF path is used for zero-copy display/encoder when appropriate.
- [ ] Raw capture metadata is tracked: width, height, FourCC, stride, timestamp, frame count.

## Interview Readiness

You are ready for interviews when you can explain the userspace sequence as a state machine and debug from symptom to kernel mechanism.

Be able to answer:

- Why does V4L2 use `VIDIOC_REQBUFS -> QBUF -> STREAMON -> DQBUF` instead of simple `read()`?
- What changes when the application uses MMAP, USERPTR, or DMABUF?
- What owns a buffer after `QBUF`?
- Why must userspace check the values returned by `S_FMT` and `REQBUFS`?
- How do `v4l2-ctl`, `media-ctl`, and `v4l2-compliance` differ?
- How would you debug `DQBUF` blocking forever?
- What does `v4l2-compliance` prove, and what does it not prove?

See `interview/34-v4l2-userspace-debugging-and-compliance.md` for structured questions and scenarios.

## Kernel Version Notes

The core capture sequence is stable, but details around buffer capabilities, media requests, debug hooks, and v4l-utils behavior evolve.

- Older notes may say `VIDIOC_QUERYBUF` returns a physical address. For MMAP, teach it as an offset/metadata used with `mmap()`, not a userspace physical address.
- `VIDIOC_REQBUFS` allocates MMAP buffers, but for USERPTR and DMABUF it primarily selects/configures the I/O method and internal queue state.
- Newer kernels expose extra buffer capabilities, request API behavior, orphaned buffer handling, cache hints, and newer ioctls such as buffer removal support.
- `dev_debug`, vb2 module debug parameters, and dynamic debug paths depend on kernel configuration and module layout.
- For serious driver validation, use a recent `v4l-utils` build; distribution versions may lag upstream compliance checks.
