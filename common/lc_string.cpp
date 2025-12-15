#include "lc_global.h"
#include "lc_string.h"

char* lcstrcasestr(const char* s, const char* find)
{
	char c, sc;
	
	if ((c = *find++) != 0)
	{
		c = tolower((unsigned char)c);
		const int len = (int)strlen(find);
		do
		{
			do
			{
				if ((sc = *s++) == 0)
					return (nullptr);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (qstrnicmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char* lcstrupr(char* string)
{
	for (char* c = string; *c; c++)
		*c = toupper(*c);
	
	return string;
}

size_t lcstrcpy(char* dest, const char* source, size_t size)
{
	if (size == 0)
		return 0;
	
	for (size_t i = 0; i < size; i++)
	{
		dest[i] = source[i];
		
		if (source[i] == '\0')
		{
			return i;
		}
	}
	
	dest[--size] = '\0';
	
	return size;
}

size_t lcstrcat(char* dest, const char* source, size_t size)
{
	char* d = dest;
	const char* s = source;
	size_t n = size;
	size_t dlen;
	
	while (n-- != 0 && *d != '\0')
		d++;
	
	dlen = d - dest;
	n = size - dlen;
	
	if (n == 0)
		return(dlen + strlen(s));
	
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	
	*d = '\0';
	
	return (dlen + (s - source));
}
