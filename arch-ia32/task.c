/* Will eventually contain 386 specific task swiching code I guess */
#include <kernel.h>
#include <task.h>
#include <mm.h>

void set_context(struct task *tsk)
{
	//BUG_ON(NULL == tsk->ctx);
	//BUG_ON(NULL == tsk);
	//printk("Context switch to %p\n", tsk->ctx->arch.pgd);
	load_pdbr(tsk->ctx->arch.pgd);
}

void idle_task_func(void)
{
	asm volatile(
		"1:\n"
		"rep; nop\n"
		"hlt;\n"
		"jmp 1b\n");
	for(;;)
		/* shutup gcc */;
}
