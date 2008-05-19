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

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal.  */
void itoa(char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor = 10;
  
	/* If %d is specified and D is minus, put `-' in the head.  */
	if (base == 'd' && d < 0)
	{
		*p++ = '-';
		buf++;
		ud = -d;
	}else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0.  */
	do
	{
		int remainder = ud % divisor;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	}while (ud /= divisor);

	/* Terminate BUF.  */
	*p = 0;
  
	/* Reverse BUF.  */
	p1 = buf;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}

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
