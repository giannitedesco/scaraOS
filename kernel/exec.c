#include <kernel.h>
#include <arch/idt.h>
#include <arch/regs.h>
#include <arch/syscalls.h>
#include <task.h>

uint32_t syscall_exec(uint32_t path)
{
	return -1;
}
