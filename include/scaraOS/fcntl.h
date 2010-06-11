#ifndef _SCARAOS_FCNTL_H
#define _SCARAOS_FCNTL_H

#define O_RDONLY 0x0001

/* A file */
struct file {
	struct inode *inode;
	unsigned int mode;
	off_t offset;
	unsigned int refcount;
};

/* File descriptor table entry */
struct fdt_entry {
	struct list_head fdt_list;
	unsigned int handle;
	struct file *file;
	unsigned int refcount;
};

/* Per-process file descriptor table */
struct fd_table {
	struct list_head fds;
	objcache_t fd_alloc;
};

/* Accessors for fd_table and fdt_entry */
void _fdt_init(void);
struct fd_table *fd_table_init(void);
int fdt_entry_add(struct fd_table *fd_table, struct file *file);
struct file *fdt_entry_retr(struct fd_table *fd_table, unsigned int handle);
void fdt_entry_del(struct fd_table *fd_table, unsigned int handle);

/* Accessors for file */
void _file_init(void);
struct file *file_new(struct inode *inode, unsigned int mode);
static inline void file_get(struct file *file)
{
	if( NULL == file )
		return;
	file->refcount++;
}
void file_release(struct file *file);

/* File ops - userspace */
int _sys_open(const char *fh, unsigned int mode);
int _sys_read(unsigned int fd, char * buf, size_t size);
int _sys_close(unsigned int fd);

/* File ops - kernelspace */
int kernel_read(struct file *file, char * buf, size_t size);
struct file *kernel_open(const char *fh, unsigned int mode);
void kernel_close(struct file *file);

#endif /* _SCARAOS_FCNTL_H */
