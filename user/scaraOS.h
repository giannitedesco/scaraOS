#ifndef _SCARAOS_H
#define _SCARAOS_H

#include <scaraOS/compiler.h>
#include <arch/types.h>
#include <scaraOS/syscall.h>

/* CRT */
int main(int argc, char **argv);

/* stdlib */
#define NULL 		((void *)0)
#define EXIT_SUCCESS 	0
#define EXIT_FAILURE 	1
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/* syscalls */
int _exit(unsigned int code);
int _exec(const char *path);
int _open(const char *fn, unsigned int mode);
int _close(int fd);
ssize_t _read(int fd, void *ptr, size_t count);
ssize_t _write(int fd, const void *ptr, size_t count);

/* string */
void itoa(char *buf, int base, int d);
int strcmp(const char * cs,const char * ct);
size_t strnlen(const char * s, size_t count);
size_t strlen(const char *s);
int strcmp(const char *cs, const char *ct);
int memcmp(const void *s1, const void *s2, size_t n);
void memcpy(void *dst, const void *src, size_t n);
char *strchr(const char *str, int c);
void memset(void *dst, int c, size_t n);

#endif /* _SCARAOS_H */
