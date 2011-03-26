#ifndef __KERNEL_HEADER_INCLUDED__
#define __KERNEL_HEADER_INCLUDED__

/* Required constants */
#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define NULL 		((void *)0)

#include <scaraOS/compiler.h>
#include <scaraOS/types.h>
#include <scaraOS/endian.h>
#include <scaraOS/list.h>
#include <scaraOS/rbtree.h>

#include <arch/user.h>
#include <arch/irq.h>

#include <stdarg.h>
#include <scaraOS/ctype.h>

/* GCC extensions */
#define __init		_section(".text.init")
#define __initdata	_section(".data.init")
#define __init_call	_section(".initcall.init")

/* Common kernel API stuff */
#if DEBUG_MODULE
#define dprintk printk
#else
#define dprintk(x...) do {}while(0)
#endif

void printkv(const char *, va_list);
void hex_dumpk(const uint8_t *tmp, size_t len, size_t llen);
_printf(1, 2) void printk(const char *, ...);
_printf(1, 2) _noreturn void panic(const char *, ...);

#if KDEBUG
#define BUG_ON(x) if ( x ) panic("%s:%s:%u: %s\n", \
			__FILE__, __FUNCTION__, __LINE__, #x)
#else 
#define BUG_ON(x) do {} while(0)
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*x))

/* Driver initialisation */
#define driver_init(fn) __initcall(fn)
typedef void (*initcall_t)(void);
#define __initcall(fn)  extern __init_call initcall_t __initcall_##fn; \
			__init_call initcall_t __initcall_##fn = fn
void do_initcalls(void);
int init_task(void *priv);

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
char *strdup(const char *s);

/* Userspace access */
static inline int strlen_from_user(const char *d)
{
	size_t max;

	max = uaddr_maxstr((vaddr_t)d);
	if ( 0 == max )
		return -1;

	return __strnlen_from_user(d, max);
}

static inline int copy_from_user(char *d, const char *s, size_t c)
{
/*	if ( !uaddr_ok((vaddr_t)s, c) )
		return -1; */
	return __copy_from_user(d, s, c);
}

static inline int copy_to_user(char *d, const char *s, size_t c)
{
/*	if ( !uaddr_ok((vaddr_t)d, c) )
		return -1; */
	return __copy_to_user(d, s, c);
}

_malloc void *kmalloc(size_t sz);
_malloc void *kmalloc0(size_t sz);
void kfree(void *);

static inline char *strdup_from_user(const char *uptr)
{
	char *ret;
	int sz, ssz;

	sz = strlen_from_user(uptr);
	if ( sz <= 0 )
		return NULL;

	sz++;

	ret = kmalloc(sz);
	if ( NULL == ret )
		return NULL;

	ssz = copy_from_user(ret, uptr, sz);
	if ( ssz != sz || ret[sz - 1] != '\0' ) {
		kfree(ret);
		return NULL;
	}

	return ret;
}

#endif /* __KERNEL_HEADER_INCLUDED__ */
