#ifndef __ARCH_IA32_PCI__
#define __ARCH_IA32_PCI__

#define PCI_CONFREG		0x0cf8
#define PCI_CONFDATA		0x0cfc

#define PCI_CONF_ID		0x00	/* vendor and device ID, 16 bits each */
#define PCI_CONF_CMDSTAT	0x01	/* commands & stats, 16 bits each */
#define	PCI_CONF_CLASS		0x02	/* class & revision, class is top 16 bits */
#define PCI_CONF_SUNDRIES	0x03	/* cache, latency, header & bist; 8 bits each */
#define PCI_CONF_IOADDR		0x04	/* I/O base address in top 27 bits */

#ifndef __ASM__
void __init pci_init(void);
#endif

#endif /* __ARCH_IA32_PCI__ */
