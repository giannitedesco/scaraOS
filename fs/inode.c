/*
 * The scaraOS inode cache.
 *
 * The purpose of this code is to lookup an inode object on a mounted
 * filesystem given its inode number. The inode contains all file 
 * related meta-data and is needed for nearly all VFS operations. The
 * cache exists to speed up this lookup.
*/
#include <scaraOS/kernel.h>
#include <scaraOS/vfs.h>
#include <scaraOS/blk.h>
#include <scaraOS/mm.h>

struct inode *nul_inode_lookup(struct inode *i, const char *name, size_t nlen)
{
	return NULL; /* ENOTDIR */
}

ssize_t nul_inode_pread(struct inode *i, void *buf, size_t len, off_t off)
{
	return -1; /* EBADF? */
}

struct page *nul_inode_readpage(struct inode *in, off_t off)
{
	return NULL; /* EBADF? */
}

static objcache_t inodes;
static LIST_HEAD(slack_inodes);
static struct semaphore slacksem = SEMAPHORE_INIT(slacksem, 1);

void _inode_cache_init(void)
{
	inodes = objcache_init(NULL, "inode", sizeof(struct inode));
	BUG_ON(NULL == inodes);
}

/* Retrieve an inode given a mounted fs descriptor and an inode number */
struct inode *iget(struct super *sb, ino_t ino)
{
	struct rb_node **p;
	struct rb_node *parent;
	struct inode *new;

	sem_P(&slacksem);
	sem_P(&sb->s_sem);

	for(p = &sb->s_inode_cache.rb_node, parent = NULL; *p; ) {
		struct inode *in;

		parent = *p;
		in = rb_entry(parent, struct inode, i_cache);
		if ( ino < in->i_ino )
			p = &(*p)->rb_child[RB_LEFT];
		else if ( ino > in->i_ino )
			p = &(*p)->rb_child[RB_RIGHT];
		else {
			in->i_count++;
			list_del(&in->i_list);
			sem_V(&slacksem);
			sem_V(&sb->s_sem);
			return in;
		}
	}

	new = objcache_alloc0(inodes);
	if ( NULL == new )
		goto out_unlock;

	/* Fill in the stuff read_inode needs */
	new->i_ino = ino;
	new->i_sb = sb;
	INIT_SEMAPHORE(&new->i_sem, 1);
	INIT_LIST_HEAD(&new->i_list);

	if ( sb->s_ops->read_inode(new) ) {
		objcache_free2(inodes, new);
		new = NULL;
		goto out_unlock;
	}

	new->i_count = 1;
	rb_link_node(&new->i_cache, parent, p);
	rb_insert_color(&new->i_cache, &sb->s_inode_cache);
out_unlock:
	sem_V(&slacksem);
	sem_V(&sb->s_sem);
	return new;
}

/* Release an inode so that it can be freed under memory pressure */
static void __inode_free(struct inode *i)
{
	if ( NULL == i )
		return;
	
	sem_P(&i->i_sb->s_sem);
	rb_erase(&i->i_cache, &i->i_sb->s_inode_cache);
	list_del(&i->i_list);
	sem_V(&i->i_sb->s_sem);

	objcache_free2(inodes, i);
}

void _squeeze_inode_cache(void)
{
	struct inode *in, *tmp;

	sem_P(&slacksem);
	list_for_each_entry_safe(in, tmp, &slack_inodes, i_list) {
		printk("inode_cache: Killing slack inode %lu\n", in->i_ino);
		__inode_free(in);
	}
	sem_V(&slacksem);
}

/* Release an inode so that it can be freed under memory pressure */
void inode_free(struct inode *i)
{
	printk("inode %lu ref dropped to 0\n", i->i_ino);
	BUG_ON(i->i_count);
	sem_P(&slacksem);
	list_add_tail(&i->i_list, &slack_inodes);
	sem_V(&slacksem);
}
