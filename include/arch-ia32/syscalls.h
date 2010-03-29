#ifndef __ARCH_IA32_SYSCALLS_INCLUDED__
#define __ARCH_IA32_SYSCALLS_INCLUDED__

#ifndef __ASM__
#endif /* __ASM__ */

#define _SYS_exit		0
#define _SYS_exec		1
#define _SYS_fork		2
#define _SYS_NR_SYSCALLS	3

#define _SYS_ARG0		0
#define _SYS_ARG1		1
#define _SYS_ARG2		2
#define _SYS_ARG3		3
#define _SYS_REGS		4

typedef uint32_t (*sys0_t)(void);
typedef uint32_t (*sys1_t)(uint32_t);
typedef uint32_t (*sys2_t)(uint32_t, uint32_t);
typedef uint32_t (*sys3_t)(uint32_t, uint32_t, uint32_t);
typedef uint32_t (*sysreg_t)(struct intr_ctx *ctx);

uint32_t syscall_exit(uint32_t);
uint32_t syscall_fork(uint32_t);
uint32_t syscall_exec(uint32_t);

#endif /* __ARCH_IA32_SYSCALLS_INCLUDED__ */
