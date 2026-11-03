#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "at24c02_priv.h"

#ifdef CONFIG_AT24C02_CHARDEV
struct at24c02_char_priv {
	struct i2c_client *client;
	struct cdev *cdev;
};

static dev_t at24c02_devt;
struct class *at24c02_class;
struct device *at24c02_device;

static struct at24c02_char_priv at24c02_char_priv;

static loff_t at24c02_llseek(struct file *filp, loff_t offset, int whence)
{
	return 0;
}

static ssize_t at24c02_read(struct file *filp, char __user *buff, size_t count,
			    loff_t *f_pos)
{
	return 0;
}

static ssize_t at24c02_write(struct file *filp, const char __user *buff,
			     size_t count, loff_t *f_pos)
{
	return 0;
}

static int at24c02_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int at24c02_release(struct inode *inode, struct file *filp)
{
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

int at24c02_chrdev_register(void)
{
	int ret;

	ret = alloc_chrdev_region(&at24c02_devt, 0, 1, MODULE_NAME);
	if (ret < 0) {
		pr_err("at24c02 chrdev allocation failed\n");
		return ret;
	}

	cdev_init(at24c02_char_priv.cdev, &at24c02_fops);
	at24c02_char_priv.cdev->owner = THIS_MODULE;

	ret = cdev_add(at24c02_char_priv.cdev, at24c02_devt, 1);
	if (ret) {
		pr_err("at24c02 cdev init failed\n");
		goto unreg_chardev;
	}

	at24c02_class = class_create(MODULE_NAME);
	if (IS_ERR(at24c02_class)) {
		pr_err("at24c02 class creation failed\n");
		ret = PTR_ERR(at24c02_class);
		goto cdev_del;
	}

	at24c02_device = device_create(at24c02_class, NULL, at24c02_devt, NULL,
				       MODULE_NAME);
	if (IS_ERR(at24c02_device)) {
		pr_err("at24c02 device creation failed\n");
		ret = PTR_ERR(at24c02_device);
		goto class_del;
	}
	return 0;

class_del:
	class_destroy(at24c02_class);
cdev_del:
	cdev_del(at24c02_char_priv.cdev);
unreg_chardev:
	unregister_chrdev_region(at24c02_devt, 1);
	return ret;
}

void at24c02_chrdev_unregister(void)
{
	device_destroy(at24c02_class, at24c02_devt);
	class_destroy(at24c02_class);
	cdev_del(at24c02_char_priv.cdev);
	unregister_chrdev_region(at24c02_devt, 1);
}
#endif /* CONFIG_AT24C02_CHARDEV */
