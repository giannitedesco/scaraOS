/* 
 * PC/AT/PS2 floppy driver
 *
 * Features
 *  o Supports reading
 *  o Multi-threaded seeks/reads
 *  
 * TODO
 *  o Generic command function
 *  o Protect critical regions with semaphores
 *  o Write support (should be easy)
 *  o Support more than one drive
 *  o Support more than one controller
 *  o Performance optimisations
*/
#include <kernel.h>
#include <blk.h>
#include <task.h>
#include <arch/8259a.h>
#include <arch/dma.h>
#include <arch/idt.h>
#include <arch/irq.h>
#include <arch/io.h>
#include <arch/floppy.h>


/* Floppy interrupt wait queue */
static struct waitq floppyq = INIT_WAITQ(floppyq);

/* Result status registers */
uint8_t status[7], slen=0;

struct floppy_media{
	uint8_t	heads,
			tracks,
			spt, /* sectors per track */
			g3_fmt, /* GAP3 during formatting */
			g3_rw; /* GAP3 during rw */
}geom[]={
	{2, 80, 18, 0x54, 0x1b} /* 1.44M floppy */
};

/* I/O ports where the registers are, for each controller */
struct floppy_ports {
	uint32_t	dor,msr,data;	/* common registers */
	uint32_t	dir,ccr;	/* AT registers */
	uint32_t	dsra,dsrb,drs;	/* PS/2 registers */
}dprts[]={
	/* PC primary controller */
	{0x3f2, 0x3f4, 0x3f5, 0x3f7, 0x3f7, 0x3f0, 0x3f1, 0x3f4},
	/* PC secondary controller */
	{0x372, 0x374, 0x375, 0x377, 0x377, 0x370, 0x371, 0x374},
};

/* Convert a block number to a hardware address */
void floppy_block(block_t block, int *head, int *track, int *sector)
{
	*head = (block % (geom->spt * geom->heads)) / (geom->spt);
	*track = block / (geom->spt * geom->heads);
	*sector = block % geom->spt + 1;
}

/* Wait for readiness then send a byte */
void floppy_send(struct floppy_ports *p, uint8_t b)
{
	uint8_t msr;
	int tmo;
	
	for(tmo=0; tmo<128; tmo++) {
		msr=inb(p->msr);
		if ((msr&0xc0) == 0x80) {
			outb(p->data,b);
			return;
		}
		inb(0x80); /* pause */
	}
}

/* Wait for readiness then recieve a byte */
uint8_t floppy_recv(struct floppy_ports *p)
{
	uint8_t msr;
	int tmo;
	
	for(tmo=0; tmo<128; tmo++) {
		msr=inb(p->msr);
		if ((msr&0xd0) == 0xd0) {
			return inb(p->data);
		}
		inb(0x80); /* pause */
	}
	return 0xff;
}

/* Seek to a given track */
int floppy_seek(struct floppy_ports *p, uint8_t trk)
{
	if ( trk==status[1] ) return 1;

	/* Send the seek command */
	cli();
	floppy_send(p, CMD_SEEK);
	floppy_send(p, 0);
	floppy_send(p, trk);
	sleep_on(&floppyq);

	if ( !(status[0] & ST0_SE) ) {
		printk("floppy: seek failed\n");
		return 0;
	}

	if ( status[1] != trk ) {
		printk("floppy: seek to %u failed (%u)\n", trk, status[1]);
		return 0;
	}

	return 1;
}

/* Recalibrate the selected drive */
void floppy_recal(struct floppy_ports *p)
{
	cli();
	floppy_send(p, CMD_RECAL);
	floppy_send(p, 0);
	sleep_on(&floppyq);
}

/* Low-level block device routine */
int floppy_rw_blk(int write, block_t blk, char *buf, size_t len)
{
	int head, track, sector;
	int tries=3;

	if ( write ) {
		printk("flopp0: read-only device\n");
		return -1;
	}

try_again:
	if ( inb(dprts->dir) & DIR_CHAN ) {
		printk("floppy: disk change on read\n");
		return -1;
	}

	floppy_block(blk, &head, &track, &sector);
	floppy_seek(dprts, track);

	/* select data rate (is this redundant?) */
	outb(dprts->ccr, 0);

	/* Do the read */
	cli();
	dma_read(2, buf, 512);
	floppy_send(dprts, CMD_READ);
	floppy_send(dprts, head<<2);
	floppy_send(dprts, track);
	floppy_send(dprts, head);
	floppy_send(dprts, sector);
	floppy_send(dprts, 2);
	floppy_send(dprts, geom->spt);
	floppy_send(dprts, geom->g3_rw);
	floppy_send(dprts, 0xff);
	sleep_on(&floppyq);

	/* Success */
	if ( (status[0]&0xc0)==0 ) {
		if ( --len ) {
			blk++;
			buf+=512;
			goto try_again;
		}
		return 0;
	}

	if ( --tries ) {
		floppy_recal(dprts);
		goto try_again;
	}
	return -1;
}

/* Reset a floppy controller */
void floppy_reset(struct floppy_ports *p)
{
	/* Stop all motors, dma, interrupts, and select disc A */
	outb(p->dor, 0);

	/* 500k/s */
	outb(p->ccr, 0);

	/* Select drive A */
	outb(p->dor, DOR_DMA|DOR_RSET|DOR_MOTA|0);

	/* Specify mechanical data */
	cli();
	floppy_send(p, CMD_FIX);
	floppy_send(p, 0xcf);
	floppy_send(p, 16<<1);
	sleep_on(&floppyq);

	floppy_recal(p);
}

/* Interrupt service routine, remember ISRs are
 * re-entrant on scaraOS - for now anyway */
void floppy_isr(int irq)
{
	unsigned long flags;

	lock_irq(flags);
	floppy_send(dprts,CMD_SENSEI);
	for(slen=0; slen<7 && (inb(dprts->msr)&MSR_BUSY); slen++) {
		status[slen]=floppy_recv(dprts);
	}
	unlock_irq(flags);

	wake_up(&floppyq);
}

struct blkdev blk_floppy={
	.name = "floppy0",
	.sectsize = 512,
	.ll_rw_blk = floppy_rw_blk
};

void __init floppy_init(void)
{
	uint8_t v;

	blkdev_add(&blk_floppy);

	/* Install handler for IRQ6 */
	set_irq_handler(6, floppy_isr);
	irq_on(6);

	printk("floppy: resetting floppy controllers\n");
	floppy_reset(dprts);

	floppy_send(dprts, CMD_VERSION);
	v = floppy_recv(dprts);
	if ( v == 0x80 ) {
		printk("floppy: NEC765 controller detected\n");
	}else{
		printk("floppy: enhanced controller detected (0x%x)\n", v);
	}
}

driver_init(floppy_init);
