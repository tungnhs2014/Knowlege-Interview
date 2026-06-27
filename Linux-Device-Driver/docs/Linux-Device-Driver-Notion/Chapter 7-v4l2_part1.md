# CHAPTER 7: V4L2 AND VIDEO CAPTURE DEVICE DRIVERS

## PART 1: KIẾN TRÚC VÀ CẤU TRÚC DỮ LIỆU CỐT LÕI

---

## 7.1. Giới Thiệu V4L2 Framework

### 7.1.1. V4L2 là gì?

**V4L2 (Video for Linux version 2)** là subsystem trong Linux kernel để quản lý thiết bị video như camera, capture card, encoder/decoder.

**Lịch sử phát triển:**
- **V4L (v1)**: Framework cũ, thiết kế đơn giản cho desktop
- **V4L2**: Ra đời để giải quyết nhu cầu embedded systems với memory management tốt hơn, hỗ trợ DMA và kiến trúc modular

### 7.1.2. Vấn Đề V4L2 Giải Quyết

**Trước khi có V4L2:**
- Mỗi driver tự implement buffer management → code trùng lặp
- Không có chuẩn chung cho DMA operations
- Khó tích hợp hardware phức tạp (nhiều IP blocks)

**V4L2 mang lại:**
- **videobuf2**: Framework quản lý buffer/DMA thống nhất
- **Sub-device model**: Tách biệt các thành phần hardware (sensor, ISP, scaler...)
- **Control framework**: Chuẩn hóa properties (brightness, contrast...)
- **Media Controller**: Routing cho video pipeline phức tạp

### 7.1.3. Kiến Trúc Tổng Thể

```
USER SPACE
    │
    ├─ Application (GStreamer, FFmpeg, custom...)
    │
    ├─ /dev/video0, /dev/video1... (device nodes)
    │
    └─ V4L2 API (VIDIOC_* ioctls)
         │
         ↓
─────────────────────────────────────────────────
KERNEL SPACE
         │
    ┌────┴────┐
    │ V4L2    │ ← Device registration
    │ Core    │ ← IOCTL dispatching  
    │         │ ← videobuf2 framework
    └────┬────┘
         │
    ┌────┴──────────┐
    │               │
┌───▼────┐    ┌────▼────────┐
│Bridge  │◄──►│ Sub-Device  │
│Driver  │    │ Driver      │
│(DMA)   │    │ (Sensor)    │
└───┬────┘    └────┬────────┘
    │              │
    │ DMA          │ I2C/SPI
    ↓              ↓
─────────────────────────────────────────────────
HARDWARE
    │              │
┌───▼──────┐  ┌───▼─────────┐
│DMA Engine│  │Camera Sensor│
└──────────┘  └─────────────┘
```

**Luồng dữ liệu:**
1. Sensor capture → DMA transfer → Kernel buffers (vb2 quản lý)
2. Kernel buffers → User space (qua mmap/read)

---

## 7.2. Bốn Cấu Trúc Dữ Liệu Cốt Lõi

> **QUAN TRỌNG:** Hiểu 4 struct này = nắm 80% V4L2

### 7.2.1. struct v4l2_device - Root Container

**Vai trò:**
- Container **cao nhất** trong hierarchy
- Chứa danh sách tất cả sub-devices
- Parent của bridge device

**Các trường quan trọng:**

```c
struct v4l2_device {
    struct device *dev;            // Parent device (platform/USB/PCI)
    struct list_head subdevs;      // Danh sách sub-devices
    char name[32];                 // Tên device
    struct v4l2_ctrl_handler *ctrl_handler; // Controls
};
```

**Khi nào dùng:**
- **Luôn luôn** - đây là root object bắt buộc
- Khởi tạo trong `probe()` của platform/USB/PCI driver

**API cơ bản:**

```c
// Đăng ký
int v4l2_device_register(struct device *dev, 
                         struct v4l2_device *v4l2_dev);

// Hủy đăng ký
void v4l2_device_unregister(struct v4l2_device *v4l2_dev);
```

**Ví dụ tối thiểu:**

```c
struct my_device {
    struct v4l2_device v4l2_dev;
    // ... driver private data
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    int ret;
    
    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    
    // Đăng ký V4L2 device
    ret = v4l2_device_register(&pdev->dev, &mydev->v4l2_dev);
    if (ret)
        return ret;
    
    // Sau khi register:
    // - mydev->v4l2_dev.name tự động set
    // - pdev->dev.driver_data = &mydev->v4l2_dev
    
    return 0;
}
```

---

### 7.2.2. struct video_device - Bridge Device Node

**Vai trò:**
- Đại diện `/dev/videoX` trong user space
- **Bridge driver** làm việc trực tiếp với struct này
- Xử lý file operations và IOCTLs

**Các trường quan trọng:**

```c
struct video_device {
    // Pointers to operations
    const struct v4l2_file_operations *fops;
    const struct v4l2_ioctl_ops *ioctl_ops;
    
    // Liên kết
    struct v4l2_device *v4l2_dev;  // Parent V4L2 device
    struct vb2_queue *queue;       // Buffer queue
    
    // Metadata
    char name[32];                 // Device name
    enum vfl_devnode_type vfl_type; // VIDEO/RADIO/VBI...
    enum vfl_devnode_direction vfl_dir; // RX/TX/M2M
    
    void (*release)(struct video_device *vdev);
};
```

**Device types (vfl_type):**
- `VFL_TYPE_GRABBER`: Video capture/output → `/dev/videoX`
- `VFL_TYPE_RADIO`: Radio → `/dev/radioX`
- `VFL_TYPE_SUBDEV`: Sub-device → `/dev/v4l-subdevX`

**Direction (vfl_dir):**
- `VFL_DIR_RX`: Capture (camera)
- `VFL_DIR_TX`: Output (display)
- `VFL_DIR_M2M`: Memory-to-memory (encoder/decoder)

**API cơ bản:**

```c
// Cấp phát (nếu standalone)
struct video_device *video_device_alloc(void);

// Đăng ký → tạo /dev/videoX
int video_register_device(struct video_device *vdev,
                          enum vfl_devnode_type type,
                          int nr); // -1 = auto-assign

// Hủy đăng ký
void video_unregister_device(struct video_device *vdev);
```

**Ví dụ khởi tạo:**

```c
static int init_video_device(struct my_device *mydev)
{
    struct video_device *vdev;
    
    vdev = video_device_alloc();
    if (!vdev)
        return -ENOMEM;
    
    // Setup
    strscpy(vdev->name, "My Camera", sizeof(vdev->name));
    vdev->v4l2_dev = &mydev->v4l2_dev;  // Link to parent
    vdev->fops = &my_fops;              // File operations
    vdev->ioctl_ops = &my_ioctl_ops;    // IOCTL handlers
    vdev->release = video_device_release;
    vdev->vfl_type = VFL_TYPE_GRABBER;
    vdev->vfl_dir = VFL_DIR_RX;         // Capture
    vdev->queue = &mydev->queue;        // Link to buffer queue
    
    // Đăng ký → tạo /dev/video0 (hoặc số khác)
    ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);
    if (ret) {
        video_device_release(vdev);
        return ret;
    }
    
    return 0;
}
```

---

### 7.2.3. struct vb2_queue - Buffer Queue Manager

**Vai trò:**
- **CORE của streaming** - quản lý buffers và DMA
- Interface giữa driver và videobuf2 framework
- Thay driver implement buffer allocation/queuing

**Các trường quan trọng:**

```c
struct vb2_queue {
    enum v4l2_buf_type type;       // CAPTURE/OUTPUT
    unsigned int io_modes;         // MMAP/USERPTR/DMABUF
    
    const struct vb2_ops *ops;     // Driver callbacks
    const struct vb2_mem_ops *mem_ops; // Memory allocator
    
    void *drv_priv;                // Driver private data
    struct device *dev;            // Device for DMA
    
    unsigned int min_buffers_needed; // Min buffers trước khi stream
};
```

**Buffer types quan trọng:**
- `V4L2_BUF_TYPE_VIDEO_CAPTURE`: Video capture (single-plane)
- `V4L2_BUF_TYPE_VIDEO_OUTPUT`: Video output
- `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`: Multi-plane capture (YUV420...)

**I/O modes:**
- `VB2_MMAP`: Kernel cấp phát, user mmap → **phổ biến nhất**
- `VB2_USERPTR`: User cấp phát (malloc), pass pointer
- `VB2_DMABUF`: Zero-copy giữa devices

**Memory allocators:**
- `vb2_dma_contig_memops`: Physically contiguous (cho hardware DMA)
- `vb2_vmalloc_memops`: Virtual memory (USB devices)
- `vb2_dma_sg_memops`: Scatter-gather DMA

**Ví dụ khởi tạo:**

```c
static int init_vb2_queue(struct my_device *mydev)
{
    struct vb2_queue *q = &mydev->queue;
    
    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_DMABUF;
    q->ops = &my_vb2_ops;           // Driver callbacks
    q->mem_ops = &vb2_dma_contig_memops; // DMA memory
    q->drv_priv = mydev;
    q->buf_struct_size = sizeof(struct my_buffer);
    q->min_buffers_needed = 2;      // Tối thiểu 2 buffers
    q->dev = mydev->dev;            // For DMA mapping
    
    return vb2_queue_init(q);
}
```

---

### 7.2.4. struct v4l2_subdev - Sub-Device Abstraction

**Vai trò:**
- Abstract các IP blocks riêng lẻ: sensor, ISP, scaler...
- Cho phép modularity trong hardware phức tạp
- **KHÔNG làm DMA** - chỉ control qua I2C/SPI

**Các trường quan trọng:**

```c
struct v4l2_subdev {
    struct list_head list;         // List trong v4l2_device
    struct v4l2_device *v4l2_dev;  // Parent
    
    const struct v4l2_subdev_ops *ops; // Operations
    struct v4l2_ctrl_handler *ctrl_handler;
    
    char name[32];                 // Tên sub-device
    u32 flags;                     // IS_I2C, HAS_DEVNODE...
    
    struct device *dev;            // Physical device (I2C/SPI)
    struct fwnode_handle *fwnode;  // Device tree node
};
```

**Flags quan trọng:**
- `V4L2_SUBDEV_FL_IS_I2C`: Sub-device là I2C device
- `V4L2_SUBDEV_FL_HAS_DEVNODE`: Tạo `/dev/v4l-subdevX`

**Khi nào cần sub-device:**
- Camera sensor (OV7740, IMX219...)
- Image Signal Processor (ISP)
- External encoder/decoder

**Khi nào KHÔNG cần:**
- USB webcam (driver all-in-one)
- Simple capture card

---

### 7.2.5. Mối Quan Hệ Giữa 4 Struct

```
┌─────────────────────────┐
│   v4l2_device           │ ← ROOT container
│   - List of subdevs     │
└──────────┬──────────────┘
           │
    ┌──────┴────────┐
    │               │
    ▼               ▼
┌─────────────┐  ┌──────────────┐
│video_device │  │ v4l2_subdev  │
│(Bridge)     │  │ (Sensor)     │
│- fops       │  │- I2C control │
│- ioctl_ops  │  │- ops         │
│- queue ────►│  └──────────────┘
└─────────────┘
    │
    │ points to
    ▼
┌──────────────┐
│ vb2_queue    │ ← Buffer manager
│- DMA ops     │
│- Memory      │
└──────────────┘
```

**Vai trò từng struct:**
1. **v4l2_device**: Root, quản lý tất cả
2. **video_device**: Bridge điều khiển DMA → `/dev/videoX`
3. **vb2_queue**: Quản lý buffers và DMA operations
4. **v4l2_subdev**: Sensor/ISP (I2C control, không DMA)

**So sánh Bridge vs Sub-device:**

| Aspect | Bridge Driver | Sub-Device Driver |
|--------|---------------|-------------------|
| DMA | ✅ Có (điều khiển DMA engine) | ❌ Không |
| Device node | `/dev/videoX` | `/dev/v4l-subdevX` (optional) |
| File ops | ✅ Cần implement | ❌ Không cần |
| Control | Format, streaming | Sensor registers, I2C |
| Ví dụ | i.MX CSI, USB UVC host | OV7740 sensor, ISP |

---

## 7.3. Bridge Driver - File Operations

### 7.3.1. struct v4l2_file_operations

**Mục đích:** Xử lý các system calls từ user space

**Struct definition:**

```c
struct v4l2_file_operations {
    struct module *owner;
    int (*open)(struct file *);
    int (*release)(struct file *);
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    __poll_t (*poll)(struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
    int (*mmap)(struct file *, struct vm_area_struct *);
};
```

**videobuf2 cung cấp helpers:**
- `vb2_fop_poll()`: Poll for buffer ready
- `vb2_fop_mmap()`: Map buffer to user space
- `vb2_fop_read()`: Read interface (optional)

**Implementation tiêu biểu:**

```c
static int my_open(struct file *file)
{
    struct my_device *mydev = video_drvdata(file);
    
    // V4L2 file handle init
    return v4l2_fh_open(file);
}

static int my_release(struct file *file)
{
    return _vb2_fop_release(file, NULL);
}

static const struct v4l2_file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .unlocked_ioctl = video_ioctl2,  // V4L2 core dispatcher
    .poll = vb2_fop_poll,
    .mmap = vb2_fop_mmap,
};
```

**Lưu ý:**
- `video_ioctl2`: V4L2 core sẽ dispatch tới `v4l2_ioctl_ops`
- Hầu hết callbacks dùng vb2 helpers → driver code ngắn gọn

---

## 7.4. Bridge Driver - IOCTL Operations

### 7.4.1. struct v4l2_ioctl_ops

**Mục đích:** Implement các V4L2 IOCTL commands

**Các IOCTL quan trọng nhất:**

| IOCTL | Callback | Mục đích |
|-------|----------|----------|
| `VIDIOC_QUERYCAP` | `vidioc_querycap` | Query device capabilities |
| `VIDIOC_ENUM_FMT` | `vidioc_enum_fmt_vid_cap` | Enumerate pixel formats |
| `VIDIOC_G_FMT` | `vidioc_g_fmt_vid_cap` | Get current format |
| `VIDIOC_S_FMT` | `vidioc_s_fmt_vid_cap` | Set format |
| `VIDIOC_REQBUFS` | `vidioc_reqbufs` | Request buffers |
| `VIDIOC_STREAMON` | `vidioc_streamon` | Start streaming |

**videobuf2 implement sẵn buffer ops:**
- `vb2_ioctl_reqbufs`
- `vb2_ioctl_qbuf`
- `vb2_ioctl_dqbuf`
- `vb2_ioctl_streamon`
- `vb2_ioctl_streamoff`

**Ví dụ implementation:**

```c
static int my_querycap(struct file *file, void *priv,
                       struct v4l2_capability *cap)
{
    strscpy(cap->driver, "my-camera", sizeof(cap->driver));
    strscpy(cap->card, "My Camera Device", sizeof(cap->card));
    
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | 
                       V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    
    return 0;
}

static int my_enum_fmt(struct file *file, void *priv,
                       struct v4l2_fmtdesc *f)
{
    if (f->index >= 1)  // Chỉ support 1 format
        return -EINVAL;
    
    f->pixelformat = V4L2_PIX_FMT_YUYV;
    return 0;
}

static const struct v4l2_ioctl_ops my_ioctl_ops = {
    .vidioc_querycap = my_querycap,
    .vidioc_enum_fmt_vid_cap = my_enum_fmt,
    
    // videobuf2 helpers
    .vidioc_reqbufs = vb2_ioctl_reqbufs,
    .vidioc_qbuf = vb2_ioctl_qbuf,
    .vidioc_dqbuf = vb2_ioctl_dqbuf,
    .vidioc_streamon = vb2_ioctl_streamon,
    .vidioc_streamoff = vb2_ioctl_streamoff,
};
```

---

## Tổng Kết Part 1

**Điểm chính cần nhớ:**

1. **4 struct cốt lõi:**
   - `v4l2_device`: Root container
   - `video_device`: Bridge → `/dev/videoX`
   - `vb2_queue`: Buffer manager (core của streaming)
   - `v4l2_subdev`: Sensor/ISP abstraction

2. **Bridge driver role:**
   - Điều khiển DMA engine
   - Implement file ops và ioctl ops
   - Sử dụng videobuf2 helpers để giảm code

3. **videobuf2 framework:**
   - Cung cấp buffer management
   - Giảm 70-80% code driver phải viết
   - Hỗ trợ nhiều memory models (MMAP, USERPTR, DMABUF)

**Next:** Part 2 sẽ đi sâu vào videobuf2 operations, sub-device drivers, và V4L2 controls.
