/*
 * Core of the VFS layer
 * TODO
 *  o Proper mount/umount
*/
#include <kernel.h>
#include <mm.h>
#include <blk.h>
#include <vfs.h>
#include <task.h>

struct vfs_fstype *fs_types=NULL;
struct super *superblocks=NULL;

/* Various slab caches */
extern struct m_cache inode_cache;
extern struct m_cache dentry_cache;

/* Initialise all aspects of the VFS layer */
void vfs_init(void)
{
	kmem_add_cache(&inode_cache);
	kmem_add_cache(&dentry_cache);
}

/* Add a filesystem type */
void vfs_add_fstype(struct vfs_fstype *t)
{
	if ( !t )
		return;
	t->next=fs_types;
	fs_types=t;
}

/* Retrieve a filesystem type object */
struct vfs_fstype *vfs_get_fstype(char *name)
{
	struct vfs_fstype *r;

	for(r=fs_types; r; r=r->next) {
		printk("%s / %s\n", name, r->name);
		if ( !strcmp(name, r->name) )
			return r;
	}

	return NULL;
}

/* hack to mount the root filesystem */
void vfs_mount_root(void)
{
	static struct super sb;
	char *type="ext2";
	char *dev="floppy0";
	
	if ( !(sb.s_type=vfs_get_fstype(type)) ) {
		printk("vfs: unknown fstype %s\n", type);
		return;
	}

	if ( !(sb.s_dev=blkdev_get(dev)) ) {
		printk("vfs: unknown block device %s\n", dev);
		return;
	}

	sb.s_blocksize=0;

	if ( sb.s_type->read_super(&sb) ) {
		printk("vfs: error mounting root filesystem\n");
		return;
	}

	superblocks=&sb;

	/* Setup the tasks structures */
	__this_task->root=superblocks->s_root;
	__this_task->cwd=superblocks->s_root;
}
