#include "scaraOS.h"

int main(int argc, char **argv)
{
	static const char * const filename = "/README";
	static char buf[512];
	int fd;
	int len;

	fd = _open(filename, 0);
	if ( fd == 1 ) {
		_write(STDOUT_FILENO, "Failed to open target.\n", 23);
		return EXIT_FAILURE;
	}

again:
	len = _read(fd, buf, sizeof(buf));
	if ( len < 0 ) {
		_write(STDOUT_FILENO, "Failed to read target.\n", 23);
		_close(fd);
		return EXIT_FAILURE;
	}

	_write(STDOUT_FILENO, buf, len);
	if ( len != 0 )
		goto again;

	_close(fd);

	return EXIT_SUCCESS;
}
