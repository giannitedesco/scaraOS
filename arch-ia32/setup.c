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
static const dt_entry_t __desc_aligned GDT[] = {
	{dummy:0},

	/* Kernel space */
	stnd_desc(0, 0xFFFFF, (D_CODE | D_READ  | D_BIG | D_BIG_LIM)),
	stnd_desc(0, 0xFFFFF, (D_DATA | D_WRITE | D_BIG | D_BIG_LIM)),

	/* User space */
	stnd_desc(0, 0xFFFFF, (D_CODE | D_READ  | D_BIG | D_BIG_LIM | D_DPL3)),
	stnd_desc(0, 0xFFFFF, (D_DATA | D_WRITE | D_BIG | D_BIG_LIM | D_DPL3)),
};
const struct gdtr loadgdt = { sizeof(GDT)-1, (uint32_t)GDT};

/* Bootup CPU info */
struct cpu_info cpu_bsp;

char *cmdline = NULL;

static const char * const cpuid_str[]={
	"fpu",  "vme",   "de",   "pse",
	"tse",  "msr2",  "pae",  "mce",
	"cx8",  "apic",  "_r1",  "sep",
	"mtrr", "pge",   "mca",  "cmov",
	"pat",  "pse36", "psn",  "clflush",
	"_r2",  "dts",   "acpi", "mmx",
	"fxsr", "sse",   "sse2", "ss",
	"htt",  "tm",    "ia64",  "pbe",
};

static void cpuid_init(void)
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
	void (*si)(void) = (void *)__pa((uint8_t *)ia32_setup_initmem);

	/* Am I booted by a Multiboot-compliant boot loader?  */
	if ( magic != MULTIBOOT_BOOTLOADER_MAGIC )
		return 1;

	if ( (mbi->flags & MBF_AOUT) && (mbi->flags & MBF_ELF) )
		return 2;

	if ( (mbi->flags & MBF_MEM) == 0 )
		return 3;

	(*si)();

	return 0;
}

static void do_initcalls(void)
{
	initcall_t *fptr;

	/* Initialise device drivers */
	for(fptr = &__initcall_start; fptr < &__initcall_end; fptr++) {
		initcall_t fn = *fptr;
		//printk("initcall fn *0x%x 0x%x\n", fptr, fn);
		fn();
	}

}

/* Init task - the job of this task is to initialise all
 * installed drivers, mount the root filesystem and
 * bootstrap the system */
_noreturn static void init_task(void *priv)
{
	/* Initialise kernel subsystems */
	blk_init();
	vfs_init();
	pci_init();

	do_initcalls();

	/* Mount the root filesystem etc.. */
	vfs_mount_root();
	printk("init_task: completed\n");

	for(;;) {
		mdelay(100);
		printk("A");
	}
}

_noreturn static void task2(void *priv)
{
	//*(uint32_t *)0xdeadbeef = 0xfeedface;
	for(;;) {
		mdelay(100);
		printk("B");
	}
}

/* Entry point in to the kernel proper */
void _asmlinkage setup(multiboot_info_t *mbi)
{
	/* print a pretty message */
	printk("ScaraOS v0.0.4 for IA-32\n");
	if ( mbi->flags & MBF_CMDLINE ) {
		printk("cmd: %s\n", __va(mbi->cmdline));
		cmdline = __va(mbi->cmdline);
	}

	/* Need this quite quickly */
	idt_init();

	/* Fire up the kernel memory allocators */
	if ( mbi->flags & MBF_MMAP ) {
		ia32_mm_init(__va(mbi->mmap), mbi->mmap_length);
	}else{
		/* this won't work */
		ia32_mm_init(NULL, 0);
	}

	/* Prints out CPU MHz */
	calibrate_delay_loop();

	/* Identify CPU features */
	cpuid_init();

	/* enable hardware interrupts */
	pic_init();

	/* setup the idle task */
	sched_init();

	/* Setup the init task */
	kernel_thread("[init]", init_task, NULL);
	kernel_thread("[cpuhog]", task2, NULL);

	/* start jiffie counter */
	pit_start_timer1();

	/* Finally, enable interupts and let it rip */
	sti();
	sched();
	idle_task_func();
}
