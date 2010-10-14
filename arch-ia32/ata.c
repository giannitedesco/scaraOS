/*
* ATA driver
* 
* TODO:
*	- Everything!
*/
#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/ata.h>

struct IDEChannelRegisters {
   unsigned short base;  // I/O Base.
   unsigned short ctrl;  // Control Base
   unsigned short bmide; // Bus Master IDE
   unsigned char  nIEN;  // nIEN (No Interrupt);
} channels[2];

static void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      outb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

static uint8_t ide_read(uint8_t channel, uint8_t reg) {
   uint8_t result;
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      result = inb(channels[channel].base + reg - 0x00);
   else if (reg < 0x0C)
      result = inb(channels[channel].base  + reg - 0x06);
   else if (reg < 0x0E)
      result = inb(channels[channel].ctrl  + reg - 0x0A);
   else if (reg < 0x16)
      result = inb(channels[channel].bmide + reg - 0x0E);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
   return result;
}

static void __init ata_init(void)
{
	int i, j;
	
	channels[ATA_PRIMARY].base = (ATA_BAR0 & 0xFFFFFFFC) + 0x1F0 * (!ATA_BAR0);
	channels[ATA_PRIMARY].ctrl = (ATA_BAR1 & 0xFFFFFFFC) + 0x3F4 * (!ATA_BAR1);
	channels[ATA_SECONDARY].base = (ATA_BAR2 & 0xFFFFFFFC) + 0x170 * (!ATA_BAR2);
	channels[ATA_SECONDARY].ctrl = (ATA_BAR3 & 0xFFFFFFFC) + 0x374 * (!ATA_BAR3);
	channels[ATA_PRIMARY].bmide = (ATA_BAR4 & 0xFFFFFFFC) + 0;
	channels[ATA_SECONDARY].bmide = (ATA_BAR4 & 0xFFFFFFFC) + 8;
	
   ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
   ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			// (I) Select Drive:
         ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.

         // (II) Send ATA Identify Command:
         ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

         // (III) Polling:
         	if (ide_read(i, ATA_REG_STATUS) == 0) {
				printk("ATA: No device at Channel: %i, Drive: %i\n", i, j);
				continue;
			}  else {
				printk("ATA: Device found at Channel: %i, Drive: %i\n", i, j);
			} // If Status = 0, No Device.
		}
	}
		
	
}

driver_init(ata_init);