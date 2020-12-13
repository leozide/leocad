#pragma once

#include "lc_commands.h"
#include "lc_math.h"

enum class lcDragState
{
	None,
	Piece,
	Color
};

enum class lcCursor
{
	First,
	Hidden = First,
	Default,
	Brick,
	Light,
	Spotlight,
	Camera,
	Select,
	SelectAdd,
	SelectRemove,
	Move,
	Rotate,
	RotateX,
	RotateY,
	Delete,
	Paint,
	ColorPicker,
	Zoom,
	ZoomRegion,
	Pan,
	Roll,
	RotateView,
	Count
};

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
	Insert,
	PointLight,
	SpotLight,
	Camera,
	Select,
	MoveX,
	MoveY,
	MoveZ,
	MoveXY,
	MoveXZ,
	MoveYZ,
	MoveXYZ,
	RotateX,
	RotateY,
	RotateZ,
	RotateXY,
	RotateXYZ,
	ScalePlus,
	ScaleMinus,
	Eraser,
	Paint,
	ColorPicker,
	Zoom,
	Pan,
	OrbitX,
	OrbitY,
	OrbitXY,
	Roll,
	ZoomRegion,
	Count
};

class lcGLWidget : public QObject
{
	Q_OBJECT

public:
	lcGLWidget(lcModel* Model);
	virtual ~lcGLWidget();

	lcGLWidget(const lcGLWidget&) = delete;
	lcGLWidget& operator=(const lcGLWidget&) = delete;

	lcModel* GetActiveModel() const;

	lcCamera* GetCamera() const
	{
		return mCamera;
	}

	bool IsLastFocused() const
	{
		return mLastFocusedView == this;
	}

	bool IsTracking() const
	{
		return mTrackButton != lcTrackButton::None;
	}

	int GetMouseX() const
	{
		return mMouseX;
	}

	int GetMouseY() const
	{
		return mMouseY;
	}

	void SetFocus(bool Focus);
	void SetMousePosition(int MouseX, int MouseY);
	void SetMouseModifiers(Qt::KeyboardModifiers MouseModifiers);
	void SetContext(lcContext* Context);
	void MakeCurrent();
	void Redraw();
	void UpdateCursor();

	lcVector3 ProjectPoint(const lcVector3& Point) const;
	lcVector3 UnprojectPoint(const lcVector3& Point) const;
	void UnprojectPoints(lcVector3* Points, int NumPoints) const;
	lcMatrix44 GetProjectionMatrix() const;

	void ZoomExtents();

	void SetViewpoint(lcViewpoint Viewpoint);
	void SetViewpoint(const lcVector3& Position);
	void SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up);
	void SetCameraAngles(float Latitude, float Longitude);
	void SetDefaultCamera();
	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetCamera(const char* CameraName);
	void SetCameraIndex(int Index);

	void DrawBackground() const;
	void DrawViewport() const;
	void DrawAxes() const;

	virtual void OnDraw() { }
	virtual void OnInitialUpdate() { }
	virtual void OnLeftButtonDown() { }
	virtual void OnLeftButtonUp() { }
	virtual void OnLeftButtonDoubleClick() { }
	virtual void OnMiddleButtonDown() { }
	virtual void OnMiddleButtonUp() { }
	virtual void OnRightButtonDown() { }
	virtual void OnRightButtonUp() { }
	virtual void OnBackButtonDown() { }
	virtual void OnBackButtonUp() { }
	virtual void OnForwardButtonDown() { }
	virtual void OnForwardButtonUp() { }
	virtual void OnMouseMove() { }
	virtual void OnMouseWheel(float Direction) { Q_UNUSED(Direction); }
	virtual void BeginDrag(lcDragState DragState) { Q_UNUSED(DragState); }
	virtual void EndDrag(bool Accept) { Q_UNUSED(Accept); }

	int mWidth = 1;
	int mHeight = 1;
	QGLWidget* mWidget = nullptr;
	lcContext* mContext = nullptr;

signals:
	void CameraChanged();

protected:
	lcCursor GetCursor() const;
	void SetCursor(lcCursor Cursor);
	lcTool GetCurrentTool() const;
	void StartTracking(lcTrackButton TrackButton);
	lcVector3 GetCameraLightInsertPosition() const;

	int mMouseX = 0;
	int mMouseY = 0;
	int mMouseDownX = 0;
	int mMouseDownY = 0;
	Qt::KeyboardModifiers mMouseModifiers = Qt::NoModifier;

	bool mTrackUpdated = false;
	lcTrackTool mTrackTool = lcTrackTool::None;
	lcTrackButton mTrackButton = lcTrackButton::None;
	lcCursor mCursor = lcCursor::Default;

	std::unique_ptr<lcScene> mScene;

	lcModel* mModel = nullptr;
	lcPiece* mActiveSubmodelInstance = nullptr;
	lcMatrix44 mActiveSubmodelTransform;

	lcCamera* mCamera = nullptr;
	bool mDeleteContext = true;

	static lcGLWidget* mLastFocusedView;
};
