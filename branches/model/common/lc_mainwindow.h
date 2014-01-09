#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include "lc_basewindow.h"
#include "lc_array.h"
#include "lc_model.h"

class View;
class Object;
class Camera;
class PiecePreview;

#define LC_MAX_RECENT_FILES 4

class lcMainWindow : public lcBaseWindow
{
 public:
	lcMainWindow();
	~lcMainWindow();

	void SetActiveView(View* ActiveView);
	void UpdateAllViews();
	void SetColorIndex(int ColorIndex);
	void Close();

	void AddRecentFile(const char* FileName);
	void RemoveRecentFile(int FileIndex);

	void SplitHorizontal();
	void SplitVertical();
	void RemoveView();
	void ResetViews();

	void TogglePrintPreview();
	void ToggleFullScreen();

	void UpdateSelection();
	void UpdateFocusObject();
	void UpdateCameraMenu();
	void UpdateModelMenu();
	void UpdateCheckpoint();
	void UpdateTransformMode();
	void UpdateCurrentTool();
	void UpdateAddKeys();

	void UpdateSelectedObjects(int Flags, int SelectedCount, Object* Focus);
	void UpdatePaste(bool Enabled);
	void UpdateCurrentTime();
	void UpdateLockSnap(lcuint32 Snap);
	void UpdateSnap();
	void UpdateUndoRedo(const char* UndoText, const char* RedoText);
	void UpdateCategories();
	void UpdateTitle(const char* Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	void SetPieceDrag(bool Drag)
	{
		mPieceDrag = Drag;
	}

	void SetTransformMode(lcTransformMode TransformMode)
	{
		mTransformMode = TransformMode;
		UpdateTransformMode();
	}

	lcVector3 GetTransformAmount();

	void SetCurrentTool(lcTool Tool)
	{
		mCurrentTool = Tool;
		UpdateCurrentTool();
		UpdateAllViews();
	}

	lcTool GetCurrentTool() const
	{
		if (mPieceDrag)
			return LC_TOOL_INSERT;
		else
			return mCurrentTool;
	}

	void SetAddKeys(bool AddKeys)
	{
		mAddKeys = AddKeys;
		UpdateAddKeys();
	}

	char mRecentFiles[LC_MAX_RECENT_FILES][LC_MAXPATH];

	View* mActiveView;
	lcArray<View*> mViews;
	PiecePreview* mPreviewWidget;

	int mColorIndex;
	lcTool mCurrentTool;
	lcTransformMode mTransformMode;
	bool mAddKeys;
	bool mPieceDrag;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_
