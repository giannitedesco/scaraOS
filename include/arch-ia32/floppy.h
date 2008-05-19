#ifndef __ARCH_IA32_FLOPPY__
#define __ARCH_IA32_FLOPPY__

int floppy_rw_blk(int, block_t, char *, size_t);

/* Digital Output Register bits (write-only) */
/* bits 1 and 2 are disc select */
#define DOR_RSET	(1<<2)
#define DOR_DMA		(1<<3)
#define DOR_MOTA	(1<<4)
#define DOR_MOTB	(1<<5)
#define DOR_MOTC	(1<<6)
#define DOR_MOTD	(1<<7)

/* Main Status Register bits (read-only) */
#define MSR_ACTA	(1<<0)
#define MSR_ACTB	(1<<1)
#define MSR_ACTC	(1<<2)
#define MSR_ACTD	(1<<3)
#define MSR_BUSY	(1<<4)
#define MSR_NDMA	(1<<5)
#define MSR_DIO		(1<<6)
#define MSR_MRQ		(1<<7)
#define MSR_READY	(MSR_DIO|MSR_MRQ)

/* Data input register */
#define DIR_CHAN	(1<<7) /* disk change */

/* Status register 0 */
#define ST0_SE		(1<<5)

/* Data commands */
#define CMD_WRITE	0xc5
#define CMD_READ	0xe6
#define CMD_FORMAT	0x4d

/* Control commands */
#define CMD_FIX		0x03
#define CMD_STATUS	0x04
#define CMD_RECAL	0x07
#define CMD_SENSEI	0x08
#define CMD_SECTOR	0x0a
#define CMD_SEEK	0x0f
#define CMD_VERSION	0x10

#endif /* __ARCH_IA32_FLOPPY__ */
