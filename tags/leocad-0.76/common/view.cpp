//
// View the project contents
//

#include "lc_global.h"
#include "view.h"

#include <stdlib.h>
#include "project.h"
#include "camera.h"
#include "system.h"
#include "lc_model.h"

View::View(Project *pProject, GLWindow *share)
	: GLWindow(share)
{
	m_Project = pProject;
	m_Camera = pProject->m_ActiveModel->GetCamera(LC_CAMERA_MAIN);
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
			return LC_CURSOR_ROTATE_VIEW;

		case LC_ACTION_ORBIT:
			switch (m_Project->GetOverlayMode())
			{
				case LC_OVERLAY_X: return LC_CURSOR_ROTATEX;
				case LC_OVERLAY_Y: return LC_CURSOR_ROTATEY;
				case LC_OVERLAY_Z: return LC_CURSOR_ROLL;
				case LC_OVERLAY_XYZ: return LC_CURSOR_ORBIT;
				default:
					LC_ASSERT_FALSE("Unknown cursor type.");
					return LC_CURSOR_ORBIT;
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

		m_Project->Render(this, false);

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

	m_Viewport[0] = 0;
	m_Viewport[1] = 0;
	m_Viewport[2] = cx;
	m_Viewport[3] = cy;

	UpdateOverlayScale();
}

Matrix44 View::GetProjectionMatrix() const
{
	if (!m_Camera)
		return IdentityMatrix44();

	float Aspect = (float)m_nWidth/(float)m_nHeight;

	if (m_Camera->IsOrtho())
	{
		float ymax, ymin, xmin, xmax, znear, zfar;
		Vector3 frontvec = Vector3(m_Camera->m_ViewWorld[2]);//m_Target - m_Eye; // FIXME: free ortho cameras = crash
		ymax = (Length(frontvec))*sinf(DTOR*m_Camera->m_FOV/2);
		ymin = -ymax;
		xmin = ymin * Aspect;
		xmax = ymax * Aspect;
		znear = m_Camera->m_NearDist;
		zfar = m_Camera->m_FarDist;
		return CreateOrthoMatrix(xmin, xmax, ymin, ymax, znear, zfar);
	}
	else
		return CreatePerspectiveMatrix(m_Camera->m_FOV, Aspect, m_Camera->m_NearDist, m_Camera->m_FarDist);
}

void View::UpdateOverlayScale()
{
	Matrix44 Projection = GetProjectionMatrix();
	const Vector3& Center = m_Project->GetOverlayCenter();

	// Calculate the scaling factor by projecting the center to the front plane then
	// projecting a point close to it back.
	Vector3 Screen = ProjectPoint(Center, m_Camera->m_WorldView, Projection, m_Viewport);
	Screen[0] += 10.0f;
	Vector3 Point = UnprojectPoint(Screen, m_Camera->m_WorldView, Projection, m_Viewport);

	Vector3 Dist = Point - Center;
	m_OverlayScale = Length(Dist) * 5.0f;
}

void View::SetCamera(lcCamera* Camera)
{
	if (Camera)
		m_CameraName = Camera->m_Name;
	else
		m_CameraName = "";

	m_Camera = Camera;
}

void View::UpdateCamera()
{
	lcCamera* Camera = m_Project->m_ActiveModel->GetCamera(m_CameraName);

	if (!Camera)
		Camera = m_Project->m_ActiveModel->GetCamera(LC_CAMERA_MAIN);

	m_Camera = Camera;
	m_CameraName = m_Camera->m_Name;
}
