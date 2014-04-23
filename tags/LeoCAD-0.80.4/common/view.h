#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"
#include "lc_projection.h"

class Camera;
class Project;

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
	const lcProjection& UpdateProjection();
	void SetProjectionType(lcProjection::Type type);
	lcProjection::Type GetProjectionType() const;

	LC_CURSOR_TYPE GetCursor() const;

	Project* m_Project;
	Camera* mCamera;
	float m_OverlayScale;

private:
	lcProjection mProjection;
};

#endif // _VIEW_H_