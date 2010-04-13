#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/fcntl.h>
#include <arch/mm.h>

int _sys_read(unsigned int handle, char *buf, size_t size)
{
	struct fdt_entry *fd;
	char *kbuf;
	int ret;
	struct task *me;

	me = __this_task;

	fd = fdt_entry_retr(me->fd_table, handle);
	if ( NULL == fd )
		return -1; /* EMAXFILE */

	kbuf = kmalloc(size + 1);
	if ( NULL == kbuf )
		return -1; /* ENOMEM */

	memset(kbuf, '\0', size + 1);

	if ( fd->file->inode->i_size < size )
		size = fd->file->inode->i_size;

	ret = fd->file->inode->i_iop->pread(fd->file->inode, kbuf, size, 0);
	if ( ret < 0 ) {
		printk("read: bad return %d.\n", ret);
		kfree(kbuf);
		return -1;
	}
	if ( ret == 0 ) {
		kfree(buf);
		return 0;
	}
	if ( ret < (int)size )
		size = (size_t)ret;

	fd->file->offset += (unsigned int)size;

	if ( kbuf[size] != '\0' )
		kbuf[size] = '\0';

	/* This code is required once we're working in userland */
	/*if ( copy_to_user(buf, kbuf, size) == -1 ) {
		kfree(kbuf);
		return -1;
	}*/

	/* This code is temporary and will cause headaches if not removed
	 * when the above code is enabled.
	 */	
	memcpy(buf, kbuf, size);

	kfree(kbuf);
	return size;
}
