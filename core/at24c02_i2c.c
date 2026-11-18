// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver core for at24c02
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "at24c02_priv.h"

static int _at24c02_i2c_read_data(struct i2c_client *client, u8 reg, u8 *buf,
				  int count)
{
	int ret;

	/* send address to read */
	ret = i2c_master_send(client, &reg, 1);
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

int at24c02_i2c_read_data(struct i2c_client *client, u8 reg, u8 *buf, int count)
{
	struct at24c02 *at24 = i2c_get_clientdata(client);
	int ret;

	if (!count)
		return 0;

	mutex_lock(&at24->lock);
	ret = _at24c02_i2c_read_data(client, reg, buf, count);
	if (ret < 0)
		goto fail;

	/* update the address pointer */
	at24->reg = reg + ret;

fail:
	mutex_unlock(&at24->lock);
	return ret;
}

static int _at24c02_i2c_write_data(struct i2c_client *client, u8 reg, u8 *buf,
				   int count)
{
	int ret, buf_len = count + 1;
	u8 kbuf[EEPROM_PAGE_SIZE + 1] = { 0 };

	kbuf[0] = reg;
	memcpy(kbuf + 1, buf, count);

	pr_debug("%s: reg:0x%02x, buffer length:%d\n", __func__, reg, count);

	ret = i2c_master_send(client, kbuf, buf_len);
	if (ret < 0) {
		pr_err("i2c write transfer failed: %d\n", ret);
		return ret;
	}
	if (ret != buf_len) {
		dev_err(&client->dev, "%s: i2c write failed: ret:%d\n",
			__func__, ret);
		return -EIO;
	}

	/*
	 * Sleep for EEPROM page write internal cycle.
	 * 10 ms ~ 2x of page write internal cycle time
	 * to keep it safer side.
	 */
	msleep(10);
	dev_info(&client->dev, "i2c write success, ret=%d\n", ret);
	return ret;
}

int at24c02_i2c_write_data(struct i2c_client *client, u8 *buf, size_t count)
{
	struct at24c02 *at24 = i2c_get_clientdata(client);
	/* Extra bytes is to store register address */
	u8 base_addr, reg;
	size_t remaining, tmp_len, room;
	size_t written = 0;
	int ret;

	base_addr = buf[0];
	remaining = count - 1;

	dev_info(&client->dev, "base_addr:0x%02x count: %ld\n", base_addr,
		 count);

	mutex_lock(&at24->lock);
	while (remaining) {
		reg = base_addr + written;
		room = EEPROM_PAGE_SIZE - (reg % EEPROM_PAGE_SIZE);

		tmp_len = min(remaining, room);

		pr_debug("tmp_len:%ld written: %ld, remaining:%ld\n", tmp_len,
			 written, remaining);

		ret = _at24c02_i2c_write_data(client, reg, buf + written + 1,
					      tmp_len);
		if (ret < 0) {
			pr_err("i2c write failed: written:%ld, remaining:%ld, ret=%d\n",
			       written, remaining, ret);
			goto fail;
		}

		/* update the address pointer */
		at24->reg = reg + tmp_len;

		/*
		 * I2c write return no of bytes written,
		 * i.e Address byte + data bytes.
		 */
		written += tmp_len;
		remaining -= tmp_len;
	}

	/*
	* If everything went ok (i.e. msg transferred), return #bytes
	* transferred, else error code.
	*/
	ret = written;
fail:
	mutex_lock(&at24->lock);
	return ret;
}
