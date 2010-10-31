#include "scaraOS.h"
#include <scaraOS/fork.h>

int main(int argc, char **argv)
{
	static const char * const hello_world = "Hello World!\n";

	if ( _write(STDOUT_FILENO, hello_world, strlen(hello_world)) <= 0 )
		return EXIT_FAILURE;
	_write(STDOUT_FILENO, (void *)0xdeadbeef, 20);

	switch (_fork(FORK_PID|FORK_FD|FORK_FS|FORK_MEM, NULL, NULL, NULL)) {
	case -1:
		_write(STDOUT_FILENO, "Fork failed!\n", 13);
		break;
	case 0:
		_exec("/sbin/cpuhog-b");
		break;
	default:
		_exec("/sbin/cpuhog-a");
		break;
	}

	return EXIT_FAILURE;
}
