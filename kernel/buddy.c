/* 
 * Indended to be a buddy system allocator. Currently it
 * only allows 0 order allocations. That is to say, there
 * is no coalscing and not even any bitmaps in place
*/
#include <scaraOS/kernel.h>
#include <scaraOS/semaphore.h>
#include <scaraOS/mm.h>

#if PAGE_POISON
#define P_POISON(ptr, len) memset(ptr, PAGE_POISON_PATTERN, len)
#else
#define P_POISON(ptr, len) do { } while(0);
#endif

/* pageframe array */
struct page *pfa;

unsigned long nr_physpages;
unsigned long nr_freepages;

static struct list_head free_list[MAX_ORDER];
static struct semaphore pagesem = SEMAPHORE_INIT(pagesem, 1);

void __init buddy_init(void)
{
	unsigned int i;

	for(i = 0; i < MAX_ORDER; i++)
		INIT_LIST_HEAD(&free_list[i]);
}

/* Free 2^order pages */
void free_pages(void *ptr, unsigned int order)
{
	struct page *page;

	if ( NULL == ptr )
		return;
	BUG_ON(order != 1);

	P_POISON(ptr, order << PAGE_SHIFT);
	page = virt_to_page(ptr);
	page->type = PG_free;
	put_page(page);

	/* TODO: Coalesce with buddy */

	/* Stitch in to free area list */
	BUG_ON(page->count != 0);
	sem_P(&pagesem);
	list_add_tail(&page->u.list, &free_list[order]);
	sem_V(&pagesem);
	nr_freepages++;
}

/* allocate 2^order physically contigous pages */
void *alloc_pages(unsigned int order)
{
	struct page *page;

	BUG_ON(order != 1);
	sem_P(&pagesem);

	/* TODO: split larger block */
	if ( list_empty(&free_list[order]) )
		goto fail;

	page = list_entry(free_list[order].next, struct page, u.list);
	BUG_ON(page->count != 0);
	list_del(&page->u.list);
	sem_V(&pagesem);

	nr_freepages--;
	get_page(page);
	return page_address(page);
fail:
	sem_V(&pagesem);
	printk("%i order allocation failed\n", order);
	return NULL;
}
