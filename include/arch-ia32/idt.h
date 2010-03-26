#ifndef __ARCH_IA32_IDT__
#define __ARCH_IA32_IDT__

struct ia32_exc_ctx {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;
	uint32_t exc_num;
	uint32_t err_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};

void idt_init(void);

void int_null(void);
void _syscall(void);

void exc_handler(struct ia32_exc_ctx ctx);
void syscall(void);

/* Used to modify the interrupt table */
void idt_exception(void *, uint8_t);
void idt_interrupt(void *, uint8_t);

/* Exception handlers  */
void _exc0(void);
void _exc1(void);
void _exc2(void);
void _exc3(void);
void _exc4(void);
void _exc5(void);
void _exc6(void);
void _exc7(void);
void _exc8(void);
void _exc9(void);
void _exc10(void);
void _exc11(void);
void _exc12(void);
void _exc13(void);
void _exc14(void);
/* exception 15 is reserved */
void _exc16(void);
void _exc17(void);
void _exc18(void);
void _exc19(void);

#endif /* __ARCH_IA32_IDT__ */
