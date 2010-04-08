#ifndef __ARCH_IA32_REGS__
#define __ARCH_IA32_REGS__

#ifndef __ASM__

struct intr_ctx {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t err_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};

_asmlinkage unsigned irq_handler(uint32_t irq, volatile struct intr_ctx ctx);
_asmlinkage unsigned exc_handler(uint32_t exc_num, volatile struct intr_ctx ctx);
_asmlinkage unsigned syscall_exc(volatile struct intr_ctx ctx);
_asmlinkage _noreturn void panic_exc(volatile struct intr_ctx ctx);
void ctx_dump(struct intr_ctx *ctx);

static inline unsigned return_from_intr(volatile struct intr_ctx *ctx)
{
	uint32_t rpl;

	rpl = ctx->cs & __CPL3;
	if ( 0 == rpl )
		return __KERNEL_DS;

	BUG_ON(rpl != __CPL3);
	return __USER_DS | rpl;
}

#endif

#endif /* __ARCH_IA32_REGS__ */
