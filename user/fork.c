#include "scaraOS.h"
#include <scaraOS/fork.h>

pid_t fork(void)
{
	int ret;

	ret = _fork(FORK_MEM|FORK_PID|FORK_FD|FORK_FS|FORK_NOFUNC,
			NULL, NULL, NULL);
	if ( ret < 0 )
		return -1;
	if ( ret == 0 )
		return 0;
	return ret;
}
