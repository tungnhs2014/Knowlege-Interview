# CHAPTER 8: INTEGRATING WITH V4L2 ASYNC AND MEDIA CONTROLLER FRAMEWORKS

## PART 1: V4L2 ASYNC & FWNODE API

---

## 📌 Giới Thiệu Chapter 8

**Vấn đề thực tế:**

Trên SoC hiện đại, video pipeline cực kỳ phức tạp:
- Nhiều IP blocks: Camera Sensor → MIPI CSI-2 → ISP → DMA
- Probing không theo thứ tự (Device Tree-based system)
- Cần routing động giữa các sub-devices

**Giải pháp:**

1. **V4L2 Async Framework**: Giải quyết vấn đề unordered probing
2. **Media Controller Framework**: Quản lý complex pipeline (Part 2)

**Nội dung Part 1:**
- 8.1. V4L2 Async Framework
- 8.2. V4L2 Fwnode API

---

## 8.1. V4L2 Async Framework

### 8.1.1. Vấn đề: Unordered Probing

**Synchronous Registration (cách cũ):**

```
Bridge Driver Probe:
  ├─ v4l2_device_register()
  ├─ Tạo I2C client cho sensor
  ├─ v4l2_device_register_subdev(sensor)  // ĐỒNG BỘ
  └─ video_register_device()
```

**❌ Vấn đề:**
- Bridge probe trước → sensor chưa ready → FAIL
- Sensor probe trước → bridge chưa có → orphan sub-device
- Device Tree probing = **KHÔNG CÓ THỨ TỰ**

**Ví dụ thực tế:**

```
DT Order:      sensor → bridge
Probe Order:   bridge → sensor ❌ (FAIL)
```

---

### 8.1.2. Giải Pháp: Async Registration

**Ý tưởng:**

```
Bridge:  "Tôi CẦN sensor X, notify tôi khi nó ready"
Sensor:  "Tôi đã ready!" → Async Core match → Bridge.bound()
```

**Flow:**

```
┌─────────────┐         ┌─────────────┐
│   Bridge    │         │   Sensor    │
│   Driver    │         │   Driver    │
└──────┬──────┘         └──────┬──────┘
       │                       │
       │ 1. Register notifier  │
       │    (danh sách sensor  │
       │     cần chờ)          │
       ├──────────────────────>│
       │                       │
       │                  2. Register subdev
       │                       │
       │  <────────────────────┤
       │                       │
       │ 3. Async Core Match   │
       │    → .bound() callback│
       │                       │
       │ 4. All matched?       │
       │    → .complete()      │
       │                       │
```

**Khi nào dùng?**
- ✅ Device Tree-based system
- ✅ Multi sub-device pipeline
- ✅ Bridge + off-chip sensors

**Khi nào KHÔNG cần?**
- ❌ USB camera (single device)
- ❌ Simple platform device (fixed probe order)

---

### 8.1.3. struct v4l2_async_notifier

**Định nghĩa:**

```c
struct v4l2_async_notifier {
    const struct v4l2_async_notifier_operations *ops;
    struct v4l2_device *v4l2_dev;              // Parent V4L2 device
    struct v4l2_async_subdev **subdevs;        // Danh sách async sub-devices
    unsigned int num_subdevs;                  // Số lượng sub-devices
    struct list_head waiting;                  // Sub-devices chờ probe
    struct list_head done;                     // Sub-devices đã match
};
```

**Ý nghĩa:**
- `ops`: Callbacks (.bound, .complete, .unbind)
- `v4l2_dev`: V4L2 device của bridge
- `subdevs`: Array các sub-device bridge cần chờ
- `waiting`: List sub-devices đang chờ
- `done`: List sub-devices đã bound

**Ai sử dụng?**
- **Bridge driver** tạo và đăng ký notifier
- **Async core** quản lý matching

---

### 8.1.4. struct v4l2_async_subdev

**Định nghĩa:**

```c
struct v4l2_async_subdev {
    enum v4l2_async_match_type match_type;
    union {
        struct fwnode_handle *fwnode;      // Match by fwnode
        const char *device_name;           // Match by device name
        struct {
            int adapter_id;
            unsigned short address;
        } i2c;                             // Match by I2C addr
        struct {
            bool (*match)(struct device *, 
                         struct v4l2_async_subdev *);
            void *priv;
        } custom;                          // Custom match
    } match;
};
```

**Match Types:**

| Type | Cách match | Khi nào dùng |
|------|-----------|--------------|
| `V4L2_ASYNC_MATCH_FWNODE` | Dùng device tree node | **Recommend** - Device Tree |
| `V4L2_ASYNC_MATCH_DEVNAME` | Dùng device name | Platform device cố định |
| `V4L2_ASYNC_MATCH_I2C` | I2C adapter + address | I2C sensor (ít dùng) |
| `V4L2_ASYNC_MATCH_CUSTOM` | Custom callback | Edge cases |

**Lưu ý:**
- Bridge driver tạo `v4l2_async_subdev`
- Sensor driver KHÔNG biết đến structure này
- Chỉ async core dùng để matching

---

### 8.1.5. Notifier Operations

```c
struct v4l2_async_notifier_operations {
    int (*bound)(struct v4l2_async_notifier *notifier,
                 struct v4l2_subdev *subdev,
                 struct v4l2_async_subdev *asd);
    int (*complete)(struct v4l2_async_notifier *notifier);
    void (*unbind)(struct v4l2_async_notifier *notifier,
                   struct v4l2_subdev *subdev,
                   struct v4l2_async_subdev *asd);
};
```

#### **A. .bound Callback**

**Khi nào gọi?**
- Sub-device vừa probe thành công
- Async core vừa match được async_subdev ↔ subdev

**Làm gì?**
```c
static int bridge_bound(struct v4l2_async_notifier *notifier,
                        struct v4l2_subdev *sd,
                        struct v4l2_async_subdev *asd)
{
    struct bridge_dev *bridge = 
        container_of(notifier, struct bridge_dev, notifier);
    
    /* Optional: Cấu hình sub-device */
    v4l2_subdev_call(sd, core, s_power, 1);
    
    dev_info(bridge->dev, "Sub-device %s bound\n", sd->name);
    return 0;
}
```

**Thường làm:**
- Print debug message
- Optional: Setup sub-device (ít khi cần)

#### **B. .complete Callback**

**Khi nào gọi?**
- TẤT CẢ sub-devices đã match (notifier->waiting = empty)
- **CHỈ gọi cho root notifier** (bridge)

**Làm gì?**
```c
static int bridge_complete(struct v4l2_async_notifier *notifier)
{
    struct bridge_dev *bridge = 
        container_of(notifier, struct bridge_dev, notifier);
    
    /* 1. Tạo /dev/v4l-subdevX nếu cần */
    v4l2_device_register_subdev_nodes(&bridge->v4l2_dev);
    
    /* 2. Đăng ký video device */
    int ret = video_register_device(bridge->vdev, 
                                     VFL_TYPE_GRABBER, -1);
    if (ret) return ret;
    
    /* 3. Đăng ký media device (nếu dùng) */
    return media_device_register(&bridge->mdev);
}
```

**⚠️ Quan trọng:**
- Đây là nơi đăng ký `/dev/videoX`
- Đảm bảo `/dev/videoX` chỉ xuất hiện khi READY

#### **C. .unbind Callback**

**Khi nào gọi?**
- Sub-device bị remove khỏi system

**Làm gì?**
```c
static void bridge_unbind(struct v4l2_async_notifier *notifier,
                          struct v4l2_subdev *sd,
                          struct v4l2_async_subdev *asd)
{
    struct bridge_dev *bridge = 
        container_of(notifier, struct bridge_dev, notifier);
    
    /* Unregister video device nếu sub-device quan trọng */
    video_unregister_device(bridge->vdev);
    
    dev_info(bridge->dev, "Sub-device %s unbound\n", sd->name);
}
```

---

### 8.1.6. Bridge Implementation Example

**Bước 1: Parse Device Tree**

```c
static int bridge_parse_dt(struct bridge_dev *bridge)
{
    struct device *dev = bridge->dev;
    struct fwnode_handle *ep = NULL;
    unsigned int i = 0;
    
    /* Allocate notifier subdevs array */
    bridge->notifier.subdevs = 
        devm_kcalloc(dev, MAX_SENSORS,
                     sizeof(*bridge->notifier.subdevs),
                     GFP_KERNEL);
    
    /* Duyệt các endpoint trong port của bridge */
    while ((ep = fwnode_graph_get_next_endpoint(
                    dev_fwnode(dev), ep))) {
        
        struct v4l2_async_subdev *asd;
        struct fwnode_handle *remote;
        
        /* Lấy fwnode của remote sensor */
        remote = fwnode_graph_get_remote_port_parent(ep);
        if (!remote)
            continue;
        
        /* Tạo async sub-device */
        asd = devm_kzalloc(dev, sizeof(*asd), GFP_KERNEL);
        asd->match_type = V4L2_ASYNC_MATCH_FWNODE;
        asd->match.fwnode = remote;
        
        /* Thêm vào notifier */
        bridge->notifier.subdevs[i++] = asd;
    }
    
    bridge->notifier.num_subdevs = i;
    return 0;
}
```

**Giải thích:**
- `fwnode_graph_get_next_endpoint()`: Lấy endpoint trong bridge node
- `fwnode_graph_get_remote_port_parent()`: Lấy sensor node từ remote-endpoint
- Set `match_type = FWNODE` → async core sẽ so sánh fwnode

**Bước 2: Đăng Ký Notifier**

```c
static int bridge_probe(struct platform_device *pdev)
{
    struct bridge_dev *bridge;
    int ret;
    
    bridge = devm_kzalloc(&pdev->dev, sizeof(*bridge), GFP_KERNEL);
    bridge->dev = &pdev->dev;
    
    /* 1. V4L2 device registration */
    ret = v4l2_device_register(bridge->dev, &bridge->v4l2_dev);
    if (ret) return ret;
    
    /* 2. Parse DT và tạo async sub-devices */
    ret = bridge_parse_dt(bridge);
    if (ret) goto unreg_v4l2;
    
    /* 3. Set notifier callbacks */
    bridge->notifier.ops = &bridge_notifier_ops;
    
    /* 4. Đăng ký notifier */
    ret = v4l2_async_notifier_register(&bridge->v4l2_dev,
                                        &bridge->notifier);
    if (ret)
        goto unreg_v4l2;
    
    return 0;

unreg_v4l2:
    v4l2_device_unregister(&bridge->v4l2_dev);
    return ret;
}
```

**Quan trọng:**
- `v4l2_device_register()` trước
- `v4l2_async_notifier_register()` sau
- **CHƯA** đăng ký video device (chờ .complete)

---

### 8.1.7. Sensor Implementation Example

**Sensor driver ĐƠN GIẢN hơn:**

```c
static int sensor_probe(struct i2c_client *client)
{
    struct v4l2_subdev *sd;
    int ret;
    
    /* 1. Allocate sub-device */
    sd = devm_kzalloc(&client->dev, sizeof(*sd), GFP_KERNEL);
    
    /* 2. Khởi tạo sub-device */
    v4l2_i2c_subdev_init(sd, client, &sensor_subdev_ops);
    
    /* 3. Đăng ký ASYNC sub-device */
    ret = v4l2_async_register_subdev(sd);
    if (ret) {
        dev_err(&client->dev, "Failed to register async subdev\n");
        return ret;
    }
    
    dev_info(&client->dev, "Sensor registered\n");
    return 0;
}

static int sensor_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    
    /* Unregister async sub-device */
    v4l2_async_unregister_subdev(sd);
    
    return 0;
}
```

**Lưu ý:**
- Sensor KHÔNG cần biết về notifier
- Chỉ gọi `v4l2_async_register_subdev()`
- Async core tự động match với bridge

---

### 8.1.8. Device Tree Binding Example

**Bridge Node:**

```dts
csi: csi@1cb4000 {
    compatible = "allwinner,sun8i-v3s-csi";
    reg = <0x01cb4000 0x1000>;
    interrupts = <GIC_SPI 84 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&ccu CLK_BUS_CSI>;
    resets = <&ccu RST_BUS_CSI>;
    
    /* Port cho graph binding */
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
```

**Sensor Node:**

```dts
&i2c1 {
    ov7740: camera@21 {
        compatible = "ovti,ov7740";
        reg = <0x21>;
        clocks = <&ov7740_clk>;
        
        port {
            ov7740_ep: endpoint {
                remote-endpoint = <&csi_ep>;
            };
        };
    };
};
```

**Giải thích:**
- `remote-endpoint`: Link giữa bridge ↔ sensor
- Bridge parse `&ov7740_ep` → lấy parent `&ov7740` → tạo async_subdev
- Sensor probe → async core match fwnode → .bound()

---

## 8.2. V4L2 Fwnode API

### 8.2.1. Firmware Node Abstraction

**Vấn đề:**
- Device Tree: `struct device_node`
- ACPI: `struct acpi_device`
- Cần API thống nhất!

**Giải pháp: struct fwnode_handle**

```
┌─────────────────────────┐
│  struct fwnode_handle   │  ← Generic abstraction
└────────┬────────────────┘
         │
    ┌────┴─────┐
    │          │
┌───▼────┐  ┌─▼──────────┐
│  DT    │  │   ACPI     │
│ Node   │  │  Device    │
└────────┘  └────────────┘
```

**Code:**

```c
/* DT node chứa fwnode */
struct device_node {
    struct fwnode_handle fwnode;
    // ...
};

/* ACPI device chứa fwnode */
struct acpi_device {
    struct fwnode_handle fwnode;
    // ...
};

/* Generic API */
struct fwnode_handle *fwnode = dev_fwnode(dev);
```

**Chuyển đổi:**

```c
/* OF → fwnode */
struct fwnode_handle *fwnode = of_fwnode_handle(of_node);

/* fwnode → OF */
struct device_node *of_node = to_of_node(fwnode);
```

---

### 8.2.2. Graph Binding Concepts

**Mô hình:**

```
Device
  └─ Port (interface)
      └─ Endpoint (connection point)
          └─ remote-endpoint → Endpoint khác
```

**Ví dụ thực tế:**

```
CSI Controller (1 port, 1 endpoint):
  port
    └─ endpoint → sensor endpoint

Camera Sensor (1 port, 1 endpoint):
  port
    └─ endpoint → CSI endpoint
```

**Nhiều endpoints:**

```dts
device {
    #address-cells = <1>;
    #size-cells = <0>;
    
    port@0 {
        reg = <0>;
        #address-cells = <1>;
        #size-cells = <0>;
        
        endpoint@0 {
            reg = <0>;
            remote-endpoint = <&device2_ep0>;
        };
        
        endpoint@1 {
            reg = <1>;
            remote-endpoint = <&device2_ep1>;
        };
    };
    
    port@1 {
        reg = <1>;
        endpoint {
            remote-endpoint = <&device3_ep>;
        };
    };
};
```

---

### 8.2.3. Media Bus Types

**Các loại bus:**

| Bus Type | Description | Use Case |
|----------|-------------|----------|
| `V4L2_MBUS_PARALLEL` | Parallel bus (HSYNC, VSYNC, PCLK) | Old cameras |
| `V4L2_MBUS_BT656` | BT.656/BT.1120 (embedded sync) | Professional video |
| `V4L2_MBUS_CSI2_DPHY` | MIPI CSI-2 D-PHY | **Modern cameras** |
| `V4L2_MBUS_CSI2_CPHY` | MIPI CSI-2 C-PHY | High-speed cameras |
| `V4L2_MBUS_CSI1` | MIPI CSI-1 | Legacy MIPI |

**Parallel Bus Properties:**

```dts
endpoint {
    remote-endpoint = <&sensor>;
    bus-width = <8>;              // 8-bit data
    hsync-active = <1>;           // HSYNC active high
    vsync-active = <1>;           // VSYNC active high
    pclk-sample = <1>;            // Sample on rising edge
    data-shift = <2>;             // Skip first 2 data lines
};
```

**MIPI CSI-2 Properties:**

```dts
endpoint {
    remote-endpoint = <&csi>;
    data-lanes = <1 2 3 4>;      // 4 data lanes
    clock-lanes = <0>;            // Clock lane 0
    clock-noncontinuous;          // Non-continuous clock
    link-frequencies = /bits/ 64 <456000000>;
};
```

---

### 8.2.4. struct v4l2_fwnode_endpoint

**Định nghĩa:**

```c
struct v4l2_fwnode_endpoint {
    struct fwnode_endpoint base;           // Generic endpoint
    enum v4l2_mbus_type bus_type;          // Bus type
    union {
        struct v4l2_fwnode_bus_parallel parallel;
        struct v4l2_fwnode_bus_mipi_csi1 mipi_csi1;
        struct v4l2_fwnode_bus_mipi_csi2 mipi_csi2;
    } bus;                                 // Bus-specific config
    u64 *link_frequencies;                 // Supported frequencies
    unsigned int nr_of_link_frequencies;
};
```

**Parsing endpoint:**

```c
int parse_endpoint(struct device *dev)
{
    struct v4l2_fwnode_endpoint bus_cfg = { 0 };
    struct fwnode_handle *ep;
    int ret;
    
    /* Lấy endpoint */
    ep = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
    if (!ep)
        return -EINVAL;
    
    /* Parse endpoint properties */
    ret = v4l2_fwnode_endpoint_parse(ep, &bus_cfg);
    if (ret) {
        dev_err(dev, "Failed to parse endpoint\n");
        goto err_put;
    }
    
    /* Kiểm tra bus type */
    if (bus_cfg.bus_type == V4L2_MBUS_CSI2_DPHY) {
        dev_info(dev, "MIPI CSI-2 D-PHY detected\n");
        dev_info(dev, "Data lanes: %d\n", 
                 bus_cfg.bus.mipi_csi2.num_data_lanes);
    }
    
err_put:
    fwnode_handle_put(ep);
    return ret;
}
```

---

### 8.2.5. Parsing Device Tree

**Common fwnode APIs:**

```c
/* Get parent node */
struct fwnode_handle *parent = fwnode_get_parent(fwnode);

/* Get next child */
struct fwnode_handle *child = 
    fwnode_get_next_child_node(parent, NULL);

/* Iterate children */
struct fwnode_handle *child;
fwnode_for_each_child_node(parent, child) {
    /* Process child */
}

/* Read property */
u32 value;
fwnode_property_read_u32(fwnode, "clock-frequency", &value);

/* Check property exists */
if (fwnode_property_present(fwnode, "hsync-active")) {
    /* Property exists */
}
```

**Graph APIs:**

```c
/* Get next endpoint in node */
struct fwnode_handle *ep = 
    fwnode_graph_get_next_endpoint(fwnode, NULL);

/* Get remote endpoint */
struct fwnode_handle *remote_ep = 
    fwnode_graph_get_remote_endpoint(ep);

/* Get remote port parent (device node) */
struct fwnode_handle *remote_dev = 
    fwnode_graph_get_remote_port_parent(ep);

/* Iterate all endpoints */
struct fwnode_handle *ep;
fwnode_graph_for_each_endpoint(fwnode, ep) {
    /* Process endpoint */
}
```

**Example: Parse bridge endpoints**

```c
static int bridge_parse_endpoints(struct bridge_dev *bridge)
{
    struct fwnode_handle *ep = NULL;
    struct device *dev = bridge->dev;
    int count = 0;
    
    /* Duyệt tất cả endpoints */
    fwnode_graph_for_each_endpoint(dev_fwnode(dev), ep) {
        struct v4l2_fwnode_endpoint v4l2_ep = { 0 };
        struct fwnode_handle *remote;
        int ret;
        
        /* Parse V4L2 endpoint properties */
        ret = v4l2_fwnode_endpoint_parse(ep, &v4l2_ep);
        if (ret) {
            dev_warn(dev, "Failed to parse endpoint %d\n", count);
            continue;
        }
        
        /* Lấy remote sensor node */
        remote = fwnode_graph_get_remote_port_parent(ep);
        if (!remote) {
            dev_warn(dev, "No remote device for endpoint %d\n", count);
            continue;
        }
        
        dev_info(dev, "Found sensor at endpoint %d\n", count);
        dev_info(dev, "  Bus type: %d\n", v4l2_ep.bus_type);
        
        fwnode_handle_put(remote);
        count++;
    }
    
    return count;
}
```

---

## 8.3. Tóm Tắt Part 1

### ✅ **V4L2 Async Framework:**

**Vấn đề giải quyết:**
- Unordered probing trong Device Tree
- Bridge cần chờ sub-devices

**Key concepts:**
- `v4l2_async_notifier`: Bridge tạo, chứa danh sách sub-devices cần chờ
- `v4l2_async_subdev`: Abstraction của sub-device trong async core
- `.bound()`: Sub-device vừa match
- `.complete()`: Tất cả sub-devices đã ready → đăng ký `/dev/videoX`

**Flow:**
```
Bridge probe → Parse DT → Create async_subdevs → Register notifier
                                                          ↓
Sensor probe → Register async subdev ──────────→ Async core match
                                                          ↓
                                                    .bound() callback
                                                          ↓
                                             All matched? → .complete()
```

### ✅ **V4L2 Fwnode API:**

**Generic abstraction:**
- `fwnode_handle`: Thống nhất DT + ACPI
- Graph binding: port → endpoint → remote-endpoint

**Bus types:**
- Parallel, BT656, MIPI CSI-2 D-PHY/C-PHY
- `v4l2_fwnode_endpoint_parse()`: Parse bus properties

**Use cases:**
- Parse endpoint trong bridge/sensor
- Validate bus compatibility

---

## 📝 **Checklist Part 1:**

- ✅ Giải thích WHY cần async (unordered probing)
- ✅ Phân biệt sync vs async registration
- ✅ Notifier operations (.bound, .complete, .unbind)
- ✅ Code examples bridge + sensor
- ✅ Device Tree binding
- ✅ Fwnode abstraction (DT + ACPI)
- ✅ Graph binding concepts (port, endpoint)
- ✅ Media bus types
- ✅ Parsing APIs

---

## 🚀 **Tiếp Theo: Part 2**

Part 2 sẽ cover:
- 8.3. Media Controller Framework
- 8.4. Integration Example (Bridge + Media Device)
- 8.5. User Space Tools (media-ctl)
- 8.6. [ADVANCED] Topics

---

**📖 References:**
- `Documentation/devicetree/bindings/media/video-interfaces.txt`
- `Documentation/driver-api/media/v4l2-async.rst`
- `include/media/v4l2-async.h`
- `include/media/v4l2-fwnode.h`

---

**© 2025 - Chapter 8 Part 1: V4L2 Async & Fwnode API**