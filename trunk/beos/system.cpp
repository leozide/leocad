/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "opengl.h"
#include "gdkgl.h"
#include "gtkglarea.h"
#include "gtkmisc.h"
#include "project.h"
#include "system.h"
#include "main.h"
#include "toolbar.h"
#include "dialogs.h"
#include "globals.h"
*/
#include "file.h"
#include "camera.h"
#include "defines.h"

// =============================================================================
// Cursor functions

/*
void Sys_BeginWait ()
{
  GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (g_pParentWnd->m_pWidget->window, cursor);
  gdk_cursor_destroy (cursor);
}

void Sys_EndWait ()
{
  GdkCursor *cursor = gdk_cursor_new (GDK_LEFT_PTR);
  gdk_window_set_cursor (g_pParentWnd->m_pWidget->window, cursor);
  gdk_cursor_destroy (cursor);
}

void Sys_GetCursorPos (int *x, int *y)
{
  gdk_window_get_pointer (NULL, x, y, NULL);
}

void Sys_SetCursorPos (int x, int y)
{
  XWarpPointer (GDK_DISPLAY(), None, GDK_ROOT_WINDOW(), 0, 0, 0, 0, x, y);
}
*/

// =============================================================================
// Message Boxes

int Sys_MessageBox (const char* text, const char* caption, int type)
{
  return 0;
}

// =============================================================================
// Memory rendering

typedef struct
{
  int width, height;
} LC_RENDER;

void* Sys_StartMemoryRender(int width, int height)
{
  return NULL;
}

void Sys_FinishMemoryRender(void* param)
{
}

// Profile functions
bool Sys_ProfileSaveInt (const char *section, const char *key, int value)
{
  return false;
}

bool Sys_ProfileSaveString (const char *section, const char *key, const char *value)
{
  return false;
}

int Sys_ProfileLoadInt (const char *section, const char *key, int default_value)
{
  return default_value;
}

char* Sys_ProfileLoadString (const char *section, const char *key, const char *default_value)
{
  static char tmp[1024];
  strcpy (tmp, default_value);
  return tmp;
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





void SystemPumpMessages()
{
}

long SystemGetTicks()
{
  return 0;//GetTickCount();
}

// User Interface
void SystemUpdateViewport(int new_vp, int old_vp)
{
}

void SystemUpdateAction(int new_action, int old_action)
{
}

void SystemSetGroup(int new_group)
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

void SystemUpdateMoveSnap(unsigned short move_snap)
{
}

void SystemUpdateSelected(unsigned long flags)
{
}

void SystemUpdateRecentMenu(char names[4][LC_MAXPATH])
{
}

void SystemUpdatePaste(bool enable)
{
}

void SystemUpdatePlay(bool play, bool stop)
{
}

void SystemUpdateFocus(void* object, unsigned char type)
{
}

void SystemInit()
{
}

void SystemFinish()
{
}

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
