#ifndef __ARCH_IA32_ATA__
#define __ARCH_IA32_ATA__

/* status bits */
#define ATA_STATUS_ERR		(1 << 0)
#define ATA_STATUS_IDX		(1 << 1)
#define ATA_STATUS_CORR		(1 << 2)
#define ATA_STATUS_DRQ		(1 << 3)
#define ATA_STATUS_DSC		(1 << 4)
#define ATA_STATUS_DWF		(1 << 5)
#define ATA_STATUS_DRDY		(1 << 6)
#define ATA_STATUS_BSY		(1 << 7)

/* device control bits */
#define ATA_DEVCTL_nIEN		(1 << 1) /* !interrupts enabled */
#define ATA_DEVCTL_SRST		(1 << 2) /* software reset */
#define ATA_DEVCTL_RSVD		(1 << 3)

/* ATA Commands */
#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_IDENTIFY_PACKET	0xA1

struct ata_identity {
	uint16_t _pad1[10]; /* 0-10 */
	char serial[20]; /* 10-19 */
	uint16_t _pad2[3]; /* 19-23 */
	char fw_rev[8]; /* 23-26 */
	char model[40]; /* 27-46 */
	uint16_t _pad3[13]; /* 47-59 */
	uint32_t max_lba28; /* 60-61 */
	uint16_t _pad4[21]; /* 47-82 */
	uint16_t features1; /* 83 */
	uint16_t _pad5[16]; /* 84 - 99 */
	uint64_t max_lba; /* 100-103 */
	uint16_t _pad6[152]; /* 104-255 */
} __attribute__((packed));

#endif /* __ARCH_IA32_ATA__ */

