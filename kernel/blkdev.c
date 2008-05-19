#include <kernel.h>
#include <blk.h>

struct blkdev *bdevs=NULL;

void blkdev_add(struct blkdev *bd)
{
	if ( !bd ) return;
	bd->next=bdevs;
	bdevs=bd;
}

struct blkdev *blkdev_get(char *name)
{
	struct blkdev *r;

	for(r=bdevs; r; r=r->next) {
		if ( !strcmp(name, r->name) )
			return r;
	}

	return NULL;
}
