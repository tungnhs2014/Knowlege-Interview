// SPDX-License-Identifier: GPL-2.0
/*
 * toy_netdev.c - learning-only virtual Ethernet net_device example.
 *
 * This module demonstrates net_device allocation, registration,
 * net_device_ops, TX SKB ownership, stats, and teardown. It is not a real
 * Ethernet hardware driver: there is no DMA, IRQ, NAPI, PHY, or RX path.
 */

#include <linux/atomic.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

struct toy_priv {
	atomic64_t tx_packets;
	atomic64_t tx_bytes;
	atomic64_t tx_dropped;
};

static netdev_tx_t toy_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct toy_priv *priv = netdev_priv(ndev);
	unsigned int len = skb->len;

	/*
	 * Returning NETDEV_TX_OK transfers SKB ownership to the driver. Since
	 * this virtual device has no hardware, it intentionally drops TX frames
	 * after accounting them.
	 */
	skb_tx_timestamp(skb);
	atomic64_inc(&priv->tx_packets);
	atomic64_add(len, &priv->tx_bytes);
	atomic64_inc(&priv->tx_dropped);
	dev_kfree_skb_any(skb);

	return NETDEV_TX_OK;
}

static int toy_open(struct net_device *ndev)
{
	netif_start_queue(ndev);
	netif_carrier_on(ndev);
	netdev_info(ndev, "opened\n");

	return 0;
}

static int toy_stop(struct net_device *ndev)
{
	netif_carrier_off(ndev);
	netif_stop_queue(ndev);
	netdev_info(ndev, "stopped\n");

	return 0;
}

static void toy_get_stats64(struct net_device *ndev,
			    struct rtnl_link_stats64 *stats)
{
	struct toy_priv *priv = netdev_priv(ndev);

	stats->tx_packets = atomic64_read(&priv->tx_packets);
	stats->tx_bytes = atomic64_read(&priv->tx_bytes);
	stats->tx_dropped = atomic64_read(&priv->tx_dropped);
}

static const struct net_device_ops toy_netdev_ops = {
	.ndo_open = toy_open,
	.ndo_stop = toy_stop,
	.ndo_start_xmit = toy_start_xmit,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_mac_address = eth_mac_addr,
	.ndo_get_stats64 = toy_get_stats64,
};

static struct net_device *toy_ndev;

static int __init toy_init(void)
{
	struct toy_priv *priv;
	int ret;

	toy_ndev = alloc_etherdev(sizeof(*priv));
	if (!toy_ndev)
		return -ENOMEM;

	strscpy(toy_ndev->name, "toyeth%d", IFNAMSIZ);
	toy_ndev->netdev_ops = &toy_netdev_ops;
	toy_ndev->min_mtu = ETH_MIN_MTU;
	toy_ndev->max_mtu = ETH_DATA_LEN;

	priv = netdev_priv(toy_ndev);
	atomic64_set(&priv->tx_packets, 0);
	atomic64_set(&priv->tx_bytes, 0);
	atomic64_set(&priv->tx_dropped, 0);

	eth_hw_addr_random(toy_ndev);
	netif_carrier_off(toy_ndev);

	ret = register_netdev(toy_ndev);
	if (ret)
		goto err_free_netdev;

	pr_info("toy_netdev: registered %s with random MAC %pM\n",
		toy_ndev->name, toy_ndev->dev_addr);

	return 0;

err_free_netdev:
	free_netdev(toy_ndev);
	toy_ndev = NULL;
	return ret;
}

static void __exit toy_exit(void)
{
	if (!toy_ndev)
		return;

	unregister_netdev(toy_ndev);
	free_netdev(toy_ndev);
	toy_ndev = NULL;

	pr_info("toy_netdev: unloaded\n");
}

module_init(toy_init);
module_exit(toy_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only virtual Ethernet net_device example");
MODULE_LICENSE("GPL");
