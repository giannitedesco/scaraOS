#ifndef __DESCRIPTOR_TABLE_INCLUDED__
#define __DESCRIPTOR_TABLE_INCLUDED__

#define __desc_aligned __attribute__((aligned(8)))

/*******************************
 * ia32 descriptor table stuff *
 *******************************/
#define	D_LDT		0x0200
#define D_TASK		0x0500
#define	D_TSS		0x0900
#define	D_CALL		0x0C00
#define	D_INT		0x0E00
#define	D_TRAP		0x0F00
#define	D_DATA		0x1000
#define	D_CODE		0x1800
#define	D_DPL3		0x6000
#define	D_DPL2		0x4000
#define	D_DPL1		0x2000
#define	D_PRESENT	0x8000
#define	D_NOT_PRESENT	0x8000
#define	D_ACC		0x0100
#define	D_WRITE		0x0200
#define	D_READ		0x0200
#define	D_BUSY		0x0200
#define	D_EXDOWN	0x0400
#define	D_CONFORM	0x0400
#define	D_BIG		0x0040
#define	D_BIG_LIM	0x0080

struct gdtr {
	uint16_t	limit;
	uint32_t	addr;
}__attribute__((packed));

struct ia32_desc2 {
	uint16_t	limit_low;
	uint16_t	base_low;
	uint8_t		base_med;
	uint16_t	type:4;
	uint16_t	desc_type:1; /* 0=system, 1=code/data */
	uint16_t	dpl:2;
	uint16_t	present:1;
	uint16_t	limit_high:4;
	uint16_t	avl:1; /* available */
	uint16_t	res:1;
	uint16_t	db:1; /* 0=16bit, 1=32bit */
	uint16_t	granularity:1; /* 1=full */
	uint8_t		base_high;
} __attribute__((packed));

struct ia32_desc {
	uint16_t	limit_low;
	uint16_t	base_low;
	uint8_t		base_med;
	uint8_t		access;
	uint8_t		limit_high:4;
	uint8_t		granularity:4;
	uint8_t		base_high;
} __attribute__ ((packed));

struct ia32_gate {
	uint16_t	offset_low;
	uint16_t	selector;
	uint16_t	access;
	uint16_t	offset_high;
} __attribute__ ((packed));

typedef union dt_entry {
	struct ia32_desc desc;
	struct ia32_gate gate;
	uint64_t dummy;
}dt_entry_t;

/* Macro to easily create descriptors */
#define stnd_desc(base, limit, control) \
	(struct ia32_desc){\
		.limit_low	= (limit & 0xffff), \
		.base_low	= (base & 0xffff), \
		.base_med	= ((base >> 16) & 0xff), \
		.access		= ((control | D_PRESENT) >> 8), \
		.limit_high	= (limit >> 16),\
		.granularity	= ((control & 0xff) >> 4), \
		.base_high	= (base >> 24) \
	}

#define gate_desc(offset, selector, control) \
	{ \
		.offset_low = (offset & 0xffff), \
		.selector = (control | D_PRESENT), \
		.access = (offset >> 16) \
		.offset_high = ((offset >> 16) & 0xffff),\
	}

#endif /* __DESCRIPTOR_TABLE_INCLUDED__ */
