#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/vfs.h>
#include <scaraOS/blk.h>
#include <scaraOS/ext2.h>

static ssize_t ext2_pread(struct inode *i, void *buf, size_t len, off_t off)
{
	size_t first_block, last_block, num_blk, x;
	struct buffer *bh;
	ssize_t copied;

	if ( off + len > i->i_size )
		len = i->i_size - off;

	first_block = off & ~(i->i_sb->s_blocksize - 1);
	last_block = (off + len) & ~(i->i_sb->s_blocksize - 1);
	num_blk = last_block - first_block;
	dprintk("EXT2: pread %lu @ %lu: blocks %lu - %lu\n",
		len, off, first_block, last_block);

	BUG_ON(last_block >= EXT2_NDIR_BLOCKS);

	for(copied = 0, x = first_block; x <= last_block; x++) {
		size_t clen, coff;

		bh = blk_read(i->i_sb->s_dev, i->u.ext2.block[x]);
		if ( NULL == bh )
			return copied;

		coff = off & (i->i_sb->s_blocksize - 1);
		clen = (coff + len > i->i_sb->s_blocksize) ? 
				i->i_sb->s_blocksize - coff : len;
		dprintk("EXT2: got block %lu, copy %lu bytes at %lu\n",
			i->u.ext2.block[x], clen, coff);
		memcpy(buf, bh->b_buf + coff, clen);
		copied += clen;
		len -= clen;
		off += clen;

		blk_free(bh);
	}

	return copied;
}

static const struct inode_ops ext2_reg_iop = {
	.lookup = nul_inode_lookup,
	.pread = ext2_pread,
	.readpage = pagecache_file_readpage,
};

/* Lookup a name in an inode */
static struct inode *ext2_lookup(struct inode *i, const char *n, size_t nlen)
{
	struct ext2_dir_entry_2 *dir;
	struct buffer *bh;
	ino_t inum;
	int x, y;
	char *j;

	y = EXT2_NDIR_BLOCKS < i->i_blocks ? EXT2_NDIR_BLOCKS : i->i_blocks;

	for(x = 0; x < y; x++) {
		/* pre-allocation ?? */
		if ( i->u.ext2.block[x] == 0 )
			continue;

		/* Read the block */
		bh = blk_read(i->i_sb->s_dev, i->u.ext2.block[x]);
		if ( NULL == bh )
			return NULL;

		/* Search for the item */
		for(j = bh->b_buf; j < (bh->b_buf + bh->b_len); ) {
			dir = (struct ext2_dir_entry_2 *)j;
			dprintk("EXT2: dentry: %.*s\n",
				dir->name_len, dir->name);
			if ( (dir->name_len == nlen) &&
				!memcmp(dir->name, n, nlen) ) {
				inum = dir->inode;
				blk_free(bh);
				return iget(i->i_sb, inum);
			}
			j += dir->rec_len;
		}

		blk_free(bh);
	}

	return NULL;
}

/* Directory ops */
static const struct inode_ops ext2_dir_iop = {
	.lookup = ext2_lookup,
	.pread = nul_inode_pread,
	.readpage = nul_inode_readpage,
};

/* Fill in an inode structure for iget */
static int ext2_read_inode(struct inode *i)
{
	unsigned long block_group;
	unsigned long group_desc, desc;
	unsigned long offset, block;
	struct ext2_group_desc *gdp;
	struct ext2_inode *raw_inode;
	struct buffer *b;

	/* 1. Check inode number */
	if ( i->i_ino > i->i_sb->u.ext2.s_es->s_inodes_count ) {
		printk("EXT2: bad inode %lu\n", i->i_ino);
		return -1;
	}

	/* 2. Calculate block group number */
	block_group = (i->i_ino - 1) / i->i_sb->u.ext2.s_inodes_per_group;
	if ( block_group >= i->i_sb->u.ext2.s_groups_count ) {
		printk("EXT2: Bad group %lu\n", block_group);
		return -1;
	}
	
	/* 3. Obtain block group descriptor */
	group_desc = block_group / i->i_sb->u.ext2.s_desc_per_block;
	desc = block_group % i->i_sb->u.ext2.s_desc_per_block;
	b = i->i_sb->u.ext2.s_group_desc[group_desc];
	gdp = (struct ext2_group_desc *)b->b_buf;
	
	/* 4. Obtain correct block from inode table */
	block = gdp[desc].bg_inode_table + 
		((i->i_ino - 1) / i->i_sb->u.ext2.s_inodes_per_block);
	if ( !(b=blk_read(i->i_sb->s_dev, block)) ) {
		printk("EXT2: Unable to read inode block - inode=%lu, block=%lu\n",
			i->i_ino, block);
		return -1;
	}

	/* 5. Obtain the inode pointer */
	offset = (i->i_ino - 1) * i->i_sb->u.ext2.s_es->s_inode_size;
	offset &= (i->i_sb->s_blocksize - 1);
	raw_inode = (struct ext2_inode *)(b->b_buf + offset);
	
	/* 6. Copy the inode */
	dprintk("Inode %lu mode %o\n", i->i_ino, raw_inode->i_mode);
	switch(raw_inode->i_mode & S_IFMT) {
	case S_IFDIR:
		i->i_iop = &ext2_dir_iop;
		break;
	case S_IFREG:
		i->i_iop = &ext2_reg_iop;
		break;
	default:
		printk("EXT2: Unsupported file type: 0%o\n",
			raw_inode->i_mode);
		blk_free(b);
		return -1;
	}

	i->i_mode = raw_inode->i_mode;
	i->i_uid = raw_inode->i_uid;
	i->i_gid = raw_inode->i_gid;
	i->i_size = raw_inode->i_size;
	i->i_nlink = raw_inode->i_links_count;
	i->i_blocks = raw_inode->i_blocks;

	for(block = 0; block < EXT2_N_BLOCKS; block++) {
		i->u.ext2.block[block] = raw_inode->i_block[block];
	}

	/* Finish up */
	blk_free(b);
	return 0;
}

/* Superblock operations */
static const struct super_ops ext2_superops = {
	.read_inode = ext2_read_inode,
};

static int ext2_get_super(struct super *sb)
{
	struct ext2_super_block *s;
	struct buffer *bh;
	int db_count, i;

	/* Set blocksize for the device */
	if ( blk_set_blocksize(sb->s_dev, EXT2_MIN_BLOCK_SIZE) ) {
		printk("EXT2: Unable to set blocksize %d\n",
			EXT2_MIN_BLOCK_SIZE);
		return -1;
	}

	/* Read in the super block */
	bh = blk_read(sb->s_dev, 1);
	if ( NULL == bh ) {
		printk("EXT2: %s: unable to read super\n",
			sb->s_dev->name);
		return -1;
	}

	s = (struct ext2_super_block *)bh->b_buf;

	/* Check the super block */
	if ( s->s_magic != EXT2_SUPER_MAGIC ) {
		printk("EXT2: Bad voodoo magic on superblock!\n");
		goto err;
	}

	if ( s->s_log_block_size ) {
		printk("EXT2: only 1KB blocksize supported\n");
		goto err;
	}

	/* Fill in the superblock structures */
	sb->s_blocksize = EXT2_MIN_BLOCK_SIZE;
	sb->s_ops = &ext2_superops;

	/* Fill in custom structures */
	sb->u.ext2.s_sbh = bh;
	sb->u.ext2.s_es = s;
	sb->u.ext2.s_inodes_per_block = sb->s_blocksize / s->s_inode_size;
	sb->u.ext2.s_blocks_per_group = s->s_blocks_per_group;
	sb->u.ext2.s_inodes_per_group = s->s_inodes_per_group;
	sb->u.ext2.s_itb_per_group = s->s_inodes_per_group / 
					sb->u.ext2.s_inodes_per_block;
	sb->u.ext2.s_desc_per_block = sb->s_blocksize / 
					sizeof(struct ext2_group_desc);
	sb->u.ext2.s_groups_count = (s->s_blocks_count - 
					s->s_first_data_block + 
					(sb->u.ext2.s_blocks_per_group-1)) /
					sb->u.ext2.s_blocks_per_group;

	/* Read in the group descriptors */
	db_count = (sb->u.ext2.s_groups_count +
		sb->u.ext2.s_desc_per_block - 1) / sb->u.ext2.s_desc_per_block;
	sb->u.ext2.s_group_desc = kmalloc(db_count * sizeof(struct buffer *));
	for(i = 0; i < db_count; i++) {
		int j;
		sb->u.ext2.s_group_desc[i] = blk_read(sb->s_dev, i + 2);
		if ( !sb->u.ext2.s_group_desc[i] ) {
			printk("EXT2: unable to read group descriptors\n");
			for(j = 0; j < i; j++)
				blk_free(sb->u.ext2.s_group_desc[i]);
			goto err;
		}
	}

	/* Lookup root inode */
	sb->s_root = iget(sb, EXT2_ROOT_INO);
	if ( NULL == sb->s_root ) {
		printk("EXT2: get root inode failed\n");
		goto err;
	}

	return 0;
err:
	blk_free(bh);
	return -1;
}

static struct vfs_fstype ext2_fstype={
	.name = "ext2",
	.read_super = ext2_get_super,
};

__init static void ext2_init(void)
{
	vfs_add_fstype(&ext2_fstype);
}
driver_init(ext2_init);
