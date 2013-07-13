#include "lc_global.h"
#include "system.h"

#ifndef WIN32

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

int stricmp(const char *str1, const char *str2)
{
	return strcasecmp(str1, str2);
}

#endif

void SystemUpdateColorList(int new_color)
{
}

void SystemUpdateAnimation(bool bAnimation, bool bAddKeys)
{
}

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
{
}

void SystemUpdatePlay(bool play, bool stop)
{
}

void SystemDoPopupMenu(int nMenu, int x, int y)
{
}

void SystemPieceComboAdd(char* name)
{
}
