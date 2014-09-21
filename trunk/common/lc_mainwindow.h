#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include "lc_basewindow.h"
#include "lc_array.h"
#include "project.h"

class View;
class PiecePreview;

#define LC_MAX_RECENT_FILES 4

struct lcSearchOptions
{
	bool MatchInfo;
	bool MatchColor;
	bool MatchName;
	PieceInfo* Info;
	int ColorIndex;
	char Name[256];
};

class lcMainWindow : public lcBaseWindow
{
public:
	lcMainWindow();
	~lcMainWindow();

	lcTool GetTool() const
	{
		return mTool;
	}

	lcTransformType GetTransformType() const
	{
		return mTransformType;
	}

	bool GetAddKeys() const
	{
		return mAddKeys;
	}

	View* GetActiveView() const
	{
		return mActiveView;
	}

	const lcArray<View*>& GetViews()
	{
		return mViews;
	}

	void AddView(View* View);
	void RemoveView(View* View);
	void SetActiveView(View* ActiveView);
	void UpdateAllViews();

	void SetTool(lcTool Tool);
	void SetTransformType(lcTransformType TransformType);
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

	void UpdateFocusObject(lcObject* Focus);
	void UpdateSelectedObjects(int Flags, int SelectedCount, lcObject* Focus);
	void UpdateAction(int NewAction);
	void UpdatePaste(bool Enabled);
	void UpdateCurrentStep();
	void SetAddKeys(bool AddKeys);
	void UpdateLockSnap(lcuint32 Snap);
	void UpdateSnap();
	void UpdateUndoRedo(const QString& UndoText, const QString& RedoText);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdatePerspective();
	void UpdateCameraMenu();
	void UpdateCategories();
	void UpdateTitle(const char* Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	char mRecentFiles[LC_MAX_RECENT_FILES][LC_MAXPATH];
	PiecePreview* mPreviewWidget;
	int mColorIndex;
	lcSearchOptions mSearchOptions;

protected:
	View* mActiveView;
	lcArray<View*> mViews;

	bool mAddKeys;
	lcTool mTool;
	lcTransformType mTransformType;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_
