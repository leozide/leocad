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

size_t lcstrcpy(char* dest, const char* source, size_t count)
{
	if (count == 0)
		return 0;
	
	for (size_t i = 0; i < count; i++)
	{
		dest[i] = source[i];
		
		if (source[i] == '\0')
		{
			return i;
		}
	}
	
	dest[--count] = '\0';
	
	return count;
}
