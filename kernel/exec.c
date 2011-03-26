#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/vfs.h>
#include <scaraOS/elf.h>
#include <scaraOS/exec.h>

#define USER_STACK_BASE 	0x80000000
#define USER_STACK_PAGES	1024UL
#define USER_STACK_SIZE		(USER_STACK_PAGES << PAGE_SHIFT)

static int do_exec(char *path)
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
		kfree(path);
		return -1;
	}

	dprintk("exec: %s: mode 0%lo, %lu bytes in %lu blocks\n",
		path, inode->i_mode, inode->i_size, inode->i_blocks);

	kfree(path);

	ret = inode->i_iop->pread(inode, &hdr, sizeof(hdr), 0);
	if ( ret <= 0 || (size_t)ret != sizeof(hdr) ) {
		printk("exec: unable to read ELF header\n");
		return -1;
	}

	if ( !memcmp(hdr.e_ident, ELFMAG, sizeof(hdr.e_ident)) ) {
		printk("exec: bad ELF magic\n");
		return -1; /* ENOEXEC */
	}

	if ( hdr.e_type != ET_EXEC ) {
		printk("exec: not an ELF executable\n");
		return -1; /* ENOEXEC */
	}

	if ( hdr.e_machine != EM_386 ) {
		printk("exec: not an IA-32 executable\n");
		return -1; /* ENOEXEC */
	}

	if ( hdr.e_phentsize < sizeof(Elf32_Phdr) ) {
		printk("exec: Unexpectedly short program header entry size\n");
		return -1; /* ENOEXEC */
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
		dprintk("elf: PT_LOAD: va=0x%.8lx %lu bytes from offset %lu\n",
			phdr->p_vaddr, phdr->p_filesz, phdr->p_offset);
		if ( phdr->p_filesz ) {
			if ( setup_vma(ctx, phdr->p_vaddr, phdr->p_memsz,
				PROT_READ|PROT_EXEC, inode, phdr->p_offset) )
			goto err_free_ctx;
		}else{
			if ( setup_vma(ctx, phdr->p_vaddr, phdr->p_memsz,
				PROT_READ|PROT_WRITE, NULL, 0) )
			goto err_free_ctx;
		}
	}

	/* setup usermode stack */
	if ( setup_vma(ctx, USER_STACK_BASE - USER_STACK_SIZE,
			USER_STACK_SIZE, PROT_READ|PROT_WRITE, NULL, 0) )
		goto err_free_ctx;

	kfree(phbuf);

	/* commit to the exec, all the way now */
	cli();
	mem_use_ctx(ctx);
	mem_ctx_put(tsk->ctx);
	tsk->ctx = ctx;
	task_init_exec(tsk, hdr.e_entry, USER_STACK_BASE);
	return 0;

err_free_ctx:
	mem_ctx_put(ctx);
err_free:
	kfree(phbuf);
	return -1;
}

int _sys_exec(const char *path)
{
	char *kpath;

	kpath = strdup_from_user(path);
	if ( NULL == kpath )
		return -1; /* EFAULT or ENOMEM */
	
	return do_exec(kpath);
}

int _kernel_exec(const char *path)
{
	char *kpath;

	kpath = strdup(path);
	if ( NULL == kpath )
		return -1; /* EFAULT or ENOMEM */
	
	return do_exec(kpath);
}
