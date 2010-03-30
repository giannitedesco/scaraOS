#include <kernel.h>
#include <arch/regs.h>
#include <arch/syscalls.h>
#include <task.h>
#include <vfs.h>
#include <elf.h>

static int do_exec(const char *path)
{
	struct inode *inode;
	Elf32_Ehdr hdr;
	ssize_t ret;

	inode = namei(path);
	if ( NULL == inode ) {
		printk("exec: %s: ENOENT or ENOTDIR\n", path);
		return -1;
	}

	printk("exec: %s: mode 0%lo, %lu bytes in %lu blocks\n",
		path, inode->i_mode, inode->i_size, inode->i_blocks);
	
	ret = inode->i_iop->pread(inode, &hdr, sizeof(hdr), 0);
	if ( ret <= 0 || (size_t)ret != sizeof(hdr) ) {
		printk("exec: unable to read header\n");
		return ret;
	}

	hex_dumpk((uint8_t *)&hdr, sizeof(hdr), 16);
	return -1;
}

uint32_t syscall_exec(uint32_t path)
{
	return do_exec((char *)path);
}
