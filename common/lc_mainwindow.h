#pragma once

#include "lc_application.h"
#include "lc_shortcuts.h"
#include "lc_array.h"
#include "lc_commands.h"
#include "lc_model.h"

class lcPartSelectionWidget;
class lcPreviewDockWidget;
class PiecePreview;
class lcQPartsTree;
class lcColorList;
class lcPropertiesWidget;
class lcTimelineWidget;
class lcElidedLabel;
#ifdef QT_NO_PRINTER
class QPrinter;
#endif

#define LC_MAX_RECENT_FILES 4

class lcTabBar : public QTabBar
{
public:
	lcTabBar(QWidget* Parent = nullptr)
		: QTabBar(Parent), mMousePressTab(-1)
	{
	}

protected:
	void mousePressEvent(QMouseEvent* Event) override;
	void mouseReleaseEvent(QMouseEvent* Event) override;

	int mMousePressTab;
};

class lcModelTabWidget : public QWidget
{
	Q_OBJECT

public:
	lcModelTabWidget(lcModel* Model)
	{
		mModel = Model;
		mActiveView = nullptr;
	}

	QWidget* GetAnyViewWidget()
	{
		QWidget* Widget = layout()->itemAt(0)->widget();

		while (Widget->metaObject() == &QSplitter::staticMetaObject)
			Widget = ((QSplitter*)Widget)->widget(0);

		return Widget;
	}

	lcView* GetActiveView() const
	{
		return mActiveView;
	}

	void SetActiveView(lcView* ActiveView)
	{
		mActiveView = ActiveView;
	}

	void RemoveView(const lcView* View)
	{
		if (View == mActiveView)
			mActiveView = nullptr;
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

protected:
	lcModel* mModel;
	lcView* mActiveView;
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

	float GetAngleSnap() const
	{
		const float AngleTable[] = { 0.0f, 1.0f, 5.0f, 15.0f, 22.5f, 30.0f, 45.0f, 60.0f, 90.0f, 180.0f };
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

	bool GetRelativeTransform() const
	{
		return mRelativeTransform;
	}

	bool GetSeparateTransform() const
	{
		return mLocalTransform;
	}

	lcSelectionMode GetSelectionMode() const
	{
		return mSelectionMode;
	}

	PieceInfo* GetCurrentPieceInfo() const
	{
		return mCurrentPieceInfo;
	}

	lcView* GetActiveView() const
	{
		const lcModelTabWidget* const CurrentTab = mModelTabWidget ? (lcModelTabWidget*)mModelTabWidget->currentWidget() : nullptr;
		return CurrentTab ? CurrentTab->GetActiveView() : nullptr;
	}

	lcModel* GetActiveModel() const;
	lcModelTabWidget* GetTabForView(lcView* View) const;

	lcModel* GetCurrentTabModel() const
	{
		const lcModelTabWidget* const CurrentTab = (lcModelTabWidget*)mModelTabWidget->currentWidget();
		return CurrentTab ? CurrentTab->GetModel() : nullptr;
	}

	lcPartSelectionWidget* GetPartSelectionWidget() const
	{
		return mPartSelectionWidget;
	}

	lcPreviewDockWidget* GetPreviewWidget() const
	{
		return mPreviewWidget;
	}

	QMenu* GetToolsMenu() const
	{
		return mToolsMenu;
	}

	QMenu* GetViewpointMenu() const
	{
		return mViewpointMenu;
	}

	QMenu* GetCameraMenu() const
	{
		return mCameraMenu;
	}

	QMenu* GetProjectionMenu() const
	{
		return mProjectionMenu;
	}

	QMenu* GetShadingMenu() const
	{
		return mShadingMenu;
	}

	QByteArray GetTabLayout();
	void RestoreTabLayout(const QByteArray& TabLayout);
	void RemoveAllModelTabs();
	void CloseCurrentModelTab();
	void SetCurrentModelTab(lcModel* Model);
	void ResetCameras();
	void AddView(lcView* View);
	void RemoveView(lcView* View);

	void SetTool(lcTool Tool);
	void SetTransformType(lcTransformType TransformType);
	void SetColorIndex(int ColorIndex);
	void SetMoveSnapEnabled(bool Enabled);
	void SetAngleSnapEnabled(bool Enabled);
	void SetMoveXYSnapIndex(int Index);
	void SetMoveZSnapIndex(int Index);
	void SetAngleSnapIndex(int Index);
	void SetRelativeTransform(bool RelativeTransform);
	void SetSeparateTransform(bool SelectionTransform);
	void SetCurrentPieceInfo(PieceInfo* Info);
	void SetShadingMode(lcShadingMode ShadingMode);
	void SetSelectionMode(lcSelectionMode SelectionMode);
	void ToggleViewSphere();
	void ToggleAxisIcon();
	void ToggleGrid();
	void ToggleFadePreviousSteps();

	void NewProject();
	bool OpenProject(const QString& FileName);
	void OpenRecentProject(int RecentFileIndex);
	void MergeProject();
	void ImportLDD();
	void ImportInventory();
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
	void UpdateShadingMode();
	void UpdateSelectionMode();
	void UpdateModels();
	void UpdateCategories();
	void UpdateTitle();
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	QString mRecentFiles[LC_MAX_RECENT_FILES];
	int mColorIndex;
	QAction* mActions[LC_NUM_COMMANDS];

public slots:
	void ProjectFileChanged(const QString& Path);
	void PreviewPiece(const QString& PartId, int ColorCode, bool ShowPreview);
	void TogglePreviewWidget(bool Visible);

protected slots:
	void CameraMenuAboutToShow();
	void ProjectionMenuAboutToShow();
	void ViewFocusReceived();
	void UpdateDockWidgetActions();
	void UpdateGamepads();
	void ModelTabContextMenuRequested(const QPoint& Point);
	void ModelTabCloseOtherTabs();
	void ModelTabClosed(int Index);
	void ModelTabChanged(int Index);
	void ClipboardChanged();
	void ActionTriggered();
	void ColorChanged(int ColorIndex);
	void ColorButtonClicked();
	void Print(QPrinter* Printer);
	void EnableWindowFlags(bool);

protected:
	void closeEvent(QCloseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent* Event) override;
	void dropEvent(QDropEvent* Event) override;
	QMenu* createPopupMenu() override;

	void CreateActions();
	void CreateMenus();
	void CreateToolBars();
	void CreateStatusBar();
	lcView* CreateView(lcModel* Model);
	void SetActiveView(lcView* ActiveView);
	void ToggleDockWidget(QWidget* DockWidget);
	void SplitView(Qt::Orientation Orientation);
	void ShowUpdatesDialog();
	void ShowAboutDialog();
	void ShowHTMLDialog();
	void ShowRenderDialog(int Command);
	void ShowInstructionsDialog();
	void ShowPrintDialog();
	void CreatePreviewWidget();

	bool OpenProjectFile(const QString& FileName);

	lcModelTabWidget* GetTabWidgetForModel(const lcModel* Model) const
	{
		for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
		{
			lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

			if (TabWidget->GetModel() == Model)
				return TabWidget;
		}

		return nullptr;
	}

	QTimer mGamepadTimer;
	QDateTime mLastGamepadUpdate;

	bool mAddKeys;
	lcTool mTool;
	lcTransformType mTransformType;
	bool mMoveSnapEnabled;
	bool mAngleSnapEnabled;
	int mMoveXYSnapIndex;
	int mMoveZSnapIndex;
	int mAngleSnapIndex;
	bool mRelativeTransform;
	bool mLocalTransform;
	PieceInfo* mCurrentPieceInfo;
	lcSelectionMode mSelectionMode;
	int mModelTabWidgetContextMenuIndex;

	QAction* mActionFileRecentSeparator;

	QTabWidget* mModelTabWidget;
	QToolBar* mStandardToolBar;
	QToolBar* mToolsToolBar;
	QToolBar* mTimeToolBar;
	QDockWidget* mPreviewToolBar;
	QDockWidget* mPartsToolBar;
	QDockWidget* mColorsToolBar;
	QDockWidget* mPropertiesToolBar;
	QDockWidget* mTimelineToolBar;

	lcPartSelectionWidget* mPartSelectionWidget;
	lcColorList* mColorList;
	QToolButton* mColorButton;
	lcPropertiesWidget* mPropertiesWidget;
	lcTimelineWidget* mTimelineWidget;
	QLineEdit* mTransformXEdit;
	QLineEdit* mTransformYEdit;
	QLineEdit* mTransformZEdit;
	lcPreviewDockWidget* mPreviewWidget;

	lcElidedLabel* mStatusBarLabel;
	QLabel* mStatusPositionLabel;
	QLabel* mStatusSnapLabel;
	QLabel* mStatusTimeLabel;

	QMenu* mTransformMenu;
	QMenu* mToolsMenu;
	QMenu* mViewpointMenu;
	QMenu* mCameraMenu;
	QMenu* mProjectionMenu;
	QMenu* mShadingMenu;
	QMenu* mSelectionModeMenu;
};

extern class lcMainWindow* gMainWindow;
