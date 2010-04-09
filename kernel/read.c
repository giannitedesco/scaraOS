#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <arch/mm.h>
#include <scaraOS/read.h>

int _sys_read(unsigned int handle, char *buf, size_t size)
{
	struct fdt_entry *fd;
	char *kbuf;
	int ret;

	fd = fdt_entry_retr(handle);
	if ( NULL == fd )
		return -1; /* EMAXFILE */

	kbuf = kmalloc(size + 1);
	if ( NULL == kbuf )
		return -1; /* ENOMEM */

	memset(kbuf, '\0', size + 1);

	if ( fd->inode->i_size < size )
		size = fd->inode->i_size;

	ret = fd->inode->i_iop->pread(fd->inode, kbuf, size, 0);
	if ( ret <= 0 || (size_t)ret != size ) {
		printk("read: bad return, %d instead of %d.\n", ret, (int)size);
		kfree(kbuf);
		return -1;
	}

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
