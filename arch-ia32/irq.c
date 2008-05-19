/*
 * Dispatch IRQs to the relevent handler
*/
#include <kernel.h>
#include <task.h>

#include <arch/8259a.h>
#include <arch/irq.h>

void irq_null(int irq)
{
	printk("unhandled IRQ %i\n", irq);
}

irqfn irq_fns[16]={
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null
};

void set_irq_handler(int irq, irqfn h)
{
	if ( irq>=0 && irq<16 ) irq_fns[irq]=h;
}

void irq_handler(int irq)
{
	irq_eoi(irq);
	sti();

	if ( irq>=0 && irq<16 ) irq_fns[irq](irq);	

	/* We may want to pre-empt this task,
	 * provided that is OK, of course */
	if ( __this_task->preempt==0 )
		sched();
}
