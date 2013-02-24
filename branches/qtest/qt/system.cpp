#include "lc_global.h"
#include "system.h"
#include "basewnd.h"

int Sys_MessageBox(const char* text, const char* caption, int type)
{
    return LC_MB_OK;
}

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
void SystemUpdateCategories(bool SearchOnly)
{
}

void SystemUpdateColorList(int new_color)
{
}

void SystemUpdateTime(bool bAnimation, int nTime, int nTotal)
{
}

void SystemUpdateAnimation(bool bAnimation, bool bAddKeys)
{
}

void SystemUpdateSnap(unsigned short move_snap, unsigned short RotateSnap)
{
}

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
{
}

void SystemUpdateRecentMenu (String names[4])
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

void SystemStartProgressBar(int nLower, int nUpper, int nStep, const char* Text)
{
}

void SytemEndProgressBar()
{
}

void SytemStepProgressBar()
{
}
