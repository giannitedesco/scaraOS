/*
 * String library - mainly copied directly from linux, no reason really
 * just that I have the code handy etc...
*/
#include <kernel.h>

size_t strlen(const char *s)
{
	size_t ret;

	for(ret = 0; *s; s++, ret++)
		/* nothing */;
	
	return ret;
}

size_t strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

int strcmp(const char *cs, const char *ct)
{
	int8_t _res;

	while (1) {
		if ((_res = *cs - *ct++) != 0 || !*cs++)
			break;
	}

	return _res;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const uint8_t *cs = s1, *ct = s2;
	int ret;

	for(ret = 0, cs = s1, ct = s2; n; --n, cs++, ct++) {
		ret = *cs - *ct;
		if ( ret )
			return ret;
	}

	return ret;
}

#if 0
int strcasecmp(const char *cs, const char *ct)
{
	char _res;

	while (1) {
		if ((_res = (tolower(*cs) - tolower(*ct++))) != 0 || !*cs++)
			break;
	}

	return _res;
}
#endif

void memcpy(void *dst, const void *src, size_t n)
{
	const uint8_t *s;
	uint8_t *d;

	for (s = src, d = dst; n; --n, s++, d++)
		*d = *s;
}

char *strchr(const char *str, int c)
{
	for(; *str; str++)
		if ( c == *str )
			return (char *)str;
	return NULL;
}

void memset(void *dst, int c, size_t n)
{
	uint8_t *d = dst;
	for(; n; --n, d++)
		*d = c;
}

#if 0
void memmove(void *dst, const void *src, size_t n)
{
}
#endif
