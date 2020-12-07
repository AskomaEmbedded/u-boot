// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Kontron Electronics GmbH
 */

#include <common.h>
#include <spl.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/ddr.h>
#include <asm/armv8/mmu.h>
#include <asm/gpio.h>

#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>

#include <i2c.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	BOARD_TYPE_KTN_N801X,
	BOARD_TYPE_KTN_N801X_LVDS,
	BOARD_TYPE_MAX
};

#define GPIO_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)
#define I2C_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)
#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

#define TOUCH_RESET_GPIO	IMX_GPIO_NR(3, 23)

static iomux_v3_cfg_t const i2c1_pads[] = {
	IMX8MM_PAD_I2C1_SCL_I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL) | MUX_MODE_SION,
	IMX8MM_PAD_I2C1_SDA_I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL) | MUX_MODE_SION
};

static iomux_v3_cfg_t const i2c2_pads[] = {
	IMX8MM_PAD_I2C2_SCL_I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL) | MUX_MODE_SION,
	IMX8MM_PAD_I2C2_SDA_I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL) | MUX_MODE_SION
};

static iomux_v3_cfg_t const touch_gpio[] = {
	IMX8MM_PAD_SAI5_RXD2_GPIO3_IO23 | MUX_PAD_CTRL(GPIO_PAD_CTRL)
};

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MM_PAD_UART3_RXD_UART3_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART3_TXD_UART3_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	case SPI_NOR_BOOT:
		return BOOT_DEVICE_SPI;
	case SD1_BOOT:
	case MMC1_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC2;
	default:
		return BOOT_DEVICE_NONE;
	}
}

bool check_ram_available(long int size)
{
	long int sz = get_ram_size((long int *)PHYS_SDRAM, size);

	if (sz == size)
		return true;

	return false;
}

void spl_dram_init(void)
{
	gd->ram_size = 0;

	/*
	 * Try the default DDR settings in lpddr4_timing.c to
	 * comply with the Micron 4GB DDR.
	 */
	if (!ddr_init(&dram_timing) && check_ram_available(SZ_4G)) {
		gd->ram_size = SZ_4G;
		return;
	}

	/*
	 * Overwrite some values to comply with the Micron 1GB/2GB DDRs.
	 */
	dram_timing.ddrc_cfg[2].val = 0xa1080020;
	dram_timing.ddrc_cfg[37].val = 0x1f;

	dram_timing.fsp_msg[0].fsp_cfg[9].val = 0x110;
	dram_timing.fsp_msg[0].fsp_cfg[21].val = 0x1;
	dram_timing.fsp_msg[1].fsp_cfg[10].val = 0x110;
	dram_timing.fsp_msg[1].fsp_cfg[22].val = 0x1;
	dram_timing.fsp_msg[2].fsp_cfg[10].val = 0x110;
	dram_timing.fsp_msg[2].fsp_cfg[22].val = 0x1;
	dram_timing.fsp_msg[3].fsp_cfg[10].val = 0x110;
	dram_timing.fsp_msg[3].fsp_cfg[22].val = 0x1;

	if (!ddr_init(&dram_timing)) {
		if (check_ram_available(SZ_2G))
			gd->ram_size = SZ_2G;
		else if (check_ram_available(SZ_1G))
			gd->ram_size = SZ_1G;
	}

	if (gd->ram_size == 0)
		printf("Failed to initialize DDR RAM!\n");
}

static void touch_reset(void)
{
	/*
	 * Toggle the reset of the touch panel.
	 */
	imx_iomux_v3_setup_multiple_pads(touch_gpio, ARRAY_SIZE(touch_gpio));

	gpio_request(TOUCH_RESET_GPIO, "touch_reset");
	gpio_direction_output(TOUCH_RESET_GPIO, 0);
	mdelay(20);
	gpio_direction_output(TOUCH_RESET_GPIO, 1);
	mdelay(20);
}

static int i2c_detect(uint8_t bus, uint16_t addr)
{
	struct udevice *udev;
	int ret;

	/*
	 * Try to probe the touch controller to check if an LVDS panel is
	 * connected.
	 */
	ret = i2c_get_chip_for_busnum(bus, addr, 0, &udev);
	if (ret == 0)
		return 0;

	return 1;
}

int do_board_detect(void)
{
	bool lvds = false;
	printf("Kontron SL i.MX8MM (N801X) module, %d GB RAM detected\n", gd->ram_size / SZ_1G);

	/*
	 * Check the I2C touch controller to detect a LVDS panel.
	 */
	imx_iomux_v3_setup_multiple_pads(i2c2_pads, ARRAY_SIZE(i2c2_pads));
	touch_reset();

	if (i2c_detect(1, 0x5d) == 0) {
		printf("Touch controller detected, "
			   "assuming LVDS panel...\n");
		lvds = true;
	}

	/*
	 * Check the I2C PMIC to detect the deprecated SoM with DA9063.
	 */
	imx_iomux_v3_setup_multiple_pads(i2c1_pads, ARRAY_SIZE(i2c1_pads));

	if (i2c_detect(0, 0x58) == 0) {
		printf("### ATTENTION: DEPRECATED SOM REVISION (N8010 Rev0) DETECTED! ###\n");
		printf("###  THIS HW IS NOT SUPPRTED AND BOOTING WILL PROBABLY FAIL   ###\n");
		printf("###             PLEASE UPGRADE TO LATEST MODULE               ###\n");
	}

	if (lvds)
		gd->board_type = BOARD_TYPE_KTN_N801X_LVDS;
	else
		gd->board_type = BOARD_TYPE_KTN_N801X;

	return 0;
}

int board_fit_config_name_match(const char *name)
{
	if (gd->board_type == BOARD_TYPE_KTN_N801X_LVDS && is_imx8mm() &&
	    !strncmp(name, "imx8mm-kontron-n801x-s-lvds", 27))
		return 0;

	if (gd->board_type == BOARD_TYPE_KTN_N801X && is_imx8mm() &&
	    !strncmp(name, "imx8mm-kontron-n801x-s", 22))
		return 0;

	return -1;
}

void spl_board_init(void)
{
	struct udevice *dev;
	int ret;

	puts("Normal Boot\n");

	ret = uclass_get_device_by_name(UCLASS_CLK,
					"clock-controller@30380000",
					&dev);
	if (ret < 0)
		printf("Failed to find clock node. Check device tree\n");
}

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

void board_init_f(ulong dummy)
{
	int ret;

	arch_cpu_init();

	init_uart_clk(2);

	board_early_init_f();

	timer_init();

	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	/* DDR initialization */
	spl_dram_init();

	/* Detect the board type */
	do_board_detect();

	board_init_r(NULL, 0);
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	puts ("resetting ...\n");

	reset_cpu(WDOG1_BASE_ADDR);

	return 0;
}

void board_boot_order(u32 *spl_boot_list)
{
	u32 bootdev = spl_boot_device();

	/*
	 * The default boot fuse settings use the SD card (MMC2) as primary
	 * boot device, but allow SPI NOR as a fallback boot device.
	 * We can't detect the fallback case and spl_boot_device() will return
	 * BOOT_DEVICE_MMC2 despite the actual boot device beeing SPI NOR.
	 * Therefore we try to load U-Boot proper vom SPI NOR after loading
	 * from MMC has failed.
	 */
	spl_boot_list[0] = bootdev;

	switch (bootdev) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
		spl_boot_list[1] = BOOT_DEVICE_SPI;
		break;
	}
}
