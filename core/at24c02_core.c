// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver for at24c02
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "at24c02_priv.h"

static ssize_t address_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct at24c02 *at24 = i2c_get_clientdata(to_i2c_client(dev));
	return sysfs_emit(buf, "0x%02x\n", at24->reg);
}

static ssize_t address_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct at24c02 *at24 = i2c_get_clientdata(to_i2c_client(dev));
	int ret;
	u8 addr;

	ret = kstrtou8(buf, 16, &addr);
	if (ret < 0)
		return ret;

	at24->reg = addr;
	return count;
}

static DEVICE_ATTR_RW(address);

static ssize_t capacity_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sysfs_emit(buf, "%d\n", EEPROM_CAPACITY);
}

static DEVICE_ATTR_RO(capacity);

static ssize_t page_size_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	return sysfs_emit(buf, "%d\n", EEPROM_PAGE_SIZE);
}

static DEVICE_ATTR_RO(page_size);

static ssize_t eeprom_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct at24c02 *at24 = i2c_get_clientdata(to_i2c_client(dev));
	return at24c02_i2c_read_data(to_i2c_client(dev), at24->reg, buf,
				     EEPROM_CAPACITY);
}

static ssize_t eeprom_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	return at24c02_i2c_write_data(to_i2c_client(dev), buf, count);
}
static DEVICE_ATTR_RW(eeprom);

static struct attribute *dev_attrs[] = {
	&dev_attr_address.attr,
	&dev_attr_eeprom.attr,
	&dev_attr_capacity.attr,
	&dev_attr_page_size.attr,
	NULL,
};

static const struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};

static int at24c02_probe(struct i2c_client *client)
{
	int ret;
	struct at24c02 *at24;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -EOPNOTSUPP;

	at24 = devm_kmalloc(&client->dev, sizeof(*at24), GFP_KERNEL);
	if (!at24)
		return -ENOMEM;

	if (IS_ENABLED(CONFIG_AT24C02_CHARDEV)) {
		at24->buf = devm_kmalloc(&client->dev, PAGE_SIZE, GFP_KERNEL);
		if (!at24->buf)
			return -ENOMEM;

		ret = at24c02_chrdev_register(at24);
		if (ret) {
			dev_err(&client->dev, "chrdev registration failed\n");
			return ret;
		}
	}

	at24->client = client;
	at24->reg = BASE_ADDR;
	i2c_set_clientdata(client, at24);

	ret = devm_device_add_group(&client->dev, &dev_attr_grp);
	if (ret) {
		pr_err("i2c client sysfs attr files creation failed\n");
		return ret;
	}

	pr_info("at24c02 probed\n");
	return 0;
}

static void at24c02_remove(struct i2c_client *client)
{
	struct at24c02 *at24 = i2c_get_clientdata(client);

	if (IS_ENABLED(CONFIG_AT24C02_CHARDEV))
		at24c02_chrdev_unregister(at24);

	at24->client = NULL;
	pr_info("at24c02 removed\n");
}

static const struct i2c_device_id at24c02_idtable[] = {
	{ "ee24c02" },
	{},
};
MODULE_DEVICE_TABLE(i2c, at24c02_idtable);

static const struct of_device_id at24c02_of_device_table[] = {
	{ .compatible = "ee24c02" },
	{},
};
MODULE_DEVICE_TABLE(of, at24c02_of_device_table);

static struct i2c_driver at24c02_driver = {
    .probe = at24c02_probe,
    .remove = at24c02_remove,
    .driver =
        {
            .name = MODULE_NAME,
            .of_match_table = at24c02_of_device_table,
        },
    .id_table = at24c02_idtable,
};

module_i2c_driver(at24c02_driver);

MODULE_AUTHOR("Ankit khushwaha <ankitkhushwaha.dev@gmail.com>");
MODULE_DESCRIPTION("I2C driver for 24c02 based on character device");
MODULE_LICENSE("GPL v2");
