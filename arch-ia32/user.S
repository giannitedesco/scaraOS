.section .text
.text

.globl strlen_from_user
strlen_from_user:
	pushl %esi
	pushl %edi
	movl 0xc(%esp), %edi
	cld
	movl $0xffffffff, %ecx
	xor %eax,%eax
__strlen_pagefault:
	repnz scas %es:(%edi),%al
	not %ecx
	dec %ecx
	movl %ecx, %eax
	popl %edi
	popl %esi
	ret

.globl copy_from_user
.globl copy_to_user
copy_from_user:
copy_to_user:
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
