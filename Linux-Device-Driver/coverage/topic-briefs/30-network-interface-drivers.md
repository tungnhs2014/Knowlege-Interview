# Topic Brief - 30 - Network Interface Drivers

## Output Targets
- Knowledge: `knowledge/30-network-interface-drivers.md`
- Interview: `interview/30-network-interface-drivers.md`
- Example: `examples/30-network-interface-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch22` | `docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md` | read/mapped/covered/merged | Primary source: NIC driver purpose, `struct sk_buff`, `struct net_device`, `alloc_etherdev()`, `netdev_priv()`, `register_netdev()`, `free_netdev()`, `struct net_device_ops`, open/stop, TX/RX paths, interrupt handling, queue control, ethtool hooks, probe/remove, dummy Ethernet example, and cleanup order. |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/mapped/covered-adjacent | Bottom-half context: networking uses softirq, ksoftirqd can reveal interrupt pressure, and tasklets/workqueues are deferred-execution mechanisms. Used to explain why modern network drivers use NAPI rather than doing RX/TX completion work directly in hard IRQ. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/covered-adjacent | Bus registration context: `module_pci_driver()`, `module_usb_driver()`, `module_mdio_driver()`, platform devices, bus matching, and MDIO as a child bus. Used to position Ethernet controllers across SPI, platform, PCI, USB, and MDIO-linked PHYs. |
| `ldd1-ch12` | `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md` | read/mapped/covered-adjacent | DMA mapping foundation: coherent versus streaming DMA, cache ownership, `dma_map_single()`, `dma_unmap_single()`, scatter/gather, and sync APIs. Used to connect network descriptors/SKB data buffers to topic 21 without re-teaching DMA. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No dedicated book-2 network interface driver chapter found. Adjacent PCI/DMA, NVMEM MAC-address, and debugging/tracepoint material is mapped separately. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/covered-adjacent | PCI Ethernet context: MSI-X usefulness for high-speed networking, PCI DMA mask/BAR/MSI setup, streaming DMA note specifically naming network drivers where each `skbuf` data area is mapped/unmapped on the fly. Detailed PCI remains topic 31. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/covered-adjacent | NVMEM consumer context for MAC addresses, SoC revision IDs, and part numbers. Used to note production Ethernet drivers often obtain permanent MAC addresses from NVMEM/OTP/EEPROM rather than hard-coding or always randomizing. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/covered-adjacent | Debugging context: boot log examples for `libphy`/MDIO probing and tracefs event categories such as `napi`, `net`, `skb`, `mac80211`, and `cfg80211`. Used for network-driver debugging workflows. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion network-interface-driver chapter found. Relevant source-tree, build-output, DT Ethernet, workqueue/tasklet, resource-management, SPI Ethernet, and MAC-address snippets are mapped separately. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/incidental | Source-tree orientation: `net/` as networking stack and `drivers/net/` as network device drivers. |
| `notion-ch01-part3` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 3 Building the Kernel.md` | read/mapped/incidental | Build-output example lists `drivers/net/ethernet/intel/e1000/e1000.ko`; used only as module/build orientation. |
| `notion-ch03-part3` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md` | read/mapped/covered-adjacent | Network bottom-half example using `struct net_device`, workqueue RX processing, `struct sk_buff`, `dev_alloc_skb()`, `skb_put()`, `eth_type_trans()`, `netif_rx()`, `register_netdev()`, `unregister_netdev()`, and `free_netdev()`. Marked adjacent because it teaches workqueue deferral rather than modern NAPI. |
| `notion-ch03-part4` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md` | read/mapped/related | Tasklet cleanup example for a network-style RX tasklet. Marked related and stale-adjacent because tasklets are not the preferred modern networking datapath abstraction. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/covered-adjacent | Platform-driver decision context: platform devices versus PCI network cards, and framework registration step mentioning `register_netdev()`. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/covered-adjacent | Probe/remove workflow: resource extraction, MMIO, IRQ, DMA, framework registration, and explicit `Network device: register_netdev()` in probe. Used for lifecycle framing. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/covered-adjacent | Ethernet DT examples: controller node, `reg`, `interrupts`, clocks, `local-mac-address`, aliases, `phy-mode`, `phy-handle`, and MDIO child PHY node. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered-adjacent | Ethernet DT resource examples: `ethernet@02188000`, MMIO `reg`, TX/RX interrupt lines, and `interrupt-names`. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered-adjacent | OF property parsing for `local-mac-address`, `mac-address`, DMA-related booleans, named resources, and `platform_get_irq_byname()`. Used for MAC-address and resource parsing caveats. |
| `notion-ch08-part1-spi` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1 SPI Architecture and Driver Structures.md` | read/mapped/related | SPI source lists Ethernet controllers such as ENC28J60 as SPI peripherals. Used to connect the book's SPI Ethernet example to bus-specific sleeping-access constraints. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | searched/mapped/incidental | Search hit only: mentions external devices such as network devices in IRQ overview. No network-specific driver detail merged. |

## Source Files Read
- `ldd1-ch22`: complete chapter. Read driver data structures, SKB allocation, `net_device`, `net_device_ops`, open/close, packet RX/TX, dummy driver, interrupt handler, ethtool support, probe/remove, and summary.
- `ldd1-ch03`: targeted softirq/ksoftirqd/tasklet/workqueue sections that mention networking and bottom halves.
- `ldd1-ch05`: targeted bus-registration macro list, MDIO bus mention, and bus matching/device hierarchy context.
- `ldd1-ch12`: targeted DMA mapping, coherent versus streaming mappings, cache ownership, scatter/gather, and DMA sync sections.
- `ldd2-ch11`: targeted PCI MSI/MSI-X, DMA mask, coherent/streaming DMA, scatter/gather, and note about network drivers mapping/unmapping each `skbuf` data buffer.
- `ldd2-ch12`: targeted NVMEM introduction that lists MAC addresses as consumer data.
- `ldd2-ch14`: targeted `libphy`/MDIO boot-log example and tracefs event categories including `napi`, `net`, `skb`, `mac80211`, and `cfg80211`.
- `notion-ch01-part1`: targeted kernel source-tree map for `net/` and `drivers/net/`.
- `notion-ch01-part3`: targeted module build-output example for `drivers/net/ethernet/intel/e1000/e1000.ko`.
- `notion-ch03-part3`: targeted "Complete Real-World Example: Network Driver Bottom Half".
- `notion-ch03-part4`: targeted network-style RX tasklet cleanup example.
- `notion-ch05-part1`: targeted platform-versus-PCI examples and framework registration step.
- `notion-ch05-part2`: targeted probe workflow with resources, IRQ, DMA, and `register_netdev()`.
- `notion-ch06-part1`: targeted Ethernet controller, MAC address, `phy-mode`, `phy-handle`, and MDIO child snippets.
- `notion-ch06-part2`: targeted Ethernet MMIO and multiple-interrupt snippets.
- `notion-ch06-part3`: targeted `local-mac-address`, `mac-address`, DMA booleans, and named IRQ/resource parsing snippets.
- `notion-ch08-part1-spi`: targeted common SPI peripheral list containing Ethernet controllers.
- `notion-ch16-part1`: searched and mapped as incidental only.

### Inventory Decisions
- `ldd1-ch22` is the only dedicated internal NIC-driver chapter and is primary for topic 30.
- `ldd2` has no dedicated network interface driver chapter. Its relevant material is adjacent: PCI/MSI/DMA, NVMEM MAC-address storage, and debugging/trace events.
- Notion has no standalone network-interface-driver chapter. Notion contributes adjacent examples across workqueues/tasklets, platform probe/resource flow, Device Tree Ethernet nodes, source-tree orientation, and SPI Ethernet peripherals.
- Same-number chapters were not merged across source groups. For example, `ldd1-ch12` is DMA, `ldd2-ch12` is NVMEM, and Notion chapter 12 is absent from this source set; their numbering is unrelated.
- V4L2 userspace/media DMABUF search hits are not network-interface-driver sources. DMA-BUF video capture remains topics 32-34, while network DMA should be taught through SKB/data-buffer ownership and streaming DMA mapping.
- Wireless, cfg80211/mac80211, CAN, PPP, TUN/TAP, bonding/bridge, switchdev/DSA, XDP, TC offload, and deep network stack internals are out of this topic unless used as brief debugging comparisons.

## Merged Source Notes
- Core mental model:
  - A network interface driver is the adapter between Ethernet hardware and the Linux networking stack.
  - The hardware speaks descriptors, FIFOs, DMA rings, IRQs, link status, and PHY/MAC signals.
  - The kernel networking stack speaks `struct net_device` and `struct sk_buff`.
  - Userspace sees an interface such as `eth0` or `enpXsY`, not a `/dev` node.
- Main objects:
  - `struct net_device`: represents the interface visible to the kernel networking stack and userspace tools.
  - Driver private data: allocated with `alloc_etherdev(sizeof_priv)` or newer `alloc_netdev*()` family and accessed with `netdev_priv()`.
  - `struct net_device_ops`: operation table for open, stop, transmit, address validation, MTU change, multicast/promiscuous filtering, stats, timeout, and ioctl-like operations.
  - `struct sk_buff`: packet container used for RX and TX; includes device pointer, data pointers, packet length, protocol, checksum/metadata, and refcounting.
  - `struct ethtool_ops`: optional driver diagnostics/control hooks for link information, registers, EEPROM, WOL, coalescing, ring parameters, statistics, and self-tests.
- Allocation and registration:
  - Allocate with `alloc_etherdev()` or modern multi-queue variants when needed.
  - Fill private data, hardware resources, operations, queues, NAPI instances, MAC address, feature bits, and ethtool hooks before registration.
  - Call `register_netdev()` only after the device can tolerate users opening the interface and invoking callbacks.
  - On remove, call `unregister_netdev()` before freeing hardware state and `free_netdev()`.
- Open/stop:
  - `.ndo_open` is called by `ip link set dev <ifname> up` or equivalent.
  - It should validate/program MAC address, reset or wake hardware, allocate or arm RX/TX resources, enable IRQs/NAPI/PHY link management, and call `netif_start_queue()`.
  - `.ndo_stop` should stop queues/NAPI, disable interrupts, stop PHY/link management, drain TX/RX work, quiesce DMA, and put hardware into a safe state.
- TX path:
  - The stack calls `.ndo_start_xmit(struct sk_buff *skb, struct net_device *dev)`.
  - If accepted, the driver owns the SKB and must eventually free or complete it, usually after DMA/hardware completion.
  - If it returns `NETDEV_TX_BUSY`, the SKB is still owned by the stack and the driver must not modify or free it.
  - Drivers stop the queue when TX resources are full (`netif_stop_queue()` or queue-specific variants) and wake it when resources are available (`netif_wake_queue()`).
  - The book's simple example immediately increments stats and frees the SKB; real hardware usually maps data for DMA, fills descriptors, rings a doorbell, and completes later.
- RX path:
  - Hardware receives frames into FIFO or DMA buffers and raises an interrupt or schedules a poll cycle.
  - Driver builds or attaches an SKB, reserves alignment/headroom when needed, copies or maps packet data, sets protocol with `eth_type_trans()`, updates stats, and hands the packet to the stack.
  - Legacy examples use `netif_rx()`/`netif_rx_ni()` and workqueues. Modern high-throughput drivers should use NAPI and typically deliver packets with `napi_gro_receive()` or related APIs.
- NAPI:
  - Internal `ldd1-ch22` mentions NAPI but explicitly does not teach it.
  - Current kernel documentation defines NAPI as the networking event-handling mechanism that combines interrupt notification with polling.
  - The final learner docs must teach `struct napi_struct`, poll budget, IRQ masking, `netif_napi_add()`, `napi_enable()`, `napi_schedule()`, `napi_complete_done()`, and RX/TX completion in the poll method.
- Link/PHY:
  - `ldd1-ch22` mentions `struct phy_device *phydev` in `struct net_device` and link-status interrupts.
  - Notion DT snippets show `phy-mode`, `phy-handle`, and MDIO child nodes.
  - Current docs require modern Ethernet drivers to consider PHYLIB/PHYLINK, fixed links, in-band status, SFP, and ethtool link settings. This is a major external-validation area because the primary source is older.
- Device Tree and MAC address:
  - Notion sources show `local-mac-address`, `mac-address`, Ethernet controller `reg`, `interrupts`, `interrupt-names`, `phy-mode`, `phy-handle`, and MDIO child PHY nodes.
  - `ldd2-ch12` shows NVMEM as a standardized source for MAC addresses.
  - The final lesson should prefer framework helpers such as `device_get_ethdev_address()`/`platform_get_ethdev_address()` where appropriate, validate addresses, and generate a random address only as a fallback for learning or hardware without permanent storage.
- DMA relationship:
  - Book 1 network chapter discusses copying from device/FIFO and mentions DMA as possible.
  - Book 2 PCI chapter explicitly says network drivers commonly use streaming DMA because each SKB data buffer is mapped/unmapped on the fly.
  - The final docs should connect TX/RX rings, descriptors, DMA ownership, cache syncing, and queue stop/wake to topic 21 without expanding all DMA API details here.
- Debugging:
  - `ip link`, `ip -s link`, `ethtool`, `ethtool -S`, `dmesg`, dynamic debug, tracefs `napi/net/skb` events, drop counters, queue state, IRQ counts, and PHY/MDIO logs are the practical workflow.
  - `ksoftirqd` CPU spikes can indicate RX overload, interrupt storms, or NAPI budget/IRQ moderation problems.

## Source Differences
- NAPI gap:
  - `ldd1-ch22` says new drivers should use NAPI if hardware supports it but then focuses on interrupt-driven RX.
  - Notion network examples use workqueues and tasklets. They are useful for deferred-work principles but should not be presented as the preferred modern RX datapath for Ethernet.
  - Current kernel docs make NAPI central to RX/TX event processing.
- API drift:
  - `ldd1-ch22` shows direct `net_device->stats` updates and `ndo_get_stats`. Current drivers commonly implement 64-bit stats via `ndo_get_stats64()` or per-queue/per-CPU stats depending on design; direct legacy stats updates should be caveated.
  - `SET_ETHTOOL_OPS` appears in older material; current code usually assigns `dev->ethtool_ops = &ops`.
  - The book uses `eth_change_mtu`; current MTU handling is usually through `.ndo_change_mtu` with modern helpers/validation, and the exact helper availability is kernel-version dependent.
  - Local Linux 6.8 headers expose `devm_alloc_etherdev*()` and `devm_register_netdev()`, which the primary book does not cover.
  - Local Linux 6.8 headers expose `phylink`, current `ethtool_ops`, `netif_napi_add()`, `napi_schedule()`, `napi_complete_done()`, `napi_gro_receive()`, and `register_netdev()`.
- Lifetime difference:
  - Current kernel docs stress that `struct net_device` may persist after module unload or unregister due to sysfs/userspace references. Learner docs should emphasize `unregister_netdev()` + `free_netdev()` ordering and not freeing private data separately from the netdev allocation.
- PHY/link difference:
  - The primary chapter only lightly touches link status and `phydev`.
  - Current Ethernet drivers may need PHYLINK instead of direct PHYLIB, especially for fixed links, in-band SGMII, SFP, and MAC/PCS separation.
- Bus difference:
  - `ldd1-ch22` example is SPI/ENC28J60-like and uses workqueues because SPI register access may sleep.
  - PCI/MMIO Ethernet drivers usually use NAPI, DMA rings, MSI/MSI-X, memory barriers, and spinlocks rather than sleeping bus operations.
- Userspace interface difference:
  - Network interfaces do not normally create `/dev` nodes. They appear through rtnetlink/sysfs and are controlled by `ip`, `ethtool`, NetworkManager/systemd-networkd, and sockets.
  - This should be contrasted with character/input/IIO/framebuffer devices from earlier topics.

## Gaps / Uncertainties
- Internal sources do not deeply teach NAPI despite the learning path requiring it.
- Internal sources do not teach descriptor rings, multi-queue networking, BQL, GRO/LRO, checksum offload, TSO/GSO, RX/TX interrupt moderation, XDP, page pool, or zero-copy APIs.
- Internal sources do not provide a current Linux 6.x Ethernet skeleton using NAPI, `ndo_get_stats64`, phylink, and managed netdev allocation.
- Internal sources do not cover modern ethtool netlink internals, only driver-side `struct ethtool_ops` at a high level.
- Internal sources do not cover DSA/switchdev, Wi-Fi/mac80211/cfg80211, USB Ethernet specifics, virtual netdevs, TUN/TAP, veth, bonding, bridge, VLANs, or routing.
- Hardware validation is mandatory for real NIC drivers: descriptor layout, DMA mask, cache coherency, memory barriers, interrupt status/ack order, PHY interface mode, reset/clocks/regulators, MAC address source, MTU/VLAN/offload limits, and link negotiation are hardware-specific.

## External Validation
- Used: `https://docs.kernel.org/networking/netdevices.html`
  - Purpose: validate current `struct net_device` lifetime rules, `alloc_netdev*()`/private-data lifetime, `register_netdev()`/`unregister_netdev()` ordering, RTNL caveats, and the rule that all initialization must be complete before registration makes the device visible.
- Used: `https://docs.kernel.org/networking/napi.html`
  - Purpose: validate NAPI as the current networking event-handling mechanism, `struct napi_struct`, poll method, control API, scheduling, IRQ masking, non-idempotent enable/disable rules, and budget-driven RX/TX completion.
- Used: `https://docs.kernel.org/networking/kapi.html`
  - Purpose: validate current kernel networking APIs and PHYLINK API descriptions.
- Used: `https://docs.kernel.org/networking/sfp-phylink.html`
  - Purpose: validate modern PHYLINK modes and conversion guidance from older PHYLIB-style drivers, including fixed links, in-band mode, `phylink_start()`, `phylink_stop()`, ethtool integration, and DT/FWNODE PHY connection.
- Used: `https://docs.kernel.org/networking/ethtool-netlink.html`
  - Purpose: validate current userspace ethtool netlink interface framing and privilege model; learner docs should still focus on driver-side `struct ethtool_ops`.
- Local validation: Linux `6.8.0-124-generic` headers were inspected:
  - `/lib/modules/6.8.0-124-generic/build/include/linux/netdevice.h`
  - `/lib/modules/6.8.0-124-generic/build/include/linux/etherdevice.h`
  - `/lib/modules/6.8.0-124-generic/build/include/linux/skbuff.h`
  - `/lib/modules/6.8.0-124-generic/build/include/linux/ethtool.h`
  - `/lib/modules/6.8.0-124-generic/build/include/linux/phylink.h`
  - `/lib/modules/6.8.0-124-generic/build/include/linux/phy.h`
  - Confirmed current availability of `struct net_device`, `struct net_device_ops`, `struct napi_struct`, `struct sk_buff`, `struct ethtool_ops`, `struct phylink`, `struct phy_device`, `alloc_etherdev()`, `devm_alloc_etherdev()`, `register_netdev()`, `devm_register_netdev()`, `unregister_netdev()`, `free_netdev()`, `netif_napi_add()`, `napi_schedule()`, `napi_complete_done()`, `napi_gro_receive()`, `eth_type_trans()`, `eth_validate_addr()`, `eth_hw_addr_set()`, `device_get_ethdev_address()`, and `platform_get_ethdev_address()`.

## Learning Content Brief
- Learning path number: `30`.
- Slug: `network-interface-drivers`.
- Topic scope:
  - Ethernet-style network interface drivers: `net_device`, `net_device_ops`, SKBs, open/stop, TX/RX, NAPI, interrupts, DMA relationship, ethtool, PHY/MDIO/PHYLINK basics, Device Tree MAC/PHY resources, debugging, and production traps.
  - Keep full PCI mechanics in topic 31, deep DMA in topic 21, generic IRQ internals in topic 15, power management in topic 24, NVMEM details in topic 28, and wireless/switching/offload subsystems out of scope.
- Beginner mental model:
  - "A network driver is not a `/dev` file. It creates a network interface. The kernel gives the driver packets to transmit as SKBs, and the driver gives received SKBs back to the kernel."
  - "The driver owns the hardware boundary: descriptors/FIFOs/DMA/IRQs/PHY link. The network stack owns protocol decisions."
- Core mechanism:
  - Allocate a `net_device`.
  - Store private state with `netdev_priv()`.
  - Fill `netdev_ops`, optional `ethtool_ops`, MAC address, feature flags, queue/NAPI state, and bus resources.
  - Register with `register_netdev()`.
  - `.ndo_open` starts hardware, queues, NAPI, PHY, and interrupts.
  - `.ndo_start_xmit` accepts SKBs, maps/queues them to hardware, and returns `NETDEV_TX_OK` or `NETDEV_TX_BUSY` correctly.
  - RX interrupt or event schedules NAPI; NAPI poll handles RX packets and TX completions within budget, then completes and re-enables interrupts.
  - `.ndo_stop` stops queueing, disables NAPI/IRQs/PHY, drains DMA/work, and quiesces hardware.
  - Remove unregisters the netdev before freeing resources.
- Important structs/APIs:
  - Core: `struct net_device`, `struct net_device_ops`, `struct sk_buff`, `struct napi_struct`, `struct ethtool_ops`, `struct phy_device`, `struct phylink`, `struct phylink_config`, `struct phylink_mac_ops`.
  - Allocation/lifetime: `alloc_etherdev()`, `alloc_etherdev_mqs()`, `devm_alloc_etherdev()`, `netdev_priv()`, `register_netdev()`, `devm_register_netdev()`, `unregister_netdev()`, `free_netdev()`.
  - TX/RX/queue: `.ndo_open`, `.ndo_stop`, `.ndo_start_xmit`, `.ndo_set_rx_mode`, `.ndo_set_mac_address`, `.ndo_change_mtu`, `.ndo_tx_timeout`, `.ndo_get_stats64`, `netif_start_queue()`, `netif_stop_queue()`, `netif_wake_queue()`, queue-specific variants, `dev_kfree_skb_any()`, `skb_tx_timestamp()`.
  - SKB/RX: `netdev_alloc_skb()`, `dev_alloc_skb()`, `skb_reserve()`, `skb_put()`, `eth_type_trans()`, `napi_gro_receive()`, `netif_rx()` for non-NAPI/simple paths.
  - NAPI: `netif_napi_add()`, `napi_enable()`, `napi_disable()`, `napi_schedule()`, `napi_complete_done()`, poll budget handling.
  - MAC/PHY: `eth_validate_addr()`, `eth_mac_addr()`, `eth_hw_addr_set()`, `eth_hw_addr_random()`, `device_get_ethdev_address()`, `platform_get_ethdev_address()`, `phylink_create()`, `phylink_of_phy_connect()`, `phylink_start()`, `phylink_stop()`, `phylink_destroy()`.
  - DMA/debug: `dma_map_single()`, `dma_unmap_single()`, `dma_map_sg()`, `dma_sync_*()`, `ip link`, `ip -s link`, `ethtool`, `ethtool -S`, tracefs `napi/net/skb`, dynamic debug.
- Lifecycle/data flow:
  - Probe: enable bus device, set DMA mask, map registers, allocate rings, allocate netdev, initialize locks/NAPI/work, read MAC address, connect PHY/phylink, assign ops, register netdev.
  - Open: reset/wake hardware, allocate or refill RX buffers, enable NAPI, start PHY/link, request/enable IRQs, start TX queue.
  - TX: stack calls `.ndo_start_xmit`; driver checks descriptor space, maps SKB or fragments, fills TX descriptor, stops queue if full, kicks hardware, completes/free SKB on TX completion.
  - RX: device raises interrupt; ISR masks/acks RX event and schedules NAPI; poll consumes descriptors, builds SKBs, passes packets to stack, refills RX buffers, processes TX completions, completes NAPI if work is done.
  - Link: PHY/PHYLINK reports link changes; driver updates carrier state, MAC speed/duplex/pause, and ethtool-visible state.
  - Stop/remove: stop carrier/queues/NAPI/IRQs, stop DMA and PHY, drain completions, unregister netdev, free rings/resources/netdev.
- Practical example candidates for later:
  - Prefer a learning-only README lab around a safe virtual interface such as `dummy`, `veth`, `ip link`, `ethtool`, tracefs `napi/net/skb`, and `/sys/class/net`.
  - If code is included later, use a learning-only skeleton or pseudo-code NAPI Ethernet driver, not a fake hardware module that pretends to validate real DMA/PHY behavior.
  - A minimal pseudo-code flow should show `alloc_etherdev()`, `netif_napi_add()`, `register_netdev()`, `.ndo_open`, `.ndo_stop`, `.ndo_start_xmit`, NAPI poll, and cleanup.
- Common bugs:
  - Calling `register_netdev()` before all callbacks/resources are initialized.
  - Freeing `netdev_priv()` separately or freeing private state while sysfs/users still reference the netdev.
  - Returning `NETDEV_TX_BUSY` after modifying/freeing the SKB.
  - Forgetting to stop/wake queues around descriptor exhaustion.
  - Doing sleeping bus operations in hard IRQ or NAPI poll.
  - Forgetting NAPI enable/disable ordering or calling NAPI control APIs twice.
  - Not masking/acking interrupts correctly before scheduling NAPI.
  - Handing SKBs to the stack without correct `skb_put()`, protocol, checksum metadata, or length.
  - Ignoring DMA direction/cache ownership and touching buffers while the device owns them.
  - Using an invalid MAC address, all-zero MAC, duplicate random MAC, or failing to read NVMEM/DT MAC.
  - Confusing PHY, MAC, MDIO bus, and `net_device`.
  - Updating legacy stats unsafely or returning inconsistent 32-bit counters on 64-bit/high-rate systems.
- Debugging notes:
  - Start with `ip link show`, `ip -s link show dev <ifname>`, `ethtool <ifname>`, `ethtool -i`, `ethtool -S`, `dmesg`, `/sys/class/net/<ifname>/`, `/proc/interrupts`, and `tracefs`.
  - Check carrier/link, MAC address, queue stopped state, IRQ counts, NAPI activity, RX drops, TX timeouts, DMA mapping errors, and PHY/MDIO logs.
  - Use dynamic debug and tracepoints before adding noisy printk in hot paths.
  - Reproduce with MTU changes, link up/down, stress traffic, unload while interface is open, suspend/resume, and error injection where possible.
- Production concerns:
  - Use NAPI for real hardware that can batch RX/TX events.
  - Use phylink/phylib according to MAC/PHY topology.
  - Use DMA API correctly; validate DMA mask and IOMMU/cache behavior.
  - Keep queue, NAPI, IRQ, PHY, and DMA teardown order strict.
  - Use per-queue/per-CPU stats and 64-bit stats where traffic rate requires.
  - Expose useful ethtool operations and statistics.
  - Validate feature flags/offloads against hardware and SKB metadata.
  - Keep fast paths allocation-light and avoid sleeping in atomic/NAPI contexts.
- Interview angles:
  - Explain why network drivers create `net_device` rather than `/dev` nodes.
  - Walk through TX SKB ownership and `NETDEV_TX_BUSY` traps.
  - Walk through RX interrupt -> NAPI poll -> SKB -> stack.
  - Compare workqueue/tasklet examples with NAPI.
  - Explain `netif_stop_queue()` versus `netif_wake_queue()`.
  - Explain `register_netdev()` visibility and netdev lifetime.
  - Explain PHY/MAC/MDIO/phylink roles.
  - Debug a TX timeout, RX drops, invalid MAC address, no carrier, interrupt storm, or DMA cache-coherency bug.
