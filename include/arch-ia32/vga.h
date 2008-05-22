#ifndef __ARCH_IA32_VGA__
#define __ARCH_IA32_VGA__

#include <arch/mm.h>

#define COLS		80
#define ROWS		25
#define VIDMEM		(0xB8000+PAGE_OFFSET)

void vga_preinit(void);

void vga_init(void);
void vga_put(uint8_t);
void vga_curs (uint16_t, uint16_t);

#define MONITOR_NONE    0
#define MONITOR_COLOUR  1
#define MONITOR_MONO    2
#define MONITOR_UNKNOWN 3

#endif /* __ARCH_IA32_VGA__ */
