#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"
#include "typedefs.h"

class Project;
class lcCamera;

class View : public GLWindow
{
public:
	View(Project *pProject, GLWindow *share);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnSize(int cx, int cy);
	void OnLeftButtonDown(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(int x, int y, bool bControl, bool bShift);
	void OnMouseMove(int x, int y, bool bControl, bool bShift);

	LC_CURSOR_TYPE GetCursor(int x, int y) const;

	void LoadViewportProjection();
	void UpdateOverlayScale();

	lcCamera* GetCamera() const
	{
		return m_Camera;
	}
	void SetCamera(lcCamera* cam);
	void UpdateCamera();

public:
	float m_OverlayScale;

protected:
	Project* m_Project;

	lcCamera* m_Camera;
	String m_CameraName;
};

#endif // _VIEW_H_
