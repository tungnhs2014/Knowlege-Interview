// SPDX-License-Identifier: GPL-2.0
/*
 * demochar.c - learning-only RAM-backed character device.
 *
 * This example teaches the core character-device lifecycle:
 * dev_t allocation, cdev registration, /dev publication, file callbacks,
 * user-copy, seeking, mutex locking, and reverse-order cleanup.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

#define DEMOCHAR_NAME "demochar"
#define DEMOCHAR_SIZE 128

struct demochar_dev {
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct mutex lock;
	char buf[DEMOCHAR_SIZE];
	size_t len;
};

static struct demochar_dev demochar;

static int demochar_open(struct inode *inode, struct file *filp)
{
	struct demochar_dev *dev;

	dev = container_of(inode->i_cdev, struct demochar_dev, cdev);
	filp->private_data = dev;

	pr_debug("%s: open minor=%u\n", DEMOCHAR_NAME, iminor(inode));
	return 0;
}

static int demochar_release(struct inode *inode, struct file *filp)
{
	pr_debug("%s: release minor=%u\n", DEMOCHAR_NAME, iminor(inode));
	return 0;
}

static ssize_t demochar_read(struct file *filp, char __user *ubuf,
			     size_t count, loff_t *ppos)
{
	struct demochar_dev *dev = filp->private_data;
	size_t available;
	size_t n;
	size_t not_copied;
	size_t done;
	ssize_t ret;

	if (count == 0)
		return 0;

	if (*ppos < 0)
		return -EINVAL;

	mutex_lock(&dev->lock);

	if (*ppos >= dev->len) {
		ret = 0;
		goto out_unlock;
	}

	available = dev->len - (size_t)*ppos;
	n = min(count, available);

	not_copied = copy_to_user(ubuf, dev->buf + *ppos, n);
	done = n - not_copied;
	*ppos += done;

	if (not_copied && !done)
		ret = -EFAULT;
	else
		ret = done;

out_unlock:
	mutex_unlock(&dev->lock);
	return ret;
}

static ssize_t demochar_write(struct file *filp, const char __user *ubuf,
			      size_t count, loff_t *ppos)
{
	struct demochar_dev *dev = filp->private_data;
	size_t space;
	size_t n;
	size_t not_copied;
	size_t done;
	ssize_t ret;

	if (count == 0)
		return 0;

	if (*ppos < 0)
		return -EINVAL;

	mutex_lock(&dev->lock);

	if (*ppos >= DEMOCHAR_SIZE) {
		ret = -ENOSPC;
		goto out_unlock;
	}

	space = DEMOCHAR_SIZE - (size_t)*ppos;
	n = min(count, space);

	not_copied = copy_from_user(dev->buf + *ppos, ubuf, n);
	done = n - not_copied;
	*ppos += done;

	if (dev->len < (size_t)*ppos)
		dev->len = (size_t)*ppos;

	if (not_copied && !done)
		ret = -EFAULT;
	else
		ret = done;

out_unlock:
	mutex_unlock(&dev->lock);
	return ret;
}

static loff_t demochar_llseek(struct file *filp, loff_t offset, int whence)
{
	return fixed_size_llseek(filp, offset, whence, DEMOCHAR_SIZE);
}

static const struct file_operations demochar_fops = {
	.owner = THIS_MODULE,
	.open = demochar_open,
	.release = demochar_release,
	.read = demochar_read,
	.write = demochar_write,
	.llseek = demochar_llseek,
};

static int __init demochar_init(void)
{
	int ret;

	mutex_init(&demochar.lock);
	demochar.len = 0;

	ret = alloc_chrdev_region(&demochar.devt, 0, 1, DEMOCHAR_NAME);
	if (ret) {
		pr_err("%s: alloc_chrdev_region failed: %d\n",
		       DEMOCHAR_NAME, ret);
		return ret;
	}

	cdev_init(&demochar.cdev, &demochar_fops);
	demochar.cdev.owner = THIS_MODULE;

	ret = cdev_add(&demochar.cdev, demochar.devt, 1);
	if (ret) {
		pr_err("%s: cdev_add failed: %d\n", DEMOCHAR_NAME, ret);
		goto err_unregister;
	}

	demochar.class = class_create(DEMOCHAR_NAME);
	if (IS_ERR(demochar.class)) {
		ret = PTR_ERR(demochar.class);
		pr_err("%s: class_create failed: %d\n", DEMOCHAR_NAME, ret);
		goto err_cdev;
	}

	demochar.device = device_create(demochar.class, NULL, demochar.devt,
					NULL, DEMOCHAR_NAME "%d",
					MINOR(demochar.devt));
	if (IS_ERR(demochar.device)) {
		ret = PTR_ERR(demochar.device);
		pr_err("%s: device_create failed: %d\n", DEMOCHAR_NAME, ret);
		goto err_class;
	}

	pr_info("%s: loaded major=%u minor=%u node=/dev/%s%u size=%u\n",
		DEMOCHAR_NAME, MAJOR(demochar.devt), MINOR(demochar.devt),
		DEMOCHAR_NAME, MINOR(demochar.devt), DEMOCHAR_SIZE);

	return 0;

err_class:
	class_destroy(demochar.class);
err_cdev:
	cdev_del(&demochar.cdev);
err_unregister:
	unregister_chrdev_region(demochar.devt, 1);
	return ret;
}

static void __exit demochar_exit(void)
{
	device_destroy(demochar.class, demochar.devt);
	class_destroy(demochar.class);
	cdev_del(&demochar.cdev);
	unregister_chrdev_region(demochar.devt, 1);

	pr_info("%s: unloaded\n", DEMOCHAR_NAME);
}

module_init(demochar_init);
module_exit(demochar_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only RAM-backed character device");
MODULE_LICENSE("GPL");
