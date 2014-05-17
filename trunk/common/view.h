#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"
#include "camera.h"
#include "project.h"

class Project;

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

class View : public lcGLWidget
{
public:
	View(Project *project);
	virtual ~View();

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
	void OnMouseMove();
	void OnMouseWheel(float Direction);

	void SetCamera(Camera* camera, bool ForceCopy);
	void SetDefaultCamera();
	lcMatrix44 GetProjectionMatrix() const;
	LC_CURSOR_TYPE GetCursor() const;

	lcObjectSection FindObjectUnderPointer(bool PiecesOnly) const;
	lcArray<lcObjectSection> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	Project* mProject;
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
	void DrawSelectMoveOverlay();
	void DrawRotateOverlay();
	void DrawSelectZoomRegionOverlay();
	void DrawRotateViewOverlay();
	void DrawViewport();

	void UpdateTrackTool();
	lcTool GetCurrentTool() const;
	float GetOverlayScale() const;
	void StartTracking(lcTrackButton TrackButton);
	void StopTracking(bool Accept);

	lcTrackButton mTrackButton;
	lcTrackTool mTrackTool;
	int mMouseDownX;
	int mMouseDownY;
};

#endif // _VIEW_H_
