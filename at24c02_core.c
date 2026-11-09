// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver for at24c02
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "at24c02_priv.h"

u8 reg = 0x00;
module_param(reg, byte, 0644);
MODULE_PARM_DESC(reg, "Address pointer of eeprom");

static ssize_t address_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sysfs_emit(buf, "0x%02x\n", reg);
}

static ssize_t address_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	u8 addr;
	ret = kstrtou8(buf, 16, &addr);
	if (ret < 0)
		return ret;

	reg = addr;
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
	u8 kbuf[EEPROM_CAPACITY];
	int i, ret, len = 0;

	ret = at24c02_i2c_read_data(to_i2c_client(dev), kbuf, sizeof(kbuf));
	if (ret < 0)
		return ret;

	for (i = 0; i < sizeof(kbuf); i++)
		len += sysfs_emit_at(buf, len, "0x%02x%s", kbuf[i],
				     i == ret - 1 ? "\n" : " ");

	return len;
}

static ssize_t eeprom_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	/* Extra bytes is to store register address */
	u8 kbuf[EEPROM_CAPACITY + 1], reg;
	char *token, *buf_copy, *tmp_buf;
	size_t remaining, tmp_len;
	size_t written = 0;
	int ret, len, i, tmp;

	i = len = 0;
	buf_copy = kstrndup(buf, count, GFP_KERNEL);
	if (!buf_copy)
		return -ENOMEM;

	tmp_buf = buf_copy;
	while ((token = strsep(&tmp_buf, " ")) != NULL) {
		ret = kstrtou8(token, 16, &kbuf[i]);
		if (ret < 0)
			return -EINVAL;
		pr_debug("token=%s, kbuf[%i]=0x%02x\n", token, i, kbuf[i]);
		/*
		 * If the data has more bytes than EEPROM_CAPACITY,
		 * that extra data will rollback to first data position of kbuf, i.e kbuf[1].
		 */
		if (i == EEPROM_CAPACITY) {
			len = EEPROM_CAPACITY + 1;
			i = 1;
		} else
			i++;
	}

	/* No data rollover happend. */
	if (len == 0)
		len = i;

	/* Only address pointer provided. */
	if (len == 1)
		return -EINVAL;
	kfree(buf_copy);

	pr_debug("i:%d, len:%d\n", i, len);

	reg = kbuf[0];
	remaining = len - 1;

	while (remaining) {
		reg += written;

		tmp = reg % EEPROM_PAGE_SIZE;
		if (tmp != 0)
			tmp_len = EEPROM_PAGE_SIZE - tmp;
		else
			tmp_len = remaining >= EEPROM_PAGE_SIZE ?
					  EEPROM_PAGE_SIZE :
					  remaining;

		pr_debug("tmp_len:%d written: %d, remaining:%d\n", tmp_len,
			 written, remaining);

		ret = at24c02_i2c_write_data(to_i2c_client(dev), reg,
					     kbuf + written + 1, tmp_len);
		if (ret < 0) {
			pr_err("i2c write failed: written:%d, remaining:%d\n",
			       written, remaining);
			return ret;
		}
		/*
		 * I2c write return no of bytes written,
		 * i.e Address byte + data bytes.
		 */
		written += ret - 1;
		remaining -= ret - 1;

		if (ret != tmp_len + 1) {
			pr_info("errrrrrrrrrrrrrrr\n");
			break;
		}
	}
	/*
		* If everything went ok (i.e. msg transferred), return #bytes
		* transferred, else error code.
		*/
	return (written == len - 1) ? count : -EINVAL;
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

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -EOPNOTSUPP;

	ret = device_add_group(&client->dev, &dev_attr_grp);
	if (ret) {
		pr_err("i2c client sysfs attr files creation failed\n");
		return ret;
	}
	pr_info("at24c02 probed\n");
	return 0;
}

static void at24c02_remove(struct i2c_client *client)
{
	device_remove_group(&client->dev, &dev_attr_grp);
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

static int __init at24c02_init(void)
{
	return i2c_add_driver(&at24c02_driver);
}

static void __exit at24c02_exit(void)
{
	i2c_del_driver(&at24c02_driver);
}

module_init(at24c02_init);
module_exit(at24c02_exit);

MODULE_AUTHOR("Ankit khushwaha <ankitkhushwaha.dev@gmail.com>");
MODULE_DESCRIPTION("I2C driver for 24c02 based on character device");
MODULE_LICENSE("GPL v2");
