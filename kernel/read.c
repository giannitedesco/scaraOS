#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/fcntl.h>
#include <arch/mm.h>

int _sys_read(unsigned int handle, char *buf, size_t size)
{
	struct file *fd;
	struct task *me;

	if ( !uaddr_ok((vaddr_t)buf, size) )
		return -1; /* EINVALID */
	
	me = __this_task;
	fd = fdt_entry_retr(me->fd_table, handle);
	if ( NULL == fd )
		return -1; /* EMAXFILE */
	return kernel_read(fd, buf, size);
}

int kernel_read(struct file *file, char *buf, size_t size)
{
	int ret;

	if ( file->inode->i_size < size )
		size = file->inode->i_size;
	ret = file->inode->i_iop->pread(file->inode, buf, size, file->offset);
	if ( ret > 0 )
		file->offset += ret;

	return ret;
}
