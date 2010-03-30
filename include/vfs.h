#ifndef __VFS_HEADER_INCLUDED__
#define __VFS_HEADER_INCLUDED__

#include <stat.h>
#include <ext2_sb.h>

/* Inode objects */
struct inode {
	/* Filled in by iget() */
	ino_t			i_ino;
	struct super		*i_sb;
	uint32_t		i_count;

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
	struct super *prev,*next;

	/* These are not for filesystems to touch */
	struct blkdev *s_dev;
	struct vfs_fstype *s_type;

	/* Here is what get_super must fill in */
	unsigned int s_blocksize;
	const struct super_ops *s_ops;
	struct inode *s_root;

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
};

struct inode *nul_inode_lookup(struct inode *, const char *name, size_t nlen);
ssize_t nul_inode_pread(struct inode *, void *buf, size_t len, off_t off);

/* VFS functions called by arch kernel */
void vfs_init(void);
void _inode_cache_init(void);
void _dentry_cache_init(void);
void vfs_mount_root(void);

/* Object registration */
void vfs_add_fstype(struct vfs_fstype *);

/* Inode cache interface */
struct inode *iget(struct super *, ino_t);
struct inode *iref(struct inode *i);
void iput(struct inode *);

/* Dentry cache interface */
struct inode *namei(const char *);

#endif /* __VFS_HEADER_INCLUDED__ */
