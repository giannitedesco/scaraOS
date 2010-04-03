/*
 * Core of the VFS layer
 * TODO
 *  o Proper mount/umount
*/
#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/blk.h>
#include <scaraOS/vfs.h>
#include <scaraOS/task.h>

static LIST_HEAD(superblocks);
static objcache_t supers;

void _mounts_init(void)
{
	supers = objcache_init(NULL, "supers", sizeof(struct super));
	BUG_ON(NULL == supers);
}

struct super *super_get(struct vfs_fstype *type, struct blkdev *blkdev)
{
	struct super *sb;

	sb = objcache_alloc0(supers);
	if ( NULL == sb )
		return NULL;

	sb->s_type = type;
	sb->s_dev = blkdev;
	sb->s_inode_cache.rb_node = NULL;
	if ( sb->s_type->read_super(sb) ) {
		printk("vfs: error mounting root filesystem\n");
		objcache_free2(supers, sb);
		return NULL;
	}

	list_add(&sb->s_list, &superblocks);
	return sb;
}
