// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only synchronization example.
 *
 * Two kthreads update equivalent counters under a mutex, a spinlock, and an
 * atomic operation. The mutex also protects a two-field invariant.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define SYNC_DEMO_WORKERS	2
#define SYNC_DEMO_MAX_ITERATIONS	1000000U

static unsigned int iterations = 100000;
module_param(iterations, uint, 0444);
MODULE_PARM_DESC(iterations, "Updates performed by each worker");

static unsigned int timeout_ms = 5000;
module_param(timeout_ms, uint, 0444);
MODULE_PARM_DESC(timeout_ms, "Maximum wait for both workers in milliseconds");

struct sync_demo {
	/* pair_lock protects pair_left and pair_right as one invariant. */
	struct mutex pair_lock;
	u64 pair_left;
	u64 pair_right;

	/* fast_lock protects spin_count. Its section must not sleep. */
	spinlock_t fast_lock;
	u64 spin_count;

	/* atomic_count is an independent scalar statistic. */
	atomic64_t atomic_count;

	atomic_t workers_left;
	struct completion workers_done;
	struct task_struct *workers[SYNC_DEMO_WORKERS];
};

static struct sync_demo *demo;

static int sync_demo_worker(void *arg)
{
	struct sync_demo *d = arg;
	unsigned int i;

	for (i = 0; i < iterations && !kthread_should_stop(); i++) {
		mutex_lock(&d->pair_lock);
		d->pair_left++;
		d->pair_right += 2;
		mutex_unlock(&d->pair_lock);

		spin_lock(&d->fast_lock);
		d->spin_count++;
		spin_unlock(&d->fast_lock);

		atomic64_inc(&d->atomic_count);

		if (!(i & 0xfff))
			cond_resched();
	}

	if (atomic_dec_and_test(&d->workers_left))
		complete(&d->workers_done);

	while (!kthread_should_stop())
		schedule_timeout_interruptible(HZ);

	return 0;
}

static void sync_demo_stop_workers(struct sync_demo *d)
{
	unsigned int i;

	for (i = 0; i < SYNC_DEMO_WORKERS; i++) {
		if (d->workers[i]) {
			kthread_stop(d->workers[i]);
			d->workers[i] = NULL;
		}
	}
}

static int sync_demo_verify(struct sync_demo *d)
{
	unsigned long flags;
	u64 atomic_value;
	u64 expected;
	u64 pair_left;
	u64 pair_right;
	u64 spin_value;

	expected = (u64)iterations * SYNC_DEMO_WORKERS;

	mutex_lock(&d->pair_lock);
	pair_left = d->pair_left;
	pair_right = d->pair_right;
	mutex_unlock(&d->pair_lock);

	spin_lock_irqsave(&d->fast_lock, flags);
	spin_value = d->spin_count;
	spin_unlock_irqrestore(&d->fast_lock, flags);

	atomic_value = atomic64_read(&d->atomic_count);

	pr_info("expected=%llu mutex_pair=%llu/%llu spin=%llu atomic=%llu\n",
		expected, pair_left, pair_right, spin_value, atomic_value);

	if (pair_left != expected || pair_right != expected * 2 ||
	    spin_value != expected || atomic_value != expected)
		return -EUCLEAN;

	return 0;
}

static int __init sync_demo_init(void)
{
	unsigned long timeout;
	unsigned int i;
	int ret;

	if (!iterations || iterations > SYNC_DEMO_MAX_ITERATIONS ||
	    !timeout_ms)
		return -EINVAL;

	demo = kzalloc(sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	mutex_init(&demo->pair_lock);
	spin_lock_init(&demo->fast_lock);
	atomic64_set(&demo->atomic_count, 0);
	atomic_set(&demo->workers_left, SYNC_DEMO_WORKERS);
	init_completion(&demo->workers_done);

	for (i = 0; i < SYNC_DEMO_WORKERS; i++) {
		demo->workers[i] = kthread_run(sync_demo_worker, demo,
					       "sync_demo/%u", i);
		if (IS_ERR(demo->workers[i])) {
			ret = PTR_ERR(demo->workers[i]);
			demo->workers[i] = NULL;
			goto err_stop;
		}
	}

	timeout = wait_for_completion_timeout(&demo->workers_done,
					      msecs_to_jiffies(timeout_ms));
	if (!timeout) {
		ret = -ETIMEDOUT;
		goto err_stop;
	}

	ret = sync_demo_verify(demo);
	if (ret)
		goto err_stop;

	pr_info("loaded: workers=%u iterations=%u timeout_ms=%u\n",
		SYNC_DEMO_WORKERS, iterations, timeout_ms);
	return 0;

err_stop:
	sync_demo_stop_workers(demo);
	kfree(demo);
	demo = NULL;
	return ret;
}

static void __exit sync_demo_exit(void)
{
	sync_demo_stop_workers(demo);
	kfree(demo);
	demo = NULL;
	pr_info("unloaded\n");
}

module_init(sync_demo_init);
module_exit(sync_demo_exit);

MODULE_AUTHOR("Learning example");
MODULE_DESCRIPTION("Mutex, spinlock, and atomic synchronization demonstration");
MODULE_LICENSE("GPL");
