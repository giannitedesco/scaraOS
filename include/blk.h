#ifndef __BLK_HEADER_INCLUDED__
#define __BLK_HEADER_INCLUDED__

struct buffer {
	struct blkdev 	*b_dev;
	block_t 	b_block;
	char 		*b_buf;
	size_t		b_len;
	uint32_t 	b_flags;
	uint32_t	b_count;
};

#define BH_uptodate	0x01
#define BH_dirty	0x02

#define get_buffer(b)	((b)->b_count++)
#define put_buffer(b)	((b)->b_count--)

struct blkdev {
	/* Used only by bio subsystem */
	struct blkdev *next;

	/* Hardcoded by driver */
	char *name;
	int sectsize;
	int (*ll_rw_blk)(int, block_t, char *, size_t);

	/* Can be frobbed with */
	size_t count;
};

/* Block and buffer cache */
void blk_init(void);
struct buffer *blk_read(struct blkdev *, int);
int blk_set_blocksize(struct blkdev *, int);
void blk_free(struct buffer *);

/* Block device maangement */
void blkdev_add(struct blkdev *);
struct blkdev *blkdev_get(char *);

#endif /* __BLK_HEADER_INCLUDED__ */
