#ifndef __ENDIAN_HEADER_INCLUDED__
#define __ENDIAN_HEADER_INCLUDED__

#define bswap16(x) \
	((uint16_t)( \
		(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) | \
	 	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ))
#define bswap32(x) \
	((uint32_t)( \
		(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
		(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
		(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
		(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __swap_16(x) bswap16(x)
#define __swap_32(x) bswap32(x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __swap_16(x) (x)
#define __swap_32(x) (x)
#else
#error "couldn't determine endianness"
#endif

/* Use to initialise constants */
#define htons(x) __swap_16(x)
#define ntohs(x) __swap_16(x)
#define htonl(x) __swap_32(x)
#define ntohl(x) __swap_32(x)

#endif /* __ENDIAN_HEADER_INCLUDED__ */
