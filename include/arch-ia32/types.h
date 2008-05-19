#ifndef __KERNEL_TYPES_INCLUDED__
#define __KERNEL_TYPES_INCLUDED__

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned long uint32_t;
typedef signed long int32_t;

__extension__ typedef unsigned long long uint64_t;
__extension__ typedef signed long long int64_t;

typedef uint32_t size_t;

#define BITS_PER_LONG 32

/* Not strictly types, but attributes */
#define asmlinkage __attribute__((regparm(0)))
#define __cacheline_aligned __attribute__((aligned(32)))

#endif /* __KERNEL_TYPES_INCLUDED__ */
