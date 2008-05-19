#ifndef __ARCH_IA32_8259A__
#define __ARCH_IA32_8259A__

typedef struct pic {
	uint16_t	port;
	uint16_t	port_imr;
	uint8_t	imr;
	uint8_t	vector;
	struct pic 	*master;
	union {
		uint8_t	slave_pins;
		uint8_t	master_pin;
	}cascade;
}pic_t, *p_pic;

/* ICW1 */
#define ICW1_INITIALISE		(1<<4)
#define ICW1_LEVEL_TRIGGER	(1<<3)	/* ~edge triggered */
#define ICW1_32BIT_VECTORS	(1<<2)	/* ~64bit */
#define ICW1_SINGLE_PIC		(1<<1)
#define ICW1_ICW4		1
#define ICW1			(ICW1_INITIALISE | ICW1_ICW4)

/* ICW4 */
#define ICW4_NESTED	(1<<4)
#define ICW4_BUFFERED	(1<<3)
#define ICW4_MASTER	(1<<2)	/* Not relevent in non-buffered */
#define ICW4_AUTO_EOI	(1<<1)
#define ICW4_8086	1
#define ICW4		ICW4_8086

#define IVECTOR_M	(0x20)
#define IVECTOR_S	(0x28)

#define IVECTOR		IVECTOR_M

/* End Of Interrupt codes */
#define EOI_SPECIFIC	0x60	/* Specific EOI */
#define EOI		0x20	/* Then send this to slave */

/* PIC driver functions */
void pic_init(void);

/* PIC functions */
void pic_unmask(p_pic, uint8_t);
void pic_mask(p_pic, uint8_t);
void pic_sync_imr(p_pic);
void pic_eoi_cascade(p_pic, uint32_t);
void pic_eoi(p_pic);

/* PC/AT functions */
void irq_eoi(uint16_t irq);
void irq_on(uint16_t);
void irq_off(uint16_t);

#endif /* __ARCH_IA32_8259A__ */
