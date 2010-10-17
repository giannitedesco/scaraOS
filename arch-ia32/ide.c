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
	uint16_t _pad1[27]; /* 0-26 */
	uint16_t model[20]; /* 27-46 */
	uint16_t _pad2[53]; /* 47-99 */
	uint16_t size[4]; /* 100-103 */
	uint16_t _pad3[151]; /* 104-255 */
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

	/* Disable interrupts */
	ide_write(&channels[ATA_PRIMARY], ATA_REG_CONTROL, 2);
	ide_write(&channels[ATA_SECONDARY], ATA_REG_CONTROL, 2);
	
	for(i = 0; i < 2; i++) {
		for(j = 0; j < 2; j++) {
			struct identity *cur_drv;
			int err = 0;
			uint8_t type;

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
			while(1) {
				int status = 
					ide_read(&channels[i], ATA_REG_STATUS);
				if((status & ATA_SR_ERR)) {
					/* Not ATA */
					err = 1;
					break;
				}
				if(!(status & ATA_SR_BSY) &&
					(status & ATA_SR_DRQ)) {
					/* ATA */
					type = ATA_IDE;
					break;
				}
			}

			if(err) {
				/* ATA type is not IDE, let's check if it's
				 * ATAPI 
				 */
				uint8_t lba1 = 
					ide_read(&channels[i], ATA_REG_LBA1);
				uint8_t lba2 =
					ide_read(&channels[i], ATA_REG_LBA2);

				if(lba1 == 0x14 && lba2 == 0xEB) {
					/* Drive is ATAPI */
					type = ATA_ATAPI;
					ide_write(&channels[i], ATA_REG_COMMAND,
						ATA_CMD_IDENTIFY_PACKET);
				}
				else {
					/* Unknown drive type */
					continue;
				}
			}

			cur_drv = kmalloc(sizeof(struct identity));

			if(cur_drv == NULL) {
				dprintk("IDE: Problem with kmalloc, quitting");
				break;
			}

			ide_read_buffer(&channels[i], (uint16_t *)cur_drv,
				sizeof(struct identity));

			identification_bytesex(cur_drv);


			if(type == ATA_IDE) {
				printk("IDE:   %u,%u - %.*s (%d)\n", 
					i, j, 
					sizeof(cur_drv->model), 
					(char*)(cur_drv->model),
					(uint64_t)(cur_drv->size[0])
					);
			} 

			if(type == ATA_ATAPI) {
				printk("ATAPI: %u,%u - %.*s\n", 
					i, j, 
					sizeof(cur_drv->model), 
					(char*)(cur_drv->model));
			}

			kfree(cur_drv);
		}
	}
}

driver_init(ata_init);
