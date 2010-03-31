#include <arch/idt.h>

.globl int_null
int_null: iret

#define DO_intr(name, c_call) \
.globl name;		\
name:			\
	pushl $0 ;	\
	pushl %ds;	\
	pushl %es;	\
	pushl %fs;	\
	pushl %gs;	\
	pusha;		\
	call c_call;	\
	popa;		\
	popl %gs;	\
	popl %fs;	\
	popl %es;	\
	popl %ds;	\
	addl $4,%esp ;	\
	iret;

#define DO_exc(nr) \
.globl _exc##nr; \
_exc##nr:\
	pushl $0 ;\
	pushl %ds;	\
	pushl %es;	\
	pushl %fs;	\
	pushl %gs;	\
	pusha ;\
	pushl $##nr ;\
	call exc_handler ;\
	addl $4,%esp ;\
	popa ;\
	popl %gs;	\
	popl %fs;	\
	popl %es;	\
	popl %ds;	\
	addl $4,%esp ;\
	iret

#define DO_excf(nr) \
.globl _exc##nr; \
_exc##nr:\
	pushl %ds;	\
	pushl %es;	\
	pushl %fs;	\
	pushl %gs;	\
	pusha ;\
	pushl $##nr ;\
	call exc_handler ;\
	addl $4,%esp ;\
	popa ;\
	popl %gs;	\
	popl %fs;	\
	popl %es;	\
	popl %ds;	\
	iret

#define DO_irq(nr) \
.globl _irq##nr;\
_irq##nr:\
	pushl %ds;	\
	pushl %es;	\
	pushl %fs;	\
	pushl %gs;	\
	pusha ;\
	pushl $##nr;\
	call irq_handler;\
	addl $4,%esp ;\
	popa ;\
	popl %gs;	\
	popl %fs;	\
	popl %es;	\
	popl %ds;	\
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
