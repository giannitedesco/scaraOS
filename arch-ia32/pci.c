#include <scaraOS/kernel.h>
#include <scaraOS/pci.h>
#include <arch/io.h>

struct pci_dom_conf1 {
	struct pci_dom d;
	uint32_t confreg;
};

#define PCI_CONFREG		0x0cf8
#define PCI_CONF_MODE		0x0cfb
#define PCI_CONFDATA		0x0cfc

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

static void conf_latch(struct pci_dom_conf1 *d, uint32_t latch)
{
	if ( d->confreg != latch ) {
		d->confreg = latch;
		outl(PCI_CONFREG, latch);
	}
}

static uint32_t read_conf1_32(struct pci_dom *dom, uint32_t addr)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	return inl(PCI_CONFDATA);
}

static void write_conf1_32(struct pci_dom *dom, uint32_t addr, uint32_t w)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	outl(PCI_CONFDATA, w);
}

static uint16_t read_conf1_16(struct pci_dom *dom, uint32_t addr)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	return inw(PCI_CONFDATA + (addr & 0x2));
}

static void write_conf1_16(struct pci_dom *dom, uint32_t addr, uint16_t w)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	outw(PCI_CONFDATA, w + (addr & 0x2));
}

static uint8_t read_conf1_8(struct pci_dom *dom, uint32_t addr)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	return inb(PCI_CONFDATA + (addr & 0x3));
}

static void write_conf1_8(struct pci_dom *dom, uint32_t addr, uint8_t w)
{
	struct pci_dom_conf1 *d = (struct pci_dom_conf1 *)dom;
	conf_latch(d, addr);
	outb(PCI_CONFDATA, w + (addr & 0x3));
}

static const struct pci_dom_ops pci_conf1_ops = {
	.read_conf32 = read_conf1_32,
	.write_conf32 = write_conf1_32,
	.read_conf16 = read_conf1_16,
	.write_conf16 = write_conf1_16,
	.read_conf8 = read_conf1_8,
	.write_conf8 = write_conf1_8,
};

static struct pci_dom_conf1 pci_conf1 = {
	.d.d_ops = &pci_conf1_ops,
	.confreg = ~0UL,
};

void __init pci_arch_init(void)
{
	unsigned int pci_conf;

	pci_conf = pci_probe();

	/* We can only deal with conf1 for now */
	switch(pci_conf) {
	case 1:
		dprintk("pci: Using configuration 1\n");
		pci_domain_add(&pci_conf1.d);
		break;
	default:
		printk("pci: Using configuration %d (deprecated)\n", pci_conf);
		return;
	}
}

extern const struct pci_cls_tbl __pci_cls_start;
extern const struct pci_cls_tbl __pci_cls_end;
const struct pci_driver *pci_arch_scan_cls(uint32_t cls)
{
	const struct pci_cls_tbl *ptr;

	for(ptr = &__pci_cls_start; ptr < &__pci_cls_end; ptr++) {
		if ( (cls & ptr->cls_mask) == ptr->cls_val )
			return ptr->cls_driver;
	}

	return NULL;
}
