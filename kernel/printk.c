/* 
 * Contains printk. This is too ugly for words at the moment,
 * x86 specific crap is hardcoded in and the formatting functions
 * are primitive. Eventually this will use the vsnprintf code and
 * print using generic output device functions
 */
#include <scaraOS/kernel.h>
#include <arch/processor.h>
#include <stdarg.h>

signed short xpos, ypos;
void vga_curs ( uint16_t x, uint16_t y );
void vga_put(uint8_t);

static void printkv_unlocked(const char *format, va_list va)
{
	static char buf[512];
	int len, i;

	len = vsnprintf(buf, sizeof(buf), format, va);
	/* Don't want interrupts coming in and
	 * mangling our output */
	for(i = 0; i < len; i++)
		vga_put(buf[i]);
	vga_curs(xpos, ypos);
}

void printkv(const char *format, va_list va)
{
	long flags;
	lock_irq(flags);
	printkv_unlocked(format, va);
	unlock_irq(flags);
}

static void printk_unlocked(const char *format, ...)
{
	va_list va;
	va_start(va, format);
	printkv_unlocked(format, va);
	va_end(va);
}

void printk(const char *format, ...)
{
	va_list va;
	long flags;
	va_start(va, format);
	lock_irq(flags);
	printkv_unlocked(format, va);
	unlock_irq(flags);
	va_end(va);
}

void hex_dumpk(const uint8_t *tmp, size_t len, size_t llen)
{
	size_t i, j;
	size_t line;
	long flags;

	if ( 0 == len )
		return;

	lock_irq(flags);

	for(j = 0; j < len; j += line, tmp += line) {
		if ( j + llen > len ) {
			line = len - j;
		}else{
			line = llen;
		}

		printk_unlocked(" | %05x : ", j);

		for(i = 0; i < line; i++) {
			if ( isprint(tmp[i]) ) {
				printk_unlocked("%c", tmp[i]);
			}else{
				printk_unlocked(".");
			}
		}

		for(; i < llen; i++)
			printk_unlocked(" ");

		for(i = 0; i < line; i++)
			printk_unlocked(" %02x", tmp[i]);

		printk_unlocked("\n");
	}
	printk_unlocked("\n");

	unlock_irq(flags);
}
