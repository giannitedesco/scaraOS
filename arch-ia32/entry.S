#define __ASM__	1
#include <arch/multiboot.h>
#include <arch/mm.h>
.section .text.init
.text

.globl	_start
_start:
	jmp	multiboot_entry

.align	4
multiboot_header:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

multiboot_entry:
	/* Assumes __end is page aligned and safe to use! */
	movl $idle_task_struct, %esp
	addl (1 << PAGE_SHIFT), %esp
	subl $PAGE_OFFSET, %esp

	/* Reset EFLAGS */
	pushl	$0
	popf

	/* Parameters for multiboot_check */
	pushl	%ebx	/* mbi structure */
	pushl	%eax	/* magic */

	/* Check multiboot structures */
	call	multiboot_check
	test	$0x0, %eax
	jne	2f

	/* Our pagetables are no fixed up so we can use linked addresses
	 * within the first 4MB (assumes the whole kernel loads
	 * within 4MB - should be OK for some time...)
	 */

	/* Enable paging */
	movl	%cr0,%edx
	orl	$0x80000000,%edx
	movl	%edx,%cr0

	/* Fixup stack pointer */
	addl	$PAGE_OFFSET,%esp

	/* Load in the GDT */
	lgdt	loadgdt

	/* Reload EIP */
	ljmp	$__KERNEL_CS, $1f

	/* Reload segment selectors */
1:
	movl	$__KERNEL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	/* Initialise output to screen */
	call	vga_preinit

	/* Main part of kernel */
	popl	%eax
	popl	%ebx
	addl	$PAGE_OFFSET,%ebx
	pushl	%ebx
	call	setup

2:
	hlt
	jmp	2b
