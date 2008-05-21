/*
 * x86 specific setup code. This file bootstraps the kernel,
 * spawns the init task and then becomes the idle task
*/
#include <kernel.h>
#include <task.h>
#include <vfs.h>
#include <blk.h>
#include <mm.h>

#include <arch/descriptor.h>
#include <arch/vga.h>
#include <arch/io.h>
#include <arch/pc_keyb.h>
#include <arch/timer.h>
#include <arch/8259a.h>
#include <arch/idt.h>
#include <arch/pci.h>

#include <arch/multiboot.h>

/* Global descriptor table */
dt_entry_t __desc_aligned GDT[]={
	{dummy:0},

	/* Kernel space */
	stnd_desc(0, 0xFFFFF, (D_CODE | D_READ  | D_BIG | D_BIG_LIM)),
	stnd_desc(0, 0xFFFFF, (D_DATA | D_WRITE | D_BIG | D_BIG_LIM)),

	/* User space */
	stnd_desc(0, 0xFFFFF, (D_CODE | D_READ  | D_BIG | D_BIG_LIM | D_DPL3)),
	stnd_desc(0, 0xFFFFF, (D_DATA | D_WRITE | D_BIG | D_BIG_LIM | D_DPL3)),
};
struct gdtr loadgdt={ sizeof(GDT)-1, (uint32_t)GDT};

void idle_task(void);

/* Bootup CPU info */
struct cpu_info cpu_bsp;

char *cmdline;

char *cpuid_str[]={
	"fpu",  "vme",   "de",   "pse",
	"tse",  "msr2",  "pae",  "mce",
	"cx8",  "apic",  "_r1",  "sep",
	"mtrr", "pge",   "mca",  "cmov",
	"pat",  "pse36", "psn",  "clflush",
	"_r2",  "dts",   "acpi", "mmx",
	"fxsr", "sse",   "sse2", "ss",
	"htt",  "tm",    "ia64",  "pbe",
};

void cpuid_init(void)
{
	char desc[13];
	int eax,ebx,ecx,edx;
	int i, cpuid_level;

	/* Get cpuid level and manufacturer string */
	cpuid(0, &eax, &ebx, &ecx, &edx);
	cpuid_level=eax;
	memcpy(desc, &ebx, 4);
	memcpy(desc+4, &edx, 4);
	memcpy(desc+8, &ecx, 4);
	desc[12]=0;

	/* Get features */
	cpuid(1, &eax, &ebx, &ecx, &edx);
	__this_cpu->features=edx;

	printk("cpu: %u.%uMHz %s cpuid=%i ",
		((__this_cpu->loops_ms*10)/5000),
		((__this_cpu->loops_ms*10)/50)%100,
		desc, cpuid_level);

	for(i=0; i<32; i++) {
		if ( edx & (1<<i) ) printk("%s ", cpuid_str[i]);
	}
	printk("\n");
}

/* Identity map 4MB of memory, and also map it to PAGE_OFFSET */
/* TODO: Don't bother with pagetable if we have PSE */
void setup_initmem(void)
{
	pgd_t dir = (pgd_t)__pa(((void *)&__end)+PAGE_SIZE);
	pgt_t tbl = ((void *)dir)+PAGE_SIZE;
	int i;

	for (i=0; i < NR_PDE; i++)
		dir[i] = 0;
	for (i=0; i < NR_PTE; i++)
		tbl[i] = (i<<PAGE_SHIFT) | PTE_PRESENT | PTE_RW;

	/* Setup paging */
	dir[0] = (uint32_t)tbl | PDE_PRESENT | PDE_RW;
	dir[dir(PAGE_OFFSET)] = (uint32_t)tbl | PDE_PRESENT | PDE_RW;
	load_pdbr(dir);
}

/* Checks the multiboot structures and take whatever info we need */
void multiboot_check(uint32_t magic, uint32_t addr)
{
	multiboot_info_t *mbi = (multiboot_info_t *)addr;

	/* Am I booted by a Multiboot-compliant boot loader?  */
	if ( magic != MULTIBOOT_BOOTLOADER_MAGIC ) {
		printk("PANIC: Invalid magic number: 0x%x\n", (unsigned) magic);
		idle_task();
	}

	if ( (mbi->flags & MBF_AOUT) && (mbi->flags & MBF_ELF) ) {
		printk("PANIC: Kernel can't be ELF AND AOUT!\n");
		idle_task();
	}

	if ( (mbi->flags & MBF_MEM) == 0 ) {
		printk("PANIC: ScaraOS relies on bootloader counting memory\n");
		idle_task();
	}

	if ( mbi->flags & MBF_CMDLINE ) {
		cmdline = __va(mbi->cmdline);
	}

	/* BIOS memory map */
	if ( mbi->flags & MBF_MMAP ) {
		p_memory_map addr = (p_memory_map)mbi->mmap;
		while((void *)addr < (void *)(mbi->mmap + mbi->mmap_length)) {
			printk("MEMMAP: 0x%x -> 0x%x (0x%x)\n",
				addr->base_addr_low,
				addr->base_addr_low + addr->length_low,
				addr->type);
			addr = (p_memory_map)
				((char *)addr + addr->size +
					sizeof (addr->size));
		}
	}

	mem_lo = mbi->mem_lower;
	mem_hi = mbi->mem_upper;
}

/* Init task - the job of this task is to initialise all
 * installed drivers, mount the root filesystem and
 * bootstrap the system */
void init_task(void)
{
	initcall_t *fptr;

	/* Initialise kernel subsystems */
	blk_init();
	vfs_init();
	pci_init();

	/* Initialise device drivers */
	for(fptr=(initcall_t *)&__initcall_start;
		fptr<(initcall_t *)&__initcall_end; fptr++) {
		initcall_t fn=(initcall_t)*fptr;
		fn();
	}

	/* Mount the root filesystem etc.. */
	vfs_mount_root();

	//idle_task();
	for(;;) {
		udelay(100);
		printk("A");
	}
}

static void task2(void)
{
	for(;;) {
		udelay(100);
		printk("B");
	}
}

/* Entry point in to the kernel proper */
void setup(void)
{
	struct task *i;

	/* Need this quite quickly */
	idt_init();

	/* Prints out CPU MHz */
	calibrate_delay_loop();

	/* print a pretty message */
	printk("ScaraOS v0.0.3 for IA-32\n");
	if ( cmdline )
		printk("cmd: %s\n", cmdline);

	/* Identify CPU features */
	cpuid_init();

	/* Fire up the kernel memory allocators */
	mm_init();

	/* enable hardware interrupts */
	pic_init();

	/* start jiffie counter */
	pit_start_timer1();

	/* setup the idle task */
	sched_init();

	/* Setup the init task */
	i=alloc_page();
	i->pid=1;
	i->t.eip=(uint32_t)init_task;
	i->t.esp=(uint32_t)i;
	i->t.esp+=PAGE_SIZE;
	i->preempt=1;
	task_to_runq(i);
	sti();
	sched();

	/* Finally, enable interupts */
	printk("starting idle task...\n");
}

void idle_task(void)
{
	asm volatile(
		"idle:\n"
		"hlt;\n"
		"rep; nop\n"
		"jmp idle\n");
}
