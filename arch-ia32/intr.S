#include <arch/idt.h>
#include <arch/gdt.h>

.globl int_null
int_null: iret

#define DO_intr(name, c_call) \
.globl name;		\
name:			\
	pushl $0 ;	\
	pusha;		\
	xorl %eax,%eax; \
	movb $__KERNEL_DS, %al; \
	movl %eax, %ds;	\
	movl %eax, %es;	\
	call c_call;	\
	movl %eax, %ds;	\
	movl %eax, %es;	\
	popa;		\
	addl $4,%esp ;	\
	iret;

#define DO_exc(nr) \
.globl _exc##nr; \
_exc##nr:\
	pushl $0 ;\
	pusha ;\
	pushl $##nr ;\
	xorl %eax,%eax; \
	movb $__KERNEL_DS, %al; \
	movl %eax, %ds;	\
	movl %eax, %es;	\
	call exc_handler ;\
	movl %eax, %ds;	\
	movl %eax, %es;	\
	addl $4,%esp ;\
	popa ;\
	addl $4,%esp ;\
	iret

#define DO_excf(nr) \
.globl _exc##nr; \
_exc##nr:\
	pusha ;\
	pushl $##nr ;\
	xorl %eax,%eax; \
	movb $__KERNEL_DS, %al; \
	movl %eax, %ds;	\
	movl %eax, %es;	\
	call exc_handler ;\
	movl %eax, %ds;	\
	movl %eax, %es;	\
	addl $4,%esp ;\
	popa ;\
	addl $4,%esp ;\
	iret

#define DO_irq(nr) \
.globl _irq##nr;\
_irq##nr:\
	pushl $0 ;\
	pusha ;\
	pushl $##nr;\
	xorl %eax,%eax; \
	movb $__KERNEL_DS, %al; \
	movl %eax, %ds;	\
	movl %eax, %es;	\
	call irq_handler;\
	movl %eax, %ds;	\
	movl %eax, %es;	\
	addl $4,%esp ;\
	popa ;\
	addl $4,%esp ;\
	iret

DO_intr(_syscall, syscall_exc)
DO_intr(_panic, panic_exc)

DO_irq(0)
DO_irq(1)
DO_irq(2)
DO_irq(3)
DO_irq(4)
DO_irq(5)
DO_irq(6)
DO_irq(7)
DO_irq(8)
DO_irq(9)
DO_irq(10)
DO_irq(11)
DO_irq(12)
DO_irq(13)
DO_irq(14)
DO_irq(15)

DO_exc(0)
DO_exc(1)
DO_exc(2)
DO_exc(3)
DO_exc(4)
DO_exc(5)
DO_exc(6)
DO_exc(7)
DO_excf(8)
DO_exc(9)
DO_excf(10)

DO_excf(11)
DO_excf(12)
DO_excf(13)
DO_excf(14)
DO_exc(15)
DO_exc(16)
DO_excf(17)
DO_exc(18)
DO_exc(19)
