#ifndef __ARCH_IA32_TASK_INCLUDED__
#define __ARCH_IA32_TASK_INCLUDED__

#ifndef __ASM__
/* per-thread state */
struct thread {
	vaddr_t	esp;
	vaddr_t	eip;
	struct intr_ctx *regs;
};

/* Actual task switching macro - can't be a function heh */
#define switch_task(prev,next) do {			\
	asm volatile( 					\
	"pusha\n" 					\
	"pushfl\n"					\
	"movl %%esp,%0\n"	/* save ESP */		\
	"movl %2,%%esp\n"	/* restore ESP */	\
	"movl $1f,%1\n"		/* save EIP */		\
	"jmp *%3\n"		/* restore EIP */	\
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

void ia32_gdt_finalize(void);

void task_init_kthread(struct task *tsk, 
			int (*thread_func)(void *),
			void *priv);
int task_stack_overflowed(struct task *tsk);
void task_init_exec(struct task *tsk, vaddr_t ip, vaddr_t sp);
void set_context(struct task *tsk);

_noreturn void idle_task_func(void);

#endif /* __ARCH_IA32_TASK_INCLUDED__ */
