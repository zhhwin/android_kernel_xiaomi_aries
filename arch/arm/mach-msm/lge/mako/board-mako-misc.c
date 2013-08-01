/*
  * Copyright (C) 2011,2012 LGE, Inc.
  *
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General
  * License version 2, as published by the Free Software Foundation,
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  * GNU General Public License for more details.
  */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/i2c/isa1200.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#ifdef CONFIG_SLIMPORT_ANX7808
#include <linux/platform_data/slimport_device.h>
#endif
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#include <mach/board_lge.h>
#include <mach/msm_xo.h>

#include "devices.h"
#include "board-mako.h"

static struct platform_device *misc_devices[] __initdata = {

};

#ifdef CONFIG_SLIMPORT_ANX7808
#define GPIO_SLIMPORT_CBL_DET    PM8921_GPIO_PM_TO_SYS(14)
#define GPIO_SLIMPORT_PWR_DWN    PM8921_GPIO_PM_TO_SYS(15)
#define ANX_AVDD33_EN            PM8921_GPIO_PM_TO_SYS(16)
#define GPIO_SLIMPORT_RESET_N    31
#define GPIO_SLIMPORT_INT_N      43

static int anx7808_dvdd_onoff(bool on)
{
	static bool power_state = 0;
	static struct regulator *anx7808_dvdd_reg = NULL;
	int rc = 0;

	if (power_state == on) {
		pr_info("anx7808 dvdd is already %s \n", power_state ? "on" : "off");
		goto out;
	}

	if (!anx7808_dvdd_reg) {
		anx7808_dvdd_reg= regulator_get(NULL, "slimport_dvdd");
		if (IS_ERR(anx7808_dvdd_reg)) {
			rc = PTR_ERR(anx7808_dvdd_reg);
			pr_err("%s: regulator_get anx7808_dvdd_reg failed. rc=%d\n",
					__func__, rc);
			anx7808_dvdd_reg = NULL;
			goto out;
		}
		rc = regulator_set_voltage(anx7808_dvdd_reg, 1100000, 1100000);
		if (rc ) {
			pr_err("%s: regulator_set_voltage anx7808_dvdd_reg failed\
				rc=%d\n", __func__, rc);
			goto out;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(anx7808_dvdd_reg, 100000);
		if (rc < 0) {
			pr_err("%s : set optimum mode 100000, anx7808_dvdd_reg failed \
					(%d)\n", __func__, rc);
			goto out;
		}
		rc = regulator_enable(anx7808_dvdd_reg);
		if (rc) {
			pr_err("%s : anx7808_dvdd_reg enable failed (%d)\n",
					__func__, rc);
			goto out;
		}
	}
	else {
		rc = regulator_disable(anx7808_dvdd_reg);
		if (rc) {
			pr_err("%s : anx7808_dvdd_reg disable failed (%d)\n",
				__func__, rc);
			goto out;
		}
		rc = regulator_set_optimum_mode(anx7808_dvdd_reg, 100);
		if (rc < 0) {
			pr_err("%s : set optimum mode 100, anx7808_dvdd_reg failed \
				(%d)\n", __func__, rc);
			goto out;
		}
	}
	power_state = on;

out:
	return rc;

}

static int anx7808_avdd_onoff(bool on)
{
	static bool init_done = 0;
	int rc = 0;

	if (!init_done) {
		rc = gpio_request_one(ANX_AVDD33_EN,
					GPIOF_OUT_INIT_HIGH, "anx_avdd33_en");
		if (rc) {
			pr_err("request anx_avdd33_en failed, rc=%d\n", rc);
			return rc;
		}
		init_done = 1;
	}

	gpio_set_value(ANX_AVDD33_EN, on);
	return 0;
}

static struct anx7808_platform_data anx7808_pdata = {
	.gpio_p_dwn = GPIO_SLIMPORT_PWR_DWN,
	.gpio_reset = GPIO_SLIMPORT_RESET_N,
	.gpio_int = GPIO_SLIMPORT_INT_N,
	.gpio_cbl_det = GPIO_SLIMPORT_CBL_DET,
	.dvdd_power = anx7808_dvdd_onoff,
	.avdd_power = anx7808_avdd_onoff,
};

struct i2c_board_info i2c_anx7808_info[] = {
	{
		I2C_BOARD_INFO("anx7808", 0x72 >> 1),
		.platform_data = &anx7808_pdata,
	},
};

static struct i2c_registry i2c_anx7808_devices __initdata = {
	I2C_FFA,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	i2c_anx7808_info,
	ARRAY_SIZE(i2c_anx7808_info),
};

static void __init lge_add_i2c_anx7808_device(void)
{
	i2c_register_board_info(i2c_anx7808_devices.bus,
		i2c_anx7808_devices.info,
		i2c_anx7808_devices.len);
}
#endif

#ifdef CONFIG_HAPTIC_ISA1200
#define ISA1200_HAP_EN_GPIO		PM8921_GPIO_PM_TO_SYS(33)
#define ISA1200_HAP_LEN_GPIO	PM8921_GPIO_PM_TO_SYS(20)

static struct isa1200_regulator isa1200_reg_data[] = {
	{
		.name = "vddp",
		.min_uV = ISA_I2C_VTG_MIN_UV,
		.max_uV = ISA_I2C_VTG_MAX_UV,
		.load_uA = ISA_I2C_CURR_UA,
	},
};

static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.pwm_ch_id = 0,
	.max_timeout = 15000,
	.hap_en_gpio = ISA1200_HAP_EN_GPIO,
	.hap_len_gpio = ISA1200_HAP_LEN_GPIO,
	.mode_ctrl = PWM_INPUT_MODE,
	.pwm_fd = {
		.pwm_freq = 44800,
	},
	.smart_en = false,
	.is_erm = true,
	.ext_clk_en = false,
	.chip_en = 1,
	.duty = 90,
	.regulator_info = isa1200_reg_data,
	.num_regulators = ARRAY_SIZE(isa1200_reg_data),
};

static struct i2c_board_info i2c_isa1200_info[] __initdata = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};

static struct i2c_registry i2c_isa1200_devices __initdata = {
	I2C_FFA,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	i2c_isa1200_info,
	ARRAY_SIZE(i2c_isa1200_info),
};
#endif

void __init apq8064_init_misc(void)
{
	platform_add_devices(misc_devices, ARRAY_SIZE(misc_devices));

#ifdef CONFIG_SLIMPORT_ANX7808
	lge_add_i2c_anx7808_device();
#endif

#ifdef CONFIG_HAPTIC_ISA1200
	i2c_register_board_info(i2c_isa1200_devices.bus,
		i2c_isa1200_devices.info,
		i2c_isa1200_devices.len);
#endif
}
