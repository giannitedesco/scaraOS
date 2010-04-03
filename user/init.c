#include "scaraOS.h"

int main(int argc, char **argv)
{
	static const char * const hello_world = "Hello World!\n";

	if ( _write(STDOUT_FILENO, hello_world, strlen(hello_world)) <= 0 )
		return EXIT_FAILURE;
	for(;;)
		/* nothing */

	return EXIT_SUCCESS;
}
