#ifndef __ARCH_IA32_MULTIBOOT__
#define __ARCH_IA32_MULTIBOOT__

#ifdef __ELF__
#	define MULTIBOOT_HEADER_FLAGS		0x00000003
#else
#	define MULTIBOOT_HEADER_FLAGS		0x00010003
#endif

#define MULTIBOOT_BOOTLOADER_MAGIC		0x2BADB002
#define MULTIBOOT_HEADER_MAGIC			0x1BADB002

#ifndef __ASM__

#define MBF_MEM		0x01
#define MBF_BOOTDEV	0x02
#define MBF_CMDLINE	0x04
#define MBF_MODULES	0x08
#define MBF_AOUT	0x10
#define MBF_ELF		0x20
#define MBF_MMAP	0x40

/* The Multiboot header.  */
typedef struct multiboot_header
{
	unsigned long magic;
	unsigned long flags;
	unsigned long checksum;
	unsigned long header_addr;
	unsigned long load_addr;
	unsigned long load_end_addr;
	unsigned long bss_end_addr;
	unsigned long entry_addr;
} multiboot_header_t;

/* The symbol table for a.out.  */
typedef struct aout_symbol_table
{
	unsigned long tabsize;
	unsigned long strsize;
	unsigned long addr;
	unsigned long reserved;
} aout_symbol_table_t;

/* The section header table for ELF.  */
typedef struct elf_section_header_table
{
	unsigned long num;
	unsigned long size;
	unsigned long addr;
	unsigned long shndx;
} elf_section_header_table_t;

/* The module structure.  */
typedef struct module
{
	unsigned long mod_start;
	unsigned long mod_end;
	unsigned long string;
	unsigned long reserved;
} module_t, *p_module;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size.  */
typedef struct memory_map
{
	unsigned long size;
	unsigned long base_addr_low;
	unsigned long base_addr_high;
	unsigned long length_low;
	unsigned long length_high;
	unsigned long type;
} memory_map_t, *p_memory_map;

/* The Multiboot information.  */
typedef struct multiboot_info
{
	unsigned long flags;
	unsigned long mem_lower;
	unsigned long mem_upper;
	unsigned long boot_device;

	/* Command line  */
	char *cmdline;

	/* Modules loaded by the bootloader */
	unsigned long mods_count;
	p_module mods_addr;

	/* Info about the kernel binary */	
	union
	{
		aout_symbol_table_t aout_sym;
		elf_section_header_table_t elf_sec;
	} u;
	
	/* Memory maps */
	unsigned long mmap_length;
	p_memory_map mmap;
} multiboot_info_t;

int _asmlinkage multiboot_check(uint32_t magic, multiboot_info_t *mbi);
void _asmlinkage setup(multiboot_info_t *mbi);

#endif /* ! __ASM__ */

#endif /* __ARCH_IA32_MULTIBOOT__ */
