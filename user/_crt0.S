#include <scaraOS/syscall.h>

.globl _start
_start:
	pushl $0
	pushl $0
	call main
	movl %eax, %ebx
	movl $_SYS_exit, %eax
	int $0xff
