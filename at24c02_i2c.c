// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver core for at24c02
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "at24c02_priv.h"

extern u8 reg;

static int _at24c02_i2c_read_data(struct i2c_client *client, u8 *buf, int count)
{
	u8 addr = 0x00;
	int ret;

	/* send address to read */
	ret = i2c_master_send(client, &addr, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;

	ret = i2c_master_recv(client, buf, count);
	if (ret < 0)
		return ret;
	if (ret != count)
		return -EIO;

	return ret;
}

int at24c02_i2c_read_data(struct i2c_client *client, u8 *buf, int count)
{
	u8 kbuf[EEPROM_CAPACITY];
	int i, ret, len = 0;

	ret = _at24c02_i2c_read_data(client, kbuf, count);
	if (ret < 0)
		return ret;

	for (i = 0; i < count; i++)
		len += sysfs_emit_at(buf, len, "0x%02x%s", kbuf[i],
				     i == ret - 1 ? "\n" : " ");

	return len;
}

int at24c02_i2c_write_data(struct i2c_client *client, u8 reg, u8 *buf,
			   int count)
{
	int ret, i, len = 0;
	u8 kbuf[EEPROM_PAGE_SIZE + 1];

	kbuf[0] = reg;
	memcpy(kbuf + 1, buf, count);

	pr_debug("%s: reg:0x%02x, buffer length:%d\n", __func__, reg, count);

	ret = i2c_master_send(client, kbuf, count + 1);
	if (ret < 0) {
		pr_err("i2c write transfer failed: %d\n", ret);
		return ret;
	}
	if (ret != count + 1) {
		pr_err("%s: i2c write failed: ret:%d\n", __func__, ret);
		return -EIO;
	}

	/*
	 * Sleep for EEPROM page write internal cycle.
	 * 10 ms ~ 2x of page write internal cycle time
	 * to keep it safer side.
	 */
	msleep(30);
	pr_debug("%s: i2c write return value:%d\n", __func__, ret);
	return ret;
}
