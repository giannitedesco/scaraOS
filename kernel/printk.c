/* 
 * Contains printk. This is too ugly for words at the moment,
 * x86 specific crap is hardcoded in and the formatting functions
 * are primitive. Eventually this will use the vsnprintf code and
 * print using generic output device functions
 */
#include <kernel.h>
#include <arch/processor.h>
#include <stdarg.h>

signed short xpos, ypos;
void vga_curs ( uint16_t x, uint16_t y );
void vga_put(uint8_t);

void printk(const char *format, ...)
{
	static char buf[512];
	va_list args;
	int len, i;
	long flags;

	va_start(args,format);
	len = vsnprintf(buf, sizeof(buf), format, args);

	/* Don't want interrupts coming in and
	 * mangling our output */
	lock_irq(flags);

	for(i = 0; i < len; i++)
		vga_put(buf[i]);

	unlock_irq(flags);
	va_end(args);
	vga_curs(xpos, ypos);
}
