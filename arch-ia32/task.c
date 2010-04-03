/* Will eventually contain 386 specific task swiching code I guess */
#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/task.h>

#include <arch/processor.h>
#include <arch/descriptor.h>
#include <arch/gdt.h>
#include <arch/regs.h>

volatile static struct ia32_tss tss;
volatile static struct ia32_tss ktss;

/* Global descriptor table */
volatile static dt_entry_t __desc_aligned GDT[] = {
	{.dummy = 0},

	/* Kernel space */
	{.desc = stnd_desc(0, 0xFFFFF,
		(D_CODE | D_READ  | D_BIG | D_BIG_LIM)) },
	{.desc = stnd_desc(0, 0xFFFFF,
		(D_DATA | D_WRITE | D_BIG | D_BIG_LIM)) },

	/* User space */
	{.desc = stnd_desc(0, 0xFFFFF,
		(D_CODE | D_READ  | D_BIG | D_BIG_LIM | D_DPL3)) },
	{.desc = stnd_desc(0, 0xFFFFF,
		(D_DATA | D_WRITE | D_BIG | D_BIG_LIM | D_DPL3)) },

	{.dummy = 0 },
	{.dummy = 0 },
};
const struct gdtr loadgdt = { sizeof(GDT) - 1, (uint32_t)GDT};

static void task_push_word(struct task *tsk, vaddr_t word)
{
	tsk->t.esp -= sizeof(word);
	*(vaddr_t *)tsk->t.esp = word;
}

void task_init_kthread(struct task *tsk,
			int (*thread_func)(void *),
			void *priv)
{
	tsk->t.eip = (vaddr_t)kthread_init;
	tsk->t.esp = (vaddr_t)tsk + PAGE_SIZE;
	tsk->t.regs = NULL;

	/* push arguments to kthread init */
	task_push_word(tsk, (vaddr_t)priv);
	task_push_word(tsk, (vaddr_t)thread_func);
	task_push_word(tsk, 0x13371337); /* return address */
}

void task_init_exec(struct task *tsk, vaddr_t ip, vaddr_t sp)
{
	BUG_ON(tsk->t.regs == NULL);

	//tsk->t.regs->eip = ip;
	tss.ss = tss.ds = tss.es = tss.gs = tss.fs = __USER_DS | __CPL3;
	tss.cs = __USER_CS | __CPL3;
	tss.eip = ip;
	tss.esp = sp;
	tss.ss0 = __KERNEL_DS;
	tss.sp0 = (vaddr_t)((uint8_t *)__this_task + PAGE_SIZE);
	tss.cr3 = (vaddr_t)tsk->ctx->arch.pgd;

	/* make sure userspace always executed with IRQs enabled */
	tss.flags = get_eflags() | (1 << 9);
	GDT[USER_TSS].desc = stnd_desc(((vaddr_t)(&tss)), (sizeof(tss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));
	asm volatile("ljmp %0, $0": : "i"(__USER_TSS));
}

#define load_tr(tr) asm volatile("ltr %w0"::"q" (tr));
void ia32_gdt_finalize(void)
{
	GDT[KERNEL_TSS].desc = stnd_desc(((vaddr_t)(&ktss)), (sizeof(ktss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));
	load_tr(__KERNEL_TSS);
}

int task_stack_overflowed(struct task *tsk)
{
	return (struct task *)tsk->t.esp <= tsk + 2;
}

void idle_task_func(void)
{
	for(;;)
		asm volatile("rep; nop\n""hlt;\n");
}
