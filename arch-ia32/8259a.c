/*
 * This handles the low level specifics of programming the
 * intel 8259A programmable interrupt controller, of which, in
 * a PC there are two.
*/
#include <scaraOS/kernel.h>
#include <arch/8259a.h>
#include <arch/io.h>

#define NUM_PINS 8

static pic_t pics[]={
	{0x20, 0x21, 0xFF, IVECTOR_M, NULL, {1<<2}}, /* Master PIC in PC/AT */
	{0xA0, 0xA1, 0xFF, IVECTOR_S, NULL, {2}}, /* Slave PIC in PC/AT */
};

void pic_init(void)
{
	unsigned int i;
	long flags;

	/* It would be very bad to recieve an interrupt
	 * during programming the PIC */
	lock_irq(flags);

	pics[1].master = &pics[0];

	/* Initialise PICs */
	for (i = 0; i < (sizeof(pics)/sizeof(*pics)); i++) {
		outb(pics[i].port, ICW1);
		outb(pics[i].port_imr, pics[i].vector);

		if ( pics[i].master ) {
			outb(pics[i].port_imr, pics[i].cascade.master_pin);
		}else{
			outb(pics[i].port_imr, pics[i].cascade.slave_pins);
		}
		
		outb(pics[i].port_imr, ICW4);
		outb(pics[i].port_imr, pics[i].imr);
	}

	unlock_irq(flags);
}

static void pic_unmask(p_pic pic, uint8_t bit)
{
	pic->imr &= ~(1<<bit);
	if ( pic->master )
		pic_unmask(pic->master, pic->cascade.master_pin);
}

static void pic_mask(p_pic pic, uint8_t bit)
{
	pic->imr |= 1<<bit;
	if ( pic->master )
		pic_mask(pic->master, pic->cascade.master_pin);
}

static void pic_sync_imr(p_pic pic)
{
	outb(pic->port_imr, pic->imr);
	if ( pic->master )
		pic_sync_imr(pic->master);
}

static void pic_eoi_cascade(p_pic pic, uint32_t pin)
{
	if ( pic->master )
		pic_eoi_cascade(pic->master, pic->cascade.master_pin);
	outb(pic->port, EOI_SPECIFIC+pin);
}

static void pic_eoi(p_pic pic)
{
	if ( pic->master )
		pic_eoi_cascade(pic->master, pic->cascade.master_pin);
	outb(pic->port, EOI);
}

/* ==========================
*   PC/AT Specific commands 
*  ========================== */
void irq_on(uint16_t irq)
{
	long flags;
	lock_irq(flags);
	if ( irq < NUM_PINS ) {
		pic_unmask(&pics[0], irq);
		pic_sync_imr(&pics[0]);
	}else{
		pic_unmask(&pics[1], irq - NUM_PINS);
		pic_sync_imr(&pics[1]);
	}
	unlock_irq(flags);
}

void irq_off(uint16_t irq)
{
	long flags;
	lock_irq(flags);
	if ( irq < NUM_PINS ) {
		pic_mask(&pics[0], irq);
		pic_sync_imr(&pics[0]);
	}else{
		pic_mask(&pics[1], irq - NUM_PINS);
		pic_sync_imr(&pics[1]);
	}
	unlock_irq(flags);
}

void irq_eoi(uint16_t irq)
{
	long flags;
	lock_irq(flags);
	if ( irq < NUM_PINS ) {
		pic_eoi(&pics[0]);
	}else{
		pic_eoi(&pics[1]);
	}
	unlock_irq(flags);
}
