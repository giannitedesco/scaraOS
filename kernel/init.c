#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/blk.h>
#include <scaraOS/mm.h>
#include <scaraOS/syscall.h>
#include <arch/pci.h>

static inline _SYSCALL1(_SYS_exec, int, _kernel_exec, const char *);
static inline _SYSCALL2(_SYS_open, int, _kernel_open, const char *, unsigned int);
static inline _SYSCALL3(_SYS_read, int, _kernel_read, struct file *, char *, size_t);
static inline _SYSCALL1(_SYS_close, int, _kernel_close, unsigned int);

/* Init task - the job of this task is to initialise all
 * installed drivers, mount the root filesystem and
 * bootstrap the system */
int init_task(void *priv)
{
	uint32_t ret;
	struct file *file;
	char buf[20];
	int rval;

	/* Initialise kernel subsystems */
	blk_init();
	vfs_init();
	pci_init();

	do_initcalls();

	/* Mount the root filesystem etc.. */
	if ( vfs_mount_root("ext2", "floppy0") ) {
		panic("Unable to mount root filesystem\n");
	}

	ret = _kernel_exec("/bin/bash");
	ret = _kernel_exec("/sbin/init");
	printk("exec: /sbin/init: %i\n", (int)ret);

	return ret;
}
