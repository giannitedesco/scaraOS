#include <scaraOS/kernel.h>
#include <arch/io.h>
#include <arch/pci.h>

#define PCICMD(bus, dev, fn)	(0x80000000 | (PCI_BDF(bus, dev, fn) << 3))

static unsigned int pci_conf;

static int __init pci_probe(void)
{
	uint32_t tmp;

	outb(PCI_CONF_MODE, 0x01);
	tmp = inl(PCI_CONFREG);
	outl(PCI_CONFREG, 0x80000000);
	if (inl(PCI_CONFREG) == 0x80000000) {
		outl(PCI_CONFREG, tmp);
		return 1;
	}
	outl(PCI_CONFREG, tmp);

	outb(PCI_CONF_MODE, 0x00);
	outb(PCI_CONFREG, 0x00);
	outb(0x0cfa, 0x00);
	if ( (inb(PCI_CONFREG) == 0x00) && 
		(inb(0xCFA) == 0x00)) {
		return 2;
	}

	return 0;
}

/* TODO: Grab whole configuration space */
static void __init pci_detect_dev(int b, int d, int f)
{
	long flags;
	uint32_t id, class;
	uint32_t loc = PCICMD(b,d,f);

	lock_irq(flags);
	outl(PCI_CONFREG, loc | (PCI_CONF_ID << 2));
	id = inl(PCI_CONFDATA);
	outl(PCI_CONFREG, loc | (PCI_CONF_CLASS << 2));
	class = inl(PCI_CONFDATA);
	unlock_irq(flags);

	if ( id != 0xffffffff ) {
		printk("pci-dev: %i:%i.%i vendor=%.4lx "
			"device=%.4lx class=%.2lx.%.2lx.%.2lx\n",
			b, d, f,
			id & 0xffff, id >> 16,
			class >> 24, class >> 16 & 0xff, class >> 8 & 0xff);
	}
}

static void __init pci_scan_bus(int bus)
{
	int d;

	for(d = 0; d < 31; d++)
		pci_detect_dev(bus, d, 0);
}

void __init pci_init(void)
{
	long flags;

	lock_irq(flags);
	pci_conf = pci_probe();
	unlock_irq(flags);

	/* We can only deal with conf1 for now */
	switch(pci_conf) {
	case 1:
		dprintk("pci: Using configuration 1\n");
		break;
	default:
		printk("pci: Using configuration %d (deprecated)\n", pci_conf);
		return;
	}

	pci_scan_bus(0);
}
