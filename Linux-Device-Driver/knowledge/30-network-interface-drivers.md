# 30 - Network Interface Drivers

## Learning Goal
After this chapter, you should understand how a Linux Ethernet-style network driver connects hardware to the kernel networking stack.

You should be able to:

- Explain why network drivers create a **network interface**, not a `/dev` node.
- Describe `struct net_device`, `struct net_device_ops`, `struct sk_buff`, and `struct napi_struct`.
- Trace probe, registration, open, TX, RX, NAPI polling, link handling, stop, and remove.
- Explain SKB ownership on TX and RX.
- Debug common bring-up failures such as no interface, no carrier, TX timeout, RX drops, interrupt storms, and invalid MAC address.
- Know where DMA, ethtool, PHY/MDIO, PHYLINK, and Device Tree fit without confusing them with the core network-driver model.

## Why This Matters In Real Work
Network drivers are where device-driver work meets high-rate, real-world traffic. A small ordering bug can create packet loss, softirq overload, DMA corruption, or a system that hangs during interface shutdown.

You will use this knowledge when you:

- Bring up an SoC Ethernet MAC, SPI Ethernet controller, USB Ethernet device, or PCIe NIC.
- Debug why `ip link` shows no carrier, no packets, or repeated TX timeouts.
- Connect a MAC to a PHY through MDIO, PHYLIB, or PHYLINK.
- Add ethtool statistics and link diagnostics.
- Review DMA descriptor ownership and cache-coherency bugs.
- Explain why a network driver is different from char, input, IIO, or framebuffer drivers.

**Production rule:** a real Ethernet driver is not just `register_netdev()` plus `ndo_start_xmit()`. It must handle queue flow control, NAPI, DMA ownership, link state, teardown ordering, stats, and userspace-visible diagnostics.

## Mental Model
A network interface driver is an adapter between Ethernet hardware and the Linux networking stack.

```text
userspace sockets / ip / ethtool
        |
        v
kernel networking stack
        |
        | TX packets as struct sk_buff
        | RX packets from driver as struct sk_buff
        v
struct net_device + net_device_ops
        |
        v
driver private state
        |
        v
hardware: DMA rings, FIFOs, IRQs, MAC, PHY, MDIO
```

The network stack owns protocols such as IP, TCP, UDP, ARP, routing, and socket behavior. The driver owns the hardware boundary:

- How packets enter and leave the device.
- How descriptors or FIFOs are filled.
- How DMA buffers are mapped and unmapped.
- How interrupts are acknowledged and batched.
- How link state reaches the kernel.
- How userspace learns statistics and capabilities.

Unlike a character driver, a network driver does not normally create a `/dev/myeth0` file. Userspace interacts through network interfaces:

- `ip link show`
- `ip addr`
- `ip -s link`
- `ethtool`
- sockets
- `/sys/class/net/<ifname>/`

## Core Concepts
The core objects are small, but their ownership rules are strict.

| Concept | Meaning | Why It Matters |
| --- | --- | --- |
| `struct net_device` | Kernel object representing one network interface | What userspace sees as `eth0`, `enp1s0`, or similar. |
| driver private data | Hardware-specific state stored behind `netdev_priv()` | Holds registers, rings, DMA mappings, locks, NAPI, PHY state. |
| `struct net_device_ops` | Driver callback table | The network stack calls these for open, stop, TX, MTU, MAC, stats, etc. |
| `struct sk_buff` / SKB | Packet container used by the kernel networking stack | TX and RX packets move through the driver as SKBs. |
| NAPI | Interrupt plus polling event model for network drivers | Prevents interrupt storms and batches RX/TX completion work. |
| TX queue | Kernel-side transmit scheduling for a netdev/queue | Driver stops/wakes it based on descriptor availability. |
| PHY | Physical layer device handling link electrical signaling | Reports link up/down, speed, duplex, pause, autonegotiation. |
| MAC | Media access controller, usually the main Ethernet controller | Sends/receives Ethernet frames and talks to PHY/PCS. |
| MDIO | Management bus used to access Ethernet PHY registers | Often appears as a child bus under an Ethernet controller. |
| ethtool | Userspace diagnostic/control tool and kernel driver hooks | Exposes link info, stats, ring settings, EEPROM, WOL, self-tests. |

### Network Driver vs Other Driver Types
Network drivers are framework drivers, but their userspace ABI is different from many earlier topics.

| Driver Type | Usual Userspace View | Main Kernel Object |
| --- | --- | --- |
| Character driver | `/dev/name` | `struct cdev`, `file_operations` |
| Input driver | `/dev/input/eventX` | `struct input_dev` |
| IIO driver | `/sys/bus/iio/...`, optional chardev buffers | `struct iio_dev` |
| Framebuffer | `/dev/fbN` | `struct fb_info` |
| Network interface | `ip link`, sockets, `/sys/class/net` | `struct net_device` |

**Interview trap:** if someone says “the Ethernet driver exposes a file in `/dev`,” they are usually mixing network drivers with character devices.

## Kernel Mechanism
The kernel networking core calls driver callbacks through `struct net_device_ops`. The driver registers a completed `struct net_device`, then the stack can immediately call into it.

The key split:

```text
net core
  owns: interface registration, protocol stack, routing, sockets, qdisc, userspace netlink

driver
  owns: hardware registers, DMA rings/FIFOs, IRQ/NAPI, PHY/MAC setup, stats, ethtool hooks
```

### Registration Visibility
`register_netdev(dev)` makes the interface visible. After it succeeds:

- Userspace may run `ip link set dev <ifname> up`.
- The kernel may call `.ndo_open`.
- The stack may call `.ndo_start_xmit` after the interface is opened.
- ethtool and sysfs readers may query state.

So all core driver state must be ready before registration:

- `dev->netdev_ops`
- `dev->ethtool_ops` if supported
- MAC address
- private state
- locks
- NAPI structs
- queue count/features
- bus/hardware resources needed by callbacks

### Netdev Lifetime
`struct net_device` can outlive driver unregister paths because sysfs or userspace may still hold references. This is why allocation and free rules matter.

Normal simple device-driver flow:

```text
probe()
  dev = alloc_etherdev(sizeof_priv)
  priv = netdev_priv(dev)
  initialize everything
  register_netdev(dev)

remove()
  unregister_netdev(dev)
  free hardware/private allocations not owned by netdev
  free_netdev(dev)
```

Do not separately free the memory returned by `netdev_priv(dev)` when it was allocated as part of the netdev.

## Key Structs And APIs
These APIs are easiest to remember by lifecycle stage.

### Allocation And Registration
Use Ethernet helpers for Ethernet-like devices.

| API | Purpose |
| --- | --- |
| `alloc_etherdev(sizeof_priv)` | Allocate Ethernet `net_device` with private storage. |
| `alloc_etherdev_mqs(sizeof_priv, txqs, rxqs)` | Allocate multi-queue Ethernet device. |
| `devm_alloc_etherdev(dev, sizeof_priv)` | Managed allocation available on modern kernels. |
| `netdev_priv(ndev)` | Get driver-private state embedded in the netdev allocation. |
| `register_netdev(ndev)` | Register interface outside RTNL-locked contexts. |
| `devm_register_netdev(dev, ndev)` | Managed registration available on modern kernels. |
| `unregister_netdev(ndev)` | Remove interface and wait for active users to quiesce. |
| `free_netdev(ndev)` | Free netdev and embedded private storage when allowed. |

### `struct net_device`
Important fields and relationships:

- `name`: interface name pattern or assigned name.
- `dev_addr`: active MAC address.
- `perm_addr`: permanent MAC address when known.
- `netdev_ops`: driver operations.
- `ethtool_ops`: optional ethtool operations.
- `features`, `hw_features`, `wanted_features`: offload capabilities and requested features.
- `mtu`, `min_mtu`, `max_mtu`: packet size constraints.
- queue and state fields managed by networking core.

Do not treat `struct net_device` as a normal private struct. The networking core owns much of its lifetime and state transitions.

### `struct net_device_ops`
Core callbacks:

| Callback | Called When | Driver Responsibility |
| --- | --- | --- |
| `.ndo_open` | Interface is brought up | Start hardware, NAPI, IRQs, PHY/link, TX queues. |
| `.ndo_stop` | Interface is brought down | Stop queues, NAPI, IRQs, PHY/link, DMA, hardware. |
| `.ndo_start_xmit` | Stack wants to transmit one SKB | Accept/map/queue packet or report busy correctly. |
| `.ndo_set_rx_mode` | Multicast/promiscuous filter changes | Program receive filter. |
| `.ndo_set_mac_address` | MAC address changes | Validate and program address. |
| `.ndo_validate_addr` | Core validates current MAC | Reject invalid Ethernet addresses. |
| `.ndo_change_mtu` | MTU changes | Validate hardware/ring limits and update device. |
| `.ndo_tx_timeout` | TX hangs too long | Recover TX path and report diagnostics. |
| `.ndo_get_stats64` | Stats requested | Return coherent 64-bit counters. |

### `struct sk_buff`
An SKB is the kernel's packet container.

Important concepts:

- `skb->data`: pointer to packet data.
- `skb->len`: length of packet data.
- `skb_put(skb, len)`: extend valid data area during RX construction.
- `skb_reserve(skb, len)`: reserve headroom/alignment before adding data.
- `skb->protocol = eth_type_trans(skb, ndev)`: classify Ethernet protocol on RX.
- `dev_kfree_skb_any(skb)`: free accepted TX SKB from safe contexts.
- `napi_gro_receive(&priv->napi, skb)`: hand received packet to stack from NAPI.

### TX Ownership Rule
The TX ownership rule is one of the most important network-driver interview topics.

```text
.ndo_start_xmit(skb, dev)
  if driver returns NETDEV_TX_OK:
      driver owns skb and must later free/complete it

  if driver returns NETDEV_TX_BUSY:
      stack still owns skb
      driver must not modify or free it
```

**Production warning:** do not return `NETDEV_TX_BUSY` after writing descriptors, changing SKB fields, or freeing the SKB. That corrupts ownership.

### NAPI APIs
NAPI mixes interrupt notification with polling to avoid one interrupt per packet under load.

| API | Purpose |
| --- | --- |
| `netif_napi_add(ndev, &priv->napi, poll_fn)` | Attach a NAPI instance to a netdev. |
| `napi_enable(&priv->napi)` | Allow scheduling, usually in `.ndo_open`. |
| `napi_disable(&priv->napi)` | Stop scheduling and wait for active poll ownership, usually in `.ndo_stop`. |
| `napi_schedule(&priv->napi)` | Schedule polling, often from ISR after masking RX interrupts. |
| `napi_complete_done(&priv->napi, work_done)` | Finish poll and allow IRQ re-enable when budget not exhausted. |
| `napi_gro_receive(&priv->napi, skb)` | Submit RX packet to stack with GRO support. |

NAPI enable/disable APIs are not idempotent. Calling `napi_disable()` twice can hang.

### Queue Control
The network stack must know when hardware cannot accept more TX packets.

| API | Meaning |
| --- | --- |
| `netif_start_queue(ndev)` | Allow TX callbacks for a single-queue device. |
| `netif_stop_queue(ndev)` | Stop TX callbacks because resources are unavailable or interface is stopping. |
| `netif_wake_queue(ndev)` | Resume TX callbacks when descriptors are available. |
| `netif_carrier_on(ndev)` / `netif_carrier_off(ndev)` | Report link carrier state. |

Multi-queue drivers use queue-specific variants, but the mental model is the same.

### PHY, MDIO, And PHYLINK
Ethernet hardware is often split between a MAC and PHY.

```text
net_device
   |
   v
Ethernet MAC driver
   |
   | MDIO / fixed-link / in-band link / SFP
   v
PHY or PHYLINK
```

Important helpers and objects:

- `struct phy_device`: PHYLIB representation of an Ethernet PHY.
- `struct phylink`: modern link-management abstraction for PHYLIB, fixed links, in-band status, SFP, MAC/PCS split.
- `phylink_create()`, `phylink_of_phy_connect()`, `phylink_start()`, `phylink_stop()`, `phylink_destroy()`.
- `phy-mode`, `phy-handle`, fixed-link, MDIO child nodes in Device Tree.

Use PHYLINK when hardware topology needs it, especially fixed links, SGMII/in-band status, SFP cages, or PCS/MAC separation.

### MAC Address APIs
A production driver should obtain a real stable MAC address when possible.

Common sources:

- Device Tree `local-mac-address` or `mac-address`.
- NVMEM/OTP/eFuse/EEPROM cell.
- Firmware or bootloader-provided data.
- Hardware registers.
- Random locally administered address only as fallback.

Useful APIs:

- `device_get_ethdev_address(dev, ndev)`
- `platform_get_ethdev_address(dev, ndev)`
- `eth_validate_addr(ndev)`
- `eth_hw_addr_set(ndev, addr)`
- `eth_hw_addr_random(ndev)`

## Lifecycle / Data Flow
Network drivers have two lifecycles: device lifecycle and interface lifecycle.

### Device Lifecycle
This is probe/remove. It happens when the bus binds or unbinds the driver.

```text
probe()
  -> enable bus-level device if needed
  -> set DMA mask if device performs DMA
  -> map MMIO registers or initialize bus client
  -> allocate descriptor rings / software state
  -> allocate net_device
  -> initialize private data, locks, NAPI, ethtool, netdev_ops
  -> read and validate MAC address
  -> set up PHY/PHYLINK relationship
  -> register_netdev()

remove()
  -> unregister_netdev()
  -> stop remaining hardware access paths
  -> disconnect/destroy PHY/PHYLINK
  -> free rings and DMA resources
  -> free_netdev()
```

### Interface Lifecycle
This is open/stop. It happens when userspace brings the interface up or down.

```text
ip link set dev eth0 up
  -> .ndo_open()
      -> reset/wake hardware
      -> allocate/refill RX buffers
      -> enable NAPI
      -> enable IRQs
      -> start PHY/PHYLINK
      -> netif_start_queue()

ip link set dev eth0 down
  -> .ndo_stop()
      -> netif_stop_queue()
      -> stop PHY/PHYLINK
      -> disable IRQs
      -> napi_disable()
      -> stop DMA
      -> clean RX/TX rings
      -> power down hardware if appropriate
```

### TX Data Flow
TX begins in the network stack and ends at hardware completion.

```text
socket/qdisc/network stack
  -> .ndo_start_xmit(skb, ndev)
      -> check free TX descriptors
      -> stop queue if ring is full
      -> map skb data/fragments for DMA
      -> fill TX descriptor
      -> ring hardware doorbell
      -> return NETDEV_TX_OK

hardware TX complete interrupt
  -> schedule NAPI or handle completion path
      -> unmap DMA
      -> free completed SKB
      -> update stats
      -> wake queue if descriptors available
```

### RX Data Flow
RX begins at hardware and ends when the driver hands an SKB to the stack.

```text
hardware receives frame
  -> writes packet into RX buffer or FIFO
  -> raises interrupt
  -> ISR masks/acks event
  -> napi_schedule()

NAPI poll
  -> process RX descriptors up to budget
  -> build or attach SKB
  -> skb_put()
  -> skb->protocol = eth_type_trans(skb, ndev)
  -> napi_gro_receive()
  -> refill RX buffer
  -> process TX completions too
  -> if work_done < budget:
         napi_complete_done()
         re-enable interrupts
```

## Minimal Practical Example
This pseudo-code is **learning-only**. It shows object relationships and ordering, not a production hardware driver.

```c
struct demo_priv {
    void __iomem *regs;
    struct net_device *ndev;
    struct napi_struct napi;
    spinlock_t tx_lock;
};

static int demo_poll(struct napi_struct *napi, int budget)
{
    struct demo_priv *priv = container_of(napi, struct demo_priv, napi);
    int work_done = 0;

    while (work_done < budget && demo_rx_ready(priv)) {
        struct sk_buff *skb;
        unsigned int len = demo_rx_len(priv);

        skb = netdev_alloc_skb(priv->ndev, len + NET_IP_ALIGN);
        if (!skb) {
            priv->ndev->stats.rx_dropped++;
            break;
        }

        skb_reserve(skb, NET_IP_ALIGN);
        demo_copy_rx_packet(priv, skb_put(skb, len), len);
        skb->protocol = eth_type_trans(skb, priv->ndev);

        napi_gro_receive(napi, skb);
        work_done++;
    }

    demo_clean_tx_completions(priv);

    if (work_done < budget && napi_complete_done(napi, work_done))
        demo_enable_rx_irq(priv);

    return work_done;
}

static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    demo_disable_rx_irq(priv);
    napi_schedule(&priv->napi);
    return IRQ_HANDLED;
}

static netdev_tx_t demo_start_xmit(struct sk_buff *skb,
                                   struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);

    if (!demo_tx_desc_available(priv))
        return NETDEV_TX_BUSY;  /* skb still belongs to stack */

    demo_map_and_queue_skb(priv, skb);

    if (demo_tx_ring_full(priv))
        netif_stop_queue(ndev);

    demo_kick_tx(priv);
    return NETDEV_TX_OK;        /* driver now owns skb */
}

static int demo_open(struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);

    demo_hw_reset(priv);
    demo_refill_rx_ring(priv);
    napi_enable(&priv->napi);
    demo_enable_irqs(priv);
    netif_start_queue(ndev);
    netif_carrier_off(ndev);

    return 0;
}

static int demo_stop(struct net_device *ndev)
{
    struct demo_priv *priv = netdev_priv(ndev);

    netif_stop_queue(ndev);
    demo_disable_irqs(priv);
    napi_disable(&priv->napi);
    demo_stop_dma(priv);
    demo_free_rings(priv);

    return 0;
}

static const struct net_device_ops demo_ops = {
    .ndo_open       = demo_open,
    .ndo_stop       = demo_stop,
    .ndo_start_xmit = demo_start_xmit,
    .ndo_validate_addr = eth_validate_addr,
};

static int demo_probe(struct platform_device *pdev)
{
    struct net_device *ndev;
    struct demo_priv *priv;
    int ret;

    ndev = alloc_etherdev(sizeof(*priv));
    if (!ndev)
        return -ENOMEM;

    priv = netdev_priv(ndev);
    priv->ndev = ndev;
    spin_lock_init(&priv->tx_lock);

    ndev->netdev_ops = &demo_ops;
    netif_napi_add(ndev, &priv->napi, demo_poll);

    ret = platform_get_ethdev_address(&pdev->dev, ndev);
    if (ret)
        eth_hw_addr_random(ndev);

    ret = register_netdev(ndev);
    if (ret) {
        free_netdev(ndev);
        return ret;
    }

    platform_set_drvdata(pdev, ndev);
    return 0;
}
```

Important lessons from the pseudo-code:

- `register_netdev()` happens after callbacks, NAPI, private state, and MAC address are ready.
- The interrupt handler does very little: mask/ack and schedule NAPI.
- NAPI poll owns RX/TX completion work.
- TX ownership depends on the return value from `.ndo_start_xmit`.
- Stop order prevents the stack, IRQ path, and DMA engine from racing each other.

## Common Bugs And Debugging
Start debugging from the visible symptom, then walk backward to the driver boundary.

### Interface Does Not Appear
Likely causes:

- Driver did not bind to the device.
- Probe failed before `register_netdev()`.
- `register_netdev()` failed due to bad initialization or duplicate name.
- Required clock, regulator, reset, MMIO, IRQ, or DMA setup failed.

Useful checks:

```bash
dmesg | grep -i -E 'eth|net|mdio|phy|dma|napi'
ip link show
ls /sys/class/net
find /sys/bus -name '*driver*' 2>/dev/null | grep -i eth
```

Fix patterns:

- Ensure bus match table and `MODULE_DEVICE_TABLE()` are correct.
- Log probe errors with real errno values.
- Fill `netdev_ops` before registration.
- Validate MAC address before registration or use a deliberate fallback.

### Interface Appears But Has No Carrier
Likely causes:

- PHY not found on MDIO bus.
- Wrong `phy-mode`.
- Wrong reset GPIO, clock, regulator, or pinctrl state.
- PHYLINK/PHYLIB not started in `.ndo_open`.
- Link status interrupt not handled.

Useful checks:

```bash
ip link show dev eth0
ethtool eth0
dmesg | grep -i -E 'phy|mdio|link|carrier'
```

Fix patterns:

- Check `phy-handle`, fixed-link, or in-band-status description.
- Confirm MAC speed/duplex/pause programming matches PHYLINK callbacks.
- Use `netif_carrier_on()` and `netif_carrier_off()` only when link state is known.

### TX Timeout
Likely causes:

- TX interrupt lost or not acknowledged.
- Descriptor ownership bit never returns to CPU.
- DMA mapping direction wrong.
- Queue stopped and never woken.
- Hardware not kicked after descriptor write.
- Missing memory barrier before doorbell write.

Useful checks:

```bash
ip -s link show dev eth0
ethtool -S eth0
cat /proc/interrupts | grep -i eth
dmesg | grep -i 'tx timeout'
```

Fix patterns:

- Verify `.ndo_tx_timeout` reports enough state to diagnose rings.
- Wake queue after cleaning TX completions.
- Unmap DMA and free SKBs exactly once.
- Check descriptor ordering and required write barriers.

### RX Drops Or Bad Packets
Likely causes:

- RX ring not refilled.
- SKB length or `skb_put()` size wrong.
- `eth_type_trans()` missing or called with wrong device.
- DMA sync/unmap missing before CPU reads data.
- NAPI budget handling wrong.
- Hardware filter or multicast/promiscuous mode wrong.

Useful checks:

```bash
ip -s link show dev eth0
ethtool -S eth0
cat /sys/kernel/debug/tracing/available_events | grep -E 'napi|net|skb'
```

Fix patterns:

- Refill buffers before re-enabling RX interrupts.
- Treat DMA ownership as a state machine.
- Confirm packet length includes/excludes FCS as expected by hardware.
- Update RX error counters for CRC/frame/overrun conditions.

### Interrupt Storm Or High `ksoftirqd`
Likely causes:

- Interrupt status not fully acknowledged.
- RX interrupts re-enabled while NAPI still owns work.
- NAPI poll always returns full budget because hardware state is not drained.
- Too much work is done in hard IRQ.

Useful checks:

```bash
top -H
cat /proc/interrupts
trace-cmd record -e napi -e net -e skb sleep 5
```

Fix patterns:

- Mask RX IRQ before `napi_schedule()`.
- Re-enable IRQ only after `napi_complete_done()`.
- Keep hard IRQ short.
- Add interrupt moderation only after correctness is proven.

## Production Checklist
Before review or bring-up, verify the driver as a state machine.

Allocation and registration:

- `net_device` allocated with the right helper and queue count.
- Private state accessed through `netdev_priv()`.
- `netdev_ops`, optional `ethtool_ops`, NAPI, locks, and MAC are initialized before registration.
- `register_netdev()` is the last step that makes the interface visible.
- Failure paths free netdev and resources in reverse order.

Open/stop:

- `.ndo_open` starts hardware, PHY/PHYLINK, NAPI, IRQs, and queues in a safe order.
- `.ndo_stop` stops queues, IRQs, NAPI, PHY/PHYLINK, DMA, and rings in a safe order.
- NAPI enable/disable calls are balanced and not repeated.
- No sleeping bus operations run in hard IRQ or NAPI poll.

TX/RX:

- `.ndo_start_xmit` follows SKB ownership rules.
- Queue stop/wake logic matches descriptor availability.
- TX completions unmap DMA and free SKBs once.
- RX path validates length, builds SKBs correctly, sets protocol, and refills buffers.
- NAPI poll handles budget and completion correctly.

DMA and memory:

- DMA mask is set and checked.
- Streaming DMA direction is correct.
- CPU does not touch device-owned buffers without sync/unmap.
- Required memory barriers are present around descriptor ownership and doorbells.

Link and userspace diagnostics:

- MAC address is stable and valid.
- PHY/MDIO/PHYLINK topology matches hardware and Device Tree.
- Carrier state is updated.
- `ethtool` reports useful link information and stats.
- `ip -s link` counters make sense under traffic.

Stress cases:

- Bring interface up/down repeatedly.
- Unload while interface is down and after it was used.
- Link cable unplug/replug.
- MTU changes.
- RX/TX flood.
- Suspend/resume if hardware supports it.
- DMA mapping failure and allocation failure paths.

## Interview Readiness
For interviews, do not memorize every field in `struct net_device`. Be ready to reason through ownership and order.

You should be able to explain:

- Why network drivers register `net_device` instead of `/dev` files.
- What an SKB is and who owns it during TX and RX.
- Why NAPI exists and how it changes interrupt-driven RX.
- How `.ndo_open`, `.ndo_stop`, and `.ndo_start_xmit` fit together.
- Why `register_netdev()` visibility affects probe ordering.
- How queue stop/wake prevents TX ring overflow.
- How DMA ownership and cache coherency affect packet corruption.
- What PHY, MAC, MDIO, and PHYLINK each do.
- How to debug no carrier, TX timeout, RX drops, and interrupt storms.

See `interview/30-network-interface-drivers.md` for structured practice questions.

## Kernel Version Notes
Network APIs evolve quickly because the networking stack is performance-sensitive.

- Modern drivers should treat **NAPI as the normal RX/TX event model** for real hardware that can batch work.
- `struct net_device` lifetime rules are stricter than many simple examples imply. The device may remain referenced after unregister paths, so use the documented unregister/free ordering.
- Current kernels commonly prefer `ndo_get_stats64()` or per-queue/per-CPU stats over old direct `dev->stats` patterns.
- Older examples may mention `SET_ETHTOOL_OPS`; modern code usually assigns `ndev->ethtool_ops = &ops`.
- PHYLINK is important for modern MAC/PHY topologies, fixed links, in-band status, PCS, and SFP support.
- Managed helpers such as `devm_alloc_etherdev()` and `devm_register_netdev()` exist on current kernels, but teardown ordering must still be understood.
