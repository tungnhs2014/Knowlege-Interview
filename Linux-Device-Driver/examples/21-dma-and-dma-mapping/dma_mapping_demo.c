// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only DMA mapping API demonstration.
 *
 * This module registers a synthetic platform device so the examples always
 * use a real struct device. It demonstrates coherent allocation, one streaming
 * mapping, one scatter-gather mapping, mapping-error handling, and cleanup.
 *
 * It does not program hardware and therefore performs no DMA transfer.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/sizes.h>
#include <linux/slab.h>

#define DMA_DEMO_COHERENT_SIZE		SZ_1K
#define DMA_DEMO_STREAMING_SIZE		SZ_2K
#define DMA_DEMO_SG_ENTRIES		3

struct dma_demo_state {
	struct device *dev;
	void *coherent_cpu;
	dma_addr_t coherent_dma;
};

static struct platform_device *dma_demo_pdev;
static int dma_demo_probe_status;

static unsigned int dma_bits = 32;
module_param(dma_bits, uint, 0444);
MODULE_PARM_DESC(dma_bits, "Synthetic device DMA address width: 32 or 64");

static unsigned int fail_step;
module_param(fail_step, uint, 0444);
MODULE_PARM_DESC(fail_step, "Inject probe failure after step 1..3");

static int dma_demo_streaming(struct device *dev)
{
	dma_addr_t dma;
	void *buf;
	int ret = 0;

	buf = kmalloc(DMA_DEMO_STREAMING_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0xa5, DMA_DEMO_STREAMING_SIZE);

	dma = dma_map_single(dev, buf, DMA_DEMO_STREAMING_SIZE,
			     DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dma)) {
		dev_err(dev, "streaming mapping failed\n");
		ret = -EIO;
		goto out_free;
	}

	dev_info(dev, "streaming: cpu=%p dma=%pad size=%u direction=to-device\n",
		 buf, &dma, DMA_DEMO_STREAMING_SIZE);

	if (fail_step == 2) {
		dev_info(dev, "injecting failure after streaming map\n");
		ret = -EIO;
	}

	/*
	 * No hardware consumes the mapping in this learning module. Do not touch
	 * the buffer between map and unmap, preserving the ownership contract.
	 */
	dma_unmap_single(dev, dma, DMA_DEMO_STREAMING_SIZE, DMA_TO_DEVICE);
	dev_info(dev, "streaming: unmapped with matching device/size/direction\n");

out_free:
	kfree(buf);
	return ret;
}

static void dma_demo_free_sg_buffers(void **bufs)
{
	unsigned int i;

	for (i = 0; i < DMA_DEMO_SG_ENTRIES; i++)
		kfree(bufs[i]);
}

static int dma_demo_scatter_gather(struct device *dev)
{
	static const size_t sizes[DMA_DEMO_SG_ENTRIES] = {
		512, 1024, 1536
	};
	struct scatterlist sgl[DMA_DEMO_SG_ENTRIES];
	struct scatterlist *sg;
	void *bufs[DMA_DEMO_SG_ENTRIES] = {};
	int mapped_nents;
	int ret = 0;
	unsigned int i;

	sg_init_table(sgl, DMA_DEMO_SG_ENTRIES);

	for (i = 0; i < DMA_DEMO_SG_ENTRIES; i++) {
		bufs[i] = kmalloc(sizes[i], GFP_KERNEL);
		if (!bufs[i]) {
			ret = -ENOMEM;
			goto out_free;
		}

		memset(bufs[i], 0x30 + i, sizes[i]);
		sg_set_buf(&sgl[i], bufs[i], sizes[i]);
	}

	mapped_nents = dma_map_sg(dev, sgl, DMA_DEMO_SG_ENTRIES,
				  DMA_TO_DEVICE);
	if (!mapped_nents) {
		dev_err(dev, "scatter-gather mapping failed\n");
		ret = -EIO;
		goto out_free;
	}

	dev_info(dev, "sg: original_nents=%u mapped_nents=%d\n",
		 DMA_DEMO_SG_ENTRIES, mapped_nents);

	for_each_sg(sgl, sg, mapped_nents, i)
		dev_info(dev, "sg: segment=%u dma=%pad len=%u\n", i,
			 &sg_dma_address(sg), sg_dma_len(sg));

	if (fail_step == 3) {
		dev_info(dev, "injecting failure after scatter-gather map\n");
		ret = -EIO;
	}

	/*
	 * Hardware would consume mapped_nents segments. Unmapping must use the
	 * original input count, not mapped_nents.
	 */
	dma_unmap_sg(dev, sgl, DMA_DEMO_SG_ENTRIES, DMA_TO_DEVICE);
	dev_info(dev, "sg: unmapped with original_nents=%u\n",
		 DMA_DEMO_SG_ENTRIES);

out_free:
	dma_demo_free_sg_buffers(bufs);
	return ret;
}

static int dma_mapping_demo_probe_impl(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dma_demo_state *state;
	u64 mask;
	int ret;

	if (dma_bits != 32 && dma_bits != 64)
		return dev_err_probe(dev, -EINVAL,
				     "dma_bits must be 32 or 64\n");

	mask = DMA_BIT_MASK(dma_bits);
	ret = dma_set_mask_and_coherent(dev, mask);
	if (ret)
		return dev_err_probe(dev, ret,
				     "cannot use a %u-bit DMA mask\n", dma_bits);

	state = devm_kzalloc(dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	state->dev = dev;
	platform_set_drvdata(pdev, state);

	state->coherent_cpu =
		dma_alloc_coherent(dev, DMA_DEMO_COHERENT_SIZE,
				   &state->coherent_dma, GFP_KERNEL);
	if (!state->coherent_cpu)
		return -ENOMEM;

	memset(state->coherent_cpu, 0, DMA_DEMO_COHERENT_SIZE);
	dev_info(dev, "coherent: cpu=%p dma=%pad size=%u\n",
		 state->coherent_cpu, &state->coherent_dma,
		 DMA_DEMO_COHERENT_SIZE);

	if (fail_step == 1) {
		dev_info(dev, "injecting failure after coherent allocation\n");
		ret = -EIO;
		goto err_free_coherent;
	}

	ret = dma_demo_streaming(dev);
	if (ret)
		goto err_free_coherent;

	ret = dma_demo_scatter_gather(dev);
	if (ret)
		goto err_free_coherent;

	dev_info(dev, "probe complete; no hardware DMA transfer was performed\n");
	return 0;

err_free_coherent:
	dma_free_coherent(dev, DMA_DEMO_COHERENT_SIZE,
			  state->coherent_cpu, state->coherent_dma);
	state->coherent_cpu = NULL;
	dev_info(dev, "cleanup: coherent allocation released after probe failure\n");
	return ret;
}

static int dma_mapping_demo_probe(struct platform_device *pdev)
{
	int ret;

	ret = dma_mapping_demo_probe_impl(pdev);
	WRITE_ONCE(dma_demo_probe_status, ret);
	return ret;
}

static void dma_mapping_demo_remove(struct platform_device *pdev)
{
	struct dma_demo_state *state = platform_get_drvdata(pdev);

	if (state->coherent_cpu) {
		dma_free_coherent(state->dev, DMA_DEMO_COHERENT_SIZE,
				  state->coherent_cpu, state->coherent_dma);
		state->coherent_cpu = NULL;
		dev_info(state->dev, "remove: coherent allocation released\n");
	}
}

static struct platform_driver dma_mapping_demo_driver = {
	.probe = dma_mapping_demo_probe,
	.remove_new = dma_mapping_demo_remove,
	.driver = {
		.name = "dma-mapping-demo",
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
};

static int __init dma_mapping_demo_init(void)
{
	struct platform_device_info info = {
		.name = "dma-mapping-demo",
		.id = PLATFORM_DEVID_NONE,
		.dma_mask = DMA_BIT_MASK(64),
	};
	int ret;

	if (dma_bits != 32 && dma_bits != 64)
		return -EINVAL;

	if (fail_step > 3)
		return -EINVAL;

	WRITE_ONCE(dma_demo_probe_status, -ENODEV);

	ret = platform_driver_register(&dma_mapping_demo_driver);
	if (ret)
		return ret;

	dma_demo_pdev = platform_device_register_full(&info);
	if (IS_ERR(dma_demo_pdev)) {
		ret = PTR_ERR(dma_demo_pdev);
		platform_driver_unregister(&dma_mapping_demo_driver);
		return ret;
	}

	ret = READ_ONCE(dma_demo_probe_status);
	if (ret) {
		platform_device_unregister(dma_demo_pdev);
		platform_driver_unregister(&dma_mapping_demo_driver);
		return ret;
	}

	pr_info("loaded: dma_bits=%u fail_step=%u\n", dma_bits, fail_step);
	return 0;
}

static void __exit dma_mapping_demo_exit(void)
{
	platform_device_unregister(dma_demo_pdev);
	platform_driver_unregister(&dma_mapping_demo_driver);
	pr_info("unloaded\n");
}

module_init(dma_mapping_demo_init);
module_exit(dma_mapping_demo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only coherent, streaming, and SG DMA mapping demo");
MODULE_LICENSE("GPL");
