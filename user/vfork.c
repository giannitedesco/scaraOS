#include "scaraOS.h"
#include <scaraOS/fork.h>

pid_t vfork(void)
{
	int ret;

	/* This is totally fucked if child process does not behave */
	ret = _fork(FORK_PID|FORK_FD|FORK_FS,
			NULL, NULL, NULL);
	if ( ret < 0 )
		return -1;
	if ( ret == 0 )
		return 0;
	return ret;
}
