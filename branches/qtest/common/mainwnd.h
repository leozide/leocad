#ifndef _MAINWND_H_
#define _MAINWND_H_

#include "basewnd.h"
#include "array.h"
#include "lc_math.h"

class Camera;

extern class lcMainWindow* gMainWindow;

#define LC_MAX_RECENT_FILES 4

class lcMainWindow : public lcBaseWindow
{
public:
	lcMainWindow()
	{
		for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
			mRecentFiles[FileIdx][0] = 0;
	}

	~lcMainWindow()
	{
		gMainWindow = NULL;
	}

	void AddToRecentFiles(const char* FileName)
	{
		int FileIdx;

		for (FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
			if (!strcmp(mRecentFiles[FileIdx], FileName))
				break;

		for (FileIdx = lcMin(FileIdx, LC_MAX_RECENT_FILES - 1); FileIdx > 0; FileIdx--)
			strcpy(mRecentFiles[FileIdx], mRecentFiles[FileIdx - 1]);

		strcpy(mRecentFiles[0], FileName);

		UpdateRecentFiles();
	}

	void RemoveRecentFile(int FileIndex)
	{
		for (int FileIdx = FileIndex; FileIdx < LC_MAX_RECENT_FILES - 1; FileIdx++)
			strcpy(mRecentFiles[FileIdx], mRecentFiles[FileIdx + 1]);

		mRecentFiles[LC_MAX_RECENT_FILES - 1][0] = 0;

		UpdateRecentFiles();
	}

	void UpdateAction(int NewAction);
	void UpdatePaste(bool Enabled);
	void UpdateTime(bool Animation, int CurrentTime, int TotalTime);
	void UpdateAnimation(bool Animation, bool AddKeys);
	void UpdateLockSnap(lcuint32 Snap);
	void UpdateSnap(lcuint16 MoveSnap, lcuint16 RotateSnap);
	void UpdateUndoRedo(const char* UndoText, const char* RedoText);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdateCameraMenu(const PtrArray<Camera>& Cameras, Camera* CurrentCamera);
	void UpdateTitle(const char* Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();

	char mRecentFiles[LC_MAX_RECENT_FILES][LC_MAXPATH];
};

#endif // _MAINWND_H_
