#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"
#include "lc_object.h"
#include "project.h"
class Project;

enum lcMouseTrack
{
	LC_MOUSETRACK_NONE,
	LC_MOUSETRACK_LEFT,
	LC_MOUSETRACK_MIDDLE,
	LC_MOUSETRACK_RIGHT
};

enum lcTrackTool
{
	LC_TRACKTOOL_NONE,
	LC_TRACKTOOL_MOVE_X,
	LC_TRACKTOOL_MOVE_Y,
	LC_TRACKTOOL_MOVE_Z,
	LC_TRACKTOOL_MOVE_XY,
	LC_TRACKTOOL_MOVE_XZ,
	LC_TRACKTOOL_MOVE_YZ,
//	LC_TRACKTOOL_MOVE_XYZ,
	LC_TRACKTOOL_ROTATE_X,
	LC_TRACKTOOL_ROTATE_Y,
	LC_TRACKTOOL_ROTATE_Z,
//	LC_TRACKTOOL_ROTATE_XY,
//	LC_TRACKTOOL_ROTATE_XZ,
//	LC_TRACKTOOL_ROTATE_YZ,
//	LC_TRACKTOOL_ROTATE_XYZ,
	LC_TRACKTOOL_ZOOM,
	LC_TRACKTOOL_PAN,
	LC_TRACKTOOL_ROTATE_VIEW_X,
	LC_TRACKTOOL_ROTATE_VIEW_Y,
	LC_TRACKTOOL_ROTATE_VIEW_Z,
	LC_TRACKTOOL_ROTATE_VIEW_XYZ
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

	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetDefaultCamera();

	LC_CURSOR_TYPE GetCursor() const;

	Project* m_Project;
	lcCamera* mCamera;
	float m_OverlayScale;

protected:
	void RenderInterface();
	void RenderViewport();

	lcObjectSection FindClosestObject() const;

	lcTrackTool GetTrackTool(float* InterfaceScale, lcVector3* InterfaceCenter) const;
	lcTool GetMouseTool(lcTrackTool TrackTool);
	void StartTracking(lcMouseTrack MouseTrack, lcTrackTool TrackTool);
	void StopTracking(bool Accept);

	lcMouseTrack mMouseTrack;
	lcTrackTool mTrackTool;
	int mMouseDownX;
	int mMouseDownY;
};

#endif // _VIEW_H_
