/*
 *  ------------------------------------------------------------------------
 *  spi_stm.c SPI/SSC driver for STMicroelectronics platforms
 *  ------------------------------------------------------------------------
 *
 *  Copyright (c) 2008 STMicroelectronics Limited
 *  Author: Angus Clark <Angus.Clark@st.com>
 *
 *  May be copied or modified under the terms of the GNU General Public
 *  License Version 2.0 only.  See linux/COPYING for more information.
 *
 *  ------------------------------------------------------------------------
 *  Changelog:
 *  2008-01-24 (angus.clark@st.com)
 *    - Initial version
 *  2008-08-28 (angus.clark@st.com)
 *    - Updates to fit with changes to 'ssc_pio_t'
 *    - SSC accesses now all 32-bit, for compatibility with 7141 Comms block
 *    - Updated to handle 7141 PIO ALT configuration
 *    - Support for user-defined, per-bus, chip_select function.  Specified
 *      in board setup
 *    - Bug fix for rx_bytes_pending updates
 *
 *  ------------------------------------------------------------------------
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/uaccess.h>
#include <linux/param.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/stm/platform.h>
#include <linux/stm/ssc.h>

#undef dgb_print

#ifdef CONFIG_SPI_DEBUG
#define SPI_LOOP_DEBUG
#define dgb_print(fmt, args...)  printk(KERN_INFO "%s: " \
					fmt, __func__ , ## args)
#else
#define dgb_print(fmt, args...)	do { } while (0)
#endif

#define NAME "spi-stm"

struct spi_stm {
	/* SSC SPI Controller */
	struct spi_bitbang	bitbang;
	unsigned long		base;
	unsigned int		fcomms;
	struct platform_device  *pdev;

	/* SSC SPI current transaction */
	const u8		*tx_ptr;
	u8			*rx_ptr;
	u16			bits_per_word;
	unsigned int		baud;
	unsigned int		tx_bytes_pending;
	unsigned int		rx_bytes_pending;
	struct completion	done;
};

static void spi_stm_gpio_chipselect(struct spi_device *spi, int value)
{
	unsigned int out;

	dgb_print("\n");

	if (spi->chip_select == (typeof(spi->chip_select))(STM_GPIO_INVALID))
		return;

	if (value == BITBANG_CS_ACTIVE)
		out = spi->mode & SPI_CS_HIGH ? 1 : 0;
	else
		out = spi->mode & SPI_CS_HIGH ? 0 : 1;

	if (unlikely(!spi->controller_data)) {
		/* Allocate the GPIO when called for the first time.
		 * FIXME: This code leaks a gpio! (no gpio_free()) */
		if (gpio_request(spi->chip_select, "spi_stm_gpio CS") < 0) {
			printk(KERN_ERR NAME " Failed to allocate CS pin\n");
			return;
		}
		spi->controller_data = (void *)1;
		gpio_direction_output(spi->chip_select, out);
	} else {
		gpio_set_value(spi->chip_select, out);
	}

	dgb_print("%s PIO%d[%d] -> %d \n",
		  value == BITBANG_CS_ACTIVE ? "select" : "deselect",
		  stm_gpio_port(spi->chip_select),
		  stm_gpio_pin(spi->chip_select), out);

	return;
}

static int spi_stm_setup_transfer(struct spi_device *spi,
				     struct spi_transfer *t)
{
	struct spi_stm *spi_stm;
	u32 hz;
	u8 bits_per_word;
	u32 reg;
	u32 sscbrg;

	spi_stm = spi_master_get_devdata(spi->master);
	bits_per_word = (t) ? t->bits_per_word : 0;
	hz = (t) ? t->speed_hz : 0;

	/* If not specified, use defaults */
	if (!bits_per_word)
		bits_per_word = spi->bits_per_word;
	if (!hz)
		hz = spi->max_speed_hz;

	/* Actually, can probably support 2-16 without any other change!!! */
	if (bits_per_word != 8 && bits_per_word != 16) {
		printk(KERN_ERR NAME " unsupported bits_per_word=%d\n",
		       bits_per_word);
		return -EINVAL;
	}
	spi_stm->bits_per_word = bits_per_word;

	/* Set SSC_BRF */
	/* TODO: program prescaler for slower baudrates */
	sscbrg = spi_stm->fcomms/(2*hz);
	if (sscbrg < 0x07 || sscbrg > (0x1 << 16)) {
		printk(KERN_ERR NAME " baudrate outside valid range"
		       " %d (sscbrg = %d)\n", hz, sscbrg);
		return -EINVAL;
	}
	spi_stm->baud = spi_stm->fcomms/(2*sscbrg);
	if (sscbrg == (0x1 << 16)) /* 16-bit counter wraps */
		sscbrg = 0x0;
	dgb_print("setting baudrate: hz = %d, sscbrg = %d\n", hz, sscbrg);
	ssc_store32(spi_stm, SSC_BRG, sscbrg);

	 /* Set SSC_CTL and enable SSC */
	 reg = ssc_load32(spi_stm, SSC_CTL);
	 reg |= SSC_CTL_MS;

	 if (spi->mode & SPI_CPOL)
		 reg |= SSC_CTL_PO;
	 else
		 reg &= ~SSC_CTL_PO;

	 if (spi->mode & SPI_CPHA)
		 reg |= SSC_CTL_PH;
	 else
		 reg &= ~SSC_CTL_PH;

	 if ((spi->mode & SPI_LSB_FIRST) == 0)
		 reg |= SSC_CTL_HB;
	 else
		 reg &= ~SSC_CTL_HB;

	 if (spi->mode & SPI_LOOP)
		 reg |= SSC_CTL_LPB;
	 else
		 reg &= ~SSC_CTL_LPB;

	 reg &= 0xfffffff0;
	 reg |= (bits_per_word - 1);

	 /* CHECK!: are we always going to use FIFO or
	    do I need CONFIG_STM_SPI_HW_FIFO? */
	 reg |= SSC_CTL_EN_TX_FIFO | SSC_CTL_EN_RX_FIFO;
	 reg |= SSC_CTL_EN;

	 dgb_print("ssc_ctl = 0x%04x\n", reg);
	 ssc_store32(spi_stm, SSC_CTL, reg);

	 /* Clear the status register */
	 ssc_load32(spi_stm, SSC_RBUF);

	 return 0;
}

/* the spi->mode bits understood by this driver: */
#define MODEBITS  (SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_LOOP | SPI_CS_HIGH)
static int spi_stm_setup(struct spi_device *spi)
{
	struct spi_stm *spi_stm;
	int retval;

	spi_stm = spi_master_get_devdata(spi->master);

	if (spi->mode & ~MODEBITS) {
		printk(KERN_ERR NAME "unsupported mode bits %x\n",
			  spi->mode & ~MODEBITS);
		return -EINVAL;
	}

	if (!spi->max_speed_hz)  {
		printk(KERN_ERR NAME " max_speed_hz unspecified\n");
		return -EINVAL;
	}

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	retval = spi_stm_setup_transfer(spi, NULL);
	if (retval < 0)
		return retval;

	return 0;
}

/* For SSC SPI as MASTER, TX/RX is handled as follows:

   1. Fill the TX_FIFO with up to (SSC_TXFIFO_SIZE - 1) words, and enable
      TX_FIFO_EMPTY interrupts.
   2. When the last word of TX_FIFO is copied to the shift register,
      a TX_FIFO_EMPTY interrupt is issued, and the last word will *start* being
      shifted out/in.
   3. On receiving a TX_FIFO_EMPTY interrupt, copy all *available* received
      words from the RX_FIFO. Note, depending on the time taken to shift out/in
      the 'last' word compared to the IRQ latency, the 'last' word may not be
      available yet in the RX_FIFO.
   4. If there are more bytes to TX, refill the TX_FIFO.  Since the 'last' word
      from the previous iteration may still be (or about to be) in the RX_FIFO,
      only add up to (SSC_TXFIFO_SIZE - 1) words.  If all bytes have been
      transmitted, disable TX and set completion.
   5. If we are interested in the received data, check to see if the 'last' word
      has been received.  If not, then wait the period of shifting 1 word, then
      read the 'last' word from the RX_FIFO.

*/
static void spi_stm_fill_tx_fifo(struct spi_stm *spi_stm)
{
	union {
		u8 bytes[4];
		u32 dword;
	} tmp = {.dword = 0,};
	int i;

	for (i = 0;
	     i < SSC_TXFIFO_SIZE - 1 && spi_stm->tx_bytes_pending > 0; i++) {
		if (spi_stm->bits_per_word > 8) {
			if (spi_stm->tx_ptr) {
				tmp.bytes[1] = *spi_stm->tx_ptr++;
				tmp.bytes[0] = *spi_stm->tx_ptr++;
			} else {
				tmp.bytes[1] = 0;
				tmp.bytes[0] = 0;
			}

			spi_stm->tx_bytes_pending -= 2;

		} else {
			if (spi_stm->tx_ptr)
				tmp.bytes[0] = *spi_stm->tx_ptr++;
			else
				tmp.bytes[0] = 0;

			spi_stm->tx_bytes_pending--;
		}
		ssc_store32(spi_stm, SSC_TBUF, tmp.dword);
	}
}

static int spi_stm_rx_mopup(struct spi_stm *spi_stm)
{
	unsigned long word_period_ns;
	u32 rx_fifo_status;
	union {
		u8 bytes[4];
		u32 dword;
	} tmp = {.dword = 0,};

	dgb_print("\n");

	word_period_ns = 1000000000 / spi_stm->baud;
	word_period_ns *= spi_stm->bits_per_word;

	/* delay for period equivalent to shifting 1 complete word
	   out of and into shift register */
	ndelay(word_period_ns);

	/* Check 'last' word is actually there! */
	rx_fifo_status = ssc_load32(spi_stm, SSC_RX_FSTAT);
	if (rx_fifo_status == 1) {
		tmp.dword = ssc_load32(spi_stm, SSC_RBUF);

		if (spi_stm->bits_per_word > 8) {
			if (spi_stm->rx_ptr) {
				*spi_stm->rx_ptr++ = tmp.bytes[1];
				*spi_stm->rx_ptr++ = tmp.bytes[0];
			}
			spi_stm->rx_bytes_pending -= 2;
		} else {
			if (spi_stm->rx_ptr)
				*spi_stm->rx_ptr++ = tmp.bytes[0];
			spi_stm->rx_bytes_pending--;
		}
	} else {
		dgb_print("should only be one word in RX_FIFO"
			  "(rx_fifo_status = %d)\n", rx_fifo_status);
	}

	return 0;
}


static int spi_stm_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_stm *spi_stm;

	dgb_print("\n");

	spi_stm = spi_master_get_devdata(spi->master);

	spi_stm->tx_ptr = t->tx_buf;
	spi_stm->rx_ptr = t->rx_buf;
	spi_stm->tx_bytes_pending = t->len;
	spi_stm->rx_bytes_pending = t->len;
	INIT_COMPLETION(spi_stm->done);

	/* fill TX_FIFO */
	spi_stm_fill_tx_fifo(spi_stm);

	/* enable TX_FIFO_EMPTY interrupts */
	ssc_store32(spi_stm, SSC_IEN, SSC_IEN_TIEN);

	/* wait for all bytes to be transmitted*/
	wait_for_completion(&spi_stm->done);

	/* check 'last' byte has been received */
	/* NOTE: need to read rxbuf, even if ignoring the result! */
	if (spi_stm->rx_bytes_pending)
		spi_stm_rx_mopup(spi_stm);

	/* disable ints */
	ssc_store32(spi_stm, SSC_IEN, 0x0);

	return t->len - spi_stm->tx_bytes_pending;
}



static irqreturn_t spi_stm_irq(int irq, void *dev_id)
{
	struct spi_stm *spi_stm = (struct spi_stm *)dev_id;
	unsigned int rx_fifo_status;
	u32 ssc_status;

	union {
		u8 bytes[4];
		u32 dword;
	} tmp = {.dword = 0,};

	ssc_status = ssc_load32(spi_stm, SSC_STA);

	/* FIFO_TX_EMPTY */
	if (ssc_status & SSC_STA_TIR) {
		/* Find number of words available in RX_FIFO: 8 if RX_FIFO_FULL,
		   else SSC_RX_FSTAT (0-7)
		*/
		rx_fifo_status = (ssc_status & SSC_STA_RIR) ? 8 :
			ssc_load32(spi_stm, SSC_RX_FSTAT);

		/* Read all available words from RX_FIFO */
		while (rx_fifo_status) {
			tmp.dword = ssc_load32(spi_stm, SSC_RBUF);

			if (spi_stm->bits_per_word > 8) {
				if (spi_stm->rx_ptr) {
					*spi_stm->rx_ptr++ = tmp.bytes[1];
					*spi_stm->rx_ptr++ = tmp.bytes[0];
				}
				spi_stm->rx_bytes_pending -= 2;
			} else {
				if (spi_stm->rx_ptr)
					*spi_stm->rx_ptr++ = tmp.bytes[0];
				spi_stm->rx_bytes_pending--;
			}

			rx_fifo_status = ssc_load32(spi_stm, SSC_RX_FSTAT);
		}

		/* See if there is more data to send */
		if (spi_stm->tx_bytes_pending > 0)
			spi_stm_fill_tx_fifo(spi_stm);
		else {
			/* No more data to send */
			ssc_store32(spi_stm, SSC_IEN, 0x0);
			complete(&spi_stm->done);
		}
	}

	return IRQ_HANDLED;
}


static int __init spi_stm_probe(struct platform_device *pdev)
{
	struct stm_plat_ssc_data *plat_data = pdev->dev.platform_data;
	struct spi_master *master;
	struct resource *res;
	struct spi_stm *spi_stm;

	u32 reg;

	/* FIXME: nice error path would be appreciated... */

	master = spi_alloc_master(&pdev->dev, sizeof(struct spi_stm));
	if (!master)
		return -ENOMEM;

	platform_set_drvdata(pdev, master);

	spi_stm = spi_master_get_devdata(master);
	spi_stm->bitbang.master = spi_master_get(master);
	spi_stm->bitbang.setup_transfer = spi_stm_setup_transfer;
	spi_stm->bitbang.txrx_bufs = spi_stm_txrx_bufs;
	spi_stm->bitbang.master->setup = spi_stm_setup;

	if (plat_data->spi_chipselect)
		spi_stm->bitbang.chipselect = plat_data->spi_chipselect;
	else
		spi_stm->bitbang.chipselect = spi_stm_gpio_chipselect;

	/* chip_select field of spi_device is declared as u8 and therefore
	 * limits number of GPIOs that can be used as a CS line. Sorry. */
	master->num_chipselect =
			sizeof(((struct spi_device *)0)->chip_select) * 256;
	master->bus_num = pdev->id;
	init_completion(&spi_stm->done);

	/* Get resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	if (!devm_request_mem_region(&pdev->dev, res->start,
				     res->end - res->start, "spi")) {
		printk(KERN_ERR NAME " Request mem 0x%x region failed\n",
		       res->start);
		return -ENOMEM;
	}

	spi_stm->base =
		(unsigned long) devm_ioremap_nocache(&pdev->dev, res->start,
						     res->end - res->start);
	if (!spi_stm->base) {
		printk(KERN_ERR NAME " Request iomem 0x%x region failed\n",
		       res->start);
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_ERR NAME " Request irq %d failed\n", res->start);
		return -ENODEV;
	}

	if (devm_request_irq(&pdev->dev, res->start, spi_stm_irq,
		IRQF_DISABLED, dev_name(&pdev->dev), spi_stm) < 0) {
		printk(KERN_ERR NAME " Request irq failed\n");
		return -ENODEV;
	}

	if (!devm_stm_pad_claim(&pdev->dev, plat_data->pad_config,
			dev_name(&pdev->dev))) {
		printk(KERN_ERR NAME " Pads request failed!\n");
		return -ENODEV;
	}

	/* Disable I2C and Reset SSC */
	ssc_store32(spi_stm, SSC_I2C, 0x0);
	reg = ssc_load16(spi_stm, SSC_CTL);
	reg |= SSC_CTL_SR;
	ssc_store32(spi_stm, SSC_CTL, reg);

	udelay(1);
	reg = ssc_load32(spi_stm, SSC_CTL);
	reg &= ~SSC_CTL_SR;
	ssc_store32(spi_stm, SSC_CTL, reg);

	/* Set SSC into slave mode before reconfiguring PIO pins */
	reg = ssc_load32(spi_stm, SSC_CTL);
	reg &= ~SSC_CTL_MS;
	ssc_store32(spi_stm, SSC_CTL, reg);

	spi_stm->fcomms = clk_get_rate(clk_get(NULL, "comms_clk"));;

	/* Start bitbang worker */
	if (spi_bitbang_start(&spi_stm->bitbang)) {
		printk(KERN_ERR NAME
		       " The SPI Core refuses the spi_stm adapter\n");
		return -1;
	}

	printk(KERN_INFO NAME ": Registered SPI Bus %d\n", master->bus_num);

	return 0;
}

static int spi_stm_remove(struct platform_device *pdev)
{
	struct spi_stm *spi_stm;
	struct spi_master *master;

	master = platform_get_drvdata(pdev);
	spi_stm = spi_master_get_devdata(master);

	spi_bitbang_stop(&spi_stm->bitbang);

	/* FIXME: Resources release... */

	return 0;
}

static struct platform_driver spi_hw_driver = {
	.driver.name = NAME,
	.driver.owner = THIS_MODULE,
	.probe = spi_stm_probe,
	.remove = spi_stm_remove,
};


static int __init spi_stm_init(void)
{
	printk(KERN_INFO NAME ": SSC SPI Driver\n");
	return platform_driver_register(&spi_hw_driver);
}

static void __exit spi_stm_exit(void)
{
	dgb_print("\n");
	platform_driver_unregister(&spi_hw_driver);
}

module_init(spi_stm_init);
module_exit(spi_stm_exit);

MODULE_AUTHOR("STMicroelectronics <www.st.com>");
MODULE_DESCRIPTION("STM SSC SPI driver");
MODULE_LICENSE("GPL");