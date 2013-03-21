#include "lc_global.h"
#include "system.h"
#include "basewnd.h"

void* Sys_StartMemoryRender(int width, int height)
{
  return NULL;
}

void Sys_FinishMemoryRender(void* param)
{
}

bool Sys_KeyDown(int Key)
{
	return false;
}

void Sys_GetFileList(const char* Path, ObjArray<String>& FileList)
{
}
#ifndef WIN32
// String
char* strupr(char* string)
{
  char *cp;
  for (cp=string; *cp; ++cp)
  {
    if ('a' <= *cp && *cp <= 'z')
      *cp += 'A' - 'a';
  }

  return string;
}

char* strlwr(char* string)
{
  char *cp;
  for (cp = string; *cp; ++cp)
  {
    if ('A' <= *cp && *cp <= 'Z')
      *cp += 'a' - 'A';
  }

  return string;
}

int stricmp(const char* str1, const char* str2)
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

void SystemExportClipboard(lcFile* clip)
{
}

lcFile* SystemImportClipboard()
{
  return NULL;
}

void SystemPieceComboAdd(char* name)
{
}
