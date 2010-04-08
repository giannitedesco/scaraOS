/*
 * Dispatch IRQs to the relevent handler
*/
#include <scaraOS/kernel.h>
#include <scaraOS/task.h>

#include <arch/8259a.h>
#include <arch/irq.h>
#include <arch/gdt.h>
#include <arch/regs.h>

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

unsigned irq_handler(uint32_t irq, volatile struct intr_ctx ctx)
{
	if ( irq >= 0 && irq < 16 )
		irq_fns[irq](irq);

	irq_eoi(irq);

	sched();
	return return_from_intr(&ctx);
}
