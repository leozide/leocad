//
//	system.h
////////////////////////////////////////////////////

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

class File;
class Camera;
class PieceInfo;
#include "defines.h"
#include "typedefs.h"

// Profile
int SystemGetProfileInt(const char* section, const char* entry, const int defaultvalue);
bool SystemSetProfileInt(const char* section, const char* entry, const int value);
bool SystemSetProfileString(const char* section, const char* entry, const char* value);
const char* SystemGetProfileString(const char* section, const char* entry, const char* defaultvalue);

// User Interface
void SystemUpdateViewport(int nNew, int nOld);
void SystemUpdateAction(int nNew, int nOld);
void SystemUpdateColorList(int nNew);
void SystemUpdateRenderingMode(bool bBackground, bool bFast);
void SystemUpdateUndoRedo(char* undo, char* redo);
void SystemUpdateSnap(const unsigned long nSnap);
void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, Camera* pCamera);
void SystemUpdateCameraMenu(Camera* pCamera);
void SystemUpdateTime(bool bAnimation, int nTime, int nTotal);
void SystemUpdateAnimation(bool bAnimation, bool bAddKeys);
void SystemUpdateMoveSnap(unsigned short nMoveSnap);
void SystemUpdateSelected(unsigned long flags);
void SystemUpdateRecentMenu(char names[4][LC_MAXPATH]);
void SystemUpdatePaste(bool enable);
void SystemUpdatePlay(bool play, bool stop);
void SystemUpdateFocus(void* object, unsigned char type);

// Memory render
void* SystemStartRender(int width, int height);
void SystemFinishRender(void* param);
LC_IMAGE* SystemGetRenderImage(void* param);

#define LC_OK		1
#define LC_CANCEL       2
#define LC_ABORT	3
#define LC_RETRY	4
#define LC_IGNORE	5
#define LC_YES		6
#define LC_NO		7

#define LC_MB_OK                 0x001
#define LC_MB_OKCANCEL           0x002
//#define LC_MB_ABORTRETRYIGNORE 0x004
#define LC_MB_YESNOCANCEL	 0x008
#define LC_MB_YESNO	         0x010
//#define LC_MB_RETRYCANCEL	 0x020
#define LC_MB_ICONERROR		 0x100
#define LC_MB_ICONQUESTION	 0x200
#define LC_MB_ICONWARNING	 0x400
#define LC_MB_ICONINFORMATION    0x800

#define LC_MB_TYPEMASK           0x0FF
#define LC_MB_ICONMASK           0xF00

void SystemInit();
void SystemFinish();
int SystemDoMessageBox(char* prompt, int nMode);
bool SystemDoDialog(int nMode, void* param);
void SystemDoPopupMenu(int nMenu, int x, int y);
void SystemDoWaitCursor(int nCode);

void SystemSetGroup(int nNew);
void SystemSetWindowCaption(char* caption);
void SystemRedrawView();
void SystemPieceComboAdd(char* name);

void SystemCaptureMouse();
void SystemReleaseMouse();

void SystemExportClipboard(File* clip);
File* SystemImportClipboard();

inline void SystemPumpMessages()
{
}

inline long SystemGetTicks()
{
  return 0;//GetTickCount();
}

void SystemSwapBuffers();

inline bool IsKeyDown(int key)
{
  return false;//(GetKeyState(key) < 0);
}

#endif // _SYSTEM_H_







