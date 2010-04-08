#ifndef __IRQ_TABLE_INCLUDED__
#define __IRQ_TABLE_INCLUDED__

#ifndef __ASM__

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

/* UP / IRQ locking primitive */
#define lock_irq(x) asm volatile("pushfl; popl %0; cli":"=g" (x): : "memory")
#define unlock_irq(x) asm volatile("pushl %0; popfl": :"g" (x): "memory", "cc")

typedef void (*irqfn)(int irq);
void set_irq_handler(int, irqfn);

void _irq0(void);
void _irq1(void);
void _irq2(void);
void _irq3(void);
void _irq4(void);
void _irq5(void);
void _irq6(void);
void _irq7(void);
void _irq8(void);
void _irq9(void);
void _irq10(void);
void _irq11(void);
void _irq12(void);
void _irq13(void);
void _irq14(void);
void _irq15(void);
#endif

#endif /* __IRQ_TABLE_INCLUDED__ */
