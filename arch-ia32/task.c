/* Will eventually contain 386 specific task swiching code I guess */
#include <kernel.h>
#include <arch/regs.h>
#include <task.h>
#include <mm.h>

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

void task_init_exec(struct task *tsk, vaddr_t ip)
{
	BUG_ON(tsk->t.regs == NULL);
	tsk->t.regs->eip = ip;
}

int task_stack_overflowed(struct task *tsk)
{
	return (struct task *)tsk->t.esp <= tsk + 2;
}

void set_context(struct task *tsk)
{
	BUG_ON(NULL == tsk->ctx);
	BUG_ON(NULL == tsk);
	dprintk("Context switch to %p\n", tsk->ctx->arch.pgd);
	load_pdbr(tsk->ctx->arch.pgd);
}

void idle_task_func(void)
{
	for(;;)
		asm volatile("rep; nop\n""hlt;\n");
}
