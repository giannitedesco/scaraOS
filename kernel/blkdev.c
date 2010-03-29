#include <kernel.h>
#include <blk.h>

static struct blkdev *bdevs;

void blkdev_add(struct blkdev *bd)
{
	BUG_ON(NULL == bd);
	bd->next = bdevs;
	bdevs = bd;
}

struct blkdev *blkdev_get(const char *name)
{
	struct blkdev *r;

	for(r = bdevs; r; r = r->next) {
		if ( !strcmp(name, r->name) )
			return r;
	}

	return NULL;
}
