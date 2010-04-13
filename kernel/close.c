#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <scaraOS/fcntl.h>
#include <arch/mm.h>

int _sys_close(unsigned int handle)
{
	struct task *me;

	me = __this_task;

	fdt_entry_del(me->fd_table, handle);
	return 0;
}
