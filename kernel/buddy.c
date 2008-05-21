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

struct buddy free_list[MAX_ORDER];

void __init buddy_init(void)
{
	int i;

	for(i=0; i<MAX_ORDER; i++) {
		free_list[i].next = (struct page *)&free_list[i];
		free_list[i].prev = (struct page *)&free_list[i];
	}
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
	page->next = free_list[order].next;
	page->prev = (struct page *)&free_list[order];
	free_list[order].next->prev = page;
	free_list[order].next = page;

	/* TODO: Coalesce with buddy */

	nr_freepages++;
	put_page(page);
}

/* allocate 2^order physically contigous pages */
void *alloc_pages(unsigned int order)
{
	struct page *page;

	if ( order )
		goto fail;
	if ( (1 << order) > nr_freepages )
		goto fail;

	page = free_list[order].next;
	if ( page == NULL )
		goto fail;

	/* Remove from list */
	free_list[order].next = page->next;
	free_list[order].prev = page->prev;

	nr_freepages--;
	get_page(page);
	return page_address(page);
fail:
	printk("%i order allocation failed\n", order);
	return NULL;
}
