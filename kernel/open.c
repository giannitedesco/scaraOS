#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <arch/mm.h>
#include <scaraOS/open.h>

int _sys_open(const char *fn, unsigned int mode)
{
	struct inode *inode;
	char *kfn;
	char *buf;
	size_t len;
	ssize_t ret;

	kfn = strdup_from_user(fn, UACCESS_KERNEL_OK);
	if ( NULL == kfn )
		return -1; /* EFAULT or ENOMEM */

	inode = namei(kfn);
	if ( NULL == inode ) {
		printk("open: %s: ENOENT or ENOTDIR\n", kfn);
		kfree(kfn);
		return -1;
	}

	len = inode->i_size;

	printk("open: %s: mode 0%lo, %lu bytes in %lu blocks\n",
		kfn, inode->i_mode, len, inode->i_blocks);

	kfree(kfn);

	buf = kmalloc(len);
	if ( NULL == buf )
		return -1; /* ENOMEM */

	memset(buf, '\0', len);

	ret = inode->i_iop->pread(inode, buf, len, 0);
	if ( ret <= 0 || (size_t)ret != len ) {
		printk("open: unable to read: expected %lu, got %lu.\n",
			len, (size_t)ret);
		kfree(buf);
		return -1;
	}

	if ( buf[len] != '\0' )
		buf[len] = '\0';

	printk("open: data: %s.\n", buf);
	kfree(buf);
	return 0;
}
