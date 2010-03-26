#ifndef __ARCH_IA32_KIMAGE__
#define __ARCH_IA32_KIMAGE__

#ifndef __ASM__

extern uint8_t __begin;

extern uint8_t __text;
extern uint8_t __text_end;

extern uint8_t __rodata;
extern uint8_t __rodata_end;

extern uint8_t __data;
extern uint8_t __data_end;

extern uint8_t __bss;
extern uint8_t __bss_end;

extern uint8_t __kernel_end;

extern uint8_t __init_start;

extern initcall_t __initcall_start;
extern initcall_t __initcall_end;

extern uint8_t __init_end;

extern uint8_t __end;

#endif /* __ASM__ */

#endif /* __ARCH_IA32_KIMAGE__ */
