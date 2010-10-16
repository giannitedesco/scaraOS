#ifndef __ARCH_IA32_ATA__
#define __ARCH_IA32_ATA__

/* Base address constants for parallel IDE */
#define ATA_BAR0 0x1F0 	
#define ATA_BAR1 0x3F4 	
#define ATA_BAR2 0x170 	
#define ATA_BAR3 0x374 
#define ATA_BAR4 0x000

/* Channel index for our array of channels */
#define ATA_PRIMARY 	0x00
#define ATA_SECONDARY 	0x01

/* Register offsets from the base */
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_LBA1 		0x04
#define ATA_REG_LBA2 		0x05

/* ATA Commands */
#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_IDENTIFY_PACKET	0xA1

/* service routines */
#define ATA_SR_BSY	0x80
#define ATA_SR_ERR	0x01
#define ATA_SR_DRQ	0x08

#endif /* __ARCH_IA32_ATA__ */

