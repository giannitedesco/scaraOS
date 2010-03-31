#include <kernel.h>
#include <mm.h>

/* exclude 1 and 2 byte allocations */
#define NR_CACHE (PAGE_SHIFT - 1)
#define CACHE_START 8

/* General purpose caches */
static objcache_t general_cache[NR_CACHE];
static char general_names[12][NR_CACHE]; /* 12 chars gets us "size-123456" */

void _kmalloc_init(void)
{
	unsigned int i;
	size_t sz;

	for(sz = (1 << 2), i = 0; i < NR_CACHE; i++, sz <<= 1) {
		snprintf(general_names[i], sizeof(general_names[i]),
			"size-%lu", sz);
		general_cache[i] = objcache_init(NULL, general_names[i], sz);
		BUG_ON(NULL == general_cache[i]);
	}
}

void *kmalloc(size_t alloc_sz)
{
	unsigned int i;
	size_t sz;

	for(sz = (1 << 2), i = 0; i < NR_CACHE; i++, sz <<= 1) {
		if ( alloc_sz <= sz )
			return objcache_alloc(general_cache[i]);
	}

	return NULL;
}

void *kmalloc0(size_t alloc_sz)
{
	unsigned int i;
	size_t sz;

	for(sz = (1 << 2), i = 0; i < NR_CACHE; i++, sz <<= 1) {
		if ( alloc_sz < sz )
			return objcache_alloc0(general_cache[i]);
	}

	return NULL;
}

void kfree(void *ptr)
{
	objcache_free(ptr);
}
