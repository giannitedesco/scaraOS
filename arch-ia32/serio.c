/* 
 * Really primitive serial driver
*/
#include <scaraOS/kernel.h>
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
