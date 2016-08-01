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
class lcTimelineWidget;

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

class lcModelTabWidget : public QWidget
{
	Q_OBJECT

public:
	lcModelTabWidget(lcModel* Model)
	{
		mModel = Model;
		mActiveView = NULL;
	}

	View* GetActiveView() const
	{
		return mActiveView;
	}

	void SetActiveView(View* ActiveView)
	{
		mActiveView = ActiveView;
	}

	void AddView(View* View)
	{
		mViews.Add(View);
	}

	void RemoveView(View* View)
	{
		if (View == mActiveView)
			mActiveView = NULL;

		mViews.Remove(View);
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	const lcArray<View*>* GetViews() const
	{
		return &mViews;
	}

protected:
	lcModel* mModel;
	View* mActiveView;
	lcArray<View*> mViews;
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

	float GetMoveXYSnap() const
	{
		const float SnapXYTable[] = { 0.0f, 1.0f, 5.0f, 8.0f, 10.0f, 20.0f, 40.0f, 60.0f, 80.0f, 160.0f };
		return mMoveSnapEnabled ? SnapXYTable[mMoveXYSnapIndex] : 0.0f;
	}

	float GetMoveZSnap() const
	{
		const float SnapZTable[] = { 0.0f, 1.0f, 5.0f, 8.0f, 10.0f, 20.0f, 24.0f, 48.0f, 96.0f, 192.0f };
		return mMoveSnapEnabled ? SnapZTable[mMoveZSnapIndex] : 0.0f;
	}

	int GetAngleSnap() const
	{
		const int AngleTable[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };
		return mAngleSnapEnabled ? AngleTable[mAngleSnapIndex] : 0.0f;
	}

	QString GetMoveXYSnapText() const
	{
		QString SnapXYText[] = { tr("0"), tr("1/20S"), tr("1/4S"), tr("1F"), tr("1/2S"), tr("1S"), tr("2S"), tr("3S"), tr("4S"), tr("8S") };
		return mMoveSnapEnabled ? SnapXYText[mMoveXYSnapIndex] : tr("None");
	}

	QString GetMoveZSnapText() const
	{
		QString SnapZText[] = { tr("0"), tr("1/20S"), tr("1/4S"), tr("1F"), tr("1/2S"), tr("1S"), tr("1B"), tr("2B"), tr("4B"), tr("8B") };
		return mMoveSnapEnabled ? SnapZText[mMoveZSnapIndex] : tr("None");
	}

	QString GetAngleSnapText() const
	{
		return mAngleSnapEnabled ? QString::number(GetAngleSnap()) : tr("None");
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
		lcModelTabWidget* CurrentTab = (lcModelTabWidget*)mModelTabWidget->currentWidget();
		return CurrentTab ? CurrentTab->GetActiveView() : NULL;
	}

	const lcArray<View*>* GetViewsForModel(lcModel* Model) const
	{
		lcModelTabWidget* TabWidget = GetTabWidgetForModel(Model);
		return TabWidget ? TabWidget->GetViews() : NULL;
	}

	lcModelTabWidget* GetTabForView(View* View) const
	{
		for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
		{
			lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

			int ViewIndex = TabWidget->GetViews()->FindIndex(View);
			if (ViewIndex != -1)
				return TabWidget;
		}

		return NULL;
	}

	QMenu* GetCameraMenu() const
	{
		return mCameraMenu;
	}

	QMenu* GetViewpointMenu() const
	{
		return mViewpointMenu;
	}

	bool DoDialog(LC_DIALOG_TYPE Type, void* Data);

	void RemoveAllModelTabs();
	void SetCurrentModelTab(lcModel* Model);
	void ResetCameras();
	void AddView(View* View);
	void RemoveView(View* View);
	void SetActiveView(View* ActiveView);
	void UpdateAllViews();

	void SetTool(lcTool Tool);
	void SetTransformType(lcTransformType TransformType);
	void SetColorIndex(int ColorIndex);
	void SetMoveSnapEnabled(bool Enabled);
	void SetAngleSnapEnabled(bool Enabled);
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
	bool SetModelFromFocus();
	void SetModelFromSelection();
	void HandleCommand(lcCommandId CommandId);

	void AddRecentFile(const QString& FileName);
	void RemoveRecentFile(int FileIndex);

	void SplitHorizontal();
	void SplitVertical();
	void RemoveActiveView();
	void ResetViews();

	void TogglePrintPreview();
	void ToggleFullScreen();

	void UpdateSelectedObjects(bool SelectionChanged);
	void UpdateTimeline(bool Clear, bool UpdateItems);
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
	void ModelTabClosed(int Index);
	void ModelTabChanged(int Index);
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
	void ShowUpdatesDialog();
	void ShowAboutDialog();
	void ShowPrintDialog();

	lcModelTabWidget* GetTabWidgetForModel(lcModel* Model) const
	{
		for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
		{
			lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

			if (TabWidget->GetModel() == Model)
				return TabWidget;
		}

		return NULL;
	}

	bool mAddKeys;
	lcTool mTool;
	lcTransformType mTransformType;
	bool mMoveSnapEnabled;
	bool mAngleSnapEnabled;
	int mMoveXYSnapIndex;
	int mMoveZSnapIndex;
	int mAngleSnapIndex;
	bool mLockX;
	bool mLockY;
	bool mLockZ;
	bool mRelativeTransform;

	QAction* mActionFileRecentSeparator;

	QTabWidget* mModelTabWidget;
	QToolBar* mStandardToolBar;
	QToolBar* mToolsToolBar;
	QToolBar* mTimeToolBar;
	QDockWidget* mPartsToolBar;
	QDockWidget* mPropertiesToolBar;
	QDockWidget* mTimelineToolBar;

	lcQGLWidget* mPiecePreviewWidget;
	lcQPartsTree* mPartsTree;
	QLineEdit* mPartSearchEdit;
	lcQColorList* mColorList;
	lcQPropertiesTree* mPropertiesWidget;
	lcTimelineWidget* mTimelineWidget;
	QLineEdit* mTransformXEdit;
	QLineEdit* mTransformYEdit;
	QLineEdit* mTransformZEdit;

	QLabel* mStatusBarLabel;
	QLabel* mStatusPositionLabel;
	QLabel* mStatusSnapLabel;
	QLabel* mStatusTimeLabel;

	QMenu* mCameraMenu;
	QMenu* mViewpointMenu;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_
