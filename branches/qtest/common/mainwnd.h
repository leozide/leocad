#ifndef _MAINWND_H_
#define _MAINWND_H_

#include "basewnd.h"
#include "array.h"

class Camera;
class PiecePreview;

#define LC_MAX_RECENT_FILES 4

class lcMainWindow : public lcBaseWindow
{
public:
	lcMainWindow();
	~lcMainWindow();

	void AddRecentFile(const char* FileName);
	void RemoveRecentFile(int FileIndex);

	void UpdateAction(int NewAction);
	void UpdatePaste(bool Enabled);
	void UpdateTime(bool Animation, int CurrentTime, int TotalTime);
	void UpdateAnimation(bool Animation, bool AddKeys);
	void UpdateLockSnap(lcuint32 Snap);
	void UpdateSnap();
	void UpdateUndoRedo(const char* UndoText, const char* RedoText);
	void UpdateTransformType(int NewType);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdateCameraMenu(const PtrArray<Camera>& Cameras, Camera* CurrentCamera);
	void UpdateCategories();
	void UpdateTitle(const char* Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	char mRecentFiles[LC_MAX_RECENT_FILES][LC_MAXPATH];
	PiecePreview* mPreviewWidget;
};

extern class lcMainWindow* gMainWindow;

#endif // _MAINWND_H_
