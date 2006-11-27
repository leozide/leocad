//
// View the project contents
//

#include <stdlib.h>
#include "project.h"
#include "view.h"
#include "camera.h"
#include "system.h"

View::View(Project *pProject, GLWindow *share)
	: GLWindow(share)
{
	m_Project = pProject;
	m_Camera = pProject->GetActiveModel()->GetCamera(LC_CAMERA_MAIN);
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
	if (m_Camera)
	{
		MakeCurrent();

		m_Project->Render(this, true, true);

		SwapBuffers();
	}
}

void View::OnInitialUpdate()
{
	GLWindow::OnInitialUpdate();
	m_Project->AddView(this);
}

void View::OnLeftButtonDown(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnLeftButtonDown(this, x, y, bControl, bShift);
}

void View::OnLeftButtonUp(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnLeftButtonUp(this, x, y, bControl, bShift);
}

void View::OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnLeftButtonDoubleClick(this, x, y, bControl, bShift);
}

void View::OnRightButtonDown(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnRightButtonDown(this, x, y, bControl, bShift);
}

void View::OnRightButtonUp(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnRightButtonUp(this, x, y, bControl, bShift);
}

void View::OnMouseMove(int x, int y, bool bControl, bool bShift)
{
	m_Project->OnMouseMove(this, x, y, bControl, bShift);
}

void View::OnSize(int cx, int cy)
{
	GLWindow::OnSize(cx, cy);

	UpdateOverlayScale();
}

void View::LoadViewportProjection()
{
	if (m_Camera)
	{
		float ratio = (float)m_nWidth/(float)m_nHeight;
		glViewport(0, 0, m_nWidth, m_nHeight);
		m_Camera->LoadProjection(ratio);
	}
}

void View::UpdateOverlayScale()
{
	GLdouble ScreenX, ScreenY, ScreenZ, PointX, PointY, PointZ;
	GLdouble ModelMatrix[16], ProjMatrix[16];
	GLint Viewport[4];

	LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, ProjMatrix);
	glGetIntegerv(GL_VIEWPORT, Viewport);

	const Vector3& Center = m_Project->GetOverlayCenter();

	// Calculate the scaling factor by projecting the center to the front plane then
	// projecting a point close to it back.
	gluProject(Center[0], Center[1], Center[2], ModelMatrix, ProjMatrix, Viewport, &ScreenX, &ScreenY, &ScreenZ);
	gluUnProject(ScreenX + 10.0f, ScreenY, ScreenZ, ModelMatrix, ProjMatrix, Viewport, &PointX, &PointY, &PointZ);

	Vector3 Dist((float)PointX - Center[0], (float)PointY - Center[1], (float)PointZ - Center[2]);
	m_OverlayScale = Dist.Length() * 5.0f;
}

void View::SetCamera(Camera* cam)
{
	if (cam)
		m_CameraName = cam->GetName();

	m_Camera = cam;
}

void View::UpdateCamera()
{
	Camera* cam = m_Project->GetActiveModel()->GetCamera(m_CameraName);

	if (!cam)
		cam = m_Project->GetActiveModel()->GetCamera(LC_CAMERA_MAIN);

	m_Camera = cam;
	m_CameraName = m_Camera->GetName();
}
