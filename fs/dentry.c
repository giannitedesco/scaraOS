/*
 * The scaraOS dentry cache.
 *
 * The purpose of this code is to perform VFS name lookups ie: to
 * convert from file-paths/file-names to actual inode objects. The
 * dcache exists to speedup this lookup.
 *
 * TODO
 *  o Actually cache items
 *  o Implement cache shrinkage routines
*/
#include <kernel.h>
#include <blk.h>
#include <vfs.h>
#include <mm.h>
#include <task.h>

struct m_cache dentry_cache={
	.name="dentry",
	.size=sizeof(struct dentry)
};

/* Resolve a name to an inode */
struct inode *namei(char *name)
{
	struct inode *i;
	char *n;
	int state,end=0;

	if ( !name ) return NULL;

	if ( *name=='/' ) {
		name++;
		i=__this_task->root;
	}else{
		i=__this_task->cwd;
	}

	/* Check for some bogus bugs */
	if ( !i ) {
		printk("uhm, bad shit...\n");
		return NULL;
	}
	if ( !i->i_iop ) {
		printk("no inode ops\n");
		return NULL;
	}
	if ( !i->i_iop->lookup ) {
		printk("no lookup iop\n");
		return NULL;
	}

	/* Iterate the components of the filename
	 * looking up each one as we go */
	for(n=name,state=1; ;n++) {
		if ( state==0 ) {
			struct inode *itmp;
			if ( *n==0 || *n=='/' ) {
				if ( *n==0 ) end=1;
				*n=0;
				state=1;
				itmp=i;
				if ( !(itmp=i->i_iop->lookup(itmp,name)) ) {
					return NULL;
				}
				if ( end ) break;
				iput(i);
			}
		}else if ( state==1 ) {
			if ( *n==0 ) break;
			if ( *n!='/' ) {
				state=0;
				name=n;
			}
		}
	}

	return i;
}
