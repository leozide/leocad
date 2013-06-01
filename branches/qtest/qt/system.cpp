#include "lc_global.h"
#include "system.h"

bool Sys_KeyDown(int Key)
{
	return false;
}

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

void SystemPumpMessages()
{
}

long SystemGetTicks()
{
    return 0;
}

// User Interface
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

void SystemInit()
{
}

void SystemFinish()
{
}

bool SystemDoDialog(int mode, void* param)
{
  return false;
}

void SystemDoPopupMenu(int nMenu, int x, int y)
{
}

void SystemPieceComboAdd(char* name)
{
}
