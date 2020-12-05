#pragma once

#include "lc_glwidget.h"
#include "lc_scene.h"
#include "lc_viewsphere.h"
#include "lc_commands.h"

class lcQGLWidget;
class lcPreviewWidget;

class lcPreviewDockWidget : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcPreviewDockWidget(QMainWindow* Parent = nullptr);
	bool SetCurrentPiece(const QString& PartType, int ColorCode);
	void ClearPreview();
	void UpdatePreview();

protected slots:
	void SetPreviewLock();

protected:
	QAction* mLockAction;
	QToolBar* mToolBar;
	QLabel* mLabel;
	lcPreviewWidget* mPreview;
	lcQGLWidget* mViewWidget;
};

class lcPreviewWidget : public lcGLWidget
{
public:
	enum class lcTrackButton
	{
		None,
		Left,
		Middle,
		Right
	};

	enum class lcTrackTool
	{
		None,
		Pan,
		OrbitXY,
		Count
	};

	lcPreviewWidget();
	~lcPreviewWidget();

	lcTool GetTool() const
	{
		return mTool;
	}

	QString GetDescription() const
	{
		return mDescription;
	}

	void ClearPreview();
	void UpdatePreview();
	bool SetCurrentPiece(const QString& PartType, int ColorCode);
	lcModel* GetActiveModel() const;
	lcCursor GetCursor() const;
	void SetCamera(lcCamera* Camera);
	void SetDefaultCamera();
	void ZoomExtents();

	// exclusively called from viewSphere
	void SetViewpoint(const lcVector3& Position);
	void StartOrbitTracking();
	bool IsTracking() const
	{
		return mTrackButton != lcTrackButton::None;
	}

	bool IsModel() const
	{
		return mIsModel;
	}
	
	void OnInitialUpdate() override;
	void OnDraw() override;
	void OnUpdateCursor() override;
	void OnLeftButtonDown() override;
	void OnLeftButtonUp() override;
	void OnLeftButtonDoubleClick() override;
	void OnMiddleButtonDown() override;
	void OnMiddleButtonUp() override;
	void OnRightButtonDown() override;
	void OnRightButtonUp() override;
	void OnMouseMove() override;
	void OnMouseWheel(float Direction) override;

protected:
	void DrawViewport();

	lcTool GetCurrentTool() const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);
	void OnButtonDown(lcTrackButton TrackButton);

	Project* mLoader;
	lcModel* mModel;
	lcViewSphere mViewSphere;

	lcScene mScene;

	lcTool mTool;
	lcTrackButton mTrackButton;
	lcTrackTool mTrackTool;

	QString mDescription;
	bool mIsModel;
	
	bool mTrackUpdated;
	int mMouseDownX;
	int mMouseDownY;
};
