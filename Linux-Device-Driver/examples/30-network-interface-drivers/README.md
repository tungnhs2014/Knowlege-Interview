# 30 - Network Interface Drivers Example

## Status

**Learning-only.** This example is intentionally a small virtual Ethernet-like interface. It demonstrates the kernel networking framework surface without pretending to be a real NIC driver.

It is not production-ready because it has no hardware, DMA rings, IRQ handler, NAPI poll loop, RX path, PHY/MDIO/PHYLINK integration, ethtool operations, offloads, or power-management handling.

## Goal

Build and load a tiny out-of-tree module that registers a `struct net_device` named `toyeth%d`.

You will see:

- `alloc_etherdev()` and `netdev_priv()` for netdev plus private state allocation.
- `register_netdev()` and `unregister_netdev()` lifetime.
- `.ndo_open`, `.ndo_stop`, `.ndo_start_xmit`, `.ndo_get_stats64`, `.ndo_set_mac_address`, and `.ndo_validate_addr`.
- TX SKB ownership: after returning `NETDEV_TX_OK`, the driver owns and frees the SKB.
- Userspace ABI impact: the module creates a network interface visible through `ip` and `/sys/class/net`, not a `/dev` node.

## Kernel Version Assumptions

Test target: Linux 6.x style headers.

The example uses stable network-driver APIs available in modern kernels:

- `alloc_etherdev()`
- `netdev_priv()`
- `register_netdev()`
- `unregister_netdev()`
- `free_netdev()`
- `eth_hw_addr_random()`
- `eth_validate_addr`
- `eth_mac_addr`
- `.ndo_get_stats64`

On much older kernels, callback signatures or helper availability may differ.

## Files

| File | Purpose |
| --- | --- |
| `toy_netdev.c` | Learning-only virtual netdev kernel module. |
| `Makefile` | Out-of-tree kernel module build file. |
| `README.md` | Build, load, test, debug, and cleanup notes. |

## Build

From this directory:

```sh
make
```

Expected result:

```text
  CC [M]  toy_netdev.o
  MODPOST Module.symvers
  LD [M]  toy_netdev.ko
```

If headers are missing:

```text
/lib/modules/$(uname -r)/build: No such file or directory
```

Install the kernel headers for the running kernel, then rebuild.

## Load

```sh
sudo insmod toy_netdev.ko
dmesg | tail
ip link show
```

Expected kernel log:

```text
toy_netdev: registered toyeth0 with random MAC xx:xx:xx:xx:xx:xx
```

Expected userspace effect:

```text
toyeth0: <BROADCAST,MULTICAST> mtu 1500 ...
```

The exact interface number may be `toyeth1`, `toyeth2`, and so on if another interface with that name already exists.

## Test

Bring the interface up:

```sh
sudo ip link set dev toyeth0 up
ip link show dev toyeth0
dmesg | tail
```

Expected log:

```text
toyeth0: opened
```

Assign an address:

```sh
sudo ip addr add 192.0.2.10/24 dev toyeth0
ip addr show dev toyeth0
```

Send one packet:

```sh
ping -c 1 -W 1 192.0.2.1 || true
ip -s link show dev toyeth0
```

Expected behavior:

- The ping does not receive a reply because this module has no RX path and no peer.
- TX counters increase.
- TX dropped also increases because the driver intentionally frees transmitted SKBs instead of sending them to hardware.

Example counter shape:

```text
TX:  bytes  packets  errors  dropped ...
     42     1        0       1
```

Change the MAC address while the interface is down:

```sh
sudo ip link set dev toyeth0 down
sudo ip link set dev toyeth0 address 02:11:22:33:44:55
ip link show dev toyeth0
```

The `02:` prefix makes the address locally administered and avoids multicast/broadcast address mistakes.

## Debug

Useful commands:

```sh
ip link show dev toyeth0
ip -s link show dev toyeth0
ls -l /sys/class/net/toyeth0
dmesg | tail -n 30
```

What to look for:

- Interface exists: `register_netdev()` succeeded.
- Interface opens and closes: `.ndo_open` and `.ndo_stop` logs appear.
- TX counters move: `.ndo_start_xmit` is being called.
- No `/dev/toyeth0`: network drivers normally expose netdev ABI, not character-device ABI.

## Cleanup

```sh
sudo ip link set dev toyeth0 down || true
sudo rmmod toy_netdev
dmesg | tail
make clean
```

Expected log:

```text
toyeth0: stopped
toy_netdev: unloaded
```

## Error Path And Lifetime Notes

Initialization order:

```text
toy_init()
  -> alloc_etherdev()
  -> initialize netdev operations, MTU, MAC, private counters
  -> register_netdev()
```

Failure cleanup:

```text
register_netdev() fails
  -> free_netdev()
```

Exit order:

```text
toy_exit()
  -> unregister_netdev()
  -> free_netdev()
```

The private data returned by `netdev_priv()` is part of the `net_device` allocation in this example. It must not be freed separately.

## Userspace ABI Impact

Loading the module creates a network interface:

- `/sys/class/net/toyeth0`
- rtnetlink-visible interface for `ip link`
- stats visible through `ip -s link`

It does not create:

- `/dev/toyeth0`
- a custom ioctl ABI
- driver-specific ethtool operations
- debugfs files
- procfs files

Removing the module unregisters the interface and removes the `/sys/class/net/toyeth0` entry.

## What Production Hardware Drivers Add

A real Ethernet driver would add:

- Hardware probe through platform, PCI, USB, SPI, or another bus.
- MMIO or bus transactions.
- DMA mask setup and streaming DMA mapping.
- TX/RX descriptor rings.
- IRQ handler that masks/acknowledges device interrupts.
- NAPI poll method for RX and TX completion.
- `netif_stop_queue()` / `netif_wake_queue()` around descriptor availability.
- PHYLIB or PHYLINK integration.
- MAC address loading from hardware, Device Tree, ACPI, NVMEM, EEPROM, or firmware.
- ethtool stats and diagnostics.
- MTU, multicast, promiscuous mode, checksum/offload, suspend/resume, and teardown testing.

Keep this module small on purpose: it is a microscope for the netdev lifecycle and SKB ownership rules before adding hardware complexity.
