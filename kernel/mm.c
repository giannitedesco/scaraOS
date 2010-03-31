#include <kernel.h>
#include <mm.h>
#include <task.h>

static objcache_t memctx;
static struct mem_ctx *kthread_ctx;

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

	ctx->count = 1;
	return ctx;
}

void mem_ctx_free(struct mem_ctx *ctx)
{
	destroy_ctx(&ctx->arch);
	objcache_free2(memctx, ctx);
}

void mm_init(void)
{
	_memchunk_init();
	_kmalloc_init();

	memctx = objcache_init(NULL, "mem_ctx", sizeof(struct mem_ctx));
	BUG_ON(NULL == memctx);

	kthread_ctx = objcache_alloc(memctx);
	BUG_ON(NULL == kthread_ctx);

	kthread_ctx->count = 0;
	setup_kthread_ctx(&kthread_ctx->arch);
}
