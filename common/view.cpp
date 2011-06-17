//
// View the project contents
//

#include <stdlib.h>
#include "project.h"
#include "view.h"
#include "system.h"

View::View(Project *pProject, GLWindow *share)
	: GLWindow(share)
{
	m_Project = pProject;
}

View::~View()
{
	if (m_Project != NULL)
		m_Project->RemoveView(this);
}

LC_CURSOR_TYPE View::GetCursor(int Ptx, int Pty) const
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
				case LC_OVERLAY_X: return LC_CURSOR_ROTATEX;
				case LC_OVERLAY_Y: return LC_CURSOR_ROTATEY;
				case LC_OVERLAY_Z: return LC_CURSOR_ROLL;
				case LC_OVERLAY_XYZ: return LC_CURSOR_ROTATE_VIEW;
				default:
					LC_ASSERT_FALSE("Unknown cursor type.");
					return LC_CURSOR_NONE;
			}

		case LC_ACTION_ROLL:
			return LC_CURSOR_ROLL;

		case LC_ACTION_CURVE:
		default:
			LC_ASSERT_FALSE("Unknown cursor type.");
			return LC_CURSOR_NONE;
	}
}

void View::OnDraw()
{
	MakeCurrent();

	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->Render(false);

	SwapBuffers();
}

void View::OnInitialUpdate()
{
	GLWindow::OnInitialUpdate();
	m_Project->AddView(this);
}

void View::OnLeftButtonDown(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnLeftButtonDown(x, y, bControl, bShift);
}

void View::OnLeftButtonUp(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnLeftButtonUp(x, y, bControl, bShift);
}

void View::OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnLeftButtonDoubleClick(x, y, bControl, bShift);
}

void View::OnRightButtonDown(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnRightButtonDown(x, y, bControl, bShift);
}

void View::OnRightButtonUp(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnRightButtonUp(x, y, bControl, bShift);
}

void View::OnMouseMove(int x, int y, bool bControl, bool bShift)
{
	m_Project->SetViewSize(m_nWidth, m_nHeight);
	m_Project->OnMouseMove(x, y, bControl, bShift);
}
