#ifndef __ARCH_IA32_TASK_INCLUDED__
#define __ARCH_IA32_TASK_INCLUDED__

#ifndef __ASM__
struct thread {
	long esp,eip;
};

/* Actual task switching macro - can't be a function heh */
#define switch_task(prev,next) do {			\
	asm volatile( 					\
	"pusha\n" 					\
	"pushfl\n"					\
	"movl %%esp,%0\n"	/* save ESP */		\
	"movl %2,%%esp\n"	/* restore ESP */	\
	"movl $1f,%1\n"		/* save EIP */		\
	"pushl %3\n"		/* restore EIP */	\
	"ret\n"						\
	"1:\n"						\
	"popfl\n"					\
	"popa\n"					\
	:"=m" (prev->t.esp),"=m" (prev->t.eip)		\
	:"m" (next->t.esp),"m" (next->t.eip)		\
);} while (0)

/* Get the current task (stored at the top of the stack) */
#define __this_task (this_task())
static inline struct task *this_task(void)
{
	struct task *ret;
	asm volatile("andl %%esp,%0":"=r"(ret) : "0" (~4095UL));
	return ret;
}
#endif

_noreturn void idle_task_func(void);

#endif /* __ARCH_IA32_TASK_INCLUDED__ */
