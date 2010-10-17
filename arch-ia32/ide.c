/*
* ATA driver
* 
* TODO:
*	- Everything!
*/
#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/ide.h>

#define DRIVETYPE_UNKNOWN	0
#define DRIVETYPE_IDE 		1
#define DRIVETYPE_ATAPI 	2

/* Base address constants for parallel IDE */
#define IDE_BAR0 0x1F0 	
#define IDE_BAR1 0x3F4 	
#define IDE_BAR2 0x170 	
#define IDE_BAR3 0x374 
#define IDE_BAR4 0x000

static struct ide_channel {
	uint16_t base;  /* I/O Base. */
	uint16_t ctrl;  /* Control Base */
	uint16_t bmide; /* Bus Master IDE */
	uint8_t  nIEN;  /* nIEN (No Interrupt); */
}channels[]={                                    
    	/* default BAR locs. for PATA */
	{IDE_BAR0,IDE_BAR1,IDE_BAR4+0,0},	
	{IDE_BAR2,IDE_BAR3,IDE_BAR4+8,0}
};

struct identity {
	uint16_t _pad1[27]; /* 0-26 */
	uint16_t model[20]; /* 27-46 */
	uint16_t _pad2[13]; /* 47-59 */
	uint32_t max_lba28; /* 60-61 */
	uint16_t _pad3[21]; /* 47-82 */
	uint16_t features1; /* 83 */
	uint16_t _pad4[17]; /* 84 - 99 */
	uint16_t max_lba[4]; /* 100-103 */
	uint16_t _pad5[151]; /* 104-255 */
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
		id->model[k] = __bswap16(id->model[k]);
	}
}

static int drive_type(const struct ide_channel *chan)
{
	unsigned int retries = 12;
	uint8_t lba1, lba2, s;
	int ret = DRIVETYPE_UNKNOWN;

	while( --retries ) {
		s = ide_read(chan, ATA_REG_STATUS);
		if( s & ATA_SR_ERR )
			break;
		if ( (s & (ATA_SR_BSY|ATA_SR_DRQ)) == ATA_SR_DRQ ) {
			ret = DRIVETYPE_IDE;
			goto out;
		}
		inb(0x80); /* pause */
	}

	/* ATA type is not IDE, let's check if it's
	 * ATAPI 
	 */
	lba1 = ide_read(chan, ATA_REG_LBA1);
	lba2 = ide_read(chan, ATA_REG_LBA2);
	if(lba1 == 0x14 && lba2 == 0xEB) {
		/* Drive is ATAPI */
		ide_write(chan, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
		ret = DRIVETYPE_ATAPI;
	}

out:
	for(retries = 12; retries; --retries) {
		s = ide_read(chan, ATA_REG_STATUS);
		if ( s & ATA_SR_ERR )
			return DRIVETYPE_UNKNOWN;
		if ( (s & ATA_SR_BSY) == 0 )
			goto out_ok;
		inb(0x80); /* pause */
	}

	return DRIVETYPE_UNKNOWN; /* ?? */
out_ok:
	return ret;
}

static void __init ata_init(void)
{
	unsigned int i,j;

	/* Disable interrupts */
	ide_write(&channels[ATA_PRIMARY], ATA_REG_CONTROL, 2);
	ide_write(&channels[ATA_SECONDARY], ATA_REG_CONTROL, 2);
	
	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++) {
			struct identity *cur_drv;
			int type;
			uint8_t addr_mode;

			/* Select drive command sent, not sure what 0xA0 is*/
			ide_write(&channels[i], ATA_REG_HDDEVSEL, 
				0xA0 | (j << 4));
			
			/* Identify drive command */
			ide_write(&channels[i], ATA_REG_COMMAND, 
				ATA_CMD_IDENTIFY);
			
			/* See what drives we have active */
			if(ide_read(&channels[i], ATA_REG_STATUS) == 0) {
				continue; /* No device so skip */
			}


			/* Check if drive is ATA */
			type = drive_type(&channels[i]);

			cur_drv = kmalloc(sizeof(struct identity));

			if(cur_drv == NULL) {
				dprintk("IDE: Problem with kmalloc, quitting");
				break;
			}

			ide_read_buffer(&channels[i], (uint16_t *)cur_drv,
				sizeof(struct identity));

			identification_bytesex(cur_drv);

			if(cur_drv->features1 & (1 << 10)) {
				addr_mode = ATA_ADDR_LBA48; 
			}


			switch(type) {
			case DRIVETYPE_IDE:
				printk("IDE:   (%s,%s) - %.*s (%luMB)\n", 
					(const char *[]){"PRIMARY",
						"SECONDARY"}[i], 
					(const char *[]){"MASTER",
						"SLAVE"}[j], 
					sizeof(cur_drv->model), 
					(char*)(cur_drv->model),
					cur_drv->max_lba28 / 1024 / 2);
				break;
			case DRIVETYPE_ATAPI:
				printk("ATAPI: (%s,%s) - %.*s\n", 
					(const char *[]){"PRIMARY",
						"SECONDARY"}[i], 
					(const char *[]){"MASTER",
						"SLAVE"}[j], 
					sizeof(cur_drv->model), 
					(char*)(cur_drv->model));
				break;
			default:
				break;
			} 

			kfree(cur_drv);
		}
	}
}

driver_init(ata_init);
