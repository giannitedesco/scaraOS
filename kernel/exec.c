#include <kernel.h>
#include <arch/regs.h>
#include <arch/syscalls.h>
#include <task.h>
#include <vfs.h>
#include <elf.h>

static int do_exec(const char *path)
{
	struct mem_ctx *ctx;
	struct inode *inode;
	uint8_t *phbuf, *ph;
	struct task *tsk;
	unsigned int i;
	Elf32_Ehdr hdr;
	size_t phbufsz;
	ssize_t ret;

	inode = namei(path);
	if ( NULL == inode ) {
		printk("exec: %s: ENOENT or ENOTDIR\n", path);
		return -1;
	}

	printk("exec: %s: mode 0%lo, %lu bytes in %lu blocks\n",
		path, inode->i_mode, inode->i_size, inode->i_blocks);
	
	ret = inode->i_iop->pread(inode, &hdr, sizeof(hdr), 0);
	if ( ret <= 0 || (size_t)ret != sizeof(hdr) ) {
		printk("exec: unable to read ELF header\n");
		return ret;
	}

	if ( !memcmp(hdr.e_ident, ELFMAG, sizeof(hdr.e_ident)) ) {
		printk("exec: bad ELF magic\n");
		return -1; /* ENOEXEC */
	}

	if ( hdr.e_type != ET_EXEC ) {
		printk("exec: not an ELF executable\n");
		return -1;
	}

	if ( hdr.e_machine != EM_386 ) {
		printk("exec: not an IA-32 executable\n");
		return -1;
	}

	if ( hdr.e_phentsize < sizeof(Elf32_Phdr) ) {
		printk("exec: Unexpectedly short program header entry size\n");
		return -1;
	}

	dprintk("exec: Program header contains %u entries\n", hdr.e_phnum);

	phbufsz = hdr.e_phentsize * hdr.e_phnum;
	phbuf = kmalloc(phbufsz);
	if ( NULL == phbuf )
		return -1; /* ENOMEM */

	ret = inode->i_iop->pread(inode, phbuf, phbufsz, hdr.e_phoff);
	if ( ret <= 0 || (size_t)ret != phbufsz ) {
		printk("exec: unable to read program header\n");
		goto err_free; /* ENOEXEC */
	}

	tsk = __this_task;

	ctx = mem_ctx_new();
	if ( NULL == ctx )
		goto err_free;

	for(ph = phbuf, i = 0; i < hdr.e_phnum; i++, ph += hdr.e_phentsize) {
		Elf32_Phdr *phdr = (Elf32_Phdr *)ph;
		if ( phdr->p_type != PT_LOAD )
			continue;
		if ( setup_vma(ctx, phdr->p_vaddr, phdr->p_memsz,
				PROT_READ|PROT_EXEC, inode, phdr->p_offset) )
			goto err_free_ctx;
		printk("elf: PT_LOAD: va=0x%.8lx %lu bytes from offset %lu\n",
			phdr->p_vaddr, phdr->p_filesz, phdr->p_offset);
	}
	kfree(phbuf);

	/* setup usermode stack */
	if ( setup_vma(ctx, 0x80000000 - PAGE_SIZE, PAGE_SIZE,
		PROT_READ|PROT_WRITE|PROT_EXEC, NULL, 0) )
		goto err_free_ctx;

	mem_ctx_put(tsk->ctx);
	tsk->ctx = ctx;
	set_context(tsk);
	task_init_exec(tsk, hdr.e_entry, 0x80000000);
	return 0;

err_free_ctx:
	mem_ctx_put(ctx);
err_free:
	kfree(phbuf);
	return -1;
}

uint32_t syscall_exec(uint32_t path)
{
	return do_exec((char *)path);
}
