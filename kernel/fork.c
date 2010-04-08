/* 
 * Contains scheduling primitives such as the scheduler itself,
 * wait queues and eventually semaphores...
*/

#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/fork.h>
#include <scaraOS/vfs.h>

int _sys_fork(unsigned int flags, void (*fn)(void *), void *priv, void *stack)
{
	printk("fork: flags=0x%x fn=%p priv=%p stack=%p\n",
		flags, fn, priv, stack);
	return -1;
}

