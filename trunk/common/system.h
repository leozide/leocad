#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "defines.h"
#include "typedefs.h"

// Profile functions
bool Sys_ProfileSaveInt (const char *section, const char *key, int value);
bool Sys_ProfileSaveString (const char *section, const char *key, const char *value);
int Sys_ProfileLoadInt (const char *section, const char *key, int default_value);
char* Sys_ProfileLoadString (const char *section, const char *key, const char *default_value);

// Memory render
void* Sys_StartMemoryRender (int width, int height);
void Sys_FinishMemoryRender (void* param);

// Cursor functions
//void Sys_GetCursorPos (int *x, int *y);
//void Sys_SetCursorPos (int x, int y);

// FIXME: moved to basewnd, remove

// Message Box
#define LC_OK           1
#define LC_CANCEL       2
#define LC_ABORT        3
#define LC_RETRY        4
#define LC_IGNORE       5
#define LC_YES          6
#define LC_NO           7
 
#define LC_MB_OK                 0x000
#define LC_MB_OKCANCEL           0x001
//#define LC_MB_ABORTRETRYIGNORE 0x002
#define LC_MB_YESNOCANCEL        0x003
#define LC_MB_YESNO              0x004
//#define LC_MB_RETRYCANCEL      0x005

#define LC_MB_ICONERROR          0x010
#define LC_MB_ICONQUESTION       0x020
#define LC_MB_ICONWARNING        0x030
#define LC_MB_ICONINFORMATION    0x040
 
#define LC_MB_TYPEMASK           0x00F
#define LC_MB_ICONMASK           0x0F0

int Sys_MessageBox (const char* text, const char* caption="LeoCAD", int type=LC_MB_OK|LC_MB_ICONINFORMATION);

// FIXME end

// Misc stuff
bool Sys_KeyDown (int key);






class File;
class Camera;
class PieceInfo;

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
void SystemUpdateSnap(unsigned short MoveSnap, unsigned short RotateSnap);
void SystemUpdateSelected(unsigned long flags);
void SystemUpdatePaste(bool enable);
void SystemUpdatePlay(bool play, bool stop);

void SystemInit();
void SystemFinish();
int SystemDoMessageBox(char* prompt, int nMode);
bool SystemDoDialog(int nMode, void* param);
void SystemDoPopupMenu(int nMenu, int x, int y);
void SystemDoWaitCursor(int nCode);

void SystemSetGroup(int nNew);
void SystemSetWindowCaption(char* caption);
//void SystemRedrawView();
void SystemPieceComboAdd(char* name);

void SystemCaptureMouse();
void SystemReleaseMouse();

void SystemExportClipboard(File* clip);
File* SystemImportClipboard();

//void SystemSwapBuffers();
void SystemPumpMessages();
long SystemGetTicks();

void SystemStartProgressBar(int nLower, int nUpper, int nStep, const char* Text);
void SytemEndProgressBar();
void SytemStepProgressBar();

#endif // _SYSTEM_H_
