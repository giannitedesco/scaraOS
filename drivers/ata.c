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
	unsigned int disk_type;
	unsigned int num_sect;
};

struct ata_dev {
	struct ata_chan ata_x;
	struct ata_chan ata_y;
	unsigned int ata_ref; /* reference count */
};
struct ata_blkdev {
	struct blkdev blk;
	struct ata_dev *dev;
	unsigned int chan; /* 0 for X, 1 for Y */
	unsigned int sel; /* 0 for master, 1 for slave */
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
	/* TODO: cache this to avoid un-necessary drive selections */
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
	for(i = 0, ptr = (uint16_t *)str; i < len / 2; i++, ptr++) {
		*ptr = be16toh(*ptr);
	}

	for(i = len; i > 1; --i) {
		if ( str[i - 1] == ' ' )
			str[i - 1] = '\0';
	}
}

static void ident_swizzle(struct ata_identity *ident)
{

	swizzle_string(ident->model, sizeof(ident->model));
	swizzle_string(ident->serial, sizeof(ident->serial));
	swizzle_string(ident->fw_rev, sizeof(ident->fw_rev));
}

static int ata_chan_init(const struct ata_dev *dev, struct ata_chan *chan)
{
	struct ata_identity *ident;
	unsigned int d;
	int ret = -1;

	ident = kmalloc(sizeof(*ident));
	if ( NULL == ident ) {
		/* ENOMEM */
		goto out;
	}

	ret = ata_chan_reset(chan);
	if ( ret )
		goto out_free;

	for(d = 0; d < 2; d++) {
		ata_set_drive_head(chan, d, 0);
		ata_command(chan, ATA_CMD_IDENTIFY);
		ata_busy_wait(chan);

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

		if(ident->features1 & (1 << 10)) {
			chan->num_sect = ident->max_lba;
		}else{
			chan->num_sect = ident->max_lba28;
		}

		printk("drive%d: %s - %s (%u MiB) rev=%s\n", d,
				ident->model,
				ident->serial,
				chan->num_sect / 2048,
				ident->fw_rev);
	}
	printk("\n");

	/* success */
	ret = 0;

out_free:
	kfree(ident);
out:
	return ret;
}

static int ata_dev_init(struct ata_dev *dev)
{
	int ret;

	ret = ata_chan_init(dev, &dev->ata_x);
	if ( ret )
		return ret;

	ret = ata_chan_init(dev, &dev->ata_y);
	if ( ret )
		return ret;

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

	/* TODO: identify and register attached devices */

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
