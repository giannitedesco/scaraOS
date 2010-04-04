#include <scaraOS/kernel.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/blk.h>

static struct blkdev *bdevs;

void blkdev_add(struct blkdev *bd)
{
	BUG_ON(NULL == bd);
	bd->next = bdevs;
	bdevs = bd;
	INIT_SEMAPHORE(&bd->blksem, 1);
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
