#include <scaraOS/kernel.h>
#include <scaraOS/pci.h>

static LIST_HEAD(pci_domains);
static unsigned int num_domains;

static const char * const cls_str[] = {
	"PCI-1.x",
	"Mass Storage",
	"Network Controller",
	"Display Controller",
	"Multimedia Controller",
	"Memory Controller",
	"Bridge",
	"Simple Comms Controller",
	"Base system peripherals",
	"Input",
	"Docking Station",
	"Processor",
	"Serial bus",
};

static const char * const bridge_str[] = {
	"Host/PCI bridge",
	"PCI/ISA bridge",
	"PCI/EISA bridge",
	"PCI/Micro Channel bridge",
	"PCI/PCI bridge",
	"PCI/PCMCIA bridge",
	"PCI/NuBus bridge",
	"PCI/CardBus bridge",
};

#define CONF_LOC(bdf, ofs)(0x80000000 | (bdf << 8) | (ofs & 0xff))

static inline uint32_t pcidev_conf_read32(struct pci_dev *dev,
						unsigned int addr)
{
	return (*dev->pci_domain->d_ops->read_conf32)(dev->pci_domain,
						CONF_LOC(dev->pci_bdf, addr));
}

static inline void pcidev_conf_write32(struct pci_dev *dev,
				unsigned int addr, uint32_t v)
{
	(*dev->pci_domain->d_ops->write_conf32)(dev->pci_domain,
					CONF_LOC(dev->pci_bdf, addr), v);
}

static inline uint16_t pcidev_conf_read16(struct pci_dev *dev,
						unsigned int addr)
{
	return (*dev->pci_domain->d_ops->read_conf16)(dev->pci_domain,
						CONF_LOC(dev->pci_bdf, addr));
}

static inline void pcidev_conf_write16(struct pci_dev *dev,
				unsigned int addr, uint16_t v)
{
	(*dev->pci_domain->d_ops->write_conf8)(dev->pci_domain,
					CONF_LOC(dev->pci_bdf, addr), v);
}

static inline uint8_t pcidev_conf_read8(struct pci_dev *dev, unsigned int addr)
{
	return (*dev->pci_domain->d_ops->read_conf8)(dev->pci_domain,
						CONF_LOC(dev->pci_bdf, addr));
}

static inline void pcidev_conf_write8(struct pci_dev *dev,
				unsigned int addr, uint8_t v)
{
	(*dev->pci_domain->d_ops->write_conf8)(dev->pci_domain,
					CONF_LOC(dev->pci_bdf, addr), v);
}

static void pcidev_scan_bars(struct pci_dev *dev)
{
	unsigned int i;
	uint32_t irq;

	for(i = 0; i < PCI_NUM_BARS; i++) {
		uint32_t bar, sz, io;
		bar = pcidev_conf_read32(dev, PCI_CONF0_BAR0 + (i << 2));
		pcidev_conf_write32(dev, PCI_CONF0_BAR0 + (i << 2), ~0UL);
		sz = pcidev_conf_read32(dev, PCI_CONF0_BAR0 + (i << 2));
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
		printk(" - %s BAR%d (%8lu @ 0x%.8lx)\n",
			(io) ? "I/O" : "MEM", i, sz, bar);
	}

	irq = pcidev_conf_read32(dev, PCI_CONF0_IRQ);
	if ( irq & 0xff ) {
		printk(" - IRQ line %ld, #INT%c pin\n",
			(irq) & 0xff,
			'A' + (int)((irq >> 8) & 0xff));
	}
}

static void pcidev_add(struct pci_dom *dom, unsigned b,
				unsigned d, unsigned f)
{
	struct pci_dev *dev;
	uint32_t id, cls;
	const char *cstr;
	char clsbuf[10];

	dev = kmalloc(sizeof(*dev));
	if ( NULL == dev )
		return;

	dev->pci_domain = dom;
	dev->pci_driver = NULL;
	dev->pci_bdf = PCI_BDF(b, d, f);
	list_add_tail(&dev->pci_list, &dom->d_devices);

	id = pcidev_conf_read32(dev, PCI_CONF_ID);
	cls = pcidev_conf_read32(dev, PCI_CONF_CLASSREV);

	if ( (cls >> 24) < ARRAY_SIZE(cls_str) ) {
		if ( (cls >> 24) == 6 &&
			((cls >> 16) & 0xff) < ARRAY_SIZE(bridge_str) ) {
			cstr = bridge_str[(cls >> 16) & 0xff];
		}else{
			cstr = cls_str[cls >> 24];
		}
	}else{
		snprintf(clsbuf, sizeof(clsbuf),
			"%.2lx.%.2lx.%.2lx",
			cls >> 24, cls >> 16 & 0xff, cls >> 8 & 0xff);
		cstr = clsbuf;
	}
	printk("pci-dev: %i:%i.%i vendor=%.4lx "
		"device=%.4lx %s rev=%ld\n",
		b, d, f,
		id & 0xffff, id >> 16,
		cstr,
		cls & 0xff);
	pcidev_scan_bars(dev);

}

__init static void probe_dev(struct pci_dom *dom, unsigned b, unsigned d)
{
	unsigned f, bdf;
	uint32_t id;

	for(f = 0; f < PCI_NUM_FUNCS; f++) {
		uint8_t type;

		bdf = PCI_BDF(b, d, f);
		id = (*dom->d_ops->read_conf32)(dom,
					CONF_LOC(bdf, PCI_CONF_ID));
		if ( id == ~0UL ) {
			if ( 0 == f )
				return;
			else
				continue;
		}

		/* extract the header type */
		type = (*dom->d_ops->read_conf8)(dom,
					CONF_LOC(bdf, PCI_CONF_HDRTYPE));

		switch(type & 0x7f) {
		case 0:
			pcidev_add(dom, b, d, f);
			break;
		case 1:
			/* TODO: handle bridge */
			break;
		default:
			printk(" - UNSUPPORTED HEADER TYPE: %d\n", type);
			break;
		}

		if ( f == 0 && (type & 0x80) ) {
			/* we know it's a single function device */
			return;
		}
	}
}

__init static void probe_bus(struct pci_dom *dom, unsigned b)
{
	unsigned d;

	for(d = 0; d < PCI_NUM_DEVS; d++) {
		probe_dev(dom, b, d);
	}
}

__init static void probe_domain(struct pci_dom *dom)
{
	unsigned long flags;
	unsigned b;

	lock_irq(flags);
	for(b = 0; b < PCI_NUM_BUSSES; b++) {
		probe_bus(dom, b);
	}
	unlock_irq(flags);
}

void __init pci_domain_add(struct pci_dom *dom)
{
	list_add_tail(&dom->d_list, &pci_domains);
	INIT_LIST_HEAD(&dom->d_devices);
	dom->d_domain = num_domains++;
}

void __init pci_init(void)
{
	struct pci_dom *dom;

	pci_arch_init();

	list_for_each_entry(dom, &pci_domains, d_list)
		probe_domain(dom);
}
