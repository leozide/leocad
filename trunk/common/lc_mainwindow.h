#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include "lc_basewindow.h"
#include "lc_array.h"
#include "lc_commands.h"
#include "lc_model.h"

class View;
class PiecePreview;
class lcQGLWidget;
class lcQPartsTree;
class lcQColorList;
class lcQPropertiesTree;

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

class lcMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	lcMainWindow();
	~lcMainWindow();

	void CreateWidgets();

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

	bool GetRelativeTransform() const
	{
		return mRelativeTransform;
	}

	View* GetActiveView() const
	{
		return mActiveView;
	}

	const lcArray<View*>& GetViews()
	{
		return mViews;
	}

	bool DoDialog(LC_DIALOG_TYPE Type, void* Data);

	int DoMessageBox(const char* Text, int Flags = LC_MB_OK | LC_MB_ICONINFORMATION)
	{
		return DoMessageBox(Text, "LeoCAD", Flags);
	}

	int DoMessageBox(const char* Text, const char* Caption = "LeoCAD", int Flags = LC_MB_OK | LC_MB_ICONINFORMATION);

	void ResetCameras();
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
	void SetRelativeTransform(bool RelativeTransform);

	void NewProject();
	bool OpenProject(const QString& FileName);
	void MergeProject();
	bool SaveProject(const QString& FileName);
	bool SaveProjectIfModified();
	void HandleCommand(lcCommandId CommandId);

	void AddRecentFile(const QString& FileName);
	void RemoveRecentFile(int FileIndex);

	void SplitHorizontal();
	void SplitVertical();
	void RemoveActiveView();
	void ResetViews();

	void TogglePrintPreview();
	void ToggleFullScreen();

	void UpdateFocusObject(lcObject* Focus);
	void UpdateSelectedObjects(int Flags, int SelectedCount, lcObject* Focus);
	void UpdatePaste(bool Enabled);
	void UpdateCurrentStep();
	void SetAddKeys(bool AddKeys);
	void UpdateLockSnap();
	void UpdateSnap();
	void UpdateColor();
	void UpdateUndoRedo(const QString& UndoText, const QString& RedoText);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdatePerspective();
	void UpdateCameraMenu();
	void UpdateModels();
	void UpdateCategories();
	void UpdateTitle();
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	QString mRecentFiles[LC_MAX_RECENT_FILES];
	PiecePreview* mPreviewWidget;
	int mColorIndex;
	lcSearchOptions mSearchOptions;
	QAction* mActions[LC_NUM_COMMANDS];

protected slots:
	void ClipboardChanged();
	void ActionTriggered();
	void PartsTreeItemChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void ColorChanged(int ColorIndex);
	void PartSearchReturn();
	void PartSearchChanged(const QString& Text);
	void Print(QPrinter* Printer);

protected:
	void closeEvent(QCloseEvent *event);
	QMenu* createPopupMenu();

	void CreateActions();
	void CreateMenus();
	void CreateToolBars();
	void CreateStatusBar();
	void SplitView(Qt::Orientation Orientation);
	void ShowPrintDialog();

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
	bool mRelativeTransform;

	QAction* mActionFileRecentSeparator;

	QToolBar* mStandardToolBar;
	QToolBar* mToolsToolBar;
	QToolBar* mTimeToolBar;
	QDockWidget* mPartsToolBar;
	QDockWidget* mPropertiesToolBar;

	lcQGLWidget* mPiecePreviewWidget;
	lcQPartsTree* mPartsTree;
	QLineEdit* mPartSearchEdit;
	lcQColorList* mColorList;
	lcQPropertiesTree* mPropertiesWidget;
	QLineEdit* mTransformXEdit;
	QLineEdit* mTransformYEdit;
	QLineEdit* mTransformZEdit;

	QLabel* mStatusBarLabel;
	QLabel* mStatusPositionLabel;
	QLabel* mStatusSnapLabel;
	QLabel* mStatusTimeLabel;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_
