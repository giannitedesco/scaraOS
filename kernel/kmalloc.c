/* 
 * This is based on the Solaris slab allocator but is more optimised
 * for small objects, we can do 4byte objects with zero overhead.We
 * basically only store lists in un-allocated slab space.
 * 
 * TODO
 *  o Order slabs (full,partial,empty)
 *  o Slabs > PAGE_SIZE
 *  o Cache shrinking / reaping
 *  o Colouring / Alignment requirements
 *  o Optimise down to 1 byte objects?
*/
#include <kernel.h>
#include <mm.h>

#define NR_CACHE 10
#define CACHE_START 8

/* General purpose caches */
static struct m_cache general_cache[NR_CACHE];
static char general_names[12][NR_CACHE]; /* 12 chars gets us "size-123456" */

/* Special purpose caches needed for the allocator itself */
static struct m_cache cache_cache={
	.name = "m_cache",
	.size = sizeof(struct m_cache)
};
static struct m_cache slab_cache={
	.name = "m_slab",
	.size = sizeof(struct m_slab)
};

static struct m_cache *caches=NULL;

/* Add a cache */
int kmem_add_cache(struct m_cache *c)
{
	if ( !c->name || !c->size )
		return -1;

	if ( c->size > PAGE_SIZE )
		return -1;

	c->num_obj  = PAGE_SIZE / c->size;
	c->order = 0;
	c->slab = NULL;

	if ( c == &slab_cache )
		c->num_obj--;

	c->next = NULL;
	caches = c;

	return 0;
}

/* Initialise a new slab */
static struct m_slab *kmem_slab_init(struct m_cache *c)
{
	void **pptr;
	struct page *p;
	unsigned int i, internal=0;
	void *mem;
	struct m_slab *s=NULL;

	/* Allocate slab descriptor */
	if ( c == &slab_cache ) {
		internal = 1;
	}else{
		s = kmem_alloc(&slab_cache);
	}

	/* Allocate the slab */
	if ( !(mem = alloc_pages(c->order)) ) {
		if ( !internal )
			kmem_free(s);
		return NULL;
	}

	/* If we have an internal slab descriptor
	 * then allocate that here */
	if ( internal ) {
		pptr = (void **)(mem + sizeof(*s));
		s = mem;
	}else{
		pptr = (void **)mem;
	}


	/* Initialise the free list */
	s->obj=pptr;
	for(i=0; i<c->num_obj-1; i++) {
		*pptr=(void *)(((void *)pptr)+c->size);
		pptr=*pptr;
	}
	*pptr = NULL;

	/* Fill in the struct page entries */
	p=virt_to_page(mem);
	for(i = 0; i < (1U << c->order); i++) {
		p->prev = (struct page *)c;
		p->next = (struct page *)s;
		p->flags |= PG_slab;
		p++;
	}

	return s;
}

void kmem_free(void *ptr)
{
	struct m_cache *c;
	struct m_slab *s;
	struct page *p = virt_to_page(ptr);
	void **pptr;

	if ( !(p->flags & PG_slab) ) {
		printk("kmem: 0x%x not a slab page!\n", page_address(p));
		return;
	}

	c = page_cache(p);
	s = page_slab(p);

#ifdef DEBUG
	printk("- %s:%x\n", c->name, ptr);
#endif

	pptr = (void **)ptr;
	*pptr = s->obj;
	s->obj = pptr;
}

/* Allocate an object from a cache */
void *kmem_alloc(struct m_cache *c)
{
	struct m_slab *n;
	void *ret;

	for(n = c->slab; n; n = n->next) {
		if ( n->obj )
			break;
	}

	/* Allocate a new slab */
	if ( !n ) {
		if ( !(n = kmem_slab_init(c)) ) {
			return NULL;
		}

		/* Link it in */
		n->next = c->slab;
		c->slab = n;
	}

	ret = (void *)n->obj;
	n->obj = *n->obj;

#ifdef DEBUG
	printk("+ %s:%x\n", c->name, ret);
#endif

	return ret;
}

void __init kmalloc_init(void)
{
	size_t s = CACHE_START;
	int i;

	/* Special-purpose caches needed for
	 * the allocator itself */
	kmem_add_cache(&cache_cache);
	kmem_add_cache(&slab_cache);

	/* geometrically distributed general caches */
	for(i=0; i<NR_CACHE; i++,s<<=1) {
		snprintf(general_names[i],
			sizeof(general_names[i]),
			"size-%lu", s);
		general_cache[i].name=general_names[i];
		general_cache[i].size=s;
		general_cache[i].num_obj=PAGE_SIZE/s;
		general_cache[i].order=0;
		general_cache[i].slab=NULL;
		kmem_add_cache(&general_cache[i]);
	}
}

/* General purpose allocator */
void *kmalloc(size_t sz)
{
	size_t s=CACHE_START;
	int i;

	for(i = 0; i < NR_CACHE; i++, s<<=1) {
		if ( s >= sz )
			return kmem_alloc(&general_cache[i]);
	}

	printk("kmalloc: (%lu) allocating too big an object\n", sz);
	return NULL;
}
