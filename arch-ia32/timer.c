/*
 * Controls the intel 8254 programmable interrupt timer which we use as
 * a somewhat primitive time-keeper. There are better ways of keeping
 * time and we will switch to them as time goes on
*/
#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <arch/timer.h>
#include <arch/8259a.h>
#include <arch/irq.h>
#include <arch/io.h>

static uint32_t ticks;

/* Delay loop  */
static inline void __delay(uint32_t loops)
{
	int d0;
	__asm__ __volatile__(
		"       jmp 1f	\n"
		".align 16	\n"
		"1:     jmp 2f	\n"
		".align 16	\n"
		"2:     decl %0	\n"
		"       jns 2b	\n"
		:"=&a" (d0)
		:"0" (loops));
}

/* millisecond delay */
void mdelay(uint32_t m)
{
	__delay(m *__this_cpu->loops_ms);
}

/* microsecond delay */
void udelay(uint32_t m)
{
	__delay( (m * __this_cpu->loops_ms) >> 10 );
}

void calibrate_delay_loop(void)
{
	uint32_t lb, lp=8;
	
	/* Coarse calibration */
	__this_cpu->loops_ms = (1<<12);
	while ( __this_cpu->loops_ms <<= 1 ) {
		set_timer_chan_oneshot(0xFFFF, 0);
		__delay(__this_cpu->loops_ms);
		if ( get_timer_chan(0,1) < 64000 )
			break;
	}

	__this_cpu->loops_ms >>= 1;
	lb = __this_cpu->loops_ms;

	/* Precision calculation */
	while (lp-- && (lb >>= 1) ) {
		__this_cpu->loops_ms |= lb;
		set_timer_chan_oneshot(0xFFFF, 0);
		__delay(__this_cpu->loops_ms);
		if ( get_timer_chan(0,1) < 64000 ) {
			__this_cpu->loops_ms &= ~lb;
		}

	}

	/* Normalise the results */
	__this_cpu->loops_ms *= PIT_SECOND / (65535 - 64000);
	__this_cpu->loops_ms /= 1000;
}

static void pit_isr(int irq)
{
	ticks++;
}

void pit_start_timer1(void)
{
	set_irq_handler(0, pit_isr);
	set_timer_chan(LATCH, 0);
	irq_on(0);
}

/* Generic PIT routines */
void set_timer_chan(uint16_t h, uint8_t chan)
{
	outb_p(TMR_PORT, (chan * 0x40) | PIT_BOTH | PIT_MODE_3);
	outb_p((0x40 + chan), (uint8_t)(h & 0xFF));
	outb_p((0x40 + chan), (uint8_t)(h >> 8));
}

void set_timer_chan_oneshot(uint16_t h, uint8_t chan)
{
	outb_p(TMR_PORT, (chan*0x40) | PIT_BOTH | PIT_MODE_0);
	outb_p((0x40 + chan), (uint8_t)(h & 0xFF));
	outb_p((0x40 + chan), (uint8_t)(h >> 8));
}

uint32_t get_timer_chan(uint8_t chan, int reset)
{
	uint32_t x;
	
	outb_p(TMR_PORT, (chan * 0x40) | (reset) ? 0x0 : PIT_LATCH);
	x = inb_p(0x40 + chan);
	x += (inb_p(0x40 + chan) << 8);
	return x;
}
