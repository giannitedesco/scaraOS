/*
 * x86 specific setup code. This file bootstraps the kernel,
 * spawns the init task and then becomes the idle task
*/
#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/blk.h>
#include <scaraOS/mm.h>

#include <arch/vga.h>
#include <arch/io.h>
#include <arch/pc_keyb.h>
#include <arch/timer.h>
#include <arch/8259a.h>
#include <arch/idt.h>
#include <arch/pci.h>
#include <arch/multiboot.h>
#include <arch/regs.h>
#include <arch/syscalls.h>

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
	cpuid_level = eax;
	memcpy(desc, &ebx, 4);
	memcpy(desc + 4, &edx, 4);
	memcpy(desc + 8, &ecx, 4);
	desc[12] = 0;

	/* Get features */
	cpuid(1, &eax, &ebx, &ecx, &edx);
	__this_cpu->features = edx;

	printk("cpu: %lu.%luMHz %s cpuid=%i ",
		((__this_cpu->loops_ms * 10) / 5000),
		((__this_cpu->loops_ms * 10) / 50) % 100,
		desc, cpuid_level);

	for(i = 0; i < 32; i++) {
		if ( edx & (1 << i) )
			printk("%s ", cpuid_str[i]);
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

#if MULTI_TASKING_DEMO
static int dumb_task(void *priv)
{
	unsigned int i;

	for(i = 0; i < 100; i++) {
		mdelay(100);
		printk("%s", (char *)priv);
	}

	syscall1(_SYS_exec, (uint32_t)"/sbin/init");
	return 123;
}
#endif

/* Init task - the job of this task is to initialise all
 * installed drivers, mount the root filesystem and
 * bootstrap the system */
static int init_task(void *priv)
{
	uint32_t ret;

	/* Initialise kernel subsystems */
	blk_init();
	vfs_init();
	pci_init();

	do_initcalls();

	/* Mount the root filesystem etc.. */
	if ( vfs_mount_root("ext2", "floppy0") ) {
		panic("Unable to mount root filesystem\n");
	}

	/* pre-emptive multi-tasking demo */
#if MULTI_TASKING_DEMO
	kernel_thread("[cpuhog-A]", dumb_task, "A");
	kernel_thread("[cpuhog-B]", dumb_task, "B");
#endif


	ret = syscall1(_SYS_exec, (uint32_t)"/sbin/bash");
	ret = syscall1(_SYS_exec, (uint32_t)"/sbin/init");
	printk("exec: /sbin/init: %i\n", (int)ret);

	return ret;
}

/* Entry point in to the kernel proper */
_noreturn _asmlinkage void setup(multiboot_info_t *mbi)
{
	vga_preinit();

	/* Need this mmediately to catch exceptions */
	idt_init();

	/* print a pretty message */
	printk("ScaraOS v0.0.5 for IA-32");
	if ( mbi->flags & MBF_CMDLINE ) {
		cmdline = __va(mbi->cmdline);
		printk(": %s", cmdline);
	}
	printk("\n");

	/* Fire up the kernel memory allocators */
	BUG_ON((mbi->flags & MBF_MMAP) == 0);
	ia32_mm_init(__va(mbi->mmap), mbi->mmap_length);

	/* Prints out CPU MHz */
	calibrate_delay_loop();

	/* Identify CPU features */
	cpuid_init();

	/* enable hardware interrupts */
	pic_init();

	/* setup the idle task */
	sched_init();
	ia32_gdt_finalize();

	/* Setup the init task */
	kernel_thread("[init]", init_task, NULL);

	/* start jiffie counter */
	pit_start_timer1();

	/* Finally, enable interupts and let it rip */
	sti();
	sched();
	idle_task_func();
}
