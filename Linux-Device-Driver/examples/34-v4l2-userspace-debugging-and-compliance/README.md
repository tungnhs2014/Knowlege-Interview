# 34 - V4L2 Userspace, Debugging, And Compliance Example

This is a **learning-only** V4L2 userspace example. It does not add a kernel
module or a new device driver. It uses an existing `/dev/videoX` node, ideally
the kernel's `vivid` virtual video driver or a simple webcam, to practice the
same ABI that real camera applications and validation tools use.

The example includes a small single-planar MMAP capture program. It is realistic
enough to show capability checks, returned format checks, buffer ownership,
`poll()` timeouts, `EAGAIN`, `V4L2_BUF_FLAG_ERROR`, and cleanup paths. It is not
a production camera application.

## Goal

Use this lab to learn the userspace side of a V4L2 capture device:

```text
open /dev/videoX
  -> VIDIOC_QUERYCAP
  -> VIDIOC_S_FMT and verify the granted format
  -> VIDIOC_REQBUFS
  -> VIDIOC_QUERYBUF + mmap
  -> VIDIOC_QBUF all buffers
  -> VIDIOC_STREAMON
  -> poll + VIDIOC_DQBUF + process + VIDIOC_QBUF
  -> VIDIOC_STREAMOFF
  -> munmap + close
```

The lab demonstrates:

- device discovery with `v4l2-ctl`;
- format and control inspection;
- a minimal buildable userspace MMAP capture app;
- raw YUYV frame capture and expected file sizing;
- `media-ctl` checks for media-controller pipelines;
- `v4l2-compliance` as an ABI test;
- kernel-side debug hooks such as `dev_debug`, dynamic debug, and `dmesg`;
- cleanup/error-path rules for userspace and the driver behavior they imply.

## Kernel Version Assumptions

Run the commands on the target system that owns the video device:

```sh
uname -r
v4l2-ctl --version
v4l2-compliance --version 2>/dev/null || true
```

This example assumes:

- Linux provides a V4L2 capture node such as `/dev/video0`;
- `v4l-utils` is installed for `v4l2-ctl`, `media-ctl`, and
  `v4l2-compliance`;
- the capture node supports single-planar `V4L2_BUF_TYPE_VIDEO_CAPTURE`;
- the capture node supports streaming MMAP buffers;
- the selected test format is YUYV.

The userspace ABI sequence is stable, but details vary by kernel and driver:

- some devices are multi-planar and use `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`;
- some embedded cameras require media-controller setup before streaming;
- `dev_debug` and dynamic debug availability depend on kernel configuration;
- `v4l2-compliance` behavior evolves with the installed v4l-utils version.

## Files

| File | Purpose |
| --- | --- |
| `README.md` | Lab commands, expected output, debug workflow, ABI impact, and cleanup rules. |
| `v4l2_mmap_capture.c` | Learning-only single-planar YUYV MMAP capture program. |

There is no kernel module and no Kbuild `Makefile` in this example. If kernel
module code is added later, add a module `Makefile` and validate it against the
exact target kernel headers.

## Build

Build the userspace capture tool:

```sh
cd Linux-Device-Driver/examples/34-v4l2-userspace-debugging-and-compliance
gcc -Wall -Wextra -O2 -o v4l2_mmap_capture v4l2_mmap_capture.c
```

Expected build result:

```text
v4l2_mmap_capture
```

## Load

No local module is loaded by this directory.

For a virtual test device, try the kernel `vivid` driver:

```sh
sudo modprobe vivid
v4l2-ctl --list-devices
```

Expected device-list shape:

```text
vivid (...)
        /dev/video0
        /dev/video1
```

If `vivid` is not available, use a real capture node:

```sh
v4l2-ctl --list-devices
ls -l /dev/video*
```

Set the device variable used below:

```sh
DEV=/dev/video0
```

## Test With Standard Tools

Query identity and capabilities:

```sh
v4l2-ctl -d "$DEV" -D
```

Expected output shape:

```text
Driver name      : vivid
Card type        : vivid
Device Caps      : Video Capture
Device Caps      : Streaming
```

List formats:

```sh
v4l2-ctl -d "$DEV" --list-formats-ext
```

Select and read back a small YUYV format:

```sh
v4l2-ctl -d "$DEV" \
  --set-fmt-video=width=640,height=480,pixelformat=YUYV
v4l2-ctl -d "$DEV" --get-fmt-video
```

Expected format shape:

```text
Width/Height      : 640/480
Pixel Format      : 'YUYV'
Bytes per Line    : 1280
Size Image        : 614400
```

Run a tool-only MMAP capture smoke test:

```sh
v4l2-ctl -d "$DEV" --stream-mmap=4 --stream-count=5 --stream-to=/tmp/v4l2-tool.yuyv
ls -lh /tmp/v4l2-tool.yuyv
```

Expected streaming shape:

```text
<<<<<
```

For 640x480 YUYV, each frame is normally:

```text
640 * 480 * 2 = 614400 bytes
```

Five frames should be about:

```text
3072000 bytes
```

## Test With The Example Program

Run the capture program:

```sh
./v4l2_mmap_capture "$DEV" /tmp/v4l2-app.yuyv 5 640 480
ls -lh /tmp/v4l2-app.yuyv
```

Expected program output shape:

```text
Granted format: 640x480 YUYV bytesperline=1280 sizeimage=614400
Frame 0: index=0 bytesused=614400 sequence=0
Frame 1: index=1 bytesused=614400 sequence=1
Frame 2: index=2 bytesused=614400 sequence=2
Frame 3: index=3 bytesused=614400 sequence=3
Frame 4: index=0 bytesused=614400 sequence=4
```

If you want to visually inspect raw YUYV, convert it with ffmpeg:

```sh
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x480 \
  -i /tmp/v4l2-app.yuyv -frames:v 1 /tmp/v4l2-frame.png
```

The conversion command must match the granted format. Wrong colors usually mean
the pixel format, Bayer order, stride, or resolution was interpreted incorrectly.

## Media-Controller Pipeline Check

For many embedded cameras, `/dev/videoX` cannot stream until `/dev/mediaX` links
and pad formats are valid. Inspect the graph first:

```sh
media-ctl -d /dev/media0 -p
```

A board-specific setup may look like this:

```sh
media-ctl -d /dev/media0 --reset
media-ctl -d /dev/media0 --links '"sensor":0 -> "csi-bridge":0 [1]'
media-ctl -d /dev/media0 -V '"sensor":0 [fmt:UYVY8_2X8/640x480]'
media-ctl -d /dev/media0 -V '"csi-bridge":0 [fmt:UYVY8_2X8/640x480]'
media-ctl -d /dev/media0 -p
```

Entity names, pad numbers, bus formats, and sizes are hardware-specific. Copy
the shape of the workflow, not the literal names.

## Compliance

Run a basic ABI compliance check:

```sh
v4l2-compliance -d "$DEV" --verbose
```

For a media-controller device, also test the media device:

```sh
v4l2-compliance -m /dev/media0 --verbose
```

Expected successful summary shape:

```text
Total for ...: ... test, ... succeeded, 0 failed, 0 warnings
```

Treat failures as driver or pipeline evidence, not as cosmetic output. Common
failure areas are capability reporting, controls, format negotiation, buffer
ioctls, stream start/stop behavior, media links, and unsupported ioctl returns.

Compliance does not prove image quality, sensor tuning, timing, power
management, bandwidth margin, or application-specific DMABUF behavior.

## Debug

Start from userspace evidence:

```sh
v4l2-ctl -d "$DEV" --all
v4l2-ctl -d "$DEV" --list-formats-ext
v4l2-ctl -d "$DEV" --stream-mmap=4 --stream-count=3
dmesg | tail -80
```

If `DQBUF` blocks or the example times out:

- confirm buffers were queued before `STREAMON`;
- check that `S_FMT` returned the format you think you are capturing;
- check media-controller links and pad formats;
- check whether the sensor or upstream sub-device started streaming;
- inspect driver logs around `start_streaming`, IRQ/DMA completion, and
  `vb2_buffer_done()`;
- verify `STREAMOFF` returns all queued buffers on error.

Enable V4L2 per-node ioctl logging if your kernel exposes it:

```sh
VIDEO=$(basename "$DEV")
cat /sys/class/video4linux/$VIDEO/dev_debug 2>/dev/null
sudo sh -c "echo 0x3 > /sys/class/video4linux/$VIDEO/dev_debug" 2>/dev/null
v4l2-ctl -d "$DEV" --stream-mmap=4 --stream-count=1
dmesg | tail -120
```

Enable dynamic debug for short reproductions when debugfs is available:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
sudo sh -c 'echo "file drivers/media/* +p" > /sys/kernel/debug/dynamic_debug/control'
sudo sh -c 'echo "module videobuf2_common +p" > /sys/kernel/debug/dynamic_debug/control' 2>/dev/null || true
sudo sh -c 'echo "module videobuf2_v4l2 +p" > /sys/kernel/debug/dynamic_debug/control' 2>/dev/null || true
dmesg | tail -120
```

If the debug output becomes too noisy:

```sh
sudo sh -c 'echo "file drivers/media/* -p" > /sys/kernel/debug/dynamic_debug/control'
sudo sh -c 'echo "module videobuf2_common -p" > /sys/kernel/debug/dynamic_debug/control' 2>/dev/null || true
sudo sh -c 'echo "module videobuf2_v4l2 -p" > /sys/kernel/debug/dynamic_debug/control' 2>/dev/null || true
sudo sh -c "echo 0 > /sys/class/video4linux/$VIDEO/dev_debug" 2>/dev/null
```

## Expected Error Paths

The example program deliberately unwinds resources in reverse order:

```text
open video fd
  -> open output file
  -> request MMAP buffers
  -> mmap each granted buffer
  -> queue buffers
  -> STREAMON
  -> capture loop

cleanup:
  STREAMOFF if streaming started
  munmap every mapped buffer
  close output file
  close video fd
```

Important behavior:

- `poll()` timeout means no completed buffer became available in time;
- `EAGAIN` from `DQBUF` in non-blocking mode means try again after readiness;
- `V4L2_BUF_FLAG_ERROR` means the buffer was returned but frame data may be bad;
- a failed `STREAMON` should not leave userspace assuming capture started;
- `STREAMOFF` tells the driver/vb2 to stop streaming and release queued buffers.

For a real driver, every buffer accepted by `QBUF` must eventually return via
`DQBUF`, or be returned by `STREAMOFF`/error cleanup. A stuck private DMA list is
one of the classic reasons userspace blocks forever.

## Userspace ABI Impact

This example does not create a new ABI. It consumes the existing V4L2 userspace
ABI exposed by the selected driver:

- `/dev/videoX` as the device node;
- `VIDIOC_QUERYCAP`, format ioctls, buffer ioctls, and stream on/off ioctls;
- MMAP buffer offsets returned by `VIDIOC_QUERYBUF`;
- blocking/non-blocking behavior and `poll()` readiness;
- buffer metadata such as `bytesused`, `sequence`, timestamps, and error flags;
- optional debug sysfs entry `/sys/class/video4linux/videoX/dev_debug`.

A production driver that exposes `/dev/videoX` creates a stable contract. Do not
change capability bits, format behavior, control ranges, buffer semantics,
timestamps, sequence behavior, or error returns casually after applications
depend on them.

## Cleanup

Remove generated files:

```sh
rm -f v4l2_mmap_capture
rm -f /tmp/v4l2-tool.yuyv /tmp/v4l2-app.yuyv /tmp/v4l2-frame.png
```

Unload `vivid` only if you loaded it for this lab:

```sh
sudo modprobe -r vivid
```

If unload fails because the module is busy, find open users:

```sh
fuser -v /dev/video* /dev/media* 2>/dev/null
```

## Why This Is Not Production-Ready

This example is intentionally small:

- it supports only single-planar YUYV capture;
- it does not implement multi-planar buffers or DMABUF import/export;
- it saves raw frames without a metadata sidecar;
- it does not integrate with a long-running event loop;
- it does not handle hot-unplug or media graph reconfiguration;
- it does not tune camera controls, frame intervals, exposure, or latency;
- it relies on the selected driver to provide a correct `/dev/videoX` ABI.

Use it to understand the ABI and debug workflow. For production, add format
enumeration, multi-planar support, signal handling, metadata recording,
media-controller setup, robust permissions, and target-specific performance
tests.
