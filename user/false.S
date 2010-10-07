#include <scaraOS/syscall.h>

.globl _start
_start:
	xorl %ebx, %ebx
	xorl %eax, %eax
	incl %ebx
	int $0xff
