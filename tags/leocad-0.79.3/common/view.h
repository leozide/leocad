#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"

class Camera;
class Project;

class View : public GLWindow
{
public:
	View(Project *pProject, GLWindow *share);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnLeftButtonDown(int x, int y, bool Control, bool Shift);
	void OnLeftButtonUp(int x, int y, bool Control, bool Shift);
	void OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift);
	void OnMiddleButtonDown(int x, int y, bool Control, bool Shift);
	void OnMiddleButtonUp(int x, int y, bool Control, bool Shift);
	void OnRightButtonDown(int x, int y, bool Control, bool Shift);
	void OnRightButtonUp(int x, int y, bool Control, bool Shift);
	void OnMouseMove(int x, int y, bool Control, bool Shift);
	void OnMouseWheel(int x, int y, float Direction, bool Control, bool Shift);

	void SetCamera(Camera* camera, bool ForceCopy);
	void SetDefaultCamera();

	LC_CURSOR_TYPE GetCursor() const;

	Project* m_Project;
	Camera* mCamera;
	float m_OverlayScale;
};

#endif // _VIEW_H_
