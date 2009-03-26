#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "opengl.h"
#include "camera.h"
#include "project.h"
#include "system.h"
#include "globals.h"
#include "lc_application.h"

// =============================================================================
// Cursor functions

int Sys_MessageBox (const char* text, const char* caption, int type)
{
	int mode = (type & LC_MB_TYPEMASK);
	int ret;
	
	if (mode == LC_MB_OK)
		ret = LC_OK;
	else if (mode == LC_MB_OKCANCEL)
		ret = LC_CANCEL;	
	else /* if (mode == LC_MB_YESNO) */
		ret = LC_NO;
	
	return ret;
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
	return strcasecmp(str1, str2);
}

void SystemPumpMessages()
{
}

long SystemGetTicks()
{
  static int basetime = 0;
  struct timezone tzp;
  struct timeval tp;

  gettimeofday (&tp, &tzp);

  if (!basetime)
    basetime = tp.tv_sec;

  return (tp.tv_sec-basetime)*1000 + tp.tv_usec/1000;
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

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
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
int SystemDoMessageBox(const char* prompt, int mode)
{
	return Sys_MessageBox(prompt, "", mode);
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

void SystemPieceComboAdd(char* name)
{
}

void SystemCaptureMouse()
{
}

void SystemReleaseMouse()
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

bool Sys_ProfileSaveInt(const char *section, const char *key, int value)
{
	return true;
}

bool Sys_ProfileSaveString(const char *section, const char *key, const char *value)
{
	return true;
}

int Sys_ProfileLoadInt (const char *section, const char *key, int default_value)
{
	return default_value;
}

char* Sys_ProfileLoadString(const char *section, const char *key, const char *default_value)
{
	return (char*)default_value;
}

void* Sys_GLGetExtension(const char *symbol)
{
	return NULL;
}

bool Sys_GLOpenLibrary(const char* libname)
{
	return true;
}

void Sys_GLCloseLibrary()
{
}
