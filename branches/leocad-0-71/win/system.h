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

void Export3DStudio();

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

// Message box
#define LC_OK		IDOK
#define LC_CANCEL	IDCANCEL
#define LC_ABORT	IDABORT
#define LC_RETRY	IDRETRY
#define LC_IGNORE	IDIGNORE
#define LC_YES		IDYES
#define LC_NO		IDNO

#define LC_MB_OK				MB_OK
#define LC_MB_OKCANCEL			MB_OKCANCEL
#define LC_MB_ABORTRETRYIGNORE	MB_ABORTRETRYIGNORE
#define LC_MB_YESNOCANCEL		MB_YESNOCANCEL
#define LC_MB_YESNO				MB_YESNO
#define LC_MB_RETRYCANCEL		MB_RETRYCANCEL
#define LC_MB_ICONERROR			MB_ICONHAND
#define LC_MB_ICONQUESTION		MB_ICONQUESTION
#define LC_MB_ICONWARNING		MB_ICONEXCLAMATION
#define LC_MB_ICONINFORMATION	MB_ICONASTERISK

void SystemInit();
void SystemFinish();
int SystemDoMessageBox(char* prompt, int nMode);
bool SystemDoDialog(int nMode, void* param);
void SystemDoPopupMenu(int nMenu, int x, int y);
void SystemDoWaitCursor(int nCode);

void SystemSetWindowCaption(char* caption);
void SystemRedrawView();
void SystemPieceComboAdd(char* name);
inline void SystemSetGroup(int group)
{
	AfxGetMainWnd()->PostMessage (WM_LC_UPDATE_LIST, group+2, 0);
}

void SystemExportClipboard(File* clip);
File* SystemImportClipboard();

void SystemCaptureMouse();
void SystemReleaseMouse();

inline void SystemPumpMessages()
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

inline long SystemGetTicks()
{
	return GetTickCount();
}

inline void SystemSwapBuffers()
{
	SwapBuffers(wglGetCurrentDC());
}

inline bool IsKeyDown(int key)
{
	return (GetKeyState(key) < 0);
}

#endif // _SYSTEM_H_
