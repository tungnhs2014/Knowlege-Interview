// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only core kernel facilities example.
 *
 * A timer queues work. The work publishes state, wakes a waiter thread, and
 * signals a completion consumed by module init. The enclosing object also
 * carries an intrusive list node to demonstrate ownership and teardown.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/completion.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define COREFAC_DEFAULT_DELAY_MS	200
#define COREFAC_INIT_TIMEOUT_MS	2000

struct corefac_demo {
	struct list_head node;
	struct timer_list timer;
	struct work_struct work;
	wait_queue_head_t readq;
	struct completion work_done;
	struct task_struct *waiter;
	/* Protects ready, stopping, and value. */
	spinlock_t state_lock;
	bool ready;
	bool stopping;
	unsigned int value;
};

static LIST_HEAD(corefac_devices);
static DEFINE_MUTEX(corefac_devices_lock);
static struct corefac_demo *demo;

static unsigned int delay_ms = COREFAC_DEFAULT_DELAY_MS;
static bool skip_timer;

module_param(delay_ms, uint, 0444);
MODULE_PARM_DESC(delay_ms, "One-shot timer delay in milliseconds");

module_param(skip_timer, bool, 0444);
MODULE_PARM_DESC(skip_timer, "Skip timer arming to exercise init-timeout cleanup");

static unsigned int corefac_list_count(void)
{
	struct corefac_demo *entry;
	unsigned int count = 0;

	mutex_lock(&corefac_devices_lock);
	list_for_each_entry(entry, &corefac_devices, node)
		count++;
	mutex_unlock(&corefac_devices_lock);

	return count;
}

static int corefac_waiter_fn(void *data)
{
	struct corefac_demo *state = data;
	unsigned long flags;
	unsigned int value;
	int ret;

	ret = wait_event_interruptible(state->readq,
				       READ_ONCE(state->ready) ||
				       READ_ONCE(state->stopping) ||
				       kthread_should_stop());
	if (ret) {
		pr_info("waiter interrupted: %d\n", ret);
		return ret;
	}

	if (READ_ONCE(state->stopping) || kthread_should_stop()) {
		pr_info("waiter observed shutdown\n");
		return 0;
	}

	spin_lock_irqsave(&state->state_lock, flags);
	value = state->value;
	state->ready = false;
	spin_unlock_irqrestore(&state->state_lock, flags);

	pr_info("waiter consumed value=%u\n", value);
	return 0;
}

static void corefac_work_fn(struct work_struct *work)
{
	struct corefac_demo *state =
		container_of(work, struct corefac_demo, work);
	unsigned long flags;
	unsigned int value;

	spin_lock_irqsave(&state->state_lock, flags);
	if (state->stopping) {
		spin_unlock_irqrestore(&state->state_lock, flags);
		return;
	}

	state->value++;
	state->ready = true;
	value = state->value;
	spin_unlock_irqrestore(&state->state_lock, flags);

	pr_info("work published value=%u list_entries=%u\n",
		value, corefac_list_count());

	wake_up_interruptible(&state->readq);
	complete(&state->work_done);
}

static void corefac_timer_fn(struct timer_list *timer)
{
	struct corefac_demo *state = from_timer(state, timer, timer);

	pr_info("timer fired; queueing process-context work\n");
	if (!READ_ONCE(state->stopping) && !schedule_work(&state->work))
		pr_warn("work was already pending\n");
}

static void corefac_stop(struct corefac_demo *state)
{
	unsigned long flags;

	spin_lock_irqsave(&state->state_lock, flags);
	state->stopping = true;
	spin_unlock_irqrestore(&state->state_lock, flags);

	/*
	 * Stop the producer before draining the consumer. timer_shutdown_sync()
	 * also prevents a later rearm, which is useful in real cyclic designs.
	 */
	timer_shutdown_sync(&state->timer);
	cancel_work_sync(&state->work);

	wake_up_interruptible_all(&state->readq);
	if (state->waiter) {
		kthread_stop(state->waiter);
		state->waiter = NULL;
	}

	mutex_lock(&corefac_devices_lock);
	if (!list_empty(&state->node))
		list_del_init(&state->node);
	mutex_unlock(&corefac_devices_lock);
}

static int __init corefac_demo_init(void)
{
	unsigned long timeout;
	unsigned long timeout_jiffies;
	int ret;

	if (!delay_ms)
		return -EINVAL;

	demo = kzalloc(sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	INIT_LIST_HEAD(&demo->node);
	INIT_WORK(&demo->work, corefac_work_fn);
	init_waitqueue_head(&demo->readq);
	init_completion(&demo->work_done);
	spin_lock_init(&demo->state_lock);
	timer_setup(&demo->timer, corefac_timer_fn, 0);

	mutex_lock(&corefac_devices_lock);
	list_add_tail(&demo->node, &corefac_devices);
	mutex_unlock(&corefac_devices_lock);

	demo->waiter = kthread_run(corefac_waiter_fn, demo,
				   "corefac_waiter");
	if (IS_ERR(demo->waiter)) {
		ret = PTR_ERR(demo->waiter);
		demo->waiter = NULL;
		pr_err("failed to start waiter: %d\n", ret);
		goto err_stop;
	}

	pr_info("loaded delay_ms=%u skip_timer=%d list_entries=%u\n",
		delay_ms, skip_timer, corefac_list_count());

	if (!skip_timer)
		mod_timer(&demo->timer,
			  jiffies + msecs_to_jiffies(delay_ms));

	timeout_jiffies = msecs_to_jiffies(COREFAC_INIT_TIMEOUT_MS);
	timeout = wait_for_completion_timeout(&demo->work_done, timeout_jiffies);
	if (!timeout) {
		ret = -ETIMEDOUT;
		pr_err("work completion timed out: %d\n", ret);
		goto err_stop;
	}

	pr_info("init observed completion with %lu jiffies remaining\n",
		timeout);
	return 0;

err_stop:
	corefac_stop(demo);
	kfree(demo);
	demo = NULL;
	return ret;
}

static void __exit corefac_demo_exit(void)
{
	pr_info("exit start\n");

	corefac_stop(demo);
	kfree(demo);
	demo = NULL;

	pr_info("unloaded list_entries=%u\n", corefac_list_count());
}

module_init(corefac_demo_init);
module_exit(corefac_demo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only core kernel facilities lifecycle demo");
MODULE_LICENSE("GPL");
