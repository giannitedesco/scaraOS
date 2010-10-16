#ifndef __ARCH_IA32_PCI__
#define __ARCH_IA32_PCI__

#define PCI_CONFREG		0x0cf8
#define PCI_CONF_MODE		0x0cfb
#define PCI_CONFDATA		0x0cfc

#define PCI_NUM_BARS		6

#define PCI_CONF_ID		0x00 /* vendor and device ID */
#define PCI_CONF_CMDSTAT	0x01 /* commands & stats */
#define	PCI_CONF_CLASS		0x02 /* class & revision */
#define PCI_CONF_BIST		0x03 /* bist, type, timer, cache line size */
#define PCI_CONF_BAR0		0x04
#define PCI_CONF_BAR1		0x05
#define PCI_CONF_BAR2		0x06
#define PCI_CONF_BAR3		0x07
#define PCI_CONF_BAR4		0x08
#define PCI_CONF_BAR5		0x09
#define PCI_CONF_SUNDRIES	0x03 /* cache, latency, header & bist; 8 bits each */
#define PCI_CONF_IOADDR		0x04 /* I/O base address in top 27 bits */

#define PCI_FUNC_SHIFT		0
#define PCI_DEV_SHIFT		8
#define PCI_BUS_SHIFT		11

#define PCI_BDF(bus, dev, fn) 	(bus << PCI_BUS_SHIFT) | \
					(dev << PCI_DEV_SHIFT) | \
					(fn << PCI_FUNC_SHIFT)

#ifndef __ASM__
void __init pci_init(void);
#endif

#endif /* __ARCH_IA32_PCI__ */
