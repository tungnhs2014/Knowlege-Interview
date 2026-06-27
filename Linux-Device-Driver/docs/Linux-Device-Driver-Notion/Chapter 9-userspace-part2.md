# CHAPTER 9: LEVERAGING V4L2 API FROM USER SPACE - PART 2

> **Continuation: Complete Application, Tools & Advanced Topics**  
> Part 1 đã cover: API Overview, Property Management, Buffer Management  
> Part 2 focus: Complete implementation, v4l2-ctl tools, Advanced topics

---

## 9.4. Complete Capture Application

### 9.4.1. Overview - Full Capture Flow

```
APPLICATION LIFECYCLE
┌─────────────────────────────────────────────────────┐
│ 1. INITIALIZATION                                   │
│    - Open device                                    │
│    - Query capabilities                             │
│    - Set format                                     │
│    - Request buffers                                │
│    - Map buffers (MMAP) or allocate (USERPTR)      │
├─────────────────────────────────────────────────────┤
│ 2. PRIME BUFFERS                                    │
│    - Enqueue all buffers (QBUF × N)                 │
├─────────────────────────────────────────────────────┤
│ 3. START STREAMING                                  │
│    - STREAMON                                       │
├─────────────────────────────────────────────────────┤
│ 4. CAPTURE LOOP                                     │
│    ┌─────────────────────────────────────────────┐ │
│    │ DQBUF → Get filled buffer                   │ │
│    │ process_frame() → Save/display/encode      │ │
│    │ QBUF → Return buffer to driver             │ │
│    └─────────────↑───────────────────────────────┘ │
│                  │                                  │
│                  └── Repeat until done              │
├─────────────────────────────────────────────────────┤
│ 5. CLEANUP                                          │
│    - STREAMOFF                                      │
│    - munmap() buffers (MMAP)                        │
│    - free() buffers (USERPTR)                       │
│    - close() device                                 │
└─────────────────────────────────────────────────────┘
```

### 9.4.2. Step 1: Initialization

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define DEVICE_NAME "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define BUF_COUNT 4

struct buffer {
    void *start;
    size_t length;
};

static int fd = -1;
static struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;

// xioctl wrapper (từ Part 1)
static int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

// Step 1: Open and query device
int init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    
    // Open device
    fd = open(DEVICE_NAME, O_RDWR);
    if (fd == -1) {
        perror("Cannot open device");
        return -1;
    }
    
    // Query capabilities
    CLEAR(cap);
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("VIDIOC_QUERYCAP");
        return -1;
    }
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Not a video capture device\n");
        return -1;
    }
    
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Does not support streaming\n");
        return -1;
    }
    
    printf("Device: %s\n", cap.card);
    printf("Driver: %s\n", cap.driver);
    
    return 0;
}
```

### 9.4.3. Step 2: Set Format

```c
int set_format(void)
{
    struct v4l2_format fmt;
    
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    
    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("VIDIOC_S_FMT");
        return -1;
    }
    
    // Verify what driver granted
    if (fmt.fmt.pix.width != WIDTH || fmt.fmt.pix.height != HEIGHT) {
        printf("Warning: Resolution changed to %dx%d\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height);
    }
    
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
        fprintf(stderr, "Pixel format not accepted\n");
        return -1;
    }
    
    printf("Format: %dx%d, %.4s\n",
           fmt.fmt.pix.width, fmt.fmt.pix.height,
           (char*)&fmt.fmt.pix.pixelformat);
    
    return 0;
}
```

### 9.4.4. Step 3: Request and Map Buffers (MMAP)

```c
int init_mmap(void)
{
    struct v4l2_requestbuffers req;
    
    CLEAR(req);
    req.count = BUF_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("VIDIOC_REQBUFS");
        return -1;
    }
    
    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        return -1;
    }
    
    // Allocate buffer array
    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }
    
    // Query and map each buffer
    for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
        struct v4l2_buffer buf;
        
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }
        
        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL,
                                        buf.length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        fd,
                                        buf.m.offset);
        
        if (buffers[n_buffers].start == MAP_FAILED) {
            perror("mmap");
            return -1;
        }
    }
    
    printf("Allocated %d buffers\n", n_buffers);
    return 0;
}
```

### 9.4.5. Step 4: Prime Buffers and Start Streaming

```c
int start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;
    
    // Enqueue all buffers (prime buffers)
    for (i = 0; i < n_buffers; i++) {
        struct v4l2_buffer buf;
        
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }
    
    // Start streaming
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("VIDIOC_STREAMON");
        return -1;
    }
    
    printf("Streaming started\n");
    return 0;
}
```

### 9.4.6. Step 5: Capture Loop

```c
void process_frame(const void *data, size_t length)
{
    // Example: Save frame to file
    static int frame_count = 0;
    char filename[64];
    FILE *fp;
    
    snprintf(filename, sizeof(filename), "frame_%04d.raw", frame_count++);
    fp = fopen(filename, "wb");
    if (fp) {
        fwrite(data, length, 1, fp);
        fclose(fp);
        printf("Saved %s (%zu bytes)\n", filename, length);
    }
}

int capture_frames(int num_frames)
{
    int i;
    
    for (i = 0; i < num_frames; i++) {
        struct v4l2_buffer buf;
        
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        // Dequeue filled buffer
        if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            if (errno == EAGAIN) {
                // No buffer ready (non-blocking mode)
                i--;
                continue;
            }
            perror("VIDIOC_DQBUF");
            return -1;
        }
        
        // Sanity check
        if (buf.index >= n_buffers) {
            fprintf(stderr, "Invalid buffer index\n");
            return -1;
        }
        
        printf("Frame %d: index=%d, bytesused=%d, timestamp=%ld.%06ld\n",
               i, buf.index, buf.bytesused,
               buf.timestamp.tv_sec, buf.timestamp.tv_usec);
        
        // Process frame
        process_frame(buffers[buf.index].start, buf.bytesused);
        
        // Requeue buffer
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }
    
    return 0;
}
```

### 9.4.7. Step 6: Stop and Cleanup

```c
int stop_capturing(void)
{
    enum v4l2_buf_type type;
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("VIDIOC_STREAMOFF");
        return -1;
    }
    
    printf("Streaming stopped\n");
    return 0;
}

void cleanup(void)
{
    unsigned int i;
    
    // Unmap buffers
    if (buffers) {
        for (i = 0; i < n_buffers; i++) {
            if (buffers[i].start != MAP_FAILED) {
                munmap(buffers[i].start, buffers[i].length);
            }
        }
        free(buffers);
    }
    
    // Close device
    if (fd != -1) {
        close(fd);
    }
    
    printf("Cleanup done\n");
}
```

### 9.4.8. Main Function - Putting It All Together

```c
int main(int argc, char **argv)
{
    int num_frames = 10;  // Default: capture 10 frames
    
    if (argc > 1) {
        num_frames = atoi(argv[1]);
    }
    
    printf("V4L2 Capture Application\n");
    printf("Capturing %d frames from %s\n", num_frames, DEVICE_NAME);
    
    // Step 1: Initialize
    if (init_device() < 0) {
        goto error;
    }
    
    // Step 2: Set format
    if (set_format() < 0) {
        goto error;
    }
    
    // Step 3: Request and map buffers
    if (init_mmap() < 0) {
        goto error;
    }
    
    // Step 4: Prime buffers and start streaming
    if (start_capturing() < 0) {
        goto error;
    }
    
    // Step 5: Capture loop
    if (capture_frames(num_frames) < 0) {
        goto error;
    }
    
    // Step 6: Stop streaming
    if (stop_capturing() < 0) {
        goto error;
    }
    
    // Step 7: Cleanup
    cleanup();
    
    printf("Success!\n");
    return 0;
    
error:
    cleanup();
    fprintf(stderr, "Error occurred\n");
    return 1;
}
```

### 9.4.9. Error Handling Best Practices

**Common errors và cách xử lý:**

| Error | Cause | Solution |
|-------|-------|----------|
| `EINVAL` | Invalid parameter | Check buffer type, memory type |
| `EAGAIN` | No buffer ready (non-blocking) | Retry or use select/poll |
| `EIO` | Hardware error | Reset device, check hardware |
| `EBUSY` | Device busy | Close other applications |
| `ENOMEM` | Out of memory | Reduce buffer count |

**Error handling tips:**
1. Always check return values
2. Use `perror()` hoặc `strerror(errno)` để debug
3. Cleanup resources trong error path
4. Log buffer state khi debug

---

## 9.5. V4L2 User Space Tools

### 9.5.1. v4l2-ctl Overview

`v4l2-ctl` là command-line tool mạnh mẽ để:
- Query device capabilities
- List và enumerate formats
- Set format, controls
- Capture frames
- Debug V4L2 issues

**Installation:**
```bash
# Debian/Ubuntu
sudo apt-get install v4l-utils

# Fedora/RHEL
sudo dnf install v4l-utils
```

### 9.5.2. Common v4l2-ctl Commands

#### **List All Video Devices**

```bash
$ v4l2-ctl --list-devices
Integrated Camera: Integrated C (usb-0000:00:14.0-8):
    /dev/video0
    /dev/video1
```

#### **Query Device Info**

```bash
$ v4l2-ctl -d /dev/video0 -D
Driver Info:
    Driver name   : uvcvideo
    Card type     : Integrated Camera
    Bus info      : usb-0000:00:14.0-8
    Driver version: 5.4.60
    Capabilities  : 0x84A00001
        Video Capture
        Streaming
        Extended Pix Format
```

#### **List Supported Formats**

```bash
$ v4l2-ctl --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
    Index       : 0
    Type        : Video Capture
    Pixel Format: 'MJPG' (compressed)
    Name        : Motion-JPEG
        Size: Discrete 1280x720
            Interval: Discrete 0.033s (30.000 fps)
        Size: Discrete 640x480
            Interval: Discrete 0.033s (30.000 fps)
    
    Index       : 1
    Type        : Video Capture
    Pixel Format: 'YUYV'
    Name        : YUYV 4:2:2
        Size: Discrete 640x480
            Interval: Discrete 0.033s (30.000 fps)
```

#### **Get Current Format**

```bash
$ v4l2-ctl --get-fmt-video
Format Video Capture:
    Width/Height  : 640/480
    Pixel Format  : 'YUYV' (YUYV 4:2:2)
    Field         : None
    Bytes per Line: 1280
    Size Image    : 614400
    Colorspace    : sRGB
```

#### **Set Format**

```bash
# Set resolution and pixel format
$ v4l2-ctl --set-fmt-video=width=1280,height=720,pixelformat=MJPG

# Set frame rate
$ v4l2-ctl --set-parm=30
Frame rate set to 30.000 fps
```

### 9.5.3. Controls Management

#### **List All Controls**

```bash
$ v4l2-ctl -L
                     brightness 0x00980900 (int)    : min=0 max=255 step=1 default=128 value=128
                       contrast 0x00980901 (int)    : min=0 max=255 step=1 default=32 value=32
                     saturation 0x00980902 (int)    : min=0 max=100 step=1 default=64 value=64
                            hue 0x00980903 (int)    : min=-180 max=180 step=1 default=0 value=0
 white_balance_temperature_auto 0x0098090c (bool)   : default=1 value=1
                          gamma 0x00980910 (int)    : min=90 max=150 step=1 default=120 value=120
           power_line_frequency 0x00980918 (menu)   : min=0 max=2 default=1 value=1
                0: Disabled
                1: 50 Hz
                2: 60 Hz
                      sharpness 0x0098091b (int)    : min=0 max=7 step=1 default=3 value=3
         backlight_compensation 0x0098091c (int)    : min=0 max=2 step=1 default=1 value=1
```

#### **Get/Set Control**

```bash
# Get brightness
$ v4l2-ctl --get-ctrl brightness
brightness: 128

# Set brightness
$ v4l2-ctl --set-ctrl brightness=192

# Set multiple controls
$ v4l2-ctl --set-ctrl brightness=200,contrast=50,saturation=80
```

### 9.5.4. Capturing Frames

#### **Capture Single Frame**

```bash
# Capture MJPEG compressed frame
$ v4l2-ctl --set-fmt-video=width=640,height=480,pixelformat=MJPG \
           --stream-mmap --stream-count=1 \
           --stream-to=capture.jpg

# Capture raw YUV frame
$ v4l2-ctl --set-fmt-video=width=640,height=480,pixelformat=YUYV \
           --stream-mmap --stream-count=1 \
           --stream-to=capture.raw
```

#### **Convert Raw YUV to PNG**

```bash
$ ffmpeg -f rawvideo -s 640x480 -pix_fmt yuyv422 \
         -i capture.raw capture.png
```

#### **Capture Multiple Frames**

```bash
# Capture 100 frames
$ v4l2-ctl --set-fmt-video=width=1280,height=720,pixelformat=MJPG \
           --stream-mmap --stream-count=100 \
           --stream-to=frames.mjpg
```

### 9.5.5. Debugging with v4l2-ctl

#### **Enable V4L2 Core Debug**

```bash
# Enable videobuf2 debug
$ echo 0x3 > /sys/module/videobuf2_v4l2/parameters/debug
$ echo 0x3 > /sys/module/videobuf2_common/parameters/debug

# Check kernel logs
$ dmesg | tail -50
```

#### **Enable Device-Specific Debug**

```bash
# Enable debug for /dev/video0
$ echo 0x3 > /sys/class/video4linux/video0/dev_debug

# Run capture and check logs
$ v4l2-ctl --stream-mmap --stream-count=1 --stream-to=test.raw
$ dmesg | grep video0
```

### 9.5.6. v4l2-compliance Testing

```bash
# Test device compliance
$ v4l2-compliance -d /dev/video0

v4l2-compliance SHA   : not available

Driver Info:
    Driver name      : uvcvideo
    Card type        : Integrated Camera
    Bus info         : usb-0000:00:14.0-8
    Driver version   : 5.4.60
    Capabilities     : 0x84A00001
        Video Capture
        Streaming

Compliance test for device /dev/video0:

Required ioctls:
    test VIDIOC_QUERYCAP: OK

Allow for multiple opens:
    test second video open: OK
    test VIDIOC_QUERYCAP: OK
    test VIDIOC_G/S_PRIORITY: OK

... (more tests)

Total: 45, Succeeded: 45, Failed: 0, Warnings: 0
```

### 9.5.7. Quick Reference Table

| Task | Command |
|------|---------|
| List devices | `v4l2-ctl --list-devices` |
| Device info | `v4l2-ctl -d /dev/video0 -D` |
| List formats | `v4l2-ctl --list-formats-ext` |
| Get format | `v4l2-ctl --get-fmt-video` |
| Set format | `v4l2-ctl --set-fmt-video=width=W,height=H,pixelformat=FMT` |
| Set FPS | `v4l2-ctl --set-parm=30` |
| List controls | `v4l2-ctl -L` |
| Set control | `v4l2-ctl --set-ctrl brightness=200` |
| Capture frame | `v4l2-ctl --stream-mmap --stream-count=1 --stream-to=file` |
| Test compliance | `v4l2-compliance -d /dev/video0` |

---

## 9.6. [ADVANCED] Advanced Topics

### 9.6.1. SELECT/POLL for Non-blocking I/O

**Tại sao dùng SELECT/POLL?**
- Avoid blocking trên `VIDIOC_DQBUF`
- Multiple device monitoring
- Timeout control
- Integration với event loop

**Using select():**

```c
#include <sys/select.h>

int wait_for_frame(int timeout_sec)
{
    fd_set fds;
    struct timeval tv;
    int r;
    
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    
    r = select(fd + 1, &fds, NULL, NULL, &tv);
    
    if (r == -1) {
        perror("select");
        return -1;
    }
    
    if (r == 0) {
        fprintf(stderr, "Timeout\n");
        return 0;
    }
    
    // Frame ready
    return 1;
}

// Usage in capture loop
while (capturing) {
    if (wait_for_frame(5) > 0) {
        // DQBUF will not block
        xioctl(fd, VIDIOC_DQBUF, &buf);
        process_frame(...);
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
}
```

**Using poll():**

```c
#include <poll.h>

int wait_for_frame_poll(int timeout_ms)
{
    struct pollfd pfd;
    int r;
    
    pfd.fd = fd;
    pfd.events = POLLIN;
    
    r = poll(&pfd, 1, timeout_ms);
    
    if (r == -1) {
        perror("poll");
        return -1;
    }
    
    if (r == 0) {
        fprintf(stderr, "Timeout\n");
        return 0;
    }
    
    if (pfd.revents & POLLIN) {
        return 1;  // Frame ready
    }
    
    return 0;
}
```

### 9.6.2. Multi-planar Buffers

**Single-planar vs Multi-planar:**

```
SINGLE-PLANAR (YUV422 packed - YUYV)
┌─────────────────────────────────┐
│ Y0U0Y1V0 Y2U1Y3V1 ... (contiguous) │
└─────────────────────────────────┘
     ↑ One buffer, one plane

MULTI-PLANAR (YUV420 - NV12)
┌─────────────────────┐  Plane 0: Y
│ Y Y Y Y Y Y Y Y ... │
└─────────────────────┘
┌───────────┐            Plane 1: UV (interleaved)
│ U V U V   │
└───────────┘
     ↑ One buffer, multiple planes
```

**Multi-planar API:**

```c
struct v4l2_format fmt;
CLEAR(fmt);

fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
fmt.fmt.pix_mp.width = WIDTH;
fmt.fmt.pix_mp.height = HEIGHT;
fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
fmt.fmt.pix_mp.num_planes = 2;  // Y plane + UV plane

xioctl(fd, VIDIOC_S_FMT, &fmt);

// Buffer for multi-planar
struct v4l2_buffer buf;
struct v4l2_plane planes[VIDEO_MAX_PLANES];

CLEAR(buf);
CLEAR(planes);

buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
buf.memory = V4L2_MEMORY_MMAP;
buf.m.planes = planes;
buf.length = 2;  // Number of planes

xioctl(fd, VIDIOC_DQBUF, &buf);

// Access plane data
void *y_data = buffers[buf.index].plane[0].start;
void *uv_data = buffers[buf.index].plane[1].start;
```

### 9.6.3. DMABUF Zero-Copy Pipeline

**Camera → Display pipeline (V4L2 → DRM):**

```c
// 1. Allocate buffers on camera (V4L2 capture)
struct v4l2_requestbuffers req;
req.count = 4;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_MMAP;
xioctl(camera_fd, VIDIOC_REQBUFS, &req);

// 2. Export buffers as DMABUF
int dmabuf_fds[4];
for (i = 0; i < 4; i++) {
    struct v4l2_exportbuffer expbuf;
    expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    expbuf.index = i;
    xioctl(camera_fd, VIDIOC_EXPBUF, &expbuf);
    dmabuf_fds[i] = expbuf.fd;
}

// 3. Import DMABUF to display (DRM)
for (i = 0; i < 4; i++) {
    // Create DRM framebuffer from DMABUF fd
    drmModeAddFB2WithModifiers(drm_fd, width, height, format,
                                handles, pitches, offsets,
                                modifiers, &fb_ids[i], 0);
}

// 4. Capture loop - Zero copy!
while (running) {
    // Get filled buffer from camera
    xioctl(camera_fd, VIDIOC_DQBUF, &buf);
    
    // Display buffer (no memcpy!)
    drmModeSetPlane(drm_fd, plane_id, crtc_id, fb_ids[buf.index],
                    0, 0, 0, width, height, 0, 0, width<<16, height<<16);
    
    // Return buffer to camera
    xioctl(camera_fd, VIDIOC_QBUF, &buf);
}
```

**Benefits:**
- ✅ Zero memory copy
- ✅ Lower CPU usage
- ✅ Lower latency
- ✅ Higher throughput

### 9.6.4. Performance Optimization Tips

#### **1. Buffer Count Optimization**

```c
// Too few buffers → Frame drops
req.count = 2;  // ❌ Risk of underrun

// Optimal for most cases
req.count = 4;  // ✅ Good balance

// Many buffers → Higher latency
req.count = 10; // ⚠️ Only if needed (e.g., slow processing)
```

#### **2. Frame Processing Optimization**

```c
// ❌ BAD: Process in capture thread
xioctl(fd, VIDIOC_DQBUF, &buf);
heavy_processing(buffer);  // Blocks next frame!
xioctl(fd, VIDIOC_QBUF, &buf);

// ✅ GOOD: Separate threads
pthread_t capture_thread, process_thread;
queue_t *frame_queue;

void *capture_worker(void *arg) {
    while (running) {
        xioctl(fd, VIDIOC_DQBUF, &buf);
        queue_push(frame_queue, &buf);  // Quick enqueue
    }
}

void *process_worker(void *arg) {
    while (running) {
        buf = queue_pop(frame_queue);
        heavy_processing(buffer);
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
}
```

#### **3. Memory Alignment**

```c
// For DMA efficiency, align buffers
#define ALIGN_SIZE 4096

void *aligned_malloc(size_t size) {
    void *ptr;
    if (posix_memalign(&ptr, ALIGN_SIZE, size) != 0) {
        return NULL;
    }
    return ptr;
}

// Use aligned buffers for USERPTR
buffers[i].start = aligned_malloc(buffer_size);
```

#### **4. Avoid Unnecessary Memcpy**

```c
// ❌ BAD: Extra copy
xioctl(fd, VIDIOC_DQBUF, &buf);
memcpy(temp_buffer, buffers[buf.index].start, buf.bytesused);
process(temp_buffer);

// ✅ GOOD: Process directly
xioctl(fd, VIDIOC_DQBUF, &buf);
process(buffers[buf.index].start);  // No copy!
```

#### **5. CPU Affinity for RT Performance**

```c
#include <sched.h>

void set_cpu_affinity(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

// Dedicate CPU core to capture thread
set_cpu_affinity(2);  // Run on CPU 2
```

#### **6. Real-time Priority**

```c
#include <sched.h>

void set_realtime_priority(void) {
    struct sched_param param;
    param.sched_priority = 50;  // RT priority
    
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler");
    }
}
```

#### **7. Prefetch Next Frame**

```c
// Prime all buffers for smoother capture
for (i = 0; i < n_buffers; i++) {
    xioctl(fd, VIDIOC_QBUF, &buf[i]);
}

// In loop, always keep buffers queued
xioctl(fd, VIDIOC_DQBUF, &buf);
process_frame(buf);
xioctl(fd, VIDIOC_QBUF, &buf);  // Immediately requeue
```

### 9.6.5. Common Performance Issues

| Issue | Symptom | Solution |
|-------|---------|----------|
| Frame drops | Missing frames | Increase buffer count, optimize processing |
| High latency | Delayed frames | Reduce buffer count, use RT priority |
| CPU overload | High CPU usage | Use hardware encoder, reduce resolution |
| Memory bandwidth | Slow transfer | Use DMABUF, avoid memcpy |
| Jitter | Inconsistent FPS | Use RT scheduler, dedicate CPU core |

---

## 9.7. Tổng Kết

### 9.7.1. Key Takeaways

**✅ V4L2 User Space Programming Flow:**
```
1. Open device
2. Query capabilities
3. Set format (resolution, pixel format, FPS)
4. Request buffers (MMAP/USERPTR/DMABUF)
5. Prime buffers (enqueue all)
6. Start streaming
7. Capture loop (DQBUF → process → QBUF)
8. Stop streaming
9. Cleanup
```

**✅ Critical Points:**

1. **ioctl sequence MUST be correct**
   - Wrong order → `-EINVAL` errors
   - Always: REQBUFS → QUERYBUF/mmap → QBUF → STREAMON → DQBUF

2. **Driver may adjust parameters**
   - Always verify after `VIDIOC_S_FMT`
   - Check granted buffer count after `VIDIOC_REQBUFS`

3. **Buffer types selection:**
   - **MMAP**: General purpose, most compatible
   - **USERPTR**: Custom memory management
   - **DMABUF**: Zero-copy device-to-device

4. **Error handling is critical**
   - Check all return values
   - Handle `EAGAIN`, `EINTR`, `EIO` properly
   - Always cleanup resources

5. **Performance matters**
   - Use enough buffers (4 is good default)
   - Avoid unnecessary memcpy
   - Consider threading for heavy processing
   - Use DMABUF for zero-copy pipelines

### 9.7.2. Common Pitfalls

**❌ Mistakes to Avoid:**

1. **Forgetting to prime buffers**
   ```c
   // ❌ WRONG
   xioctl(fd, VIDIOC_STREAMON, &type);
   xioctl(fd, VIDIOC_DQBUF, &buf);  // Blocks forever!
   
   // ✅ CORRECT
   for (i = 0; i < n_buffers; i++)
       xioctl(fd, VIDIOC_QBUF, &buf[i]);  // Prime first
   xioctl(fd, VIDIOC_STREAMON, &type);
   ```

2. **Not checking buffer index**
   ```c
   // ❌ WRONG
   xioctl(fd, VIDIOC_DQBUF, &buf);
   process(buffers[buf.index].start);  // Could be out of bounds!
   
   // ✅ CORRECT
   xioctl(fd, VIDIOC_DQBUF, &buf);
   if (buf.index < n_buffers) {
       process(buffers[buf.index].start);
   }
   ```

3. **Memory leaks**
   ```c
   // ❌ WRONG
   buffers = malloc(...);
   // ... error occurs, return without free
   
   // ✅ CORRECT
   if (error)
       goto cleanup;
   
   cleanup:
       if (buffers) free(buffers);
   ```

4. **Accessing locked buffers**
   ```c
   // ❌ WRONG
   xioctl(fd, VIDIOC_QBUF, &buf);  // Buffer locked
   memcpy(dst, buffers[buf.index].start, size);  // UNDEFINED!
   
   // ✅ CORRECT
   xioctl(fd, VIDIOC_DQBUF, &buf);  // Dequeue first
   memcpy(dst, buffers[buf.index].start, size);
   xioctl(fd, VIDIOC_QBUF, &buf);   // Then requeue
   ```

5. **Wrong buffer type in ioctl**
   ```c
   // ❌ WRONG
   req.memory = V4L2_MEMORY_MMAP;
   xioctl(fd, VIDIOC_REQBUFS, &req);
   
   buf.memory = V4L2_MEMORY_USERPTR;  // Mismatch!
   xioctl(fd, VIDIOC_QBUF, &buf);
   
   // ✅ CORRECT - Keep consistent
   buf.memory = req.memory;
   ```

### 9.7.3. Debugging Checklist

**When things go wrong:**

- [ ] Check device exists: `ls -l /dev/video*`
- [ ] Check permissions: `sudo chmod 666 /dev/video0`
- [ ] Verify capabilities: `v4l2-ctl -D`
- [ ] List formats: `v4l2-ctl --list-formats-ext`
- [ ] Test with v4l2-ctl first
- [ ] Enable debug logs: `echo 0x3 > /sys/.../debug`
- [ ] Check dmesg for kernel errors
- [ ] Run v4l2-compliance test
- [ ] Verify ioctl sequence order
- [ ] Check buffer count (≥2 for MMAP)
- [ ] Verify all QBUF before STREAMON
- [ ] Check for `-EINVAL` → wrong parameters
- [ ] Check for `-EAGAIN` → no buffer ready
- [ ] Verify buffer not accessed while locked

### 9.7.4. Quick Reference: ioctl Commands Summary

| Phase | ioctl | Purpose |
|-------|-------|---------|
| **Init** | `VIDIOC_QUERYCAP` | Query device capabilities |
| **Format** | `VIDIOC_ENUM_FMT` | List supported formats |
| | `VIDIOC_G_FMT` | Get current format |
| | `VIDIOC_S_FMT` | Set new format |
| | `VIDIOC_TRY_FMT` | Validate format |
| **Controls** | `VIDIOC_QUERYCTRL` | Query control info |
| | `VIDIOC_G_CTRL` | Get control value |
| | `VIDIOC_S_CTRL` | Set control value |
| **Buffers** | `VIDIOC_REQBUFS` | Request buffer allocation |
| | `VIDIOC_QUERYBUF` | Query buffer details |
| | `VIDIOC_EXPBUF` | Export DMABUF fd |
| **Streaming** | `VIDIOC_QBUF` | Enqueue buffer |
| | `VIDIOC_DQBUF` | Dequeue buffer |
| | `VIDIOC_STREAMON` | Start streaming |
| | `VIDIOC_STREAMOFF` | Stop streaming |

### 9.7.5. Tools Summary

| Tool | Purpose | Example |
|------|---------|---------|
| `v4l2-ctl` | Query/configure devices | `v4l2-ctl -D` |
| `v4l2-compliance` | Test driver compliance | `v4l2-compliance -d /dev/video0` |
| `media-ctl` | Configure media pipeline | `media-ctl -p` |
| `yavta` | Test application | `yavta -c10 /dev/video0` |
| `ffmpeg` | Convert raw frames | `ffmpeg -f rawvideo ...` |

### 9.7.6. Further Learning

**Recommended resources:**
- Kernel documentation: `Documentation/userspace-api/media/v4l/`
- V4L2 API spec: https://linuxtv.org/downloads/v4l-dvb-apis/
- Example code: `v4l2-utils` package source
- GStreamer v4l2 plugins for production use

**Next steps:**
- Implement complete capture application
- Experiment with different buffer types
- Try DMABUF zero-copy pipeline
- Integrate with video processing libraries
- Test real-time performance

---

## Appendix A: Complete Example Compilation

**Compile example:**
```bash
# Simple compilation
gcc -o v4l2_capture capture.c -Wall

# With optimization
gcc -o v4l2_capture capture.c -O2 -Wall

# With pthread support
gcc -o v4l2_capture capture.c -pthread -O2 -Wall

# With debugging
gcc -o v4l2_capture capture.c -g -Wall -DDEBUG
```

**Run example:**
```bash
# Capture 10 frames
./v4l2_capture 10

# With sudo if permission needed
sudo ./v4l2_capture 100

# With specific device
./v4l2_capture /dev/video1 50
```

---

## Appendix B: Pixel Format FourCC Codes

| FourCC | Format | Description |
|--------|--------|-------------|
| `YUYV` | YUV 4:2:2 | Packed, interleaved |
| `UYVY` | YUV 4:2:2 | Packed, U first |
| `NV12` | YUV 4:2:0 | Semi-planar, Y + UV |
| `NV21` | YUV 4:2:0 | Semi-planar, Y + VU |
| `YU12` | YUV 4:2:0 | Planar, Y + U + V |
| `YV12` | YUV 4:2:0 | Planar, Y + V + U |
| `RGB3` | RGB 8:8:8 | 24-bit RGB |
| `BGR3` | BGR 8:8:8 | 24-bit BGR |
| `MJPG` | Motion JPEG | Compressed |
| `H264` | H.264 | Compressed video |

---

## Appendix C: Error Codes Reference

| errno | Meaning | Common cause |
|-------|---------|--------------|
| `EINVAL` | Invalid argument | Wrong parameter, wrong buffer type |
| `EAGAIN` | Try again | No buffer ready (non-blocking) |
| `EBUSY` | Device busy | Already opened by another app |
| `EIO` | I/O error | Hardware problem |
| `ENOMEM` | Out of memory | Too many buffers requested |
| `EBADF` | Bad file descriptor | Device not opened |
| `EINTR` | Interrupted | Signal received, retry |
| `ENODEV` | No such device | Device unplugged |

---

## Final Words

Chapter 9 đã đưa bạn qua hành trình **từ lý thuyết đến thực hành** với V4L2 user space API:

**Part 1:**
- ✅ API fundamentals
- ✅ Device property management
- ✅ Buffer management concepts

**Part 2:**
- ✅ Complete capture application
- ✅ v4l2-ctl tools mastery
- ✅ Advanced optimization techniques

**Bạn đã học được:**
1. Cách mở và cấu hình video device
2. 3 buffer types và khi nào dùng
3. Complete capture application từng bước
4. Tools để debug và test
5. Advanced topics: SELECT/POLL, DMABUF, Performance tuning

**Hành trình tiếp theo:**
- Áp dụng vào dự án thực tế
- Tích hợp với image processing
- Video encoding/streaming
- Real-time video analytics

---

**🎉 HOÀN THÀNH CHAPTER 9! 🎉**

**Next Chapter: Linux Kernel Power Management**  
*Quản lý điện năng để tối ưu hóa battery life và hiệu suất*

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024 | Initial release - Complete Chapter 9 documentation |

---

**Contact & Feedback:**
- Issues/Questions: Open an issue on GitHub
- Contributions: Pull requests welcome
- Discussion: Join V4L2 mailing list

---

**License:** This documentation is for educational purposes.  
**Source:** Based on "Linux Device Driver Development (Edition 2)"

---

## About V4L2

Video4Linux2 (V4L2) là Linux kernel API cho video capture và output devices. Được maintain bởi Linux Media community, V4L2 hỗ trợ:
- USB webcams
- MIPI CSI cameras
- HDMI capture cards
- Video encoders/decoders
- Image signal processors (ISP)

**V4L2 ecosystem:**
- Kernel drivers (Chapter 7, 8)
- User space API (Chapter 9)
- GStreamer integration
- OpenCV support
- Media controller framework

**Community:**
- Mailing list: linux-media@vger.kernel.org
- IRC: #v4l on irc.libera.chat
- Documentation: https://linuxtv.org

---

**END OF CHAPTER 9 - PART 2** 📚