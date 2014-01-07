/*
* ATA driver
*
* TODO:
*	- Everything!
*/
#include <scaraOS/kernel.h>
#include <scaraOS/pci.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/blk.h>
#include <arch/timer.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/8259a.h>
#include <scaraOS/ata.h>

/* Table 4 - PCI Adapter bit definitions in Programming Interface byte */
#define PROGIF_X_MODE		(1 << 0)
#define PROGIF_X_MODE_FIXED	(1 << 1)
#define PROGIF_Y_MODE		(1 << 2)
#define PROGIF_Y_MODE_FIXED	(1 << 3)
#define PROGIF_BUS_MASTER	(1 << 7)

#define DRIVETYPE_UNKNOWN	0
#define DRIVETYPE_ATA		1
#define DRIVETYPE_ATAPI		2

/* Base address constants for parallel ATA */
#define ATA_BAR0 0x1F0
#define ATA_BAR1 0x3F6
#define ATA_BAR2 0x170
#define ATA_BAR3 0x376
#define ATA_BAR4 0x000

struct ata_chan {
	unsigned int cmd_bar;
	unsigned int ctl_bar;
	unsigned int dma_bar;
	unsigned int irq;

#define DISK_TYPE_NONE		0
#define DISK_TYPE_ATA		1
#define DISK_TYPE_ATAPI		2
	unsigned long num_sect;
	uint8_t disk_type;
	uint8_t drvsel;
};

struct ata_dev {
	struct ata_chan ata_x;
	struct ata_chan ata_y;
	unsigned int ata_ref; /* reference count */
	struct ata_blkdev *bd[4]; /* X0, X1, Y0, Y1 */
};

struct ata_blkdev {
	struct blkdev blk;
	struct ata_dev *dev;
	struct ata_chan *chan;
	unsigned int drvsel; /* 0 for master, 1 for slave */
};

static inline
void ata_devctl(const struct ata_chan *chan, uint8_t data)
{
	outb(chan->ctl_bar, ATA_DEVCTL_RSVD|data);
}

static inline
uint8_t ata_alt_status(const struct ata_chan *chan)
{
	return inb(chan->ctl_bar);
}

static inline
uint16_t ata_data(const struct ata_chan *chan)
{
	return inw(chan->cmd_bar);
}

static inline
uint8_t ata_error(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 1);
}

static inline
void ata_feature(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 1, f);
}

static inline
uint8_t ata_get_sector_count(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 2);
}
static inline
void ata_set_sector_count(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 2, f);
}

static inline
uint8_t ata_get_sector(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 3);
}
static inline
void ata_set_sector(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 3, f);
}

static inline
uint8_t ata_get_lcyl(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 4);
}
static inline
void ata_set_lcyl(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 4, f);
}

static inline
uint8_t ata_get_hcyl(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 5);
}
static inline
void ata_set_hcyl(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 5, f);
}

static inline
uint8_t ata_get_drive_head(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 6);
}
static inline
void ata_set_drive_head(const struct ata_chan *chan, uint8_t d, uint8_t h)
{
	/* 0xa0 are the obsolete bits which must be set to one */
	outb(chan->cmd_bar + 6, 0xa0 | (!!d << 4) | (h & 0xf));
}

static inline
uint8_t ata_status(const struct ata_chan *chan)
{
	return inb(chan->cmd_bar + 7);
}

static inline
void ata_command(const struct ata_chan *chan, uint8_t f)
{
	outb(chan->cmd_bar + 7, f);
}

static inline void ata_busy_wait(const struct ata_chan *chan)
{
	do{
		udelay(10);
	}while( ata_status(chan) & ATA_STATUS_BSY );
}

static inline
void ata_drvsel(struct ata_chan *chan, uint8_t drv)
{
	if ( chan->drvsel != !!drv ) {
		ata_set_drive_head(chan, !!drv, 0);
		chan->drvsel = !!drv;
		ata_busy_wait(chan);
	}
}

static int ata_chan_reset(const struct ata_chan *chan)
{
	/* reset and disable interrupts */
	ata_devctl(chan, ATA_DEVCTL_SRST|ATA_DEVCTL_nIEN);
	udelay(10);

	/* de-assert reset, leave interrupts disabled */
	ata_devctl(chan, ATA_DEVCTL_nIEN);
	ata_busy_wait(chan);

	return 0;
}

static void ata_pio_read_blk(const struct ata_chan *chan,
					uint8_t blk[static 512])
{
	uint16_t *ptr = (uint16_t *)blk;
	unsigned int i;
	for(i = 0; i < 256; i++) {
		ptr[i] = ata_data(chan);
	}
}

static void swizzle_string(char *str, size_t len)
{
	uint16_t *ptr;
	unsigned int i;
	/* register at the last possible moment because they need
	 * to be ready for instant operation at register-time.
	*/
	for(i = 0, ptr = (uint16_t *)str; i < len / 2; i++, ptr++) {
		*ptr = be16toh(*ptr);
	}

	for(i = len; i > 1; --i) {
		if ( str[i - 1] == ' ' )
			str[i - 1] = '\0';
		else
			break;
	}
}

static void ident_swizzle(struct ata_identity *ident)
{
	swizzle_string(ident->model, sizeof(ident->model));
	swizzle_string(ident->serial, sizeof(ident->serial));
	swizzle_string(ident->fw_rev, sizeof(ident->fw_rev));
}

static void ata_isr(int irq, void *priv)
{
	printk("ATA IRQ %d\n", irq);
}

static int ata_rw_blk(struct blkdev *kbdev, int write,
			block_t blk, char *buf, size_t len)
{
	struct ata_blkdev *bdev = (struct ata_blkdev *)kbdev;
	printk(" ==== ll_rw_blk: %s\n", bdev->blk.name);
	return -1;
}

#define ATA_MAX_BLKNAME 64
static struct ata_blkdev *ata_new_blkdev(struct ata_dev *dev,
					struct ata_chan *chan,
					unsigned int controller_num,
					unsigned int drvsel)
{
	struct ata_blkdev *bdev;

	bdev = kmalloc(sizeof(*bdev));
	if ( NULL == bdev )
		goto out;

	bdev->blk.name = kmalloc(ATA_MAX_BLKNAME);
	if ( NULL == bdev->blk.name )
		goto out_free;

	snprintf((char *)bdev->blk.name, ATA_MAX_BLKNAME,
		"ata%u%c%u",
		controller_num,
		(chan == &dev->ata_x) ? 'X' : 'Y', drvsel);
	bdev->blk.sectsize = 512;
	bdev->blk.ll_rw_blk = ata_rw_blk,
	bdev->dev = dev;
	bdev->chan = chan;
	bdev->drvsel = drvsel;

	/* success */
	goto out;

out_free:
	kfree(bdev);
out:
	return bdev;
}

static int ata_chan_init(struct ata_dev *dev, struct ata_chan *chan,
			 struct ata_blkdev *bd[static 2])
{
	static unsigned nr_controllers;
	struct ata_identity *ident;
	unsigned int d;
	int ret = -1;

	/* this is a bit too big to fit comfortably on the stack */
	ident = kmalloc(sizeof(*ident));
	if ( NULL == ident ) {
		/* ENOMEM */
		goto out;
	}

	ret = ata_chan_reset(chan);
	if ( ret )
		goto out_free;

	for(d = 0; d < 2; d++) {
		struct ata_blkdev *bdev;

		/* select drive, and initialise drvsel for later,
		 * where we'll be caching this register
		 */
		ata_set_drive_head(chan, d, 0);
		chan->drvsel = d;
		ata_busy_wait(chan);

		ata_command(chan, ATA_CMD_IDENTIFY);
		ata_busy_wait(chan);

		/* we get zero if nothing is attached, and we
		 * can't test for RDY since ATAPI devices don't set it
		*/
		if ( !ata_status(chan) )
			continue;

		/* Identify fails on packet devices with abort error */
		if ( ata_status(chan) & ATA_STATUS_ERR ) {
			uint8_t hi, lo;

			/* but then identifies itself in cylinder regs */
			hi = ata_get_hcyl(chan);
			lo = ata_get_lcyl(chan);
			if ( hi != 0xeb || lo != 0x14 ) {
				continue;
			}

			/* so let's do a packet identify command instead */
			chan->disk_type = DISK_TYPE_ATAPI;
			ata_command(chan, ATA_CMD_IDENTIFY_PACKET);
			ata_busy_wait(chan);
		}else{
			chan->disk_type = DISK_TYPE_ATA;
		}

		ata_pio_read_blk(chan, (uint8_t *)ident);
		ident_swizzle(ident);

		/* FIXME: this doesn't work, we need to grab all
		 * relevant capabilities bits at this time
		*/
		if(ident->features1 & (1 << 10)) {
			chan->num_sect = ident->max_lba;
		}else{
			chan->num_sect = ident->max_lba28;
		}

		bdev = ata_new_blkdev(dev, chan, nr_controllers, d);
		if ( NULL == bdev )
			continue;

		printk("%s: %s - %s (%lu MiB) rev=%s\n",
				bdev->blk.name,
				ident->model,
				ident->serial,
				chan->num_sect / 2048,
				ident->fw_rev);

		bd[!!d] = bdev;
	}

	if ( chan->irq ) {
		set_irq_handler(chan->irq, ata_isr, dev);
		irq_on(chan->irq);
	}

	/* enable interrupts */
	ata_devctl(chan, 0);

	/* success */
	nr_controllers++;
	ret = 0;

out_free:
	kfree(ident);
out:
	return ret;
}

static int ata_dev_init(struct ata_dev *dev)
{
	unsigned int i;
	int ret;

	/* first initialise each channel individually,
	 * detecting drives and creating block devices.
	 */
	ret = ata_chan_init(dev, &dev->ata_x, dev->bd);
	if ( ret )
		return ret;

	ret = ata_chan_init(dev, &dev->ata_y, dev->bd + 2);
	if ( ret )
		return ret;

	/* register at the last possible moment because they need
	 * to be ready for instant operation at register-time.
	*/
	for(i = 0; i < sizeof(dev->bd)/sizeof(dev->bd[0]); i++) {
		if ( dev->bd[i] )
			blkdev_add(&dev->bd[i]->blk);
	}

	return 0;
}

static int pci_ata_attach(struct pci_dev *pcidev)
{
	struct ata_dev *dev;
	uint8_t progif;
	int ret = -1; /* ENOMEM */

	dev = kmalloc0(sizeof(*dev));
	if ( NULL == dev )
		goto out;

	progif = pcidev_conf_read8(pcidev, PCI_CONF_PROGIF);

	if ( progif & PROGIF_X_MODE ) {
		dev->ata_x.cmd_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR0);
		dev->ata_x.ctl_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR1);
		dev->ata_x.dma_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR4 + 0);
		dev->ata_x.irq = pcidev_conf_read8(pcidev, PCI_CONF0_IRQ);
	}else{
		dev->ata_x.cmd_bar = ATA_BAR0;
		dev->ata_x.ctl_bar = ATA_BAR1;
		dev->ata_x.dma_bar = ATA_BAR4 + 0;
		dev->ata_x.irq = 14;
	}

	if ( progif & PROGIF_Y_MODE ) {
		dev->ata_y.cmd_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR2);
		dev->ata_y.ctl_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR3);
		dev->ata_y.dma_bar = pcidev_conf_read32(pcidev,
							PCI_CONF0_BAR4 + 8);
		dev->ata_y.irq = pcidev_conf_read8(pcidev, PCI_CONF0_IRQ);
	}else{
		dev->ata_y.cmd_bar = ATA_BAR2;
		dev->ata_y.ctl_bar = ATA_BAR3;
		dev->ata_y.dma_bar = ATA_BAR4 + 8;
		dev->ata_y.irq = 15;
	}

	if ( !(progif & PROGIF_BUS_MASTER) ) {
		dev->ata_x.dma_bar = 0;
		dev->ata_y.dma_bar = 0;
	}

	/* TODO: sanity checks */

	ret = ata_dev_init(dev);
	if ( ret )
		goto out_free;

	/* success */
	ret = 0;

out_free:
	kfree(dev);
out:
	return ret;
}

static int pci_ata_detach(struct pci_dev *dev)
{
	return -1; /* EBUSY */
}

static const struct pci_driver pci_ata_driver = {
	.name = "PCI ATA",
	.attach = pci_ata_attach,
	.detach = pci_ata_detach,
};

pci_cls_driver(pci_ata_driver, 0xffff0000, 0x01010000);
