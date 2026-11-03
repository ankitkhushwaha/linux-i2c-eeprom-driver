// SPDX-License-Identifier: GPL-2.0-only
/*
 * i2c Driver for at24c02
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "at24c02_priv.h"

static int at24c02_probe(struct i2c_client *client)
{
	pr_info("at24c02 probed\n");
	return 0;
}

static void at24c02_remove(struct i2c_client *client)
{
	pr_info("at24c02 removed\n");
}

static const struct i2c_device_id at24c02_idtable[] = {
	{ "24c02", 0 },
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
