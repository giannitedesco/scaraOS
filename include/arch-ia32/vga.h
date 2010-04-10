#ifndef __ARCH_IA32_VGA__
#define __ARCH_IA32_VGA__

#include <scaraOS/mm.h>

#define COLS		80
#define ROWS		25

void vga_preinit(void);

void vga_init(void);
void vga_put(uint8_t);
void vga_curs(int x, int y);

#endif /* __ARCH_IA32_VGA__ */
