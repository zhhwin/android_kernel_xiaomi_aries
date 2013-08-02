/* Copyright (c) 2009-2010, The Linux Foundation. All rights reserved.
 * Copyright (c) 2012, LGE Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <mach/vreg.h>
#include <mach/rpc_server_handset.h>
#include <mach/board.h>

/* keypad */
#include <linux/mfd/pm8xxx/pm8921.h>

/* i2c */
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>

#include <linux/earlysuspend.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <mach/board_xiaomi.h>
#include "board-aries.h"

/* TOUCH GPIOS */
#define ATMEL_TS_I2C_INT_GPIO		6
#define ATMEL_TS_RESET_GPIO			PM8921_GPIO_PM_TO_SYS(8)
#define ATMEL_TS_IRQ_GPIO			6
#define ATMEL_TS_POWER_GPIO			PM8921_GPIO_PM_TO_SYS(5)

/* touch screen device */
#define APQ8064_GSBI3_QUP_I2C_BUS_ID            3

static int mxt_init_hw(bool init)
{
	int rc=0;

	if (init) {
		rc = gpio_request(ATMEL_TS_POWER_GPIO, "mxt_gpio_power");
		if (!rc) {
			rc = gpio_direction_output(ATMEL_TS_POWER_GPIO, 0);
			if (rc) {
				gpio_free(ATMEL_TS_POWER_GPIO);
				pr_err("%s: unable to set direction gpio %d\n", __func__, ATMEL_TS_POWER_GPIO);
			}
		} 
		else {
			pr_err("%s: unable to request power gpio %d\n", __func__, ATMEL_TS_POWER_GPIO);
		}
	} 
	else {
		gpio_free(ATMEL_TS_POWER_GPIO);
	}

	return rc;
}

static int mxt_power_on(bool on)
{
	gpio_set_value(ATMEL_TS_POWER_GPIO, on);
	msleep(1);
	return 0;
}

static int mxt_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	KEY_MENU, KEY_HOME, KEY_BACK,
};

static const u8 mxt336s_config_wintek_jdi[] = {
	0, 0, 0, 0, 0, 0, 1, 1, 29, 231,
	229, 218, 0, 2, 255, 255, 255, 3, 28, 0,
	10, 10, 0, 0, 1, 12, 1, 128, 11, 0,
	0, 24, 13, 0, 96, 35, 2, 1, 5, 8, 
	8, 0, 10, 5, 15, 15, 255, 4, 207, 2, 
	0, 0, 0, 0, 188, 30, 242, 60, 16, 5, 
	40, 42, 0, 0, 3, 19, 13, 3, 1, 0,
	48, 30, 2, 0, 0, 4, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
	212, 112, 76, 93, 212, 112, 116, 78, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16, 16, 0, 0, 3, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 1,
	32, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 2, 0, 1, 2, 4,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 3, 0, 7, 0, 0, 0, 0,
	0, 0, 2, 13, 15, 18, 5, 0, 10, 5, 
	5, 80, 18, 16, 40, 15, 52, 32, 32, 4, 
	64, 0, 0, 0, 0, 0, 80, 40, 2, 8, 
	8, 0, 10, 20, 20, 5, 5, 0, 0, 232,
	60, 242, 60, 15, 5, 0,
};

static const u8 mxt336s_config_wintek_sharp[] = {
	0, 0, 0, 0, 0, 0, 2, 1, 29, 171,
	246, 148, 0, 1, 255, 255, 255, 3, 28, 0,
	10, 10, 0, 0, 1, 12, 1, 128, 11, 0,
	0, 24, 13, 0, 96, 35, 2, 1, 5, 8,
	8, 0, 10, 5, 15, 15, 255, 4, 207, 2,
	0, 0, 0, 0, 188, 30, 242, 60, 16, 5,
	40, 42, 0, 0, 3, 19, 13, 3, 1, 0,
	48, 30, 2, 0, 0, 4, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
	212, 112, 76, 93, 212, 112, 116, 78, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16, 16, 0, 0, 3, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 1,
	32, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 2, 0, 1, 2, 4,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 3, 0, 7, 0, 0, 0, 0,
	0, 6, 12, 13, 22, 27, 5, 0, 10, 5,
	5, 80, 12, 22, 40, 15, 52, 32, 32, 4,
	64, 0, 0, 0, 0, 0, 80, 40, 2, 8,
	8, 0, 10, 20, 20, 5, 5, 0, 0, 232,
	60, 242, 60, 15, 5, 0,
};

static const u8 mxt336s_config_wintek_jdi_fw20[] = {
	0, 0, 0, 0, 0, 0, 1, 1, 8, 187,
	215, 84, 0, 2, 15, 15, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 255, 255, 255, 3,
	13, 0, 10, 10, 0, 0, 1, 12, 1, 128,
	11, 0, 0, 24, 13, 0, 64, 35, 0, 1,
	5, 8, 2, 0, 10, 5, 15, 15, 255, 4,
	207, 2, 0, 0, 0, 0, 188, 30, 178, 60,
	16, 5, 40, 42, 0, 2, 67, 150, 65, 100,
	128, 32, 35, 50, 60, 120, 0, 1, 1, 5,
	5, 0, 0, 0, 0, 0, 3, 19, 13, 3,
	1, 0, 16, 18, 2, 0, 0, 4, 0, 0,
	0, 0, 0, 0, 0, 3, 0, 144, 101, 8,
	82, 224, 110, 160, 79, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8, 0, 32, 16, 0, 0, 3, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 50, 64, 0,
	0, 0, 20, 1, 0, 1, 33, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 2, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 6, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	26, 0, 9, 0, 0, 8, 0, 0, 0, 1,
	26, 0, 9, 0, 0, 34, 1, 0, 0, 1,
	22, 0, 9, 0, 0, 8, 2, 0, 0, 1,
	22, 0, 9, 0, 0, 34, 3, 0, 0, 1,
	24, 0, 9, 0, 0, 8, 4, 0, 0, 1,
	24, 0, 9, 0, 0, 34, 5, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 5, 10, 5, 5, 32, 10,
	25, 6, 5, 32, 65, 0, 64, 0, 0, 14,
	1, 6, 10, 11, 12, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 0, 0, 0, 0, 15,
	1, 6, 10, 11, 12, 42, 42, 42, 42, 42,
	42, 42, 42, 42, 42, 0, 0, 0, 0, 15,
	1, 6, 10, 11, 12, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 0, 0, 0, 0,
};

static const u8 mxt336s_config_wintek_sharp_fw20[] = {
	0, 0, 0, 0, 0, 0, 2, 1, 8, 82,
	87, 236, 0, 1, 15, 15, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 255, 255, 255, 3,
	13, 0, 10, 10, 0, 0, 1, 12, 1, 128,
	11, 0, 0, 24, 13, 0, 64, 35, 0, 1,
	5, 8, 2, 0, 10, 5, 15, 15, 255, 4,
	207, 2, 0, 0, 0, 0, 188, 30, 178, 60,
	16, 5, 40, 42, 0, 2, 67, 150, 65, 100,
	128, 32, 35, 50, 60, 120, 0, 1, 1, 5,
	5, 0, 0, 0, 0, 0, 3, 19, 13, 3, 
	1, 0, 16, 18, 2, 0, 0, 4, 0, 0,
	0, 0, 0, 0, 0, 3, 0, 144, 101, 8, 
	82, 224, 110, 160, 79, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8, 0, 32, 16, 0, 0, 3, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 50, 64, 0,
	0, 0, 20, 1, 0, 1, 33, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 2, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 6, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	26, 0, 9, 0, 0, 8, 0, 0, 0, 1,
	26, 0, 9, 0, 0, 34, 1, 0, 0, 1,
	22, 0, 9, 0, 0, 8, 2, 0, 0, 1,
	22, 0, 9, 0, 0, 34, 3, 0, 0, 1,
	24, 0, 9, 0, 0, 8, 4, 0, 0, 1,
	24, 0, 9, 0, 0, 34, 5, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 5, 10, 5, 5, 32, 10,
	25, 6, 5, 32, 65, 0, 64, 0, 0, 14,
	0, 4, 8, 11, 16, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 0, 0, 0, 0, 15,
	0, 4, 8, 11, 16, 42, 42, 42, 42, 42,
	42, 42, 42, 42, 42, 0, 0, 0, 0, 15,
	0, 4, 8, 11, 16, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 0, 0, 0, 0,
};

static const u8 mxt336s_config_wintek_jdi_fw21[] = {
	0, 0, 0, 0, 0, 0, 1, 1, 2, 1,
	255, 5, 0, 2, 15, 15, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
};

static const u8 mxt336s_config_wintek_sharp_fw21[] = {
	0, 0, 0, 0, 0, 0, 2, 1, 2, 232,
	127, 189, 0, 1, 15, 15, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const struct mxt_config_info mxt336s_config_array[] = {
	{
		.config		= mxt336s_config_wintek_jdi,
		.config_length	= ARRAY_SIZE(mxt336s_config_wintek_jdi),
		.family_id	= 0x82,
		.variant_id	= 0x19,
		.version	= 0x10,
		.build		= 0xAA,
		.bootldr_id	= MXT_BOOTLOADER_ID_336S,
	},
	{
		.config		= mxt336s_config_wintek_sharp,
		.config_length	= ARRAY_SIZE(mxt336s_config_wintek_sharp),
		.family_id	= 0x82,
		.variant_id	= 0x19,
		.version	= 0x10,
		.build		= 0xAA,
		.bootldr_id	= MXT_BOOTLOADER_ID_336S,
	},
	{	.config		= mxt336s_config_wintek_jdi_fw20,
		.config_length	= ARRAY_SIZE(mxt336s_config_wintek_jdi_fw20),
		.family_id	= 0x82,
		.variant_id	= 0x19,
		.version	= 0x10,
		.build		= 0x01,
		.bootldr_id	= MXT_BOOTLOADER_ID_336S,
	},
	{
		.config		= mxt336s_config_wintek_sharp_fw20,
		.config_length	= ARRAY_SIZE(mxt336s_config_wintek_sharp_fw20),
		.family_id	= 0x82,
		.variant_id	= 0x19,
		.version	= 0x10,
		.build		= 0x01,
		.bootldr_id	= MXT_BOOTLOADER_ID_336S,
	},
	{
		.config         = mxt336s_config_wintek_jdi_fw21,
		.config_length  = ARRAY_SIZE(mxt336s_config_wintek_jdi_fw21),
		.family_id      = 0x82,
		.variant_id     = 0x29,
		.version        = 0x11,
		.build          = 0x1,
		.bootldr_id     = MXT_BOOTLOADER_ID_336S,
	},
	{
		.config         = mxt336s_config_wintek_sharp_fw21,
		.config_length  = ARRAY_SIZE(mxt336s_config_wintek_sharp_fw21),
		.family_id      = 0x82,
		.variant_id     = 0x29,
		.version        = 0x11,
		.build          = 0x1,
		.bootldr_id     = MXT_BOOTLOADER_ID_336S,
	},
};

static struct mxt_platform_data mxt336s_platform_data = {
	.config_array		= mxt336s_config_array,
	.config_array_size	= ARRAY_SIZE(mxt336s_config_array),
	.panel_minx		= 0,
	.panel_maxx		= 719,
	.panel_miny		= 0,
	.panel_maxy		= 1279,
	.disp_minx		= 0,
	.disp_maxx		= 719,
	.disp_miny		= 0,
	.disp_maxy		= 1279,
	.irqflags		= IRQF_TRIGGER_LOW | IRQF_ONESHOT,
	.i2c_pull_up		= false,
	.reset_gpio		= ATMEL_TS_RESET_GPIO,
	.irq_gpio		= ATMEL_TS_IRQ_GPIO,
	.digital_pwr_regulator	= false,
	.key_codes		= mxt_key_codes,
	.read_chg		= NULL,
	.init_hw		= mxt_init_hw,
	.power_on		= mxt_power_on,
};

static struct i2c_board_info mxt336s_device_info[] = {
	[0] = {
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4b),
		.platform_data = &mxt336s_platform_data,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_I2C_INT_GPIO),
	},
};

void __init apq8064_init_input(void)
{
	printk(KERN_INFO "[Touch D] %s: NOT DCM KDDI, reg atmel driver \n",
	       __func__);
	i2c_register_board_info(APQ8064_GSBI3_QUP_I2C_BUS_ID,
				&mxt336s_device_info[0], 1);
}
