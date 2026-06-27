# Topic Brief - 33 - V4L2 Async, Subdevs, And Media Controller

## Output Targets
- Knowledge: `knowledge/33-v4l2-async-subdevs-and-media-controller.md`
- Interview: `interview/33-v4l2-async-subdevs-and-media-controller.md`
- Example: `examples/33-v4l2-async-subdevs-and-media-controller/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/no direct V4L2 source found | No V4L2 async, sub-device, media controller, `v4l2_subdev`, `v4l2_async`, `media_entity`, pad/link, or `media-ctl` chapter exists in book 1. Hits for async/fwnode are unrelated SPI, DMA, IRQ-domain, or generic async references. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/covered/merged | Foundation for `v4l2_device`, `video_device`, `v4l2_subdev`, subdev flags, subdev ops, `v4l2_subdev_call()`, sync versus async registration boundary, `/dev/v4l-subdevX`, and V4L2 controls/control inheritance. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/covered/merged | Primary source for this topic: graph binding, fwnode graph APIs, `v4l2_fwnode_endpoint`, V4L2 async notifier flow, bridge/sensor registration, media controller model, `media_device`, entities, pads, links, pad formats, link validation, media-device registration, `media-ctl`, and i.MX examples. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/related-topic | Boundary context for userspace tools: `v4l2-ctl` can query/configure V4L2 devices and subdevices, capture after media topology setup, enable V4L2/vb2 debug, and run `v4l2-compliance`. Defer most content to topic 34. |
| `notion-ch07-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part1.md` | read/mapped/covered/merged | Beginner-oriented mental model for the four core V4L2 structs and bridge-vs-subdev split; useful to explain why subdevices do not own DMA and why `/dev/v4l-subdevX` is optional. |
| `notion-ch07-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/covered/merged | Practical sub-device and controls notes: `v4l2_subdev_ops`, core/video/pad/sensor ops, I2C sensor probe flow, async subdev registration, `v4l2_subdev_call()`, control handler setup, private controls, and inheritance. |
| `notion-ch08-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md` | read/mapped/covered/merged | Clear teaching version of async and fwnode: unordered probing problem, notifier callbacks, bridge/sensor flows, DT endpoint examples, media bus types, and endpoint parsing. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2.md` | read/mapped/covered/merged | Clear teaching version of media controller: entity/pad/link model, bridge integration example, userspace `media-ctl`, topology interpretation, dynamic routing, subdev userspace API, pipeline start/stop, link validation, debugging checklist, and quick reference. |

## Source Files Read
- Required project guidance:
  - `Linux-Device-Driver/CODEX.md`
  - `Linux-Device-Driver/LEARNING_PATH.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
  - `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `ldd1-source-root`: searched with V4L2 async/subdev/media-controller/fwnode/entity/pad/link terms.
  - Result: no direct V4L2 material. `ldd1-ch07`, `ldd1-ch08`, `ldd1-ch12`, `ldd1-ch16`, and other hits are unrelated I2C/SPI/DMA/IRQ/general async references.
- `ldd2-ch07`: relevant sections read.
  - `Framework Architecture and the Main Data Structures`
  - `Initializing and Registering a V4L2 Device`
  - `Introducing Video Device Drivers - The Bridge Driver`
  - `The Concept of Sub-devices`
  - `Sub-device Initialization`
  - `Sub-device Operations`
  - `Calling Sub-device Operations`
  - `Traditional Sub-device (Un)registration`
  - `V4L2 Controls Infrastructure`
  - `A Word About Control Inheritance`
- `ldd2-ch08`: full file read.
  - `The V4L2 async interface and the concept of graph binding`
  - `Graph binding`
  - `Port and endpoint representations`
  - `Endpoint linking`
  - `The V4L2 async and graph-oriented API`
  - `The V4L2 firmware node API`
  - `V4L2 fwnode or media bus types`
  - `V4L2 async`
  - `Async bridge and sub-device probing example`
  - `The Linux media controller framework`
  - `The media controller abstraction model`
  - `V4L2 device abstraction`
  - `Media controller data structures`
  - `Integrating media controller support in the driver`
  - `Initializing and registering pads and entities`
  - `Media entity operations`
  - `The concept of a media bus`
  - `Registering the media device`
  - `Media controller from user space`
  - `WaRP7 with an OV2680 example`
  - Note: the markdown source itself contains repeated chapter text in places; duplicate text was read and compared as a source artifact, not treated as additional unique content.
- `ldd2-ch09`: relevant userspace/tool sections read.
  - `V4L2 user space tools`
  - `Using v4l2-ctl`
  - `Changing the device properties`
  - `Capturing frames and streaming`
  - `Debugging V4L2 in user space`
  - `V4L2 compliance driver testing`
- `notion-ch07-extra-part1`: relevant subdevice architecture sections read.
  - `7.2.4. struct v4l2_subdev - Sub-Device Abstraction`
  - `7.2.5. Mối Quan Hệ Giữa 4 Struct`
- `notion-ch07-extra-part2`: relevant sections read.
  - `7.6. Sub-Device Drivers`
  - `7.6.2. struct v4l2_subdev_ops`
  - `7.6.3. Sub-Device Initialization`
  - `7.6.4. Bridge Gọi Sub-Device`
  - `7.7. V4L2 Controls Infrastructure`
  - `7.7.4. Control Inheritance`
- `notion-ch08-part1`: full file read.
  - `8.1. V4L2 Async Framework`
  - `8.2. V4L2 Fwnode API`
  - Part 1 summary/checklist/reference sections.
- `notion-ch08-part2`: full file read.
  - `8.3. Media Controller Framework`
  - `8.4. Complete Integration Example`
  - `8.5. User Space Tools`
  - `8.6. Advanced`
  - `8.7. Tổng Kết`
  - Appendix quick reference.

## Merged Source Notes
- Use `ldd2-ch08` as the primary technical spine. It explains why async probing is needed in DT/fwnode systems and why media controller exists once a capture path becomes a graph rather than a single bridge-plus-sensor pair.
- Use Notion Chapter 8 as the learner-facing mental model source. It makes the async problem concrete: bridge says "I need this remote sensor"; sensor says "I am ready"; the async core matches them and invokes `.bound()` and, once all expected connections are bound, `.complete()`.
- Preserve `ldd2-ch07` as prerequisite material inside this topic, not as a separate finished output. Topic 33 needs enough sub-device detail to explain sensors, controls, pad ops, and `v4l2_subdev_call()`, even though topic 32 covered the broader V4L2/vb2 bridge side.
- Merge the bridge/subdevice responsibility split from all sources:
  - Bridge/video device owns `/dev/videoX`, DMA, vb2, and final capture node registration.
  - Sub-device owns a sensor, decoder, ISP, mux, scaler, or similar pipeline block, often controlled through I2C/SPI/platform registers.
  - Media controller models both as graph entities when the driver enables the media API.
- Merge fwnode/graph material from `ldd2-ch08` and `notion-ch08-part1`:
  - DT/ACPI firmware nodes describe endpoints and remote endpoints.
  - `fwnode_graph_get_next_endpoint()`, `fwnode_graph_get_remote_endpoint()`, and `fwnode_graph_get_remote_port_parent()` discover the remote hardware block.
  - `v4l2_fwnode_endpoint_parse()` parses V4L2 bus properties such as bus type, parallel sync flags, CSI-2 lanes, clock lane, non-continuous clock, and link frequencies.
- Merge media-controller material from `ldd2-ch08` and `notion-ch08-part2`:
  - `media_device` is the top-level `/dev/mediaX` object.
  - `media_entity` represents an IP block, video node, sensor, mux, scaler, ISP, or interface.
  - `media_pad` represents a source or sink endpoint on an entity.
  - `media_link` connects a source pad to a sink pad and may be enabled, immutable, or dynamic.
  - `media_entity_pads_init()` initializes entity pads; `media_create_pad_link()` creates pad-to-pad links; `media_device_register()` publishes `/dev/mediaX`.
- Merge pad-format material carefully:
  - `ldd2-ch08` uses old `struct v4l2_subdev_pad_config *cfg` and `v4l2_subdev_get_try_format()` examples.
  - Notion examples partially use modern `struct v4l2_subdev_state`.
  - Current learner docs should teach the concept first: sub-device pad ops negotiate media-bus formats per pad, distinguishing TRY from ACTIVE state, and then validate current API before buildable examples.
- Merge control material lightly:
  - Sensor controls are implemented through `v4l2_ctrl_handler` and `v4l2_ctrl_ops`.
  - Sub-device controls may be inherited by the bridge's V4L2 control handler when the subdev is registered.
  - Private controls should stay private to the subdevice.
  - Deep controls and userspace control workflows can be linked to topics 32 and 34, but topic 33 must explain controls because the learning-path scope includes them.
- Treat `ldd2-ch09` and the tool sections in Notion Chapter 8 as boundary material:
  - Topic 33 should include `media-ctl -p`, link setup, pad format setup, and topology interpretation.
  - Full userspace capture, compliance, ffmpeg/GStreamer, and broader debugging belong to topic 34.

## Source Differences
- `ldd1` has no direct V4L2 async/subdev/media-controller source. Do not infer coverage from unrelated I2C, SPI, DMA, IRQ-domain, framebuffer, or generic async material.
- `ldd2-ch08` targets Linux v4.19.x. Modern kernel docs, opened on 2026-06-02 and showing Linux documentation version 7.1.0-rc6, use newer async names and helpers:
  - Old source: `struct v4l2_async_subdev`, `V4L2_ASYNC_MATCH_FWNODE`, `v4l2_async_notifier_register()`, `v4l2_async_notifier_unregister()`.
  - Current docs: `struct v4l2_async_connection`, `struct v4l2_async_match_desc`, `V4L2_ASYNC_MATCH_TYPE_FWNODE`, `v4l2_async_nf_init()`, `v4l2_async_nf_add_fwnode_remote()`, `v4l2_async_nf_register()`, `v4l2_async_nf_unregister()`, `v4l2_async_nf_cleanup()`.
  - Final learner docs should explicitly mark the book API as legacy and prefer current names for examples.
- `ldd2-ch08` says bridge drivers manually allocate/add async subdev arrays and notes a v4.20 helper. Current docs center the notifier add helpers and cleanup rules; final examples should avoid the old hand-built array style unless discussing historical code.
- `ldd2-ch08` lists `V4L2_MBUS_CSI2` and later notes the v5.0 split into D-PHY/C-PHY. Notion already uses `V4L2_MBUS_CSI2_DPHY` and `V4L2_MBUS_CSI2_CPHY`. Prefer the modern split when explaining current drivers.
- `ldd2-ch08` and some Notion examples use `v4l2_subdev_pad_config`, `v4l2_subdev_get_try_format()`, and `.init_cfg`. Current subdev docs emphasize `struct v4l2_subdev_state`, active state, streams support, and helpers such as `v4l2_subdev_init_finalize()`/`v4l2_subdev_cleanup()` when state resources are used. Validate exact signatures before creating code examples.
- `ldd2-ch08` says `media_device_register()` should be delayed until the root notifier `.complete()` callback. That remains a good lifecycle rule conceptually, but exact ordering around video-device registration, link creation, subdev node creation, media-device cleanup, and async notifier cleanup must be validated against current drivers before writing examples.
- Notion Chapter 8 is clearer pedagogically but inherits old async API names in many snippets. Use it for flow diagrams/checklists, not as a copyable current-kernel API reference.
- Notion Chapter 8 sometimes says a simple 1-to-1 sensor-to-bridge pipeline does not need media controller. That is true as a simplification, but modern camera stacks and MC-centric drivers may expose media controller even for simple-looking pipelines. Phrase final docs as "may not need" rather than a universal rule.
- `ldd2-ch07` says sub-devices are usually kernel-only but can expose `/dev/v4l-subdevX` when supported. Current userspace docs also emphasize subdev nodes for controls/events/pad formats/selections and modern streams/routing. Include both kernel-only default and optional userspace API.
- `ldd2-ch08` contains duplicate chapter text in the markdown source. The repeated material did not add unique concepts; it was treated as repeated source content.

## Gaps / Uncertainties
- Need current-kernel validation before writing buildable examples:
  - Exact async API migration: `v4l2_async_connection` allocation pattern, first-member rule, notifier initialization, fwnode remote add helper, register/unregister/cleanup, `destroy` callback.
  - Exact subdev pad op signatures for the target kernel: `struct v4l2_subdev_state *state` versus legacy `struct v4l2_subdev_pad_config *cfg`.
  - Current recommendations for `v4l2_async_register_subdev_sensor()` versus `v4l2_async_register_subdev()` for camera sensors.
  - Current media pipeline APIs and whether a simple example should use `media_pipeline_start()`/`media_pipeline_stop()` directly or rely on V4L2 helpers.
  - Current entity cleanup requirements: `media_entity_cleanup()`, `v4l2_subdev_cleanup()`, `media_device_cleanup()`, and ordering with async unregister.
  - How to expose subdev nodes safely under `CONFIG_VIDEO_V4L2_SUBDEV_API`.
- Example scope chosen and implemented:
  - `examples/33-v4l2-async-subdevs-and-media-controller/README.md` is a learning-only lab and code map.
  - It shows a DTS graph snippet, sensor subdev skeleton, bridge async/media skeleton, `media-ctl` setup, debug checklist, ABI impact, and cleanup rules.
  - It does not include a buildable mock module because a realistic CSI/sensor/media example requires hardware-specific registers, clocks, regulators, resets, pinctrl, vb2/DMA integration, and target-kernel validation.
- Need decide where to put advanced stream/routing material:
  - Basic entity/pad/link and single-stream pad formats belong in topic 33.
  - Multiplexed streams, internal routing, virtual channels, metadata streams, and libcamera-style pipelines may be a gap or an advanced subsection.
- Need avoid overloading topic 33 with topic 34:
  - `media-ctl` topology and link/pad setup belongs here.
  - Full capture apps, `v4l2-compliance`, ffmpeg/GStreamer, ioctl sequences, and broad debug workflows belong mostly in topic 34.
- Need validate current device-tree binding docs:
  - The book references old paths such as `Documentation/devicetree/bindings/media/video-interfaces.txt`; current kernel uses YAML bindings under `Documentation/devicetree/bindings/media/`.

## External Validation
- Used: https://docs.kernel.org/driver-api/media/v4l2-async.html
  - Validates current async kAPI names and structures: `struct v4l2_async_connection`, `struct v4l2_async_match_desc`, `struct v4l2_async_notifier`, `v4l2_async_nf_init()`, `v4l2_async_subdev_nf_init()`, `v4l2_async_nf_add_fwnode_remote()`, `v4l2_async_nf_add_fwnode()`, `v4l2_async_nf_add_i2c()`, `v4l2_async_nf_register()`, `v4l2_async_nf_unregister()`, `v4l2_async_nf_cleanup()`, `v4l2_async_register_subdev()`, and the root-notifier-only `.complete()` rule.
- Used: https://docs.kernel.org/driver-api/media/v4l2-subdev.html
  - Validates current sub-device kAPI, media-entity integration, `media_entity_pads_init()`, `media_entity_cleanup()`, `v4l2_subdev_link_validate_default()`, `v4l2_subdev_state`, active state cleanup, streams-aware concepts, and current caveats around link validation.
- Used: https://docs.kernel.org/userspace-api/media/mediactl/media-controller-model.html
  - Validates the userspace media-controller graph model: entities, interfaces, pads, data links, interface links, ancillary links, and the goal of discovering/configuring internal topology through `/dev/mediaX`.
- Used: https://docs.kernel.org/userspace-api/media/v4l/dev-subdev.html
  - Validates the userspace sub-device interface: subdevices are usually kernel-only, may expose character device nodes, can support controls/events/pad formats/selections, and modern subdevs may support streams, multiplexed media pads, and internal routing.

## Learning Content Brief
- Mental model:
  - A modern camera path is not one driver talking to one chip. It is usually a graph: sensor -> MIPI CSI-2 receiver -> mux -> CSI/ISP/scaler -> capture node.
  - **V4L2 async** solves "who probes first?" The bridge registers the connections it expects; each sensor/subdevice registers itself when ready; the async core matches them.
  - **Subdevices** model non-DMA pipeline blocks such as camera sensors, decoders, ISPs, bridges, muxes, and scalers.
  - **Media controller** solves "what is connected to what, and which route/format is active?" It exposes a graph through `/dev/mediaX`.
- Core mechanism:
  - Firmware graph binding describes hardware links with `port`, `endpoint`, and `remote-endpoint`.
  - The bridge parses its endpoints, finds remote devices, creates async connection descriptors, initializes a notifier, and registers it.
  - A sensor/subdevice initializes `struct v4l2_subdev`, its ops, controls, entity function, pads, and then registers with the async core.
  - The async core matches a waiting connection with a real `v4l2_subdev`, registers the subdev with the `v4l2_device`, invokes `.bound()`, and when all required connections are bound invokes the root notifier `.complete()`.
  - `.complete()` is the natural place to finish the usable graph: register subdev nodes if needed, register `/dev/videoX`, create pad links if not already created, and register `/dev/mediaX`.
  - Userspace configures the media graph using `media-ctl`, then streams through `/dev/videoX` using normal V4L2/vb2 operations.
- Important structs/APIs:
  - Subdevs: `struct v4l2_subdev`, `struct v4l2_subdev_ops`, `struct v4l2_subdev_core_ops`, `struct v4l2_subdev_video_ops`, `struct v4l2_subdev_pad_ops`, `struct v4l2_subdev_sensor_ops`, `v4l2_subdev_init()`, `v4l2_i2c_subdev_init()`, `v4l2_spi_subdev_init()`, `v4l2_subdev_call()`, `v4l2_device_register_subdev_nodes()`, `V4L2_SUBDEV_FL_HAS_DEVNODE`.
  - Async, book-era names to recognize in older code: `struct v4l2_async_notifier`, `struct v4l2_async_subdev`, `v4l2_async_notifier_register()`, `v4l2_async_notifier_unregister()`, `v4l2_async_register_subdev()`, `v4l2_async_unregister_subdev()`.
  - Async, current names to prefer in examples: `struct v4l2_async_connection`, `struct v4l2_async_match_desc`, `v4l2_async_nf_init()`, `v4l2_async_nf_add_fwnode_remote()`, `v4l2_async_nf_add_fwnode()`, `v4l2_async_nf_add_i2c()`, `v4l2_async_nf_register()`, `v4l2_async_nf_unregister()`, `v4l2_async_nf_cleanup()`, `v4l2_async_register_subdev()`, `v4l2_async_register_subdev_sensor()`, `v4l2_async_unregister_subdev()`.
  - Fwnode/graph: `struct fwnode_handle`, `struct fwnode_endpoint`, `struct v4l2_fwnode_endpoint`, `dev_fwnode()`, `fwnode_graph_get_next_endpoint()`, `fwnode_graph_get_remote_endpoint()`, `fwnode_graph_get_remote_port_parent()`, `fwnode_handle_put()`, `v4l2_fwnode_endpoint_parse()`.
  - Media controller: `struct media_device`, `struct media_entity`, `struct media_pad`, `struct media_link`, `struct media_pipeline`, `media_device_init()`, `media_device_register()`, `media_device_unregister()`, `media_device_cleanup()`, `media_entity_pads_init()`, `media_entity_cleanup()`, `media_create_pad_link()`, `media_pipeline_start()`, `media_pipeline_stop()`.
  - Media flags/functions: `MEDIA_PAD_FL_SOURCE`, `MEDIA_PAD_FL_SINK`, `MEDIA_LNK_FL_ENABLED`, `MEDIA_LNK_FL_IMMUTABLE`, `MEDIA_LNK_FL_DYNAMIC`, `MEDIA_ENT_F_CAM_SENSOR`, `MEDIA_ENT_F_VID_IF_BRIDGE`, `MEDIA_ENT_F_PROC_VIDEO_ISP`, `MEDIA_ENT_F_PROC_VIDEO_SCALER`, `MEDIA_ENT_F_VID_MUX`, `MEDIA_ENT_F_IO_V4L`.
  - Controls: `struct v4l2_ctrl_handler`, `struct v4l2_ctrl`, `struct v4l2_ctrl_ops`, `v4l2_ctrl_handler_init()`, `v4l2_ctrl_new_std()`, `v4l2_ctrl_new_std_menu()`, `v4l2_ctrl_handler_setup()`, `v4l2_ctrl_handler_free()`, `V4L2_CTRL_FLAG_PRIVATE`, `V4L2_CTRL_FLAG_VOLATILE`.
- Lifecycle/data flow:
  - Sensor probe:
    - Allocate private state.
    - Get clocks, regulators, GPIOs, and regmap/I2C resources.
    - Initialize `v4l2_subdev`.
    - Set subdev ops, flags, control handler, media entity function, and source pad.
    - Parse endpoint/bus properties if the sensor needs them.
    - Optionally detect chip ID after powering resources.
    - Register async subdev.
  - Bridge probe:
    - Initialize `media_device`.
    - Attach it to `v4l2_device.mdev`.
    - Register `v4l2_device`.
    - Parse firmware graph endpoints and add async connections.
    - Set notifier ops and register notifier.
    - Avoid publishing `/dev/videoX` until required subdevices are ready.
  - Async match:
    - A connection and subdev match by fwnode/I2C/custom criteria.
    - `.bound()` records the `v4l2_subdev *`, creates per-subdev state, or performs light setup.
    - `.unbind()` handles removal and should tear down dependent video/media nodes if the pipeline cannot work without that subdev.
    - `.complete()` registers final nodes and media topology once all required connections are bound.
  - Runtime configuration:
    - Userspace inspects `/dev/mediaX`.
    - Userspace enables links and sets pad formats with `media-ctl`.
    - Bridge validates links/formats, starts media pipeline, calls subdev `s_stream`, starts DMA/vb2 capture, then stops in reverse order.
- Example implemented:
  - Learning-only async bridge plus I2C-like sensor skeleton using current async notifier helpers.
  - DT snippet with `port`, `endpoint`, `remote-endpoint`, and CSI-2 lane properties.
  - Media topology example with one sensor source pad linked to one capture sink pad.
  - `media-ctl -p`, link setup, pad format setup, and `v4l2-ctl --stream-mmap` smoke test.
  - All code is marked learning-only because it is not backed by real hardware registers and current kernel build validation.
- Common bugs:
  - Registering `/dev/videoX` before required sensors/subdevices are bound.
  - Forgetting `v4l2_device.mdev = &media_dev`, so entities are not tied into the media graph.
  - Leaking fwnode references after graph parsing.
  - Using old async API names in a current kernel example.
  - Forgetting `v4l2_async_nf_cleanup()` after adding async connections with current helper APIs.
  - Missing `media_entity_cleanup()` or `v4l2_subdev_cleanup()` on sensor remove/error paths.
  - Setting a pad as both source and sink, or creating links in the wrong direction.
  - Creating a link before both entities/pads are initialized.
  - Forgetting `MEDIA_ENT_F_CAM_SENSOR` or another meaningful `entity.function`, making topology hard to inspect.
  - Setting active format only at `/dev/videoX` while subdev pad formats remain incompatible.
  - Ignoring `-ENOIOCTLCMD` versus real errors from `v4l2_subdev_call()`.
  - Starting DMA before the sensor/subdev pipeline has been configured and streaming.
  - Returning success from stream start after `media_pipeline_start()` but failing to unwind when `s_stream` or DMA setup fails.
- Debugging notes:
  - `media-ctl -d /dev/media0 -p` to inspect topology, entities, pads, links, and formats.
  - `media-ctl --print-dot > graph.dot` and Graphviz to visualize complex graphs.
  - `media-ctl --reset`, `media-ctl --links`, and `media-ctl --set-v4l2`/`-V` to configure routes and pad formats.
  - `v4l2-ctl -d /dev/v4l-subdevX --all`, `--get-subdev-fmt`, and `--set-subdev-fmt` when subdev nodes are exposed.
  - `dmesg | grep -E 'v4l2|media|async'` for probe/match/topology errors.
  - Dynamic debug/tracepoints under media/v4l2 where available; exact paths vary by kernel config.
  - If `DQBUF` blocks forever, check media links, pad formats, sensor `s_stream`, bridge start order, IRQ/DMA completion, and vb2 buffer return paths.
- Production concerns:
  - Prefer fwnode APIs over OF-only APIs for new drivers so DT and ACPI paths share code.
  - Treat async matching as lifetime-sensitive: subdevices can unbind; bridge userspace nodes must not outlive required pipeline pieces unsafely.
  - Use runtime PM, regulators, clocks, resets, and pinctrl in the right order around sensor stream enable/disable.
  - Keep pad format state coherent across the pipeline; validate links before streaming.
  - Do not expose a subdev node unless userspace configuration/control is intended and the driver supports the required ioctls safely.
  - Avoid vendor-specific sysfs/ioctl routing hacks; use media controller links/pads for topology.
  - Design DT bindings carefully; graph binding describes physical connectivity, not arbitrary driver policy.
- Interview angles:
  - Why is V4L2 async needed in DT/fwnode systems?
  - What is the difference between a bridge driver and a sub-device driver?
  - What happens when a sensor probes before its bridge?
  - What is the difference between `.bound()` and `.complete()`?
  - Why should `/dev/videoX` often be registered after async completion?
  - How do `port`, `endpoint`, and `remote-endpoint` map to media entities, pads, and links?
  - What is `/dev/mediaX`, and how is it different from `/dev/videoX` and `/dev/v4l-subdevX`?
  - Explain entity, pad, link, source pad, sink pad, enabled link, and immutable link.
  - How do pad formats differ from video-node pixel formats?
  - What does `media-ctl -p` tell you when a camera pipeline fails?
  - Why can a control belong to a sensor subdev but still appear at the bridge/video node?
  - How would you debug a camera where the sensor probes, `/dev/video0` appears, but streaming returns no frames?
