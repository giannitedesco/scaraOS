#ifndef __ARCH_IA32_DMA__
#define __ARCH_IA32_DMA__

#ifndef __ASM__
void dma_cmd(uint8_t, uint8_t, uint16_t, uint16_t, uint8_t);

static inline void dma_write(uint8_t chan, void *addr, uint16_t len)
{
	uint32_t a=(uint32_t)addr;
	dma_cmd(chan, a>>16, a&0xffff, len-1, 0x48+chan);
}

static inline void dma_read(uint8_t chan, void *addr, uint16_t len)
{
	uint32_t a=(uint32_t)addr;
	dma_cmd(chan, a>>16, a&0xffff, len-1, 0x44+chan);
}
#endif

#endif /* __ARCH_IA32_DMA__ */
