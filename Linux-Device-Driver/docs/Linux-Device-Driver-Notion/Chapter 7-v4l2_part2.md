# CHAPTER 7: V4L2 AND VIDEO CAPTURE DEVICE DRIVERS

## PART 2: VIDEOBUF2 FRAMEWORK VÀ SUB-DEVICE DRIVERS

---

## 7.5. videobuf2 (vb2) Framework - Chi Tiết

### 7.5.1. videobuf2 là gì?

**videobuf2** (viết tắt vb2) là framework quản lý buffers cho streaming video:
- Thay driver implement buffer allocation, queueing, DMA
- Cung cấp API thống nhất cho tất cả V4L2 drivers
- Giảm code driver phải viết ~70-80%

**Ưu điểm so với videobuf v1:**
- Memory backend modular (dễ extend)
- Support nhiều memory types (contiguous, scatter-gather, vmalloc)
- Thread-safe
- Integration tốt hơn với DMA

### 7.5.2. Buffer Concept và Lifecycle

**struct vb2_buffer** - đại diện 1 buffer:

```c
struct vb2_buffer {
    struct vb2_queue *vb2_queue;  // Queue chứa buffer này
    unsigned int index;           // Buffer ID (0, 1, 2...)
    unsigned int type;            // CAPTURE/OUTPUT
    unsigned int memory;          // MMAP/USERPTR/DMABUF
    
    enum vb2_buffer_state state;  // Buffer state
    struct list_head queued_entry; // Queue list
    struct list_head done_entry;   // Done list
};
```

**Buffer states:**

```
DEQUEUED ─────► QUEUED ─────► ACTIVE ─────► DONE
   ▲               │              │            │
   │               │              │            │
   └───────────────┴──────────────┴────────────┘
            VIDIOC_DQBUF (user dequeues)
```

| State | Ý nghĩa |
|-------|---------|
| `DEQUEUED` | Buffer ở user space, chưa queue |
| `QUEUED` | Buffer đã queue, đợi DMA |
| `ACTIVE` | DMA đang transfer vào buffer |
| `DONE` | DMA xong, sẵn sàng dequeue |
| `ERROR` | DMA lỗi |

**Flow từ user space:**

1. `VIDIOC_REQBUFS`: Allocate buffers → state = DEQUEUED
2. `VIDIOC_QBUF`: Queue buffer → state = QUEUED
3. Driver DMA bắt đầu → state = ACTIVE
4. DMA xong → state = DONE
5. `VIDIOC_DQBUF`: User lấy buffer → state = DEQUEUED
6. Lặp lại từ bước 2

---

### 7.5.3. struct vb2_ops - Driver Callbacks

**Mục đích:** Driver implement các callbacks để videobuf2 gọi

**Struct definition:**

```c
struct vb2_ops {
    int (*queue_setup)(struct vb2_queue *q,
                       unsigned int *num_buffers,
                       unsigned int *num_planes,
                       unsigned int sizes[],
                       struct device *alloc_devs[]);
    
    int (*buf_prepare)(struct vb2_buffer *vb);
    void (*buf_queue)(struct vb2_buffer *vb);
    void (*buf_finish)(struct vb2_buffer *vb);
    
    int (*start_streaming)(struct vb2_queue *q, unsigned int count);
    void (*stop_streaming)(struct vb2_queue *q);
};
```

**Chi tiết từng callback:**

#### 7.5.3.1. queue_setup - Negotiation

**Khi nào gọi:** `VIDIOC_REQBUFS`, `VIDIOC_CREATE_BUFS`

**Mục đích:** Negotiate số buffers và kích thước

```c
static int my_queue_setup(struct vb2_queue *vq,
                          unsigned int *nbuffers,
                          unsigned int *nplanes,
                          unsigned int sizes[],
                          struct device *alloc_devs[])
{
    struct my_device *mydev = vb2_get_drv_priv(vq);
    
    // Tính kích thước buffer dựa trên format hiện tại
    unsigned int size = mydev->width * mydev->height * 2; // YUYV
    
    *nplanes = 1;        // Single-plane format
    sizes[0] = size;     // Kích thước plane 0
    
    // Tối thiểu 3 buffers để DMA pipeline hiệu quả
    if (*nbuffers < 3)
        *nbuffers = 3;
    
    return 0;
}
```

**Giải thích:**
- `*nbuffers`: User request bao nhiêu buffers, driver có thể adjust
- `*nplanes`: Số planes (1 cho packed format, 3 cho YUV420...)
- `sizes[]`: Kích thước mỗi plane

#### 7.5.3.2. buf_prepare - Chuẩn bị buffer

**Khi nào gọi:** Trước khi buffer được queue vào DMA

**Mục đích:** Validate buffer, setup DMA mapping

```c
static int my_buf_prepare(struct vb2_buffer *vb)
{
    struct my_device *mydev = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct my_buffer *buf = container_of(vbuf, struct my_buffer, vb);
    
    // Validate buffer size
    unsigned long size = mydev->width * mydev->height * 2;
    if (vb2_plane_size(vb, 0) < size) {
        dev_err(mydev->dev, "Buffer too small\n");
        return -EINVAL;
    }
    
    vb2_set_plane_payload(vb, 0, size);
    
    // Get DMA address
    buf->dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
    
    return 0;
}
```

#### 7.5.3.3. buf_queue - Queue buffer

**Khi nào gọi:** `VIDIOC_QBUF` từ user space

**Mục đích:** Add buffer vào driver's queue, start DMA nếu cần

```c
static void my_buf_queue(struct vb2_buffer *vb)
{
    struct my_device *mydev = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct my_buffer *buf = container_of(vbuf, struct my_buffer, vb);
    unsigned long flags;
    
    // Add buffer to driver's internal list
    spin_lock_irqsave(&mydev->lock, flags);
    list_add_tail(&buf->list, &mydev->buf_list);
    spin_unlock_irqrestore(&mydev->lock, flags);
    
    // Nếu streaming đã start và đây là buffer đầu tiên
    // có thể kick DMA ngay (hoặc để start_streaming làm)
}
```

**Lưu ý:** Driver thường maintain list riêng để track buffers queued

#### 7.5.3.4. start_streaming - Bắt đầu streaming

**Khi nào gọi:** `VIDIOC_STREAMON`

**Mục đích:** Start DMA engine, enable interrupts

```c
static int my_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    struct my_device *mydev = vb2_get_drv_priv(vq);
    struct my_buffer *buf;
    int ret;
    
    // Check minimum buffers
    if (count < vq->min_buffers_needed) {
        dev_err(mydev->dev, "Need at least %d buffers\n",
                vq->min_buffers_needed);
        return -ENOBUFS;
    }
    
    // Start sensor (sub-device)
    ret = v4l2_subdev_call(mydev->sensor, video, s_stream, 1);
    if (ret) {
        dev_err(mydev->dev, "Failed to start sensor\n");
        goto err_return_buffers;
    }
    
    // Get first buffer from list
    buf = list_first_entry(&mydev->buf_list, struct my_buffer, list);
    
    // Configure DMA with buffer address
    writel(buf->dma_addr, mydev->regs + DMA_ADDR_REG);
    writel(mydev->width * mydev->height * 2, mydev->regs + DMA_SIZE_REG);
    
    // Enable DMA interrupts
    writel(DMA_IRQ_ENABLE, mydev->regs + DMA_IRQ_REG);
    
    // Start DMA
    writel(DMA_START, mydev->regs + DMA_CTRL_REG);
    
    return 0;

err_return_buffers:
    // Nếu fail, return tất cả buffers về state ERROR
    while (!list_empty(&mydev->buf_list)) {
        buf = list_first_entry(&mydev->buf_list, struct my_buffer, list);
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }
    return ret;
}
```

**Lưu ý quan trọng:**
- Phải check `count >= min_buffers_needed`
- Nếu fail, phải return buffers về state ERROR
- Start sensor trước khi start DMA

#### 7.5.3.5. stop_streaming - Dừng streaming

**Khi nào gọi:** `VIDIOC_STREAMOFF`

**Mục đích:** Stop DMA, disable interrupts, return buffers

```c
static void my_stop_streaming(struct vb2_queue *vq)
{
    struct my_device *mydev = vb2_get_drv_priv(vq);
    struct my_buffer *buf, *node;
    unsigned long flags;
    
    // Stop DMA engine
    writel(DMA_STOP, mydev->regs + DMA_CTRL_REG);
    
    // Disable interrupts
    writel(0, mydev->regs + DMA_IRQ_REG);
    
    // Stop sensor
    v4l2_subdev_call(mydev->sensor, video, s_stream, 0);
    
    // Return all buffers to vb2 in ERROR state
    spin_lock_irqsave(&mydev->lock, flags);
    list_for_each_entry_safe(buf, node, &mydev->buf_list, list) {
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }
    spin_unlock_irqrestore(&mydev->lock, flags);
}
```

**Lưu ý:** Phải return **TẤT CẢ** buffers về vb2, nếu không sẽ memory leak

---

### 7.5.4. DMA Completion Handling (Interrupt)

**Mục đích:** Khi DMA xong 1 buffer, move sang buffer tiếp theo

```c
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    struct my_device *mydev = dev_id;
    struct my_buffer *buf, *next_buf;
    u32 status;
    
    status = readl(mydev->regs + DMA_STATUS_REG);
    if (!(status & DMA_IRQ_FLAG))
        return IRQ_NONE;
    
    // Clear interrupt
    writel(DMA_IRQ_FLAG, mydev->regs + DMA_STATUS_REG);
    
    spin_lock(&mydev->lock);
    
    // Get completed buffer
    buf = list_first_entry(&mydev->buf_list, struct my_buffer, list);
    list_del(&buf->list);
    
    // Fill timestamp và sequence
    buf->vb.vb2_buf.timestamp = ktime_get_ns();
    buf->vb.sequence = mydev->sequence++;
    
    // Return buffer to vb2 (user có thể DQBUF ngay)
    vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
    
    // Nếu còn buffers trong queue, start DMA cho buffer tiếp
    if (!list_empty(&mydev->buf_list)) {
        next_buf = list_first_entry(&mydev->buf_list, 
                                     struct my_buffer, list);
        writel(next_buf->dma_addr, mydev->regs + DMA_ADDR_REG);
        writel(DMA_START, mydev->regs + DMA_CTRL_REG);
    }
    
    spin_unlock(&mydev->lock);
    
    return IRQ_HANDLED;
}
```

**Flow hoàn chỉnh:**

```
User QBUF → buf_queue() → Add to list
                              ↓
                     start_streaming() → Start DMA
                              ↓
                         IRQ Handler
                              ↓
                    vb2_buffer_done() → Move to DONE list
                              ↓
User DQBUF ← Return buffer ← Wake up user
    ↓
User QBUF lại → Cycle tiếp tục
```

---

### 7.5.5. Custom Buffer Structure

**Vấn đề:** `vb2_buffer` không chứa driver-specific data

**Giải pháp:** Embed vào custom struct

```c
struct my_buffer {
    struct vb2_v4l2_buffer vb;  // PHẢI là field đầu tiên
    struct list_head list;       // For driver's queue
    dma_addr_t dma_addr;         // Physical address
    // Driver-specific fields...
};

// Convert vb2_buffer → my_buffer
static inline struct my_buffer *to_my_buffer(struct vb2_buffer *vb)
{
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    return container_of(vbuf, struct my_buffer, vb);
}
```

**Setup trong queue init:**

```c
q->buf_struct_size = sizeof(struct my_buffer);
```

videobuf2 sẽ tự động allocate `my_buffer` thay vì `vb2_buffer`

---

## 7.6. Sub-Device Drivers

### 7.6.1. Sub-Device Concept

**Sub-device** là abstraction của các hardware components riêng lẻ:
- **Camera sensor** (OV7740, IMX219...)
- **Image Signal Processor** (ISP)
- **Video decoder/encoder**
- **Scaler, rotator**

**Đặc điểm:**
- Kết nối qua I2C/SPI/Platform bus
- **KHÔNG làm DMA** - chỉ control registers
- Có thể tạo `/dev/v4l-subdevX` (optional)
- Bridge driver gọi sub-device qua `v4l2_subdev_call()`

**Khi nào cần:**
- Hardware có nhiều IP blocks độc lập
- Muốn reuse sensor driver cho nhiều platforms
- Complex video pipeline (cần Media Controller)

**Khi nào KHÔNG cần:**
- USB webcam (all-in-one driver)
- Simple capture card
- Chỉ có 1 IP block

---

### 7.6.2. struct v4l2_subdev_ops

**Mục đích:** Define operations mà bridge có thể gọi

```c
struct v4l2_subdev_ops {
    const struct v4l2_subdev_core_ops *core;
    const struct v4l2_subdev_video_ops *video;
    const struct v4l2_subdev_pad_ops *pad;
    const struct v4l2_subdev_sensor_ops *sensor;
};
```

#### 7.6.2.1. Core Operations

```c
struct v4l2_subdev_core_ops {
    int (*s_power)(struct v4l2_subdev *sd, int on);
    int (*init)(struct v4l2_subdev *sd, u32 val);
    // ... other ops
};
```

**Ví dụ implementation:**

```c
static int sensor_s_power(struct v4l2_subdev *sd, int on)
{
    struct sensor_device *sensor = to_sensor(sd);
    
    if (on) {
        // Power on sensor
        regulator_enable(sensor->vdd);
        clk_prepare_enable(sensor->clk);
        usleep_range(5000, 10000);  // Wait stabilize
        
        // Load default registers
        sensor_write_array(sensor, sensor_default_regs);
    } else {
        // Power off
        clk_disable_unprepare(sensor->clk);
        regulator_disable(sensor->vdd);
    }
    
    return 0;
}
```

#### 7.6.2.2. Video Operations

```c
struct v4l2_subdev_video_ops {
    int (*s_stream)(struct v4l2_subdev *sd, int enable);
    int (*g_frame_interval)(struct v4l2_subdev *sd, ...);
    int (*s_frame_interval)(struct v4l2_subdev *sd, ...);
};
```

**s_stream - Quan trọng nhất:**

```c
static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct sensor_device *sensor = to_sensor(sd);
    
    if (enable) {
        // Start streaming
        sensor_write_reg(sensor, 0x3008, 0x00);  // Exit standby
        sensor->streaming = true;
    } else {
        // Stop streaming
        sensor_write_reg(sensor, 0x3008, 0x42);  // Enter standby
        sensor->streaming = false;
    }
    
    return 0;
}
```

**Được gọi từ bridge:**

```c
// In bridge's start_streaming()
ret = v4l2_subdev_call(mydev->sensor, video, s_stream, 1);
```

#### 7.6.2.3. Pad Operations (Media Controller)

**Dùng khi:** Complex pipeline, cần Media Controller

```c
struct v4l2_subdev_pad_ops {
    int (*enum_mbus_code)(struct v4l2_subdev *sd, ...);
    int (*get_fmt)(struct v4l2_subdev *sd, ...);
    int (*set_fmt)(struct v4l2_subdev *sd, ...);
};
```

**Ví dụ:**

```c
static int sensor_get_fmt(struct v4l2_subdev *sd,
                          struct v4l2_subdev_state *state,
                          struct v4l2_subdev_format *fmt)
{
    struct sensor_device *sensor = to_sensor(sd);
    
    fmt->format.width = sensor->width;
    fmt->format.height = sensor->height;
    fmt->format.code = MEDIA_BUS_FMT_YUYV8_2X8;
    fmt->format.field = V4L2_FIELD_NONE;
    
    return 0;
}
```

---

### 7.6.3. Sub-Device Initialization (I2C Example)

**Typical I2C sensor driver:**

```c
static int sensor_probe(struct i2c_client *client)
{
    struct sensor_device *sensor;
    struct v4l2_subdev *sd;
    int ret;
    
    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;
    
    sensor->client = client;
    
    // Get resources (regulators, clocks, GPIOs)
    sensor->vdd = devm_regulator_get(&client->dev, "vdd");
    sensor->clk = devm_clk_get(&client->dev, "xclk");
    sensor->reset_gpio = devm_gpiod_get(&client->dev, "reset", 
                                        GPIOD_OUT_HIGH);
    
    // Initialize sub-device
    sd = &sensor->sd;
    v4l2_i2c_subdev_init(sd, client, &sensor_subdev_ops);
    
    // Set sub-device flags
    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    
    // Initialize controls (brightness, contrast...)
    ret = sensor_init_controls(sensor);
    if (ret)
        return ret;
    
    // Power on and check chip ID
    ret = sensor_s_power(sd, 1);
    if (ret)
        goto err_controls;
    
    ret = sensor_check_chip_id(sensor);
    if (ret) {
        dev_err(&client->dev, "Failed to detect sensor\n");
        goto err_power;
    }
    
    sensor_s_power(sd, 0);
    
    // Register sub-device (async)
    ret = v4l2_async_register_subdev(sd);
    if (ret)
        goto err_controls;
    
    dev_info(&client->dev, "Sensor probed successfully\n");
    return 0;

err_power:
    sensor_s_power(sd, 0);
err_controls:
    v4l2_ctrl_handler_free(&sensor->ctrl_handler);
    return ret;
}
```

**Lưu ý:**
- Dùng `v4l2_async_register_subdev()` thay vì `v4l2_device_register_subdev()`
- Async registration giải quyết probe order issue (sẽ học ở Chapter 8)

---

### 7.6.4. Bridge Gọi Sub-Device

**Macro `v4l2_subdev_call()`:**

```c
// Syntax
v4l2_subdev_call(subdev, category, operation, args...);

// Ví dụ
v4l2_subdev_call(mydev->sensor, video, s_stream, 1);
v4l2_subdev_call(mydev->sensor, core, s_power, 1);
```

**Implementation trong bridge:**

```c
struct my_device {
    struct v4l2_device v4l2_dev;
    struct v4l2_subdev *sensor;  // Pointer to sensor sub-device
    // ...
};

// Trong start_streaming()
ret = v4l2_subdev_call(mydev->sensor, video, s_stream, 1);
if (ret && ret != -ENOIOCTLCMD) {
    dev_err(mydev->dev, "Failed to start sensor\n");
    return ret;
}
```

**Return values:**
- `0`: Success
- `-ENOIOCTLCMD`: Operation not implemented (có thể ignore)
- Other: Error

---

## 7.7. V4L2 Controls Infrastructure

### 7.7.1. Controls Concept

**Controls** = user-settable properties:
- Standard: `brightness`, `contrast`, `saturation`, `hue`...
- Camera-specific: `exposure`, `gain`, `white_balance`...
- Custom controls (driver-defined)

**Framework cung cấp:**
- Automatic value validation
- Default values
- Min/max/step checking
- Inheritance (sub-device → bridge)

---

### 7.7.2. Control Handler Initialization

**Bước 1: Initialize handler**

```c
#include <media/v4l2-ctrls.h>

struct sensor_device {
    struct v4l2_subdev sd;
    struct v4l2_ctrl_handler ctrl_handler;
    struct v4l2_ctrl *exposure;
    struct v4l2_ctrl *gain;
    // ...
};

static int sensor_init_controls(struct sensor_device *sensor)
{
    struct v4l2_ctrl_handler *hdl = &sensor->ctrl_handler;
    int ret;
    
    // Init handler với số controls dự kiến
    v4l2_ctrl_handler_init(hdl, 8);
    
    // Create controls
    v4l2_ctrl_new_std(hdl, &sensor_ctrl_ops,
                      V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
    
    v4l2_ctrl_new_std(hdl, &sensor_ctrl_ops,
                      V4L2_CID_CONTRAST, 0, 127, 1, 64);
    
    sensor->exposure = v4l2_ctrl_new_std(hdl, &sensor_ctrl_ops,
                                          V4L2_CID_EXPOSURE, 0, 1000, 1, 500);
    
    sensor->gain = v4l2_ctrl_new_std(hdl, &sensor_ctrl_ops,
                                      V4L2_CID_GAIN, 0, 1023, 1, 16);
    
    // Check for errors
    if (hdl->error) {
        ret = hdl->error;
        v4l2_ctrl_handler_free(hdl);
        return ret;
    }
    
    // Link handler to sub-device
    sensor->sd.ctrl_handler = hdl;
    
    return 0;
}
```

---

### 7.7.3. Control Operations

**struct v4l2_ctrl_ops:**

```c
struct v4l2_ctrl_ops {
    int (*g_volatile_ctrl)(struct v4l2_ctrl *ctrl);
    int (*s_ctrl)(struct v4l2_ctrl *ctrl);
};
```

**Implementation:**

```c
static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
    struct sensor_device *sensor = container_of(ctrl->handler,
                                                  struct sensor_device,
                                                  ctrl_handler);
    int ret = 0;
    
    switch (ctrl->id) {
    case V4L2_CID_BRIGHTNESS:
        ret = sensor_write_reg(sensor, BRIGHT_REG, ctrl->val);
        break;
        
    case V4L2_CID_CONTRAST:
        ret = sensor_write_reg(sensor, CONTRAST_REG, ctrl->val);
        break;
        
    case V4L2_CID_EXPOSURE:
        ret = sensor_set_exposure(sensor, ctrl->val);
        break;
        
    case V4L2_CID_GAIN:
        ret = sensor_set_gain(sensor, ctrl->val);
        break;
        
    default:
        return -EINVAL;
    }
    
    return ret;
}

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
    .s_ctrl = sensor_s_ctrl,
};
```

**Setup default values:**

```c
// Sau khi register sub-device, set all controls to default
ret = v4l2_ctrl_handler_setup(&sensor->ctrl_handler);
```

---

### 7.7.4. Control Inheritance

**Tự động:** Khi register sub-device, controls tự động merge vào bridge

```c
// In bridge driver:
v4l2_device_register_subdev(&mydev->v4l2_dev, sensor_sd);

// Nếu cả bridge và sensor có brightness control:
// → Bridge's control được ưu tiên
// → Sensor's control bị skip
```

**Private controls:**

```c
// Trong sensor driver, nếu không muốn expose control ra bridge:
ctrl->flags |= V4L2_CTRL_FLAG_PRIVATE;
```

---

## 7.8. [ADVANCED] Các Chủ Đề Nâng Cao

### 7.8.1. Memory-to-Memory (M2M) Devices

**Use case:** Encoder, decoder, image processor

**Đặc điểm:**
- Có cả INPUT và OUTPUT queues
- Không có DMA từ hardware → memory
- Process: Memory → Device → Memory

**Framework:** `v4l2-mem2mem.c`

```c
struct my_m2m_dev {
    struct v4l2_m2m_dev *m2m_dev;
    // ...
};

// Init
mydev->m2m_dev = v4l2_m2m_init(&m2m_ops);
```

---

### 7.8.2. Multi-Plane Buffers

**Khi nào cần:** Planar formats (YUV420, NV12...)

```c
// Queue setup
q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

// In queue_setup:
*nplanes = 3;  // Y, U, V planes
sizes[0] = width * height;      // Y plane
sizes[1] = width * height / 4;  // U plane
sizes[2] = width * height / 4;  // V plane
```

---

### 7.8.3. Debugging Tips

**Enable dynamic debug:**

```bash
# V4L2 core
echo 'file drivers/media/v4l2-core/* +p' > /sys/kernel/debug/dynamic_debug/control

# videobuf2
echo 'file drivers/media/common/videobuf2/* +p' > /sys/kernel/debug/dynamic_debug/control

# Specific driver
echo 'file drivers/media/platform/my-driver.c +p' > /sys/kernel/debug/dynamic_debug/control
```

**Common issues:**

| Issue | Cause | Fix |
|-------|-------|-----|
| `VIDIOC_STREAMON` fail | Min buffers not met | Queue more buffers |
| DMA timeout | Wrong DMA address | Check `buf_prepare()` |
| Image corruption | Buffer size mismatch | Verify `queue_setup()` |

---

## 7.9. Tổng Kết

**Key Takeaways:**

1. **4 struct cốt lõi:**
   - `v4l2_device`: Root container
   - `video_device`: Bridge → `/dev/videoX`
   - `vb2_queue`: Buffer manager (DMA core)
   - `v4l2_subdev`: Sensor/ISP abstraction

2. **videobuf2 flow:**
   ```
   queue_setup → buf_prepare → buf_queue
        ↓
   start_streaming → IRQ → vb2_buffer_done()
        ↓
   stop_streaming → return all buffers
   ```

3. **Sub-device:**
   - Control hardware qua I2C/SPI
   - KHÔNG làm DMA
   - Bridge gọi via `v4l2_subdev_call()`
   - Async registration giải quyết probe order

4. **Controls:**
   - Init handler → Create controls → Implement `s_ctrl`
   - Auto validation, default values
   - Inheritance: sub-device controls → bridge

5. **DMA interrupt handler:**
   - Get completed buffer từ list
   - `vb2_buffer_done()` return về user
   - Start DMA cho buffer tiếp theo

**So sánh Bridge vs Sub-Device:**

| Aspect | Bridge Driver | Sub-Device Driver |
|--------|---------------|-------------------|
| **DMA** | ✅ Có (điều khiển DMA engine) | ❌ Không |
| **Device node** | `/dev/videoX` | `/dev/v4l-subdevX` (optional) |
| **File ops** | ✅ Cần implement | ❌ Không cần |
| **IOCTL ops** | ✅ Cần implement | ❌ Không cần (có pad ops) |
| **Buffer queue** | ✅ Có `vb2_queue` | ❌ Không |
| **Control** | Format, streaming, DMA | Sensor registers (I2C/SPI) |
| **Hardware** | Platform/USB/PCI | I2C/SPI sensor, ISP |
| **Ví dụ** | i.MX CSI, USB UVC | OV7740, IMX219 sensor |

**Checklist viết V4L2 driver:**

✅ **Step 1: v4l2_device registration**
```c
v4l2_device_register(&pdev->dev, &mydev->v4l2_dev);
```

✅ **Step 2: Setup video_device**
```c
vdev->v4l2_dev = &mydev->v4l2_dev;
vdev->fops = &my_fops;
vdev->ioctl_ops = &my_ioctl_ops;
vdev->queue = &mydev->queue;
video_register_device(vdev, VFL_TYPE_GRABBER, -1);
```

✅ **Step 3: Initialize vb2_queue**
```c
q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
q->ops = &my_vb2_ops;
q->mem_ops = &vb2_dma_contig_memops;
vb2_queue_init(q);
```

✅ **Step 4: Implement vb2_ops**
```c
.queue_setup = my_queue_setup,
.buf_prepare = my_buf_prepare,
.buf_queue = my_buf_queue,
.start_streaming = my_start_streaming,
.stop_streaming = my_stop_streaming,
```

✅ **Step 5: DMA interrupt handler**
```c
vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
```

✅ **Step 6: Sub-device integration (nếu có)**
```c
v4l2_async_notifier_register();  // Bridge
v4l2_async_register_subdev();    // Sensor
```

✅ **Step 7: Controls (optional)**
```c
v4l2_ctrl_handler_init();
v4l2_ctrl_new_std();
```

**Common Pitfalls:**

❌ **Quên return buffers trong stop_streaming**
```c
// WRONG: Memory leak!
static void my_stop_streaming(struct vb2_queue *vq)
{
    stop_dma();
    // Quên return buffers!
}

// CORRECT:
static void my_stop_streaming(struct vb2_queue *vq)
{
    stop_dma();
    list_for_each_entry_safe(buf, node, &mydev->buf_list, list) {
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }
}
```

❌ **Sai buffer size trong queue_setup**
```c
// WRONG: Hardcoded size
sizes[0] = 640 * 480 * 2;  // Nếu user chọn 1920x1080 → fail!

// CORRECT: Dùng current format
sizes[0] = mydev->width * mydev->height * mydev->bytes_per_pixel;
```

❌ **Không check min_buffers_needed**
```c
// WRONG:
static int my_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    start_dma();  // Start luôn không check!
}

// CORRECT:
static int my_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    if (count < vq->min_buffers_needed)
        return -ENOBUFS;
    
    start_dma();
}
```

**Performance Tips:**

⚡ **Buffer count:**
- Tối thiểu 3 buffers cho smooth streaming
- USB devices: 4-8 buffers (higher latency)
- Platform devices: 3-4 buffers đủ

⚡ **DMA efficiency:**
```c
// Good: Physically contiguous memory
q->mem_ops = &vb2_dma_contig_memops;

// Slower: Scatter-gather (nếu hardware support)
q->mem_ops = &vb2_dma_sg_memops;

// Very slow: Virtual memory (USB only)
q->mem_ops = &vb2_vmalloc_memops;
```

⚡ **Interrupt handling:**
```c
// Good: Minimal work in IRQ, schedule tasklet/workqueue
static irqreturn_t my_irq(int irq, void *dev_id)
{
    clear_irq();
    vb2_buffer_done();  // Fast operation
    start_next_dma();
    return IRQ_HANDLED;
}

// Bad: Heavy processing in IRQ
static irqreturn_t bad_irq(int irq, void *dev_id)
{
    memcpy(...);  // Slow!
    complex_processing();  // Very bad!
}
```

**Debugging Commands:**

```bash
# List video devices
v4l2-ctl --list-devices

# Device info
v4l2-ctl -d /dev/video0 --all

# Test capture
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10

# Enable kernel debug
echo 8 > /proc/sys/kernel/printk
echo 'module videobuf2_v4l2 +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module videodev +p' > /sys/kernel/debug/dynamic_debug/control
```

**Kernel Source Code References:**

Để hiểu sâu hơn, đọc code của các drivers hiện có:

```
drivers/media/platform/
├── imx/                    # i.MX CSI bridge drivers
├── sunxi/sun6i-csi/       # Allwinner CSI (example in book)
├── qcom/camss/            # Qualcomm camera subsystem
└── amphion/               # Video encoder/decoder (M2M)

drivers/media/i2c/
├── ov7740.c               # OV7740 sensor (example in book)
├── ov5640.c               # Popular sensor, good reference
└── imx219.c               # Raspberry Pi camera

drivers/media/usb/uvc/     # USB Video Class (complex example)
```

**Recommended Reading Order:**

1. **Simple sensor driver**: `ov7740.c` (I2C sub-device)
2. **Simple bridge**: `sunxi/sun6i-csi/` (Platform DMA)
3. **USB camera**: `uvc/` (Advanced, all-in-one)

**Next Steps:**

📚 **Chapter 8**: V4L2 Async & Media Controller
- Async sub-device registration
- Device tree integration
- Media graph topology
- Complex video pipelines

📚 **Chapter 9**: V4L2 User Space API
- IOCTL usage from application
- Buffer management flow
- GStreamer integration

---

## PHỤ LỤC: Complete Minimal Bridge Driver

**Minimal working example (~150 lines):**

```c
// my-camera.c - Minimal V4L2 bridge driver
#include <linux/module.h>
#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

struct my_buffer {
    struct vb2_v4l2_buffer vb;
    struct list_head list;
    dma_addr_t dma_addr;
};

struct my_device {
    struct v4l2_device v4l2_dev;
    struct video_device vdev;
    struct vb2_queue queue;
    struct list_head buf_list;
    spinlock_t lock;
    void __iomem *regs;
    u32 sequence;
};

/* vb2 operations */
static int queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
                       unsigned int *nplanes, unsigned int sizes[],
                       struct device *alloc_devs[])
{
    *nplanes = 1;
    sizes[0] = 640 * 480 * 2;  // YUYV
    if (*nbuffers < 3)
        *nbuffers = 3;
    return 0;
}

static int buf_prepare(struct vb2_buffer *vb)
{
    vb2_set_plane_payload(vb, 0, 640 * 480 * 2);
    return 0;
}

static void buf_queue(struct vb2_buffer *vb)
{
    struct my_device *dev = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct my_buffer *buf = container_of(vbuf, struct my_buffer, vb);
    unsigned long flags;
    
    spin_lock_irqsave(&dev->lock, flags);
    list_add_tail(&buf->list, &dev->buf_list);
    spin_unlock_irqrestore(&dev->lock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
    struct my_device *dev = vb2_get_drv_priv(vq);
    /* Start DMA here */
    return 0;
}

static void stop_streaming(struct vb2_queue *vq)
{
    struct my_device *dev = vb2_get_drv_priv(vq);
    struct my_buffer *buf, *node;
    unsigned long flags;
    
    /* Stop DMA */
    
    spin_lock_irqsave(&dev->lock, flags);
    list_for_each_entry_safe(buf, node, &dev->buf_list, list) {
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
    }
    spin_unlock_irqrestore(&dev->lock, flags);
}

static const struct vb2_ops my_vb2_ops = {
    .queue_setup = queue_setup,
    .buf_prepare = buf_prepare,
    .buf_queue = buf_queue,
    .start_streaming = start_streaming,
    .stop_streaming = stop_streaming,
};

/* IOCTL operations */
static int my_querycap(struct file *file, void *priv,
                       struct v4l2_capability *cap)
{
    strscpy(cap->driver, "my-camera", sizeof(cap->driver));
    strscpy(cap->card, "My Camera", sizeof(cap->card));
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    return 0;
}

static const struct v4l2_ioctl_ops my_ioctl_ops = {
    .vidioc_querycap = my_querycap,
    .vidioc_reqbufs = vb2_ioctl_reqbufs,
    .vidioc_qbuf = vb2_ioctl_qbuf,
    .vidioc_dqbuf = vb2_ioctl_dqbuf,
    .vidioc_streamon = vb2_ioctl_streamon,
    .vidioc_streamoff = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = v4l2_fh_open,
    .release = vb2_fop_release,
    .unlocked_ioctl = video_ioctl2,
    .poll = vb2_fop_poll,
    .mmap = vb2_fop_mmap,
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;
    int ret;
    
    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;
    
    spin_lock_init(&dev->lock);
    INIT_LIST_HEAD(&dev->buf_list);
    
    /* Register V4L2 device */
    ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
    if (ret)
        return ret;
    
    /* Initialize queue */
    dev->queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dev->queue.io_modes = VB2_MMAP;
    dev->queue.ops = &my_vb2_ops;
    dev->queue.mem_ops = &vb2_dma_contig_memops;
    dev->queue.drv_priv = dev;
    dev->queue.buf_struct_size = sizeof(struct my_buffer);
    dev->queue.min_buffers_needed = 2;
    dev->queue.dev = &pdev->dev;
    
    ret = vb2_queue_init(&dev->queue);
    if (ret)
        goto err_v4l2;
    
    /* Setup video device */
    dev->vdev.v4l2_dev = &dev->v4l2_dev;
    dev->vdev.fops = &my_fops;
    dev->vdev.ioctl_ops = &my_ioctl_ops;
    dev->vdev.queue = &dev->queue;
    dev->vdev.release = video_device_release_empty;
    strscpy(dev->vdev.name, "my-camera", sizeof(dev->vdev.name));
    
    ret = video_register_device(&dev->vdev, VFL_TYPE_GRABBER, -1);
    if (ret)
        goto err_v4l2;
    
    platform_set_drvdata(pdev, dev);
    dev_info(&pdev->dev, "Device registered as %s\n",
             video_device_node_name(&dev->vdev));
    
    return 0;

err_v4l2:
    v4l2_device_unregister(&dev->v4l2_dev);
    return ret;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *dev = platform_get_drvdata(pdev);
    
    video_unregister_device(&dev->vdev);
    v4l2_device_unregister(&dev->v4l2_dev);
    
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my-camera",
    },
};

module_platform_driver(my_driver);

MODULE_DESCRIPTION("Minimal V4L2 Bridge Driver");
MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");
```

**Build và test:**

```bash
# Compile
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load
insmod my-camera.ko

# Test
v4l2-ctl -d /dev/video0 --all
```

---

**HẾT PART 2**

Tài liệu đã đầy đủ! Bây giờ bạn có:
- ✅ Part 1: Kiến trúc và 4 struct cốt lõi
- ✅ Part 2: videobuf2, sub-device, controls, code examples

Cần tôi tạo **file tổng hợp** hoặc giải thích thêm phần nào không? 🚀