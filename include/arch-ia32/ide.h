#ifndef __ARCH_IA32_ATA__
#define __ARCH_IA32_ATA__

/* Base address constants for parallel IDE */
#define ATA_BAR0 0x1F0 	// Primary channel port
#define ATA_BAR1 0x3F4 	// Primary channel control port 
#define ATA_BAR2 0x170 	// Secondary channel port
#define ATA_BAR3 0x374 	// Secondary channel control port
#define ATA_BAR4 0x000 	// Bus Master IDE; refers to the base of I/O range 
						// consisting of 16 ports. Each 8 ports controls DMA 
						// on the primary and secondary channel respectively.

/* Channel index for our array of channels */
#define ATA_PRIMARY 	0x00
#define ATA_SECONDARY 	0x01

/* Register offsets from the base */
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

/* ATA Commands */
#define ATA_CMD_IDENTIFY	0xEC

#endif /* __ARCH_IA32_ATA__ */

