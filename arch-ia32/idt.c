/*
 * This code handles the interrupt descriptor tables.
*/
#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/mm.h>

#include <arch/processor.h>
#include <arch/descriptor.h>
#include <arch/kimage.h>
#include <arch/irq.h>
#include <arch/idt.h>
#include <arch/gdt.h>
#include <arch/regs.h>

#define load_idt(idtr) \
	asm volatile("lidt (%0)": :"r" (idtr));

/* IDT structure */
static dt_entry_t __desc_aligned IDT[256];
static struct gdtr loadidt = { sizeof(IDT) - 1, (uint32_t)IDT};

/* Add a new trap vector to the IDT  */
void idt_exception(void *handler, uint8_t intr)
{
	long flags;
	lock_irq(flags);
	IDT[intr].gate.selector		= __KERNEL_CS;
	IDT[intr].gate.offset_low	= (uint32_t)handler & 0xffff;
	IDT[intr].gate.offset_high 	= (uint32_t)handler >> 16;
	IDT[intr].gate.access		= D_PRESENT | D_TRAP;
	unlock_irq(flags);
}

/* Add a new interrupt vector to the IDT  */
void idt_interrupt(void *handler, uint8_t intr)
{
	long flags;
	lock_irq(flags);
	IDT[intr].gate.selector		= __KERNEL_CS;
	IDT[intr].gate.offset_low	= (uint32_t)handler & 0xffff;
	IDT[intr].gate.offset_high 	= (uint32_t)handler >> 16;
	IDT[intr].gate.access		= D_PRESENT | D_INT;
	unlock_irq(flags);
}

/* Add a new interrupt vector to the IDT  */
void idt_user_interrupt(void *handler, uint8_t intr)
{
	long flags;
	lock_irq(flags);
	IDT[intr].gate.selector		= __KERNEL_CS;
	IDT[intr].gate.offset_low	= (uint32_t)handler & 0xffff;
	IDT[intr].gate.offset_high 	= (uint32_t)handler >> 16;
	IDT[intr].gate.access		= D_PRESENT | D_TRAP | D_DPL3;
	unlock_irq(flags);
}

static void stack_dump(struct intr_ctx *ctx)
{
	uint32_t *stop, *sbase;
	int i, max;

	stop = (uint32_t *)((uint8_t *)ctx + sizeof(*ctx));
	sbase = (uint32_t *)((uint8_t *)__this_task + PAGE_SIZE);
	max = (sbase - stop > 32) ? 32 : sbase - stop;
	for(i = 0; i < max; i += 4 ) {
		printk(" 0x%.8lx", stop[i]);
		if ( i + 1 < (sbase - stop) )
			printk(" 0x%.8lx", stop[i + 1]);
		if ( i + 2 < (sbase - stop) )
			printk(" 0x%.8lx", stop[i + 2]);
		if ( i + 3 < (sbase - stop) )
			printk(" 0x%.8lx", stop[i + 3]);
		printk("\n");
	}
}

void ctx_dump(struct intr_ctx *ctx)
{
	struct task *tsk = __this_task;
	printk("Task pid=%lu name=%s: CS=0x%lx tsk=%p\n",
			tsk->pid, tsk->name, ctx->cs, tsk);
	printk("  EIP=0x%.8lx EFLAGS=0x%.8lx\n", ctx->eip, ctx->eflags);
	printk("  EAX=0x%.8lx    EBX=0x%.8lx\n", ctx->eax, ctx->ebx);
	printk("  ECX=0x%.8lx    EDX=0x%.8lx\n", ctx->ecx, ctx->edx);
	printk("  ESP=0x%.8lx    EBP=0x%.8lx\n", ctx->esp, ctx->ebp);
	printk("  ESI=0x%.8lx    EDI=0x%.8lx\n", ctx->edi, ctx->esi);
	if ( ctx->esp >= PAGE_OFFSET && 0 == (ctx->cs & __CPL3) )
		stack_dump(ctx);
}

static void page_fault(struct intr_ctx *ctx)
{
	unsigned prot;

	if ( (ctx->err_code & PAGEFAULT_WRITE) )
		prot = PROT_WRITE;
	else
		prot = PROT_READ;

	if ( 0 == (ctx->err_code & (PAGEFAULT_PROTECTION)) ) {
		if ( !mm_pagefault(__this_task, pf_address(), prot) )
			return;
	}else if ( 0 == (ctx->err_code & PAGEFAULT_PROTECTION) ) {
		/* copy on write? */
	}

	if ( (ctx->err_code & PAGEFAULT_USER) ) {
		printk("%s: User mode pagefault %sing 0x%.8lx ip 0x%.8lx\n",
			__this_task->name,
			(ctx->err_code & PAGEFAULT_WRITE) ? "read" : "writ",
			pf_address(), ctx->eip);
		_sys_exit(~0);
	}else{
		struct pagefault_fixup *fix;
		for(fix = &__rodata_pagefault;
				fix < &__rodata_pagefault_end; fix++) {
			if ( fix->fault_addr == ctx->eip ) {
				ctx->eip = fix->fixup_addr;
				return;
			}
		}
	}

	cli();
	printk("Unhandled #PF in %s mode: %s fault_addr=0x%.8lx/%s%s\n",
		(ctx->err_code & PAGEFAULT_USER) ?
			"user" : "supervisor",
		(ctx->err_code & PAGEFAULT_PROTECTION) ?
			"PROTECTION_VIOLATION" : "NONPRESENT",
		pf_address(),
		(ctx->err_code & PAGEFAULT_WRITE) ?
			"WRITE" : "READ",
		(ctx->err_code & PAGEFAULT_RESERVED_BIT) ?
			" RESERVED_BIT" : "");
	ctx_dump(ctx);
	idle_task_func();
}

#define EXC_TYPE_FAULT		1
#define EXC_TYPE_TRAP		2
#define EXC_TYPE_ABORT		3
static const struct {
	uint16_t type;
	uint16_t err_code;
	char * const name;
	void (*handler)(struct intr_ctx *ctx);
}exc[]={
	{EXC_TYPE_FAULT, 0, "Divide Error"},
	{EXC_TYPE_TRAP /* or fault */, 0, "Debug"},
	{EXC_TYPE_TRAP, 0, "NMI"},
	{EXC_TYPE_TRAP, 0, "Breakpoint"},
	{EXC_TYPE_TRAP, 0, "Overflow"},

	{EXC_TYPE_FAULT, 0, "Bounds Check"},
	{EXC_TYPE_FAULT, 0, "Invalid Opcode"},
	{EXC_TYPE_FAULT, 0, "Coprocessor not available"},
	{EXC_TYPE_ABORT, 1, "Double fault"},
	{EXC_TYPE_FAULT, 0, "Coprocessor segment overrun"},

	{EXC_TYPE_FAULT, 1, "Invalid TSS"},
	{EXC_TYPE_FAULT, 1, "Segment not present"},
	{EXC_TYPE_FAULT, 1, "Stack segment"},
	{EXC_TYPE_FAULT, 1, "General Protection"},
	{EXC_TYPE_FAULT, 1, "Page Fault",
			.handler = page_fault},

	{0, 0, NULL},
	{EXC_TYPE_FAULT, 0, "FPU error"},
	{EXC_TYPE_FAULT, 1, "Alignment Check"},
	{EXC_TYPE_ABORT, 0, "Machine Check"},
	{EXC_TYPE_FAULT, 0, "SIMD Exception"},
};

unsigned exc_handler(uint32_t exc_num, volatile struct intr_ctx ctx)
{
	static const char * const tname[] = {
		"UNKNOWN",
		"fault",
		"trap",
		"abort condition",
	};

	if ( exc[exc_num].handler ) {
		(*exc[exc_num].handler)((struct intr_ctx *)&ctx);
		return return_from_intr(&ctx);
	}

	if ( (ctx.cs & __CPL3) == __CPL3 ) {
		printk("%s: User mode %s %s\n",
			__this_task->name,
			exc[exc_num].name,
			tname[exc[exc_num].type]);
		_sys_exit(~0);
	}

	cli();
	printk("Unhandled exception in kernel mode, halting\n");
	printk("%s: %s @ 0x%.8lx",
		tname[exc[exc_num].type], exc[exc_num].name, ctx.eip);
	if ( exc[exc_num].err_code ) {
		if ( ctx.err_code & 0x1 )
			printk(" EXT");
		switch ( ctx.err_code & 0x6 ) {
		case 0:
			printk(" GDT");
			break;
		case 2:
			printk(" IDT");
			break;
		case 4:
			printk(" LDT");
			break;
		}
		printk(" selector=0x%.lx", ctx.err_code & 0xfff8);
	}
	printk("\n");
	ctx_dump((struct intr_ctx *)&ctx);
	idle_task_func();
}

void panic_exc(volatile struct intr_ctx ctx)
{
	cli();
	ctx_dump((struct intr_ctx *)&ctx);
	stack_dump((struct intr_ctx *)&ctx);
	idle_task_func();
}

void panic(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	printk("Panic: ");
	printkv(fmt, va);
	va_end(va);

	asm volatile("int $0xf0");
	idle_task_func();
}

void __init idt_init(void)
{
	int count;

	for(count = 0; count < 256; count++)
		idt_exception(int_null, count);

	/* Standard fault/trap handlers */
	idt_exception(_exc0, 0);
	idt_exception(_exc1, 1);
	idt_exception(_exc2, 2);
	idt_exception(_exc3, 3);
	idt_exception(_exc4, 4);
	idt_exception(_exc5, 5);
	idt_exception(_exc6, 6);
	idt_exception(_exc7, 7);
	idt_exception(_exc8, 8);
	idt_exception(_exc9, 9);
	idt_exception(_exc10, 10);
	idt_exception(_exc11, 11);
	idt_exception(_exc12, 12);
	idt_exception(_exc13, 13);
	idt_exception(_exc14, 14);
	/* exception 15 is resrved */
	idt_exception(_exc16, 16);
	idt_exception(_exc17, 17);
	idt_exception(_exc18, 18);
	idt_exception(_exc19, 19);

	/* 20 -> 32 are reserved */

	/* Interrupts caused by externally
	 * generated IRQ lines */
	idt_interrupt(_irq0, 0x20);
	idt_interrupt(_irq1, 0x21);
	idt_interrupt(_irq2, 0x22);
	idt_interrupt(_irq3, 0x23);
	idt_interrupt(_irq4, 0x24);
	idt_interrupt(_irq5, 0x25);
	idt_interrupt(_irq6, 0x26);
	idt_interrupt(_irq7, 0x27);
	idt_interrupt(_irq8, 0x28);
	idt_interrupt(_irq9, 0x29);
	idt_interrupt(_irq10, 0x30);
	idt_interrupt(_irq11, 0x3a);
	idt_interrupt(_irq12, 0x3b);
	idt_interrupt(_irq13, 0x3c);
	idt_interrupt(_irq14, 0x3d);
	idt_interrupt(_irq15, 0x3e);

	/* System call */
	idt_interrupt(_panic, 0xf0);
	idt_user_interrupt(_syscall, 0xff);

	/* Yay - we're finished */
	load_idt(&loadidt);
}
