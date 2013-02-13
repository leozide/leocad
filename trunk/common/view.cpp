#include "lc_global.h"
#include <stdlib.h>
#include "project.h"
#include "camera.h"
#include "view.h"
#include "system.h"

View::View(Project *pProject, GLWindow *share)
	: GLWindow(share)
{
	m_Project = pProject;
	mCamera = NULL;
	m_OverlayScale = 1.0f;
}

View::~View()
{
	if (m_Project != NULL)
		m_Project->RemoveView(this);

	if (mCamera && mCamera->IsSimple())
		delete mCamera;
}

void View::SetCamera(Camera* camera, bool ForceCopy)
{
	if (camera->IsSimple() || ForceCopy)
	{
		if (!mCamera || !mCamera->IsSimple())
			mCamera = new Camera(true);

		mCamera->CopyPosition(camera);
	}
	else
	{
		if (mCamera && mCamera->IsSimple())
			delete mCamera;

		mCamera = camera;
	}
}

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new Camera(true);

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME, 1, false, false);
}

LC_CURSOR_TYPE View::GetCursor() const
{
	// TODO: check if we're the focused window and return just the default arrow if we aren't.

	switch (m_Project->GetAction())
	{
		case LC_ACTION_SELECT:
			if (Sys_KeyDown(KEY_CONTROL))
				return LC_CURSOR_SELECT_GROUP;
			else
				return LC_CURSOR_SELECT;

		case LC_ACTION_INSERT:
			return LC_CURSOR_BRICK;

		case LC_ACTION_LIGHT:
			return LC_CURSOR_LIGHT;

		case LC_ACTION_SPOTLIGHT:
			return LC_CURSOR_SPOTLIGHT;

		case LC_ACTION_CAMERA:
			return LC_CURSOR_CAMERA;

		case LC_ACTION_MOVE:
			return LC_CURSOR_MOVE;

		case LC_ACTION_ROTATE:
			return LC_CURSOR_ROTATE;

		case LC_ACTION_ERASER:
			return LC_CURSOR_DELETE;

		case LC_ACTION_PAINT:
			return LC_CURSOR_PAINT;

		case LC_ACTION_ZOOM:
			return LC_CURSOR_ZOOM;

		case LC_ACTION_ZOOM_REGION:
			return LC_CURSOR_ZOOM_REGION;

		case LC_ACTION_PAN:
			return LC_CURSOR_PAN;

		case LC_ACTION_ROTATE_VIEW:
			switch (m_Project->GetOverlayMode())
			{
				case LC_OVERLAY_ROTATE_VIEW_X:
					return LC_CURSOR_ROTATEX;
				case LC_OVERLAY_ROTATE_VIEW_Y:
					return LC_CURSOR_ROTATEY;
				case LC_OVERLAY_ROTATE_VIEW_Z:
					return LC_CURSOR_ROLL;
				case LC_OVERLAY_ROTATE_VIEW_XYZ:
				default:
					return LC_CURSOR_ROTATE_VIEW;
			}

		case LC_ACTION_ROLL:
			return LC_CURSOR_ROLL;

		case LC_ACTION_CURVE:
		default:
			LC_ASSERT_FALSE("Unknown cursor type.");
			return LC_CURSOR_DEFAULT;
	}
}

void View::OnDraw()
{
	m_Project->Render(this, false);
}

void View::OnInitialUpdate()
{
	GLWindow::OnInitialUpdate();
	m_Project->AddView(this);
}

void View::OnLeftButtonDown(int x, int y, bool Control, bool Shift)
{
	m_Project->OnLeftButtonDown(this, x, y, Control, Shift);
}

void View::OnLeftButtonUp(int x, int y, bool Control, bool Shift)
{
	m_Project->OnLeftButtonUp(this, x, y, Control, Shift);
}

void View::OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift)
{
	m_Project->OnLeftButtonDoubleClick(this, x, y, Control, Shift);
}

void View::OnMiddleButtonDown(int x, int y, bool Control, bool Shift)
{
	m_Project->OnMiddleButtonDown(this, x, y, Control, Shift);
}

void View::OnMiddleButtonUp(int x, int y, bool Control, bool Shift)
{
	m_Project->OnMiddleButtonUp(this, x, y, Control, Shift);
}

void View::OnRightButtonDown(int x, int y, bool Control, bool Shift)
{
	m_Project->OnRightButtonDown(this, x, y, Control, Shift);
}

void View::OnRightButtonUp(int x, int y, bool Control, bool Shift)
{
	m_Project->OnRightButtonUp(this, x, y, Control, Shift);
}

void View::OnMouseMove(int x, int y, bool Control, bool Shift)
{
	m_Project->OnMouseMove(this, x, y, Control, Shift);
}

void View::OnMouseWheel(int x, int y, float Direction, bool Control, bool Shift)
{
	m_Project->OnMouseWheel(this, x, y, Direction, Control, Shift);
}
