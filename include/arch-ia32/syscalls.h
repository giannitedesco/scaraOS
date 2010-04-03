#ifndef __ARCH_IA32_SYSCALLS_INCLUDED__
#define __ARCH_IA32_SYSCALLS_INCLUDED__

#include <arch/syscall-numbers.h>

#define _SYS_ARG0		0
#define _SYS_ARG1		1
#define _SYS_ARG2		2
#define _SYS_ARG3		3
#define _SYS_ARG4		4
#define _SYS_ARG5		5

#ifndef __ASM__
typedef uint32_t (*sys0_t)(void);
typedef uint32_t (*sys1_t)(uint32_t);
typedef uint32_t (*sys2_t)(uint32_t, uint32_t);
typedef uint32_t (*sys3_t)(uint32_t, uint32_t, uint32_t);
typedef uint32_t (*sys4_t)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*sys5_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

#endif /* __ASM__ */

#endif /* __ARCH_IA32_SYSCALLS_INCLUDED__ */
