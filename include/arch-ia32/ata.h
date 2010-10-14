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

#define ATA_PRIMARY 	0x00
#define ATA_SECONDARY 	0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC


#endif /* __ARCH_IA32_ATA__ */