#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/fcntl.h>
#include <arch/mm.h>

int _sys_read(unsigned int handle, char *buf, size_t size)
{
	if ( !uaddr_ok((vaddr_t)buf, size) )
		return -1; /* EINVALID */
	
	return kernel_read(handle, buf, size);
}

int kernel_read(unsigned int handle, char *buf, size_t size)
{
	struct file *fd;
	int ret;
	struct task *me;

	me = __this_task;

	fd = fdt_entry_retr(me->fd_table, handle);
	if ( NULL == fd )
		return -1; /* EMAXFILE */

	if ( fd->inode->i_size < size )
		size = fd->inode->i_size;

	ret = fd->inode->i_iop->pread(fd->inode, buf, size, 0);
	if ( ret < 0 ) {
		printk("read: bad return %d.\n", ret);
		return -1;
	}
	if ( ret == 0 )
		return 0;

	if ( ret < (int)size )
		size = (size_t)ret;

	fd->offset += (unsigned int)size;

	return size;
}
