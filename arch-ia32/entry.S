#define __ASM__	1
#include <arch/multiboot.h>
#include <arch/mm.h>
.text

.globl	_start
_start:
	cli
	jmp	multiboot_entry


.align	4
multiboot_header:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

multiboot_entry:
	# Assumes __end is page aligned
	movl	__end, %esp
	addl	$(1<<PAGE_SHIFT), %esp

	# Reset EFLAGS
	pushl	$0
	popf

	# Parameters for multiboot_check
	pushl	%ebx	# mbi structure
	pushl	%eax	# magic

	# Fix up our pagetables so we can use linked addresses
	# within the first 4MB (assumes the whole kernel loads
	# within 4MB - should be OK for some time...)
	call	setup_initmem

	# Enable paging
	movl	%cr0,%edx
	orl	$0x80000000,%edx
	movl	%edx,%cr0

	# Initialise output to screen
	call	vga_preinit

	# Fixup stack pointer
	addl	$PAGE_OFFSET,%esp

	# Load in the GDT
	lgdt	loadgdt

	# Reload EIP
	ljmp	$__KERNEL_CS, $1f
	# Reload segment selectors
1:
	movl	$__KERNEL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	# Check multiboot structures
	call	multiboot_check
	addl	$0x8, %esp

	# Main part of kernel
	call	setup

hang:
	hlt
	jmp	hang
