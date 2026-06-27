# CHAPTER 9: LEVERAGING V4L2 API FROM USER SPACE - PART 1

> **Tài liệu học tập về V4L2 User Space API**  
> Mục tiêu: Hiểu cách sử dụng V4L2 từ user space để tương tác với video devices

---

## Tổng Quan Chapter 9

Chapter này tập trung vào **user space programming** với V4L2 API - cách application tương tác với video device driver đã học ở Chapter 7 và 8.

**Nội dung Part 1:**
- 9.1. V4L2 User Space API Overview
- 9.2. Video Device Property Management
- 9.3. Buffer Management (Lý thuyết & So sánh)

**Nội dung Part 2** (sẽ viết tiếp):
- 9.4. Complete Capture Application
- 9.5. V4L2 User Space Tools
- 9.6. Advanced Topics
- 9.7. Tổng kết

---

## 9.1. V4L2 User Space API Overview

### 9.1.1. Giới thiệu

V4L2 user space API là **interface** giữa application và kernel driver, cho phép:
- Mở/đóng video device
- Query device capabilities
- Cấu hình format, resolution, frame rate
- Quản lý buffer và streaming
- Điều khiển camera properties (brightness, contrast...)

**Hai cách sử dụng V4L2 từ user space:**
1. **All-in-one tools**: GStreamer, FFmpeg → Dùng cho rapid prototyping
2. **Custom application**: Dùng V4L2 API trực tiếp → Kiểm soát chi tiết

Chapter này tập trung vào cách 2.

### 9.1.2. Core Functions

V4L2 API sử dụng **ít functions** nhưng **nhiều ioctl commands**:

```c
#include <linux/videodev2.h>

// Core system calls
open()    // Mở video device
close()   // Đóng device
ioctl()   // Gửi commands đến driver
mmap()    // Map buffer từ kernel → user space
read()    // Đọc dữ liệu (nếu driver hỗ trợ)
write()   // Ghi dữ liệu (output devices)
```

### 9.1.3. Key ioctl Commands

| ioctl Command | Chức năng |
|---------------|-----------|
| `VIDIOC_QUERYCAP` | Query device capabilities |
| `VIDIOC_ENUM_FMT` | Enumerate supported formats |
| `VIDIOC_G_FMT` | Get current format |
| `VIDIOC_S_FMT` | Set new format |
| `VIDIOC_TRY_FMT` | Validate format trước khi set |
| `VIDIOC_REQBUFS` | Request buffers allocation |
| `VIDIOC_QUERYBUF` | Query buffer info (physical address) |
| `VIDIOC_QBUF` | Enqueue buffer (cho driver fill data) |
| `VIDIOC_DQBUF` | Dequeue filled buffer |
| `VIDIOC_STREAMON` | Bắt đầu streaming |
| `VIDIOC_STREAMOFF` | Dừng streaming |

### 9.1.4. V4L2 Data Structures

```c
// Device capabilities
struct v4l2_capability {
    __u8 driver[16];      // Driver name
    __u8 card[32];        // Device name
    __u8 bus_info[32];    // Bus location
    __u32 version;        // Kernel version
    __u32 capabilities;   // Device capabilities
    __u32 device_caps;    // Device-specific caps
    __u32 reserved[3];
};

// Buffer format
struct v4l2_format {
    __u32 type;                    // Buffer type
    union {
        struct v4l2_pix_format pix;     // Video capture
        struct v4l2_window win;          // Video overlay
        // ... other types
    } fmt;
};

// Buffer info
struct v4l2_buffer {
    __u32 index;          // Buffer index
    __u32 type;           // Buffer type
    __u32 bytesused;      // Data size
    __u32 flags;          // Buffer state
    __u32 memory;         // Memory type (MMAP/USERPTR/DMABUF)
    union {
        __u32 offset;     // MMAP: offset for mmap()
        unsigned long userptr;  // USERPTR: user space address
        __s32 fd;         // DMABUF: file descriptor
    } m;
    __u32 length;         // Buffer size
    // ... more fields
};

// Request buffers
struct v4l2_requestbuffers {
    __u32 count;          // Number of buffers
    __u32 type;           // Buffer type
    __u32 memory;         // Memory type
    __u32 reserved[2];
};
```

### 9.1.5. Typical Capture Sequence

```
┌─────────────────────────────────────────────────┐
│         V4L2 CAPTURE SEQUENCE FLOW              │
└─────────────────────────────────────────────────┘

1. open("/dev/video0")
         ↓
2. VIDIOC_QUERYCAP         → Check capabilities
         ↓
3. VIDIOC_S_FMT            → Set format (resolution, pixel format)
         ↓
4. VIDIOC_REQBUFS          → Request N buffers
         ↓
5. VIDIOC_QUERYBUF + mmap  → Map buffers to user space
         ↓
6. VIDIOC_QBUF (×N)        → Enqueue all buffers (prime buffers)
         ↓
7. VIDIOC_STREAMON         → Start streaming
         ↓
8. CAPTURE LOOP:
   ┌──────────────────────────────────────┐
   │ VIDIOC_DQBUF  → Get filled buffer    │
   │ process_frame() → Xử lý data         │
   │ VIDIOC_QBUF   → Return buffer        │
   └────────────↑─────────────────────────┘
                │
                └── Repeat loop
         ↓
9. VIDIOC_STREAMOFF        → Stop streaming
         ↓
10. munmap() + close()     → Cleanup
```

### 9.1.6. ioctl Error Handling

ioctl có thể bị **interrupt**, cần retry:

```c
static int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    
    return r;
}
```

**Lý do**: System call có thể bị interrupt bởi signal → errno = EINTR → Retry

---

## 9.2. Video Device Property Management

### 9.2.1. Opening Video Device

```c
static const char *dev_name = "/dev/video0";
int fd;

// Open in blocking mode
fd = open(dev_name, O_RDWR);
if (fd == -1) {
    perror("Failed to open device");
    return -1;
}

// Open in non-blocking mode (DQBUF won't block)
fd = open(dev_name, O_RDWR | O_NONBLOCK);
```

**Blocking vs Non-blocking:**
- **Blocking**: `VIDIOC_DQBUF` chờ cho đến khi có buffer filled
- **Non-blocking**: `VIDIOC_DQBUF` return ngay với `EAGAIN` nếu chưa có buffer

**Close device:**
```c
close(fd);
```

### 9.2.2. Querying Device Capabilities

**Tại sao cần query capabilities?**
- Kiểm tra device type (capture/output/overlay)
- Xác định I/O methods hỗ trợ (streaming/read-write)
- Biết được features available

```c
#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct v4l2_capability cap;
CLEAR(cap);

if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
    if (errno == EINVAL) {
        fprintf(stderr, "%s is not a V4L2 device\n", dev_name);
    }
    return -1;
}

// Check device type
if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "Not a video capture device\n");
    return -1;
}

// Check streaming support
if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    fprintf(stderr, "Device does not support streaming\n");
    return -1;
}

// Check read/write support
if (cap.capabilities & V4L2_CAP_READWRITE) {
    printf("Device supports read/write I/O\n");
}
```

**Common capability flags:**
```c
V4L2_CAP_VIDEO_CAPTURE         // Video capture device
V4L2_CAP_VIDEO_OUTPUT          // Video output device
V4L2_CAP_STREAMING             // Streaming I/O (MMAP/USERPTR/DMABUF)
V4L2_CAP_READWRITE             // read()/write() support
V4L2_CAP_VIDEO_CAPTURE_MPLANE  // Multi-planar capture
```

### 9.2.3. Format Negotiation

#### **Get Current Format**

```c
struct v4l2_format fmt;
CLEAR(fmt);

fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

if (xioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
    perror("VIDIOC_G_FMT failed");
    return -1;
}

printf("Current format:\n");
printf("  Width: %d\n", fmt.fmt.pix.width);
printf("  Height: %d\n", fmt.fmt.pix.height);
printf("  Pixel Format: %.4s\n", (char*)&fmt.fmt.pix.pixelformat);
```

#### **Set New Format**

```c
#define WIDTH  1920
#define HEIGHT 1080

// Set desired format
fmt.fmt.pix.width = WIDTH;
fmt.fmt.pix.height = HEIGHT;
fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // YUV422
fmt.fmt.pix.field = V4L2_FIELD_ANY;
fmt.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;

if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
    perror("VIDIOC_S_FMT failed");
    return -1;
}

// Check what driver actually granted
if (fmt.fmt.pix.width != WIDTH || fmt.fmt.pix.height != HEIGHT) {
    fprintf(stderr, "Warning: Driver changed to %dx%d\n",
            fmt.fmt.pix.width, fmt.fmt.pix.height);
}

if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
    fprintf(stderr, "Error: Pixel format not accepted\n");
    return -1;
}
```

**Common pixel formats:**
- `V4L2_PIX_FMT_YUYV` - YUV422 interleaved
- `V4L2_PIX_FMT_NV12` - YUV420 semi-planar
- `V4L2_PIX_FMT_RGB24` - RGB888 packed
- `V4L2_PIX_FMT_MJPEG` - Motion JPEG compressed

#### **Try Format (Validate trước khi set)**

```c
struct v4l2_format fmt;
CLEAR(fmt);

fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
fmt.fmt.pix.width = 1920;
fmt.fmt.pix.height = 1080;
fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

// Try format - driver sẽ adjust nếu không support
if (xioctl(fd, VIDIOC_TRY_FMT, &fmt) == -1) {
    perror("VIDIOC_TRY_FMT failed");
}

// Check adjusted values
printf("Driver can support: %dx%d\n", 
       fmt.fmt.pix.width, fmt.fmt.pix.height);

// If OK, then set
if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
    perror("VIDIOC_S_FMT failed");
}
```

### 9.2.4. Frame Rate Control

```c
#define FRAMERATE 30

struct v4l2_streamparm parm;
CLEAR(parm);

parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

// Query current streaming parameters
if (xioctl(fd, VIDIOC_G_PARM, &parm) == -1) {
    perror("VIDIOC_G_PARM failed");
    return -1;
}

// Check if frame rate control is supported
if (parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = FRAMERATE;
    
    if (xioctl(fd, VIDIOC_S_PARM, &parm) == -1) {
        perror("Unable to set frame rate");
    } else {
        printf("Frame rate set to %d fps\n",
               parm.parm.capture.timeperframe.denominator);
    }
}
```

**Lưu ý**: Driver có thể không hỗ trợ hoặc adjust frame rate.

### 9.2.5. Controls (Brightness, Contrast...)

```c
// Query control
struct v4l2_queryctrl qctrl;
CLEAR(qctrl);

qctrl.id = V4L2_CID_BRIGHTNESS;

if (xioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == -1) {
    if (errno != EINVAL) {
        perror("VIDIOC_QUERYCTRL");
    } else {
        printf("Brightness control not supported\n");
    }
} else {
    printf("Brightness: min=%d max=%d step=%d default=%d\n",
           qctrl.minimum, qctrl.maximum, qctrl.step, qctrl.default_value);
}

// Set control value
struct v4l2_control ctrl;
ctrl.id = V4L2_CID_BRIGHTNESS;
ctrl.value = 128;  // Set to desired value

if (xioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {
    perror("VIDIOC_S_CTRL");
}

// Get current value
if (xioctl(fd, VIDIOC_G_CTRL, &ctrl) == -1) {
    perror("VIDIOC_G_CTRL");
} else {
    printf("Current brightness: %d\n", ctrl.value);
}
```

**Common controls:**
- `V4L2_CID_BRIGHTNESS` - Độ sáng
- `V4L2_CID_CONTRAST` - Độ tương phản
- `V4L2_CID_SATURATION` - Độ bão hòa màu
- `V4L2_CID_HUE` - Sắc màu
- `V4L2_CID_EXPOSURE_AUTO` - Auto exposure

---

## 9.3. Buffer Management

### 9.3.1. Buffer Management Concept

**V4L2 Buffer Queue System:**

```
┌──────────────────────────────────────────────────────────┐
│                    BUFFER FLOW                           │
└──────────────────────────────────────────────────────────┘

USER SPACE              DRIVER (INPUT Queue)      V4L2 (OUTPUT Queue)
    │                           │                         │
    │ QBUF (empty) ────────────→│                         │
    │ QBUF (empty) ────────────→│                         │
    │ QBUF (empty) ────────────→│                         │
    │                           │                         │
    │ STREAMON ────────────────→│                         │
    │                           │                         │
    │                           │←─── DMA fills ─────────→│
    │                           │     buffer              │
    │                           │                         │
    │←─── DQBUF (filled) ───────────────────────────────←│
    │                           │                         │
    │  process_frame()          │                         │
    │                           │                         │
    │ QBUF (empty) ────────────→│                         │
    │                           │                         │
    │←─── DQBUF (filled) ───────────────────────────────←│
    │                           │                         │
    │  ... loop continues ...   │                         │
```

**Hai queue:**
1. **INPUT Queue** (Driver queue): Chứa empty buffers chờ được fill
2. **OUTPUT Queue** (User queue): Chứa filled buffers chờ được dequeue

**Buffer lifecycle:**
1. **REQBUFS** → Allocate buffers
2. **QBUF** → Enqueue empty buffer vào INPUT queue
3. **Driver fills** → Buffer đầy → Move sang OUTPUT queue
4. **DQBUF** → Dequeue filled buffer từ OUTPUT queue
5. **Process** → Application xử lý data
6. **QBUF** → Return buffer vào INPUT queue
7. Repeat từ step 3

### 9.3.2. Buffer Types - So Sánh Chi Tiết

V4L2 hỗ trợ 3 loại buffer memory:

| Feature | MMAP | USERPTR | DMABUF |
|---------|------|---------|--------|
| **Memory allocation** | Kernel allocates | User allocates | Shared DMA buffer |
| **Performance** | Good | Good | Best (zero-copy) |
| **Complexity** | Medium | Low | High |
| **Use case** | General purpose | Custom memory management | Device-to-device sharing |
| **Driver support** | Most common | Common | Less common |

#### **MMAP (Memory Mapped)**

**Đặc điểm:**
- Kernel allocates buffers
- User space map vào virtual memory bằng `mmap()`
- Phổ biến nhất, hỗ trợ rộng rãi

**Khi nào dùng:**
- ✅ General capture application
- ✅ Không cần control memory allocation
- ✅ Muốn driver quản lý memory

**Flow:**
```
REQBUFS → QUERYBUF → mmap() → QBUF → DQBUF
```

#### **USERPTR (User Pointer)**

**Đặc điểm:**
- Application allocates buffers bằng `malloc()`
- Pass pointer vào kernel
- Kernel fill data vào user's buffer

**Khi nào dùng:**
- ✅ Cần control memory allocation
- ✅ Integration với existing buffer management
- ✅ Custom memory allocator

**Flow:**
```
malloc() → REQBUFS → QBUF (with userptr) → DQBUF
```

#### **DMABUF (DMA Buffer Sharing)**

**Đặc điểm:**
- Zero-copy buffer sharing giữa devices
- Dùng file descriptor để share buffer
- Device A export → Device B import

**Khi nào dùng:**
- ✅ Camera → Display (V4L2 → DRM)
- ✅ Camera → Encoder (V4L2 → V4L2 mem2mem)
- ✅ Zero-copy pipeline

**Flow:**
```
Device A: REQBUFS → EXPBUF (export as fd)
Device B: QBUF (with fd from Device A)
```

### 9.3.3. Requesting Buffers

#### **MMAP Buffers**

```c
#define BUF_COUNT 4
#define BUF_COUNT_MIN 3

struct v4l2_requestbuffers req;
CLEAR(req);

req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_MMAP;

if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    if (errno == EINVAL) {
        fprintf(stderr, "Device does not support MMAP\n");
    }
    return -1;
}

// Check granted buffer count
if (req.count < BUF_COUNT_MIN) {
    fprintf(stderr, "Insufficient buffer memory: %d\n", req.count);
    return -1;
}

printf("Allocated %d buffers\n", req.count);
```

**Lưu ý:**
- Driver có thể grant ít hoặc nhiều hơn requested
- Phải check `req.count` sau ioctl

#### **USERPTR Buffers**

```c
struct v4l2_requestbuffers req;
CLEAR(req);

req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_USERPTR;

if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    if (errno == EINVAL) {
        fprintf(stderr, "Device does not support USERPTR\n");
    }
    return -1;
}

// Allocate buffers in user space
struct {
    void *start;
    size_t length;
} *buffers;

buffers = calloc(req.count, sizeof(*buffers));

for (i = 0; i < req.count; i++) {
    buffers[i].length = buffer_size;  // From format
    buffers[i].start = malloc(buffer_size);
    
    if (!buffers[i].start) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }
}
```

#### **DMABUF Buffers**

```c
struct v4l2_requestbuffers req;
CLEAR(req);

req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_DMABUF;

if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    perror("VIDIOC_REQBUFS DMABUF");
    return -1;
}

// Export buffers as DMABUF file descriptors (nếu capture device)
int dmabuf_fds[BUF_COUNT];

for (i = 0; i < req.count; i++) {
    struct v4l2_exportbuffer expbuf;
    CLEAR(expbuf);
    
    expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    expbuf.index = i;
    
    if (xioctl(fd, VIDIOC_EXPBUF, &expbuf) == -1) {
        perror("VIDIOC_EXPBUF");
        return -1;
    }
    
    dmabuf_fds[i] = expbuf.fd;
    printf("Buffer %d exported as fd %d\n", i, expbuf.fd);
}
```

### 9.3.4. Query and Map Buffers (MMAP only)

```c
struct {
    void *start;
    size_t length;
} *buffers;

buffers = calloc(req.count, sizeof(*buffers));

for (i = 0; i < req.count; i++) {
    struct v4l2_buffer buf;
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    
    // Query buffer physical address
    if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("VIDIOC_QUERYBUF");
        return -1;
    }
    
    // Map buffer to user space
    buffers[i].length = buf.length;
    buffers[i].start = mmap(NULL,                // Start anywhere
                            buf.length,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            fd,
                            buf.m.offset);      // Physical offset
    
    if (buffers[i].start == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    
    printf("Buffer %d: mapped at %p, size %zu\n",
           i, buffers[i].start, buffers[i].length);
}
```

**Giải thích:**
1. `VIDIOC_QUERYBUF` → Get buffer's physical offset (`buf.m.offset`)
2. `mmap()` → Map buffer vào user space virtual memory
3. Lưu lại `start` address và `length` để dùng sau

### 9.3.5. Buffer State và Flags

```c
// Buffer flags (v4l2_buffer.flags)
V4L2_BUF_FLAG_MAPPED      // Buffer is mapped
V4L2_BUF_FLAG_QUEUED      // Buffer is in INPUT queue
V4L2_BUF_FLAG_DONE        // Buffer is in OUTPUT queue (filled)
V4L2_BUF_FLAG_ERROR       // Buffer có lỗi khi capture
V4L2_BUF_FLAG_KEYFRAME    // Buffer chứa keyframe (video encoding)
V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC // Timestamp type
```

---

## Tổng Kết Part 1

**Đã học:**
✅ V4L2 User Space API overview - ioctl commands và data structures  
✅ Device opening, capabilities query  
✅ Format negotiation (resolution, pixel format, frame rate)  
✅ Controls management (brightness, contrast...)  
✅ Buffer management concepts - INPUT/OUTPUT queues  
✅ 3 buffer types so sánh: MMAP vs USERPTR vs DMABUF  
✅ Buffer allocation và mapping  

**Part 2 sẽ học:**
- Complete capture application (từng bước chi tiết)
- Buffer enqueueing/dequeueing
- Streaming control
- v4l2-ctl tools
- Advanced topics (SELECT/POLL, multi-planar, performance)

---

## Key Takeaways

1. **ioctl sequence PHẢI đúng thứ tự**: `QUERYCAP → S_FMT → REQBUFS → QUERYBUF → QBUF → STREAMON → DQBUF loop`

2. **Driver có thể adjust parameters**: Luôn check giá trị sau `VIDIOC_S_FMT` và `VIDIOC_REQBUFS`

3. **3 buffer types**:
   - **MMAP**: Phổ biến nhất, kernel manages memory
   - **USERPTR**: User control allocation
   - **DMABUF**: Zero-copy device-to-device

4. **Buffer queue flow**: User `QBUF` → Driver fills → Move to OUTPUT queue → User `DQBUF` → Process → `QBUF` again

5. **Always use xioctl wrapper**: Retry khi `errno == EINTR`

---

**Tiếp tục Part 2 để xem complete capture application và tools!** 🚀