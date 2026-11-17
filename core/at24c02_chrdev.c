// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver chardev core for at24c02
 */

#define pr_fmt(fmt) "%s :" fmt, __func__

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

#include "at24c02_priv.h"

static loff_t at24c02_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = filp->f_pos + offset;
		break;
	case SEEK_END:
		newpos = EEPROM_CAPACITY + offset;
		break;
	default:
		return -EINVAL;
	}

	if (newpos < 0 || newpos > EEPROM_CAPACITY)
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;
}

static ssize_t at24c02_read(struct file *filp, char __user *buf, size_t count,
			    loff_t *f_pos)
{
	struct at24c02 *at24 = filp->private_data;
	struct i2c_client *client = at24->client;
	size_t len;
	int ret;

	if (*f_pos >= EEPROM_CAPACITY)
		return 0;

	len = min_t(size_t, count, EEPROM_CAPACITY - *f_pos);

	pr_debug("before f_pos: %llu, count given: %zu, count_f:%zu\n", *f_pos,
		 count, len);

	ret = at24c02_i2c_read_data(client, BASE_ADDR + *f_pos, at24->buf, len);
	if (ret < 0)
		return ret;

	if (copy_to_user(buf, at24->buf, ret) > 0) {
		pr_warn("copy_to_user failed to read\n");
		return -EINVAL;
	}
	*f_pos += ret;

	pr_debug("after f_pos: %llu, count: %zu, ret:%d\n", *f_pos, len, ret);
	return len;
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

	at24->buf = devm_kmalloc(&at24->client->dev, PAGE_SIZE, GFP_KERNEL);
	if (!at24->buf)
		return -ENOMEM;

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
