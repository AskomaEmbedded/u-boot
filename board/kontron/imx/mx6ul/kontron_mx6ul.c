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
#include <asm/arch/sys_proto.h>
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

static int find_ethernet_phy(void)
{
	struct phy_device *phydev;
	struct mii_dev *bus;
	int phy_addr;

	bus = fec_get_miibus(ENET2_BASE_ADDR, -1);
	if (!bus)
		return -ENOENT;

	/* Scan Network PHY addresses 2 and 7 */
	phydev = phy_find_by_mask(bus, 0x84, PHY_INTERFACE_MODE_RMII);
	if (!phydev) {
		free(bus);
		return -ENOENT;
	}

	phy_addr = phydev->addr;
	free(phydev);

	return phy_addr;
}

int board_phy_config(struct phy_device *phydev)
{
	int phy;

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	phy = find_ethernet_phy();
	switch (phy) {
	case 2:
		env_set("board_rev", "1.2");
		printf("Board revision detected: EM 1.2\n");
		break;
	case 7:
		env_set("board_rev", "1.3");
		printf("Board revision detected: EM 1.3\n");
		break;
	}

	return 0;
}

int board_early_init_f(void)
{
	enable_qspi_clk(0);

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	setup_fec();

	return 0;
}

int board_late_init(void)
{
	unsigned char mac[6];
	u32 value;

	value = readl(OCOTP_BASE_ADDR + 0x640);

	mac[0] = value >> 24;
	mac[1] = value >> 16;
	mac[2] = value >> 8;
	mac[3] = value;

	value = readl(OCOTP_BASE_ADDR + 0x630);
	mac[4] = value >> 24;
	mac[5] = value >> 16;

	if (!is_valid_ethaddr(mac)) {
		printf("Invalid MAC address for FEC2 set in OTP fuses!\n");
		return -EINVAL;
	}

	if (!env_get("eth1addr"))
		eth_env_set_enetaddr("eth1addr", mac);

	return 0;
}
