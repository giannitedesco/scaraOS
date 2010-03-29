#include <kernel.h>
#include <arch/idt.h>
#include <arch/syscalls.h>

void syscall_exc(struct ia32_exc_ctx ctx)
{
	printk("syscall %lu\n", ctx.eax);
	switch(ctx.eax) {
	case _SYS_exit:
		syscall_exit();
		break;
	default:
		break;
	}
}

