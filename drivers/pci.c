#include <scaraOS/kernel.h>
#include <scaraOS/pci.h>

static LIST_HEAD(pci_domains);
static unsigned int num_domains;

#define CONF_LOC(bdf, ofs)(0x80000000 | (bdf << 8) | (ofs << 2))

static uint32_t pcidev_conf_read(struct pci_dev *dev, unsigned int addr)
{
	return (*dev->pci_domain->d_ops->read_conf)(dev->pci_domain,
						CONF_LOC(dev->pci_bdf, addr));
}

static void pcidev_conf_write(struct pci_dev *dev,
				unsigned int addr, uint32_t v)
{
	(*dev->pci_domain->d_ops->write_conf)(dev->pci_domain,
					CONF_LOC(dev->pci_bdf, addr), v);
}

static void pcidev_scan_bars(struct pci_dev *dev)
{
	unsigned int i;

	for(i = 0; i < PCI_NUM_BARS; i++) {
		uint32_t bar, sz, io;
		bar = pcidev_conf_read(dev, PCI_CONF_BAR0 + i);
		pcidev_conf_write(dev, PCI_CONF_BAR0 + i, ~0UL);
		sz = pcidev_conf_read(dev, PCI_CONF_BAR0 + i);
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
}

static void pcidev_add(struct pci_dom *dom, unsigned b,
				unsigned d, unsigned f)
{
	struct pci_dev *dev;
	uint32_t id, cls;

	dev = kmalloc(sizeof(*dev));
	if ( NULL == dev )
		return;

	dev->pci_domain = dom;
	dev->pci_bdf = PCI_BDF(b, d, f);
	list_add_tail(&dev->pci_list, &dom->d_devices);

	id = pcidev_conf_read(dev, PCI_CONF_ID);
	cls = pcidev_conf_read(dev, PCI_CONF_CLASS);
	printk("pci-dev: %i:%i.%i vendor=%.4lx "
		"device=%.4lx rev=0x%.2lx class=%.2lx.%.2lx.%.2lx\n",
		b, d, f,
		id & 0xffff, id >> 16,
		cls & 0xff,
		cls >> 24, cls >> 16 & 0xff, cls >> 8 & 0xff);
	
	pcidev_scan_bars(dev);
}

__init static void probe_dev(struct pci_dom *dom, unsigned b, unsigned d)
{
	unsigned f, bdf;
	uint32_t id;

	for(f = 0; f < PCI_NUM_FUNCS; f++) {
		bdf = PCI_BDF(b, d, f);
		id = (*dom->d_ops->read_conf)(dom, CONF_LOC(bdf, PCI_CONF_ID));
		if ( id == ~0UL ) {
			if ( 0 == f )
				return;
			else
				continue;
		}

		pcidev_add(dom, b, d, f);
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
