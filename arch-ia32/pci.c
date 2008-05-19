#include <kernel.h>
#include <arch/pci.h>

#define PCICMD(bus, dev, fn)	(0x80000000 | (bus << 16) | (dev << 11) | (fn << 3))

int pci_conf=0;

int __init pci_probe()
{
	uint32_t tmp;

	outb(0x0cfb, 0x01);
	tmp = inl(PCI_CONFREG);
	outl(PCI_CONFREG, 0x80000000);
	if (inl(PCI_CONFREG) == 0x80000000) {
		outl(PCI_CONFREG, tmp);
		printk("pci: Using configuration 1\n");
		return 1;
	}
	outl(PCI_CONFREG, tmp);

	outb(0x0cfb, 0x00);
	outb(PCI_CONFREG, 0x00);
	outb(0x0cfa, 0x00);
	if ( (inb(PCI_CONFREG) == 0x00) && 
		(inb(0xCFA) == 0x00)) {
		printk("pci: Using configuration 2 (deprecated)\n");
		return 2;
	}

	printk("pci: no pci busses detected\n");
	return 0;
}

/* TODO: Grab whole configuration space */
void __init pci_detect_dev(int b, int d, int f)
{
	long flags;

	uint32_t id,class;
	uint32_t loc=PCICMD(b,d,f);

	lock_irq(flags);
	outl(PCI_CONFREG, loc | (PCI_CONF_ID<<2));
	id=inl(PCI_CONFDATA);
	outl(PCI_CONFREG, loc | (PCI_CONF_CLASS<<2));
	class=inl(PCI_CONFDATA);
	unlock_irq(flags);

	if ( id!=0xffffffff ) {
		printk("pci-dev: %i:%i.%i device=%x vendor=%x class=%x.%x.%x\n",
			b, d, f,
			id>>16, id&0xffff,
			class>>24, class>>16&0xff, class>>8&0xff);
	}
}

void __init pci_scan_bus(int bus)
{
	int d;

	for(d=0; d<31; d++) {
		pci_detect_dev(bus, d, 0);
	}
}

void __init pci_init(void)
{
	long flags;

	lock_irq(flags);
	pci_conf=pci_probe();
	unlock_irq(flags);

	/* We can only deal with conf1 for now */
	if ( pci_conf!= 1 ) {
		printk("pci: PCI access method not supported, or not found\n");
		return;
	}

	pci_scan_bus(0);
}
