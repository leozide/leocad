#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include "lc_basewindow.h"
#include "lc_array.h"
#include "lc_commands.h"

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

	int GetMoveXYSnap() const
	{
		const int SnapXYTable[] = { 0, 1, 5, 8, 10, 20, 40, 60, 80, 160 };
		return SnapXYTable[mMoveXYSnapIndex];
	}

	int GetMoveZSnap() const
	{
		const int SnapZTable[] = { 0, 1, 5, 8, 10, 20, 24, 48, 96, 192 };
		return SnapZTable[mMoveZSnapIndex];
	}

	int GetAngleSnap() const
	{
		const int AngleTable[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };
		return AngleTable[mAngleSnapIndex];
	}

	int GetMoveXYSnapIndex() const
	{
		return mMoveXYSnapIndex;
	}

	int GetMoveZSnapIndex() const
	{
		return mMoveZSnapIndex;
	}

	int GetAngleSnapIndex() const
	{
		return mAngleSnapIndex;
	}

	const char* GetMoveXYSnapText() const
	{
		const char* SnapXYText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "2S", "3S", "4S", "8S" };
		return SnapXYText[mMoveXYSnapIndex];
	}

	const char* GetMoveZSnapText() const
	{
		const char* SnapZText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "1B", "2B", "4B", "8B" };
		return SnapZText[mMoveZSnapIndex];
	}

	bool GetLockX() const
	{
		return mLockX;
	}

	bool GetLockY() const
	{
		return mLockY;
	}

	bool GetLockZ() const
	{
		return mLockZ;
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
	void SetMoveXYSnapIndex(int Index);
	void SetMoveZSnapIndex(int Index);
	void SetAngleSnapIndex(int Index);
	void SetLockX(bool LockX);
	void SetLockY(bool LockY);
	void SetLockZ(bool LockZ);

	void Close();
	void NewProject();
	bool OpenProject(const QString& FileName);
	bool SaveProject(const QString& FileName);
	bool SaveProjectIfModified();
	void HandleCommand(lcCommandId CommandId);

	void AddRecentFile(const QString& FileName);
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
	void UpdateLockSnap();
	void UpdateSnap();
	void UpdateUndoRedo(const QString& UndoText, const QString& RedoText);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdatePerspective();
	void UpdateCameraMenu();
	void UpdateCategories();
	void UpdateTitle(const QString& Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	QString mRecentFiles[LC_MAX_RECENT_FILES];
	PiecePreview* mPreviewWidget;
	int mColorIndex;
	lcSearchOptions mSearchOptions;

protected:
	View* mActiveView;
	lcArray<View*> mViews;

	bool mAddKeys;
	lcTool mTool;
	lcTransformType mTransformType;
	int mMoveXYSnapIndex;
	int mMoveZSnapIndex;
	int mAngleSnapIndex;
	bool mLockX;
	bool mLockY;
	bool mLockZ;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_
