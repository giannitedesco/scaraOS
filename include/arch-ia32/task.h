#ifndef __ARCH_IA32_TASK_INCLUDED__
#define __ARCH_IA32_TASK_INCLUDED__

#ifndef __ASM__
struct ia32_tss {
	unsigned short		back_link, __blh;
	unsigned long		sp0;
	unsigned short		ss0, __ss0h;
	unsigned long		sp1;
	/* ss1 caches MSR_IA32_SYSENTER_CS: */
	unsigned short		ss1, __ss1h;
	unsigned long		sp2;
	unsigned short		ss2, __ss2h;
	unsigned long		cr3;
	unsigned long		eip;
	unsigned long		flags;
	unsigned long		eax;
	unsigned long		ecx;
	unsigned long		edx;
	unsigned long		ebx;
	unsigned long		esp;
	unsigned long		ebp;
	unsigned long		esi;
	unsigned long		edi;
	unsigned short		es, __esh;
	unsigned short		cs, __csh;
	unsigned short		ss, __ssh;
	unsigned short		ds, __dsh;
	unsigned short		fs, __fsh;
	unsigned short		gs, __gsh;
	unsigned short		ldt, __ldth;
	unsigned short		trace;
	unsigned short		io_bitmap_base;
} __attribute__((packed));

/* per-thread state */
struct thread {
	struct intr_ctx *regs;
	struct ia32_tss tss;
};

void switch_task(struct task *prev, struct task *next);

/* Get the current task (stored at the top of the stack) */
#define __this_task (this_task())
static inline struct task *this_task(void)
{
	struct task *ret;
	asm volatile("andl %%esp,%0":"=r"(ret) : "0" (~4095UL));
	return ret;
}

void task_init_kthread(struct task *tsk, 
			int (*thread_func)(void *),
			void *priv);
int task_stack_overflowed(struct task *tsk);
void task_init_exec(struct task *tsk, vaddr_t ip, vaddr_t sp);
void set_context(struct task *tsk);

_noreturn void idle_task_func(void);
#endif

#endif /* __ARCH_IA32_TASK_INCLUDED__ */
