#include <kernel.h>
#include <arch/regs.h>
#include <arch/syscalls.h>

struct syscall_desc {
	unsigned int type;
	union {
		sys0_t arg0;
		sys1_t arg1;
		sys2_t arg2;
		sys3_t arg3;
		sysreg_t argreg;
	}u;
};

static const struct syscall_desc syscall_tbl[_SYS_NR_SYSCALLS] = {
	[_SYS_exit] {.type = _SYS_ARG1, .u.arg1 = syscall_exit },
	[_SYS_fork] {.type = _SYS_ARG1, .u.arg1 = syscall_fork },
	[_SYS_exec] {.type = _SYS_ARG1, .u.arg1 = syscall_exec },
};

void syscall_exc(struct intr_ctx ctx)
{
	const struct syscall_desc *sys;
	uint32_t ret;

	if ( ctx.eax >= _SYS_NR_SYSCALLS )
		printk("Unknown syscall %lu\n", ctx.eax);

	sys = &syscall_tbl[ctx.eax];
	switch( sys->type ) {
	case _SYS_ARG0:
		ret = sys->u.arg0();
		break;
	case _SYS_ARG1:
		ret = sys->u.arg1(ctx.ebx);
		break;
	case _SYS_ARG2:
		ret = sys->u.arg2(ctx.ebx, ctx.ecx);
		break;
	case _SYS_ARG3:
		ret = sys->u.arg3(ctx.ebx, ctx.ecx, ctx.edx);
		break;
	case _SYS_REGS:
		ret = sys->u.argreg(&ctx);
		break;
	}

	ctx.eax = ret;
}

