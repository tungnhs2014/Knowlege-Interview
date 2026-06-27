// SPDX-License-Identifier: GPL-2.0
/*
 * abidemo.c - learning-only userspace ABI demo driver.
 *
 * Exposes:
 * - /dev/abidemo0 for read/write/poll/ioctl
 * - /sys/class/abidemo/abidemo0/mode for one simple property
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "abidemo_uapi.h"

#define ABIDEMO_NAME		"abidemo"
#define ABIDEMO_NODE		"abidemo0"
#define ABIDEMO_BUF_SIZE	128
#define ABIDEMO_VERSION		1

struct abidemo_dev {
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct mutex lock;
	wait_queue_head_t read_wq;
	char buf[ABIDEMO_BUF_SIZE];
	size_t len;
	bool data_ready;
	__u32 mode;
};

static struct abidemo_dev abidemo;

static ssize_t mode_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct abidemo_dev *demo = dev_get_drvdata(dev);
	__u32 mode;

	mutex_lock(&demo->lock);
	mode = demo->mode;
	mutex_unlock(&demo->lock);

	return sysfs_emit(buf, "%u\n", mode);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct abidemo_dev *demo = dev_get_drvdata(dev);
	unsigned int mode;
	int ret;

	ret = kstrtouint(buf, 0, &mode);
	if (ret)
		return ret;

	if (mode > 1)
		return -EINVAL;

	mutex_lock(&demo->lock);
	demo->mode = mode;
	mutex_unlock(&demo->lock);

	return count;
}
static DEVICE_ATTR_RW(mode);

static int abidemo_open(struct inode *inode, struct file *filp)
{
	struct abidemo_dev *demo;

	demo = container_of(inode->i_cdev, struct abidemo_dev, cdev);
	filp->private_data = demo;

	pr_debug("%s: open minor=%u\n", ABIDEMO_NAME, iminor(inode));
	return 0;
}

static ssize_t abidemo_read(struct file *filp, char __user *ubuf,
			    size_t count, loff_t *ppos)
{
	struct abidemo_dev *demo = filp->private_data;
	size_t n;
	int ret;

	if (count == 0)
		return 0;

	if (*ppos != 0)
		return 0;

	if (filp->f_flags & O_NONBLOCK) {
		mutex_lock(&demo->lock);
		ret = demo->data_ready ? 0 : -EAGAIN;
		mutex_unlock(&demo->lock);
		if (ret)
			return ret;
	} else {
		ret = wait_event_interruptible(demo->read_wq, demo->data_ready);
		if (ret)
			return ret;
	}

	mutex_lock(&demo->lock);
	if (!demo->data_ready) {
		ret = -EAGAIN;
		goto out_unlock;
	}

	n = min(count, demo->len);
	if (copy_to_user(ubuf, demo->buf, n)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	demo->data_ready = false;
	*ppos += n;
	ret = n;

out_unlock:
	mutex_unlock(&demo->lock);
	return ret;
}

static ssize_t abidemo_write(struct file *filp, const char __user *ubuf,
			     size_t count, loff_t *ppos)
{
	struct abidemo_dev *demo = filp->private_data;
	size_t n;
	int ret;

	if (count == 0)
		return 0;

	n = min(count, (size_t)ABIDEMO_BUF_SIZE);

	mutex_lock(&demo->lock);
	if (copy_from_user(demo->buf, ubuf, n)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	demo->len = n;
	demo->data_ready = true;
	ret = n;

out_unlock:
	mutex_unlock(&demo->lock);

	if (ret > 0)
		wake_up_interruptible(&demo->read_wq);

	return ret;
}

static __poll_t abidemo_poll(struct file *filp, poll_table *wait)
{
	struct abidemo_dev *demo = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &demo->read_wq, wait);

	mutex_lock(&demo->lock);
	if (demo->data_ready)
		mask |= EPOLLIN | EPOLLRDNORM;
	mutex_unlock(&demo->lock);

	return mask;
}

static long abidemo_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct abidemo_dev *demo = filp->private_data;
	struct abidemo_config cfg;

	if (_IOC_TYPE(cmd) != ABIDEMO_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case ABIDEMO_IOC_GET_CONFIG:
		memset(&cfg, 0, sizeof(cfg));

		mutex_lock(&demo->lock);
		cfg.version = ABIDEMO_VERSION;
		cfg.mode = demo->mode;
		mutex_unlock(&demo->lock);

		if (copy_to_user((void __user *)arg, &cfg, sizeof(cfg)))
			return -EFAULT;
		return 0;

	case ABIDEMO_IOC_SET_CONFIG:
		if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg)))
			return -EFAULT;

		if (cfg.version != ABIDEMO_VERSION || cfg.mode > 1 ||
		    cfg.flags || cfg.reserved[0] || cfg.reserved[1] ||
		    cfg.reserved[2] || cfg.reserved[3] || cfg.reserved[4])
			return -EINVAL;

		mutex_lock(&demo->lock);
		demo->mode = cfg.mode;
		mutex_unlock(&demo->lock);
		return 0;

	case ABIDEMO_IOC_CLEAR_EVENT:
		mutex_lock(&demo->lock);
		demo->data_ready = false;
		demo->len = 0;
		mutex_unlock(&demo->lock);
		return 0;

	default:
		return -ENOTTY;
	}
}

static const struct file_operations abidemo_fops = {
	.owner = THIS_MODULE,
	.open = abidemo_open,
	.read = abidemo_read,
	.write = abidemo_write,
	.poll = abidemo_poll,
	.unlocked_ioctl = abidemo_ioctl,
	.llseek = no_llseek,
};

static int __init abidemo_init(void)
{
	int ret;

	mutex_init(&abidemo.lock);
	init_waitqueue_head(&abidemo.read_wq);
	abidemo.mode = 0;

	ret = alloc_chrdev_region(&abidemo.devt, 0, 1, ABIDEMO_NAME);
	if (ret)
		return ret;

	cdev_init(&abidemo.cdev, &abidemo_fops);
	abidemo.cdev.owner = THIS_MODULE;

	ret = cdev_add(&abidemo.cdev, abidemo.devt, 1);
	if (ret)
		goto err_unregister;

	abidemo.class = class_create(ABIDEMO_NAME);
	if (IS_ERR(abidemo.class)) {
		ret = PTR_ERR(abidemo.class);
		goto err_cdev;
	}

	abidemo.device = device_create(abidemo.class, NULL, abidemo.devt,
				       &abidemo, ABIDEMO_NODE);
	if (IS_ERR(abidemo.device)) {
		ret = PTR_ERR(abidemo.device);
		goto err_class;
	}

	ret = device_create_file(abidemo.device, &dev_attr_mode);
	if (ret)
		goto err_device;

	pr_info("%s: loaded node=/dev/%s sysfs=/sys/class/%s/%s/mode\n",
		ABIDEMO_NAME, ABIDEMO_NODE, ABIDEMO_NAME, ABIDEMO_NODE);
	return 0;

err_device:
	device_destroy(abidemo.class, abidemo.devt);
err_class:
	class_destroy(abidemo.class);
err_cdev:
	cdev_del(&abidemo.cdev);
err_unregister:
	unregister_chrdev_region(abidemo.devt, 1);
	return ret;
}

static void __exit abidemo_exit(void)
{
	device_remove_file(abidemo.device, &dev_attr_mode);
	device_destroy(abidemo.class, abidemo.devt);
	class_destroy(abidemo.class);
	cdev_del(&abidemo.cdev);
	unregister_chrdev_region(abidemo.devt, 1);

	pr_info("%s: unloaded\n", ABIDEMO_NAME);
}

module_init(abidemo_init);
module_exit(abidemo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only userspace ABI design demo");
MODULE_LICENSE("GPL");
