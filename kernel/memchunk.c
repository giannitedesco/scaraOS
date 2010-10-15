/*
 * This file is part of Firestorm NIDS.
 * Copyright (c) 2008 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the terms of the GNU GPL version 3
 *
 * Efficient memory allocator for flow tracking. All object types are allocated
 * from a pre-allocated block of memory of a fixed size. Expected uses are:
 *  - flowstates (eg: top level hash tables)
 *  - flows (eg: ipq / tcp_session)
 *  - buffer headers (eg: ip_fragment / tcp_rbuf)
 *  - buffer data: blocks of raw data, some fixed power of 2 size
 *
 * TODO:
 *  o Analysis printout with fragmentation stats
*/

#include <scaraOS/kernel.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/mm.h>
#include <scaraOS/memchunk.h>

#if OBJCACHE_POISON
#define O_POISON(ptr, len) memset(ptr, OBJCACHE_POISON_PATTERN, len)
#else
#define O_POISON(ptr, len) do { } while(0)
#endif

static struct _memchunk mc;
static struct semaphore memsem = SEMAPHORE_INIT(memsem, 1);

static void do_cache_init(struct _mempool *p, struct _objcache *o,
				const char *label, size_t obj_sz)
{
	o->o_sz = obj_sz;
	o->o_num = PAGE_SIZE / obj_sz;
	o->o_ptr = o->o_ptr_end = NULL;
	o->o_cur = NULL;
	INIT_LIST_HEAD(&o->o_partials);
	INIT_LIST_HEAD(&o->o_full);
	list_add_tail(&o->o_list, &p->p_caches);
	o->o_pool = p;
	o->o_label = label;

	//printk("objcache: new: %s/%s (%lu bytes)\n",
	//	p->p_label, o->o_label, o->o_sz);
}

__init void _memchunk_init(void)
{
	struct _memchunk *m = &mc;

	INIT_LIST_HEAD(&m->m_pools);
	INIT_LIST_HEAD(&m->m_gpool.p_caches);
	m->m_gpool.p_label = "_global";

	do_cache_init(&m->m_gpool, &m->m_self_cache, "_objcache",
			sizeof(struct _objcache));
	do_cache_init(&m->m_gpool, &m->m_pool_cache, "_mempool",
			sizeof(struct _mempool));
}

static struct slab_hdr *memchunk_get(mempool_t p)
{
	struct slab_hdr *c;
	struct page *page;
	void *pptr;

	pptr = alloc_page();
	/* FIXME: search reserved pool */
	BUG_ON(NULL == pptr);

	page = virt_to_page(pptr);
	BUG_ON(page->count != 1);
	c = &page->u.slab_hdr;
	page->type = PG_slab;

	c->c_r.ptr = pptr;

	return c;
}

static void memchunk_put(mempool_t p, struct slab_hdr *hdr)
{
	struct page *page;

	/* FIXME: return page to reserved pool if it's depelted */
	if ( p->p_num_reserve < p->p_reserve ) {
	}
	page = container_of(hdr, struct page, u.slab_hdr);
	free_page(page_address(page));
}

#if 0
mempool_t mempool_new(const char *label, size_t numchunks)
{
	struct _mempool *p;
	size_t n = numchunks;

	if ( 0 == numchunks )
		return NULL;

	p = objcache_alloc(&mc.m_pool_cache);
	if ( NULL == p )
		return NULL;

	INIT_LIST_HEAD(&p->p_caches);
	list_add_tail(&p->p_list, &mc.m_pools);
	p->p_reserve = numchunks;
	p->p_num_reserve = 0;
	p->p_label = label;
	for(n = 0; n < numchunks; n++) {
		struct slab_hdr *tmp;
		tmp = memchunk_get(&mc.m_gpool);
		BUG_ON(NULL == tmp);
		tmp->c_r.next = p->p_reserved;
		p->p_reserved = tmp;
	}

	return p;
}

void mempool_free(mempool_t p)
{
	struct _objcache *o, *tmp;
	struct slab_hdr *c, *nxt;

	printk("mempool: free: %s\n", p->p_label);
	list_for_each_entry_safe(o, tmp, &p->p_caches, o_list) {
		objcache_fini(o);
	}

	BUG_ON(p->p_num_reserve > p->p_reserve);
	for(c = p->p_reserved; c; c = nxt) {
		nxt = c->c_r.next;
		memchunk_put(&mc.m_gpool, c);
	}

	list_del(&p->p_list);
	objcache_free2(&mc.m_pool_cache, p);
}
#endif

objcache_t objcache_init(mempool_t pool, const char *label, size_t obj_sz)
{
	struct _objcache *o;

	BUG_ON(obj_sz > PAGE_SIZE);

	if ( 0 == obj_sz )
		return NULL;

	if ( obj_sz < sizeof(void *) )
		obj_sz = sizeof(void *);

	o = objcache_alloc(&mc.m_self_cache);
	if ( o == NULL )
		return NULL;

	if ( NULL == pool )
		pool = &mc.m_gpool;
	do_cache_init(pool, o, label, obj_sz);
	return o;
}

#if 0
void objcache_fini(objcache_t o)
{
	struct slab_hdr *c, *tmp;
	size_t total = 0, obj = 0;

	list_for_each_entry_safe(c, tmp, &o->o_full, c_o.list) {
		total++;
		obj += c->c_o.inuse;
		BUG_ON(c->c_o.inuse != o->o_num);
		list_del(&c->c_o.list);
		memchunk_put(o->o_pool, c);
	}

	list_for_each_entry_safe(c, tmp, &o->o_partials, c_o.list) {
		if ( o->o_cur == c )
			continue;
		total++;
		obj += c->c_o.inuse;
		BUG_ON(c->c_o.inuse >= o->o_num);
		list_del(&c->c_o.list);
		memchunk_put(o->o_pool, c);
	}

	if ( o->o_cur ) {
		total++;
		obj += o->o_cur->c_o.inuse;
		list_del(&o->o_cur->c_o.list);
		memchunk_put(o->o_pool, o->o_cur);
	}

	obj *= o->o_sz;
	total <<= PAGE_SHIFT;

	printk("objcache: free: %s: %luK total, %luK inuse\n",
		o->o_label, total >> 10, obj >> 10);
	objcache_free2(&mc.m_self_cache, o);

}
#endif

static void *alloc_from_partial(struct _objcache *o, struct slab_hdr *c)
{
	void *ret;
	ret = c->c_o.free_list;
	c->c_o.free_list = *(uint8_t **)ret;
	c->c_o.inuse++;
	if ( NULL == c->c_o.free_list && c != o->o_cur )
		list_move(&c->c_o.list, &o->o_full);
	O_POISON(ret, o->o_sz);
	return ret;
}

static void *alloc_fast(struct _objcache *o)
{
	void *ret;
	ret = o->o_ptr;
	o->o_ptr += o->o_sz;
	o->o_cur->c_o.inuse++;
	if ( unlikely(o->o_cur->c_o.inuse == o->o_num &&
		NULL == o->o_cur->c_o.free_list) ) {
		list_move(&o->o_cur->c_o.list, &o->o_full);
		o->o_cur = NULL;
	}
	O_POISON(ret, o->o_sz);
	return ret;
}

static void *alloc_slow(struct _objcache *o)
{
	struct slab_hdr *c;

	c = memchunk_get(o->o_pool);
	if ( NULL == c )
		return NULL;

	o->o_cur = c;
	o->o_ptr = c->c_r.ptr;
	o->o_ptr_end = o->o_ptr + o->o_sz * o->o_num;

	c->c_o.cache = o;
	c->c_o.free_list = NULL;
	c->c_o.inuse = 0;
	INIT_LIST_HEAD(&c->c_o.list);

	return alloc_fast(o);
}

static struct slab_hdr *first_partial(struct _objcache *o)
{
	if ( list_empty(&o->o_partials) )
		return NULL;
	return list_entry(o->o_partials.next, struct slab_hdr, c_o.list);
}

static void *do_alloc(struct _objcache *o)
{
	struct slab_hdr *c;
	void *ret;

	if ( NULL == o )
		return NULL;

	sem_P(&memsem);

	/* First check free list */
	if ( (c = first_partial(o)) && c->c_o.free_list ) {
		ret = alloc_from_partial(o, c);
		goto out;
	}

	/* Then check ptr/ptr_end */
	if ( likely(o->o_ptr + o->o_sz <= o->o_ptr_end) ) {
		ret = alloc_fast(o);
		goto out;
	}

	/* Finall resort to slow path */
	ret = alloc_slow(o);
out:
	sem_V(&memsem);
	return ret;
}

void *objcache_alloc(objcache_t o)
{
	return do_alloc(o);
}

void *objcache_alloc0(objcache_t o)
{
	void *ret;
	
	ret = do_alloc(o);
	if ( likely(ret != NULL) )
		memset(ret, 0, o->o_sz);

	return ret;
}

static void do_cache_free(struct _objcache *o, struct slab_hdr *c, void *obj)
{
#if OBJCACHE_DEBUG_FREE
	uint8_t **tmp;
#endif

	BUG_ON(0 == c->c_o.inuse);
	BUG_ON(c->c_o.inuse > o->o_num);
	BUG_ON((uint8_t *)obj >= o->o_ptr && (uint8_t *)obj <= o->o_ptr_end);

	sem_P(&memsem);

#if OBJCACHE_DEBUG_FREE
	for(tmp = (uint8_t **)c->c_o.free_list; tmp; tmp = (uint8_t **)*tmp)
		BUG_ON(tmp == obj);
#endif

	/* First add to partials if this is first free from chunk */
	if ( NULL == c->c_o.free_list ) {
		BUG_ON(c != o->o_cur && c->c_o.inuse != o->o_num);
		list_move(&c->c_o.list, &o->o_partials);
	}

	O_POISON(obj, o->o_sz);

	/* add object to free list */
	*(uint8_t **)obj = c->c_o.free_list;
	c->c_o.free_list = obj;

	/* decrement inuse and free the chunk if it's the last object */
	if ( 0 == --c->c_o.inuse ) {
		list_del(&c->c_o.list);
		if ( o->o_cur == c ) {
			o->o_ptr = o->o_ptr_end = NULL;
			o->o_cur = NULL;
		}
		memchunk_put(o->o_pool, c);
	}
	sem_V(&memsem);
}
#if OBJCACHE_DEBUG_FREE
static int slab_is_on(struct slab_hdr *hdr, struct list_head *list)
{
	struct slab_hdr *chk;

	list_for_each_entry(chk, list, c_o.list) {
		if ( hdr == chk )
			return 1;
	}

	return 0;
}
#endif

void objcache_free(void *obj)
{
	struct page *page;
	struct slab_hdr *c;

	if ( NULL == obj )
		return;

	page = virt_to_page(obj);
	c = &page->u.slab_hdr;

	BUG_ON(page->type != PG_slab);
	BUG_ON(page->count != 1);

#if OBJCACHE_DEBUG_FREE
	BUG_ON(c != c->c_o.cache->o_cur &&
		!slab_is_on(c, &c->c_o.cache->o_partials) &&
		!slab_is_on(c, &c->c_o.cache->o_full) );
#endif

	do_cache_free(c->c_o.cache, c, obj);
}

void objcache_free2(objcache_t o, void *obj)
{
	struct slab_hdr *c;
	struct page *page;

	if ( NULL == obj )
		return;

	page = virt_to_page(obj);
	c = &page->u.slab_hdr;

	BUG_ON(page->type != PG_slab);
	BUG_ON(page->count != 1);
	BUG_ON(c->c_o.cache != o);

#if OBJCACHE_DEBUG_FREE
	BUG_ON(c != c->c_o.cache->o_cur &&
		!slab_is_on(c, &c->c_o.cache->o_partials) &&
		!slab_is_on(c, &c->c_o.cache->o_full) );
#endif
	do_cache_free(c->c_o.cache, c, obj);
}
