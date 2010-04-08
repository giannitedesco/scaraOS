/*
 * Global file descriptor table
 *
 * TODO: Implement file descriptors for STDIN/OUT/ERR.
 * TODO: Later, we'll want to provide this per-process.
 */
#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/vfs.h>

static LIST_HEAD(file_descriptors);
static objcache_t fds;
static unsigned int next_handle;

void _fd_table_init (void)
{
	fds = objcache_init(NULL, "fds", sizeof(struct fdt_entry));
	next_handle = 3;
	BUG_ON(NULL == fds);
}

struct fdt_entry *fdt_entry_add(struct inode *inode, unsigned int mode)
{
	struct fdt_entry *fdt_entry;

	fdt_entry = objcache_alloc0(fds);
	if ( NULL == fdt_entry)
		return NULL;

	fdt_entry->handle = next_handle++;
	fdt_entry->inode = inode;
	fdt_entry->mode = mode;
	list_add(&fdt_entry->fdt_list, &file_descriptors);
	return fdt_entry;
}

struct fdt_entry *fdt_entry_retr(unsigned int handle)
{
	struct fdt_entry *fdt, *tmp;

	if ( list_empty(&file_descriptors) )
		return NULL;

	list_for_each_entry_safe(fdt, tmp, &file_descriptors, fdt_list) {
		fdt = list_entry(tmp->fdt_list.next, struct fdt_entry,
				fdt_list);
		if ( fdt->handle == handle )
			return fdt;
	}

	return NULL;
}

void fdt_entry_del(unsigned int handle)
{
	struct fdt_entry *fdt, *tmp, *dead;

	if ( list_empty(&file_descriptors) )
		return;

	list_for_each_entry_safe(fdt, tmp, &file_descriptors, fdt_list) {
		fdt = list_entry(tmp->fdt_list.next, struct fdt_entry,
				fdt_list);
		if ( fdt->handle == handle ) {
			dead = list_del(&fdt->list);
			objcache_free(dead);
			return;
		}
	}

	return;
}
