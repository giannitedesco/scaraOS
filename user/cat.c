#include "scaraOS.h"

int main (int argc, char **argv)
{
  static const char * const filename = "/test.txt";
  int FH;
  char *buf;
  int len;

  FH = _open(filename, 0);
  if ( FH == 1 ) {
    _write(STDOUT_FILENO, "Failed to open target.\n", 23);
    return EXIT_FAILURE;
  }

  len = _read(FH, buf, 512);
  if ( len < 0 ) {
    _write(STDOUT_FILENO, "Failed to read target.\n", 23);
    _close(FH);
    return EXIT_FAILURE;
  }

  _write(STDOUT_FILENO, buf, len);
  _close(FH);

  return EXIT_SUCCESS;
}
