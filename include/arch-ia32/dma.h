#ifndef __ARCH_IA32_DMA__
#define __ARCH_IA32_DMA__

#ifndef __ASM__
void dma_write(uint8_t chan, void *addr, uint16_t len);
void dma_read(uint8_t chan, void *addr, uint16_t len);
#endif

#endif /* __ARCH_IA32_DMA__ */
