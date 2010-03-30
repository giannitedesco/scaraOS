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

static struct vfs_fstype *fs_types;
static struct super *superblocks;

/* Initialise all aspects of the VFS layer */
void vfs_init(void)
{
	_inode_cache_init();
	_dentry_cache_init();
}

/* Add a filesystem type */
void vfs_add_fstype(struct vfs_fstype *t)
{
	if ( !t )
		return;
	t->next = fs_types;
	fs_types = t;
}

/* Retrieve a filesystem type object */
static struct vfs_fstype *vfs_get_fstype(const char *name)
{
	struct vfs_fstype *r;

	for(r = fs_types; r; r = r->next) {
		if ( !strcmp(name, r->name) )
			return r;
	}

	return NULL;
}

/* hack to mount the root filesystem */
void vfs_mount_root(void)
{
	static struct super sb;
	const char *type = "ext2";
	const char *dev = "floppy0";
	
	sb.s_type = vfs_get_fstype(type);
	if ( NULL == sb.s_type ) {
		printk("vfs: unknown fstype %s\n", type);
		return;
	}

	sb.s_dev = blkdev_get(dev);
	if ( NULL == sb.s_dev ) {
		printk("vfs: unknown block device %s\n", dev);
		return;
	}

	sb.s_blocksize = 0;

	if ( sb.s_type->read_super(&sb) ) {
		printk("vfs: error mounting root filesystem\n");
		return;
	}

	superblocks = &sb;

	/* Setup the tasks structures */
	__this_task->root = superblocks->s_root;
	__this_task->cwd = iref(__this_task->root);
}
