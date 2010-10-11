/* 
 * Really primitive serial driver
*/
#include <scaraOS/kernel.h>
#include <arch/8259a.h>
#include <arch/serio.h>
#include <arch/io.h>

#define COM1	0x3f8
#define COM2	0x2f8
#define COM3	0x3e8
#define COM4	0x2e8

void serio_init(void)
{
	uint8_t lcr;

	/* disable interrupts */
	outb(COM1 + 1, 0);

	lcr = inb(COM1 + 3);

	/* 8 bits */
	lcr |= 0x3;

	/* 1 stop bit */
	lcr &= ~(1 << 2);

	/* no parity */
	lcr &= ~(1 << 3);

	/* set baud rate divisor to 1 */
	outb(COM1 + 3, lcr | 0x80);
	outb(COM1 + 0, 0);
	outb(COM1 + 1, 1);
	outb(COM1 + 3, lcr);
}

void serio_put(uint8_t c)
{
	outb(COM1 + 0, c);
}

static void serio_isr(int irq)
{
	uint32_t base;
	uint8_t c;

	switch(irq) {
	case 4:
		base = COM1;
		break;
	default:
		return;
	}

	for(c = inb(base + 5); c & 1; c = inb(base + 5)) {
		uint8_t k;
		k = inb(base + 0);
		printk("%c", (k == '\r') ? '\n' : k);
	}
}

static void serio_driver_init(void)
{
	printk("serio: com1 initialised\n");
	outb(COM1 + 1, 1);

	set_irq_handler(4, serio_isr);
	irq_on(4);
}

driver_init(serio_driver_init);
