/* 
 * Indended to be a buddy system allocator. Currently it
 * only allows 0 order allocations. That is to say, there
 * is no coalscing and not even any bitmaps in place
*/
#include <kernel.h>
#include <mm.h>

/* pageframe array */
struct page *pfa;

unsigned long nr_physpages;
unsigned long nr_freepages;

struct list_head free_list[MAX_ORDER];

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

	if ( !ptr )
		return;
	if ( order )
		return;

	page = virt_to_page(ptr);

	/* Stitch in to free area list */
	list_add_tail(&page->u.list, &free_list[order]);

	/* TODO: Coalesce with buddy */

	nr_freepages++;
	put_page(page);
}

/* allocate 2^order physically contigous pages */
void *alloc_pages(unsigned int order)
{
	struct page *page;

	if ( list_empty(&free_list[order]) )
		goto fail;

	page = free_list[order].next;
	if ( page == NULL )
		goto fail;

	/* Remove from list */
	list_del(&page->u.list);

	nr_freepages--;
	get_page(page);
	return page_address(page);
fail:
	printk("%i order allocation failed\n", order);
	return NULL;
}
