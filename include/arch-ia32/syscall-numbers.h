#ifndef _SYSCALL_NUMBERS_H
#define _SYSCALL_NUMBERS_H

#define _SYS_exit		0
#define _SYS_exec		1
#define _SYS_fork		2
#define _SYS_open		3
#define _SYS_close		4
#define _SYS_read		5
#define _SYS_write		6
#define _SYS_NR_SYSCALLS	7

#ifndef __ASM__

#define _SYSCALL1(nr, ret_type, name, type1) 				\
	ret_type name(type1 arg1) 					\
	{								\
		ret_type ret;						\
		asm volatile ("xchgl %%edi,%%ebx\n"			\
				"int $0xff\n"				\
				"xchgl %%ebx,%%edi\n"			\
				: "=a" (ret) : "0" (nr), 		\
				"D" (arg1));				\
		return ret;						\
	}

#define _SYSCALL2(nr, ret_type, name, type1, type2) 			\
	ret_type name(type1 arg1, type2 arg2) 				\
	{								\
		ret_type ret;						\
		asm volatile ("xchgl %%edi,%%ebx\n"			\
				"int $0xff\n"				\
				"xchgl %%edi,%%ebx\n"			\
				: "=a" (ret) : "0" (nr), 		\
					"D" (arg1),			\
					"c" (arg2));			\
		return ret;						\
	}

#define _SYSCALL3(nr, ret_type, name, type1, type2, type3)		\
	ret_type name(type1 arg1, type2 arg2, type3 arg3)		\
	{								\
		ret_type ret;						\
		asm volatile ("xchgl %%edi,%%ebx\n"			\
				"int $0xff\n"				\
				"xchgl %%edi,%%ebx\n"			\
				: "=a" (ret) : "0" (nr), 		\
					"D" (arg1),			\
					"c" (arg2), 			\
					"d" (arg3));			\
		return ret;						\
	}

#define _SYSCALL4(nr, ret_type, name, type1, type2, type3, type4)	\
	ret_type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)	\
	{								\
		ret_type ret;						\
		asm volatile ("xchgl %%edi,%%ebx\n"			\
				"int $0xff\n"				\
				"xchgl %%edi,%%ebx\n"			\
				: "=a" (ret) : 				\
				"0" (nr), 				\
				"D" (arg1),				\
				"c" (arg2), 				\
				"d" (arg3), 				\
				"S" (arg4));				\
		return ret;						\
	}

#define _SYSCALL5(nr, ret_type, name, type1, type2, type3, type4)	\
	ret_type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)	\
	{								\
		ret_type ret;						\
		asm volatile ("movl %%ebx,%7\n"				\
				"movl %2,%%ebx\n"			\
				"int $0xff\n"				\
				"movl %7, %%ebx\n"			\
				: "=a" (ret) : 				\
				"0" (nr), 				\
				"rm" (arg1),				\
				"c" (arg2), 				\
				"d" (arg3), 				\
				"S" (arg4), 				\
				"D" (arg5));				\
		return ret;						\
	}
#endif /* __ASM__ */

#endif /* _SYSCALL_NUMBERS_H */
