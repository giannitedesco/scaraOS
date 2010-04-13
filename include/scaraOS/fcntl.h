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

void _fdt_init(void);
struct fd_table *fd_table_init(void);
struct fdt_entry *fdt_entry_add(struct fd_table *fd_table,
	struct inode *inode, unsigned int mode);
struct fdt_entry *fdt_entry_retr(struct fd_table *fd_table,
	unsigned int handle);
void fdt_entry_del(struct fd_table *fd_table, unsigned int handle);

/* File ops - userspace */
int _sys_open(const char *fh, unsigned int mode);
int _sys_read(unsigned int fd, char * buf, size_t size);
int _sys_close(unsigned int fd);

/* File ops - kernelspace */
int kernel_read(unsigned int fd, char * buf, size_t size);

#endif /* _SCARAOS_FCNTL_H */
