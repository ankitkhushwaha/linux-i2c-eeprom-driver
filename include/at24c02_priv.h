/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * at24c02.h - defination for 24fc02 eeprom i2c driver
 */

#ifndef __EEPROM_24C02_H__
#define __EEPROM_24C02_H__

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kconfig.h>


#define MODULE_NAME "eeprom"

struct i2c_client;
struct device;
struct class;

#define EEPROM_PAGE_SIZE 8
#define EEPROM_CAPACITY 256

struct at24c02 {
	struct i2c_client *client;
	struct device *device;
	struct cdev cdev;
	struct class *class;
	dev_t devt;
	char *buf;
};
#if IS_ENABLED(CONFIG_AT24C02_CHARDEV)
int at24c02_chrdev_register(struct at24c02 *at24);
void at24c02_chrdev_unregister(struct at24c02 *at24);
#else
static inline int at24c02_chrdev_register(struct at24c02 *at24) { return 0; }
static inline void at24c02_chrdev_unregister(struct at24c02 *at24) {}
#endif  /* CONFIG_AT24C02_CHARDEV */

extern int at24c02_i2c_read_data(struct i2c_client *client, u8 reg, char *buf, int count);
extern int at24c02_i2c_write_data(struct i2c_client *client, const char *buf, size_t count);

#endif  /* __EEPROM_24C02_H__ */
