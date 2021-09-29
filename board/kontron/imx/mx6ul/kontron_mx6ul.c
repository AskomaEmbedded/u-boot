// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Kontron Electronics GmbH
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <miiphy.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <netdev.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();

	return 0;
}


/*
 * Read second MAC address set in OTP fuses and pass it to kernel via devicetree.
 * U-boot does only initialize one FEC so we have to read the second MAC adress
 * manually.
 */
int fdt_fixup_second_ethaddr(void *blob)
{
	u32 value = readl(OCOTP_BASE_ADDR + 0x640);
	unsigned char mac[6];
	int node, ret;

	mac[0] = value >> 24;
	mac[1] = value >> 16;
	mac[2] = value >> 8;
	mac[3] = value;

	value = readl(OCOTP_BASE_ADDR + 0x630);
	mac[4] = value >> 24;
	mac[5] = value >> 16;

	if (!is_valid_ethaddr(mac)) {
		printf("Invalid MAC address for FEC2"
		       "set in OTP fuses!\n");
		return -EINVAL;
	}

	node = fdt_path_offset(blob, fdt_get_alias(blob, "ethernet1"));
	if (node < 0) {
		printf("Did not find ethernet1 node in dt, "
		       "skip setting MAC address for FEC2\n");
		return 0;
	}

	ret = fdt_setprop(blob, node, "local-mac-address", &mac, 6);
	if (ret)
		ret = fdt_setprop(blob, node, "mac-address", &mac, 6);

	if (ret) {
		printf("Missing mac-address or local-mac-addresss property in dt, "
		       "skip setting MAC address for FEC2\n");
	}

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	/*
	 * Use 50M anatop loopback REF_CLK1 for ENET1,
	 * clear gpr1[13], set gpr1[17].
	 */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
			IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);

	/*
	 * Use 50M anatop loopback REF_CLK2 for ENET2,
	 * clear gpr1[14], set gpr1[18].
	 */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC2_MASK,
			IOMUX_GPR1_FEC2_CLOCK_MUX1_SEL_MASK);

	ret = enable_fec_anatop_clock(0, ENET_50MHZ);
	if (ret)
		return ret;

	ret = enable_fec_anatop_clock(1, ENET_50MHZ);
	if (ret)
		return ret;

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

int board_early_init_f(void)
{
	enable_qspi_clk(0);

	return 0;
}


int ft_board_setup(void *blob, bd_t *bd)
{
	return fdt_fixup_second_ethaddr(blob);
}


int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	setup_fec();

	return 0;
}
