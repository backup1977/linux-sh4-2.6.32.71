/*
 * arch/sh/boards/mach-mb796/setup.c
 *
 * Copyright (C) 2010 STMicroelectronics Limited
 * Author: Pawel Moll <pawel.moll@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * STMicroelectronics STx5289 processor board support.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/stm/pci-synopsys.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx5206.h>
#include <asm/irq.h>



#define MB796_NOTPIORESETMII stm_gpio(0, 6)



static void __init mb796_setup(char **cmdline_p)
{
	printk(KERN_INFO "STMicroelectronics STx5289 Mboard initialisation\n");

	stx5206_early_device_init();

	stx5206_configure_asc(2, &(struct stx5206_asc_config) {
			.hw_flow_control = 1,
			.is_console = 1, });
	stx5206_configure_asc(3, &(struct stx5206_asc_config) {
			.hw_flow_control = 1,
			.is_console = 0, });
}



static struct platform_device mb796_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev.platform_data = &(struct gpio_led_platform_data) {
		.num_leds = 1,
		.leds = (struct gpio_led[]) {
			{	/* J26-D fitted, J21-A 2-3 */
				.name = "LD15 (GREEN)",
				.default_trigger = "heartbeat",
				.gpio = stm_gpio(3, 3),
			},
		},
	},
};



static int mb796_phy_reset(void *bus)
{
	gpio_set_value(MB796_NOTPIORESETMII, 0);
	udelay(15000);
	gpio_set_value(MB796_NOTPIORESETMII, 1);

	return 0;
}

static struct plat_stmmacphy_data mb796_phy_plat_data = {
	/* Micrel */
	.bus_id = 0,
	.phy_addr = -1,
	.phy_mask = 0,
	.interface = PHY_INTERFACE_MODE_MII,
	.phy_reset = &mb796_phy_reset,
};

static struct platform_device mb796_phy_device = {
	.name = "stmmacphy",
	.id = -1,
	.dev.platform_data = &mb796_phy_plat_data,
};



static struct platform_device *mb796_devices[] __initdata = {
	&mb796_leds,
	&mb796_phy_device,
};



/* PCI configuration */

static struct stm_plat_pci_config mb796_pci_config = {
	.pci_irq = {
		[0] = PCI_PIN_DEFAULT,
		[1] = PCI_PIN_DEFAULT, /* J20-C fitted, J20-D not fitted */
		[2] = PCI_PIN_DEFAULT,
		[3] = PCI_PIN_DEFAULT
	},
	.serr_irq = PCI_PIN_UNUSED,
	.idsel_lo = 30, /* J15 2-3 */
	.idsel_hi = 30,
	.req_gnt = {
		[0] = PCI_PIN_DEFAULT,
		[1] = PCI_PIN_UNUSED,
		[2] = PCI_PIN_UNUSED,
		[3] = PCI_PIN_UNUSED
	},
	.pci_clk = 33333333,
	/* Well, this board doesn't really provide any
	 * means of resetting the PCI devices... */
	.pci_reset_gpio = -EINVAL,
};

int pcibios_map_platform_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
       /* We can use the standard function on this board */
       return stx5206_pcibios_map_platform_irq(&mb796_pci_config, pin);
}



static int __init mb796_devices_init(void)
{
	gpio_request(MB796_NOTPIORESETMII, "notPioResetMii");
	gpio_direction_output(MB796_NOTPIORESETMII, 1);

	/* Internal I2C link */
	stx5206_configure_ssc_i2c(0);
	/* LNB (IC20) & CN21 - J34-A & J34-B fitted  */
	stx5206_configure_ssc_i2c(1);
	/* NIM, CN4, CN5 & CN7 - J26-E & J26-F fitted, J21-B 2-3, J19-A 2-3 */
	stx5206_configure_ssc_i2c(2);
	/* HDMI chip (IC34) & CN31 - J26-G & J26-H fitted, J19-B 2-3, J25 2-3 */
	stx5206_configure_ssc_i2c(3);

	stx5206_configure_pci(&mb796_pci_config);

	stx5206_configure_usb();

	/* RMII mode jumper settings:
	 * - J38: 2-3
	 * - J39-A: open
	 * - J39-B: 2-3
	 * - SW4: 1-3=OFF, 4=ON
	 * - SW5: 1-2=ON, 3-4=OFF
	 * Required hardware rework:
	 * - R191-2 and R191-4 need to be pulled down with 1k resistor */
	stx5206_configure_ethernet(&(struct stx5206_ethernet_config) {
			.mode = stx5206_ethernet_mode_rmii,
			.ext_clk = 0,
			.phy_bus = 0, });

	stx5206_configure_lirc(&(struct stx5206_lirc_config) {
			.rx_mode = stx5206_lirc_rx_mode_ir, });

	return platform_add_devices(mb796_devices, ARRAY_SIZE(mb796_devices));
}
arch_initcall(mb796_devices_init);



static void __iomem *mb796_ioport_map(unsigned long port, unsigned int size)
{
	/* However picking somewhere safe isn't as easy as you might
	 * think.  I used to use external ROM, but that can cause
	 * problems if you are in the middle of updating Flash. So I'm
	 * now using the processor core version register, which is
	 * guaranteed to be available, and non-writable. */
	return (void __iomem *)CCN_PVR;
}

struct sh_machine_vector mv_mb796 __initmv = {
	.mv_name		= "mb796",
	.mv_setup		= mb796_setup,
	.mv_nr_irqs		= NR_IRQS,
	.mv_ioport_map		= mb796_ioport_map,
	STM_PCI_IO_MACHINE_VEC
};