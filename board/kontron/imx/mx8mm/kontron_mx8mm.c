// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Kontron Electronics GmbH
 */

#include <common.h>
#include <asm/io.h>
#include <asm/armv8/mmu.h>

DECLARE_GLOBAL_DATA_PTR;

int board_phys_sdram_size(phys_size_t *sdram_size)
{
	*sdram_size = get_ram_size((long int *)PHYS_SDRAM, (SZ_4G + SZ_4G));
	return 0;
}

#if IS_ENABLED(CONFIG_FEC_MXC)
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}
#endif

#ifndef CONFIG_SPL_BUILD
int board_early_init_f(void)
{
	return 0;
}
#endif

/*
 * If the SoM is mounted on a baseboard with a USB ethernet controller,
 * there might be an additional MAC address programmed to the MAC OTP fuses.
 * Although the i.MX8MM has only one MAC, the MAC0, MAC1 and MAC2 registers
 * in the OTP fuses can still be used to store two separate addresses.
 * Try to read the secondary address from MAC1 and MAC2 and adjust the
 * devicetree so Linux can pick up the MAC address.
 */
int fdt_set_usb_eth_addr(void *blob)
{
	u32 value = readl(OCOTP_BASE_ADDR + 0x660);
	unsigned char mac[6];
	int node, ret;

	mac[0] = value >> 24;
	mac[1] = value >> 16;
	mac[2] = value >> 8;
	mac[3] = value;

	value = readl(OCOTP_BASE_ADDR + 0x650);
	mac[4] = value >> 24;
	mac[5] = value >> 16;

	if (is_zero_ethaddr(mac)) {
		/*
		 * There is no MAC address for the USB ethernet set
		 * in the OTP fuses. Just skip.
		 */
		return 0;
	}

	if (!is_valid_ethaddr(mac)) {
		printf("Invalid MAC address for USB ethernet "
		       "set in OTP fuses!\n");
		return -EINVAL;
	}

	node = fdt_path_offset(blob, fdt_get_alias(blob, "ethernet1"));
	if (node < 0) {
		printf("Did not find ethernet1 node in dt, "
		       "skip setting MAC address for USB ethernet\n");
		return 0;
	}

	ret = fdt_setprop(blob, node, "local-mac-address", &mac, 6);
	if (ret)
		ret = fdt_setprop(blob, node, "mac-address", &mac, 6);

	if (ret) {
		printf("Missing mac-address or local-mac-addresss property in dt, "
		       "skip setting MAC address for USB ethernet\n");
	}

	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	int ret = fdt_set_usb_eth_addr(blob);
	if (ret)
		return ret;

	return fdt_fixup_memory(blob, PHYS_SDRAM, gd->ram_size);
}

int board_init(void)
{
	if (IS_ENABLED(CONFIG_FEC_MXC))
		setup_fec();

	return 0;
}

int board_late_init(void)
{
	return 0;
}


