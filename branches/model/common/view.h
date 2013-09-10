#ifndef _VIEW_H_
#define _VIEW_H_

#include "lc_glwidget.h"

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

	void SetCamera(lcCamera* Camera, bool ForceCopy);
	void SetDefaultCamera();

	LC_CURSOR_TYPE GetCursor() const;

	Project* m_Project;
	lcCamera* mCamera;
	float m_OverlayScale;
};

#endif // _VIEW_H_
