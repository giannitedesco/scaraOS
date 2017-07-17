/*
 * String library - mainly copied directly from linux, no reason really
 * just that I have the code handy etc...
*/
#include "scaraOS.h"

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal.  */
void itoa(char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor = 10;

	/* If %d is specified and D is minus, put `-' in the head.  */
	if (base == 'd' && d < 0)
	{
		*p++ = '-';
		buf++;
		ud = -d;
	}else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0.  */
	do
	{
		int remainder = ud % divisor;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	}while (ud /= divisor);

	/* Terminate BUF.  */
	*p = 0;

	/* Reverse BUF.  */
	p1 = buf;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}

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
