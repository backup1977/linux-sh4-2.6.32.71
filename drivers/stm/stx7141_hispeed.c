/*
 * (c) 2010 STMicroelectronics Limited
 *
 * Author: Pawel Moll <pawel.moll@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ethtool.h>
#include <linux/dma-mapping.h>
#include <linux/stm/miphy.h>
#include <linux/stm/pad.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/stx7141.h>
#include <linux/delay.h>
#include <asm/irq-ilc.h>

/* Ethernet MAC resources ------------------------------------------------- */

static struct stm_pad_config *stx7141_ethernet_pad_configs[] = {
	[0] = (struct stm_pad_config []) {
		[stx7141_ethernet_mode_mii] = {
			.gpios_num = 20,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(8, 0, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(8, 1, 1),	/* TXEN */
				STM_PAD_PIO_OUT(8, 2, 1),	/* TXERR */
				STM_PAD_PIO_OUT(8, 3, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(8, 4, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(8, 5, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(8, 6, 1),	/* TXD.3 */
				STM_PAD_PIO_IN(9, 3, -1),	/* RXCLK */
				STM_PAD_PIO_IN(9, 4, -1),	/* RXDV */
				STM_PAD_PIO_IN(9, 5, -1),	/* RXERR */
				STM_PAD_PIO_IN(9, 6, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(9, 7, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(10, 0, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(10, 1, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(10, 6, -1),	/* CRS */
				STM_PAD_PIO_IN(10, 7, -1),	/* COL */
				STM_PAD_PIO_OUT(11, 0, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(11, 1, 1),	/* MDIO */
				STM_PAD_PIO_IN(11, 2, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(11, 3, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII0_CLOCK_OUT:
				 * 0 = PIO11.3 is controlled by PIO muxing,
				 * 1 = PIO11.3 is delayed version of PIO8.0
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 13, 13, 1),
				/* ETHERNET_INTERFACE_ON0 */
				STM_PAD_SYS_CFG(7, 16, 16, 1),
				/* PHY_INTF_SEL0: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 24, 26, 0),
				/* ENMII0: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 27, 27, 1),
			},
		},
		[stx7141_ethernet_mode_gmii] = {
			.gpios_num = 28,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(8, 0, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(8, 1, 1),	/* TXEN */
				STM_PAD_PIO_OUT(8, 2, 1),	/* TXERR */
				STM_PAD_PIO_OUT(8, 3, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(8, 4, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(8, 5, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(8, 6, 1),	/* TXD.3 */
				STM_PAD_PIO_OUT(8, 7, 1),	/* TXD.4 */
				STM_PAD_PIO_OUT(9, 0, 1),	/* TXD.5 */
				STM_PAD_PIO_OUT(9, 1, 1),	/* TXD.6 */
				STM_PAD_PIO_OUT(9, 2, 1),	/* TXD.7 */
				STM_PAD_PIO_IN(9, 3, -1),	/* RXCLK */
				STM_PAD_PIO_IN(9, 4, -1),	/* RXDV */
				STM_PAD_PIO_IN(9, 5, -1),	/* RXERR */
				STM_PAD_PIO_IN(9, 6, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(9, 7, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(10, 0, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(10, 1, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(10, 2, -1),	/* RXD.4 */
				STM_PAD_PIO_IN(10, 3, -1),	/* RXD.5 */
				STM_PAD_PIO_IN(10, 4, -1),	/* RXD.6 */
				STM_PAD_PIO_IN(10, 5, -1),	/* RXD.7 */
				STM_PAD_PIO_IN(10, 6, -1),	/* CRS */
				STM_PAD_PIO_IN(10, 7, -1),	/* COL */
				STM_PAD_PIO_OUT(11, 0, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(11, 1, 1),	/* MDIO */
				STM_PAD_PIO_IN(11, 2, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(11, 3, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII0_CLOCK_OUT:
				 * 0 = PIO11.3 is controlled by PIO muxing,
				 * 1 = PIO11.3 is delayed version of PIO8.0
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 13, 13, 1),
				/* ETHERNET_INTERFACE_ON0 */
				STM_PAD_SYS_CFG(7, 16, 16, 1),
				/* PHY_INTF_SEL0: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 24, 26, 0),
				/* ENMII0: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 27, 27, 1),
			},
		},
		[stx7141_ethernet_mode_rgmii] = { /* TODO */ },
		[stx7141_ethernet_mode_sgmii] = { /* TODO */ },
		[stx7141_ethernet_mode_rmii] = {
			.gpios_num = 12,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_OUT(8, 1, 1),	/* TXEN */
				STM_PAD_PIO_OUT(8, 2, 1),	/* TXERR */
				STM_PAD_PIO_OUT(8, 3, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(8, 4, 1),	/* TXD.1 */
				STM_PAD_PIO_IN(9, 4, -1),	/* RXDV */
				STM_PAD_PIO_IN(9, 5, -1),	/* RXERR */
				STM_PAD_PIO_IN(9, 6, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(9, 7, -1),	/* RXD.1 */
				STM_PAD_PIO_OUT(11, 0, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(11, 1, 1),	/* MDIO */
				STM_PAD_PIO_IN(11, 2, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(11, 3, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII0_CLOCK_OUT:
				 * 0 = PIO11.3 is controlled by PIO muxing,
				 * 1 = PIO11.3 is delayed version of PIO8.0
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 13, 13, 1),
				/* ETHERNET_INTERFACE_ON0 */
				STM_PAD_SYS_CFG(7, 16, 16, 1),
				/* PHY_INTF_SEL0: 4 = RMII */
				STM_PAD_SYS_CFG(7, 24, 26, 4),
				/* ENMII0: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 27, 27, 1),
			},
		},
		[stx7141_ethernet_mode_reverse_mii] = {
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII0_CLOCK_OUT:
				 * 0 = PIO11.3 is controlled by PIO muxing,
				 * 1 = PIO11.3 is delayed version of PIO8.0
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 13, 13, 1),
				/* ETHERNET_INTERFACE_ON0 */
				STM_PAD_SYS_CFG(7, 16, 16, 1),
				/* PHY_INTF_SEL0: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 24, 26, 0),
				/* ENMII0: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 27, 27, 0),
			},
			.gpios_num = 20,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(8, 0, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(8, 1, 1),	/* TXEN */
				STM_PAD_PIO_OUT(8, 2, 1),	/* TXERR */
				STM_PAD_PIO_OUT(8, 3, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(8, 4, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(8, 5, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(8, 6, 1),	/* TXD.3 */
				STM_PAD_PIO_IN(9, 3, -1),	/* RXCLK */
				STM_PAD_PIO_IN(9, 4, -1),	/* RXDV */
				STM_PAD_PIO_IN(9, 5, -1),	/* RXERR */
				STM_PAD_PIO_IN(9, 6, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(9, 7, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(10, 0, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(10, 1, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(10, 6, -1),	/* CRS */
				STM_PAD_PIO_IN(10, 7, -1),	/* COL */
				STM_PAD_PIO_OUT(11, 0, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(11, 1, 1),	/* MDIO */
				STM_PAD_PIO_IN(11, 2, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(11, 3, 1, "PHYCLK"),
			},
		},
	},
	[1] = (struct stm_pad_config []) {
		[stx7141_ethernet_mode_mii] = {
			.gpios_num = 20,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(11, 4, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(11, 5, 1),	/* TXEN */
				STM_PAD_PIO_OUT(11, 6, 1),	/* TXERR */
				STM_PAD_PIO_OUT(11, 7, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(12, 0, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(12, 1, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(12, 2, 1),	/* TXD.3 */
				STM_PAD_PIO_IN(12, 7, -1),	/* RXCLK */
				STM_PAD_PIO_IN(13, 0, -1),	/* RXDV */
				STM_PAD_PIO_IN(13, 1, -1),	/* RXERR */
				STM_PAD_PIO_IN(13, 2, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(13, 3, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(13, 4, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(13, 5, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(14, 2, -1),	/* CRS */
				STM_PAD_PIO_IN(14, 3, -1),	/* COL */
				STM_PAD_PIO_OUT(14, 4, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(14, 5, 1),	/* MDIO */
				STM_PAD_PIO_IN(14, 6, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(14, 7, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII1_CLOCK_OUT:
				 * 0 = PIO14.7 is controlled by PIO muxing,
				 * 1 = PIO14.7 is delayed version of PIO11.4
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 15, 15, 1),
				/* ETHERNET_INTERFACE_ON1 */
				STM_PAD_SYS_CFG(7, 17, 17, 1),
				/* PHY_INTF_SEL1: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 28, 30, 0),
				/* ENMII1: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 31, 31, 1),
			},
		},
		[stx7141_ethernet_mode_gmii] = {
			.gpios_num = 28,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(11, 4, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(11, 5, 1),	/* TXEN */
				STM_PAD_PIO_OUT(11, 6, 1),	/* TXERR */
				STM_PAD_PIO_OUT(11, 7, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(12, 0, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(12, 1, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(12, 2, 1),	/* TXD.3 */
				STM_PAD_PIO_OUT(12, 3, 1),	/* TXD.4 */
				STM_PAD_PIO_OUT(12, 4, 1),	/* TXD.5 */
				STM_PAD_PIO_OUT(12, 5, 1),	/* TXD.6 */
				STM_PAD_PIO_OUT(12, 6, 1),	/* TXD.7 */
				STM_PAD_PIO_IN(12, 7, -1),	/* RXCLK */
				STM_PAD_PIO_IN(13, 0, -1),	/* RXDV */
				STM_PAD_PIO_IN(13, 1, -1),	/* RXERR */
				STM_PAD_PIO_IN(13, 2, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(13, 3, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(13, 4, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(13, 5, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(13, 6, -1),	/* RXD.4 */
				STM_PAD_PIO_IN(13, 7, -1),	/* RXD.5 */
				STM_PAD_PIO_IN(14, 0, -1),	/* RXD.6 */
				STM_PAD_PIO_IN(14, 1, -1),	/* RXD.7 */
				STM_PAD_PIO_IN(14, 2, -1),	/* CRS */
				STM_PAD_PIO_IN(14, 3, -1),	/* COL */
				STM_PAD_PIO_OUT(14, 4, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(14, 5, 1),	/* MDIO */
				STM_PAD_PIO_IN(14, 6, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(14, 7, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII1_CLOCK_OUT:
				 * 0 = PIO14.7 is controlled by PIO muxing,
				 * 1 = PIO14.7 is delayed version of PIO11.4
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 15, 15, 1),
				/* ETHERNET_INTERFACE_ON1 */
				STM_PAD_SYS_CFG(7, 17, 17, 1),
				/* PHY_INTF_SEL1: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 28, 30, 0),
				/* ENMII1: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 31, 31, 1),
			},
		},
		[stx7141_ethernet_mode_rgmii] = { /* TODO */ },
		[stx7141_ethernet_mode_sgmii] = { /* TODO */ },
		[stx7141_ethernet_mode_rmii] = {
			.gpios_num = 12,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_OUT(11, 5, 1),	/* TXEN */
				STM_PAD_PIO_OUT(11, 6, 1),	/* TXERR */
				STM_PAD_PIO_OUT(11, 7, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(12, 0, 1),	/* TXD.1 */
				STM_PAD_PIO_IN(13, 0, -1),	/* RXDV */
				STM_PAD_PIO_IN(13, 1, -1),	/* RXERR */
				STM_PAD_PIO_IN(13, 2, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(13, 3, -1),	/* RXD.1 */
				STM_PAD_PIO_OUT(14, 4, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(14, 5, 1),	/* MDIO */
				STM_PAD_PIO_IN(14, 6, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(14, 7, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII1_CLOCK_OUT:
				 * 0 = PIO14.7 is controlled by PIO muxing,
				 * 1 = PIO14.7 is delayed version of PIO11.4
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 15, 15, 1),
				/* ETHERNET_INTERFACE_ON1 */
				STM_PAD_SYS_CFG(7, 17, 17, 1),
				/* PHY_INTF_SEL1: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 28, 30, 0),
				/* ENMII1: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 31, 31, 1),
			},
		},
		[stx7141_ethernet_mode_reverse_mii] = {
			.gpios_num = 20,
			.gpios = (struct stm_pad_gpio []) {
				STM_PAD_PIO_IN(11, 4, -1),	/* TXCLK */
				STM_PAD_PIO_OUT(11, 5, 1),	/* TXEN */
				STM_PAD_PIO_OUT(11, 6, 1),	/* TXERR */
				STM_PAD_PIO_OUT(11, 7, 1),	/* TXD.0 */
				STM_PAD_PIO_OUT(12, 0, 1),	/* TXD.1 */
				STM_PAD_PIO_OUT(12, 1, 1),	/* TXD.2 */
				STM_PAD_PIO_OUT(12, 2, 1),	/* TXD.3 */
				STM_PAD_PIO_IN(12, 7, -1),	/* RXCLK */
				STM_PAD_PIO_IN(13, 0, -1),	/* RXDV */
				STM_PAD_PIO_IN(13, 1, -1),	/* RXERR */
				STM_PAD_PIO_IN(13, 2, -1),	/* RXD.0 */
				STM_PAD_PIO_IN(13, 3, -1),	/* RXD.1 */
				STM_PAD_PIO_IN(13, 4, -1),	/* RXD.2 */
				STM_PAD_PIO_IN(13, 5, -1),	/* RXD.3 */
				STM_PAD_PIO_IN(14, 2, -1),	/* CRS */
				STM_PAD_PIO_IN(14, 3, -1),	/* COL */
				STM_PAD_PIO_OUT(14, 4, 1),	/* MDC */
				STM_PAD_PIO_BIDIR(14, 5, 1),	/* MDIO */
				STM_PAD_PIO_IN(14, 6, -1),	/* MDINT */
				STM_PAD_PIO_OUT_NAMED(14, 7, 1, "PHYCLK"),
			},
			.sysconfs_num = 4,
			.sysconfs = (struct stm_pad_sysconf []) {
				/* CONF_GMII1_CLOCK_OUT:
				 * 0 = PIO14.7 is controlled by PIO muxing,
				 * 1 = PIO14.7 is delayed version of PIO11.4
				 *     (ETHGMII0_TXCLK) */
				STM_PAD_SYS_CFG(7, 15, 15, 1),
				/* ETHERNET_INTERFACE_ON1 */
				STM_PAD_SYS_CFG(7, 17, 17, 1),
				/* PHY_INTF_SEL1: 0 = GMII/MII */
				STM_PAD_SYS_CFG(7, 28, 30, 0),
				/* ENMII1: 0 = reverse MII, 1 = MII mode */
				STM_PAD_SYS_CFG(7, 31, 31, 0),
			},
		},
	},
};

static void stx7141_ethernet_fix_mac_speed(void *bsp_priv, unsigned int speed)
{
	struct sysconf_field *mac_speed_sel = bsp_priv;

	sysconf_write(mac_speed_sel, (speed == SPEED_100) ? 1 : 0);
}

/*
 * Cut 2 of 7141 has AHB wrapper bug for ethernet gmac.
 * Need to disable read-ahead - performance impact
 */
#define GMAC_AHB_CONFIG		0x7000
#define GMAC_AHB_CONFIG_READ_AHEAD_MASK	0xFFCFFFFF
static void stx7141_ethernet_bus_setup(unsigned long ioaddr)
{
	u32 value = readl(ioaddr + GMAC_AHB_CONFIG);
	value &= GMAC_AHB_CONFIG_READ_AHEAD_MASK;
	writel(value, ioaddr + GMAC_AHB_CONFIG);
}

static struct plat_stmmacenet_data stx7141_ethernet_platform_data[] = {
	[0] = {
		.pbl = 32,
		.has_gmac = 1,
		.fix_mac_speed = stx7141_ethernet_fix_mac_speed,
		/* .pad_config set in stx7141_configure_ethernet() */
	},
	[1] = {
		.pbl = 32,
		.has_gmac = 1,
		.fix_mac_speed = stx7141_ethernet_fix_mac_speed,
		/* .pad_config set in stx7141_configure_ethernet() */
	},
};

static struct platform_device stx7141_ethernet_devices[] = {
	[0] = {
		.name = "stmmaceth",
		.id = 0,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0xfd110000, 0x8000),
			STM_PLAT_RESOURCE_IRQ_NAMED("macirq", ILC_IRQ(40), -1),
		},
		.dev = {
			.power.can_wakeup = 1,
			.platform_data = &stx7141_ethernet_platform_data[0],
		}
	},
	[1] = {
		.name = "stmmaceth",
		.id = 1,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0xfd118000, 0x8000),
			STM_PLAT_RESOURCE_IRQ_NAMED("macirq", ILC_IRQ(47), -1),
		},
		.dev = {
			.power.can_wakeup = 1,
			.platform_data = &stx7141_ethernet_platform_data[1],
		}
	},
};

void __init stx7141_configure_ethernet(int port,
		struct stx7141_ethernet_config *config)
{
	static int configured[ARRAY_SIZE(stx7141_ethernet_devices)];
	struct stx7141_ethernet_config default_config;
	struct stm_pad_config *pad_config;

	BUG_ON(port < 0 || port >= ARRAY_SIZE(stx7141_ethernet_devices));

	BUG_ON(configured[port]);
	configured[port] = 1;

	if (!config)
		config = &default_config;

	/* TODO: RGMII and SGMII configurations */
	BUG_ON(config->mode == stx7141_ethernet_mode_rgmii);
	BUG_ON(config->mode == stx7141_ethernet_mode_sgmii);

	pad_config = &stx7141_ethernet_pad_configs[port][config->mode];

	/* TODO: ext_clk configuration */

	stx7141_ethernet_platform_data[port].pad_config = pad_config;
	stx7141_ethernet_platform_data[port].bus_id = config->phy_bus;

	/* mac_speed */
	stx7141_ethernet_platform_data[port].bsp_priv = sysconf_claim(SYS_CFG,
			7, 20 + port, 20 + port, "stmmac");

	if (cpu_data->cut_major == 2)
		stx7141_ethernet_platform_data[port].bus_setup =
					stx7141_ethernet_bus_setup;

	platform_device_register(&stx7141_ethernet_devices[port]);
}



/* USB resources ---------------------------------------------------------- */

static u64 stx7141_usb_dma_mask = DMA_BIT_MASK(32);

static int stx7141_usb_enable(struct stm_pad_state *state, void *priv);
static void stx7141_usb_disable(struct stm_pad_state *state, void *priv);

static struct stm_plat_usb_data stx7141_usb_platform_data[] = {
	[0] = { /* USB 2.0 port */
		.flags = STM_PLAT_USB_FLAGS_STRAP_16BIT |
				STM_PLAT_USB_FLAGS_STRAP_PLL |
				STM_PLAT_USB_FLAGS_STBUS_CONFIG_THRESHOLD256,
		/* pad_config done in stx7141_configure_usb() */
	},
	[1] = { /* USB 2.0 port */
		.flags = STM_PLAT_USB_FLAGS_STRAP_16BIT |
				STM_PLAT_USB_FLAGS_STRAP_PLL |
				STM_PLAT_USB_FLAGS_STBUS_CONFIG_THRESHOLD256,
		/* pad_config done in stx7141_configure_usb() */
	},
	[2] = { /* USB 1.1 port */
		.flags = STM_PLAT_USB_FLAGS_OPC_MSGSIZE_CHUNKSIZE,
		/* pad_config done in stx7141_configure_usb() */
	},
	[3] = { /* USB 1.1 port */
		.flags = STM_PLAT_USB_FLAGS_OPC_MSGSIZE_CHUNKSIZE,
		/* pad_config done in stx7141_configure_usb() */
	},
};

static struct platform_device stx7141_usb_devices[] = {
	[0] = {
		.name = "stm-usb",
		.id = 0,
		.dev = {
			.dma_mask = &stx7141_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stx7141_usb_platform_data[0],
		},
		.num_resources = 6,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM_NAMED("ehci", 0xfe1ffe00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ehci", ILC_IRQ(93), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("ohci", 0xfe1ffc00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ohci", ILC_IRQ(94), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("wrapper", 0xfe100000,
						    0x100),
			STM_PLAT_RESOURCE_MEM_NAMED("protocol", 0xfe1fff00,
						    0x100),
		},
	},
	[1] = {
		.name = "stm-usb",
		.id = 1,
		.dev = {
			.dma_mask = &stx7141_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stx7141_usb_platform_data[1],
		},
		.num_resources = 6,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM_NAMED("ehci", 0xfeaffe00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ehci", ILC_IRQ(95), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("ohci", 0xfeaffc00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ohci", ILC_IRQ(96), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("wrapper", 0xfea00000,
						    0x100),
			STM_PLAT_RESOURCE_MEM_NAMED("protocol", 0xfeafff00,
						    0x100),
		},
	},
	[2] = {
		.name = "stm-usb",
		.id = 2,
		.dev = {
			.dma_mask = &stx7141_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stx7141_usb_platform_data[2],
		},
		.num_resources = 4,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM_NAMED("ohci", 0xfebffc00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ohci", ILC_IRQ(97), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("wrapper", 0xfeb00000,
						    0x100),
			STM_PLAT_RESOURCE_MEM_NAMED("protocol", 0xfebfff00,
						    0x100),
		},
	},
	[3] = {
		.name = "stm-usb",
		.id = 3,
		.dev = {
			.dma_mask = &stx7141_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stx7141_usb_platform_data[3],
		},
		.num_resources = 4,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM_NAMED("ohci", 0xfecffc00, 0x100),
			STM_PLAT_RESOURCE_IRQ_NAMED("ohci", ILC_IRQ(98), -1),
			STM_PLAT_RESOURCE_MEM_NAMED("wrapper", 0xfec00000,
						    0x100),
			STM_PLAT_RESOURCE_MEM_NAMED("protocol", 0xfecfff00,
						    0x100),
		},
	},
};

static DEFINE_SPINLOCK(stx7141_usb_spin_lock);
static struct sysconf_field *stx7141_usb_phy_clock_sc;
static struct sysconf_field *stx7141_usb_clock_sc;
static struct sysconf_field *stx7141_usb_pll_sc;

enum stx7141_usb_ports {
	stx7141_usb_2_ports = 0,
	stx7141_usb_11_ports = 1,
};
static int stx7141_usb_enabled_ports[2];
static struct sysconf_field *stx7141_usb_oc_enable_sc[2];
static struct sysconf_field *stx7141_usb_oc_invert_sc[2];

static struct sysconf_field *stx7141_usb_enable_sc[4];
static struct sysconf_field *stx7141_usb_pwr_req_sc[4];
static struct sysconf_field *stx7141_usb_pwr_ack_sc[4];

/* stx7141_configure_usb() must ensure the pairs of ports are the same */
static enum stx7141_usb_overcur_mode stx7141_usb_overcur_mode[4];

static int stx7141_usb_enable(struct stm_pad_state *state, void *priv)
{
	int port = (int)priv;
	enum stx7141_usb_ports port_group = port >> 1;

	spin_lock(&stx7141_usb_spin_lock);

	/* State shared by all four ports */
	if ((stx7141_usb_enabled_ports[stx7141_usb_2_ports] +
	     stx7141_usb_enabled_ports[stx7141_usb_11_ports]) == 0) {
		/* Select SATA clock */
		stx7141_usb_phy_clock_sc = sysconf_claim(SYS_CFG,
				40, 2, 3, "USB");
		sysconf_write(stx7141_usb_phy_clock_sc, 0);

		if (cpu_data->cut_major >= 2) {
			/* Enable USB_XTAL_VALID */
			stx7141_usb_pll_sc = sysconf_claim(SYS_CFG,
					4, 10, 10, "USB");
			sysconf_write(stx7141_usb_pll_sc, 1);
		}
	}

	/* Select USB 1.1 clock source */
	if ((port_group == stx7141_usb_11_ports) &&
	    (stx7141_usb_enabled_ports[stx7141_usb_11_ports] == 0)) {
		if (cpu_data->cut_major < 2) {
			/* ENABLE_USB48_CLK: Enable 48 MHz clock */
			stx7141_usb_clock_sc = sysconf_claim(SYS_CFG,
					4, 5, 5, "USB");
			sysconf_write(stx7141_usb_clock_sc, 1);
		} else {
			/* ENABLE_USB_CLK: use USB PHY */
			stx7141_usb_clock_sc = sysconf_claim(SYS_CFG,
					4, 4, 5, "USB");
			sysconf_write(stx7141_usb_clock_sc, 3);
		}
	}

	/* State shared by two ports */
	if ((stx7141_usb_enabled_ports[port_group] == 0) &&
	    (cpu_data->cut_major >= 2)) {
		const struct {
			unsigned char enable, invert;
		} oc_mode[2][3] = {
			{
				/* USB 2 ports, internally active low oc */
				[stx7141_usb_ovrcur_disabled] = { 0, 0 },
				[stx7141_usb_ovrcur_active_high] = { 1, 1 },
				[stx7141_usb_ovrcur_active_low] = { 1, 0 },
			}, {
				/* USB 1.1 ports, internally active high oc */
				[stx7141_usb_ovrcur_disabled] = { 0, 0 },
				[stx7141_usb_ovrcur_active_high] = { 1, 0 },
				[stx7141_usb_ovrcur_active_low] = { 1, 1 },
			}
		};
		enum stx7141_usb_overcur_mode mode =
			stx7141_usb_overcur_mode[port];

		stx7141_usb_oc_enable_sc[port_group] =
			sysconf_claim(SYS_CFG, 4, 12 - port_group,
				      12 - port_group, "USB");
		stx7141_usb_oc_invert_sc[port_group] =
			sysconf_claim(SYS_CFG, 4, 7 - port_group,
				      7 - port_group, "USB");
		sysconf_write(stx7141_usb_oc_enable_sc[port_group],
			      oc_mode[port_group][mode].enable);
		sysconf_write(stx7141_usb_oc_invert_sc[port_group],
			      oc_mode[port_group][mode].invert);
	}

	/* Enable USB port */
	if (cpu_data->cut_major >= 2) {
		static const unsigned char enable_bit[4] = {
			1,
			14,
			8,
			13
		};

		stx7141_usb_enable_sc[port] = sysconf_claim(SYS_CFG, 4,
				enable_bit[port], enable_bit[port], "USB");
		sysconf_write(stx7141_usb_enable_sc[port], 1);
	}

	/* Power up USB port */
	stx7141_usb_pwr_req_sc[port] = sysconf_claim(SYS_CFG,
			32, 7 + port, 7 + port, "USB");
	sysconf_write(stx7141_usb_pwr_req_sc[port], 0);
	stx7141_usb_pwr_ack_sc[port] = sysconf_claim(SYS_STA,
			15, 7 + port, 7 + port, "USB");
	do {
	} while (sysconf_read(stx7141_usb_pwr_ack_sc[port]));

	stx7141_usb_enabled_ports[port_group]++;

	spin_unlock(&stx7141_usb_spin_lock);

	return 0;
}

static void stx7141_usb_disable(struct stm_pad_state *state, void *priv)
{
	int port = (int)priv;
	enum stx7141_usb_ports port_group = (port < 2) ?
			stx7141_usb_2_ports : stx7141_usb_11_ports;

	spin_lock(&stx7141_usb_spin_lock);

	/* Power down USB */
	sysconf_write(stx7141_usb_pwr_req_sc[port], 1);
	sysconf_release(stx7141_usb_pwr_req_sc[port]);
	sysconf_release(stx7141_usb_pwr_ack_sc[port]);

	/* Put USB into reset */
	if (cpu_data->cut_major >= 2) {
		sysconf_write(stx7141_usb_enable_sc[port], 0);
		sysconf_release(stx7141_usb_enable_sc[port]);
	}

	/* State shared by two ports */
	if ((stx7141_usb_enabled_ports[port_group] == 1) &&
			(cpu_data->cut_major >= 2)) {
		sysconf_release(stx7141_usb_oc_invert_sc[port_group]);
		sysconf_release(stx7141_usb_oc_enable_sc[port_group]);
	}

	/* USB 1.1 clock source */
	if ((port_group == stx7141_usb_11_ports) &&
	    (stx7141_usb_enabled_ports[stx7141_usb_11_ports] == 1))
		sysconf_release(stx7141_usb_clock_sc);

	/* State shared by all four ports */
	if ((stx7141_usb_enabled_ports[stx7141_usb_2_ports] +
	     stx7141_usb_enabled_ports[stx7141_usb_11_ports]) == 0) {
		sysconf_release(stx7141_usb_phy_clock_sc);
		if (cpu_data->cut_major >= 2)
			sysconf_release(stx7141_usb_pll_sc);
	}

	spin_unlock(&stx7141_usb_spin_lock);
}

void __init stx7141_configure_usb(int port, struct stx7141_usb_config *config)
{
	static int configured[ARRAY_SIZE(stx7141_usb_devices)];
	struct stm_pad_config *pad_config;

	BUG_ON(port < 0 || port > ARRAY_SIZE(stx7141_usb_devices));

	BUG_ON(configured[port]);

	/* USB over current configuration is complicated.
	 * Cut 1 had hardwired configuration: USB 2 ports active low,
	 * USB 1.1 ports active high.
	 * Cut 2 introduced sysconf registers to control this, but the
	 * bits are shared so both USB 2 ports and both USB 1.1 ports must
	 * agree on the configuration.
	 * See GNBvd63856 and GNBvd65685 for details. */
	if (cpu_data->cut_major == 1) {
		enum stx7141_usb_overcur_mode avail_mode;

		avail_mode = (port < 2) ? stx7141_usb_ovrcur_active_low :
				stx7141_usb_ovrcur_active_high;

		if (config->ovrcur_mode != avail_mode)
			goto fail;
	} else {
		int shared_port = port ^ 1;

		if (configured[shared_port] &&
				(stx7141_usb_overcur_mode[shared_port] !=
				config->ovrcur_mode))
			goto fail;
	}

	configured[port] = 1;
	stx7141_usb_overcur_mode[port] = config->ovrcur_mode;

	pad_config = stm_pad_config_alloc(2, 0);
	stx7141_usb_platform_data[port].pad_config = pad_config;

	if (config->ovrcur_mode != stx7141_usb_ovrcur_disabled) {
		static const struct {
			unsigned char port, pin;
		} oc_pins[4] = {
			{ 4, 6 },
			{ 5, 0 },
			{ 4, 2 },
			{ 4, 4 },
		};

		stm_pad_config_add_pio_in(pad_config, oc_pins[port].port,
				oc_pins[port].pin, -1);
	}

	if (config->pwr_enabled) {
		static const struct {
			unsigned char port, pin, function;
		} pwr_pins[4] = {
			{ 4, 7, 1 },
			{ 5, 1, 1 },
			{ 4, 3, 1 },
			{ 4, 5, 1 },
		};

		stm_pad_config_add_pio_out(pad_config, pwr_pins[port].port,
				pwr_pins[port].pin, pwr_pins[port].function);
	}

	pad_config->custom_claim = stx7141_usb_enable;
	pad_config->custom_release = stx7141_usb_disable,
	pad_config->custom_priv = (void *)port;

	platform_device_register(&stx7141_usb_devices[port]);

	return;

fail:
	printk(KERN_WARNING "Disabling USB port %d, "
			"as requested over current mode not available", port);
	return;
}



/* SATA resources --------------------------------------------------------- */

static struct platform_device stx7141_sata_device = {
	.name = "sata-stm",
	.id = -1,
	.dev.platform_data = &(struct stm_plat_sata_data) {
		.phy_init = 0,
		.pc_glue_logic_init = 0,
		.only_32bit = 0,
	},
	.num_resources = 3,
	.resource = (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0xfe209000, 0x1000),
		STM_PLAT_RESOURCE_IRQ_NAMED("hostc", ILC_IRQ(89), -1),
		STM_PLAT_RESOURCE_IRQ_NAMED("dmac", ILC_IRQ(88), -1),
	},
};

void __init stx7141_configure_sata(void)
{
	static int configured;
	struct sysconf_field *sc;
	struct stm_miphy_sysconf_soft_jtag jtag;
	struct stm_miphy miphy = {
		.ports_num = 1,
		.jtag_tick = stm_miphy_sysconf_jtag_tick,
		.jtag_priv = &jtag,
	};

	BUG_ON(configured++);

	if (cpu_data->cut_major < 2) {
		pr_warning("SATA is only supported on cut 2 or later\n");
		return;
	}

	jtag.tck = sysconf_claim(SYS_CFG, 33, 0, 0, "SATA");
	BUG_ON(!jtag.tck);
	jtag.tms = sysconf_claim(SYS_CFG, 33, 5, 5, "SATA");
	BUG_ON(!jtag.tms);
	jtag.tdi = sysconf_claim(SYS_CFG, 33, 1, 1, "SATA");
	BUG_ON(!jtag.tdi);
	jtag.tdo = sysconf_claim(SYS_STA, 0, 1, 1, "SATA");
	BUG_ON(!jtag.tdo);

	/* SATA_ENABLE */
	sc = sysconf_claim(SYS_CFG, 4, 9, 9, "SATA");
	BUG_ON(!sc);
	sysconf_write(sc, 1);

	/* SATA_SLUMBER_POWER_MODE */
	sc = sysconf_claim(SYS_CFG, 32, 6, 6, "SATA");
	BUG_ON(!sc);
	sysconf_write(sc, 1);

	/* SOFT_JTAG_EN */
	sc = sysconf_claim(SYS_CFG, 33, 6, 6, "SATA");
	BUG_ON(!sc);
	sysconf_write(sc, 0);

	/* TMS should be set to 1 when taking the TAP
	 * machine out of reset... */
	sysconf_write(jtag.tms, 1);

	/* SATA_TRSTN */
	sc = sysconf_claim(SYS_CFG, 33, 4, 4, "SATA");
	BUG_ON(!sc);
	sysconf_write(sc, 1);
	udelay(100);

	stm_miphy_init(&miphy, 0);

	platform_device_register(&stx7141_sata_device);
}