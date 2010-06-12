/*
 * Direct file access mechanisms
 */

#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/fcntl.h>

static objcache_t file_alloc;

void _file_init(void)
{
	file_alloc = objcache_init(NULL, "file_alloc", sizeof(struct file));
	BUG_ON(NULL == file_alloc);
}

struct file *file_new(struct inode *inode, unsigned int mode)
{
	struct file *file;

	file = objcache_alloc0(file_alloc);
	if ( NULL == file )
		return NULL;

	file->inode = inode;
	file->mode = mode;
	file->offset = 0;
	file->refcount = 1;

	return file;
}

void file_release(struct file *file)
{
	file->refcount--;
	if ( file->refcount == 0 )
		objcache_free(file);
}

