#ifndef __KERNEL_USER_INCLUDED__
#define __KERNEL_USER_INCLUDED__

#include <arch/page.h>

static inline int uaddr_ok(vaddr_t vaddr, size_t count)
{
	size_t sz;

	if ( vaddr >= PAGE_OFFSET )
		return 0;

	sz = PAGE_OFFSET - vaddr;
	if ( count > sz )
		return 0;

	return 1;
}

static inline size_t uaddr_maxstr(vaddr_t vaddr)
{
	if ( vaddr >= PAGE_OFFSET )
		return 0;

	return PAGE_OFFSET - vaddr;
}

int __copy_to_user(char *d, const char *s, size_t c);
int __copy_from_user(char *d, const char *s, size_t c);
int __strnlen_from_user(const char *s, size_t c);
#endif /* __KERNEL_USER_INCLUDED__ */
