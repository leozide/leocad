#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "camera.h"
#include "str.h"

int Sys_MessageBox (const char* text, const char* caption, int type)
{
	return 0;
}

// =============================================================================
// Memory rendering

void* Sys_StartMemoryRender(int width, int height)
{
	return NULL;
}

void Sys_FinishMemoryRender(void* param)
{
}

// =============================================================================
// Misc stuff

// FIXME: should have a table of LC_KEY_* defined
bool Sys_KeyDown (int key)
{
  return false;
}

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
	return strcmp(str1, str2);
}

// Profile functions
bool Sys_ProfileSaveInt (const char *section, const char *key, int value)
{
  return true;
}

bool Sys_ProfileSaveString (const char *section, const char *key, const char *value)
{
  return true;
}

int Sys_ProfileLoadInt (const char *section, const char *key, int default_value)
{
  return default_value;
}

char* Sys_ProfileLoadString (const char *section, const char *key, const char *default_value)
{
  return (char*)default_value;
}





void SystemPumpMessages()
{
}

long SystemGetTicks()
{
	return 0;
}

// User Interface
void SystemUpdateViewport(int new_vp, int old_vp)
{
}

void SystemUpdateCategories(bool SearchOnly)
{
}

void SystemUpdateAction(int new_action, int old_action)
{
}

void SystemUpdateColorList(int new_color)
{
}

void SystemUpdateRenderingMode(bool bBackground, bool bFast)
{
}

void SystemUpdateUndoRedo(char* undo, char* redo)
{
}

void SystemUpdateSnap(const unsigned long snap)
{
}

void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, Camera* pCamera)
{
}

void SystemUpdateCameraMenu(Camera* pCamera)
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

void SystemUpdateSelected(unsigned long flags, int SelectedCount, class Object* Focus)
{
}

void SystemUpdateRecentMenu (String names[4])
{
}

void SystemUpdatePaste(bool enable)
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

// FIXME: remove 
int SystemDoMessageBox(char* prompt, int mode)
{
	return 0;
}

bool SystemDoDialog(int mode, void* param)
{
  return false;
}

void SystemDoPopupMenu(int nMenu, int x, int y)
{
}

void SystemDoWaitCursor(int code)
{
}

void SystemExportClipboard(File* clip)
{
}

File* SystemImportClipboard()
{
  return NULL;
}

void SystemSetWindowCaption(char* caption)
{
}

void SystemRedrawView()
{
}

void SystemPieceComboAdd(char* name)
{
}


void SystemCaptureMouse()
{
}

void SystemReleaseMouse()
{
}

void SystemSwapBuffers()
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
