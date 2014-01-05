#ifndef __PCI_H__
#define __PCI_H__

#define PCI_NUM_BARS		6

/* PCI CONFIG SPACE */
#define PCI_CONF_ID		0x00 /* vendor and device ID */
#define PCI_CONF_CMDSTAT	0x01 /* commands & stats */
#define	PCI_CONF_CLASS		0x02 /* class & revision */
#define PCI_CONF_BIST		0x03 /* bist, type, timer, cache line size */

/* type 0 header */
#define PCI_CONF0_BAR0		0x04
#define PCI_CONF0_BAR1		0x05
#define PCI_CONF0_BAR2		0x06
#define PCI_CONF0_BAR3		0x07
#define PCI_CONF0_BAR4		0x08
#define PCI_CONF0_BAR5		0x09
#define PCI_CONF0_CIS		0x0a /* CardBUS CIS pointer */
#define PCI_CONF0_SUBSYS	0x0b
#define PCI_CONF0_ROM_BAR	0x0c
#define PCI_CONF0_CAP_OFS	0x0d
#define PCI_CONF0_RSVD1		0x0e
#define PCI_CONF0_IRQ		0x0f /* line, pin, min/max lat */
#define PCI_CONF0_RSVD		0x10

#define PCI_NUM_BUSSES		0x100
#define PCI_NUM_DEVS		0x20
#define PCI_NUM_FUNCS		0x08

#define PCI_BUS_MASK		(PCI_NUM_BUSSES-1)
#define PCI_DEV_MASK		(PCI_NUM_DEVS-1)
#define PCI_FUNC_MASK		(PCI_NUM_FUNCS-1)

#define PCI_FUNC_SHIFT		0
#define PCI_DEV_SHIFT		3
#define PCI_BUS_SHIFT		8

#define PCI_BDF(bus, dev, fn)	((bus & PCI_BUS_MASK) << PCI_BUS_SHIFT) | \
				((dev & PCI_DEV_MASK) << PCI_DEV_SHIFT) | \
				((fn & PCI_FUNC_MASK) << PCI_FUNC_SHIFT)

#ifndef __ASM__
struct pci_dom {
	struct list_head d_list;
	struct list_head d_devices;
	uint32_t d_domain;
	const struct pci_dom_ops *d_ops;
};

struct pci_dom_ops {
	uint32_t (*read_conf)(struct pci_dom *dom, uint32_t addr);
	void (*write_conf)(struct pci_dom *dom, uint32_t addr, uint32_t w);
};

struct pci_dev {
	struct pci_dom *pci_domain;
	struct pci_driver *pci_driver;
	struct list_head pci_list;
	uint32_t pci_bdf;
};

void __init pci_domain_add(struct pci_dom *dom);
void __init pci_arch_init(void);
void __init pci_init(void);

struct pci_cls_driver {
	uint32_t cls_rev;
	uint32_t mask;
};
#endif

#endif /* __PCI_H__ */
