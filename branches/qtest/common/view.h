#ifndef _VIEW_H_
#define _VIEW_H_

#include "glwindow.h"

class Camera;
class Project;

class View : public GLWindow
{
public:
	View(Project *project);
	virtual ~View();

	void OnDraw();
	void OnInitialUpdate();
	void OnLeftButtonDown(int x, int y, bool Control, bool Shift, bool Alt);
	void OnLeftButtonUp(int x, int y, bool Control, bool Shift, bool Alt);
	void OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift, bool Alt);
	void OnMiddleButtonDown(int x, int y, bool Control, bool Shift, bool Alt);
	void OnMiddleButtonUp(int x, int y, bool Control, bool Shift, bool Alt);
	void OnRightButtonDown(int x, int y, bool Control, bool Shift, bool Alt);
	void OnRightButtonUp(int x, int y, bool Control, bool Shift, bool Alt);
	void OnMouseMove(int x, int y, bool Control, bool Shift, bool Alt);
	void OnMouseWheel(int x, int y, float Direction, bool Control, bool Shift, bool Alt);

	void SetCamera(Camera* camera, bool ForceCopy);
	void SetDefaultCamera();

	LC_CURSOR_TYPE GetCursor() const;

	Project* m_Project;
	Camera* mCamera;
	float m_OverlayScale;
};

#endif // _VIEW_H_
