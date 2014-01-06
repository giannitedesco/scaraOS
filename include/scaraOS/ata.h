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
#define ATA_DEVCTL_nIEN		(1 << 0) /* !interrupts enabled */
#define ATA_DEVCTL_SRST		(1 << 1) /* software reset */
#define ATA_DEVCTL_RSVD		(1 << 2)

/* ATA Commands */
#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_IDENTIFY_PACKET	0xA1

#endif /* __ARCH_IA32_ATA__ */

