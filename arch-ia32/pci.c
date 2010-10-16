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
	uint32_t id, class;
	uint32_t loc = PCICMD(b,d,f);
	unsigned int i;
	long flags;

	lock_irq(flags);

	outl(PCI_CONFREG, loc | (PCI_CONF_ID << 2));
	id = inl(PCI_CONFDATA);
	outl(PCI_CONFREG, loc | (PCI_CONF_CLASS << 2));
	class = inl(PCI_CONFDATA);

	if ( id == 0xffffffff )
		goto out;

	printk("pci-dev: %i:%i.%i vendor=%.4lx "
		"device=%.4lx rev=0x%.2x class=%.2lx.%.2lx.%.2lx\n",
		b, d, f,
		id & 0xffff, id >> 16,
		class & 0xff,
		class >> 24, class >> 16 & 0xff, class >> 8 & 0xff);
	
	for(i = 0; i < PCI_NUM_BARS; i++) {
		uint32_t bar, sz, io;
		outl(PCI_CONFREG, loc | ((PCI_CONF_BAR0 + i) << 2));
		bar = inl(PCI_CONFDATA);
		outl(PCI_CONFDATA, ~0UL);
		sz = inl(PCI_CONFDATA);
		if ( !bar )
			continue;
		if ( bar & 1 ) {
			io = 1;
			bar &= ~0x3;
			sz = ~(sz & ~0x3) + 1;
		}else{
			uint32_t mask;
			switch ( (bar & 0x3) >> 1 ) {
			case 0:
				mask = ~0xfUL;
				break;
			case 1:
				mask = ~0xfUL;
				break;
			case 2:
				i++;
				continue;
			default:
				/* ??? */
				continue;
			}
			io = 0;
			bar &= mask;
			sz = ~(sz & mask) + 1;
		}
		printk(" - %s BAR%d (%8u @ 0x%.8x)\n",
			(io) ? "I/O" : "MEM", i, sz, bar);
	}

out:
	unlock_irq(flags);
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
