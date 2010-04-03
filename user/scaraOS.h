#ifndef _SCARAOS_H
#define _SCARAOS_H

#include <scaraOS/compiler.h>
#include <arch/types.h>
#include <scaraOS/syscall.h>

#define _SYSCALL1(nr, ret_type, name, type1, name1) \
	ret_type name(type1 name1) 					\
	{								\
		ret_type ret;						\
		asm volatile ("movl %1,%%eax\n"				\
				"movl %2, %%ebx\n"			\
				"int $0xff\n"				\
				"movl %%eax, %0\n"			\
				: "=r" (ret) : "r" (nr), "r" (name1)	\
				: "%eax", "%ebx");			\
		return ret;						\
	}

int main(int argc, char **argv);
int _exit(unsigned int code);

#endif /* _SCARAOS_H */
