// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver chardev core for at24c02
 */

#define pr_fmt(fmt) "%s :" fmt, __func__

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "at24c02_priv.h"

extern u8 reg;

static loff_t at24c02_llseek(struct file *filp, loff_t offset, int whence)
{
	u8 off = (u8)offset;

	switch (whence) {
	case SEEK_SET:
		reg = off;
		break;
	case SEEK_CUR:
		reg += off;
		break;
	case SEEK_END:
		reg -= off;
		break;
	default:
		return -EINVAL;
	}
	return reg;
}

static ssize_t at24c02_read(struct file *filp, char __user *buf, size_t count,
			    loff_t *f_pos)
{
	return 0;
}

static ssize_t at24c02_write(struct file *filp, const char __user *buf,
			     size_t count, loff_t *f_pos)
{
	return 0;
}

static int at24c02_open(struct inode *inode, struct file *filp)
{
	struct at24c02 *at24 =
		container_of(inode->i_cdev, struct at24c02, cdev);
	if (!at24)
		return -EINVAL;

	filp->private_data = at24;
	return 0;
}

static int at24c02_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static const struct file_operations at24c02_fops = {
	.owner = THIS_MODULE,
	.open = at24c02_open,
	.read = at24c02_read,
	.write = at24c02_write,
	.llseek = at24c02_llseek,
	.release = at24c02_release,
};

int at24c02_chrdev_register(struct at24c02 *at24)
{
	int ret;

	ret = alloc_chrdev_region(&at24->devt, 0, 1, MODULE_NAME);
	if (ret < 0) {
		pr_err("at24c02 chrdev allocation failed\n");
		return ret;
	}

	cdev_init(&at24->cdev, &at24c02_fops);
	at24->cdev.owner = THIS_MODULE;

	ret = cdev_add(&at24->cdev, at24->devt, 1);
	if (ret) {
		pr_err("at24c02 cdev init failed\n");
		goto unreg_chardev;
	}

	at24->class = class_create(MODULE_NAME);
	if (IS_ERR(at24->class)) {
		pr_err("at24c02 class creation failed\n");
		ret = PTR_ERR(at24->class);
		goto cdev_del;
	}

	at24->device =
		device_create(at24->class, NULL, at24->devt, NULL, MODULE_NAME);
	if (IS_ERR(at24->device)) {
		pr_err("at24c02 device creation failed\n");
		ret = PTR_ERR(at24->device);
		goto class_del;
	}
	return 0;

class_del:
	class_destroy(at24->class);
cdev_del:
	cdev_del(&at24->cdev);
unreg_chardev:
	unregister_chrdev_region(at24->devt, 1);
	return ret;
}

void at24c02_chrdev_unregister(struct at24c02 *at24)
{
	device_destroy(at24->class, at24->devt);
	class_destroy(at24->class);
	cdev_del(&at24->cdev);
	unregister_chrdev_region(at24->devt, 1);
}
