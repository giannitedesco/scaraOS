#define __ASM__ 1

.globl int_null
int_null: iret

.globl _syscall
_syscall:
	pusha
	call syscall
	popa
	iret

#define DO_exc(nr) \
.globl _exc##nr; \
_exc##nr:\
	pushl $0 ;\
	pushl $##nr ;\
	pusha ;\
	call exc_handler ;\
	popa ;\
	addl $8,%esp ;\
	iret

#define DO_excf(nr) \
.globl _exc##nr; \
_exc##nr:\
	pushl $##nr ;\
	pusha ;\
	call exc_handler ;\
	popa ;\
	addl $4,%esp ;\
	iret

#define DO_irq(nr) \
.globl _irq##nr;\
_irq##nr:\
	pusha ;\
	pushl $##nr;\
	call irq_handler;\
	addl $4,%esp ;\
	popa ;\
	iret

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
