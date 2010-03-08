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

void printk (const char *format, ...)
{
	int c;
	char buf[20];
	va_list args;
	long flags;

	va_start(args,format);

	/* Don't want interrupts coming in and
	 * mangling our output */
	lock_irq(flags);

	while ((c = *format++) != 0)
	{
		if (c != '%')
			vga_put(c);
		else {
			char *p;
	  
			c = *format++;
			switch (c) {
				case 'i':
				case 'd':
				case 'u':
				case 'x':
					itoa(buf, c, va_arg(args,int));
					p = buf;
					goto string;
					break;

				case 's':
					p = va_arg(args,char *);
					if (!p) p = "(null)";
string:
					while (*p) vga_put(*p++);
					break;

				default:
					vga_put(va_arg(args,int));
					break;
			}
		}
	}

	unlock_irq(flags);
	va_end(args);
	vga_curs(xpos, ypos);
}
