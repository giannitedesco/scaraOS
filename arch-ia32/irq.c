/*
 * Dispatch IRQs to the relevent handler
*/
#include <scaraOS/kernel.h>
#include <scaraOS/task.h>

#include <arch/8259a.h>
#include <arch/irq.h>
#include <arch/gdt.h>
#include <arch/regs.h>

static void irq_null(int irq, void *priv)
{
	printk("Spurious IRQ %i\n", irq);
}

static irqfn irq_fns[16] = {
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null,
	irq_null, irq_null, irq_null, irq_null
};
static void *irq_priv[16];

void set_irq_handler(int irq, irqfn h, void *priv)
{
	if ( irq >= 0 && irq < 16 ) {
		long flags;
		lock_irq(flags);
		irq_fns[irq] = h;
		irq_priv[irq] = priv;
		unlock_irq(flags);
	}
}

_asmlinkage unsigned irq_handler(uint32_t irq, volatile struct intr_ctx ctx)
{
	if ( irq >= 0 && irq < 16 )
		irq_fns[irq](irq, irq_priv[irq]);

	irq_eoi(irq);

	sched();
	return return_from_intr(&ctx);
}
