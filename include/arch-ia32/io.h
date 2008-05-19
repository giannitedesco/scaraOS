#ifndef __ARCH_IA32_IO__
#define __ARCH_IA32_IO__

#ifndef __ASM__

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

/* INPUT: Non-pausing */
uint8_t inb (uint32_t port);
uint16_t inw (uint32_t port);
uint32_t inl (uint32_t port);

/* INPUT: Pausing */
uint8_t inb_p (uint32_t port);

/* OUTPUT: Non-pausing */
void outb(uint32_t port, uint8_t val);
void outw(uint32_t port, uint16_t val);
void outl(uint32_t port, uint32_t val);

/* OUTPUT: Pausing */
void outb_p (uint32_t port, uint8_t val);
void outw_p (uint32_t port, uint16_t val);
#endif

#endif /* __ARCH_IA32_IO__ */
