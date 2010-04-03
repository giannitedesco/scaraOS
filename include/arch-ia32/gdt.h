#ifndef __ARCH_IA32_GDT__
#define __ARCH_IA32_GDT__

#define KERNEL_CS	1
#define KERNEL_DS	2
#define USER_CS		3
#define USER_DS		4
#define KERNEL_TSS	5
#define USER_TSS	6

/* Segment selectors (see GDT in arch-ia32/setup.c) */
#define __KERNEL_CS	(KERNEL_CS << 3)
#define __KERNEL_DS	(KERNEL_DS << 3)
#define __USER_CS	(USER_CS << 3)
#define __USER_DS	(USER_DS << 3)
#define __KERNEL_TSS	(KERNEL_TSS << 3)
#define __USER_TSS	(USER_TSS << 3)

#define __CPL0		0x0
#define __CPL1		0x1
#define __CPL2		0x2
#define __CPL3		0x3

#ifndef __ASM__
void ia32_gdt_finalize(void);
#endif

#endif /* __ARCH_IA32_GDT__ */
