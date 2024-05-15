#pragma once

#include "lc_context.h"
#include "lc_math.h"
#include "lc_commands.h"

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
	PointLight,
	SpotLight,
	DirectionalLight,
	AreaLight,
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
	DirectionalLight,
	AreaLight,
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

enum class lcViewType
{
	View,
	Preview,
	Minifig,
	PartsList,
	Count
};

struct lcFindReplaceParams
{
	PieceInfo* FindInfo = nullptr;
	QString FindString;
	int FindColorIndex = 0;
	PieceInfo* ReplacePieceInfo = nullptr;
	int ReplaceColorIndex = 0;
};

class lcView : public QObject
{
	Q_OBJECT

public:
	lcView(lcViewType ViewType, lcModel* Model);
	~lcView();

	lcView(const lcView&) = delete;
	lcView(lcView&&) = delete;
	lcView& operator=(const lcView&) = delete;
	lcView& operator=(lcView&&) = delete;

	static lcFindReplaceParams& GetFindReplaceParams()
	{
		return mFindReplaceParams;
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcViewType GetViewType() const
	{
		return mViewType;
	}

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

	int GetWidth() const
	{
		return mWidth;
	}

	int GetHeight() const
	{
		return mHeight;
	}

	void SetSize(int Width, int Height)
	{
		mWidth = Width;
		mHeight = Height;
	}

	lcViewWidget* GetWidget() const
	{
		return mWidget;
	}

	void SetWidget(lcViewWidget* Widget)
	{
		mWidget = Widget;
	}

	int GetMouseX() const
	{
		return mMouseX;
	}

	int GetMouseY() const
	{
		return mMouseY;
	}

	void SetBackgroundColorOverride(quint32 BackgroundColor)
	{
		mOverrideBackgroundColor = true;
		mBackgroundColor = BackgroundColor;
	}

	lcMatrix44 GetActiveSubmodelTransform() const
	{
		return mActiveSubmodelTransform;
	}

	static std::vector<lcView*> GetModelViews(const lcModel* Model);
	static void UpdateProjectViews(const Project* Project);
	static void UpdateAllViews();

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

	void MakeCurrent();
	void Redraw();

	void SetOffscreenContext();

	void SetFocus(bool Focus);
	void SetMousePosition(int MouseX, int MouseY);
	void SetMouseModifiers(Qt::KeyboardModifiers MouseModifiers);

	lcModel* GetActiveModel() const;
	void SetTopSubmodelActive();
	void SetSelectedSubmodelActive();

	void OnDraw();
	void OnLeftButtonDown();
	void OnLeftButtonUp();
	void OnLeftButtonDoubleClick();
	void OnMiddleButtonDown();
	void OnMiddleButtonUp();
	void OnRightButtonDown();
	void OnRightButtonUp();
	void OnBackButtonDown();
	void OnBackButtonUp();
	void OnForwardButtonDown();
	void OnForwardButtonUp();
	void OnMouseMove();
	void OnMouseWheel(float Direction);
	void BeginDrag(lcDragState DragState);
	void EndDrag(bool Accept);

	void UpdateCursor();
	void StartPanGesture();
	void UpdatePanGesture(int dx, int dy);
	void EndPanGesture(bool Accept);
	void StartOrbitTracking();
	void CancelTrackingOrClearSelection();

	void SetViewpoint(lcViewpoint Viewpoint);
	void SetViewpoint(const lcVector3& Position);
	void SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up);
	void SetCameraAngles(float Latitude, float Longitude);
	void SetDefaultCamera();
	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetCamera(const QString& CameraName);
	void SetCameraIndex(int Index);

	void SetProjection(bool Ortho);
	void LookAt();
	void MoveCamera(const lcVector3& Direction);
	void Zoom(float Amount);
	void ZoomExtents();

	void RemoveCamera();
	void ShowContextMenu() const;
	bool CloseFindReplaceDialog();
	void ShowFindReplaceWidget(bool Replace);

	float GetOverlayScale() const;
	lcVector3 GetMoveDirection(const lcVector3& Direction) const;
	lcMatrix44 GetPieceInsertPosition(bool IgnoreSelected, PieceInfo* Info) const;
	lcVector3 GetCameraLightInsertPosition() const;
	void GetRayUnderPointer(lcVector3& Start, lcVector3& End) const;
	lcObjectSection FindObjectUnderPointer(bool PiecesOnly, bool IgnoreSelected) const;
	lcPieceInfoRayTest FindPieceInfoUnderPointer(bool IgnoreSelected) const;
	lcArray<lcObject*> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	lcVector3 ProjectPoint(const lcVector3& Point) const;
	lcVector3 UnprojectPoint(const lcVector3& Point) const;
	void UnprojectPoints(lcVector3* Points, int NumPoints) const;
	lcMatrix44 GetProjectionMatrix() const;

	bool BeginRenderToImage(int Width, int Height);
	void EndRenderToImage();
	QImage GetRenderImage() const;
	void BindRenderFramebuffer();
	void UnbindRenderFramebuffer();
	QImage GetRenderFramebufferImage() const;
	std::vector<QImage> GetStepImages(lcStep Start, lcStep End);
	void SaveStepImages(const QString& BaseName, bool AddStepSuffix, lcStep Start, lcStep End, std::function<void(const QString&)> ProgressCallback);

	lcContext* mContext = nullptr;

signals:
	void FocusReceived();

protected:
	void DrawBackground(int CurrentTileRow, int TotalTileRows, int CurrentTileHeight) const;
	void DrawViewport() const;
	void DrawAxes() const;

	void DrawSelectZoomRegionOverlay();
	void DrawRotateViewOverlay();
	void DrawGrid();

	lcMatrix44 GetTileProjectionMatrix(int CurrentRow, int CurrentColumn, int CurrentTileWidth, int CurrentTileHeight) const;

	lcCursor GetCursor() const;
	void SetCursor(lcCursor Cursor);
	lcTool GetCurrentTool() const;
	void UpdateTrackTool();
	lcTrackTool GetOverrideTrackTool(Qt::MouseButton Button) const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);
	void OnButtonDown(lcTrackButton TrackButton);
	void StartPan(int x, int y);
	void UpdatePan(int x, int y);

	lcViewWidget* mWidget = nullptr;
	int mWidth = 1;
	int mHeight = 1;
	bool mDeleteContext = true;
	lcViewType mViewType;

	int mMouseX = 0;
	int mMouseY = 0;
	int mMouseDownX = 0;
	int mMouseDownY = 0;
	Qt::KeyboardModifiers mMouseModifiers = Qt::NoModifier;

	bool mTrackUpdated = false;
	bool mToolClicked = false;
	lcTrackTool mTrackTool = lcTrackTool::None;
	lcTrackButton mTrackButton = lcTrackButton::None;
	lcCursor mCursor = lcCursor::Default;

	lcDragState mDragState;
	bool mTrackToolFromOverlay;
	lcVector3 mMouseDownPosition;
	PieceInfo* mMouseDownPiece;
	int mPanX = 0;
	int mPanY = 0;

	QImage mRenderImage;
	std::unique_ptr<QOpenGLFramebufferObject> mRenderFramebuffer;
	bool mOverrideBackgroundColor = false;
	quint32 mBackgroundColor = 0;

	std::unique_ptr<lcScene> mScene;
	std::unique_ptr<lcViewManipulator> mViewManipulator;
	std::unique_ptr<lcViewSphere> mViewSphere;

	lcModel* mModel = nullptr;
	lcPiece* mActiveSubmodelInstance = nullptr;
	lcMatrix44 mActiveSubmodelTransform;

	lcCamera* mCamera = nullptr;

	lcVertexBuffer mGridBuffer;
	int mGridSettings[7];

	static QPointer<lcFindReplaceWidget> mFindWidget;
	static lcFindReplaceParams mFindReplaceParams;

	static lcView* mLastFocusedView;
	static std::vector<lcView*> mViews;
};
