/* Will eventually contain 386 specific task swiching code I guess */
#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/task.h>

#include <arch/processor.h>
#include <arch/descriptor.h>
#include <arch/gdt.h>

#define load_tr(tr) asm volatile("ltr %w0"::"q" (tr));
#define store_tr(tr) asm volatile("str %w0":"=q" (tr));

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
	tsk->t.tss.esp -= sizeof(word);
	tsk->t.tss.sp0 -= sizeof(word);
	*(vaddr_t *)tsk->t.tss.esp = word;
}

void task_init_kthread(struct task *tsk,
			int (*thread_func)(void *),
			void *priv)
{
	struct ia32_tss *tss;

	tss = &tsk->t.tss;
	tss->ss = tss->ds = tss->es = tss->gs = tss->fs = __KERNEL_DS;
	tss->ss0 = tss->ss;
	tss->cs = __KERNEL_CS;
	tss->esp = (vaddr_t)tsk + PAGE_SIZE;
	tss->sp0 = tss->esp;
	tss->flags = (1 << 9);
	tss->cr3 = (vaddr_t)tsk->ctx->arch.pgd;
	tss->io_bitmap_base = 0x8000;

	/* push arguments to kthread init */
	if ( thread_func ) {
		tss->eip = (vaddr_t)kthread_init;
		task_push_word(tsk, (vaddr_t)priv);
		task_push_word(tsk, (vaddr_t)thread_func);
		task_push_word(tsk, 0x13371337); /* return address */
	}
}

void task_init_exec(struct task *tsk, vaddr_t ip, vaddr_t sp)
{
	unsigned short tr, scratch_tr;
	struct ia32_tss *tss;
	static struct ia32_tss scratch;

	cli();

	tss = &tsk->t.tss;

	store_tr(tr);
	tr >>= 3;
	BUG_ON(tr != TSS0 && tr != TSS1);
	scratch_tr = (tr == TSS0) ? TSS1 : TSS0;

	GDT[scratch_tr].desc = stnd_desc(((vaddr_t)(&scratch)),
			(sizeof(*tss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));
	load_tr(scratch_tr << 3);

	tss->ss = tss->ds = tss->es = tss->gs = tss->fs = __USER_DS | __CPL3;
	tss->cs = __USER_CS | __CPL3;
	tss->eip = ip;
	tss->esp = sp;
	tss->ss0 = __KERNEL_DS;
	tss->sp0 = (vaddr_t)((uint8_t *)__this_task + PAGE_SIZE);
	tss->cr3 = (vaddr_t)tsk->ctx->arch.pgd;
	tss->flags = (1 << 9);
	tss->io_bitmap_base = 0x8000;
	GDT[tr].desc = stnd_desc(((vaddr_t)(tss)), (sizeof(*tss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));

	if ( tr == TSS1 )
		asm volatile("ljmp %0, $0": : "i"(__TSS1));
	else
		asm volatile("ljmp %0, $0": : "i"(__TSS0));
	panic("danger will robinson\n");
}

void switch_task(struct task *prev, struct task *next)
{
	unsigned short tr;
	struct ia32_tss *tss;

	store_tr(tr);

	tr >>= 3;
	BUG_ON(tr != TSS0 && tr != TSS1);
	tr = (tr == TSS0) ? TSS1 : TSS0;

	tss = &next->t.tss;
	GDT[tr].desc = stnd_desc(((vaddr_t)(tss)), (sizeof(*tss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));

	if ( tr == TSS1 )
		asm volatile("ljmp %0, $0": : "i"(__TSS1));
	else
		asm volatile("ljmp %0, $0": : "i"(__TSS0));
}

__init void ia32_gdt_finalize(void)
{
	struct task *tsk = __this_task;
	struct ia32_tss *tss;

	tss = &tsk->t.tss;

	GDT[TSS0].desc = stnd_desc(((vaddr_t)(tss)), (sizeof(*tss) - 1),
			(D_TSS | D_BIG | D_BIG_LIM));
	load_tr(__TSS0);
}

int task_stack_overflowed(struct task *tsk)
{
	struct ia32_tss *tss = &tsk->t.tss;

	if ( (tss->cs & __CPL3) == __CPL3 )
		return 0;
	BUG_ON(tss->esp < PAGE_OFFSET);

	return (struct task *)tss->esp <= tsk + 2;
}

void idle_task_func(void)
{
	for(;;)
		asm volatile("rep; nop; hlt;");
}
