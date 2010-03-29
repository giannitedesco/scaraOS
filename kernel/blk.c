/*
 * Kernel block device management routines.
 *
 * TODO:
 *   o Write support
 *   o Asynchronous API
*/

#include <kernel.h>
#include <mm.h>
#include <blk.h>

static struct m_cache bh_cache={
	.name = "buffer",
	.size = sizeof(struct buffer)
};

void blk_init(void)
{
	kmem_add_cache(&bh_cache);
}

/* Read a block in to a buffer */
struct buffer *blk_read(struct blkdev *dev, int logical)
{
	struct buffer *ret;

	if ( !(ret=kmem_alloc(&bh_cache)) )
		return NULL;

	ret->b_dev = dev;
	ret->b_block = logical;
	ret->b_buf = kmalloc(dev->count * dev->sectsize);
	ret->b_len = dev->count * dev->sectsize;
	if ( NULL == ret->b_buf ) {
		kfree(ret);
		return NULL;
	}

	/* Synchronously read from the device */
	if ( dev->ll_rw_blk(0, logical * dev->count, ret->b_buf, dev->count) ) {
		kfree(ret->b_buf);
		kfree(ret);
		return NULL;
	}

	return ret;
}

void blk_free(struct buffer *b)
{
	kfree(b->b_buf);
	kfree(b);
}

/* Set blocksize of device (NOTE: Blocksize is not the same as sector size) */
int blk_set_blocksize(struct blkdev *dev, unsigned int size)
{
	if ( dev->sectsize > size )
		return -1;

	if ( size > PAGE_SIZE || size < 512 || (size & (size-1)) )
		return -1;

	/* dont need to change it */
	if ( dev->sectsize * dev->count == size )
		return 0;

	dev->count = size / dev->sectsize;
	return 0;
}
