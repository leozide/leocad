#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"
#include "lc_object.h"
#include "lc_model.h"
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
	LC_TRACKTOOL_ROTATE_X,
	LC_TRACKTOOL_ROTATE_Y,
	LC_TRACKTOOL_ROTATE_Z,
	LC_TRACKTOOL_ROTATE_VIEW_X,
	LC_TRACKTOOL_ROTATE_VIEW_Y,
	LC_TRACKTOOL_ROTATE_VIEW_Z,
	LC_TRACKTOOL_ZOOM,
	LC_TRACKTOOL_PAN,
	LC_TRACKTOOL_ROTATE_VIEW
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

	Project* mProject;
	lcCamera* mCamera;

protected:
	void DrawMouseTracking();
	void DrawViewport();

	lcObjectSection FindClosestObject(bool PiecesOnly) const;
	void FindObjectsInRectangle(float x1, float y1, float x2, float y2, lcArray<lcObjectSection>& Objects) const;
	void GetPieceInsertPosition(lcVector3* Position, lcVector4* AxisAngle);

	lcTrackTool GetTrackTool(float* InterfaceScale, lcVector3* InterfaceCenter) const;
	lcTool GetMouseTool(lcTrackTool TrackTool) const;
	void StartTracking(lcMouseTrack MouseTrack, lcTrackTool TrackTool);
	void StopTracking(bool Accept);

	lcMouseTrack mMouseTrack;
	lcTrackTool mTrackTool;
	int mMouseDownX;
	int mMouseDownY;
};

#endif // _VIEW_H_
