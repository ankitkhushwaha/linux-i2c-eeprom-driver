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

int at24c02_i2c_read_data(struct i2c_client *client, u8 reg, char *buf,
			  int count)
{
	u8 kbuf[EEPROM_CAPACITY];
	int i, ret, len = 0;

	ret = _at24c02_i2c_read_data(client, reg, kbuf, count);
	if (ret < 0)
		return ret;

	for (i = 0; i < count; i++)
		len += sysfs_emit_at(buf, len, "0x%02x%s", kbuf[i],
				     i == ret - 1 ? "\n" : " ");

	return len;
}

static int _at24c02_i2c_write_data(struct i2c_client *client, u8 reg, u8 *buf,
				   int count)
{
	int ret, buf_len = count + 1;
	u8 kbuf[EEPROM_PAGE_SIZE + 1];

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

int at24c02_i2c_write_data(struct i2c_client *client, const char *buf,
			   size_t count)
{
	/* Extra bytes is to store register address */
	u8 kbuf[EEPROM_CAPACITY + 1], base_addr, reg;
	char *token, *buf_copy, *tmp_buf;
	size_t remaining, tmp_len, room;
	size_t written = 0;
	int ret, len, i;

	if (count >= PAGE_SIZE - 1) {
		dev_err(&client->dev,
			"input too long (%zu bytes), possibly truncated\n",
			count);
		return -E2BIG;
	}

	i = len = 0;
	buf_copy = kstrndup(buf, count, GFP_KERNEL);
	if (!buf_copy)
		return -ENOMEM;

	tmp_buf = buf_copy;
	while ((token = strsep(&tmp_buf, " \t\n")) != NULL) {
		if (!*token)
			continue;

		ret = kstrtou8(token, 16, &kbuf[i]);
		if (ret < 0)
			goto fail;
		;

		pr_debug("token=%s, kbuf[%i]=0x%02x\n", token, i, kbuf[i]);
		/*
		 * Beyond capacity: wrap onto kbuf[1],
		 * same as EEPROM address wrap
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
	if (len <= 1) {
		ret = -EINVAL;
		goto fail;
	}

	pr_debug("i:%d, len:%d\n", i, len);

	base_addr = kbuf[0];
	remaining = len - 1;

	while (remaining) {
		reg = base_addr + written;
		room = EEPROM_PAGE_SIZE - (reg % EEPROM_PAGE_SIZE);

		tmp_len = min(remaining, room);

		pr_debug("tmp_len:%d written: %d, remaining:%d\n", tmp_len,
			 written, remaining);

		ret = _at24c02_i2c_write_data(client, reg, kbuf + written + 1,
					      tmp_len);
		if (ret < 0) {
			pr_err("i2c write failed: written:%d, remaining:%d, ret=%d\n",
			       written, remaining, ret);
			goto fail;
		}

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
	ret = count;
fail:
	kfree(buf_copy);
	return ret;
}
