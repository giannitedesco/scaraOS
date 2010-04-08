#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>
#include <arch/mm.h>
#include <scaraOS/close.h>

int _sys_close(unsigned int handle)
{
	fdt_entry_del(handle);
	return 0;
}
