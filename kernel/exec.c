#include <kernel.h>
#include <arch/regs.h>
#include <arch/syscalls.h>
#include <task.h>
#include <vfs.h>
#include <elf.h>

static int do_exec(const char *path)
{
	struct inode *inode;

	inode = namei(path);
	printk("exec: %s -> %p\n", path, inode);
	return -1;
}

uint32_t syscall_exec(uint32_t path)
{
	return do_exec((char *)path);
}
