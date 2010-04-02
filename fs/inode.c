/*
 * The scaraOS inode cache.
 *
 * The purpose of this code is to lookup an inode object on a mounted
 * filesystem given its inode number. The inode contains all file 
 * related meta-data and is needed for nearly all VFS operations. The
 * cache exists to speed up this lookup.
 *
 * TODO
 *  o Actually cache items
 *  o Implement cache shrinkage routines
*/
#include <kernel.h>
#include <blk.h>
#include <vfs.h>
#include <mm.h>

struct inode *nul_inode_lookup(struct inode *i, const char *name, size_t nlen)
{
	return NULL; /* ENOTDIR */
}

ssize_t nul_inode_pread(struct inode *i, void *buf, size_t len, off_t off)
{
	return -1; /* EBADF? */
}

static objcache_t inode_cache;
void _inode_cache_init(void)
{
	inode_cache = objcache_init(NULL, "inode", sizeof(struct inode));
	BUG_ON(NULL == inode_cache);
}

/* Retrieve an inode given a mounted fs descriptor and an inode number */
struct inode *iget(struct super *sb, ino_t ino)
{
	struct inode *ret;

	ret = objcache_alloc(inode_cache);
	if ( NULL == ret )
		return NULL;

	/* Fill in the stuff read_inode needs */
	ret->i_ino = ino;
	ret->i_sb = sb;

	if ( sb->s_ops->read_inode(ret) ) {
		objcache_free2(inode_cache, ret);
		return NULL;
	}

	ret->i_count = 1;
	return ret;
}

/* Release an inode so that it can be freed
 * under memory pressure */
void inode_free(struct inode *i)
{
	if ( NULL == i )
		return;
	objcache_free2(inode_cache, i);
}
