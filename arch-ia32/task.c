/* Will eventually contain 386 specific task swiching code I guess */
#include <kernel.h>
#include <task.h>
#include <mm.h>

uint8_t _section(".init.pgalign") idle_task_struct[PAGE_SIZE];

void idle_task_func(void)
{
	asm volatile(
		"1:\n"
		"rep; nop\n"
		"hlt;\n"
		"jmp 1b\n");
}
