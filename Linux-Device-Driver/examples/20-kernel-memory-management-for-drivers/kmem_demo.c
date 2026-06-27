// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only kernel memory management example.
 *
 * The module allocates:
 * - one small state object with kzalloc();
 * - an overflow-safe array with kcalloc();
 * - a larger software-only buffer with kvzalloc();
 * - one page with alloc_page().
 *
 * Delayed work reads the allocations. Module exit synchronously cancels that
 * consumer before releasing storage in reverse acquisition order.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/overflow.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#define KMEM_DEMO_DEFAULT_ENTRIES	128U
#define KMEM_DEMO_MAX_ENTRIES		65536U
#define KMEM_DEMO_DEFAULT_BLOB_KB	256U
#define KMEM_DEMO_MAX_BLOB_KB		16384U
#define KMEM_DEMO_DEFAULT_DELAY_MS	200U

struct kmem_demo_entry {
	u32 id;
	u32 value;
};

struct kmem_demo {
	struct delayed_work work;
	struct completion work_done;
	struct kmem_demo_entry *entries;
	void *blob;
	struct page *page;
	size_t blob_size;
	unsigned int entry_count;
};

static struct kmem_demo *demo;

static unsigned int entries = KMEM_DEMO_DEFAULT_ENTRIES;
module_param(entries, uint, 0444);
MODULE_PARM_DESC(entries, "Number of array entries (1..65536)");

static unsigned int blob_kb = KMEM_DEMO_DEFAULT_BLOB_KB;
module_param(blob_kb, uint, 0444);
MODULE_PARM_DESC(blob_kb, "Software-only kvzalloc buffer size in KiB (1..16384)");

static unsigned int delay_ms = KMEM_DEMO_DEFAULT_DELAY_MS;
module_param(delay_ms, uint, 0444);
MODULE_PARM_DESC(delay_ms, "Delay before the work callback runs in milliseconds");

static unsigned int fail_step;
module_param(fail_step, uint, 0444);
MODULE_PARM_DESC(fail_step, "Inject init failure after allocation step 1..4");

static int kmem_demo_injected_failure(unsigned int step)
{
	if (fail_step != step)
		return 0;

	pr_info("injecting failure after allocation step %u\n", step);
	return -EIO;
}

static void kmem_demo_work(struct work_struct *work)
{
	struct kmem_demo *state =
		container_of(to_delayed_work(work), struct kmem_demo, work);
	u64 sum = 0;
	unsigned int i;
	u8 *blob = state->blob;

	for (i = 0; i < state->entry_count; i++)
		sum += state->entries[i].value;

	/*
	 * The blob may have kmalloc or vmalloc backing. Byte access is valid for
	 * either, and kvfree() will release either form.
	 */
	blob[0] = 0xa5;
	blob[state->blob_size - 1] = 0x5a;

	pr_info("work: entries=%u sum=%llu blob=%zu bytes edge=%02x/%02x page_pfn=%lu\n",
		state->entry_count, sum, state->blob_size, blob[0],
		blob[state->blob_size - 1], page_to_pfn(state->page));

	complete(&state->work_done);
}

static void kmem_demo_free(struct kmem_demo *state)
{
	if (!state)
		return;

	if (state->page) {
		__free_page(state->page);
		state->page = NULL;
		pr_info("cleanup: page released\n");
	}

	if (state->blob) {
		kvfree(state->blob);
		state->blob = NULL;
		pr_info("cleanup: blob released with kvfree\n");
	}

	if (state->entries)
		pr_info("cleanup: entry array released with kfree\n");
	kfree(state->entries);
	state->entries = NULL;

	kfree(state);
	pr_info("cleanup: state released with kfree\n");
}

static int __init kmem_demo_init(void)
{
	unsigned long delay;
	unsigned int i;
	int ret;

	if (!entries || entries > KMEM_DEMO_MAX_ENTRIES ||
	    !blob_kb || blob_kb > KMEM_DEMO_MAX_BLOB_KB ||
	    fail_step > 4)
		return -EINVAL;

	demo = kzalloc(sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	ret = kmem_demo_injected_failure(1);
	if (ret)
		goto err_free;

	demo->entries = kcalloc(entries, sizeof(*demo->entries), GFP_KERNEL);
	if (!demo->entries) {
		ret = -ENOMEM;
		goto err_free;
	}
	demo->entry_count = entries;

	ret = kmem_demo_injected_failure(2);
	if (ret)
		goto err_free;

	demo->blob_size = (size_t)blob_kb * SZ_1K;
	demo->blob = kvzalloc(demo->blob_size, GFP_KERNEL);
	if (!demo->blob) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = kmem_demo_injected_failure(3);
	if (ret)
		goto err_free;

	demo->page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!demo->page) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = kmem_demo_injected_failure(4);
	if (ret)
		goto err_free;

	for (i = 0; i < demo->entry_count; i++) {
		demo->entries[i].id = i;
		demo->entries[i].value = i + 1;
	}

	init_completion(&demo->work_done);
	INIT_DELAYED_WORK(&demo->work, kmem_demo_work);

	delay = msecs_to_jiffies(delay_ms);
	schedule_delayed_work(&demo->work, delay);

	pr_info("loaded: entries=%u array_bytes=%zu blob=%zu bytes delay_ms=%u\n",
		demo->entry_count,
		array_size(demo->entry_count, sizeof(*demo->entries)),
		demo->blob_size, delay_ms);
	pr_info("allocation roles: kzalloc state, kcalloc array, kvzalloc blob, alloc_page page\n");

	return 0;

err_free:
	kmem_demo_free(demo);
	demo = NULL;
	return ret;
}

static void __exit kmem_demo_exit(void)
{
	bool completed;

	/*
	 * Work is a consumer of every allocation. Synchronize it before freeing
	 * any storage. The return value tells whether pending work was canceled.
	 */
	completed = completion_done(&demo->work_done);
	if (cancel_delayed_work_sync(&demo->work))
		pr_info("exit: pending work canceled before it ran\n");
	else if (completed)
		pr_info("exit: work had completed before teardown\n");
	else
		pr_info("exit: work callback synchronized during teardown\n");

	kmem_demo_free(demo);
	demo = NULL;
	pr_info("unloaded\n");
}

module_init(kmem_demo_init);
module_exit(kmem_demo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only kernel allocation and lifetime demonstration");
MODULE_LICENSE("GPL");
