##############################
#ASSEMBLY FUNCTIONS
##############################

#define _C_SYM( sym ) .globl sym ; sym
#define prologue pushl %ebp; movl %esp, %ebp
#define epilogue leave; ret

##############################
# INPUT I/O
##############################
_C_SYM(inb):
	prologue
	movl	8(%ebp), %edx	# arg1: Port
	xorl	%eax, %eax
	inb	%dx, %al
	epilogue

_C_SYM(inb_p):
	prologue
	movl	8(%ebp), %edx	# arg1: Port
	xorl	%eax, %eax
	inb	%dx, %al
	outb	%al, $0x80	# Pause
	epilogue

_C_SYM(inw):
	prologue
	movl	8(%ebp), %edx	# arg1: Port
	xorl	%eax, %eax
	inw	%dx, %ax
	epilogue

_C_SYM(inl):
	prologue
	movl	8(%ebp), %edx	# arg1: Port
	xorl	%eax, %eax
	inl	%dx, %eax
	epilogue

##############################
# OUTPUT I/O
##############################
_C_SYM(outb):
	prologue
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	outb	%al, %dx
	epilogue

_C_SYM(outb_p):
	prologue
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	outb	%al, %dx
	outb	%al, $0x80	# Pause
	epilogue

_C_SYM(outw):
	prologue
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	outw	%ax, %dx
	epilogue

_C_SYM(outl):
	prologue
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	outl	%eax, %dx
	epilogue
