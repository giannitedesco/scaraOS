#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/fcntl.h>
#include <arch/mm.h>

int _sys_open(const char *fn, unsigned int mode)
{
	struct inode *inode;
	int fd;
	char *kfn;
	struct task *me;

	me = __this_task;

	kfn = strdup_from_user(fn);
	if ( NULL == kfn )
		return -1; /* EFAULT or ENOMEM */

	inode = namei(kfn);
	if ( NULL == inode ) {
		printk("open: %s: ENOENT or ENOTDIR\n", kfn);
		kfree(kfn);
		return -1;
	}
  
	fd = fdt_entry_add(me->fd_table, file_new(inode, mode));
	kfree(kfn);

	return fd;
}

struct file *kernel_open(const char *fn, unsigned int mode)
{
	struct inode *inode;
	
	inode = namei(fn);
	if ( NULL == inode ) {
		printk("open: %s: ENOENT or ENOTDIR\n", fn);
		return NULL;
	}

	return file_new(inode, mode);
}
