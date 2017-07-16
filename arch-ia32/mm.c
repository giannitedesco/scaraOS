/*
 * x86 specific setup code. This file bootstraps the kernel,
 * spawns the init task and then becomes the idle task
*/
#include <scaraOS/kernel.h>
#include <arch/mm.h>
#include <scaraOS/mm.h>

#include <arch/multiboot.h>
#include <arch/kimage.h>

/* For bootmem allocator */
static void *bootmem_begin;
static void *bootmem_end;
static void *bootmem_ptr;

/* Simple allocator for initial page tables and the PFA */
static void *bootmem_alloc(unsigned int pages)
{
	void *ret = bootmem_ptr;

	if ( (bootmem_ptr + (pages << PAGE_SHIFT)) > bootmem_end )
		return NULL;

	memset(bootmem_ptr, 0, pages << PAGE_SHIFT);
	bootmem_ptr += (pages << PAGE_SHIFT);
	return ret;
}

/* Identity map 4MB of memory, and also map it to PAGE_OFFSET */
void ia32_setup_initmem(void)
{
	unsigned int i;
	pgd_t dir;
	pgt_t tbl;

	dir = &__init_pgd_pa;
	tbl = &__init_pgt_pa;

	for (i = 0; i < NR_PDE; i++)
		dir[i] = 0;

	for (i = 0; i < NR_PTE; i++)
		tbl[i] = (i << PAGE_SHIFT) | PTE_PRESENT | PTE_RW | PTE_USER;

	/* 0-4MB identity mapped */
	dir[0] = (uint32_t)tbl | PDE_PRESENT | PDE_RW;

	/* PAGE_OFFSET + 4MB mapped */
	dir[dir(PAGE_OFFSET)] = (uint32_t)tbl | PDE_PRESENT | PDE_RW | PDE_USER;

	/* Load CR3 */
	load_pdbr(dir);
}

/* Map in all present RAM */
/* TODO: Don't bother with pagetable if we have PSE, set PSE bit in cr4 */
/* TODO: Use global page and set PGE bit in cr4 if possible */
static void map_ram(pgd_t pgdir, pgt_t pgtbl)
{
	unsigned long i;

	/* Only use PAGE_OFFSET mapping, so zap identity map now */
	pgdir[0] = 0;

	for(i = 0; i < nr_physpages; i++) {
		pgtbl[i] = (i << PAGE_SHIFT) | PTE_PRESENT | PTE_RW;
	}

	for(i = 0; i < nr_physpages; i++) {
		if ( (i & 0x3ff) )
			continue;
		pgdir[dir(__va(i << PAGE_SHIFT))] =
			(uint32_t)__pa(&pgtbl[i]) | PDE_PRESENT | PDE_RW;
	}

	__flush_tlb();
}

static void write_protect(pgt_t pgtbl, void *vptr, size_t len)
{
	uint32_t pa_begin, pa_end, i;

	pa_begin = __pa(vptr) & ~PAGE_MASK;
	pa_end = pa_begin + len;
	pa_end = (pa_end + PAGE_MASK) & ~PAGE_MASK;

	pa_begin >>= PAGE_SHIFT;
	pa_end >>= PAGE_SHIFT;

	printk("Write protecting %lu pages @ %p\n",
		(pa_end + 1) - pa_begin, vptr);
	for(i = pa_begin; i < pa_end; i++)
		pgtbl[i] = (i << PAGE_SHIFT) | PTE_PRESENT;

	__flush_tlb();
}

static void print_e820(void *addr, size_t len)
{
#if DEBUG_E820
	void *end = addr + len;

	printk("e820 map from BIOS (%lu bytes at %p)\n", len, addr);
	printk(" o kernel @ 0x%.8lx - 0x%.8lx (cmdline = 0x%.8lx)\n",
		__pa(&__begin), __pa(&__end), __pa(cmdline));

	for(;;) {
		p_memory_map map = (p_memory_map)addr;
		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;

		dprintk(" o (%lu) 0x%.8lx -> 0x%.8lx (0x%.8lx) :",
			map->size + sizeof(map->size),
			map->base_addr_low,
			map->base_addr_low + map->length_low,
			map->type);

		if ( map->base_addr_low > (~0 - PAGE_OFFSET) )
			dprintk(" UNMAPPABLE");
		if ( map->type & MBF_MEM )
			dprintk(" MEM");
		if ( map->type & MBF_BOOTDEV )
			dprintk(" BOOTDEV");
		if ( map->type & MBF_CMDLINE )
			dprintk(" CMDLINE");
		if ( map->type & MBF_MODULES )
			dprintk(" MODULES");
		if ( map->type & MBF_AOUT )
			dprintk(" AOUT");
		if ( map->type & MBF_ELF )
			dprintk(" ELF");
		if ( map->type & MBF_MMAP )
			dprintk(" MMAP");
		dprintk("\n");

		addr += map->size + sizeof(map->size);
	}
#endif
}

static uint32_t size_up_ram(void *addr, size_t len)
{
	void *end = addr + len;
	uint32_t tot = 0;

	for(;;) {
		p_memory_map map = (p_memory_map)addr;
		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;
		if ( map->base_addr_low > (~0 - PAGE_OFFSET) )
			break;
		if ( map->type | (MBF_MEM|MBF_BOOTDEV) )
			tot = map->base_addr_low + map->length_low;

		addr += map->size + sizeof(map->size);
	}

	return tot;
}

static void setup_max_bootmem(void *addr, size_t len)
{
	void *end = addr + len;
	uint32_t cur_end = __pa(bootmem_end);

	for(;;) {
		p_memory_map map = (p_memory_map)addr;
		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;
		if ( map->base_addr_low > (~0 - PAGE_OFFSET) )
			break;

		if ( cur_end < map->base_addr_low + map->length_low
				&& map->type == MBF_MEM ) {
			cur_end = map->base_addr_low + map->length_low;
		}

		addr += map->size + sizeof(map->size);
	}

	bootmem_end = (void *)__va(cur_end);
}

static uint32_t nr_reserved;
static void reserve_page_range(uint32_t first_page, uint32_t last_page)
{
	uint32_t i;

	for(i = first_page; i <= last_page; i++) {
		pfa[i].count = 1;
		pfa[i].type = PG_reserved;
		nr_reserved++;
	}
}

static void reserve_pages(void *vptr, size_t len)
{
	uint32_t pa_begin, pa_end;

	/* reserve page range is inclusive range so don't
	 * need vptr + len to point to byte after last byte
	 * to reserve */
	len--;

	pa_begin = __pa(vptr);
	pa_end = __pa((uint8_t *)vptr + len);

	pa_begin &= ~PAGE_MASK;
	pa_end = (pa_end + PAGE_MASK) & ~PAGE_MASK;
	//printk("reserve_pages: %.8lx - %.8lx\n", pa_begin, pa_end);

	reserve_page_range(pa_begin >> PAGE_SHIFT, pa_end >> PAGE_SHIFT);
}

static void reserve_from_e820(void *addr, size_t len)
{
	void *end = addr + len;
	uint32_t contig_end = 0;

	for(;;) {
		p_memory_map map = (p_memory_map)addr;
		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;
		if ( map->base_addr_low > (~0 - PAGE_OFFSET) )
			break;

		if ( map->base_addr_low != contig_end ) {
			printk("e820 hole: %.8lx - %.8lx\n",
				contig_end, map->base_addr_low);
			reserve_pages(__va(contig_end),
					map->base_addr_low - contig_end);
			contig_end = map->base_addr_low;
		}

		if ( map->type != MBF_MEM )
			reserve_pages(__va(map->base_addr_low),
					map->length_low);

		contig_end += map->length_low;

		addr += map->size + sizeof(map->size);
	}
}

/* Initialise some very low-level memory management stuff such
 * as the page frame array. That is one struct page for each
 * page frame in the system
*/
void ia32_mm_init(void *e820_map, size_t e820_len)
{
	uint32_t tot_mem;
	uint32_t nr_pgtbls = 1;
	uint32_t pfa_size = 0;
	unsigned int i;
	pgt_t tbls;

	bootmem_begin = &__bootmem_begin;
	bootmem_end = bootmem_ptr = bootmem_begin;

	/* Calculate size of physical memory */
	print_e820(e820_map, e820_len);
	tot_mem = size_up_ram(e820_map, e820_len);
	nr_physpages = tot_mem >> PAGE_SHIFT;

	/* Work out how many pages to order */
	nr_pgtbls = tot_mem >> PDE_SHIFT;
	if ( nr_pgtbls == 0 )
		nr_pgtbls = 1;

	/* Allocate 'em, order matters here */
	setup_max_bootmem(e820_map, e820_len);

	/* Map in all physical memory */
	/* FIXME: initial page table is leaked */
	tbls = bootmem_alloc(nr_pgtbls);
	BUG_ON(NULL == tbls);
	map_ram(&__init_pgd, tbls);
	dprintk("Kernel page tables = %lu pages @ %p\n", nr_pgtbls, tbls);
	write_protect(tbls, &__begin, &__rodata_end - &__begin);

	buddy_init();

	pfa_size = nr_physpages * sizeof(struct page);
	pfa_size = (pfa_size + PAGE_MASK) & ~PAGE_MASK;

	pfa = bootmem_alloc(pfa_size >> PAGE_SHIFT);
	BUG_ON(NULL == pfa);
	dprintk("Page frame array: %lu pages @ %p, struct page=%u bytes\n",
		pfa_size >> PAGE_SHIFT, pfa, sizeof(struct page));

	reserve_from_e820(e820_map, e820_len);

	bootmem_end = bootmem_ptr;
	reserve_pages(&__begin, (uint8_t *)bootmem_ptr - &__begin);
	for(i = 0; i < nr_physpages; i++) {
		struct page *p = &pfa[i];

		if ( p->count )
			continue;

		/* free_pages() will call put_page() */
		p->count = 1;
		free_page(page_address(p));
	}

	/* Print some stats */
	printk("mem: ram=%luMB %lu/%lu pageframes free (%lu reserved)\n",
		tot_mem/(1 << 20), nr_freepages, nr_physpages, nr_reserved);

	mm_init();
}

void setup_kthread_ctx(struct arch_ctx *ctx)
{
	ctx->pgd = (paddr_t)&__init_pgd_pa;
}

int setup_new_ctx(struct arch_ctx *ctx)
{
	pgd_t pgd, kpgd;
	unsigned int i;

	kpgd = &__init_pgd;
	pgd = alloc_page();
	if ( NULL == pgd )
		return -1;

	for (i = 0; i < NR_PDE; i++)
		pgd[i] = kpgd[i];

	ctx->pgd = __pa(pgd);
	return 0;
}

int clone_ctx(const struct arch_ctx *old, struct arch_ctx *new)
{
#if 1
	pgd_t old_pgd, pgd;
	unsigned int i;
	int ret = -1;

	pgd = alloc_page();
	if ( NULL == pgd )
		goto out;

	old_pgd = (void *)__va(old->pgd);
	for (i = 0; i < NR_PDE; i++)
		pgd[i] = old_pgd[i];

	new->pgd = __pa(pgd);
	ret = 0;
	goto out;

out_free:
	free_page(pgd);
out:
	return ret;
#else
	return setup_new_ctx(new);
#endif
}

void destroy_ctx(struct arch_ctx *ctx)
{
	/* FIXME: destroy all pagetables too */
	free_page(__va(ctx->pgd));
}

void use_ctx(struct arch_ctx *ctx)
{
	load_pdbr(ctx->pgd);
}

int map_page_to_ctx(struct arch_ctx *ctx, struct page *page,
			vaddr_t addr, unsigned prot)
{
	pgd_t pd;
	pgt_t pt;
	uint32_t pa;
	uint32_t tblent;

	BUG_ON(addr >= PAGE_OFFSET);

	pd = __va(ctx->pgd);

	if ( !(pd[dir(addr)] & PDE_PRESENT) ) {
		pt = alloc_page();
		if ( NULL == pt )
			return -1;

		memset(pt, 0, NR_PTE * sizeof(*pt));
		pd[dir(addr)] = __pa(pt) | PDE_PRESENT | PDE_RW | PDE_USER;
	}else{
		pt = __va(__pg_val(pd[dir(addr)]));
	}

	pa = page_phys(page);
	tblent = pa | PTE_PRESENT | PTE_USER;
	if ( prot & PROT_WRITE )
		tblent |= PTE_RW;

	pt[tbl(addr)] = tblent;

	/* FFS: use INVLPG */
	__flush_tlb();
	dprintk("ctx %.8lx: 0x%.8lx:page table %lu / %lu = 0x%.8lx\n",
		ctx->pgd, addr, dir(addr), tbl(addr), pa);
	return 0;
}
