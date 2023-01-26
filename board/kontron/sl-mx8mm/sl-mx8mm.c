// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Kontron Electronics GmbH
 */

#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/mach-imx/boot_mode.h>
#include <efi.h>
#include <efi_loader.h>
#include <env_internal.h>
#include <fdt_support.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <net.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
struct efi_fw_image fw_images[] = {
	{
		.image_type_id = KONTRON_SL_MX8MM_FIT_IMAGE_GUID,
		.fw_name = u"KONTROL-SL-MX8MM-UBOOT",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "sf 0:0=flash-bin raw 0x400 0x1f0000",
	.images = fw_images,
};

u8 num_image_type_guids = ARRAY_SIZE(fw_images);
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_phys_sdram_size(phys_size_t *size)
{
	u32 ddr_size = readl(M4_BOOTROM_BASE_ADDR);

	if (ddr_size == 4) {
		*size = 0x100000000;
	} else if (ddr_size == 3) {
		*size = 0xc0000000;
	} else if (ddr_size == 2) {
		*size = 0x80000000;
	} else if (ddr_size == 1) {
		*size = 0x40000000;
	} else {
		printf("Unknown DDR type!!!\n");
		*size = 0x40000000;
	}

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	return fdt_fixup_memory(blob, PHYS_SDRAM, gd->ram_size);
}

int board_init(void)
{
	return 0;
}

static int get_mac_from_fuse(unsigned char *enetaddr)
{
	imx_get_mac_from_fuse(0, enetaddr);
	if (!is_valid_ethaddr(enetaddr))
		return -EINVAL;

	return 0;
}

int board_late_init(void)
{
	unsigned char enetaddr[6];
	int ret;

	if (!fdt_node_check_compatible(gd->fdt_blob, 0, "kontron,imx8mm-n802x-som") ||
	    !fdt_node_check_compatible(gd->fdt_blob, 0, "kontron,imx8mm-osm-s")) {
		env_set("som_type", "osm-s");
		env_set("touch_rst_gpio", "111");
	} else {
		env_set("som_type", "sl");
		env_set("touch_rst_gpio", "87");
	}

	ret = get_mac_from_fuse(enetaddr);
	if (ret < 0) {
		printf("Cannot read eth0 MAC address\n");
		return 0;
	}

	/* eth1 MAC address is eth0 MAC address + 1 */
	enetaddr[5]++;

	if (!env_get("eth1addr"))
		eth_env_set_enetaddr("eth1addr", enetaddr);

	return 0;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum boot_device boot_dev = get_boot_device();

	if (prio)
		return ENVL_UNKNOWN;

	/*
	 * Make sure that the environment is loaded from
	 * the MMC if we are running from SD card or eMMC.
	 */
	if (CONFIG_IS_ENABLED(ENV_IS_IN_MMC) &&
	    (boot_dev == SD1_BOOT || boot_dev == SD2_BOOT))
		return ENVL_MMC;

	if (CONFIG_IS_ENABLED(ENV_IS_IN_SPI_FLASH))
		return ENVL_SPI_FLASH;

	return ENVL_NOWHERE;
}

#if defined(CONFIG_ENV_IS_IN_MMC)
int board_mmc_get_env_dev(int devno)
{
	return devno;
}
#endif
