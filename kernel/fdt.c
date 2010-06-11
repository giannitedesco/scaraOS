/*
 * Per-process file descriptor table
 *
 */
#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/fcntl.h>

static objcache_t fd_table_alloc;

void _fdt_init(void)
{
	fd_table_alloc = objcache_init(NULL, "fd_table_alloc",
		sizeof(struct fd_table));
	BUG_ON(NULL == fd_table_alloc);
	_file_init();
}

struct fd_table *fd_table_init(void)
{
	struct fd_table *fd_table;

	fd_table = objcache_alloc0(fd_table_alloc);
	if ( NULL == fd_table )
		return NULL;

	fd_table->fd_alloc = objcache_init(NULL, "fd_alloc",
		sizeof(struct fdt_entry));
	if ( NULL == fd_table->fd_alloc ) {
		objcache_free(fd_table);
		return NULL;
	}

	INIT_LIST_HEAD(&(fd_table->fds));

	return fd_table;
}

int fdt_entry_add(struct fd_table *fd_table, struct file *file)
{
	struct fdt_entry *fdt_entry, *fdt, *tmp;
	unsigned int handle, old_handle;

	if ( NULL == file )
		return -1;

	fdt_entry = objcache_alloc0(fd_table->fd_alloc);
	if ( NULL == fdt_entry )
		return -1;

	if ( list_empty(&fd_table->fds) )
		handle = 0;
	else {
		handle = 0;
		do {
			old_handle = handle;
			list_for_each_entry_safe(fdt, tmp, &fd_table->fds,
					fdt_list) {
				fdt = list_entry(tmp->fdt_list.next,
					struct fdt_entry, fdt_list);
				if ( fdt->handle == handle )
					handle++;
			}
		} while ( old_handle != handle );
	}

	fdt_entry->handle = handle;
	fdt_entry->file = file;
	fdt_entry->refcount = 1;
	list_add(&fdt_entry->fdt_list, &fd_table->fds);
	return handle;
}

struct file *fdt_entry_retr(struct fd_table *fd_table, unsigned int handle)
{
	struct fdt_entry *fdt, *tmp;

	if ( list_empty(&fd_table->fds) )
		return NULL;

	list_for_each_entry_safe(fdt, tmp, &fd_table->fds, fdt_list) {
		fdt = list_entry(tmp->fdt_list.next, struct fdt_entry,
				fdt_list);
		if ( fdt->handle == handle )
			return fdt->file;
	}

	return NULL;
}

void fdt_entry_del(struct fd_table *fd_table, unsigned int handle)
{
	struct fdt_entry *fdt, *tmp;

	if ( list_empty(&fd_table->fds) )
		return;

	list_for_each_entry_safe(fdt, tmp, &fd_table->fds, fdt_list) {
		fdt = list_entry(tmp->fdt_list.next, struct fdt_entry,
				fdt_list);
		if ( fdt->handle == handle ) {
			if ( fdt->refcount == 1 ) {
				file_release(fdt->file);
				list_del(&fdt->fdt_list);
			} else
				fdt->refcount--;

			return;
		}
	}

	return;
}
