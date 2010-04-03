#include "scaraOS.h"

int main(int argc, char **argv)
{
	const char *hello_world = "Hello World!\n";
	if ( _write(STDOUT_FILENO, hello_world, strlen(hello_world)) )
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
