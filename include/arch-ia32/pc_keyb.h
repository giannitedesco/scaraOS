#ifndef __ARCH_IA32_PC_KEYB__
#define __ARCH_IA32_PC_KEYB__

/* Register I/O ports */
#define KBR_STATUS	0x64 /* read */
#define KBR_CNTL	0x64 /* write */
#define KBR_DATA	0x60 /* read/write */

/* Status register bits */
#define KB_STAT_OBF 		0x01	/* Keyboard output buffer full */
#define KB_STAT_IBF 		0x02	/* Keyboard input buffer full */
#define KB_STAT_SELFTEST	0x04	/* Self test successful */
#define KB_STAT_CMD		0x08	/* Last write was a command write (0=data) */
#define KB_STAT_UNLOCKED	0x10	/* Zero if keyboard locked */
#define KB_STAT_MOUSE_OBF	0x20	/* Mouse output buffer full */
#define KB_STAT_GTO 		0x40	/* General receive/xmit timeout */
#define KB_STAT_PERR 		0x80	/* Parity error */

/* Commands */
#define KB_CMD_SET_LEDS		0xED	/* Set keyboard leds */
#define KB_CMD_SET_RATE		0xF3	/* Set typematic rate */
#define KB_CMD_ENABLE		0xF4	/* Enable scanning */
#define KB_CMD_DISABLE		0xF5	/* Disable scanning */
#define KB_CMD_RESET		0xFF	/* Reset */

#endif /* __ARCH_IA32_PC_KEYB__ */
