#include <scaraOS/kernel.h>
#include <arch/regs.h>
#include <arch/syscalls.h>
#include <scaraOS/task.h>

#include <scaraOS/exec.h>

struct syscall_desc {
	unsigned int type;
	union {
		sys0_t arg0;
		sys1_t arg1;
		sys2_t arg2;
		sys3_t arg3;
	}u;
};

_noreturn void _sys_exit(uint32_t code);
int _sys_fork(uint32_t flags);

static const struct syscall_desc syscall_tbl[_SYS_NR_SYSCALLS] = {
	[_SYS_exit] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_exit },
	[_SYS_fork] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_fork },
	[_SYS_exec] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_exec },
};

void syscall_exc(volatile struct intr_ctx ctx)
{
	const struct syscall_desc *sys;
	struct task *tsk = __this_task;
	uint32_t ret;

	if ( ctx.eax >= _SYS_NR_SYSCALLS ) {
		printk("Unknown syscall %lu\n", ctx.eax);
		return;
	}

	tsk->t.regs = (struct intr_ctx *)&ctx;

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
	default:
		ret = -1;
	}

	ctx.eax = ret;
	tsk->t.regs = NULL;
}
