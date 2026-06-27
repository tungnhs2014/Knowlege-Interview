# 33 - V4L2 Async, Subdevs, And Media Controller

## Learning Goal

After this chapter, you should be able to explain how a modern V4L2 camera pipeline is discovered, connected, configured, and debugged when the hardware is made from multiple blocks instead of one simple capture device.

By the end, you should be able to:

- Explain why **V4L2 async** exists and what problem it solves.
- Distinguish a **bridge driver**, a **sub-device driver**, and a **media controller graph**.
- Explain `v4l2_subdev`, async notifier callbacks, firmware graph endpoints, media entities, pads, and links.
- Walk through bridge probe, sensor probe, async match, media-device registration, and streaming setup.
- Use `media-ctl` output to reason about topology, links, and pad formats.
- Recognize old V4L2 async API names in legacy code and prefer current API names in new examples.
- Debug missing `/dev/videoX`, missing `/dev/mediaX`, broken links, incompatible pad formats, and stuck streaming.

## Why This Matters In Real Work

Embedded camera systems are rarely just "one camera chip." A realistic board may contain an I2C sensor, a MIPI CSI-2 receiver, a mux, a CSI capture block, an ISP, a scaler, and a DMA engine. The kernel needs a way to discover those pieces, wait until they are all ready, expose their topology to userspace, and start streaming in the right order.

This topic solves three common real-driver problems:

- **Probe order problem:** the bridge may probe before the sensor, or the sensor may probe before the bridge.
- **Pipeline topology problem:** userspace and the kernel need to know which block feeds which block.
- **Format/routing problem:** each hardware block may have pads with their own media-bus format, width, height, and active route.

Common systems:

| System | What topic 33 explains |
| --- | --- |
| Sensor over I2C plus SoC CSI | Async bridge-to-sensor binding and sensor sub-device ops. |
| MIPI CSI-2 receiver plus mux plus capture | Media entities, pads, links, and pad formats. |
| ISP pipeline | Multiple sub-devices, per-pad negotiation, and link validation. |
| Camera board using Device Tree graph | `port`, `endpoint`, `remote-endpoint`, fwnode parsing. |
| MC-centric camera stack | `/dev/mediaX` topology setup before `/dev/videoX` streaming. |

**Production rule:** do not publish a usable capture node or start DMA until the required upstream sub-devices, links, formats, clocks, regulators, and stream state are coherent.

## Mental Model

Think of a camera driver as a train route. The sensor is the first station, the CSI receiver and ISP are middle stations, and `/dev/videoX` is where frames arrive in memory. V4L2 async finds the stations even if they appear in random order. Media controller describes the track map. Sub-device ops start, stop, and configure each station.

```text
Device Tree / ACPI graph
  port -> endpoint -> remote-endpoint
        |
        v
V4L2 async
  "bridge expects this remote sensor"
  "sensor is ready"
  async core matches them
        |
        v
Media controller
  /dev/media0
  entities -> pads -> links
        |
        v
V4L2 capture
  /dev/video0
  vb2 buffers and DMA
```

The important split:

- **V4L2 async** answers: "when are all required pipeline parts present?"
- **V4L2 sub-devices** answer: "how does the bridge talk to sensors, receivers, muxes, ISPs, and scalers?"
- **Media controller** answers: "what is connected to what, and which path/format is active?"

## Core Concepts

Sub-devices, async notifiers, and media controller are separate ideas that often appear together in camera drivers.

| Concept | Simple meaning | Main object |
| --- | --- | --- |
| Bridge driver | Final capture interface, often owns DMA and `/dev/videoX`. | `video_device`, `vb2_queue` |
| Sub-device driver | Pipeline component such as sensor, receiver, mux, ISP, scaler. | `v4l2_subdev` |
| Async notifier | Bridge-side waiting list for expected sub-devices. | `v4l2_async_notifier` |
| Async connection | Description of a sub-device the bridge wants to match. | `v4l2_async_connection` in current kernels |
| Firmware graph | Hardware connectivity described by firmware. | `port`, `endpoint`, `remote-endpoint` |
| Media device | Top-level media graph exposed as `/dev/mediaX`. | `media_device` |
| Entity | One graph node, often a sensor, CSI block, mux, or video node. | `media_entity` |
| Pad | One input or output endpoint on an entity. | `media_pad` |
| Link | Directed connection from source pad to sink pad. | `media_link` |

### Bridge Driver Versus Sub-Device Driver

A bridge driver is responsible for the capture node and usually DMA. A sub-device driver is responsible for a block inside the video pipeline.

| Question | Bridge driver | Sub-device driver |
| --- | --- | --- |
| Typical node | `/dev/videoX` | Usually kernel-only; optional `/dev/v4l-subdevX` |
| Owns final vb2 queue? | Yes | Usually no |
| Programs memory DMA? | Usually yes | Usually no |
| Talks to sensor registers? | Calls subdev ops | Usually yes |
| Example | CSI capture, HDMI receiver, USB capture interface | OV5640, IMX219, MIPI CSI-2 receiver, ISP, mux |

**Interview trap:** a camera sensor sub-device is not the same thing as a V4L2 video capture node. The sensor may produce pixels, but the bridge/capture node usually moves those pixels into userspace buffers.

### When To Use Async And Media Controller

Use V4L2 async when probe order is not under the bridge driver's control.

| Situation | Async? | Media controller? |
| --- | --- | --- |
| Sensor described in Device Tree and connected to CSI | Usually yes | Often yes |
| External I2C sensor reused across boards | Usually yes | Often yes |
| USB webcam with one integrated driver | Usually no | Usually no |
| Fixed PCI capture card with internal blocks known by bridge | Maybe no | Maybe yes if topology matters |
| Multi-input mux or ISP route selected at runtime | Often yes | Yes |
| Simple one-to-one sensor -> capture path | Often yes | May still expose MC in MC-centric drivers |

**Practical wording:** "simple pipelines may not need media controller" is not the same as "simple pipelines must not use media controller." Many modern camera stacks expose media controller consistently.

## Kernel Mechanism

The kernel mechanism has two lifecycles running toward each other: sub-devices register themselves when their own resources are ready, while the bridge registers an async notifier describing which remote sub-devices it needs.

### Firmware Graph Binding

Firmware describes physical connectivity, not driver policy.

```dts
csi: csi@12340000 {
        compatible = "vendor,my-csi";

        port {
                csi_in: endpoint {
                        remote-endpoint = <&sensor_out>;
                        data-lanes = <1 2>;
                };
        };
};

&i2c1 {
        sensor@36 {
                compatible = "vendor,my-sensor";
                reg = <0x36>;

                port {
                        sensor_out: endpoint {
                                remote-endpoint = <&csi_in>;
                                link-frequencies = /bits/ 64 <456000000>;
                        };
                };
        };
};
```

The bridge parses its own endpoint, follows `remote-endpoint`, and discovers the remote sensor node.

Key ideas:

- `port` represents an interface on a device.
- `endpoint` represents one end of a hardware connection.
- `remote-endpoint` links two endpoints together.
- V4L2-specific endpoint properties describe the media bus, such as CSI-2 lanes or parallel sync polarity.
- New drivers should prefer **fwnode** APIs over OF-only APIs so Device Tree and ACPI can share code.

### Async Matching

Async matching compares a bridge-side expected connection with a real `v4l2_subdev` registered by a sub-device driver.

```text
Sensor probes first:
  sensor -> v4l2_async_register_subdev()
  async core has no matching bridge yet
  sensor waits in global async state

Bridge probes later:
  bridge -> parse firmware graph
  bridge -> add async connection for remote sensor
  bridge -> register notifier
  async core finds waiting sensor
  async core calls .bound()
  if all required connections are bound:
      async core calls root .complete()
```

```text
Bridge probes first:
  bridge -> register notifier with expected sensor
  async core waits

Sensor probes later:
  sensor -> v4l2_async_register_subdev()
  async core matches sensor to bridge connection
  async core calls .bound()
  if complete:
      async core calls root .complete()
```

The bridge's `.complete()` callback is the usual place to finish publishing a usable camera device:

- Create `/dev/v4l-subdevX` nodes if intended.
- Register `/dev/videoX` after required sub-devices are ready.
- Register `/dev/mediaX` after entities and links are coherent.
- Avoid making userspace race with a half-built pipeline.

### Media Controller Graph

Media controller represents the hardware as an oriented graph.

```text
entity: ov2680 sensor
  pad0: SOURCE
        |
        | enabled link
        v
entity: imx7-mipi-csis
  pad0: SINK
  pad1: SOURCE
        |
        v
entity: csi_mux
  pad1: SINK
  pad2: SOURCE
        |
        v
entity: csi capture
  pad0: SINK
  /dev/video0
```

Graph objects:

| Object | Meaning | Rule |
| --- | --- | --- |
| `media_device` | Whole media topology, exposed as `/dev/mediaX`. | Register after graph is ready. |
| `media_entity` | Hardware/software block in the graph. | Give it a useful `function`. |
| `media_pad` | Input or output endpoint on an entity. | A pad is source or sink, not both. |
| `media_link` | Directed source-to-sink connection. | Link direction follows data flow. |

Link flags:

| Flag | Meaning |
| --- | --- |
| `MEDIA_LNK_FL_ENABLED` | Link is active. |
| `MEDIA_LNK_FL_IMMUTABLE` | Userspace cannot disable it. |
| `MEDIA_LNK_FL_DYNAMIC` | Driver allows changes during streaming. Use carefully. |

### Pad Formats And Link Validation

In a media graph, the format at the video node is not enough. Each sub-device pad may have its own media-bus format.

Example:

```text
sensor pad0:        SBGGR10_1X10 / 800x600
mipi receiver pad0: SBGGR10_1X10 / 800x600
csi pad0:           SBGGR10_1X10 / 800x600
/dev/video0:        Bayer or converted pixel format in memory
```

Sub-device pad ops handle this negotiation:

- `enum_mbus_code`: list supported media-bus codes.
- `enum_frame_size`: list supported frame sizes.
- `get_fmt`: return current or try pad format.
- `set_fmt`: clamp/validate requested format and store try or active state.
- `link_validate`: reject incompatible source/sink formats before streaming.

**Production rule:** do not start streaming just because `/dev/video0` has a format. Validate the enabled media route and the active formats on every relevant pad.

## Key Structs And APIs

The API surface is large, so learn it in groups rather than as one memorization dump.

### Sub-Device APIs

| Struct/API | Why it matters |
| --- | --- |
| `struct v4l2_subdev` | Kernel object representing a sensor, receiver, mux, ISP, scaler, or similar block. |
| `v4l2_subdev_init()` | Initializes a generic sub-device. |
| `v4l2_i2c_subdev_init()` | Initializes an I2C sub-device and connects I2C client data with the subdev. |
| `v4l2_spi_subdev_init()` | SPI variant. |
| `struct v4l2_subdev_ops` | Top-level callback table. |
| `struct v4l2_subdev_core_ops` | Core operations such as power/log/status-style callbacks. |
| `struct v4l2_subdev_video_ops` | Video operations such as `s_stream`. |
| `struct v4l2_subdev_pad_ops` | Pad format, media-bus, and link validation operations. |
| `struct v4l2_subdev_sensor_ops` | Sensor-specific helpers such as skipped frames/lines in some drivers. |
| `v4l2_subdev_call()` | Safe macro for bridge-to-subdev callback calls. |
| `V4L2_SUBDEV_FL_HAS_DEVNODE` | Requests optional `/dev/v4l-subdevX` exposure when subdev nodes are registered. |

`v4l2_subdev_call()` returns useful errors:

| Return | Meaning |
| --- | --- |
| `0` | Operation succeeded. |
| `-ENODEV` | Sub-device pointer is missing/unavailable. |
| `-ENOIOCTLCMD` | Sub-device does not implement that operation. Often not fatal for optional ops. |
| Other negative errno | Real failure from the sub-device. |

### Async APIs

Modern kernels use newer names than many older books and examples.

| Purpose | Current-kernel names to prefer | Older names you may see |
| --- | --- | --- |
| Async connection object | `struct v4l2_async_connection` | `struct v4l2_async_subdev` |
| Match descriptor | `struct v4l2_async_match_desc` | `match_type` and `match` inside `v4l2_async_subdev` |
| Initialize bridge notifier | `v4l2_async_nf_init()` | manual notifier setup |
| Add remote fwnode connection | `v4l2_async_nf_add_fwnode_remote()` | manual async subdev array |
| Add fwnode/I2C connection | `v4l2_async_nf_add_fwnode()`, `v4l2_async_nf_add_i2c()` | manual match setup |
| Register notifier | `v4l2_async_nf_register()` | `v4l2_async_notifier_register()` |
| Unregister notifier | `v4l2_async_nf_unregister()` | `v4l2_async_notifier_unregister()` |
| Free notifier resources | `v4l2_async_nf_cleanup()` | often omitted in old examples |
| Register sub-device | `v4l2_async_register_subdev()` or sensor helper | same name often still seen |

Important notifier callbacks:

| Callback | When called | Typical use |
| --- | --- | --- |
| `.bound` | One expected connection matched one real subdev. | Save subdev pointer, inspect entity, create driver-private relation. |
| `.complete` | All root-notifier connections are bound. | Register final video/media nodes and make pipeline usable. |
| `.unbind` | A bound subdev is leaving. | Tear down dependent nodes or mark pipeline unusable. |
| `.destroy` | Async connection is about to be freed in current API. | Free driver-specific async connection resources. |

### Fwnode And Media-Bus APIs

| API | Use |
| --- | --- |
| `dev_fwnode(dev)` | Get generic firmware node for DT/ACPI-backed device. |
| `fwnode_graph_get_next_endpoint()` | Iterate graph endpoints. |
| `fwnode_graph_get_remote_endpoint()` | Get the paired endpoint. |
| `fwnode_graph_get_remote_port_parent()` | Get the remote device node, usually the sensor/subdev device. |
| `fwnode_handle_put()` | Drop references returned by fwnode helpers. |
| `struct v4l2_fwnode_endpoint` | V4L2-specific parsed endpoint configuration. |
| `v4l2_fwnode_endpoint_parse()` | Parse bus type, lane, sync, polarity, and link frequency properties. |

Common media bus types:

| Type | Typical hardware |
| --- | --- |
| `V4L2_MBUS_PARALLEL` | Parallel camera bus with HSYNC/VSYNC/PCLK. |
| `V4L2_MBUS_BT656` | Parallel bus with embedded sync. |
| `V4L2_MBUS_CSI2_DPHY` | Common MIPI CSI-2 D-PHY camera link. |
| `V4L2_MBUS_CSI2_CPHY` | MIPI CSI-2 C-PHY camera link. |
| `V4L2_MBUS_CSI1` | Older MIPI CSI-1 link. |

### Media Controller APIs

| Struct/API | Use |
| --- | --- |
| `struct media_device` | Top-level graph object; creates `/dev/mediaX`. |
| `media_device_init()` | Initialize media device fields/lists. |
| `media_device_register()` | Publish `/dev/mediaX`. |
| `media_device_unregister()` | Remove media device node. |
| `media_device_cleanup()` | Release media-device resources. |
| `struct media_entity` | Graph node embedded in `video_device` and `v4l2_subdev`. |
| `struct media_pad` | Source or sink endpoint on an entity. |
| `media_entity_pads_init()` | Attach pad array to an entity. |
| `media_entity_cleanup()` | Release entity resources. |
| `media_create_pad_link()` | Create a directed source-pad to sink-pad link. |
| `media_pipeline_start()` | Mark/check pipeline use at stream start in drivers that use it. |
| `media_pipeline_stop()` | Stop pipeline use tracking. |

Entity functions make `media-ctl -p` readable:

| Function | Use |
| --- | --- |
| `MEDIA_ENT_F_CAM_SENSOR` | Camera sensor. |
| `MEDIA_ENT_F_VID_IF_BRIDGE` | Video interface bridge such as CSI-2 receiver. |
| `MEDIA_ENT_F_PROC_VIDEO_ISP` | ISP block. |
| `MEDIA_ENT_F_PROC_VIDEO_SCALER` | Scaler block. |
| `MEDIA_ENT_F_VID_MUX` | Video mux. |
| `MEDIA_ENT_F_IO_V4L` | V4L2 video node. |

### Controls In This Topic

Controls belong here because sensor drivers often expose exposure, gain, blanking, link frequency, test pattern, and orientation controls.

| API | Use |
| --- | --- |
| `struct v4l2_ctrl_handler` | Owns a group of controls. |
| `v4l2_ctrl_handler_init()` | Initialize control handler. |
| `v4l2_ctrl_new_std()` | Create a standard control. |
| `v4l2_ctrl_new_std_menu()` | Create a menu control. |
| `struct v4l2_ctrl_ops` | Implements set/get/validate callbacks. |
| `v4l2_ctrl_handler_setup()` | Apply defaults to hardware. |
| `v4l2_ctrl_handler_free()` | Free controls. |
| `V4L2_CTRL_FLAG_PRIVATE` | Keep a subdev control private instead of inheriting it into a parent handler. |

## Lifecycle / Data Flow

Bring-up becomes much easier if you can trace the order of object creation and publication.

### Probe And Bind Flow

```text
1. Sensor/sub-device probe
   - allocate private state
   - get regulators, clocks, GPIOs, regmap/I2C resources
   - initialize v4l2_subdev
   - set subdev ops and control handler
   - set entity function and pads
   - parse endpoint if needed
   - register async subdev

2. Bridge probe
   - initialize media_device
   - attach media device to v4l2_device.mdev
   - register v4l2_device
   - parse firmware graph endpoints
   - add async connections for remote subdevices
   - register async notifier

3. Async core match
   - match connection by fwnode/I2C/custom criteria
   - register matched subdev with v4l2_device
   - call .bound()
   - when all required connections are bound, call root .complete()

4. Complete callback
   - initialize/register video device if not done earlier
   - create media links if needed
   - create subdev nodes if intended
   - register media device

5. Userspace configuration
   - inspect topology with media-ctl
   - enable links
   - set pad formats
   - capture through /dev/videoX
```

### Stream Start And Stop Flow

```text
VIDIOC_STREAMON
  -> vb2 start_streaming()
      -> validate media route and formats
      -> media_pipeline_start()
      -> v4l2_subdev_call(sensor, video, s_stream, 1)
      -> enable receiver/CSI/ISP blocks
      -> start DMA

DMA IRQs fill buffers
  -> bridge returns buffers with vb2_buffer_done()

VIDIOC_STREAMOFF
  -> vb2 stop_streaming()
      -> stop DMA
      -> v4l2_subdev_call(sensor, video, s_stream, 0)
      -> media_pipeline_stop()
      -> return all queued buffers
```

**Production rule:** stop in the reverse order of start, and unwind partially-started pipelines on every error path.

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows the relationships and current-style naming direction, but it is not production-ready or guaranteed to compile against every kernel version.

### Sub-Device Skeleton

```c
struct my_sensor {
        struct v4l2_subdev sd;
        struct media_pad pad;
        struct v4l2_ctrl_handler ctrls;
        struct device *dev;
};

static int my_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
        struct my_sensor *sensor = container_of(sd, struct my_sensor, sd);

        if (enable) {
                /* enable clocks/regulators if runtime PM did not already do it */
                /* write sensor mode registers */
                /* exit standby */
        } else {
                /* enter standby */
        }

        return 0;
}

static const struct v4l2_subdev_video_ops my_sensor_video_ops = {
        .s_stream = my_sensor_s_stream,
};

static const struct v4l2_subdev_ops my_sensor_ops = {
        .video = &my_sensor_video_ops,
        /* .pad would provide get_fmt/set_fmt in real media drivers */
};

static int my_sensor_probe(struct i2c_client *client)
{
        struct my_sensor *sensor;
        int ret;

        sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
        if (!sensor)
                return -ENOMEM;

        sensor->dev = &client->dev;

        v4l2_i2c_subdev_init(&sensor->sd, client, &my_sensor_ops);
        sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

        sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
        sensor->pad.flags = MEDIA_PAD_FL_SOURCE;

        ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
        if (ret)
                return ret;

        /* Initialize controls, parse endpoint, detect chip ID, etc. */

        ret = v4l2_async_register_subdev(&sensor->sd);
        if (ret) {
                media_entity_cleanup(&sensor->sd.entity);
                return ret;
        }

        return 0;
}
```

Important lines:

- `v4l2_i2c_subdev_init()` makes the I2C device and `v4l2_subdev` find each other.
- `MEDIA_ENT_F_CAM_SENSOR` makes the topology meaningful.
- `MEDIA_PAD_FL_SOURCE` says the sensor outputs data.
- `v4l2_async_register_subdev()` lets the async core match this sensor with a bridge.
- Real drivers also need controls, runtime PM, endpoint parsing, pad ops, cleanup, and hardware validation.

### Bridge Skeleton

```c
struct my_bridge {
        struct device *dev;
        struct v4l2_device v4l2_dev;
        struct media_device mdev;
        struct v4l2_async_notifier notifier;
        struct v4l2_subdev *sensor;
        struct video_device vdev;
        struct media_pad vdev_pad;
};

static int my_bridge_bound(struct v4l2_async_notifier *notifier,
                           struct v4l2_subdev *sd,
                           struct v4l2_async_connection *asc)
{
        struct my_bridge *bridge =
                container_of(notifier, struct my_bridge, notifier);

        bridge->sensor = sd;
        return 0;
}

static int my_bridge_complete(struct v4l2_async_notifier *notifier)
{
        struct my_bridge *bridge =
                container_of(notifier, struct my_bridge, notifier);
        int ret;

        bridge->vdev_pad.flags = MEDIA_PAD_FL_SINK;
        bridge->vdev.entity.function = MEDIA_ENT_F_IO_V4L;

        ret = media_entity_pads_init(&bridge->vdev.entity, 1,
                                     &bridge->vdev_pad);
        if (ret)
                return ret;

        ret = media_create_pad_link(&bridge->sensor->entity, 0,
                                    &bridge->vdev.entity, 0,
                                    MEDIA_LNK_FL_ENABLED |
                                    MEDIA_LNK_FL_IMMUTABLE);
        if (ret)
                return ret;

        ret = video_register_device(&bridge->vdev, VFL_TYPE_VIDEO, -1);
        if (ret)
                return ret;

        ret = v4l2_device_register_subdev_nodes(&bridge->v4l2_dev);
        if (ret)
                return ret;

        return media_device_register(&bridge->mdev);
}

static const struct v4l2_async_notifier_operations my_bridge_nf_ops = {
        .bound = my_bridge_bound,
        .complete = my_bridge_complete,
};
```

In a real bridge probe, the driver would:

- Initialize `media_device`.
- Set `bridge->v4l2_dev.mdev = &bridge->mdev`.
- Register `v4l2_device`.
- Parse firmware endpoints.
- Use current async helper APIs to add expected remote fwnode connections.
- Register the notifier.
- Initialize `video_device`, ioctl ops, vb2 queue, locks, and release handling.
- Unregister and clean up everything in the correct remove/error path.

### Userspace Topology Setup

```bash
# Inspect graph
media-ctl -d /dev/media0 -p

# Enable a link
media-ctl -d /dev/media0 \
  -l "'my-sensor 1-0036':0 -> 'my-csi-capture':0[1]"

# Set sub-device pad format
media-ctl -d /dev/media0 \
  -V "'my-sensor 1-0036':0[fmt:SBGGR10_1X10/800x600]"

# Capture through the video node after graph setup
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

## Common Bugs And Debugging

Start from the symptom. Camera failures are usually ordering, topology, format, power, or buffer-completion bugs.

| Symptom | Likely causes | What to inspect |
| --- | --- | --- |
| Sensor probes but `/dev/video0` never appears | Async connection did not match; `.complete()` not called; bridge did not register video node. | `dmesg`, async debug, fwnode endpoint references, bridge notifier setup. |
| `/dev/video0` appears but `/dev/media0` is missing | `media_device` not initialized/registered; `v4l2_device.mdev` not set. | Bridge probe, `.complete()`, `media_device_register()` return value. |
| `media-ctl -p` shows no sensor entity | Sensor did not register as subdev; media entity not tied to media device; async not complete. | Sensor probe logs, `v4l2_async_register_subdev()`, entity function/pads. |
| Link cannot be enabled | Wrong source/sink direction; missing pads; driver rejects `link_setup`. | Pad flags, `media_create_pad_link()`, entity ops. |
| `STREAMON` fails with pipe/format error | Pad formats incompatible; link validation fails; route not enabled. | `media-ctl -p`, `media-ctl -V`, `link_validate` logs. |
| `DQBUF` blocks forever | Sensor not streaming; receiver not enabled; DMA IRQ missing; driver never calls `vb2_buffer_done()`. | `s_stream` logs, IRQ counters, vb2 debug, enabled links. |
| Frames are corrupt or wrong colors | Media-bus code mismatch, Bayer order mismatch, lane polarity/timing problem, wrong memory pixel format. | Pad formats, endpoint bus properties, sensor mode table. |
| Remove/unbind crashes | Lifetime cleanup order wrong; userspace node outlives required subdev; missing async unregister/cleanup. | Remove path, `.unbind()`, `media_entity_cleanup()`, `v4l2_subdev_cleanup()`. |

Useful commands:

```bash
# Topology and pad formats
media-ctl -d /dev/media0 -p

# Generate a graph
media-ctl -d /dev/media0 --print-dot > graph.dot
dot -Tpng graph.dot > graph.png

# Show subdev state when subdev node exists
v4l2-ctl -d /dev/v4l-subdev0 --all
v4l2-ctl -d /dev/v4l-subdev0 --get-subdev-fmt pad=0

# Short capture smoke test
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 --verbose

# Kernel logs
dmesg | grep -E 'v4l2|media|async|csi|sensor'
```

Debugging checklist:

- Confirm every endpoint has a matching `remote-endpoint`.
- Confirm the bridge uses fwnode handles and releases references.
- Confirm the sensor calls async subdev registration after its resources are ready.
- Confirm `.bound()` fires for every required sub-device.
- Confirm `.complete()` fires once and publishes nodes in a safe order.
- Confirm entity functions and pad directions make sense in `media-ctl -p`.
- Confirm links are enabled and active pad formats match.
- Confirm `s_stream(1)` is called before DMA expects data.
- Confirm error paths stop the pipeline and return all buffers.

## Production Checklist

Use this before code review or board bring-up.

### Firmware / Binding

- `port`, `endpoint`, and `remote-endpoint` describe real hardware connectivity.
- Every endpoint reference is bidirectional where the binding expects it.
- CSI-2 lanes, clock lane, link frequencies, bus width, sync polarities, and clock mode match the schematic and sensor mode.
- Binding uses current YAML documentation style where applicable.
- Graph binding is not used as a dumping ground for driver policy.

### Sub-Device Driver

- Initializes `v4l2_subdev` with bus helper where appropriate.
- Sets meaningful `entity.function`.
- Initializes pads before async registration.
- Implements pad format ops if userspace or bridge needs pad negotiation.
- Initializes controls and frees them on failure/remove.
- Uses runtime PM, regulators, clocks, resets, and GPIOs in a safe order.
- Calls `v4l2_async_register_subdev()` only after the device is usable enough to bind.
- Calls `v4l2_async_unregister_subdev()` and entity/subdev cleanup on remove.

### Bridge Driver

- Initializes `media_device` and assigns `v4l2_device.mdev`.
- Registers `v4l2_device` before async notifier registration.
- Parses firmware graph with fwnode APIs for new code.
- Adds expected async connections with current helper APIs.
- Cleans notifier resources with `v4l2_async_nf_cleanup()` when current APIs allocate connections.
- Publishes `/dev/videoX` and `/dev/mediaX` only when required pipeline pieces are ready.
- Handles `.unbind()` by unregistering or disabling dependent userspace nodes.
- Validates links and pad formats before streaming.

### Streaming

- Starts upstream sub-devices and receiver blocks before relying on DMA.
- Stops hardware in reverse order.
- Handles partial failures with complete unwind.
- Returns all vb2 buffers on stream failure/stop.
- Does not sleep in hard IRQ path.
- Uses appropriate locking for media graph, driver state, queue state, and IRQ buffer lists.

### Userspace ABI

- `media-ctl -p` shows meaningful entity names, functions, pads, links, and formats.
- Optional `/dev/v4l-subdevX` nodes are exposed only when intended.
- Controls are visible at the right level; private sensor controls stay private.
- `v4l2-ctl --stream-mmap` succeeds after media graph setup.
- Topic 34 checks such as `v4l2-compliance` are planned before production sign-off.

## Interview Readiness

You are ready for interviews when you can explain the camera pipeline without hiding behind struct names.

Be able to answer:

- Why async exists and what breaks without it.
- What `.bound()`, `.complete()`, and `.unbind()` mean.
- How a sensor sub-device differs from a capture bridge.
- How Device Tree endpoints become async matches and media links.
- How `/dev/mediaX`, `/dev/videoX`, and `/dev/v4l-subdevX` differ.
- Why pad formats and video-node formats are not the same.
- How you would debug "sensor probes, `/dev/video0` exists, but streaming hangs."
- Why old examples may not compile against current kernels without API updates.

See `interview/33-v4l2-async-subdevs-and-media-controller.md` for structured questions and traps.

## Kernel Version Notes

V4L2 async and sub-device APIs are version-sensitive. Many older examples and book-era notes use names such as `struct v4l2_async_subdev`, `V4L2_ASYNC_MATCH_FWNODE`, and `v4l2_async_notifier_register()`.

For current kernels, prefer the newer async notifier API style:

- `struct v4l2_async_connection`
- `struct v4l2_async_match_desc`
- `v4l2_async_nf_init()`
- `v4l2_async_nf_add_fwnode_remote()`
- `v4l2_async_nf_register()`
- `v4l2_async_nf_unregister()`
- `v4l2_async_nf_cleanup()`

Sub-device pad APIs also changed over time. Older code often uses `struct v4l2_subdev_pad_config` and `v4l2_subdev_get_try_format()`. Newer code commonly uses `struct v4l2_subdev_state` and active-state helpers. Always check the headers and `docs.kernel.org` for the target kernel before writing buildable examples.
