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

/* Checks the multiboot structures and take whatever info we need */
int _asmlinkage multiboot_check(uint32_t magic, multiboot_info_t *mbi)
{
	/* Am I booted by a Multiboot-compliant boot loader?  */
	if ( magic != MULTIBOOT_BOOTLOADER_MAGIC ) {
		printk("PANIC: Invalid magic number: 0x%x\n", (unsigned) magic);
		return 0;
	}

	if ( (mbi->flags & MBF_AOUT) && (mbi->flags & MBF_ELF) ) {
		printk("PANIC: Kernel can't be ELF AND AOUT!\n");
		return 0;
	}

	if ( (mbi->flags & MBF_MEM) == 0 ) {
		printk("PANIC: ScaraOS relies on bootloader counting memory\n");
		return 0;
	}

	if ( mbi->flags & MBF_CMDLINE ) {
		cmdline = __va(mbi->cmdline);
	}

	/* print a pretty message */
	printk("ScaraOS v0.0.3 for IA-32\n");
	if ( cmdline )
		printk("cmd: %s\n", cmdline);

	return 1;
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
void _asmlinkage setup(multiboot_info_t *mbi)
{
	struct task *i;

	/* Need this quite quickly */
	idt_init();

	/* Fire up the kernel memory allocators */
	if ( mbi->flags & MBF_MMAP ) {
		ia32_mm_init(mbi->mmap, mbi->mmap_length);
	}else{
		ia32_mm_init(NULL, 0);
	}

	/* Prints out CPU MHz */
	calibrate_delay_loop();

	/* Identify CPU features */
	cpuid_init();

	/* enable hardware interrupts */
	pic_init();

	/* start jiffie counter */
	pit_start_timer1();

	/* setup the idle task */
	sched_init();

	/* Finally, enable interupts */
	printk("starting idle task...\n");
	idle_task();

	/* Setup the init task */
	i = alloc_page();
	i->pid = 1;
	i->t.eip = (uint32_t)init_task;
	i->t.esp = (uint32_t)i;
	i->t.esp += PAGE_SIZE;
	i->preempt = 1;
	task_to_runq(i);
	sti();
	sched();
}

void idle_task(void)
{
	asm volatile(
		"1:\n"
		"rep; nop\n"
		"hlt;\n"
		"jmp 1b\n");
}
