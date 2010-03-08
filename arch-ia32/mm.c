/*
 * x86 specific setup code. This file bootstraps the kernel,
 * spawns the init task and then becomes the idle task
*/
#include <kernel.h>
#include <mm.h>
#include <arch/multiboot.h>

/* For bootmem allocator */
static void *bootmem_begin;
static void *bootmem_end;
static void *bootmem_ptr;

/* lower and upper memory */
uint32_t mem_lo, mem_hi;

/* Initial paging structures */
static _section(".init.pgalign") uint32_t kernel_pgdir[NR_PDE];
static _section(".init.pgalign") uint32_t kernel_bootmem[NR_PTE];

/* Identity map 4MB of memory, and also map it to PAGE_OFFSET */
/* TODO: Don't bother with pagetable if we have PSE */
void ia32_setup_initmem(void *ptr, size_t len)
{
	pgd_t dir = (pgd_t)__pa(kernel_pgdir);
	pgt_t tbl = (pgt_t)__pa(kernel_bootmem);
	unsigned int i;

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
static void map_ram(pgd_t pgdir, pgt_t pgtbl, unsigned int nr_pgtbl)
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

static uint32_t do_e820(void *addr, size_t len)
{
	void *end = addr + len;
	uint32_t tot = 0;

	printk("e820 map from BIOS (%u bytes at 0x%x)\n", len, addr);
	printk(" o kernel @ 0x%x - 0x%x (cmdline = 0x%x)\n",
		__pa(&__begin), __pa(&__end), __pa(cmdline));

	for(;;) {
		p_memory_map map;

		map = (p_memory_map)addr;

		if ( addr + sizeof(map->size) > end )
			break;
		if ( addr + map->size + sizeof(map->size) > end )
			break;

		printk(" o (%u) 0x%x -> 0x%x (0x%x) :",
			map->size + sizeof(map->size),
			map->base_addr_low,
			map->base_addr_low + map->length_low,
			map->type);

		if ( map->base_addr_low <= __pa(bootmem_begin) &&
			(map->base_addr_low + map->length_low) > 
				__pa(bootmem_begin) && map->type == MBF_MEM )
			bootmem_end = __va(map->base_addr_low + map->length_low);
		if ( map->type == 3 )
			tot = map->base_addr_low + map->length_low;

		if ( map->type & MBF_MEM )
			printk(" MEM");
		if ( map->type & MBF_BOOTDEV )
			printk(" BOOTDEV");
		if ( map->type & MBF_CMDLINE )
			printk(" CMDLINE");
		if ( map->type & MBF_MODULES )
			printk(" MODULES");
		if ( map->type & MBF_AOUT )
			printk(" AOUT");
		if ( map->type & MBF_ELF )
			printk(" ELF");
		if ( map->type & MBF_MMAP )
			printk(" MMAP");
		printk("\n");

		addr += map->size + sizeof(map->size);
	}

	return tot;
}

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

/* Initialise some very low-level memory management stuff such
 * as the page frame array. That is one struct page for each
 * page frame in the system
*/
void ia32_mm_init(void *e820_map, size_t e820_len)
{
	uint32_t tot_mem;
	uint32_t nr_pgtbls = 1;
	uint32_t pfa_size = 0;
	uint32_t nr_reserved = 0;
	unsigned int i;
	pgt_t tbls;

	bootmem_begin = (uint8_t *)&__end;
	bootmem_ptr = bootmem_begin;
	bootmem_end = bootmem_begin;

	/* Calculate size of physical memory */
	tot_mem = do_e820(e820_map, e820_len);
	nr_physpages = tot_mem >> PAGE_SHIFT;

	/* Work out how many pages to order */
	nr_pgtbls = tot_mem >> PDE_SHIFT;
	if ( nr_pgtbls == 0 )
		nr_pgtbls = 1;
	pfa_size = nr_physpages * sizeof(struct page);

	/* Round up to nearest page */
	if ( pfa_size & PAGE_MASK )
		pfa_size += PAGE_SIZE - (pfa_size & PAGE_MASK);

	/* Allocate 'em, order matters here */
	tbls = bootmem_alloc(nr_pgtbls);
	pfa = bootmem_alloc(pfa_size >> PAGE_SHIFT);

	printk("Kernel page tables = %u pages @ 0x%x\n", nr_pgtbls, tbls);
	printk("Kernel page frame array %u entries/%u pages @ 0x%x\n",
		pfa_size, pfa_size >> PAGE_SHIFT, pfa);

	/* Map in all physical memory */
	map_ram(kernel_pgdir, tbls, nr_pgtbls);

	/* Only use PAGE_OFFSET mapping, so zap identity map now */
	kernel_pgdir[0] = 0;
	__flush_tlb();

	buddy_init();

	for(i = 0; i < nr_reserved; i++) {
		struct page *p = &pfa[i];

		p->count = 1;
		p->flags = PG_reserved;
		nr_reserved++;
	}

	for(i = nr_reserved; i < nr_physpages; i++) {
		struct page *p = &pfa[i];

		p->count = 0;
		p->flags = 0;
		free_pages(page_address(p), 0);
	}

	/* Print some stats */
	printk("mem: ram=%uMB %u/%u pageframes free (%u reserved)\n",
		tot_mem/(1 << 20), nr_freepages, nr_physpages, nr_reserved);

	kmalloc_init();
}
