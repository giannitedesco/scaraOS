#include <scaraOS/kernel.h>
#include <scaraOS/mm.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>

static objcache_t memctx;
static objcache_t vmas;
static struct mem_ctx *kthread_ctx;

#define rb_vma(x) rb_entry((x), struct vma, vma_rbt)

struct vma *lookup_vma(struct mem_ctx *ctx, vaddr_t va)
{
	struct rb_node *n;
	struct vma *vma;
	
	for(n = ctx->vmas.rb_node; n; ) {
		vma = rb_entry(n, struct vma, vma_rbt);
		if ( va < vma->vma_begin )
			n = n->rb_child[RB_LEFT];
		else if ( va > vma->vma_end )
			n = n->rb_child[RB_RIGHT];
		else
			return vma;
	}

	return NULL;
}

int mm_pagefault(struct task *tsk, vaddr_t va, unsigned prot)
{
	struct page *page;
	struct vma *vma;
	off_t off;

	dprintk("faulting in 0x%.8lx\n", va);
	vma = lookup_vma(tsk->ctx, va);
	if ( NULL == vma )
		return -1;

	if ( !(vma->vma_prot & prot) )
		return -1;

	off = vma->vma_off + ((va - vma->vma_begin) & ~PAGE_MASK);
	va &= ~PAGE_MASK;

	if ( vma->vma_ino ) {
		dprintk("...from pagecache\n");
		page = vma->vma_ino->i_iop->readpage(vma->vma_ino, off);
		if ( NULL == page )
			return -1;
	}else{
		void *ptr;

		dprintk("...anon map\n");
		ptr = alloc_page();
		if ( NULL == ptr )
			return -1;

		memset(ptr, 0, PAGE_SIZE);
		page = virt_to_page(ptr);
	}

	if ( map_page_to_ctx(&tsk->ctx->arch, page, va, prot) )
		return -1;

	return 0;
}

static void vma_insert(struct mem_ctx *ctx, struct vma *vma)
{
	struct rb_node **p;
	struct rb_node *parent;

	memset(&vma->vma_rbt, 0, sizeof(vma->vma_rbt));
	
	for(p = &ctx->vmas.rb_node, parent = NULL; *p; ) {
		struct vma *area;

		parent = *p;
		area = rb_entry(parent, struct vma, vma_rbt);
		if ( vma->vma_end <= area->vma_begin )
			p = &(*p)->rb_child[RB_LEFT];
		else if ( vma->vma_begin >= area->vma_end )
			p = &(*p)->rb_child[RB_RIGHT];
		else
			BUG_ON(vma);
	}

	rb_link_node(&vma->vma_rbt, parent, p);
	rb_insert_color(&vma->vma_rbt, &ctx->vmas);
}

int setup_vma(struct mem_ctx *ctx, vaddr_t va, size_t len, unsigned prot,
		struct inode *ino, off_t off)
{
	struct vma *vma;

	vma = objcache_alloc(vmas);
	if ( NULL == vma )
		return -1; /* ENOMEM */

	vma->vma_begin = va_round_down(va);
	vma->vma_end = va_round_up(va + len);
	vma->vma_prot = prot;

	if ( ino ) {
		vma->vma_ino = iref(ino);
		vma->vma_off = off - (va - vma->vma_begin);
		BUG_ON(vma->vma_off & PAGE_MASK);
	}else
		vma->vma_ino = NULL;

	dprintk("setup_vma: 0x%.8lx - 0x%.8lx (off=0x%lx)\n",
		vma->vma_begin, vma->vma_end, vma->vma_off);

	/* FIXME: kill overlapping vma's */

	vma_insert(ctx, vma);
	return 0;
}

static void do_free_vma(struct vma *vma)
{
	if ( NULL == &vma->vma_rbt )
		return;
	do_free_vma(rb_vma(vma->vma_rbt.rb_child[RB_LEFT]));
	do_free_vma(rb_vma(vma->vma_rbt.rb_child[RB_RIGHT]));
	objcache_free2(vmas, vma);
}

static int do_clone_vma(struct vma *orig, struct rb_node **pnew)
{
	struct vma *new;

	if ( NULL == &orig->vma_rbt )
		return 1;

	new = objcache_alloc(vmas);
	if ( NULL == new )
		return 0;

	memcpy(new, orig, sizeof(*new));
	memset(&new->vma_rbt, 0, sizeof(new->vma_rbt));

	if( new->vma_ino ) {
		new->vma_ino = iref(orig->vma_ino);
	}else{
		/* this is what mem nodes are for */
	}

	if ( !do_clone_vma(rb_vma(orig->vma_rbt.rb_child[RB_LEFT]),
			&new->vma_rbt.rb_child[RB_LEFT]) )
		return 0;
	if ( !do_clone_vma(rb_vma(orig->vma_rbt.rb_child[RB_RIGHT]),
			&new->vma_rbt.rb_child[RB_RIGHT]) )
		return 0;

	return 1;
}

struct mem_ctx *mem_ctx_clone(struct mem_ctx *ctx)
{
	struct mem_ctx *new;

	new = objcache_alloc(memctx);
	if ( NULL == new )
		return NULL;

	if ( setup_new_ctx(&new->arch) ) {
		objcache_free2(memctx, new);
		return NULL; /* ENOMEM */
	}

	new->vmas.rb_node = NULL;

	if ( !do_clone_vma(rb_vma(ctx->vmas.rb_node), &new->vmas.rb_node) ) {
		do_free_vma(rb_vma(ctx->vmas.rb_node));
		objcache_free2(memctx, new);
		return NULL; /* ENOMEM */
	}

	new->count = 1;
	return new;
}

struct mem_ctx *get_kthread_ctx(void)
{
	return mem_ctx_get(kthread_ctx);
}

struct mem_ctx *mem_ctx_new(void)
{
	struct mem_ctx *ctx;

	ctx = objcache_alloc(memctx);
	if ( NULL == ctx )
		return NULL;

	if ( setup_new_ctx(&ctx->arch) ) {
		objcache_free2(memctx, ctx);
		return NULL;
	}

	ctx->vmas.rb_node = NULL;
	ctx->count = 1;
	return ctx;
}

void mem_use_ctx(struct mem_ctx *ctx)
{
	use_ctx(&ctx->arch);
}

void mem_ctx_free(struct mem_ctx *ctx)
{
	do_free_vma(rb_vma(ctx->vmas.rb_node));
	destroy_ctx(&ctx->arch);
	objcache_free2(memctx, ctx);
}

__init void mm_init(void)
{
	_memchunk_init();
	_kmalloc_init();

	memctx = objcache_init(NULL, "mem_ctx", sizeof(struct mem_ctx));
	BUG_ON(NULL == memctx);

	kthread_ctx = objcache_alloc(memctx);
	BUG_ON(NULL == kthread_ctx);

	kthread_ctx->count = 0;
	kthread_ctx->vmas.rb_node = NULL;
	setup_kthread_ctx(&kthread_ctx->arch);

	vmas = objcache_init(NULL, "vma", sizeof(struct vma));
	BUG_ON(NULL == vmas);
}
