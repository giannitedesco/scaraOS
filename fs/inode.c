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

struct m_cache inode_cache={
	.name="inode",
	.size=sizeof(struct inode)
};

/* Retrieve an inode given a mounted fs descriptor and an
 * inode number */
struct inode *iget(struct super *sb, ino_t ino)
{
	struct inode *ret;

	if ( !sb || !sb->s_ops->read_inode ) return NULL;

	if ( !(ret=kmem_alloc(&inode_cache)) )
		return NULL;

	/* Fill in the stuff read_inode needs */
	ret->i_ino=ino;
	ret->i_sb=sb;

	if ( sb->s_ops->read_inode(ret) ) {
		kmem_free(ret);
		return NULL;
	}

	ret->i_count=1;
	return ret;
}

/* Release an inode so that it can be freed
 * under memory pressure */
void iput(struct inode *i)
{
	if ( i==NULL ) return;

	if ( !i->i_count ) {
		printk("iput: Inode already free\n");
		return;
	}

	if ( --i->i_count ) return;

	kmem_free(i);
}
