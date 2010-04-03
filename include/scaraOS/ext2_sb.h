#ifndef __EXT2_SB_HEADER_INCLUDED__
#define __EXT2_SB_HEADER_INCLUDED__

struct ext2_sb_info {
	unsigned long s_inodes_per_block;/* Number of inodes per block */
	unsigned long s_blocks_per_group;/* Number of blocks in a group */
	unsigned long s_inodes_per_group;/* Number of inodes in a group */
	unsigned long s_itb_per_group;	/* Number of inode table blocks per group */
	unsigned long s_gdb_count;	/* Number of group descriptor blocks */
	unsigned long s_desc_per_block;	/* Number of group descriptors per block */
	unsigned long s_groups_count;	/* Number of groups in the fs */
	struct buffer *s_sbh;	/* Real super block buffer */
	struct ext2_super_block *s_es;	/* Real super block */
	struct buffer **s_group_desc;
};

struct ext2_in_info {
	uint32_t block[15];
};

#endif /* __EXT2_SB_HEADER_INCLUDED__ */
