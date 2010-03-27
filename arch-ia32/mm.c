/*
 * x86 specific setup code. This file bootstraps the kernel,
 * spawns the init task and then becomes the idle task
*/
#define DEBUG_MODULE 1
#include <kernel.h>
#include <mm.h>
#include <arch/multiboot.h>

/* For bootmem allocator */
static void *bootmem_begin;
static void *bootmem_end;
static void *bootmem_ptr;
static pgd_t kernel_pgdir;

/* lower and upper memory */
uint32_t mem_lo, mem_hi;

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
/* TODO: Don't bother with pagetable if we have PSE */
void ia32_setup_initmem(void)
{
	unsigned int i;
	uint8_t *pgd_ptr;
	uint8_t *pgt_ptr;
	pgd_t dir;
	pgt_t tbl;

	pgd_ptr = &__end + (1 << PAGE_SHIFT);
	pgt_ptr = &__end + (2 << PAGE_SHIFT);

	/* Assume space for initial page table and page directory */
	dir = (pgd_t)__pa(pgd_ptr);
	tbl = (pgt_t)__pa(pgt_ptr);

	for (i=0; i < NR_PDE; i++)
		dir[i] = 0;

	for (i=0; i < NR_PTE; i++)
		tbl[i] = (i << PAGE_SHIFT) | PTE_PRESENT | PTE_RW;

	/* 0-4MB identity mapped */
	dir[0] = (uint32_t)tbl | PDE_PRESENT | PDE_RW;

	/* PAGE_OFFSET + 4MB mapped */
	dir[dir(PAGE_OFFSET)] = (uint32_t)tbl | PDE_PRESENT | PDE_RW;

	/* Load CR3 */
	load_pdbr(dir);
}

/* Map in all present RAM */
static void map_ram(pgd_t pgdir, pgt_t pgtbl)
{
	unsigned long i;

	for(i=0; i < nr_physpages; i++) {
		pgtbl[i] = (i << PAGE_SHIFT) | PTE_PRESENT | PTE_RW;
		if ( (i & 0x3ff) == 0 ) {
			pgdir[dir(__va(i << PAGE_SHIFT))] =
				(uint32_t)__pa(&pgtbl[i]) |
					PDE_PRESENT | PDE_RW;
		}
	}

	__flush_tlb();
}

static void write_protect(pgt_t pgtbl, void *vptr, size_t len)
{
	uint32_t pa_begin, pa_end, i;

	pa_begin = __pa(vptr) & ~PAGE_MASK;
	pa_end = pa_begin + (len - 1);
	pa_end = (pa_end + PAGE_MASK) & ~PAGE_MASK;

	pa_begin >>= PAGE_SHIFT;
	pa_end >>= PAGE_SHIFT;

	printk("Write protecting %u pages @ 0x%x\n",
		(pa_end + 1) - pa_begin, vptr);
	for(i = pa_begin; i <= pa_end; i++)
		pgtbl[i] = (i << PAGE_SHIFT) | PDE_PRESENT;

	__flush_tlb();
}

static void print_e820(void *addr, size_t len)
{
	void *end = addr + len;

	printk("e820 map from BIOS (%u bytes at 0x%x)\n", len, addr);
	printk(" o kernel @ 0x%x - 0x%x (cmdline = 0x%x)\n",
		__pa(&__begin), __pa(&__end), __pa(cmdline));

	for(;;) {
		p_memory_map map = (p_memory_map)addr;
		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;

		dprintk(" o (%u) 0x%x -> 0x%x (0x%x) :",
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
		pfa[i].flags = PG_reserved;
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
	//printk("reserve_pages: %x - %x\n", pa_begin, pa_end);

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
			printk("e820 hole: %x - %x\n",
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

	bootmem_begin = (uint8_t *)&__end + (3 << PAGE_SHIFT);
	bootmem_end = bootmem_ptr = bootmem_begin;

	/* Calculate size of physical memory */
	//print_e820(e820_map, e820_len);
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
	get_pdbr(kernel_pgdir);
	kernel_pgdir = __va(kernel_pgdir);
	map_ram(kernel_pgdir, tbls);
	write_protect(tbls, &__begin, &__rodata_end - &__begin);
	printk("Kernel page tables = %u pages @ 0x%x\n", nr_pgtbls, tbls);

	/* Only use PAGE_OFFSET mapping, so zap identity map now */
	kernel_pgdir[0] = 0;
	__flush_tlb();

	buddy_init();

	pfa_size = nr_physpages * sizeof(struct page);
	if ( pfa_size & PAGE_MASK )
		pfa_size += PAGE_SIZE - (pfa_size & PAGE_MASK);

	pfa = bootmem_alloc(pfa_size >> PAGE_SHIFT);
	printk("Kernel page frame array %u entries/%u pages @ 0x%x\n",
		pfa_size, pfa_size >> PAGE_SHIFT, pfa);

	reserve_from_e820(e820_map, e820_len);

	bootmem_end = bootmem_ptr;
	reserve_pages(&__begin, (uint8_t *)bootmem_ptr - &__begin);
	for(i = 0; i < nr_physpages; i++) {
		struct page *p = &pfa[i];

		if ( p->count )
			continue;

		p->count = 0;
		p->flags = 0;
		free_pages(page_address(p), 0);
	}

	/* Print some stats */
	printk("mem: ram=%uMB %u/%u pageframes free (%u reserved)\n",
		tot_mem/(1 << 20), nr_freepages, nr_physpages, nr_reserved);

	kmalloc_init();
}
