# 33 - V4L2 Async, Subdevs, And Media Controller Example

This is a **learning-only** V4L2 async/sub-device/media-controller example. It
does not ship a buildable kernel module because a useful camera pipeline depends
on real board wiring, sensor registers, clocks, regulators, resets, pinctrl,
MIPI CSI-2 receiver behavior, vb2/DMA integration, and target kernel headers.

The goal is to learn the shape of the integration:

```text
Device Tree graph
  -> bridge parses local endpoint
  -> bridge adds expected remote sensor to async notifier
  -> sensor registers itself as a v4l2_subdev
  -> V4L2 async core matches bridge connection to sensor subdev
  -> bridge creates media entities, pads, links, and video node
  -> userspace configures /dev/mediaX and streams from /dev/videoX
```

Use this README as a lab checklist and code map before reading or writing a
real camera bridge/sensor driver.

## Goal

This example demonstrates:

- how `port`, `endpoint`, and `remote-endpoint` describe camera wiring;
- how a bridge driver waits for a sensor with `struct v4l2_async_notifier`;
- how a sensor registers as `struct v4l2_subdev`;
- how media controller objects model the runtime graph;
- where `/dev/mediaX`, `/dev/videoX`, and `/dev/v4l-subdevX` fit;
- how `media-ctl` configures links and pad formats before capture;
- what cleanup and error paths a real driver must not forget.

## Kernel Version Assumptions

Validate V4L2 async and sub-device APIs against the exact target kernel:

```sh
uname -r
ls /lib/modules/$(uname -r)/build/include/media
```

This example uses current-style async names:

- `struct v4l2_async_connection`;
- `struct v4l2_async_match_desc`;
- `v4l2_async_nf_init()`;
- `v4l2_async_nf_add_fwnode_remote()`;
- `v4l2_async_nf_register()`;
- `v4l2_async_nf_unregister()`;
- `v4l2_async_nf_cleanup()`;
- `v4l2_async_register_subdev()` or `v4l2_async_register_subdev_sensor()`;
- `v4l2_async_unregister_subdev()`.

Older examples and older books may use names such as
`struct v4l2_async_subdev`, `V4L2_ASYNC_MATCH_FWNODE`, and
`v4l2_async_notifier_register()`. Treat those as legacy until the target kernel
headers prove otherwise.

Also validate sub-device pad API signatures. Older code may use
`struct v4l2_subdev_pad_config`, while newer kernels use
`struct v4l2_subdev_state` and state helpers.

## Files

| File | Purpose |
| --- | --- |
| `README.md` | Learning-only DTS graph, async bridge flow, sensor subdev flow, media-ctl commands, debug notes, and cleanup rules. |

There is no `Makefile` because this directory intentionally does not contain a
build-tested module.

## Build

No local build is required.

For a real module, build against the target kernel tree:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Before converting the skeletons below into source, check:

- async notifier helper prototypes in `include/media/v4l2-async.h`;
- subdev operation prototypes in `include/media/v4l2-subdev.h`;
- media entity helpers in `include/media/media-entity.h`;
- video node/vb2 integration from topic 32;
- the real sensor and bridge bindings under kernel Devicetree media bindings.

## Load And Unload

There is no local `.ko` to load from this directory.

On a real target, the load step is board-specific and normally comes from one
of these paths:

```sh
# Built-in drivers and a DTB selected by the bootloader.
dmesg | grep -Ei 'camera|sensor|csi|v4l2|media|async'

# Or load target-specific modules if the drivers are modular.
sudo modprobe my_sensor
sudo modprobe my_csi_bridge
dmesg | tail -80
```

Expected probe log shape from a real driver:

```text
my-sensor 1-0036: chip id 0x1234 detected
my-csi-bridge 12340000.csi: parsed endpoint: 2 CSI-2 lanes
my-csi-bridge 12340000.csi: async bound subdev my-sensor 1-0036
my-csi-bridge 12340000.csi: async complete, registering video/media nodes
```

Expected userspace node shape:

```sh
media-ctl --list-devices
v4l2-ctl --list-devices
ls -l /dev/media* /dev/video* /dev/v4l-subdev* 2>/dev/null
```

```text
my-csi-bridge (...)
        /dev/media0
        /dev/video0
        /dev/v4l-subdev0
```

Unload is also board-specific. If the drivers are modular, stop applications
first, then remove the bridge before the sensor:

```sh
fuser -v /dev/video* /dev/media* /dev/v4l-subdev* 2>/dev/null
sudo modprobe -r my_csi_bridge
sudo modprobe -r my_sensor
dmesg | tail -80
```

Expected unload log shape:

```text
my-csi-bridge 12340000.csi: unregistering media/video nodes
my-csi-bridge 12340000.csi: async notifier cleanup
my-sensor 1-0036: unregistering subdev
```

## Device Tree Graph Shape

This DTS fragment is **illustrative**. Real bindings require the compatible
strings, clocks, regulators, resets, pinctrl states, supplies, GPIO polarity,
lane mapping, and link frequencies documented for your hardware.

```dts
i2c1 {
        #address-cells = <1>;
        #size-cells = <0>;

        camera_sensor: camera@36 {
                compatible = "vendor,my-sensor";
                reg = <0x36>;

                clocks = <&cam_xclk>;
                reset-gpios = <&gpio1 5 GPIO_ACTIVE_LOW>;

                port {
                        sensor_out: endpoint {
                                remote-endpoint = <&csi_in>;
                                bus-type = <4>; /* CSI-2 D-PHY */
                                data-lanes = <1 2>;
                                clock-lanes = <0>;
                                link-frequencies = /bits/ 64 <456000000>;
                        };
                };
        };
};

csi_bridge: csi@12340000 {
        compatible = "vendor,my-csi-bridge";
        reg = <0x12340000 0x1000>;
        interrupts = <42>;

        port {
                csi_in: endpoint {
                        remote-endpoint = <&sensor_out>;
                        bus-type = <4>; /* CSI-2 D-PHY */
                        data-lanes = <1 2>;
                        clock-lanes = <0>;
                };
        };
};
```

Mental map:

```text
camera_sensor endpoint sensor_out  ---- remote-endpoint ----  csi_bridge endpoint csi_in
sensor media source pad             ---- media link ---------  bridge/video sink pad
```

## Sensor Sub-Device Skeleton

This is **pseudo-code**. It shows object ownership and lifecycle, not a complete
sensor driver.

```c
struct my_sensor {
        struct device *dev;
        struct v4l2_subdev sd;
        struct media_pad pad;
        struct v4l2_ctrl_handler ctrls;
};

static int my_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
        struct my_sensor *sensor = container_of(sd, struct my_sensor, sd);

        if (enable) {
                /* Runtime PM get, program mode registers, exit standby. */
        } else {
                /* Enter standby, then runtime PM put. */
        }

        return 0;
}

static const struct v4l2_subdev_video_ops my_sensor_video_ops = {
        .s_stream = my_sensor_s_stream,
};

static const struct v4l2_subdev_ops my_sensor_ops = {
        .video = &my_sensor_video_ops,
        /* Real drivers also provide pad get_fmt/set_fmt helpers. */
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

        /* Real driver: controls, endpoint parsing, chip ID, PM setup. */

        ret = v4l2_async_register_subdev(&sensor->sd);
        if (ret)
                goto err_entity;

        return 0;

err_entity:
        media_entity_cleanup(&sensor->sd.entity);
        return ret;
}

static void my_sensor_remove(struct i2c_client *client)
{
        struct v4l2_subdev *sd = i2c_get_clientdata(client);
        struct my_sensor *sensor = container_of(sd, struct my_sensor, sd);

        v4l2_async_unregister_subdev(&sensor->sd);
        media_entity_cleanup(&sensor->sd.entity);
        v4l2_ctrl_handler_free(&sensor->ctrls);
}
```

Production sensor drivers add:

- runtime PM, regulators, clocks, resets, and pinctrl;
- mode tables and register writes;
- `v4l2_ctrl_handler` setup for exposure, gain, blanking, test pattern, and link frequency;
- pad format state for TRY and ACTIVE formats;
- endpoint parsing with `v4l2_fwnode_endpoint_parse()`;
- `v4l2_subdev_cleanup()` when modern subdev state/endpoints require it.

## Bridge Async Skeleton

The bridge owns the capture side: `struct v4l2_device`, `struct media_device`,
`struct video_device`, vb2/DMA queues, and the async notifier.

```c
struct my_bridge_async {
        struct v4l2_async_connection asc;
        /* Driver-specific fields may follow. asc must be first. */
};

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

static void my_bridge_unbind(struct v4l2_async_notifier *notifier,
                             struct v4l2_subdev *sd,
                             struct v4l2_async_connection *asc)
{
        struct my_bridge *bridge =
                container_of(notifier, struct my_bridge, notifier);

        if (bridge->sensor == sd)
                bridge->sensor = NULL;
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
                goto err_vdev_entity;

        ret = video_register_device(&bridge->vdev, VFL_TYPE_VIDEO, -1);
        if (ret)
                goto err_vdev_entity;

        ret = v4l2_device_register_subdev_nodes(&bridge->v4l2_dev);
        if (ret)
                goto err_video;

        ret = media_device_register(&bridge->mdev);
        if (ret)
                goto err_video;

        return 0;

err_video:
        video_unregister_device(&bridge->vdev);
err_vdev_entity:
        media_entity_cleanup(&bridge->vdev.entity);
        return ret;
}

static const struct v4l2_async_notifier_operations my_bridge_nf_ops = {
        .bound = my_bridge_bound,
        .unbind = my_bridge_unbind,
        .complete = my_bridge_complete,
};
```

Bridge probe flow:

```c
static int my_bridge_probe(struct platform_device *pdev)
{
        struct my_bridge *bridge;
        struct fwnode_handle *ep;
        int ret;

        bridge = devm_kzalloc(&pdev->dev, sizeof(*bridge), GFP_KERNEL);
        if (!bridge)
                return -ENOMEM;

        bridge->dev = &pdev->dev;

        strscpy(bridge->mdev.model, "my-csi-bridge",
                sizeof(bridge->mdev.model));
        bridge->mdev.dev = &pdev->dev;
        media_device_init(&bridge->mdev);

        bridge->v4l2_dev.mdev = &bridge->mdev;
        ret = v4l2_device_register(&pdev->dev, &bridge->v4l2_dev);
        if (ret)
                goto err_mdev;

        v4l2_async_nf_init(&bridge->notifier, &bridge->v4l2_dev);
        bridge->notifier.ops = &my_bridge_nf_ops;

        ep = fwnode_graph_get_next_endpoint(dev_fwnode(&pdev->dev), NULL);
        if (!ep) {
                ret = -ENODEV;
                goto err_v4l2;
        }

        ret = v4l2_async_nf_add_fwnode_remote(&bridge->notifier, ep,
                                              struct my_bridge_async);
        fwnode_handle_put(ep);
        if (ret)
                goto err_nf_cleanup;

        /* Real driver: initialize vb2 queue and video_device before complete. */

        ret = v4l2_async_nf_register(&bridge->notifier);
        if (ret)
                goto err_nf_cleanup;

        platform_set_drvdata(pdev, bridge);
        return 0;

err_nf_cleanup:
        v4l2_async_nf_cleanup(&bridge->notifier);
err_v4l2:
        v4l2_device_unregister(&bridge->v4l2_dev);
err_mdev:
        media_device_cleanup(&bridge->mdev);
        return ret;
}
```

Bridge remove flow:

```c
static void my_bridge_remove(struct platform_device *pdev)
{
        struct my_bridge *bridge = platform_get_drvdata(pdev);

        media_device_unregister(&bridge->mdev);
        video_unregister_device(&bridge->vdev);
        media_entity_cleanup(&bridge->vdev.entity);
        v4l2_async_nf_unregister(&bridge->notifier);
        v4l2_async_nf_cleanup(&bridge->notifier);
        v4l2_device_unregister(&bridge->v4l2_dev);
        media_device_cleanup(&bridge->mdev);
}
```

The exact unregister order depends on what was successfully registered. In real
code, track registration state carefully so an error path does not unregister an
object that was never published.

## Test

Install tools:

```sh
sudo apt-get install v4l-utils graphviz
```

Inspect devices:

```sh
media-ctl --list-devices
v4l2-ctl --list-devices
```

Print the graph:

```sh
MEDIA=/dev/media0
media-ctl -d "$MEDIA" -p
```

Expected topology shape:

```text
- entity 1: my-sensor 1-0036 (1 pad)
    pad0: Source
        -> "my-csi-capture":0 [ENABLED,IMMUTABLE]

- entity 2: my-csi-capture (1 pad, 1 link)
    pad0: Sink
```

Export a visual graph:

```sh
media-ctl -d "$MEDIA" --print-dot > /tmp/camera.dot
dot -Tpng /tmp/camera.dot -o /tmp/camera.png
```

For dynamic links, reset and enable the selected route:

```sh
media-ctl -d "$MEDIA" --reset
media-ctl -d "$MEDIA" \
  -l "'my-sensor 1-0036':0 -> 'my-csi-capture':0[1]"
```

Set the sensor pad format:

```sh
media-ctl -d "$MEDIA" \
  -V "'my-sensor 1-0036':0[fmt:SBGGR10_1X10/1280x720 field:none]"
```

Inspect subdev state if subdev nodes are exposed:

```sh
v4l2-ctl -d /dev/v4l-subdev0 --all
v4l2-ctl -d /dev/v4l-subdev0 --get-subdev-fmt pad=0
```

Stream through the video node after topology setup:

```sh
DEV=/dev/video0
v4l2-ctl -d "$DEV" -D
v4l2-ctl -d "$DEV" --list-formats-ext
v4l2-ctl -d "$DEV" --stream-mmap=3 --stream-count=10
```

Expected stream output shape:

```text
<<<<<<<<<<
```

If the command prints no frame markers or blocks forever, move to the debug
checklist below.

## Command To Driver Map

| Action | Kernel object | Driver responsibility |
| --- | --- | --- |
| Parse DTS graph | `fwnode_handle`, `v4l2_fwnode_endpoint` | Find local endpoints, parse bus properties, release fwnode references. |
| Add async connection | `v4l2_async_notifier` | Add expected remote sensor before registering notifier. |
| Sensor probe | `v4l2_subdev` | Initialize ops, controls, entity function, pads, and async registration. |
| Async `.bound()` | notifier callback | Record the matched sensor and create light per-subdev state. |
| Async `.complete()` | root notifier callback | Publish final media/video/subdev nodes once required subdevs are ready. |
| `media-ctl -p` | `/dev/mediaX` | Show entity, pad, link, and format state. |
| `media-ctl -l/-V` | media links and subdev pads | Enable route and set compatible pad formats. |
| `VIDIOC_STREAMON` | `/dev/videoX` and vb2 | Validate pipeline, start subdev stream, start bridge receiver/DMA. |
| `VIDIOC_STREAMOFF` | `/dev/videoX` and vb2 | Stop DMA, stop subdev stream, return all queued buffers. |

## Debug

If the sensor probes but no `/dev/videoX` appears:

- check that the bridge endpoint has the correct `remote-endpoint`;
- check that the sensor endpoint points back to the bridge endpoint;
- log every endpoint discovered by the bridge;
- confirm the bridge calls `v4l2_async_nf_register()`;
- confirm the sensor calls `v4l2_async_register_subdev()` or the sensor helper;
- check whether `.bound()` runs;
- check whether `.complete()` runs;
- check whether `/dev/videoX` is intentionally delayed until async completion.

If `/dev/mediaX` exists but the graph is wrong:

- confirm `bridge->v4l2_dev.mdev = &bridge->mdev`;
- confirm each entity has a meaningful `entity.function`;
- confirm pads are source-to-sink, not reversed;
- confirm `media_entity_pads_init()` succeeded before link creation;
- inspect the graph with `media-ctl -d "$MEDIA" -p`.

If `/dev/videoX` exists but streaming hangs:

- verify links are enabled;
- verify sensor and bridge pad formats match;
- check sensor `s_stream(1)` and bridge stream start logs;
- check IRQ/DMA completion and `vb2_buffer_done()` from topic 32;
- ensure stream-start error paths unwind `media_pipeline_start()` and subdev streaming;
- ensure stream-stop returns every queued vb2 buffer.

Useful commands:

```sh
dmesg | grep -Ei 'v4l2|media|async|subdev|csi|camera'
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
sudo sh -c 'echo "file drivers/media/* +p" > /sys/kernel/debug/dynamic_debug/control'
media-ctl -d "$MEDIA" -p
v4l2-compliance -d "$DEV"
```

## ABI Impact

A real implementation may expose:

- `/dev/mediaX` for media graph discovery and configuration;
- `/dev/videoX` for capture streaming;
- `/dev/v4l-subdevX` if `V4L2_SUBDEV_FL_HAS_DEVNODE` and subdev node
  registration are enabled;
- controls inherited from subdevices or exposed through subdev nodes.

These are userspace ABI surfaces. Once applications depend on entity names,
pad indexes, controls, formats, frame sizes, link behavior, and errors, changes
can break userspace camera stacks.

## Cleanup And Error Paths

Sensor cleanup normally unwinds:

```text
v4l2_async_unregister_subdev()
  -> v4l2_ctrl_handler_free()
  -> media_entity_cleanup()
  -> v4l2_subdev_cleanup() when required by the target API
  -> disable runtime PM resources
```

Bridge cleanup normally unwinds:

```text
stop streaming users
  -> unregister media/video/subdev nodes
  -> v4l2_async_nf_unregister()
  -> v4l2_async_nf_cleanup()
  -> v4l2_device_unregister()
  -> media_device_cleanup()
```

Production rules:

- release every fwnode reference acquired while parsing graph endpoints;
- call `v4l2_async_nf_cleanup()` after notifier add helpers;
- avoid publishing `/dev/videoX` before required subdevices are available;
- stop the media pipeline in reverse order after partial stream-start failures;
- protect unbind/remove from racing with open files and active streaming;
- return all queued vb2 buffers on stop or failure.

## Why This Is Not Production-Ready

This example is intentionally not a finished driver:

- it does not compile against a selected kernel tree;
- it does not include real sensor register programming;
- it does not implement pad format negotiation;
- it does not manage clocks, regulators, resets, pinctrl, or runtime PM;
- it does not include vb2 queue setup or DMA/IRQ handling;
- it does not define a stable DT binding;
- it does not test unbind/remove while userspace is streaming.

Use it to understand the object model and bring-up sequence. For a product,
start from a real upstream driver with similar hardware and adapt it against the
target kernel and board binding.

## References

- Kernel V4L2 async kAPI:
  <https://docs.kernel.org/driver-api/media/v4l2-async.html>
- Kernel V4L2 sub-device documentation:
  <https://docs.kernel.org/driver-api/media/v4l2-subdev.html>
- Kernel media controller model:
  <https://docs.kernel.org/userspace-api/media/mediactl/media-controller-model.html>
