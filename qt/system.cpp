#include "lc_global.h"

#ifdef WIN32

char* strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0)
	{
		c = tolower((unsigned char)c);
		len = strlen(find);
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

#else

char* strupr(char *string)
{
	for (char *c = string; *c; c++)
		*c = toupper(*c);

	return string;
}

char* strlwr(char *string)
{
	for (char *c = string; *c; c++)
		*c = tolower(*c);

	return string;
}

#endif
