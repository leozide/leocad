#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"
#include "lc_model.h"
#include "camera.h"

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

enum lcDragState
{
	LC_DRAGSTATE_NONE,
	LC_DRAGSTATE_PIECE
//	LC_DRAGSTATE_COLOR
};

class View : public lcGLWidget
{
public:
	View(lcModel* Model);
	virtual ~View();

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

	void CancelTrackingOrClearSelection();
	void BeginPieceDrag();
	void EndPieceDrag(bool Accept);

	void SetProjection(bool Ortho);
	void LookAt();
	void ZoomExtents();

	void RemoveCamera();
	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetCameraIndex(int Index);
	void SetViewpoint(lcViewpoint Viewpoint);
	void SetDefaultCamera();
	lcMatrix44 GetProjectionMatrix() const;
	LC_CURSOR_TYPE GetCursor() const;
	void ShowContextMenu() const;

	lcVector3 GetMoveDirection(const lcVector3& Direction) const;
	lcMatrix44 GetPieceInsertPosition() const;
	void GetRayUnderPointer(lcVector3& Start, lcVector3& End) const;
	lcObjectSection FindObjectUnderPointer(bool PiecesOnly) const;
	lcArray<lcObject*> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	lcModel* mModel;
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
	lcTool GetCurrentTool() const;
	float GetOverlayScale() const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);

	lcScene mScene;
	lcDragState mDragState;
	lcTrackButton mTrackButton;
	lcTrackTool mTrackTool;
	int mMouseDownX;
	int mMouseDownY;

	lcVertexBuffer mGridBuffer;
	int mGridSettings[7];

	static lcVertexBuffer mRotateMoveVertexBuffer;
	static lcIndexBuffer mRotateMoveIndexBuffer;
};

#endif // _VIEW_H_
