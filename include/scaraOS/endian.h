#ifndef __ENDIAN_HEADER_INCLUDED__
#define __ENDIAN_HEADER_INCLUDED__

#define __bswap16(x) \
	((uint16_t)( \
		(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) | \
	 	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ))
#define __bswap32(x) \
	((uint32_t)( \
		(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
		(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
		(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
		(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define htobe16(x) __bswap16 (x)
#  define htole16(x) (x)
#  define be16toh(x) __bswap16 (x)
#  define le16toh(x) (x)

#  define htobe32(x) __bswap32 (x)
#  define htole32(x) (x)
#  define be32toh(x) __bswap32 (x)
#  define le32toh(x) (x)

#  define htobe64(x) __bswap64 (x)
#  define htole64(x) (x)
#  define be64toh(x) __bswap64 (x)
#  define le64toh(x) (x)
# else
#  define htobe16(x) (x)
#  define htole16(x) __bswap16 (x)
#  define be16toh(x) (x)
#  define le16toh(x) __bswap16 (x)

#  define htobe32(x) (x)
#  define htole32(x) __bswap32 (x)
#  define be32toh(x) (x)
#  define le32toh(x) __bswap32 (x)

#  define htobe64(x) (x)
#  define htole64(x) __bswap64 (x)
#  define be64toh(x) (x)
#  define le64toh(x) __bswap64 (x)
# endif

#endif /* __ENDIAN_HEADER_INCLUDED__ */
