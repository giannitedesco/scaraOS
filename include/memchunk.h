/*
 * This file is part of Firestorm NIDS.
 * Copyright (c) 2008 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the terms of the GNU GPL version 3
*/
#ifndef _FIRESTORM_MEMCHUNK_HEADER_INCLUDED_
#define _FIRESTORM_MEMCHUNK_HEADER_INCLUDED_

#define OBJCACHE_DEBUG_FREE 1
#define OBJCACHE_POISON 1
#define OBJCACHE_POISON_PATTERN 0xa5

struct _objcache {
	/** Size of objects to allocate */
	size_t o_sz;
	/** Number of objects which can be packed in to one chunk */
	unsigned int o_num;
	/** Pointer to next object to allocate */
	uint8_t *o_ptr;
	/** Pointer to byte after last object in current chunk */
	uint8_t *o_ptr_end;
	/** Freshest chunk (we never allocated to o_ptr_end yet) */
	struct slab_hdr *o_cur;
	/** List of chunks which have a free list */
	struct list_head o_partials;
	/** List of full chunks */
	struct list_head o_full;
	/** Mempool to allocate from */
	struct _mempool *o_pool;
	/** Every objcache is in the main memchunk list */
	struct list_head o_list;
	/** Text label for this objcache */
	const char *o_label;
};

struct _mempool {
	struct slab_hdr *p_reserved;
	size_t p_num_reserve;
	size_t p_reserve;
	struct list_head p_caches;
	struct list_head p_list;
	const char *p_label;
};

struct _memchunk {
	struct _mempool m_gpool;
	struct _objcache m_self_cache;
	struct _objcache m_pool_cache;
	struct list_head m_pools;
};

#endif /* _FIRESTORM_MEMCHUNK_HEADER_INCLUDED_ */
