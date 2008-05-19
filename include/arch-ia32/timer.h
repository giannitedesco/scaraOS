#ifndef __ARCH_IA32_TIMER__
#define __ARCH_IA32_TIMER__

void calibrate_delay_loop(void);
void pit_start_timer1(void);

/* Routines for using the PCs 8254 programmable interrupt timer */
void set_timer_chan(uint16_t, uint8_t);
void set_timer_chan_oneshot(uint16_t, uint8_t);
uint32_t get_timer_chan(uint8_t, int);

#define PIT_SECOND	1193180
#define HZ		100
#define LATCH		((PIT_SECOND + HZ/2) / HZ)

/* I/O port for 8254 commands */
#define TMR_PORT	0x43

/* I/O for individual counters */
#define COUNTER_0	0x40
#define COUNTER_1	0x41
#define COUNTER_2	0x42

/* Channel selection */
#define CHANNEL_0	0x00
#define CHANNEL_1	0x40
#define CHANNEL_2	0x80

/* Which bytes are set */
#define PIT_LOW		0x10
#define PIT_HIGH	0x20
#define PIT_BOTH	0x30

/* Modes */
#define PIT_MODE_0	0x0	/* One shot */
#define PIT_MODE_1	0x2	/* No worky */
#define PIT_MODE_2	0x4	/* forever */
#define PIT_MODE_3	0x6	/* forever */
#define PIT_MODE_4	0x8	/* No worky */
#define PIT_MODE_5	0xA	/* No worky */

#define PIT_LATCH	0x00
#define PIT_BCD		0x01
#define PIT_CH0		0x02
#define PIT_CH1		0x04
#define PIT_CH2		0x08
#define PIT_STAT	0x10
#define PIT_CNT		0x20
#define PIT_READ	0xF0

#endif /* __ARCH_IA32_TIMER__ */
