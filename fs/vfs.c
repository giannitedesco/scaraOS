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

/* Initialise all aspects of the VFS layer */
void vfs_init(void)
{
	_mounts_init();
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

void vfs_squeeze(void)
{
	_squeeze_inode_cache();
}

/* hack to mount the root filesystem */
int vfs_mount_root(const char *type, const char *dev)
{
	struct vfs_fstype *fstype;
	struct blkdev *blkdev;
	struct super *s;

	fstype = vfs_get_fstype(type);
	if ( NULL == fstype ) {
		printk("vfs: unknown fstype %s\n", type);
		return -1;
	}

	blkdev = blkdev_get(dev);
	if ( NULL == blkdev ) {
		printk("vfs: unknown block device %s\n", dev);
		return -1;
	}

	s = super_get(fstype, blkdev);
	if ( NULL == s )
		return -1;

	/* Setup the tasks structures */
	__this_task->root = s->s_root;
	__this_task->cwd = iref(__this_task->root);
	return 0;
}
