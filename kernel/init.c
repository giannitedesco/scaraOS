#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/blk.h>
#include <scaraOS/mm.h>
#include <scaraOS/syscall.h>
#include <arch/pci.h>

static inline _SYSCALL1(_SYS_exec, int, _kernel_exec, const char *);
static inline _SYSCALL2(_SYS_open, int, _kernel_open, const char *, unsigned int);
static inline _SYSCALL3(_SYS_read, int, _kernel_read, unsigned int, char *, size_t);
static inline _SYSCALL1(_SYS_close, int, _kernel_close, unsigned int);

/* Init task - the job of this task is to initialise all
 * installed drivers, mount the root filesystem and
 * bootstrap the system */
int init_task(void *priv)
{
	uint32_t ret;
	int fd, prv_fd;
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

	fd = _kernel_open("/test.txt", 0);
	if ( fd < 0 ) {
		printk("init_task: open failed, returned %u\n", fd);
	} else {
		rval = _kernel_read(fd, buf, 16);
		if ( rval <= 0 )
			printk("read error: %d\n", rval);
		else {
			buf[rval] = '\0';
			printk("read: %s.\n", buf);
		}
		_kernel_close(fd);
		/* Dirty test to check fd allocation */
		prv_fd = fd;
		fd = _kernel_open("/test.txt", 0);
		if ( fd != prv_fd )
			printk("init_task: reopen returned different fd!\n");
		else
			printk("init_task: reopen returned same fd!\n");
		_kernel_close(fd);
	}

	ret = _kernel_exec("/bin/bash");
	ret = _kernel_exec("/sbin/init");
	printk("exec: /sbin/init: %i\n", (int)ret);

	return ret;
}
