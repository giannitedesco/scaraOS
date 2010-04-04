#ifndef __VFS_HEADER_INCLUDED__
#define __VFS_HEADER_INCLUDED__

#include <scaraOS/semaphore.h>
#include <scaraOS/stat.h>
#include <scaraOS/ext2_sb.h>

/* Inode objects */
struct inode {
	/* Filled in by iget() */
	struct rb_node		i_cache;
	struct list_head	i_list;
	ino_t			i_ino;
	struct super		*i_sb;
	uint32_t		i_count;
	struct rb_root		i_pagecache;

	/* protectes page cache */
	struct semaphore	i_sem;

	/* Filled in by super->read_inode */
	const struct inode_ops	*i_iop;
	umode_t			i_mode;
	uid_t			i_uid;
	gid_t			i_gid;
	off_t			i_size;
	nlink_t			i_nlink;

	unsigned long		i_blocks;

	union {
		struct ext2_in_info	ext2;
		char			pad[32];
	}u;
};

/* Directory entry */
struct dentry {
	struct inode		*d_ino;
	uint32_t		d_count;
	const unsigned char	*d_name;
	size_t			d_len;
	uint32_t		d_hash;
};

/* Superblock */
struct super {
	struct list_head 	s_list;

	/* These are not for filesystems to touch */
	struct blkdev 		*s_dev;
	struct vfs_fstype 	*s_type;
	struct rb_root 		s_inode_cache;

	/* protects inode cache */
	struct semaphore	s_sem;

	/* Here is what get_super must fill in */
	unsigned int 		s_blocksize;
	const struct super_ops	*s_ops;
	struct inode 		*s_root;

	union {
		struct ext2_sb_info	ext2;
		char			pad[32];
	}u;
};

/* FS type plugin */
struct vfs_fstype {
	struct vfs_fstype *next;
	char *name;
	int (*read_super)(struct super *s);
};

/* Superblock operations */
struct super_ops {
	int (*read_inode)(struct inode *);
};

struct inode_ops {
	struct inode *(*lookup)(struct inode *, const char *name, size_t nlen);
	ssize_t (*pread)(struct inode *, void *buf, size_t len, off_t off);
	struct page *(*readpage)(struct inode *, off_t off);
};

struct inode *nul_inode_lookup(struct inode *, const char *name, size_t nlen);
ssize_t nul_inode_pread(struct inode *, void *buf, size_t len, off_t off);
struct page *nul_inode_readpage(struct inode *, off_t off);

struct page *pagecache_file_readpage(struct inode *, off_t off);

/* VFS functions called by arch kernel */
void vfs_init(void);
void _inode_cache_init(void);
void _dentry_cache_init(void);
void _mounts_init(void);

/* MM balancing */
void vfs_squeeze(void);
void _squeeze_inode_cache(void);

/* Superblocks and mount points */
int vfs_mount_root(const char *type, const char *dev);
struct super *super_get(struct vfs_fstype *type, struct blkdev *blkdev);

/* Object registration */
void vfs_add_fstype(struct vfs_fstype *);

/* Inode cache interface */
struct inode *iget(struct super *, ino_t);
void inode_free(struct inode *i);
static inline struct inode *iref(struct inode *i)
{
	i->i_count++;
	return i;
}
static inline void iput(struct inode *i)
{
	BUG_ON(i->i_count == 0);
	if ( 0 == --i->i_count )
		inode_free(i);
}

/* Dentry cache interface */
struct inode *namei(const char *);

#endif /* __VFS_HEADER_INCLUDED__ */
