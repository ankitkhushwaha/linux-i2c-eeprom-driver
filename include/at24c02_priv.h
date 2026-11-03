#ifndef __EEPROM_24C02_H__
#define __EEPROM_24C02_H__

#define MODULE_NAME "eeprom"

struct i2c_client;
struct device;
struct class;
struct cdev;

#ifdef CONFIG_AT24C02_CHARDEV
int at24c02_chrdev_register(void);
void at24c02_chrdev_unregister(void);
#else
static inline int at24c02_chrdev_register(void) { return 0; }
static inline void at24c02_chrdev_unregister(void) {}
#endif  /* CONFIG_AT24C02_CHARDEV */

#endif  /* __EEPROM_24C02_H__ */
