#ifndef _COMPILER_HEADER_INCLUDED_
#define _COMPILER_HEADER_INCLUDED_

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define _private __attribute__((visibility("hidden")))
#define _protected __attribute__((visibility("protected")))
#endif

#if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96 )
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#endif

#ifdef __WIN32__
#define _public __declspec(dllexport)
#endif

#if __GNUC__
#define _section(x) __attribute__((__section__(x)))
#define _asmlinkage __attribute__((regparm(0)))
#define _noreturn __attribute__((noreturn))
#define _purefn __attribute__((pure))
#define _printf(x,y) __attribute__((format(printf,x,y)))
#endif

#if __GNUC__ > 2
#define _nonull(x...) __attribute__((nonnull (x)))
#define _constfn __attribute__((const))
#define _malloc_nocheck __attribute__((malloc))
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define _check_result __attribute__((warn_unused_result))
#endif

#ifndef _private
#define _private
#endif

#ifndef _hidden
#define _hidden
#endif

#ifndef _public
#define _public
#endif

#ifndef _noreturn
#define _noreturn
#endif

#ifndef _purefn
#define _purefn
#endif

#ifndef _printf
#define _printf(x,y)
#endif

#ifndef _nonull
#define _nonull(x...)
#endif

#ifndef _malloc_nocheck
#define _malloc_nocheck
#endif

#ifndef _constfn
#define _constfn
#endif

#ifndef _check_result
#define _check_result
#endif

#ifndef _malloc
#define _malloc _malloc_nocheck _check_result
#endif

#ifndef likely
#define likely(x) (x)
#endif

#ifndef unlikely
#define unlikely(x) (x)
#endif

#ifndef _asmlinkage
#define _asmlinkage
#endif

#define BITMASK_ANY (-1UL)
typedef unsigned long bitmask_t;

#endif /* _COMPILER_HEADER_INCLUDED_ */
