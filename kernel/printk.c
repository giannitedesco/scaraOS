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

void printkv(const char *format, va_list va)
{
	static char buf[512];
	long flags;
	int len, i;

	len = vsnprintf(buf, sizeof(buf), format, va);
	/* Don't want interrupts coming in and
	 * mangling our output */
	lock_irq(flags);
	for(i = 0; i < len; i++)
		vga_put(buf[i]);
	vga_curs(xpos, ypos);
	unlock_irq(flags);
}

void printk(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	printkv(format, va);
	va_end(va);
}
