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
		sys4_t arg4;
		sys5_t arg5;
	}u;
};

_noreturn void _sys_exit(uint32_t code);
int _sys_fork(uint32_t flags);

static int _sys_open(const char *fn, unsigned int flags)
{
	printk("sys_open: unimplemented\n");
	return -1;
}

static int _sys_close(int fd)
{
	printk("sys_close: fd=%i unimplemented\n", fd);
	return -1;
}

static int _sys_read(int fd, void *buf, size_t count)
{
	printk("sys_read: fd=%i len=%lu unimplemented\n", fd, count);
	return -1;
}

static int _sys_write(int fd, void *buf, size_t count)
{
	if ( fd == 1 ) {
		char *kbuf;
		int sz;

		kbuf = kmalloc(count);
		if ( NULL == kbuf )
			return -1;

		sz = copy_from_user(kbuf, buf, count);
		if ( sz < 0 )
			return -1;

		printk("%.*s", sz, kbuf);
		kfree(kbuf);

		return count;
	}

	printk("sys_write: fd=%i len=%lu unimplemented\n", fd, count);
	return -1;
}

static const struct syscall_desc syscall_tbl[_SYS_NR_SYSCALLS] = {
	[_SYS_exit] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_exit },
	[_SYS_fork] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_fork },
	[_SYS_exec] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_exec },
	[_SYS_open] {.type = _SYS_ARG2, .u.arg2 = (sys2_t)_sys_open },
	[_SYS_close] {.type = _SYS_ARG1, .u.arg1 = (sys1_t)_sys_close },
	[_SYS_read] {.type = _SYS_ARG3, .u.arg3 = (sys3_t)_sys_read },
	[_SYS_write] {.type = _SYS_ARG3, .u.arg3 = (sys3_t)_sys_write },
};

void syscall_exc(volatile struct intr_ctx ctx)
{
	const struct syscall_desc *sys;
	uint32_t ret;

	if ( ctx.eax >= _SYS_NR_SYSCALLS ) {
		printk("Unknown syscall %lu\n", ctx.eax);
		return;
	}

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
	case _SYS_ARG4:
		ret = sys->u.arg4(ctx.ebx, ctx.ecx, ctx.edx, ctx.esi);
		break;
	case _SYS_ARG5:
		ret = sys->u.arg5(ctx.ebx, ctx.ecx, ctx.edx, ctx.esi, ctx.edi);
		break;
	default:
		ret = -1;
	}

	ctx.eax = ret;
}
