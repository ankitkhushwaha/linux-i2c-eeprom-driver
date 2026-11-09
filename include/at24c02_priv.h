/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * at24c02.h - defination for 24fc02 eeprom i2c driver
 */

#ifndef __EEPROM_24C02_H__
#define __EEPROM_24C02_H__

#include <linux/types.h>

#define MODULE_NAME "eeprom"

struct i2c_client;
struct device;
struct class;
struct cdev;

#define EEPROM_PAGE_SIZE 8
#define EEPROM_CAPACITY 256

#ifdef CONFIG_AT24C02_CHARDEV
int at24c02_chrdev_register(void);
void at24c02_chrdev_unregister(void);
#else
static inline int at24c02_chrdev_register(void) { return 0; }
static inline void at24c02_chrdev_unregister(void) {}
#endif  /* CONFIG_AT24C02_CHARDEV */

#endif  /* __EEPROM_24C02_H__ */
