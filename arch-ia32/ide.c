/*
* ATA driver
* 
* TODO:
*	- Everything!
*/
#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/ide.h>

struct ide_channel {
   uint16_t base,  // I/O Base.
			ctrl,  // Control Base
			bmide; // Bus Master IDE
   uint8_t  nIEN;  // nIEN (No Interrupt);
}channels[]={
	{ATA_BAR0,ATA_BAR1,ATA_BAR4+0,0},	// Using default BAR locs. for PATA
	{ATA_BAR2,ATA_BAR3,ATA_BAR4+8,0}
};


static void ide_write(struct ide_channel channel, uint8_t reg_offset, 
uint8_t data) {
	outb(channel.base  + reg_offset, data);
}

static uint8_t ide_read(struct ide_channel channel, uint8_t reg_offset) {
	uint8_t result;
	result = inb(channel.base  + reg_offset);
	return result;
}

static void __init ata_init(void)
{
	int i,j;
	
	ide_write(channels[ATA_PRIMARY], ATA_REG_CONTROL, 2);
	ide_write(channels[ATA_SECONDARY], ATA_REG_CONTROL, 2);
	
	for(i=0;i<2;i++) {
		for(j=0;j<2;j++) {
			// 1: Select drive command sent, not sure what 0xA0 is.
			ide_write(channels[i], ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
			
			// 2: Identify drive command
			ide_write(channels[i], ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			
			// 3: See what drives we have active
			if(ide_read(channels[i], ATA_REG_STATUS) != 0) {
				printk("IDE: Drive detected at %i,%i\n", i, j);
			}
			
		}
	}
}

driver_init(ata_init);