#include <scaraOS/syscall.h>

.globl _start
_start:
	xorl %ebx, %ebx
	xorl %eax, %eax
	int $0xff
