# CHAPTER 8: INTEGRATING WITH V4L2 ASYNC AND MEDIA CONTROLLER FRAMEWORKS

## PART 2: MEDIA CONTROLLER FRAMEWORK

---

## 📌 Giới Thiệu Part 2

**Part 1 đã giải quyết:**
- ✅ Unordered probing (V4L2 Async)
- ✅ Device Tree graph binding (Fwnode API)

**Part 2 giải quyết:**
- ❓ Complex video pipeline: Sensor → MIPI → CSI → Resizer → ISP → Memory?
- ❓ Làm sao configure format cho từng sub-device?
- ❓ Làm sao routing động giữa các IP blocks?

**→ Media Controller Framework!**

**Nội dung Part 2:**
- 8.3. Media Controller Framework
- 8.4. Complete Integration Example
- 8.5. User Space Tools (media-ctl)
- 8.6. [ADVANCED] Topics

---

## 8.3. Media Controller Framework

### 8.3.1. Why Media Controller?

**Vấn đề: Complex Pipeline**

```
Sensor → MIPI CSI-2 → CSI Mux → Resizer → ISP → /dev/video0
                            ↓
                     Parallel Input
```

**Câu hỏi:**
- Làm sao switch giữa MIPI vs Parallel input?
- Làm sao set format khác nhau cho mỗi IP block?
- Làm sao biết sensor nào đang active?

**❌ Giải pháp cũ (KHÔNG TỐT):**
- Hardcode trong driver
- Sysfs hacks
- Không có unified API

**✅ Media Controller:**
- Unified framework để quản lý pipeline
- User space configure topology
- Per-pad format negotiation
- Runtime routing

**Khi nào CẦN Media Controller?**
- ✅ Multi sub-device pipeline (>2 devices)
- ✅ Format conversion giữa các blocks
- ✅ Runtime routing/switching
- ✅ SoC với nhiều IP blocks (ISP, scaler, mux)

**Khi nào KHÔNG CẦN?**
- ❌ USB camera (single device)
- ❌ Simple sensor → bridge (1-to-1)
- ❌ Fixed pipeline không thay đổi

---

### 8.3.2. Media Controller Abstraction Model

**Concepts:**

```
Media Device
  └─ Media Entity (IP block abstraction)
      └─ Media Pad (connection point)
          └─ Media Link (connection)
```

**Ví dụ thực tế:**

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  Sensor  │────▶│ MIPI CSI │────▶│   CSI    │────▶│  Capture │
│ (Entity) │     │ (Entity) │     │ (Entity) │     │ (Entity) │
└──────────┘     └──────────┘     └──────────┘     └──────────┘
    pad:0────────────pad:0   pad:1────────────pad:0   pad:0
   (source)         (sink)  (source)        (sink)   (sink)
         Link              Link              Link
```

**Key points:**
- **Entity** = IP block (sensor, CSI, ISP, etc.)
- **Pad** = Interface của entity (sink hoặc source)
- **Link** = Connection giữa 2 pads
- **Media Device** = Container của toàn bộ pipeline

---

### 8.3.3. struct media_device

**Định nghĩa:**

```c
struct media_device {
    struct device *dev;                    // Parent device
    struct media_devnode *devnode;         // /dev/mediaX
    char model[32];                        // Model name
    char driver_name[32];                  // Driver name
    u32 hw_revision;                       // Hardware revision
    u64 topology_version;                  // Topology version counter
    
    struct list_head entities;             // List of entities
    struct list_head pads;                 // List of pads
    struct list_head links;                // List of links
    
    struct mutex graph_mutex;              // Protect topology
    const struct media_device_ops *ops;    // Operations
};
```

**Khởi tạo và đăng ký:**

```c
struct my_bridge {
    struct media_device mdev;
    struct v4l2_device v4l2_dev;
    // ...
};

static int bridge_probe(struct platform_device *pdev)
{
    struct my_bridge *bridge;
    int ret;
    
    bridge = devm_kzalloc(&pdev->dev, sizeof(*bridge), GFP_KERNEL);
    
    /* 1. Khởi tạo media device */
    media_device_init(&bridge->mdev);
    bridge->mdev.dev = &pdev->dev;
    strscpy(bridge->mdev.model, "My Camera", sizeof(bridge->mdev.model));
    strscpy(bridge->mdev.driver_name, "my-bridge", 
            sizeof(bridge->mdev.driver_name));
    bridge->mdev.hw_revision = 0x01;
    
    /* 2. Khởi tạo V4L2 device và link với media device */
    bridge->v4l2_dev.mdev = &bridge->mdev;
    ret = v4l2_device_register(&pdev->dev, &bridge->v4l2_dev);
    if (ret)
        return ret;
    
    /* 3. Setup async notifier... */
    // (xem Part 1)
    
    /* 4. CHƯA đăng ký media device ở đây!
     *    Chờ .complete callback
     */
    
    return 0;
}
```

**⚠️ Quan trọng:**
- Đăng ký media device trong `.complete` callback
- Đảm bảo tất cả entities đã ready

---

### 8.3.4. struct media_entity

**Định nghĩa:**

```c
struct media_entity {
    struct media_gobj graph_obj;
    const char *name;                      // Entity name
    enum media_entity_type obj_type;       // V4L2_SUBDEV, VIDEO_DEVICE
    u32 function;                          // SENSOR, SCALER, etc.
    unsigned long flags;                   // Entity flags
    
    u16 num_pads;                          // Number of pads
    struct media_pad *pads;                // Pad array
    
    struct list_head links;                // List of links
    const struct media_entity_operations *ops;  // Operations
    
    int use_count;                         // Reference count
    struct media_pipeline *pipe;           // Current pipeline
};
```

**Entity Functions (common):**

| Function | Description | Example |
|----------|-------------|---------|
| `MEDIA_ENT_F_CAM_SENSOR` | Camera sensor | OV5640, IMX219 |
| `MEDIA_ENT_F_VID_IF_BRIDGE` | Video interface bridge | MIPI CSI-2 receiver |
| `MEDIA_ENT_F_PROC_VIDEO_SCALER` | Video scaler | Resize block |
| `MEDIA_ENT_F_IO_V4L` | V4L2 video device | /dev/videoX |
| `MEDIA_ENT_F_VID_MUX` | Video multiplexer | CSI mux |

**V4L2 structures embed media_entity:**

```c
struct video_device {
    struct media_entity entity;   // Video device = entity
    // ...
};

struct v4l2_subdev {
    struct media_entity entity;   // Sub-device = entity
    // ...
};
```

**→ Sensor, bridge đều là media entities!**

---

### 8.3.5. struct media_pad

**Định nghĩa:**

```c
struct media_pad {
    struct media_entity *entity;           // Parent entity
    u16 index;                             // Pad index trong entity
    unsigned long flags;                   // SINK hoặc SOURCE
};
```

**Pad flags:**

```c
#define MEDIA_PAD_FL_SINK        (1 << 0)  // Input pad
#define MEDIA_PAD_FL_SOURCE      (1 << 1)  // Output pad
```

**Khởi tạo pads:**

```c
/* Sensor: 1 source pad */
static int sensor_probe(struct i2c_client *client)
{
    struct sensor_dev *sensor;
    struct media_pad *pad;
    int ret;
    
    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    
    /* Allocate 1 pad */
    pad = &sensor->pad;
    pad->flags = MEDIA_PAD_FL_SOURCE;
    
    /* Khởi tạo entity + pads */
    sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    ret = media_entity_pads_init(&sensor->sd.entity, 1, pad);
    if (ret)
        return ret;
    
    /* Register subdev (entity sẽ tự động register) */
    ret = v4l2_async_register_subdev(&sensor->sd);
    if (ret)
        goto err_cleanup;
    
    return 0;

err_cleanup:
    media_entity_cleanup(&sensor->sd.entity);
    return ret;
}
```

**Bridge: 1 sink pad**

```c
/* Bridge: 1 sink pad (nhận từ sensor) */
static int bridge_init_entity(struct bridge_dev *bridge)
{
    struct media_pad *pad;
    
    pad = &bridge->pad;
    pad->flags = MEDIA_PAD_FL_SINK;
    
    bridge->vdev->entity.function = MEDIA_ENT_F_IO_V4L;
    
    return media_entity_pads_init(&bridge->vdev->entity, 1, pad);
}
```

---

### 8.3.6. struct media_link

**Định nghĩa:**

```c
struct media_link {
    struct media_pad *source;              // Source pad
    struct media_pad *sink;                // Sink pad
    struct media_link *reverse;            // Backlink
    unsigned long flags;                   // Link flags
    bool is_backlink;                      // Is this a backlink?
};
```

**Link flags:**

```c
#define MEDIA_LNK_FL_ENABLED       (1 << 0)  // Link is active
#define MEDIA_LNK_FL_IMMUTABLE     (1 << 1)  // Can't be disabled
#define MEDIA_LNK_FL_DYNAMIC       (1 << 2)  // Can change during streaming
```

**Tạo link:**

```c
int media_create_pad_link(struct media_entity *source,
                          u16 source_pad,
                          struct media_entity *sink,
                          u16 sink_pad,
                          u32 flags);
```

**Example:**

```c
/* Link sensor → bridge */
static int bridge_create_links(struct bridge_dev *bridge,
                               struct v4l2_subdev *sensor)
{
    int ret;
    
    /* sensor.pad[0] ──ENABLED──> bridge.pad[0] */
    ret = media_create_pad_link(&sensor->entity, 0,
                                 &bridge->vdev->entity, 0,
                                 MEDIA_LNK_FL_ENABLED |
                                 MEDIA_LNK_FL_IMMUTABLE);
    if (ret) {
        dev_err(bridge->dev, "Failed to create link\n");
        return ret;
    }
    
    return 0;
}
```

**Link & Backlink:**

```
Entity A                    Entity B
pad[0] (source) ──link──▶  pad[0] (sink)
                 ◀─backlink─
```

- **Link**: Thuộc entity A, num_links++
- **Backlink**: Thuộc entity B, num_backlinks++
- **Mục đích**: Graph traversal

---

### 8.3.7. Entity/Pad/Link Registration

**Tự động registration:**

```c
/* Sub-device registration → entity auto-registered */
v4l2_async_register_subdev(sd);
  └─▶ calls media_device_register_entity(&sd->entity)

/* Video device registration → entity auto-registered */
video_register_device(vdev, ...);
  └─▶ calls media_device_register_entity(&vdev->entity)
```

**Manual cleanup nếu cần:**

```c
static void sensor_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    
    /* Unregister subdev (entity sẽ tự động unregister) */
    v4l2_async_unregister_subdev(sd);
    
    /* Cleanup entity resources */
    media_entity_cleanup(&sd->entity);
}
```

---

### 8.3.8. Media Entity Operations

**Optional callbacks:**

```c
struct media_entity_operations {
    int (*link_setup)(struct media_entity *entity,
                      const struct media_pad *local,
                      const struct media_pad *remote,
                      u32 flags);
    int (*link_validate)(struct media_link *link);
};
```

**link_setup callback:**

```c
static int sensor_link_setup(struct media_entity *entity,
                              const struct media_pad *local,
                              const struct media_pad *remote,
                              u32 flags)
{
    struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
    struct sensor_dev *sensor = to_sensor(sd);
    
    if (flags & MEDIA_LNK_FL_ENABLED) {
        /* Link được enable */
        dev_info(sensor->dev, "Link enabled\n");
        /* Optional: Power on sensor */
    } else {
        /* Link bị disable */
        dev_info(sensor->dev, "Link disabled\n");
        /* Optional: Power off sensor */
    }
    
    return 0;
}

static const struct media_entity_operations sensor_entity_ops = {
    .link_setup = sensor_link_setup,
};
```

**Set operations:**

```c
static int sensor_probe(struct i2c_client *client)
{
    struct sensor_dev *sensor;
    
    // ...
    
    sensor->sd.entity.ops = &sensor_entity_ops;
    
    // ...
}
```

---

## 8.4. Complete Integration Example

### 8.4.1. Bridge Driver với Async + Media

**Complete bridge driver:**

```c
struct my_bridge {
    struct device *dev;
    struct v4l2_device v4l2_dev;
    struct media_device mdev;
    struct video_device *vdev;
    struct v4l2_async_notifier notifier;
    
    struct media_pad pad;
    struct v4l2_subdev *sensor_sd;
};

/* Notifier callbacks */
static int bridge_bound(struct v4l2_async_notifier *notifier,
                        struct v4l2_subdev *sd,
                        struct v4l2_async_subdev *asd)
{
    struct my_bridge *bridge = 
        container_of(notifier, struct my_bridge, notifier);
    
    /* Save sensor sub-device pointer */
    bridge->sensor_sd = sd;
    
    dev_info(bridge->dev, "Sensor %s bound\n", sd->name);
    return 0;
}

static int bridge_complete(struct v4l2_async_notifier *notifier)
{
    struct my_bridge *bridge = 
        container_of(notifier, struct my_bridge, notifier);
    int ret;
    
    /* 1. Khởi tạo video device */
    bridge->vdev = video_device_alloc();
    if (!bridge->vdev)
        return -ENOMEM;
    
    /* Setup video device (fops, queue, etc.) */
    // ... (xem Chapter 7)
    
    /* 2. Khởi tạo entity cho video device */
    bridge->pad.flags = MEDIA_PAD_FL_SINK;
    bridge->vdev->entity.function = MEDIA_ENT_F_IO_V4L;
    ret = media_entity_pads_init(&bridge->vdev->entity, 1, &bridge->pad);
    if (ret)
        goto err_vdev;
    
    /* 3. Đăng ký video device (entity auto-registered) */
    ret = video_register_device(bridge->vdev, VFL_TYPE_GRABBER, -1);
    if (ret)
        goto err_entity;
    
    /* 4. Tạo link: sensor → bridge */
    ret = media_create_pad_link(&bridge->sensor_sd->entity, 0,
                                 &bridge->vdev->entity, 0,
                                 MEDIA_LNK_FL_ENABLED |
                                 MEDIA_LNK_FL_IMMUTABLE);
    if (ret)
        goto err_unreg;
    
    /* 5. Tạo /dev/v4l-subdevX cho sensor nếu cần */
    ret = v4l2_device_register_subdev_nodes(&bridge->v4l2_dev);
    if (ret)
        goto err_unreg;
    
    /* 6. Đăng ký media device */
    ret = media_device_register(&bridge->mdev);
    if (ret)
        goto err_unreg;
    
    dev_info(bridge->dev, "Media device registered as /dev/media%d\n",
             bridge->mdev.devnode->minor);
    
    return 0;

err_unreg:
    video_unregister_device(bridge->vdev);
err_entity:
    media_entity_cleanup(&bridge->vdev->entity);
err_vdev:
    video_device_release(bridge->vdev);
    return ret;
}

static const struct v4l2_async_notifier_operations bridge_notifier_ops = {
    .bound = bridge_bound,
    .complete = bridge_complete,
};

/* Probe function */
static int bridge_probe(struct platform_device *pdev)
{
    struct my_bridge *bridge;
    int ret;
    
    bridge = devm_kzalloc(&pdev->dev, sizeof(*bridge), GFP_KERNEL);
    if (!bridge)
        return -ENOMEM;
    
    bridge->dev = &pdev->dev;
    
    /* 1. Khởi tạo media device */
    media_device_init(&bridge->mdev);
    bridge->mdev.dev = bridge->dev;
    strscpy(bridge->mdev.model, "My Camera", 
            sizeof(bridge->mdev.model));
    
    /* 2. Khởi tạo V4L2 device */
    bridge->v4l2_dev.mdev = &bridge->mdev;
    ret = v4l2_device_register(bridge->dev, &bridge->v4l2_dev);
    if (ret)
        return ret;
    
    /* 3. Parse DT và tạo async notifier */
    ret = bridge_parse_dt(bridge);  // (xem Part 1)
    if (ret)
        goto err_unreg;
    
    bridge->notifier.ops = &bridge_notifier_ops;
    
    /* 4. Đăng ký async notifier */
    ret = v4l2_async_notifier_register(&bridge->v4l2_dev,
                                        &bridge->notifier);
    if (ret)
        goto err_unreg;
    
    platform_set_drvdata(pdev, bridge);
    
    dev_info(bridge->dev, "Bridge driver probed\n");
    return 0;

err_unreg:
    v4l2_device_unregister(&bridge->v4l2_dev);
    return ret;
}

static int bridge_remove(struct platform_device *pdev)
{
    struct my_bridge *bridge = platform_get_drvdata(pdev);
    
    /* 1. Unregister media device */
    media_device_unregister(&bridge->mdev);
    media_device_cleanup(&bridge->mdev);
    
    /* 2. Unregister async notifier */
    v4l2_async_notifier_unregister(&bridge->notifier);
    
    /* 3. Unregister video device */
    if (bridge->vdev) {
        video_unregister_device(bridge->vdev);
        media_entity_cleanup(&bridge->vdev->entity);
    }
    
    /* 4. Unregister V4L2 device */
    v4l2_device_unregister(&bridge->v4l2_dev);
    
    return 0;
}
```

---

### 8.4.2. The Concept of Media Bus

**Vấn đề:**
- Mỗi pad có format riêng (width, height, pixel format)
- Pipeline phải coherent (format phải compatible)

**Giải pháp: Pad-level format negotiation**

**struct v4l2_subdev_pad_ops:**

```c
struct v4l2_subdev_pad_ops {
    int (*enum_mbus_code)(struct v4l2_subdev *sd,
                          struct v4l2_subdev_pad_config *cfg,
                          struct v4l2_subdev_mbus_code_enum *code);
    int (*enum_frame_size)(struct v4l2_subdev *sd,
                           struct v4l2_subdev_pad_config *cfg,
                           struct v4l2_subdev_frame_size_enum *fse);
    int (*get_fmt)(struct v4l2_subdev *sd,
                   struct v4l2_subdev_pad_config *cfg,
                   struct v4l2_subdev_format *format);
    int (*set_fmt)(struct v4l2_subdev *sd,
                   struct v4l2_subdev_pad_config *cfg,
                   struct v4l2_subdev_format *format);
};
```

**set_fmt implementation:**

```c
static int sensor_set_fmt(struct v4l2_subdev *sd,
                          struct v4l2_subdev_pad_config *cfg,
                          struct v4l2_subdev_format *format)
{
    struct sensor_dev *sensor = to_sensor(sd);
    struct v4l2_mbus_framefmt *mbus_fmt = &format->format;
    
    /* Validate format */
    if (mbus_fmt->code != MEDIA_BUS_FMT_SBGGR10_1X10) {
        mbus_fmt->code = MEDIA_BUS_FMT_SBGGR10_1X10;
    }
    
    /* Clamp resolution */
    mbus_fmt->width = clamp_t(u32, mbus_fmt->width, 320, 1920);
    mbus_fmt->height = clamp_t(u32, mbus_fmt->height, 240, 1080);
    
    /* Store format */
    if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
        /* Active format → apply to hardware */
        sensor->fmt = *mbus_fmt;
    } else {
        /* Try format → store in pad config */
        *v4l2_subdev_get_try_format(sd, cfg, 0) = *mbus_fmt;
    }
    
    return 0;
}

static int sensor_get_fmt(struct v4l2_subdev *sd,
                          struct v4l2_subdev_pad_config *cfg,
                          struct v4l2_subdev_format *format)
{
    struct sensor_dev *sensor = to_sensor(sd);
    
    if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
        format->format = sensor->fmt;
    } else {
        format->format = *v4l2_subdev_get_try_format(sd, cfg, 0);
    }
    
    return 0;
}

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
    .get_fmt = sensor_get_fmt,
    .set_fmt = sensor_set_fmt,
};
```

**User space set format:**

```bash
# Set format on sensor pad 0
media-ctl -V "'sensor 1-003c':0 [fmt:SBGGR10_1X10/1920x1080]"

# Set format on bridge pad 0
media-ctl -V "'my-bridge':0 [fmt:SBGGR10_1X10/1920x1080]"
```

---

### 8.4.3. Device Tree Complete

```dts
/* Bridge node */
csi: csi@1c09000 {
    compatible = "allwinner,sun8i-v3s-csi";
    reg = <0x01c09000 0x1000>;
    interrupts = <GIC_SPI 83 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&ccu CLK_BUS_CSI>,
             <&ccu CLK_CSI_SCLK>,
             <&ccu CLK_DRAM_CSI>;
    clock-names = "bus", "mod", "ram";
    resets = <&ccu RST_BUS_CSI>;
    
    port {
        csi_ep: endpoint {
            remote-endpoint = <&ov7740_ep>;
            bus-width = <8>;
            hsync-active = <1>;
            vsync-active = <1>;
            pclk-sample = <1>;
        };
    };
};

/* Sensor node */
&i2c1 {
    ov7740: camera@21 {
        compatible = "ovti,ov7740";
        reg = <0x21>;
        clocks = <&ov7740_clk>;
        clock-names = "xvclk";
        reset-gpios = <&pio 4 23 GPIO_ACTIVE_LOW>; /* PE23 */
        powerdown-gpios = <&pio 4 24 GPIO_ACTIVE_HIGH>; /* PE24 */
        
        port {
            ov7740_ep: endpoint {
                remote-endpoint = <&csi_ep>;
            };
        };
    };
};
```

---

### 8.4.4. Boot Sequence Flow

```
1. Sensor probe:
   ├─ v4l2_i2c_subdev_init()
   ├─ media_entity_pads_init() (1 source pad)
   └─ v4l2_async_register_subdev()
        └─ Async core: Add to subdev_list (chờ bridge)

2. Bridge probe:
   ├─ v4l2_device_register()
   ├─ media_device_init()
   ├─ Parse DT → Create async_subdevs
   └─ v4l2_async_notifier_register()
        └─ Async core: Match sensor ✓
             ├─ .bound(sensor) callback
             └─ notifier->waiting empty
                  └─ .complete() callback
                       ├─ video_register_device() → /dev/video0
                       ├─ media_create_pad_link()
                       └─ media_device_register() → /dev/media0

3. User space:
   ├─ media-ctl --print-topology
   ├─ media-ctl -V (set format)
   └─ v4l2-ctl --stream-mmap (capture)
```

---

## 8.5. User Space Tools

### 8.5.1. media-ctl Usage

**Install:**

```bash
sudo apt-get install v4l-utils
```

**A. Print Topology**

```bash
# Print full topology
media-ctl -d /dev/media0 --print-topology

# Short version
media-ctl -d /dev/media0 -p
```

**Output example:**

```
Media controller API version 5.10.0

Media device information
------------------------
driver          my-bridge
model           My Camera
serial
bus info        platform:csi
hw revision     0x1
driver version  5.10.0

Device topology
- entity 1: my-bridge (1 pad, 1 link)
            type Node subtype V4L flags 0
            device node name /dev/video0
        pad0: Sink
                <- "ov7740 1-0021":0 [ENABLED,IMMUTABLE]

- entity 5: ov7740 1-0021 (1 pad, 1 link)
            type V4L2 subdev subtype Sensor flags 0
            device node name /dev/v4l-subdev0
        pad0: Source
                [fmt:SBGGR10_1X10/640x480 field:none]
                -> "my-bridge":0 [ENABLED,IMMUTABLE]
```

**B. Generate Graph**

```bash
# Generate DOT file
media-ctl --print-dot > graph.dot

# Convert to PNG (requires graphviz)
dot -Tpng graph.dot > graph.png
```

**C. Reset Links**

```bash
# Disable all links
media-ctl --reset
```

**D. Set Links**

```bash
# Enable link: sensor.pad0 → bridge.pad0
media-ctl --links "'ov7740 1-0021':0 -> 'my-bridge':0[1]"

# [1] = ENABLED, [0] = DISABLED
```

**E. Set Format**

```bash
# Set format on sensor pad 0
media-ctl --set-v4l2 "'ov7740 1-0021':0[fmt:SBGGR10_1X10/640x480]"

# Short version
media-ctl -V "'ov7740 1-0021':0[fmt:SBGGR10_1X10/640x480]"
```

**F. Get Format**

```bash
# Get format from sensor pad 0
media-ctl --get-v4l2 "'ov7740 1-0021':0"
```

---

### 8.5.2. Complete Configuration Example

**i.MX7 + OV2680 setup:**

```bash
#!/bin/bash

# Reset all links
media-ctl --reset

# Setup pipeline links
media-ctl --links "'ov2680 1-0036':0 -> 'imx7-mipi-csis.0':0[1]"
media-ctl --links "'imx7-mipi-csis.0':1 -> 'csi_mux':1[1]"
media-ctl --links "'csi_mux':2 -> 'csi':0[1]"
media-ctl --links "'csi':1 -> 'csi capture':0[1]"

# Configure formats for each pad
media-ctl -V "'ov2680 1-0036':0[fmt:SBGGR10_1X10/800x600]"
media-ctl -V "'imx7-mipi-csis.0':0[fmt:SBGGR10_1X10/800x600]"
media-ctl -V "'csi_mux':1[fmt:SBGGR10_1X10/800x600]"
media-ctl -V "'csi_mux':2[fmt:SBGGR10_1X10/800x600]"
media-ctl -V "'csi':0[fmt:SBGGR10_1X10/800x600]"

# Verify topology
media-ctl -p

echo "Pipeline configured successfully!"
```

**Single command version:**

```bash
# Reset + Links + Formats in one command
media-ctl -r \
  -l "'ov2680 1-0036':0->'imx7-mipi-csis.0':0[1], \
      'imx7-mipi-csis.0':1->'csi_mux':1[1], \
      'csi_mux':2->'csi':0[1], \
      'csi':1->'csi capture':0[1]" \
  -V "'ov2680 1-0036':0[fmt:SBGGR10_1X10/800x600], \
      'imx7-mipi-csis.0':0[fmt:SBGGR10_1X10/800x600], \
      'csi_mux':1[fmt:SBGGR10_1X10/800x600], \
      'csi_mux':2[fmt:SBGGR10_1X10/800x600], \
      'csi':0[fmt:SBGGR10_1X10/800x600]"
```

---

### 8.5.3. v4l2-ctl với Media Controller

**List formats:**

```bash
# List formats supported by /dev/video0
v4l2-ctl -d /dev/video0 --list-formats-ext
```

**Set format (video device level):**

```bash
# Set format on video device
v4l2-ctl -d /dev/video0 --set-fmt-video=width=800,height=600,pixelformat=BA10
```

**Capture:**

```bash
# Capture 10 frames
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 --stream-to=frame.raw

# Capture with format info
v4l2-ctl -d /dev/video0 --verbose --stream-mmap=3
```

**Query capabilities:**

```bash
# Show device capabilities
v4l2-ctl -d /dev/video0 --all

# Show controls
v4l2-ctl -d /dev/video0 --list-ctrls
```

---

### 8.5.4. Topology Interpretation

**Hiểu output của media-ctl:**

```
- entity 1: csi (2 pads, 2 links)
        pad0: Sink
                [fmt:SBGGR10_1X10/800x600 field:none]
                <- "csi-mux":2 [ENABLED]
        pad1: Source
                [fmt:SBGGR10_1X10/800x600 field:none]
                -> "csi capture":0 [ENABLED]
```

**Giải thích:**

```
Entity: csi
├─ pad0 (SINK): Nhận data từ csi-mux.pad2
│   ├─ Format: SBGGR10_1X10/800x600
│   └─ Link: <- "csi-mux":2 [ENABLED]
│
└─ pad1 (SOURCE): Gửi data tới csi capture.pad0
    ├─ Format: SBGGR10_1X10/800x600
    └─ Link: -> "csi capture":0 [ENABLED]
```

**Quy tắc đọc:**
- `<-` : Pad nhận data (SINK)
- `->` : Pad gửi data (SOURCE)
- `[ENABLED]` : Link đang active
- `[IMMUTABLE]` : Link không thể disable

---

## 8.6. [ADVANCED] Nâng Cao

### 8.6.1. Media Entity Types

**Chi tiết về entity functions:**

```c
/* Camera sensors */
#define MEDIA_ENT_F_CAM_SENSOR           0x00020001

/* Video interface bridges */
#define MEDIA_ENT_F_VID_IF_BRIDGE        0x00030002

/* Video processing */
#define MEDIA_ENT_F_PROC_VIDEO_SCALER    0x00040001
#define MEDIA_ENT_F_PROC_VIDEO_ENCODER   0x00040004
#define MEDIA_ENT_F_PROC_VIDEO_DECODER   0x00040005
#define MEDIA_ENT_F_PROC_VIDEO_ISP       0x00040006

/* Video multiplexers/switches */
#define MEDIA_ENT_F_VID_MUX              0x00050001

/* V4L2 video nodes */
#define MEDIA_ENT_F_IO_V4L               0x01000001
```

**Use case: Multi-function entity**

```c
/* ISP có cả scaling và encoding */
static int isp_probe(struct platform_device *pdev)
{
    struct isp_dev *isp;
    
    // ...
    
    /* ISP entity */
    isp->sd.entity.function = MEDIA_ENT_F_PROC_VIDEO_ISP;
    
    /* 2 sink pads, 2 source pads */
    isp->pads[0].flags = MEDIA_PAD_FL_SINK;   // Raw input
    isp->pads[1].flags = MEDIA_PAD_FL_SINK;   // Params input
    isp->pads[2].flags = MEDIA_PAD_FL_SOURCE; // Processed output
    isp->pads[3].flags = MEDIA_PAD_FL_SOURCE; // Stats output
    
    ret = media_entity_pads_init(&isp->sd.entity, 4, isp->pads);
    
    // ...
}
```

---

### 8.6.2. Dynamic Topology

**Runtime link switching:**

```c
static const struct media_entity_operations mux_ops = {
    .link_setup = mux_link_setup,
};

static int mux_link_setup(struct media_entity *entity,
                          const struct media_pad *local,
                          const struct media_pad *remote,
                          u32 flags)
{
    struct mux_dev *mux = entity_to_mux(entity);
    
    if (local->flags & MEDIA_PAD_FL_SOURCE)
        return 0; /* Source pad không cần xử lý */
    
    /* Sink pad: switch input source */
    if (flags & MEDIA_LNK_FL_ENABLED) {
        /* Enable input từ pad này */
        mux->active_input = local->index;
        
        /* Configure hardware */
        writel(local->index, mux->regs + MUX_SEL);
        
        dev_info(mux->dev, "Switched to input %d\n", local->index);
    }
    
    return 0;
}
```

**User space switching:**

```bash
# Disable input 0
media-ctl -l "'mux':0 -> 'output':0[0]"

# Enable input 1
media-ctl -l "'mux':1 -> 'output':0[1]"
```

---

### 8.6.3. V4L2 Subdev Userspace API

**Enable subdev nodes:**

```c
/* Sensor driver */
static int sensor_probe(struct i2c_client *client)
{
    struct v4l2_subdev *sd;
    
    // ...
    
    /* Enable /dev/v4l-subdevX creation */
    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    
    ret = v4l2_async_register_subdev(sd);
    
    // ...
}
```

**Direct subdev control:**

```bash
# Query subdev capabilities
v4l2-ctl -d /dev/v4l-subdev0 --all

# Set format directly on subdev
v4l2-ctl -d /dev/v4l-subdev0 \
  --set-subdev-fmt pad=0,width=1920,height=1080,code=0x3007

# Get format
v4l2-ctl -d /dev/v4l-subdev0 --get-subdev-fmt pad=0
```

**Application code:**

```c
#include <linux/v4l2-subdev.h>

int set_subdev_format(int fd, int pad, int width, int height)
{
    struct v4l2_subdev_format fmt = {
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
        .pad = pad,
        .format = {
            .width = width,
            .height = height,
            .code = MEDIA_BUS_FMT_SBGGR10_1X10,
            .field = V4L2_FIELD_NONE,
        },
    };
    
    return ioctl(fd, VIDIOC_SUBDEV_S_FMT, &fmt);
}
```

---

### 8.6.4. Complex Topology Example

**i.MX6 IPU Pipeline:**

```
Sensor → MIPI CSI-2 → CSI → IC PRP → IC PRP ENC → Capture
                        ↓
                     CSI Direct (bypass)
```

**Device Tree:**

```dts
&mipi_csi {
    port@0 {
        reg = <0>;
        mipi_csi_in: endpoint {
            remote-endpoint = <&ov5640_out>;
            data-lanes = <1 2>;
        };
    };
    
    port@1 {
        reg = <1>;
        mipi_csi_out: endpoint {
            remote-endpoint = <&ipu1_csi0_in>;
        };
    };
};

&ipu1_csi0 {
    port@0 {
        reg = <0>;
        ipu1_csi0_in: endpoint {
            remote-endpoint = <&mipi_csi_out>;
        };
    };
    
    port@1 {
        reg = <1>;
        ipu1_csi0_out: endpoint {
            remote-endpoint = <&ipu1_ic_prp_in>;
        };
    };
};
```

**Configuration:**

```bash
# Full pipeline
media-ctl -l "'ov5640 2-003c':0 -> 'imx6-mipi-csi2':0[1]"
media-ctl -l "'imx6-mipi-csi2':2 -> 'ipu1_csi0':0[1]"
media-ctl -l "'ipu1_csi0':2 -> 'ipu1_ic_prp':0[1]"
media-ctl -l "'ipu1_ic_prp':2 -> 'ipu1_ic_prpenc':0[1]"
media-ctl -l "'ipu1_ic_prpenc':1 -> 'ipu1_ic_prpenc capture':0[1]"

# Set formats
media-ctl -V "'ov5640 2-003c':0[fmt:UYVY8_2X8/1920x1080]"
media-ctl -V "'ipu1_csi0':2[fmt:UYVY8_2X8/1920x1080]"
media-ctl -V "'ipu1_ic_prp':2[fmt:UYVY8_2X8/1920x1080]"
media-ctl -V "'ipu1_ic_prpenc':1[fmt:UYVY8_2X8/640x480]" # Downscale!
```

---

### 8.6.5. Pipeline State Management

**struct media_pipeline:**

```c
struct media_pipeline {
    int streaming_count;
    struct media_graph graph;
};
```

**Start/Stop pipeline:**

```c
/* Bridge driver: Start streaming */
static int bridge_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    struct bridge_dev *bridge = vb2_get_drv_priv(vq);
    struct media_entity *entity = &bridge->vdev->entity;
    struct media_pipeline *pipe;
    int ret;
    
    /* Allocate pipeline */
    pipe = kzalloc(sizeof(*pipe), GFP_KERNEL);
    if (!pipe)
        return -ENOMEM;
    
    /* Start media pipeline */
    ret = media_pipeline_start(entity, pipe);
    if (ret) {
        kfree(pipe);
        return ret;
    }
    
    /* Enable sensor streaming */
    ret = v4l2_subdev_call(bridge->sensor_sd, video, s_stream, 1);
    if (ret) {
        media_pipeline_stop(entity);
        kfree(pipe);
        return ret;
    }
    
    bridge->pipe = pipe;
    return 0;
}

static void bridge_stop_streaming(struct vb2_queue *vq)
{
    struct bridge_dev *bridge = vb2_get_drv_priv(vq);
    struct media_entity *entity = &bridge->vdev->entity;
    
    /* Disable sensor streaming */
    v4l2_subdev_call(bridge->sensor_sd, video, s_stream, 0);
    
    /* Stop media pipeline */
    media_pipeline_stop(entity);
    
    kfree(bridge->pipe);
    bridge->pipe = NULL;
}
```

---

### 8.6.6. Link Validation

**Custom validation:**

```c
static int bridge_link_validate(struct media_link *link)
{
    struct v4l2_subdev *sd = 
        media_entity_to_v4l2_subdev(link->source->entity);
    struct video_device *vdev = 
        media_entity_to_video_device(link->sink->entity);
    struct v4l2_subdev_format source_fmt = {
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
        .pad = link->source->index,
    };
    struct v4l2_format sink_fmt;
    int ret;
    
    /* Get source format */
    ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &source_fmt);
    if (ret)
        return ret;
    
    /* Get sink format */
    ret = vdev->fops->vidioc_g_fmt_vid_cap(NULL, NULL, &sink_fmt);
    if (ret)
        return ret;
    
    /* Validate compatibility */
    if (source_fmt.format.width != sink_fmt.fmt.pix.width ||
        source_fmt.format.height != sink_fmt.fmt.pix.height) {
        pr_err("Format mismatch: %dx%d vs %dx%d\n",
               source_fmt.format.width, source_fmt.format.height,
               sink_fmt.fmt.pix.width, sink_fmt.fmt.pix.height);
        return -EPIPE;
    }
    
    return 0;
}

static const struct media_entity_operations bridge_media_ops = {
    .link_validate = bridge_link_validate,
};
```

---

## 8.7. Tổng Kết

### ✅ **Key Takeaways**

**1. V4L2 Async Framework:**
- Giải quyết unordered probing
- Bridge đăng ký notifier với danh sách sub-devices
- Sensor đăng ký async subdev
- Async core matching → .bound() → .complete()
- Đăng ký `/dev/videoX` trong `.complete`

**2. V4L2 Fwnode API:**
- Generic abstraction: DT + ACPI
- Graph binding: port → endpoint → remote-endpoint
- Parse bus properties: `v4l2_fwnode_endpoint_parse()`
- Support nhiều bus: Parallel, BT656, MIPI CSI-2

**3. Media Controller Framework:**
- Quản lý complex pipeline
- Entity = IP block (sensor, CSI, ISP, etc.)
- Pad = Connection point (sink/source)
- Link = Connection giữa pads
- User space configure qua `/dev/mediaX`

---

### 📋 **Khi Nào Dùng Gì?**

**V4L2 Async:**
- ✅ Device Tree-based system
- ✅ Bridge + external sensors
- ✅ Unordered probing

**Media Controller:**
- ✅ Multi sub-device (>2)
- ✅ Format conversion
- ✅ Runtime routing
- ✅ SoC với ISP/scaler

**Không cần:**
- ❌ USB camera
- ❌ Simple 1-to-1 pipeline
- ❌ Fixed routing

---

### ✅ **Driver Development Checklist**

**Bridge Driver:**
- [ ] Khởi tạo `media_device`
- [ ] Link `v4l2_device.mdev`
- [ ] Parse DT, tạo `async_subdev` array
- [ ] Implement notifier ops (.bound, .complete)
- [ ] Đăng ký notifier
- [ ] Trong `.complete`:
  - [ ] Khởi tạo video device entity + pads
  - [ ] Đăng ký video device
  - [ ] Tạo links giữa entities
  - [ ] Đăng ký media device

**Sensor Driver:**
- [ ] Khởi tạo sub-device
- [ ] Set `entity.function = MEDIA_ENT_F_CAM_SENSOR`
- [ ] Khởi tạo pads (1 source)
- [ ] Implement pad ops (get_fmt, set_fmt)
- [ ] Đăng ký async sub-device
- [ ] Optional: Enable subdev node (`V4L2_SUBDEV_FL_HAS_DEVNODE`)

**Device Tree:**
- [ ] Bridge node với port/endpoint
- [ ] Sensor node với port/endpoint
- [ ] Link qua `remote-endpoint`
- [ ] Bus properties (data-lanes, hsync-active, etc.)

---

### 🔧 **Debugging Tips**

**1. Check topology:**
```bash
media-ctl -p
```

**2. Verify links:**
```bash
# All links should show [ENABLED]
media-ctl -p | grep ENABLED
```

**3. Check formats:**
```bash
media-ctl -p | grep fmt:
```

**4. Kernel logs:**
```bash
dmesg | grep -E 'v4l2|media|async'
```

**5. Trace:**
```bash
echo 1 > /sys/kernel/debug/tracing/events/v4l2/enable
cat /sys/kernel/debug/tracing/trace
```

---

### 📚 **Common Media Bus Formats**

| Format | Description | Bits per pixel |
|--------|-------------|----------------|
| `MEDIA_BUS_FMT_SBGGR10_1X10` | 10-bit Bayer BGGR | 10 |
| `MEDIA_BUS_FMT_SGRBG10_1X10` | 10-bit Bayer GRBG | 10 |
| `MEDIA_BUS_FMT_UYVY8_2X8` | YUV 4:2:2 UYVY | 16 |
| `MEDIA_BUS_FMT_YUYV8_2X8` | YUV 4:2:2 YUYV | 16 |
| `MEDIA_BUS_FMT_RGB888_1X24` | RGB888 | 24 |

---

### 🎯 **Real-World Examples**

**Simple: Single sensor**
```
Sensor → Bridge → /dev/video0
```
- Không cần media controller
- Async framework đủ

**Medium: ISP pipeline**
```
Sensor → MIPI → CSI → ISP → /dev/video0
```
- Cần media controller
- Format negotiation per pad

**Complex: Multi-input**
```
MIPI Sensor   ↘
               Mux → CSI → ISP → /dev/video0
Parallel Sensor ↗
```
- Cần media controller
- Runtime link switching

---

### 📖 **Further Reading**

**Kernel Documentation:**
- `Documentation/driver-api/media/v4l2-async.rst`
- `Documentation/driver-api/media/mc-core.rst`
- `Documentation/devicetree/bindings/media/video-interfaces.txt`

**Header Files:**
- `include/media/v4l2-async.h`
- `include/media/v4l2-fwnode.h`
- `include/media/media-device.h`
- `include/media/media-entity.h`

**Example Drivers:**
- `drivers/media/platform/imx7-media-csi.c` (i.MX7 CSI)
- `drivers/media/platform/rcar-vin/` (Renesas R-Car)
- `drivers/media/platform/ti-vpe/cal.c` (TI CAL)
- `drivers/media/i2c/ov5640.c` (OV5640 sensor)

**User Space:**
- `v4l-utils` source code
- `media-ctl` implementation
- `libcamera` project

---

## 📝 **Complete Example Reference**

**Minimal working example:**

```c
/* sensor.c */
static int sensor_probe(struct i2c_client *client)
{
    struct v4l2_subdev *sd = devm_kzalloc(...);
    
    v4l2_i2c_subdev_init(sd, client, &sensor_ops);
    sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
    
    sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
    media_entity_pads_init(&sd->entity, 1, &sensor->pad);
    
    return v4l2_async_register_subdev(sd);
}

/* bridge.c */
static int bridge_probe(struct platform_device *pdev)
{
    /* Init media + v4l2 device */
    media_device_init(&bridge->mdev);
    bridge->v4l2_dev.mdev = &bridge->mdev;
    v4l2_device_register(dev, &bridge->v4l2_dev);
    
    /* Parse DT + register notifier */
    bridge_parse_dt(bridge);
    bridge->notifier.ops = &bridge_notifier_ops;
    v4l2_async_notifier_register(&bridge->v4l2_dev, &bridge->notifier);
    
    return 0;
}

static int bridge_complete(struct v4l2_async_notifier *notifier)
{
    /* Init video device */
    video_register_device(bridge->vdev, ...);
    
    /* Create links */
    media_create_pad_link(&sensor->entity, 0, &vdev->entity, 0, ...);
    
    /* Register media device */
    return media_device_register(&bridge->mdev);
}
```

---

## 🎓 **Quiz Yourself**

1. **Khi nào cần V4L2 Async?**
   - Answer: Device Tree probing, unordered sub-device registration

2. **Sự khác biệt giữa .bound và .complete?**
   - Answer: .bound gọi khi 1 subdev match, .complete khi TẤT CẢ match

3. **Pad có thể vừa SINK vừa SOURCE?**
   - Answer: KHÔNG, phải chọn 1 trong 2

4. **Khi nào đăng ký media device?**
   - Answer: Trong .complete callback, sau khi entities ready

5. **Link và Backlink khác nhau thế nào?**
   - Answer: Link thuộc source entity, backlink thuộc sink entity

---

## 🚀 **Next Steps**

Sau Chapter 8, bạn có thể:
1. **Implement driver thực tế**: Pick một sensor + SoC
2. **Study existing drivers**: i.MX7, R-Car, TI CAL
3. **User space development**: Sử dụng libcamera
4. **Advanced topics**: 
   - Image processing pipelines
   - Camera sensor calibration
   - ISP tuning

---

**© 2025 - Chapter 8 Complete: V4L2 Async & Media Controller Framework**

---

## 📎 **Appendix: Quick Reference**

**V4L2 Async APIs:**
```c
v4l2_async_notifier_register()
v4l2_async_register_subdev()
v4l2_async_notifier_unregister()
v4l2_async_unregister_subdev()
```

**Media Controller APIs:**
```c
media_device_init()
media_device_register()
media_entity_pads_init()
media_create_pad_link()
media_pipeline_start()
media_pipeline_stop()
```

**Fwnode Graph APIs:**
```c
fwnode_graph_get_next_endpoint()
fwnode_graph_get_remote_port_parent()
fwnode_graph_get_remote_endpoint()
v4l2_fwnode_endpoint_parse()
```

**Command Line:**
```bash
# Topology
media-ctl -p

# Links
media-ctl -l "'source':0 -> 'sink':0[1]"

# Format
media-ctl -V "'entity':0[fmt:CODE/WIDTHxHEIGHT]"

# Graph
media-ctl --print-dot > graph.dot
dot -Tpng graph.dot > graph.png
```

---

**END OF CHAPTER 8 - PART 2**