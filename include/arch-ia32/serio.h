#ifndef __ARCH_IA32_SERIO__
#define __ARCH_IA32_SERIO__

#include <scaraOS/mm.h>

void serio_init(void);
void serio_put(uint8_t c);

#endif /* __ARCH_IA32_SERIO__ */
