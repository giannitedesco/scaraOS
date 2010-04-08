.section .text
.text

.globl __strnlen_from_user
__strnlen_from_user:
	pushl %esi
	pushl %edi
	movl 0xc(%esp), %edi
	cld
	movl 0x10(%esp), %ecx
	movl %ecx, %edx
	xor %eax, %eax
__strlen_pagefault:
	repnz scas %es:(%edi),%al
	incl %ecx
	subl %ecx, %edx
	movl %edx, %eax
	popl %edi
	popl %esi
	ret

.globl __copy_from_user
.globl __copy_to_user
__copy_from_user:
__copy_to_user:
	pushl %esi
	pushl %edi
	movl 0xc(%esp), %edi
	movl 0x10(%esp), %esi
	movl 0x14(%esp), %ecx
	movl %ecx, %eax
__copy_user_pagefault:
	rep movsb
	popl %edi
	popl %esi
	ret

__pop_edi_esi_fixup:
	movl $0xffffffff, %eax
	popl %edi
	popl %esi
	ret
.data
.section .rodata.pagefault
	.long __strlen_pagefault
	.long __pop_edi_esi_fixup
	.long __copy_user_pagefault
	.long __pop_edi_esi_fixup
