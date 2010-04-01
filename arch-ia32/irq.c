/*
 * Dispatch IRQs to the relevent handler
*/
#include <kernel.h>
#include <task.h>

#include <arch/8259a.h>
#include <arch/irq.h>

static void irq_null(int irq)
{
	printk("Spurious IRQ %i\n", irq);
}

static irqfn irq_fns[16]={
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null
};

void set_irq_handler(int irq, irqfn h)
{
	if ( irq >= 0 && irq < 16 )
		irq_fns[irq] = h;
}

void irq_handler(int irq)
{
	if ( irq >= 0 && irq < 16 )
		irq_fns[irq](irq);

	irq_eoi(irq);

	sched();
}
