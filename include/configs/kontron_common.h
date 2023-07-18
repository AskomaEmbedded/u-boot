// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Kontron Electronics GmbH
 *
 * Common configuration settings for the Kontron Electronics boards.
 *
 */

#ifndef __KONTRON_COMMON_CONFIG_H
#define __KONTRON_COMMON_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#ifdef CONFIG_MX6
#include "mx6_common.h"
#ifdef CONFIG_SPL_BUILD
#include "imx6_spl.h"
#endif
#endif
#include <asm/mach-imx/gpio.h>

/*
 * #######################################
 * ### MISC                            ###
 * #######################################
 */
#define CONFIG_SYS_MALLOC_LEN		SZ_64M
#define CONFIG_SYS_HZ			1000

// Offset of U-Boot proper image within SPI flash
#define CONFIG_SYS_UBOOT_BASE		SZ_128K

/*
 * #######################################
 * ### RAM                             ###
 * #######################################
 */
#ifdef CONFIG_IMX8M
#define PHYS_SDRAM			DDR_CSD1_BASE_ADDR
#define PHYS_SDRAM_SIZE			(SZ_2G + SZ_1G)
#define PHYS_SDRAM_2			0x100000000
#define PHYS_SDRAM_2_SIZE		(SZ_4G + SZ_1G)
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#else
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR
#endif

#ifdef CONFIG_IMX8M
#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	0x80000
#else
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE
#endif

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + SZ_256M)

/*
 * #######################################
 * ### Ethernet                        ###
 * #######################################
 */
#ifdef CONFIG_FEC_MXC
#define CONFIG_ETHPRIME			"eth0"
#else
#define CONFIG_ETHPRIME
#endif

/*
 * #######################################
 * ### USB                             ###
 * #######################################
 */
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS		0
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2
#endif

/*
 * #######################################
 * ### ENVIRONMENT                     ###
 * #######################################
 */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootargs_base=" KONTRON_ENV_KERNEL_MTDPARTS " " \
			 KONTRON_ENV_KERNEL_CONSOLE "\0" \
	"script=boot.scr\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"kernel_addr_r=" KONTRON_ENV_KERNEL_ADDR "\0" \
	"fdt_addr_r=" KONTRON_ENV_FDT_ADDR "\0" \
	"ramdisk_addr_r=" KONTRON_ENV_RAMDISK_ADDR "\0" \
	"pxefile_addr_r=" KONTRON_ENV_PXE_ADDR "\0" \
	"scriptaddr=" KONTRON_ENV_PXE_ADDR "\0" \
	"bootdir=\0" \
	"bootdelay=3\0" \
	"ipaddr=192.168.1.11\0" \
	"serverip=192.168.1.10\0" \
	"gatewayip=192.168.1.10\0" \
	"netmask=255.255.255.0\0" \
	"ethact=" CONFIG_ETHPRIME "\0" \
	"hostname=" CONFIG_HOSTNAME "\0" \
	"bootubipart=spi-nand0\0" \
	"bootubivol=boot\0" \
	BOOTENV

#endif /* __KONTRON_COMMON_CONFIG_H */
