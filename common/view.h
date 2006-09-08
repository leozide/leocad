#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"

class Project;
class Camera;

class View : public GLWindow
{
public:
	View(Project *pProject, GLWindow *share);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnLeftButtonDown(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(int x, int y, bool bControl, bool bShift);
	void OnMouseMove(int x, int y, bool bControl, bool bShift);

	LC_CURSOR_TYPE GetCursor(int x, int y) const;

	void LoadViewportProjection();

	Camera* GetCamera() const
	{
		return m_Camera;
	}
	void SetCamera(Camera* cam);
	void UpdateCamera();

public:
	float m_OverlayScale;

protected:
	Project* m_Project;

	Camera* m_Camera;
	String m_CameraName;
};

#endif // _VIEW_H_
