#pragma once

#include <QMainWindow>
#include <QString>
#include "lc_global.h"
#include "lc_glwidget.h"
#include "lc_scene.h"
#include "lc_viewsphere.h"
#include "lc_commands.h"
#include "lc_application.h"
#include "camera.h"

class QLabel;
class Project;
class lcModel;
class lcPiece;
class lcQGLWidget;

class lcPreviewDockWidget : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcPreviewDockWidget(QMainWindow* parent = nullptr);
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

	enum lcTrackTool
	{
		LC_TRACKTOOL_NONE,
		LC_TRACKTOOL_PAN,
		LC_TRACKTOOL_ORBIT_XY,
		LC_TRACKTOOL_COUNT
	};

	lcPreviewWidget();
	~lcPreviewWidget();

	lcTool GetTool() const
	{
		return mTool;
	}

	lcCamera* GetCamera() const
	{
		return mCamera;
	}

	QString GetDescription() const
	{
		return mDescription;
	}

	lcVector3 UnprojectPoint(const lcVector3& Point) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		return lcUnprojectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
	}

	void UnprojectPoints(lcVector3* Points, int NumPoints) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		lcUnprojectPoints(Points, NumPoints, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
	}

	void ClearPreview();
	void UpdatePreview();
	bool SetCurrentPiece(const QString& PartType, int ColorCode);
	lcMatrix44 GetProjectionMatrix() const;
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
	void DrawAxes();
	void DrawViewport();

	lcTool GetCurrentTool() const;
	lcTrackTool GetOverrideTrackTool(Qt::MouseButton Button) const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);
	void OnButtonDown(lcTrackButton TrackButton);

	Project* mLoader;
	lcModel* mModel;
	lcCamera* mCamera;
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

extern lcPreviewWidget* gPreviewWidget;
