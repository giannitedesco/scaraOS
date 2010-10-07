#include <scaraOS/kernel.h>
#include <scaraOS/vfs.h>
#include <scaraOS/mm.h>

struct page *pagecache_file_readpage(struct inode *in, off_t off)
{
	struct rb_node **p;
	struct rb_node *parent;
	struct page *new = NULL;
	void *ptr;
	ssize_t ret;

	BUG_ON(off & PAGE_MASK);

	sem_P(&in->i_sem);

	for(p = &in->i_pagecache.rb_node, parent = NULL; *p; ) {
		struct page *pg;

		parent = *p;
		pg = rb_entry(parent, struct page, u.pagecache.pc_rbt);
		if ( off < pg->u.pagecache.pc_off )
			p = &(*p)->rb_child[RB_LEFT];
		else if ( off > pg->u.pagecache.pc_off )
			p = &(*p)->rb_child[RB_RIGHT];
		else {
			dprintk("pagecache: soft fault\n");
			BUG_ON(pg->type != PG_pagecache);
			get_page(pg);
			sem_V(&in->i_sem);
			return pg;
		}
	}

	ptr = alloc_page();
	if ( NULL == ptr )
		goto out_unlock;
	
	ret = in->i_iop->pread(in, ptr, PAGE_SIZE, off);
	if ( ret <= 0 ) {
		free_page(ptr);
		goto out_unlock;
	}

	dprintk("pagecache: hard fault: %lu bytes read, %lu bytes to zero\n",
		(size_t)ret, PAGE_SIZE - (size_t)ret);
	memset((uint8_t *)ptr + (size_t)ret, 0, PAGE_SIZE - (size_t)ret);

	new = virt_to_page(ptr);
	new->type = PG_pagecache;

	new->u.pagecache.pc_off = off;
	rb_link_node(&new->u.pagecache.pc_rbt, parent, p);
	rb_insert_color(&new->u.pagecache.pc_rbt, &in->i_pagecache);
out_unlock:
	sem_V(&in->i_sem);
	return new;
}
