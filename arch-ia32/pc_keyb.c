/*
 * Primitive PC keyboard drivers. I suppose there will be an input
 * device layer at some point which we will hook in to
*/
#include <scaraOS/kernel.h>
#include <arch/8259a.h>
#include <arch/idt.h>
#include <arch/irq.h>
#include <arch/io.h>

#include <arch/pc_keyb.h>

#define KB_BUFLEN 0x100

static volatile uint8_t kb_buffer[KB_BUFLEN];
static volatile uint8_t kb_len;

static void kb_isr(int irq)
{
	uint8_t x;
	long flags;

	x = inb(KBR_DATA);
	if ( kb_len >= KB_BUFLEN ) {
		printk("keyboard buffer overflow. scan=0x%x\n", x);
		return;
	}

	lock_irq(flags);
	kb_buffer[kb_len++] = x;
	unlock_irq(flags);
}

static void kb_wait(void)
{
	while(inb(KBR_STATUS) & KB_STAT_IBF)
		/* nothing */;
}

static void kb_setleds(uint8_t l)
{
	outb(KBR_DATA, KB_CMD_SET_LEDS);
	kb_wait();
	outb(KBR_DATA, l);
	kb_wait();
}

static void __init pc_keyb_init(void)
{
	/* Turn the keyboard controller on */
	outb(KBR_DATA, KB_CMD_RESET);
	kb_wait();
	outb(KBR_DATA, KB_CMD_ENABLE);
	kb_wait();
	kb_setleds(0);

	/* Enable IRQ 1 */
	set_irq_handler(1, kb_isr);
	irq_on(1);

	/* Ah, all done */
	dprintk("AT keyboard enabled\n");
}

driver_init(pc_keyb_init);
