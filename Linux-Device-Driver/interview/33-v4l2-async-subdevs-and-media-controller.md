# 33 - V4L2 Async, Subdevs, And Media Controller Interview Questions

Strong candidates should be able to reason about camera topology, async probe ordering, sub-device responsibilities, media entities/pads/links, and stream bring-up. Good answers connect firmware graph, kernel objects, userspace tools, and production failure modes.

## Beginner

### 1. What Problem Does V4L2 Async Solve?

- **Level:** Beginner
- **Question:** Why does the V4L2 async framework exist?
- **Short Answer:** V4L2 async solves unordered probing between a bridge driver and its sub-devices, such as an SoC CSI bridge and an I2C camera sensor.
- **Deep Explanation:** In Device Tree or fwnode-based systems, the kernel does not guarantee that the bridge probes before the sensor or that the sensor probes before the bridge. The bridge needs the sensor before the full camera pipeline is usable, but the sensor driver should not need to know which bridge will use it. V4L2 async lets the bridge register expected connections and lets sub-devices register themselves independently. The async core matches them and calls notifier callbacks.
- **API / Code Anchor:**
  ```text
  bridge: v4l2_async_nf_init()
          v4l2_async_nf_add_fwnode_remote()
          v4l2_async_nf_register()

  sensor: v4l2_async_register_subdev()
  ```
- **Production or Debugging Angle:** If the sensor probes but `/dev/video0` never appears, check whether the async match happened and whether the bridge's `.complete()` callback ran.
- **Common Traps:**
  - Assuming probe order follows Device Tree order.
  - Making the sensor driver depend directly on one bridge driver.
  - Registering `/dev/videoX` before required sub-devices are bound.
  - Treating `-EPROBE_DEFER` as a replacement for V4L2 async.
- **Follow-up Questions:**
  - What happens if the sensor probes first?
  - What happens if the bridge probes first?
  - Why is `.complete()` important?

### 2. What Is A V4L2 Sub-Device?

- **Level:** Beginner
- **Question:** What is `struct v4l2_subdev`, and what hardware does it usually represent?
- **Short Answer:** `v4l2_subdev` represents a component inside a V4L2 media pipeline, such as a camera sensor, decoder, MIPI CSI-2 receiver, ISP, scaler, or mux.
- **Deep Explanation:** A modern camera pipeline has multiple hardware blocks. The bridge driver usually exposes `/dev/videoX` and owns final DMA buffers. Sub-device drivers model upstream or intermediate blocks and provide operations the bridge or userspace can call, such as power, stream enable, pad format negotiation, and controls. Sub-devices are usually kernel-only, but they may expose `/dev/v4l-subdevX` if the driver supports direct subdev userspace ioctls.
- **API / Code Anchor:**
  ```c
  struct sensor {
          struct v4l2_subdev sd;
          struct media_pad pad;
          struct v4l2_ctrl_handler ctrls;
  };
  ```
- **Production or Debugging Angle:** If `/dev/video0` exists but no frames arrive, inspect whether the upstream `v4l2_subdev` was found and whether its `s_stream(1)` callback is called.
- **Common Traps:**
  - Thinking a sensor sub-device owns the final `vb2_queue`.
  - Expecting every sub-device to expose `/dev/videoX`.
  - Ignoring optional `/dev/v4l-subdevX` support.
  - Confusing `video_device` with `v4l2_subdev`.
- **Follow-up Questions:**
  - Which object usually owns DMA?
  - What does `V4L2_SUBDEV_FL_HAS_DEVNODE` do?
  - Why are sub-device drivers reusable across boards?

### 3. Bridge Driver Versus Sensor Driver

- **Level:** Beginner
- **Question:** How is a V4L2 bridge driver different from a camera sensor sub-device driver?
- **Short Answer:** The bridge driver exposes the capture video node and usually owns DMA to memory; the sensor sub-device driver controls the sensor registers, power, mode, controls, and stream output.
- **Deep Explanation:** The bridge is the endpoint that receives video data and moves it into userspace buffers through V4L2/vb2. The sensor is upstream. It may be connected over I2C for control and MIPI/parallel bus for pixel data. During stream start, the bridge often calls the sensor's `s_stream(1)` operation, then starts its receiver and DMA engine.
- **API / Code Anchor:**
  ```c
  ret = v4l2_subdev_call(sensor, video, s_stream, 1);
  ```
- **Production or Debugging Angle:** A sensor can be detected over I2C while the capture pipeline still fails because the CSI receiver, media links, pad formats, or DMA are wrong.
- **Common Traps:**
  - Putting vb2 buffer ownership in the sensor driver.
  - Starting bridge DMA before the sensor is streaming.
  - Assuming I2C probe success means pixel data is arriving.
  - Forgetting clocks, regulators, reset GPIOs, and runtime PM.
- **Follow-up Questions:**
  - Which driver creates `/dev/videoX`?
  - Which driver typically handles exposure and gain controls?
  - What should happen on stream stop?

### 4. What Is Media Controller?

- **Level:** Beginner
- **Question:** What is the media controller framework in V4L2?
- **Short Answer:** Media controller exposes the internal topology of a media device as a graph of entities, pads, and links through `/dev/mediaX`.
- **Deep Explanation:** Complex camera hardware is a graph, not a single device. A sensor may feed a CSI-2 receiver, then a mux, then a CSI capture block, then memory. Media controller gives the kernel and userspace a common way to discover and configure that graph. Entities represent blocks, pads represent input/output endpoints, and links represent data flow between pads.
- **API / Code Anchor:**
  ```text
  /dev/media0
    entity: sensor
      pad0: SOURCE
        -> entity: csi receiver pad0: SINK
  ```
- **Production or Debugging Angle:** `media-ctl -d /dev/media0 -p` is often the first command when debugging an MC-centric camera pipeline.
- **Common Traps:**
  - Thinking `/dev/mediaX` is where frames are dequeued.
  - Confusing media-controller links with Device Tree phandles.
  - Ignoring pad formats.
  - Assuming media controller is only for very large pipelines.
- **Follow-up Questions:**
  - What is an entity?
  - What is a pad?
  - How is `/dev/mediaX` different from `/dev/videoX`?

## Mid-level

### 5. Explain `.bound()`, `.complete()`, And `.unbind()`

- **Level:** Mid-level
- **Question:** What do the async notifier callbacks `.bound()`, `.complete()`, and `.unbind()` mean?
- **Short Answer:** `.bound()` is called when one expected connection matches one subdev, `.complete()` is called when all root-notifier connections are bound, and `.unbind()` is called when a bound subdev leaves.
- **Deep Explanation:** The async core matches bridge-side connection descriptors against real `v4l2_subdev` objects. When one match succeeds, `.bound()` lets the bridge remember the subdev or create per-link state. When the root notifier has no more waiting connections, `.complete()` means the required pipeline is present; drivers commonly register video/media nodes there. `.unbind()` handles hot-unplug, driver removal, or teardown of a required subdev.
- **API / Code Anchor:**
  ```c
  static const struct v4l2_async_notifier_operations ops = {
          .bound = bridge_bound,
          .complete = bridge_complete,
          .unbind = bridge_unbind,
  };
  ```
- **Production or Debugging Angle:** If `.complete()` never runs, check that every expected async connection has a matching subdev and that fwnode references point to the same device nodes.
- **Common Traps:**
  - Registering `/dev/videoX` in bridge probe before `.complete()`.
  - Doing heavy hardware initialization in `.bound()` unnecessarily.
  - Ignoring `.unbind()` and leaving userspace nodes alive after a required subdev disappeared.
  - Assuming `.complete()` runs for sub-device notifiers; it is root-notifier-specific in the usual bridge case.
- **Follow-up Questions:**
  - What should `.bound()` store?
  - Why is `.complete()` a good place to publish `/dev/mediaX`?
  - What should `.unbind()` do if the sensor is mandatory?

### 6. How Does Firmware Graph Binding Connect To Media Controller?

- **Level:** Mid-level
- **Question:** How do `port`, `endpoint`, and `remote-endpoint` relate to media entities, pads, and links?
- **Short Answer:** Firmware graph endpoints describe physical connectivity; the driver parses them to find remote subdevices and then creates media entities, pads, and links that expose the runtime topology.
- **Deep Explanation:** In Device Tree, a device node can contain `port` nodes, each with `endpoint` children. Two endpoints point to each other with `remote-endpoint`. The bridge parses its endpoint and follows the remote endpoint to find the sensor or next block. Media controller then represents the resulting topology with `media_entity`, `media_pad`, and `media_link`. The firmware graph is input data; the media graph is the kernel/userspace topology model.
- **API / Code Anchor:**
  ```c
  ep = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
  remote = fwnode_graph_get_remote_port_parent(ep);
  ret = v4l2_fwnode_endpoint_parse(ep, &vep);
  ```
- **Production or Debugging Angle:** If async matching fails, inspect DT endpoint labels, bidirectional `remote-endpoint` properties, and whether fwnode references are released correctly.
- **Common Traps:**
  - Treating `remote-endpoint` as optional in a connected camera graph.
  - Creating media links opposite to the data-flow direction.
  - Forgetting `fwnode_handle_put()`.
  - Encoding driver policy in the graph instead of physical connectivity.
- **Follow-up Questions:**
  - Why prefer fwnode APIs over OF-only APIs?
  - What does `v4l2_fwnode_endpoint_parse()` parse?
  - How do CSI-2 data lanes appear in firmware?

### 7. What Are Entities, Pads, And Links?

- **Level:** Mid-level
- **Question:** Define media entity, media pad, and media link, and explain their direction rules.
- **Short Answer:** An entity is a media pipeline block, a pad is an input or output endpoint on that block, and a link is a directed connection from a source pad to a sink pad.
- **Deep Explanation:** Media controller models data flow. A sensor entity usually has a source pad. A capture video node usually has a sink pad. Processing blocks may have both sink and source pads. A pad must not be both source and sink. Links connect a source pad to a sink pad and may be enabled, immutable, or dynamic.
- **API / Code Anchor:**
  ```c
  sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
  capture->pad.flags = MEDIA_PAD_FL_SINK;

  media_create_pad_link(&sensor->sd.entity, 0,
                        &bridge->vdev.entity, 0,
                        MEDIA_LNK_FL_ENABLED);
  ```
- **Production or Debugging Angle:** Wrong pad direction or wrong link direction can make `media-ctl -p` confusing and can prevent userspace from enabling a valid route.
- **Common Traps:**
  - Setting both `MEDIA_PAD_FL_SOURCE` and `MEDIA_PAD_FL_SINK`.
  - Forgetting meaningful `entity.function`.
  - Creating links before pads are initialized.
  - Assuming enabled links always imply compatible formats.
- **Follow-up Questions:**
  - What is `MEDIA_LNK_FL_IMMUTABLE`?
  - When would a mux have multiple sink pads?
  - Why does entity naming matter?

### 8. Explain Pad Format Negotiation

- **Level:** Mid-level
- **Question:** Why do media-controller drivers need sub-device pad formats, and how are they different from `/dev/videoX` formats?
- **Short Answer:** Pad formats describe the media-bus data between pipeline blocks, while `/dev/videoX` formats describe the memory buffer format seen by userspace.
- **Deep Explanation:** A sensor may output 10-bit Bayer over MIPI CSI-2, a receiver may preserve that media-bus code, and a CSI/ISP block may convert it before DMA. Each enabled link should have compatible source and sink pad formats. Sub-device pad ops such as `get_fmt` and `set_fmt` manage this per-pad state. The final video-node format is what userspace receives in memory and may differ after conversion.
- **API / Code Anchor:**
  ```c
  struct v4l2_subdev_pad_ops {
          int (*get_fmt)(...);
          int (*set_fmt)(...);
          int (*enum_mbus_code)(...);
  };
  ```
- **Production or Debugging Angle:** If `STREAMON` fails or frames have wrong colors, compare all active pad formats with `media-ctl -p`, not only `v4l2-ctl --get-fmt-video`.
- **Common Traps:**
  - Setting only `/dev/video0` format.
  - Confusing media bus codes with V4L2 memory pixel formats.
  - Ignoring TRY versus ACTIVE format state.
  - Assuming the driver must accept the exact requested format instead of clamping it.
- **Follow-up Questions:**
  - What is a media-bus code?
  - What should `set_fmt` do with unsupported sizes?
  - What does link validation check?

### 9. Debug Scenario: Sensor Probes But `/dev/video0` Is Missing

- **Level:** Mid-level
- **Question:** A camera sensor driver probes successfully, but `/dev/video0` never appears. How do you debug it?
- **Short Answer:** Check bridge probe, async notifier registration, fwnode matching, `.bound()`, `.complete()`, and whether the video device is registered only after async completion.
- **Deep Explanation:** Sensor probe only proves the control bus side worked. `/dev/video0` is usually created by the bridge driver. If the bridge never probes, no video node will appear. If the bridge probes but async matching fails, `.complete()` will not run. If `.complete()` runs but video registration fails, inspect `video_register_device()` setup and return values.
- **API / Code Anchor:**
  ```text
  sensor probe -> v4l2_async_register_subdev()
  bridge probe -> v4l2_async_nf_register()
  match -> .bound()
  all matched -> .complete()
  .complete -> video_register_device()
  ```
- **Production or Debugging Angle:** Use `dmesg | grep -E 'v4l2|media|async|sensor|csi'` and add temporary logs in bridge probe, `.bound()`, and `.complete()`.
- **Common Traps:**
  - Debugging only the sensor driver.
  - Assuming `/dev/video0` comes from the sensor.
  - Missing `remote-endpoint` or using the wrong endpoint node for matching.
  - Not checking return codes from notifier or video registration.
- **Follow-up Questions:**
  - What if `.bound()` runs but `.complete()` does not?
  - What if `.complete()` runs but `/dev/media0` is missing?
  - What if the bridge has optional sensors?

### 10. How Do Controls Work With Sub-Devices?

- **Level:** Mid-level
- **Question:** How do V4L2 controls fit into sensor sub-device drivers and bridge drivers?
- **Short Answer:** A sensor sub-device usually owns controls such as exposure and gain through a `v4l2_ctrl_handler`; when the subdev is registered, controls can be inherited by the parent V4L2 device unless marked private.
- **Deep Explanation:** Controls represent user-settable or driver-managed properties. A sensor driver initializes a control handler, creates controls, implements `v4l2_ctrl_ops`, and assigns the handler to `sd.ctrl_handler`. During subdev registration, controls may be added to the parent V4L2 device's control handler. If a low-level control should not be visible through the bridge, the driver can mark it private.
- **API / Code Anchor:**
  ```c
  v4l2_ctrl_handler_init(&sensor->ctrls, 8);
  sensor->exposure = v4l2_ctrl_new_std(&sensor->ctrls, &ops,
                                       V4L2_CID_EXPOSURE, ...);
  sensor->sd.ctrl_handler = &sensor->ctrls;
  ```
- **Production or Debugging Angle:** If `v4l2-ctl --list-ctrls` misses expected exposure or gain controls, check control handler initialization, handler assignment, registration order, and private flags.
- **Common Traps:**
  - Forgetting to free control handlers on probe failure.
  - Writing sensor registers while powered off.
  - Exposing private hardware controls to the bridge unintentionally.
  - Ignoring volatile controls whose value changes in hardware.
- **Follow-up Questions:**
  - What does `V4L2_CTRL_FLAG_PRIVATE` do?
  - When should `v4l2_ctrl_handler_setup()` be called?
  - Why might a bridge override a subdev control?

## Senior

### 11. Current Kernel API Drift

- **Level:** Senior
- **Question:** You find an old V4L2 async example using `struct v4l2_async_subdev` and `v4l2_async_notifier_register()`. What do you check before using it in a new driver?
- **Short Answer:** Check the target kernel's `include/media/v4l2-async.h` and current docs, because modern kernels use `struct v4l2_async_connection`, `v4l2_async_nf_*` helpers, current match descriptors, and notifier cleanup rules.
- **Deep Explanation:** The concept is stable: bridge registers expected async connections, subdevs register themselves, async core matches them. But the API names and object model changed. Current drivers should generally initialize notifiers with `v4l2_async_nf_init()`, add connections with helpers such as `v4l2_async_nf_add_fwnode_remote()`, register with `v4l2_async_nf_register()`, unregister with `v4l2_async_nf_unregister()`, and free helper-allocated connections with `v4l2_async_nf_cleanup()`.
- **API / Code Anchor:**
  ```text
  Old style to recognize:
    struct v4l2_async_subdev
    v4l2_async_notifier_register()

  Current style to check:
    struct v4l2_async_connection
    v4l2_async_nf_add_fwnode_remote()
    v4l2_async_nf_register()
    v4l2_async_nf_cleanup()
  ```
- **Production or Debugging Angle:** API drift bugs often compile-fail obviously, but cleanup/lifetime drift can become probe-failure leaks or remove-time crashes.
- **Common Traps:**
  - Copying book-era code into a current kernel.
  - Forgetting first-member requirements for driver-specific async connection structs.
  - Omitting `.destroy` or cleanup paths when helper APIs allocate resources.
  - Updating names mechanically without understanding lifetime.
- **Follow-up Questions:**
  - What is the first-member rule for async connection wrappers?
  - When is `v4l2_async_nf_cleanup()` needed?
  - How would you make docs version caveats clear for your team?

### 12. Lifetime And Unbind Race

- **Level:** Senior
- **Question:** A required sensor sub-device unbinds while userspace has `/dev/video0` open. What lifetime issues must the bridge handle?
- **Short Answer:** The bridge must prevent new streaming, stop active pipelines safely, unregister or disable dependent nodes, avoid use-after-free of the subdev pointer, and synchronize with file operations, vb2 queue state, and media graph access.
- **Deep Explanation:** Async binding is not just a probe convenience; it affects lifetime. Once a subdev leaves, the bridge's saved `v4l2_subdev *` may become invalid. If userspace can still call ioctls or stream operations, the bridge must serialize teardown with open file handles and queue operations. Depending on driver design, `.unbind()` may unregister the video device, mark the pipeline unavailable, or force stream stop.
- **API / Code Anchor:**
  ```c
  static void bridge_unbind(struct v4l2_async_notifier *notifier,
                            struct v4l2_subdev *sd,
                            struct v4l2_async_connection *asc)
  {
          /* stop/disable dependent pipeline, drop references safely */
  }
  ```
- **Production or Debugging Angle:** Look for races between `.unbind()`, `start_streaming`, `stop_streaming`, `video_unregister_device()`, `media_device_unregister()`, and vb2 queue release.
- **Common Traps:**
  - Keeping a raw subdev pointer without validity rules.
  - Letting `start_streaming` call into a removed subdev.
  - Destroying media entities while userspace still traverses topology.
  - Forgetting lock ordering between graph, device mutex, and queue locks.
- **Follow-up Questions:**
  - Which lock protects your bridge's subdev pointer?
  - What should happen to queued buffers on forced teardown?
  - How do you test unbind/remove races?

### 13. Designing A Multi-Input Camera Pipeline

- **Level:** Senior
- **Question:** You have two sensors feeding a mux, then CSI, then capture. How would you model and configure this with media controller?
- **Short Answer:** Model each sensor, the mux, CSI, and capture node as media entities. Sensors have source pads, the mux has multiple sink pads and one source pad, CSI has sink/source pads, and the capture node has a sink pad. Userspace enables one route and sets compatible pad formats before streaming.
- **Deep Explanation:** The hardware graph should be represented explicitly. The mux entity's sink pads correspond to possible inputs. Links from both sensors to the mux may exist, but usually only one is enabled at a time unless the hardware supports more. The mux's `link_setup` or routing ops configure hardware selection. Pad formats must be compatible along the enabled route. The bridge should validate the active pipeline before starting DMA.
- **API / Code Anchor:**
  ```text
  sensor0:pad0 -> mux:pad0
  sensor1:pad0 -> mux:pad1
  mux:pad2     -> csi:pad0
  csi:pad1     -> capture:pad0

  media-ctl -l "'sensor0':0 -> 'mux':0[1]"
  media-ctl -l "'sensor1':0 -> 'mux':1[0]"
  ```
- **Production or Debugging Angle:** Dynamic routing must reject invalid simultaneous links, protect hardware register changes, and avoid switching routes while streaming unless explicitly supported.
- **Common Traps:**
  - Hardcoding mux selection in private sysfs instead of media links.
  - Allowing two active sensor links when hardware supports one.
  - Forgetting to propagate/clamp pad formats through the selected path.
  - Not handling route changes versus active streaming.
- **Follow-up Questions:**
  - What does `MEDIA_LNK_FL_DYNAMIC` mean?
  - Where would you program the mux hardware?
  - How would userspace discover available routes?

### 14. Debug Scenario: `/dev/video0` Exists But Streaming Hangs

- **Level:** Senior
- **Question:** `/dev/video0` exists and `media-ctl -p` shows a graph, but `v4l2-ctl --stream-mmap` blocks forever. How do you debug it?
- **Short Answer:** Check enabled links, pad formats, link validation, subdev `s_stream`, receiver/CSI enablement, DMA IRQs, and whether the bridge returns buffers with `vb2_buffer_done()`.
- **Deep Explanation:** Node registration only proves objects exist. Streaming requires a valid route, compatible active formats, powered and streaming upstream sub-devices, receiver blocks configured for the bus, DMA started with queued buffers, and IRQ completion returning buffers. A hang in `DQBUF` usually means no buffer reached done/error state.
- **API / Code Anchor:**
  ```bash
  media-ctl -d /dev/media0 -p
  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 --verbose
  dmesg | grep -E 'v4l2|media|async|csi|irq|dma'
  ```
- **Production or Debugging Angle:** Instrument `start_streaming`, every `v4l2_subdev_call(..., s_stream, ...)`, IRQ handler, and `vb2_buffer_done()` path. Confirm queued buffers are available before starting hardware.
- **Common Traps:**
  - Looking only at `/dev/video0` format and ignoring subdev pad formats.
  - Assuming `s_stream(1)` succeeded without checking the return value.
  - Missing interrupt enablement or wrong IRQ polarity.
  - Forgetting to return buffers with `VB2_BUF_STATE_ERROR` on failed start.
- **Follow-up Questions:**
  - What if `STREAMON` succeeds but no IRQ fires?
  - What if IRQ fires but `DQBUF` still blocks?
  - What if pad formats differ only in media-bus code?

### 15. Publishing `/dev/videoX` In Probe Versus `.complete()`

- **Level:** Senior
- **Question:** Should a bridge driver register `/dev/videoX` during probe or only in async `.complete()`?
- **Short Answer:** For pipelines that require async sub-devices, registering in `.complete()` is usually safer because `/dev/videoX` appears only after required pipeline components are bound. Some designs can register earlier if all ioctls handle missing subdevices correctly, but that increases complexity.
- **Deep Explanation:** A userspace-visible video node is an ABI promise. If it appears before the sensor and media graph are ready, applications may open it, query it, or try to stream. The driver must then return sensible errors and handle races. Delaying registration until `.complete()` makes the node mean "pipeline is usable enough to configure." However, drivers with optional inputs or hotplug semantics may choose different behavior if they carefully manage state.
- **API / Code Anchor:**
  ```text
  bridge probe:
    register v4l2_device
    register async notifier
    do not necessarily register video_device yet

  .complete:
    video_register_device()
    media_device_register()
  ```
- **Production or Debugging Angle:** Decide based on ABI semantics, optional hardware, hotplug behavior, and how userspace discovers devices. Document the state machine.
- **Common Traps:**
  - Publishing a half-ready node without clear ioctl behavior.
  - Assuming `.complete()` handles all future hotplug conditions.
  - Forgetting unregister ordering if `.complete()` partially fails.
  - Ignoring userspace that opens devices immediately after uevents.
- **Follow-up Questions:**
  - How would you handle optional sensors?
  - What should `open()` or `STREAMON` return if no route is ready?
  - What is the remove path if video registration succeeds but media registration fails?

### 16. Link Validation And Production Robustness

- **Level:** Senior
- **Question:** Why is link validation important before streaming, and what should it check?
- **Short Answer:** Link validation prevents starting an incompatible pipeline. It should check that enabled source and sink pad formats are compatible, including width, height, media-bus code, and any driver-specific constraints.
- **Deep Explanation:** Media links describe routing, but routing alone does not guarantee that blocks agree on pixel encoding, dimensions, field order, or bus details. The default V4L2 subdev link validation can check basic width, height, and media-bus code equality. Complex drivers may need additional checks for colorspace, crop/compose rectangles, virtual channels, metadata streams, or hardware-specific restrictions.
- **API / Code Anchor:**
  ```c
  static const struct media_entity_operations ops = {
          .link_validate = v4l2_subdev_link_validate_default,
  };
  ```
- **Production or Debugging Angle:** A robust driver should fail `STREAMON` clearly if userspace configured incompatible pad formats, rather than starting hardware and producing corrupt frames.
- **Common Traps:**
  - Treating enabled links as sufficient.
  - Not checking media-bus code.
  - Forgetting crop/compose or scaler constraints.
  - Returning vague errors without logs that explain the mismatch.
- **Follow-up Questions:**
  - What does default link validation check?
  - When do you need custom validation?
  - How would you expose the mismatch to userspace/debug logs?

### 17. Subdev Userspace ABI Tradeoff

- **Level:** Senior
- **Question:** When should a driver expose `/dev/v4l-subdevX`, and what are the tradeoffs?
- **Short Answer:** Expose subdev nodes when userspace is expected to configure or inspect sub-device controls, events, selections, pad formats, or routing directly. Avoid exposing them casually because they become userspace ABI and require correct ioctl behavior.
- **Deep Explanation:** Sub-devices are usually kernel-only. In MC-centric camera stacks, userspace may need direct subdev access to configure pad formats or controls before streaming. Setting `V4L2_SUBDEV_FL_HAS_DEVNODE` and registering subdev nodes makes that possible. But once exposed, the driver must support the relevant subdev ioctls safely, handle state and locking, and maintain ABI behavior.
- **API / Code Anchor:**
  ```c
  sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
  v4l2_device_register_subdev_nodes(&bridge->v4l2_dev);
  ```
- **Production or Debugging Angle:** If exposing subdev nodes, test `v4l2-ctl --help-subdev`, `--get-subdev-fmt`, `--set-subdev-fmt`, controls, and invalid pad indexes.
- **Common Traps:**
  - Exposing subdev nodes without pad ops.
  - Treating subdev ioctls as internal-only after exposing them.
  - Forgetting locking around active state changes.
  - Breaking libcamera or media-controller userspace expectations.
- **Follow-up Questions:**
  - What can userspace do with a subdev node?
  - How do subdev controls differ from video-node controls?
  - What config option affects subdev userspace API availability?

### 18. Bring-Up Plan For A New Camera Board

- **Level:** Senior
- **Question:** You are bringing up a new board with an I2C sensor connected to a MIPI CSI-2 receiver. What is your step-by-step plan?
- **Short Answer:** Validate DT graph and resources, bring up sensor I2C/chip ID, register subdev and bridge async notifier, confirm async match, inspect media topology, configure links/pad formats, start streaming, then debug DMA/IRQ and compliance issues.
- **Deep Explanation:** Board bring-up should move from static description to control bus to media topology to streaming. First verify supplies, clocks, reset GPIOs, pinctrl, endpoint links, lane count, lane order, and link frequencies. Then prove sensor probe and chip ID. Then prove bridge probe and async `.bound()`/`.complete()`. Then use `media-ctl -p` to confirm entities/pads/links. Then set formats and stream with `v4l2-ctl`. Only after frames arrive do deeper quality/compliance work.
- **API / Code Anchor:**
  ```text
  DT graph -> sensor probe -> bridge probe -> async match
  -> media-ctl -p -> media-ctl -l/-V -> v4l2-ctl --stream-mmap
  ```
- **Production or Debugging Angle:** Keep logs at each boundary: resource enable, chip ID, endpoint parse, async match, media registration, stream start, first IRQ, first `vb2_buffer_done()`.
- **Common Traps:**
  - Starting with userspace capture before proving topology.
  - Ignoring lane polarity/order and link frequency.
  - Assuming chip ID proves MIPI data is valid.
  - Skipping error-path cleanup until late.
- **Follow-up Questions:**
  - What would you check if chip ID works but no MIPI packets arrive?
  - What would you check if `media-ctl -p` shows `fmt:unknown`?
  - When would you run `v4l2-compliance`?
