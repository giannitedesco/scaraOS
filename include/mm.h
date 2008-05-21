#ifndef __MM_HEADER_INCLUDED__
#define __MM_HEADER_INCLUDED__

#include <arch/mm.h>

/*
 * struct page
 */

#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1UL)

/* There is one of these structures for every page
 * frame in the system. */
struct page {
	struct page *prev,*next;
	uint32_t count;
	uint32_t flags;
};

/* Members must be in the same order as struct page */
struct buddy {
	struct page *prev, *next;
};

/* Page flags */
#define PG_reserved	0x01U
#define PG_slab		0x02U

/* Page reference counts */
#define get_page(p) ((p)->count++)
#define put_page(p) ((p)->count--)

/* kmalloc accessors */
#define page_cache(p) ((struct m_cache *)((p)->prev))
#define page_slab(p) ((struct m_slab *)((p)->next))

/* Getting at struct page's */
#define page_address(page) __va( ((page)-pfa) << PAGE_SHIFT )
#define virt_to_page(kaddr) (pfa + (__pa(kaddr) >> PAGE_SHIFT))


/* 
 * Buddy system 
 */

#define MAX_ORDER 10U

extern struct page *pfa;
extern uint32_t mem_lo, mem_hi;
extern unsigned long nr_physpages;
extern unsigned long nr_freepages;
extern char *cmdline;
//extern unsigned long nr_physpages;
//extern unsigned long nr_freepages;

#define alloc_page() alloc_pages(0)
#define free_page(x) free_pages(x,0)

void buddy_init(void);
void *alloc_pages(unsigned int order);
void free_pages(void *ptr, unsigned int order);


/*
 * Kernel memory allocator
 */

struct m_slab {
	struct m_slab *next;
	void **obj; /* next free object */
};

struct m_cache {
	struct m_cache *next;
	char *name; /* cache name */
	size_t size; /* size of object */
	size_t num_obj; /* number of objects per slab */
	unsigned int order; /* order for get_free_pages() */
	struct m_slab *slab; /* slabs */
};

#define NR_AREA PAGE_SHIFT-3

int kmem_add_cache(struct m_cache *);
void *kmem_alloc(struct m_cache *);
void kmem_free(void *);
void kmalloc_init(void);
void *kmalloc(size_t sz);
#define kfree(x) kmem_free(x)

#endif /* __MM_HEADER_INCLUDED__ */
