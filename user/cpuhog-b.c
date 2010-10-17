#include "scaraOS.h"

static inline void __delay(uint32_t loops)
{
	int d0;
	asm volatile (
		"       jmp 1f	\n"
		".align 16	\n"
		"1:     jmp 2f	\n"
		".align 16	\n"
		"2:     decl %0	\n"
		"       jns 2b	\n"
		:"=&a" (d0)
		:"0" (loops));
}

int main(int argc, char **argv)
{
	unsigned int i;

	for(i = 0; i < 50; i++) {
		_write(STDOUT_FILENO, "B", 1);
		__delay(100000000);
	}

	return EXIT_SUCCESS;
}
