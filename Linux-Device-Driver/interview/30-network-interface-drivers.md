# 30 - Network Interface Drivers Interview Questions

Strong candidates should be able to reason from userspace tools down to `net_device`, SKB ownership, NAPI polling, DMA rings, queue flow control, and PHY link state. The best answers do not stop at “implement `ndo_start_xmit`”; they explain lifetime, ownership, teardown, and debugging evidence.

## Beginner Questions

### 1. What does a Linux network interface driver expose?

- Level: Beginner
- Question: Does an Ethernet driver normally expose a `/dev` node? What does it expose instead?

**Short Answer**

No. A network interface driver normally exposes a network interface such as `eth0` or `enp1s0`, represented by `struct net_device`. Userspace controls it with `ip`, `ethtool`, sockets, rtnetlink, and `/sys/class/net`, not normal character-device file operations.

**Deep Explanation**

Network drivers are part of the kernel networking stack. They register a `struct net_device`, and the networking core provides the userspace control surface. Applications send data through sockets, not by writing packet bytes to `/dev/eth0`.

The driver provides callbacks through `struct net_device_ops`. The stack calls those callbacks when the interface is opened, stopped, transmits packets, changes MAC address, changes MTU, updates multicast filtering, or recovers from TX timeout.

**API / Code Anchor**

- `struct net_device`
- `struct net_device_ops`
- `register_netdev()`
- `unregister_netdev()`
- `/sys/class/net/<ifname>/`
- `ip link show`
- `ethtool <ifname>`

**Production or Debugging Angle**

If the interface is missing from `ip link`, debug probe, bus matching, and `register_netdev()`. Do not look for a missing `/dev` node.

**Common Traps**

- Confusing network drivers with character drivers.
- Expecting `read()`/`write()` file operations for Ethernet packets.
- Forgetting that userspace can invoke callbacks immediately after successful registration.

**Follow-up Questions**

- What kernel object represents the interface?
- Which command shows RX/TX counters?
- What makes the interface visible to userspace?

### 2. What is `struct sk_buff`?

- Level: Beginner
- Question: What is an SKB, and why do network drivers care about it?

**Short Answer**

`struct sk_buff` is the kernel networking stack's packet container. Drivers receive SKBs from the stack for TX and create or attach SKBs for RX before handing packets back to the stack.

**Deep Explanation**

Hardware understands descriptors, FIFOs, DMA addresses, and byte streams. The kernel networking stack understands SKBs. The driver converts between these worlds.

On TX, `.ndo_start_xmit()` receives an SKB that already contains packet data and metadata. The driver maps or copies it to hardware and later frees it when transmission is complete.

On RX, the driver allocates or builds an SKB, places received packet bytes into it, sets metadata such as protocol, and passes it upward, often through NAPI.

**API / Code Anchor**

- `struct sk_buff`
- `.ndo_start_xmit(struct sk_buff *skb, struct net_device *dev)`
- `netdev_alloc_skb()`
- `skb_reserve()`
- `skb_put()`
- `eth_type_trans()`
- `napi_gro_receive()`

**Production or Debugging Angle**

Malformed packets, RX drops, and crashes often come from bad SKB length, missing `skb_put()`, wrong DMA sync, or handing an uninitialized SKB to the stack.

**Common Traps**

- Treating `skb->data` as always writable or linear enough for every operation.
- Forgetting to set `skb->protocol` on RX.
- Freeing an SKB twice.

**Follow-up Questions**

- Who owns an SKB after `NETDEV_TX_OK`?
- What does `skb_put()` do?
- Why call `eth_type_trans()` on RX?

### 3. What are `netif_stop_queue()` and `netif_wake_queue()` for?

- Level: Beginner
- Question: Why does a driver stop and wake the TX queue?

**Short Answer**

The driver stops the TX queue when hardware has no room for more transmit packets and wakes it when descriptors or buffers become available again.

**Deep Explanation**

The network stack may keep calling `.ndo_start_xmit()` while traffic is queued. If the hardware TX ring is full, the driver must tell the stack to pause transmission for that interface or queue.

For a simple single-queue device:

```text
TX ring has space    -> netif_wake_queue()
TX ring becomes full -> netif_stop_queue()
```

This prevents the driver from accepting SKBs it cannot store safely.

**API / Code Anchor**

- `netif_start_queue()`
- `netif_stop_queue()`
- `netif_wake_queue()`
- `.ndo_start_xmit`
- TX completion interrupt or NAPI poll

**Production or Debugging Angle**

A stuck stopped queue often appears as a TX timeout or no outgoing traffic. Check whether TX completion actually wakes the queue after descriptors are cleaned.

**Common Traps**

- Stopping the queue and never waking it.
- Waking the queue before descriptors are actually free.
- Returning `NETDEV_TX_BUSY` as normal flow control instead of stopping the queue early.

**Follow-up Questions**

- Where should the queue usually be woken?
- What happens if the queue is not stopped when descriptors are full?
- How does this change for multi-queue devices?

## Mid-level Questions

### 4. Walk through probe and remove for a simple Ethernet driver.

- Level: Mid
- Question: What should happen in probe and remove around `register_netdev()`?

**Short Answer**

Probe allocates and initializes the netdev and all state needed by callbacks, then calls `register_netdev()` near the end. Remove calls `unregister_netdev()` before freeing state and finally calls `free_netdev()`.

**Deep Explanation**

`register_netdev()` makes the interface visible. Once it succeeds, userspace can bring it up and the kernel can invoke driver callbacks. Therefore, probe must finish initialization first.

Typical flow:

```text
probe()
  -> allocate netdev
  -> initialize private data, locks, NAPI, ops, MAC address
  -> map resources / set DMA mask / prepare rings
  -> connect PHY or PHYLINK if needed
  -> register_netdev()

remove()
  -> unregister_netdev()
  -> disconnect link management
  -> free rings and hardware resources
  -> free_netdev()
```

`unregister_netdev()` closes the device and waits for active users. The `struct net_device` memory can still have lifetime rules beyond the driver's private intuition, so the documented ordering matters.

**API / Code Anchor**

- `alloc_etherdev()`
- `netdev_priv()`
- `netif_napi_add()`
- `register_netdev()`
- `unregister_netdev()`
- `free_netdev()`

**Production or Debugging Angle**

If userspace races with half-initialized callbacks, the bug may look like a random crash after `ip link set up`. The root cause is often calling `register_netdev()` too early.

**Common Traps**

- Registering before `netdev_ops` or NAPI is initialized.
- Freeing `netdev_priv()` separately.
- Forgetting failure-path cleanup after `register_netdev()` fails.
- Assuming unregister instantly frees all possible references.

**Follow-up Questions**

- Why does registration order matter?
- What should happen if `register_netdev()` fails?
- How does `devm_register_netdev()` change cleanup, and what does it not change?

### 5. Explain TX SKB ownership in `.ndo_start_xmit()`.

- Level: Mid
- Question: What is the difference between returning `NETDEV_TX_OK` and `NETDEV_TX_BUSY`?

**Short Answer**

If the driver returns `NETDEV_TX_OK`, it has accepted ownership of the SKB and must eventually free or complete it. If it returns `NETDEV_TX_BUSY`, the stack still owns the SKB, and the driver must not modify or free it.

**Deep Explanation**

The TX path is ownership-sensitive. A driver may accept an SKB, map it for DMA, put it in a TX descriptor, ring the hardware doorbell, and return `NETDEV_TX_OK`. At that point, cleanup happens later in TX completion.

If the driver cannot accept the SKB, it may return `NETDEV_TX_BUSY`, but only before taking ownership. Returning busy after partially queueing the packet corrupts ownership and can cause duplicate transmission, leaks, or use-after-free.

Good drivers avoid frequent busy returns by stopping the queue when resources are nearly exhausted.

**API / Code Anchor**

- `.ndo_start_xmit`
- `NETDEV_TX_OK`
- `NETDEV_TX_BUSY`
- `netif_stop_queue()`
- `netif_wake_queue()`
- `dev_kfree_skb_any()`
- `dma_map_single()`

**Production or Debugging Angle**

TX timeout debugging should inspect descriptor availability, queue stopped state, DMA mapping, interrupt completion, and whether SKBs are freed exactly once.

**Common Traps**

- Returning `NETDEV_TX_BUSY` after touching descriptors.
- Freeing the SKB on a busy return.
- Forgetting to unmap DMA on TX completion.
- Waking the queue before cleaning descriptors.

**Follow-up Questions**

- Where should a DMA-mapped TX SKB be unmapped?
- Why is queue stop/wake better than repeated `NETDEV_TX_BUSY`?
- What should `.ndo_tx_timeout` inspect?

### 6. Why does modern RX use NAPI?

- Level: Mid
- Question: Why is NAPI preferred over doing all RX work in the interrupt handler?

**Short Answer**

NAPI reduces interrupt overhead by using interrupts as notifications and polling to process batches of RX packets and TX completions under load.

**Deep Explanation**

Without NAPI, a busy network device can interrupt the CPU for every packet or burst, causing interrupt storms. NAPI changes the model:

```text
hardware interrupt
  -> mask/ack interrupt
  -> napi_schedule()

NAPI poll
  -> process RX packets up to budget
  -> clean TX completions
  -> if done, napi_complete_done()
  -> re-enable interrupts
```

NAPI usually runs in softirq context. It is not a place for sleeping operations. The poll function must respect its budget and avoid monopolizing CPU time.

**API / Code Anchor**

- `struct napi_struct`
- `netif_napi_add()`
- `napi_enable()`
- `napi_schedule()`
- `napi_complete_done()`
- `napi_disable()`
- `napi_gro_receive()`

**Production or Debugging Angle**

High `ksoftirqd` CPU usage, RX drops, or interrupt storms often point to NAPI/IRQ masking mistakes, budget misuse, or hardware events not being acknowledged.

**Common Traps**

- Calling `napi_disable()` twice.
- Re-enabling RX interrupts before NAPI completes.
- Sleeping in NAPI poll.
- Returning full budget forever because the driver does not drain or clear hardware state.

**Follow-up Questions**

- Why does the ISR mask RX interrupts before scheduling NAPI?
- What does the poll budget mean?
- What work besides RX can happen in NAPI poll?

### 7. How does an RX packet become visible to the network stack?

- Level: Mid
- Question: Walk through building and submitting an RX SKB.

**Short Answer**

The driver obtains received packet data from hardware, builds or attaches an SKB, sets the valid length and protocol, then passes it to the stack through NAPI or a legacy receive API.

**Deep Explanation**

A simple copy-based RX path looks like:

```text
read packet length
allocate skb
reserve alignment/headroom
copy packet bytes into skb_put(skb, len)
set skb->protocol = eth_type_trans(skb, ndev)
submit with napi_gro_receive()
update/refill RX resources
```

DMA-ring drivers often allocate RX buffers in advance. On completion, the driver syncs/unmaps or otherwise transfers ownership back to the CPU, wraps the data in an SKB or page-based receive path, submits it, and refills the descriptor with a fresh buffer.

**API / Code Anchor**

- `netdev_alloc_skb()`
- `skb_reserve()`
- `skb_put()`
- `eth_type_trans()`
- `napi_gro_receive()`
- `netif_rx()` for simple non-NAPI paths
- `dma_sync_single_for_cpu()`

**Production or Debugging Angle**

Wrong RX length, missing DMA sync, or missing `eth_type_trans()` can look like drops, bad protocol decoding, checksum errors, or packets reaching tcpdump with garbage.

**Common Traps**

- Copying into `skb->data` without calling `skb_put()`.
- Ignoring packet length limits and MTU/VLAN overhead.
- Touching a DMA buffer while the device still owns it.
- Forgetting to refill RX descriptors.

**Follow-up Questions**

- Why reserve `NET_IP_ALIGN`?
- What should happen when SKB allocation fails?
- Where do RX error counters get updated?

### 8. How do PHY, MAC, MDIO, and PHYLINK relate?

- Level: Mid
- Question: Explain the roles of the MAC, PHY, MDIO bus, and PHYLINK.

**Short Answer**

The MAC is the Ethernet controller that sends and receives frames. The PHY handles electrical link signaling. MDIO is commonly used to manage PHY registers. PHYLINK is a modern kernel abstraction that coordinates MAC and PHY/fixed-link/in-band/SFP link state.

**Deep Explanation**

The network driver usually owns the MAC. It may communicate with a PHY through MDIO or through a firmware-described topology. Link mode matters because the MAC must be configured to match speed, duplex, pause, and interface mode.

PHYLINK helps when the topology is not a simple directly attached PHY:

- fixed links
- in-band SGMII status
- SFP modules
- separate PCS blocks
- MAC drivers that need clean link-up/link-down callbacks

Device Tree often describes this with properties such as `phy-mode`, `phy-handle`, fixed-link, and MDIO child nodes.

**API / Code Anchor**

- `struct phy_device`
- `struct phylink`
- `struct phylink_config`
- `struct phylink_mac_ops`
- `phylink_create()`
- `phylink_of_phy_connect()`
- `phylink_start()`
- `phylink_stop()`
- `phylink_destroy()`

**Production or Debugging Angle**

No carrier often comes from PHY reset, wrong `phy-mode`, missing MDIO child, wrong clock, incorrect fixed-link description, or MAC not responding to PHYLINK link-up callbacks.

**Common Traps**

- Treating MAC and PHY as the same hardware.
- Hard-coding speed/duplex while autonegotiation is active.
- Ignoring in-band status rules.
- Updating carrier state without validating actual link state.

**Follow-up Questions**

- What does `phy-mode` describe?
- When is PHYLINK preferable to direct PHYLIB calls?
- Which tool shows link speed and duplex?

## Senior Questions

### 9. Design the teardown order for a NAPI + DMA Ethernet driver.

- Level: Senior
- Question: In `.ndo_stop()` and remove, how do you avoid races among TX, RX, NAPI, IRQs, DMA, and userspace?

**Short Answer**

Stop new stack entry first, disable hardware event sources, synchronize NAPI/IRQs, stop DMA, clean rings, and unregister/free the netdev only in the correct lifecycle phase. The exact order depends on whether the path is interface stop or device remove.

**Deep Explanation**

Interface stop should quiesce the datapath:

```text
.ndo_stop()
  -> netif_stop_queue()
  -> stop carrier/link management
  -> disable device interrupts
  -> napi_disable()
  -> stop DMA engines / prevent new descriptors
  -> clean TX completions and free SKBs
  -> recycle/free RX buffers
  -> put hardware in low-power state if appropriate
```

Remove should first make the interface disappear and force close paths:

```text
remove()
  -> unregister_netdev()
  -> disconnect PHYLINK/PHY if not already done
  -> free persistent rings/resources
  -> free_netdev()
```

The main risk is a late IRQ or NAPI poll using memory that remove just freed. Another common risk is freeing TX SKBs before stopping hardware access.

**API / Code Anchor**

- `.ndo_stop`
- `unregister_netdev()`
- `netif_stop_queue()`
- `napi_disable()`
- `free_irq()` or managed IRQ teardown
- `dma_unmap_single()`
- `free_netdev()`

**Production or Debugging Angle**

Use stress tests: `ip link set up/down` loops, traffic during unload, cable flap, suspend/resume, and forced error paths. Race bugs often appear only when traffic is active.

**Common Traps**

- Freeing rings before disabling IRQ/NAPI.
- Calling `napi_disable()` when NAPI was never enabled.
- Letting hardware DMA into freed RX buffers.
- Assuming `unregister_netdev()` frees all driver private resources.

**Follow-up Questions**

- What should happen to in-flight TX SKBs during stop?
- Why does `unregister_netdev()` belong before `free_netdev()`?
- How would MSI-X queues affect teardown?

### 10. Debug a TX timeout.

- Level: Senior
- Question: A driver logs a TX timeout under load. How do you debug it?

**Short Answer**

Check whether the TX queue stopped, whether descriptors completed, whether interrupts/NAPI ran, whether DMA ownership returned, and whether the driver woke the queue after cleaning completions.

**Deep Explanation**

A TX timeout means the networking stack believes a queued packet did not complete in time. Causes can live in multiple layers:

- Stack called `.ndo_start_xmit()`.
- Driver accepted the SKB and returned `NETDEV_TX_OK`.
- Driver mapped DMA and filled a descriptor.
- Hardware should transmit and signal completion.
- Driver should process completion, unmap DMA, free SKB, update stats, and wake queue.

Breakage at any step can look the same externally.

Debug sequence:

```text
ip -s link show dev eth0
ethtool -S eth0
cat /proc/interrupts
dmesg | grep -i 'tx timeout\|dma\|irq'
trace napi/net/skb events if available
dump driver ring indexes in timeout handler
```

**API / Code Anchor**

- `.ndo_start_xmit`
- `.ndo_tx_timeout`
- `netif_stop_queue()`
- `netif_wake_queue()`
- `dma_map_single()`
- `dma_unmap_single()`
- TX completion path

**Production or Debugging Angle**

A useful `.ndo_tx_timeout` logs ring producer/consumer indices, descriptor ownership bits, interrupt status, queue state, DMA error state, and link state. A timeout handler that only resets hardware hides the real bug.

**Common Traps**

- Blaming the network stack before checking descriptor ownership.
- Forgetting memory barriers before ringing the TX doorbell.
- Cleaning descriptors but not waking the queue.
- Returning `NETDEV_TX_OK` without actually making hardware aware of the packet.

**Follow-up Questions**

- What counters would you add to ethtool stats?
- How can missing DMA unmap show up?
- How would you safely recover in `.ndo_tx_timeout`?

### 11. Debug RX drops and high `ksoftirqd`.

- Level: Senior
- Question: The interface receives traffic but drops packets and `ksoftirqd` consumes high CPU. What do you inspect?

**Short Answer**

Inspect NAPI scheduling/completion, interrupt masking, RX ring refill, budget usage, packet/error counters, and whether the device interrupt is being acknowledged correctly.

**Deep Explanation**

High `ksoftirqd` often means the system is spending too much time in network softirq processing or falling behind. RX drops may be caused by hardware overruns, ring starvation, SKB allocation failures, bad NAPI budget handling, or an interrupt storm.

Questions to answer:

- Are RX interrupts disabled while NAPI owns the work?
- Does poll drain enough descriptors?
- Does poll call `napi_complete_done()` only when work is complete?
- Are RX buffers refilled before interrupts are re-enabled?
- Are drops hardware overruns, DMA errors, SKB allocation failures, or stack drops?
- Are IRQ affinity and queue count reasonable?

**API / Code Anchor**

- `napi_schedule()`
- `napi_complete_done()`
- `napi_gro_receive()`
- `netdev_alloc_skb()`
- `ethtool -S`
- `/proc/interrupts`
- tracefs `napi`, `net`, `skb` events

**Production or Debugging Angle**

Instrument separate counters for RX packets, RX bytes, RX allocation failures, RX DMA errors, RX ring full, RX no-buffer, NAPI polls, budget-exhausted polls, and interrupt count. Aggregated `rx_dropped` is not enough.

**Common Traps**

- Re-enabling interrupts before NAPI completion.
- Always returning budget even when no more work exists.
- Processing too much in hard IRQ.
- Treating all drops as stack drops instead of separating hardware/driver reasons.

**Follow-up Questions**

- Why does NAPI have a budget?
- What is the relationship between IRQ moderation and latency?
- How would you prove RX ring refill is failing?

### 12. How should a production driver handle MAC addresses?

- Level: Senior
- Question: Where should a driver get its MAC address, and what should it do if none is available?

**Short Answer**

It should read a stable hardware or firmware-provided MAC address when possible, validate it, program it into the netdev and hardware, and use a random locally administered address only as an explicit fallback.

**Deep Explanation**

Common MAC address sources include:

- Hardware registers.
- Device Tree `local-mac-address` or `mac-address`.
- NVMEM/OTP/eFuse/EEPROM.
- Firmware/bootloader data.

The driver should reject invalid addresses such as all-zero or multicast addresses for a unicast Ethernet interface. If no permanent address exists, a random address may be acceptable for development or some products, but it must be understood as non-stable unless persisted somewhere.

**API / Code Anchor**

- `device_get_ethdev_address()`
- `platform_get_ethdev_address()`
- `eth_hw_addr_set()`
- `eth_hw_addr_random()`
- `eth_validate_addr()`
- `.ndo_set_mac_address`

**Production or Debugging Angle**

Duplicate MAC addresses cause strange network symptoms: ARP instability, intermittent connectivity, DHCP confusion, and switches learning the same address on multiple ports.

**Common Traps**

- Shipping devices with all-zero MAC addresses.
- Randomizing on every boot without product approval.
- Reading `mac-address` but not validating it.
- Updating `ndev->dev_addr` without programming hardware filters.

**Follow-up Questions**

- What is a locally administered MAC address?
- How can NVMEM provide a MAC address?
- What should `.ndo_set_mac_address` do?

### 13. Compare workqueues, tasklets, and NAPI for network RX.

- Level: Senior
- Question: Why are old workqueue/tasklet RX examples not enough for a modern Ethernet driver?

**Short Answer**

Workqueues and tasklets teach deferred work, but NAPI is the networking-specific mechanism for batching RX/TX event processing, reducing interrupt storms, and integrating with the network stack's softirq behavior.

**Deep Explanation**

Workqueues run in process context and can sleep, which is useful for slow buses like SPI where register access may sleep. Tasklets run in softirq context but are an older generic bottom-half mechanism. NAPI is designed specifically for network device event processing.

For an SPI Ethernet controller, a workqueue may still be part of the design because SPI transfers can sleep. For an MMIO/PCI Ethernet controller, the normal datapath should be NAPI-based:

```text
hard IRQ -> schedule NAPI -> poll descriptors -> submit SKBs -> complete
```

The design must match the bus and hardware. Do not blindly copy a workqueue example into a high-throughput MMIO NIC.

**API / Code Anchor**

- `struct work_struct`
- `struct tasklet_struct`
- `struct napi_struct`
- `napi_schedule()`
- `napi_gro_receive()`
- `spi_sync()` as an example sleeping bus access

**Production or Debugging Angle**

Wrong deferral choice shows up as sleeping-in-atomic warnings, high latency, RX drops, or CPU overload. For slow bus devices, threaded IRQ or workqueue may be necessary around sleeping transfers, but packet delivery still needs careful queueing.

**Common Traps**

- Sleeping inside NAPI poll.
- Using tasklets because an old example did.
- Doing full RX processing in hard IRQ.
- Ignoring that SPI, USB, PCI, and platform MMIO devices have different constraints.

**Follow-up Questions**

- When would a workqueue be justified in a network driver?
- Why is NAPI not a normal kernel thread by default?
- How does bus access type affect interrupt design?

### 14. What makes ethtool support useful?

- Level: Senior
- Question: Why should a network driver implement ethtool operations and stats?

**Short Answer**

ethtool gives users and engineers driver-specific diagnostics and controls such as link settings, driver info, ring parameters, coalescing, wake-on-LAN, EEPROM access, and detailed statistics.

**Deep Explanation**

Basic interface counters show only part of the story. Real driver debugging often needs hardware-specific counters:

- TX descriptor unavailable.
- TX timeout recovery count.
- RX no-buffer drops.
- RX CRC/frame/overrun errors.
- DMA mapping failures.
- NAPI budget exhausted count.
- PHY link changes.

Driver-side `struct ethtool_ops` exposes these capabilities. Modern userspace ethtool uses netlink for many requests, but the driver still provides meaningful data through kernel hooks.

**API / Code Anchor**

- `struct ethtool_ops`
- `ndev->ethtool_ops = &ops`
- `ethtool -i <ifname>`
- `ethtool -S <ifname>`
- `ethtool <ifname>`

**Production or Debugging Angle**

Without ethtool stats, production debugging becomes guesswork. Packet loss reports need counters that separate stack drops, driver drops, DMA errors, and hardware link issues.

**Common Traps**

- Exposing counters with unclear names.
- Returning stale or non-atomic stats.
- Forgetting ethtool callbacks can be called while the interface is down.
- Treating ethtool as optional for production network hardware.

**Follow-up Questions**

- Which stats would help debug TX timeout?
- What should ethtool show when the interface is down?
- How do ethtool stats differ from `ip -s link` counters?

### 15. Explain the DMA relationship in a network driver.

- Level: Senior
- Question: How does DMA ownership affect TX and RX correctness?

**Short Answer**

The driver must map packet buffers for the device, avoid touching buffers while the device owns them, synchronize or unmap before CPU access, and free SKBs only after DMA and hardware completion.

**Deep Explanation**

Network drivers often use streaming DMA because each packet buffer or SKB fragment is used for a finite transfer. The ownership states matter:

```text
TX:
  CPU owns skb
  -> dma_map for device
  -> descriptor owned by device
  -> hardware transmits
  -> completion returns ownership
  -> dma_unmap
  -> free skb

RX:
  CPU allocates/refills buffer
  -> dma_map for device
  -> hardware writes packet
  -> completion returns ownership
  -> sync/unmap for CPU
  -> build/submit skb
  -> refill descriptor
```

On non-coherent systems, missing sync/unmap can create packets with stale data or corrupt headers.

**API / Code Anchor**

- `dma_map_single()`
- `dma_unmap_single()`
- `dma_map_sg()`
- `dma_sync_single_for_cpu()`
- `dma_sync_single_for_device()`
- `DMA_TO_DEVICE`
- `DMA_FROM_DEVICE`
- `struct sk_buff`

**Production or Debugging Angle**

DMA bugs may look like random packet corruption, checksum errors, only failing under load, or only failing on non-coherent architectures. Always test on target-like hardware when possible.

**Common Traps**

- Using `DMA_BIDIRECTIONAL` everywhere instead of correct direction.
- CPU reading RX buffer before sync/unmap.
- Freeing an SKB before hardware is done.
- Forgetting memory barriers around descriptor ownership bits.

**Follow-up Questions**

- Why are network drivers a common use case for streaming DMA?
- Where do you unmap a TX SKB?
- What changes with scatter/gather SKBs?
