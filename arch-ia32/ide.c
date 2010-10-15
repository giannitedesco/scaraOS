/*
* ATA driver
* 
* TODO:
*	- Everything!
*/
#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/ide.h>

static struct ide_channel {
	uint16_t base;  /* I/O Base. */
	uint16_t ctrl;  /* Control Base */
	uint16_t bmide; /* Bus Master IDE */
	uint8_t  nIEN;  /* nIEN (No Interrupt); */
}channels[]={                                    
    	/* default BAR locs. for PATA */
	{ATA_BAR0,ATA_BAR1,ATA_BAR4+0,0},	
	{ATA_BAR2,ATA_BAR3,ATA_BAR4+8,0}
};

struct identity {
	uint16_t generalconf;
	uint16_t cylinders;
	uint16_t reserved;
	uint16_t heads;
	uint16_t unfmtd_bytes_per_track;
	uint16_t unfmtd_bytes_per_sector;
	uint16_t sectors_per_track;
	uint16_t vendor_unq[3];
	uint16_t sn[10];
	uint16_t buf_typ;
	uint16_t buf_sz;
	uint16_t ECC_bytes_avail;
	uint16_t fw_rev[4];
	uint16_t model[20];
	uint16_t max_sec_per_ir;
	uint16_t dblw_io_cap;
	uint16_t capabilities;
	uint16_t rsrvd2;
	uint16_t pio_mode;
	uint16_t dma_mode;
	uint16_t rsrvd3;
	uint16_t cur_cylinders;
	uint16_t cur_heads;
	uint16_t cur_sec_per_track;
	uint16_t cur_cap_in_sectors[2];
	uint16_t rsrvd4;
	uint16_t total_addressable_sectors[2];
	uint16_t singlew_dma_mode;
	uint16_t multiw_dma_mode;
	uint16_t padding[192];
};


static void ide_write(const struct ide_channel *channel, uint8_t reg_offset, 
	uint8_t data) 
{
	outb(channel->base  + reg_offset, data);
}

static uint8_t ide_read(const struct ide_channel *channel, uint8_t reg_offset) 
{
	uint8_t result;
	result = inb(channel->base  + reg_offset);
	return result;
}

static void ide_read_buffer(const struct ide_channel *channel, uint16_t *buf, 
	unsigned int len) 
{
	unsigned int i;
	for(i = 0; i < len; i++) {
		buf[i] = inw(channel->base);
	}
}

/* Some of the values stored are strings which need byteswapping */
static void identification_bytesex(struct identity *id)
{
	unsigned int k;

	for(k = 0; k < ARRAY_SIZE(id->model); k++) {
		id->model[k] = bswap16(id->model[k]);
	}
}

static void __init ata_init(void)
{
	unsigned int i,j;

	ide_write(&channels[ATA_PRIMARY], ATA_REG_CONTROL, 2);
	ide_write(&channels[ATA_SECONDARY], ATA_REG_CONTROL, 2);
	
	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++) {
			struct identity *cur_drv;
			/* Select drive command sent, not sure what 0xA0 is*/
			ide_write(&channels[i], ATA_REG_HDDEVSEL, 
				0xA0 | (j << 4));
			
			/* Identify drive command */
			ide_write(&channels[i], ATA_REG_COMMAND, 
				ATA_CMD_IDENTIFY);
			
			/* See what drives we have active */
			if(ide_read(&channels[i], ATA_REG_STATUS) == 0) {
				continue;
			}

			cur_drv = kmalloc(sizeof(struct identity));

			if(cur_drv == NULL) {
				dprintk("IDE: Problem with kmalloc, quitting");
				break;
			}

			ide_read_buffer(&channels[i], (uint16_t *)cur_drv,
				sizeof(struct identity));

			identification_bytesex(cur_drv);


			printk("IDE: %u,%u - %.*s\n", i, j, 
				sizeof(cur_drv->model), 
				(char*)(cur_drv->model));

			kfree(cur_drv);
		}
	}
}

driver_init(ata_init);
