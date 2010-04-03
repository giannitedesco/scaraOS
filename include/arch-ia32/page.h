#ifndef __ARCH_IA32_PAGE__
#define __ARCH_IA32_PAGE__

/* Characteristics of the IA32 architecture */
#define PAGE_SHIFT	12

/* Load the kernel in at 3GB */
#define PAGE_OFFSET	0xc0000000

#define MAP_INVALID	0xFFFFFFFF

/* C Specific */
#ifndef __ASM__
/* per memory context state */
struct arch_ctx {
	paddr_t pgd;
};

#define __pa(x)		((unsigned long)(x)-PAGE_OFFSET)
#define __va(x)		((void *)((unsigned long)(x)+PAGE_OFFSET))

#endif /* __ASM__ */
#endif /* __ARCH_IA32_PAGE__ */
