#ifndef __MM_HEADER_INCLUDED__
#define __MM_HEADER_INCLUDED__

#include <arch/mm.h>
#include <list.h>
#include <rbtree.h>

/*
 * struct page
 */

#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1UL)

static inline vaddr_t va_round_up(vaddr_t va)
{
	return (va + PAGE_MASK) & ~PAGE_MASK;
}

static inline vaddr_t va_round_down(vaddr_t va)
{
	return va & ~PAGE_MASK;
}

#define PROT_READ	(1 << 0)
#define PROT_EXEC	(1 << 2)
#define PROT_WRITE	(1 << 3)

struct vma {
	struct rb_node		vma_rbt;
	vaddr_t			vma_begin;
	vaddr_t			vma_end;
	unsigned		vma_prot;

	/* for file backed mappings */
	off_t			vma_off;
	struct inode		*vma_ino;
};

struct mem_ctx {
	struct rb_root		vmas;
	struct arch_ctx		arch;
	unsigned int		count;
};
struct mem_ctx *mem_ctx_new(void);
void mem_ctx_free(struct mem_ctx *ctx);
struct mem_ctx *get_kthread_ctx(void);

/* Manipulating a tasks memory mappings */
int setup_vma(struct mem_ctx *ctx, vaddr_t va, size_t len,
		unsigned prot, struct inode *ino, off_t off);
struct vma *lookup_vma(struct mem_ctx *ctx, vaddr_t va);

static inline struct mem_ctx *mem_ctx_get(struct mem_ctx *ctx)
{
	ctx->count++;
	return ctx;
}

static inline void mem_ctx_put(struct mem_ctx *ctx)
{
	BUG_ON(0 == ctx->count);
	if ( 0 == --ctx->count )
		mem_ctx_free(ctx);
}

/* Full chunks: nowhere, c_o poulated c_o.list is in objcache->o_full */
/* Partial chunks: c_o populated and c_o.list is in objcache->o_partials */
/* Free chunks: in page allocator system, see: kernel/buddy.c */
struct chunk_hdr {
	union {
		struct {
			struct chunk_hdr *next;
			uint8_t *ptr;
		}c_r;
		struct {
			struct _objcache *cache;
			uint8_t *free_list;
			unsigned int inuse;
			struct list_head list;
		}c_o;
	};
};

/* There is one of these structures for every page
 * frame in the system. */
struct page {
	union {
		struct list_head list;
		struct chunk_hdr chunk_hdr;
	}u;
	unsigned count;
	unsigned flags;
};

/* Page flags */
#define PG_reserved	(1<<0)
#define PG_slab		(1<<1)

/* Page reference counts */
#define get_page(p) ((p)->count++)
#define put_page(p) ((p)->count--)

/* Getting at struct page's */
#define page_address(page) __va( ((page)-pfa) << PAGE_SHIFT )
#define virt_to_page(kaddr) (pfa + (__pa(kaddr) >> PAGE_SHIFT))


/* 
 * Buddy system 
 */

#define MAX_ORDER 10U

extern struct page *pfa;
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

typedef struct _memchunk *memchunk_t;
typedef struct _mempool *mempool_t;
typedef struct _objcache *objcache_t;

void mm_init(void);
void _memchunk_init(void);
void _kmalloc_init(void);

_malloc mempool_t mempool_new(const char *label, size_t numchunks);
void mempool_free(mempool_t m);

_malloc objcache_t objcache_init(mempool_t p, const char *l, size_t obj_sz);
void objcache_fini(objcache_t o);
void *objcache_alloc(objcache_t o);
_malloc void *objcache_alloc0(objcache_t o);
void objcache_free(void *obj);
void objcache_free2(objcache_t o, void *obj);

_malloc void *kmalloc(size_t sz);
_malloc void *kmalloc0(size_t sz);
void kfree(void *);

#endif /* __MM_HEADER_INCLUDED__ */
