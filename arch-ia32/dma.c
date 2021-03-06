/*
 * Simple DMA controller code
*/
#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/dma.h>

static const struct dma{
	uint8_t page;
	uint8_t addr;
	uint8_t count;
	uint8_t mask;
	uint8_t mode;
	uint8_t clear;
}dma_chan[]={
	/* 8 bit DMA channels */
	{0x87,	0x00,	0x01,	0x0a,	0x0b,	0x0c},
	{0x83,	0x02,	0x03,	0x0a,	0x0b,	0x0c},
	{0x81,	0x04,	0x05,	0x0a,	0x0b,	0x0c},
	{0x82,	0x06,	0x07,	0x0a,	0x0b,	0x0c},

	/* 16 bit DMA channels */
	{0x8f,	0xc0,	0xc2,	0xd4,	0xd6,	0xd8},
	{0x8b,	0xc6,	0xc6,	0xd4,	0xd6,	0xd8},
	{0x89,	0xc8,	0xca,	0xd4,	0xd6,	0xd8},
	{0x8a,	0xcc,	0xce,	0xd4,	0xd6,	0xd8},
/*	{page,	addr,	count,	mask,	mode,	clear}, */
};

static void dma_cmd(uint8_t chan, uint8_t page, uint16_t ofs,
			uint16_t len, uint8_t mode)
{
	unsigned long flags;

	BUG_ON(chan >= ARRAY_SIZE(dma_chan));

	lock_irq(flags);

	/* Mask that sucker */
	outb(dma_chan[chan].mask, 0x04 | chan);

	/* Clear any current transfers */
	outb(dma_chan[chan].clear, 0x00);

	/* Send the mode to the controller */
	outb(dma_chan[chan].mode, mode);

	/* Send the offset address byte at a time */
	outb(dma_chan[chan].addr, ofs & 0xff);
	outb(dma_chan[chan].addr, (ofs >> 8) & 0xff);

	/* Send the page */
	outb(dma_chan[chan].page, page);

	/* Send data length */
	outb(dma_chan[chan].count, len & 0xff);
	outb(dma_chan[chan].count, (len >> 8) & 0xff);

	/* unmask her */
	outb(dma_chan[chan].mask, chan);

	unlock_irq(flags);
}

void dma_write(uint8_t chan, void *addr, uint16_t len)
{
	uint32_t a = (uint32_t)addr;
	dma_cmd(chan, a >> 16, a & 0xffff, len - 1, 0x48 + chan);
}

void dma_read(uint8_t chan, void *addr, uint16_t len)
{
	uint32_t a = (uint32_t)addr;
	dma_cmd(chan, a >> 16, a & 0xffff, len - 1, 0x44 + chan);
}
