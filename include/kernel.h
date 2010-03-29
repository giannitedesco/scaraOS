#ifndef __KERNEL_HEADER_INCLUDED__
#define __KERNEL_HEADER_INCLUDED__

/* Required constants */
#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define NULL 		((void *)0)

#include <compiler.h>
#include <endian.h>
#include <list.h>

#include <arch/types.h>
#include <arch/processor.h>
#include <arch/io.h>

#include <stdarg.h>
#include <ctype.h>

/* GCC extensions */
#define __init		_section(".text.init")
#define __initdata	_section(".data.init")
#define __init_call	_section(".initcall.init")

/* Kernel types */
typedef uint32_t pid_t;
typedef uint32_t block_t;
typedef uint32_t ino_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t nlink_t;
typedef uint32_t loff_t;
typedef uint32_t umode_t;

/* Common kernel API stuff */
#if DEBUG_MODULE
#define dprintk printk
#else
#define dprintk(x...) do {}while(0);
#endif

void printk(const char *, ...);
_noreturn void panic(void);

/* Driver initialisation */
#define driver_init(fn) __initcall(fn)
typedef void (*initcall_t)(void);
#define __initcall(fn) __init_call initcall_t __initcall_##fn = fn

/* vsprintf */
unsigned long simple_strtoul(const char *,char **,unsigned int);
long simple_strtol(const char *,char **,unsigned int);
unsigned long long simple_strtoull(const char *,char **,unsigned int);
long long simple_strtoll(const char *,char **,unsigned int);
int sprintf(char * buf, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
int vsprintf(char *buf, const char *, va_list);
int snprintf(char * buf, size_t size, const char * fmt, ...)
	__attribute__ ((format (printf, 3, 4)));
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vsscanf(const char *, const char *, va_list);
int sscanf(const char *, const char *, ...)
	__attribute__ ((format (scanf,2,3)));

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

#endif /* __KERNEL_HEADER_INCLUDED__ */
