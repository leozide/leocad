#pragma once

#include "lc_glwidget.h"
#include "camera.h"
#include "lc_scene.h"
#include "lc_viewcube.h"
#include "lc_commands.h"

enum lcTrackButton
{
	LC_TRACKBUTTON_NONE,
	LC_TRACKBUTTON_LEFT,
	LC_TRACKBUTTON_MIDDLE,
	LC_TRACKBUTTON_RIGHT
};

enum lcTrackTool
{
	LC_TRACKTOOL_NONE,
	LC_TRACKTOOL_INSERT,
	LC_TRACKTOOL_POINTLIGHT,
	LC_TRACKTOOL_SPOTLIGHT,
	LC_TRACKTOOL_CAMERA,
	LC_TRACKTOOL_SELECT,
	LC_TRACKTOOL_MOVE_X,
	LC_TRACKTOOL_MOVE_Y,
	LC_TRACKTOOL_MOVE_Z,
	LC_TRACKTOOL_MOVE_XY,
	LC_TRACKTOOL_MOVE_XZ,
	LC_TRACKTOOL_MOVE_YZ,
	LC_TRACKTOOL_MOVE_XYZ,
	LC_TRACKTOOL_ROTATE_X,
	LC_TRACKTOOL_ROTATE_Y,
	LC_TRACKTOOL_ROTATE_Z,
	LC_TRACKTOOL_ROTATE_XY,
	LC_TRACKTOOL_ROTATE_XYZ,
	LC_TRACKTOOL_SCALE_PLUS,
	LC_TRACKTOOL_SCALE_MINUS,
	LC_TRACKTOOL_ERASER,
	LC_TRACKTOOL_PAINT,
	LC_TRACKTOOL_ZOOM,
	LC_TRACKTOOL_PAN,
	LC_TRACKTOOL_ORBIT_X,
	LC_TRACKTOOL_ORBIT_Y,
	LC_TRACKTOOL_ORBIT_XY,
	LC_TRACKTOOL_ROLL,
	LC_TRACKTOOL_ZOOM_REGION
};

enum class lcDragState
{
	NONE,
	PIECE,
	COLOR
};

class View : public lcGLWidget
{
public:
	View(lcModel* Model);
	virtual ~View();

	void Clear()
	{
		mModel = nullptr;
		mActiveSubmodelInstance = nullptr;
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	lcModel* GetActiveModel() const;
	void SetTopSubmodelActive();
	void SetSelectedSubmodelActive();

	void SetHighlight(bool Highlight)
	{
		mHighlight = Highlight;
	}

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

	void OnDraw();
	void OnInitialUpdate();
	void OnUpdateCursor();
	void OnLeftButtonDown();
	void OnLeftButtonUp();
	void OnLeftButtonDoubleClick();
	void OnMiddleButtonDown();
	void OnMiddleButtonUp();
	void OnRightButtonDown();
	void OnRightButtonUp();
	void OnBackButtonUp();
	void OnForwardButtonUp();
	void OnMouseMove();
	void OnMouseWheel(float Direction);

	bool IsTracking() const
	{
		return mTrackButton != LC_TRACKBUTTON_NONE;
	}

	void StartOrbitTracking();
	void CancelTrackingOrClearSelection();
	void BeginDrag(lcDragState DragState);
	void EndDrag(bool Accept);

	void SetProjection(bool Ortho);
	void LookAt();
	void ZoomExtents();
	void MoveCamera(const lcVector3& Direction);

	void RemoveCamera();
	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetCamera(const char* CameraName);
	void SetCameraIndex(int Index);
	void SetViewpoint(lcViewpoint Viewpoint);
	void SetViewpoint(const lcVector3& Position);
	void SetCameraAngles(float Latitude, float Longitude);
	void SetDefaultCamera();
	lcMatrix44 GetProjectionMatrix() const;
	LC_CURSOR_TYPE GetCursor() const;
	void ShowContextMenu() const;

	lcVector3 GetMoveDirection(const lcVector3& Direction) const;
	lcMatrix44 GetPieceInsertPosition(bool IgnoreSelected, PieceInfo* Info) const;
	void GetRayUnderPointer(lcVector3& Start, lcVector3& End) const;
	lcObjectSection FindObjectUnderPointer(bool PiecesOnly, bool IgnoreSelected) const;
	lcArray<lcObject*> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	lcCamera* mCamera;

	lcVector3 ProjectPoint(const lcVector3& Point) const
	{
		int Viewport[4] = { 0, 0, mWidth, mHeight };
		return lcProjectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
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

	bool BeginRenderToImage(int Width, int Height);
	void EndRenderToImage();

	QImage GetRenderImage() const
	{
		return mRenderImage;
	}

protected:
	static void CreateSelectMoveOverlayMesh(lcContext* Context);

	void DrawSelectMoveOverlay();
	void DrawRotateOverlay();
	void DrawSelectZoomRegionOverlay();
	void DrawRotateViewOverlay();
	void DrawGrid();
	void DrawAxes();
	void DrawViewport();

	void UpdateTrackTool();
	bool IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms) const;
	lcTool GetCurrentTool() const;
	lcTrackTool GetOverrideTrackTool(Qt::MouseButton Button) const;
	float GetOverlayScale() const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);
	void OnButtonDown(lcTrackButton TrackButton);
	lcMatrix44 GetTileProjectionMatrix(int CurrentRow, int CurrentColumn, int CurrentTileWidth, int CurrentTileHeight) const;

	lcModel* mModel;
	lcPiece* mActiveSubmodelInstance;
	lcMatrix44 mActiveSubmodelTransform;

	lcScene mScene;
	lcDragState mDragState;
	lcTrackButton mTrackButton;
	lcTrackTool mTrackTool;
	bool mTrackToolFromOverlay;
	bool mTrackUpdated;
	int mMouseDownX;
	int mMouseDownY;
	lcVector3 mMouseDownPosition;
	PieceInfo* mMouseDownPiece;
	bool mHighlight;
	QImage mRenderImage;
	std::pair<lcFramebuffer, lcFramebuffer> mRenderFramebuffer;
	lcViewCube mViewCube;

	lcVertexBuffer mGridBuffer;
	int mGridSettings[7];

	static lcVertexBuffer mRotateMoveVertexBuffer;
	static lcIndexBuffer mRotateMoveIndexBuffer;
};

