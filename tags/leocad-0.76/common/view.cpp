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
#include "lc_mesh.h"
#include "lc_colors.h"
#include "texfont.h"
#include "lc_application.h"
#include "system.h"

View::View(Project *pProject, GLWindow *share)
	: GLWindow(share)
{
	m_Project = pProject;
	mCamera = NULL;//pProject->m_ActiveModel->GetCamera(LC_CAMERA_MAIN);

	mTransitionActive = false;

	m_ViewCubeTrack = false;
	m_ViewCubeHover = 0;
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
	if (mTransitionActive)
		UpdateViewpointTransition();

	MakeCurrent();
	m_Project->Render(this, true, true);
	SwapBuffers();
}

void View::OnInitialUpdate()
{
	GLWindow::OnInitialUpdate();
	m_Project->AddView(this);
}

void View::OnLeftButtonDown(int x, int y, bool bControl, bool bShift)
{
	if (m_ViewCubeHover)
	{
		m_ViewCubeTrack = true;
		CaptureMouse();
		ViewCubeClick();
	}
	else
		m_Project->OnLeftButtonDown(this, x, y, bControl, bShift);
}

void View::OnLeftButtonUp(int x, int y, bool bControl, bool bShift)
{
	if (m_ViewCubeTrack)
	{
		m_ViewCubeTrack = false;
		ReleaseMouse();
	}
	else
		m_Project->OnLeftButtonUp(this, x, y, bControl, bShift);
}

void View::OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
		m_Project->OnLeftButtonDoubleClick(this, x, y, bControl, bShift);
}

void View::OnMiddleButtonDown(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
		m_Project->OnMiddleButtonDown(this, x, y, bControl, bShift);
}

void View::OnMiddleButtonUp(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
		m_Project->OnMiddleButtonUp(this, x, y, bControl, bShift);
}

void View::OnRightButtonDown(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
		m_Project->OnRightButtonDown(this, x, y, bControl, bShift);
}

void View::OnRightButtonUp(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
		m_Project->OnRightButtonUp(this, x, y, bControl, bShift);
}

void View::OnMouseMove(int x, int y, bool bControl, bool bShift)
{
	if (!m_ViewCubeTrack)
	{
		int CubeHover = ViewCubeHitTest(x, y);
		if (CubeHover != m_ViewCubeHover)
		{
			m_ViewCubeHover = CubeHover;
			Redraw();
		}

		m_Project->OnMouseMove(this, x, y, bControl, bShift);
	}
}

void View::OnSize(int cx, int cy)
{
	GLWindow::OnSize(cx, cy);

	mViewport[0] = 0;
	mViewport[1] = 0;
	mViewport[2] = cx;
	mViewport[3] = cy;

	UpdateOverlayScale();
}

Matrix44 View::GetProjectionMatrix() const
{
	const lcViewpoint* Viewpoint = GetViewpoint();

	float Aspect = (float)m_nWidth/(float)m_nHeight;

	/*
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
	*/
		return CreatePerspectiveMatrix(Viewpoint->mFOV, Aspect, Viewpoint->mNearDist, Viewpoint->mFarDist);
}

void View::UpdateOverlayScale()
{
	Matrix44 Projection = GetProjectionMatrix();
	const Vector3& Center = m_Project->GetOverlayCenter();
	lcViewpoint* Viewpoint = GetViewpoint();

	// Calculate the scaling factor by projecting the center to the front plane and then
	// projecting a point close to it back.
	Vector3 Screen = ProjectPoint(Center, Viewpoint->mWorldView, Projection, mViewport);
	Screen[0] += 10.0f;
	Vector3 Point = UnprojectPoint(Screen, Viewpoint->mWorldView, Projection, mViewport);

	Vector3 Dist = Point - Center;
	m_OverlayScale = Length(Dist) * 5.0f;
}

#define LC_VIEWPOINT_CUBE_WIDTH 100
#define LC_VIEWPOINT_CUBE_HEIGHT 100
/*
void View::UpdateCamera()
{
	lcCamera* Camera = m_Project->m_ActiveModel->GetCamera(m_CameraName);

	if (!Camera)
		Camera = m_Project->m_ActiveModel->GetCamera(LC_CAMERA_MAIN);

	m_Camera = Camera;
	m_CameraName = m_Camera->m_Name;
}
*/
void View::DrawViewCube()
{
	if (m_nWidth < LC_VIEWPOINT_CUBE_WIDTH || m_nHeight < LC_VIEWPOINT_CUBE_HEIGHT)
		return;

	Project* project = lcGetActiveProject();

	glViewport(m_nWidth - LC_VIEWPOINT_CUBE_WIDTH, m_nHeight - LC_VIEWPOINT_CUBE_HEIGHT, LC_VIEWPOINT_CUBE_WIDTH, LC_VIEWPOINT_CUBE_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	Matrix44 Projection = CreatePerspectiveMatrix(60.0f, 1.0f, 0.1f, 20.0f);
	glLoadMatrixf(Projection);

	glMatrixMode(GL_MODELVIEW);
	Matrix44 WorldView = GetViewpoint()->mWorldView;
	WorldView.m_Rows[3] = Vector4(0, 0, -4, 1);
	glLoadMatrixf(WorldView);

	lcBoxMesh->Render(LC_COLOR_DEFAULT);
	lcWireframeBoxMesh->Render(0);

	if (m_ViewCubeHover)
	{
		glColor3f(0.5f, 0.5f, 0.5f);
		int Centers = (m_ViewCubeHover & 0x2) + ((m_ViewCubeHover & 0x20) >> 4) + ((m_ViewCubeHover & 0x200) >> 8);

		float d = 0.77f;// + 0.125f;

		float x = (m_ViewCubeHover & 0x001) / 0x001 * -d + (m_ViewCubeHover & 0x004) / 0x004 * d;
		float y = (m_ViewCubeHover & 0x010) / 0x010 * -d + (m_ViewCubeHover & 0x040) / 0x040 * d;
		float z = (m_ViewCubeHover & 0x100) / 0x100 * -d + (m_ViewCubeHover & 0x400) / 0x400 * d;

		float sx = 0.25f + (m_ViewCubeHover & 0x002) / 0x002 * 0.25f;
		float sy = 0.25f + (m_ViewCubeHover & 0x020) / 0x020 * 0.25f;
		float sz = 0.25f + (m_ViewCubeHover & 0x200) / 0x200 * 0.25f;

		glPushMatrix();
		glTranslatef(x, y, z);
		glScalef(sx, sy, sz);
		lcBoxMesh->Render(4);
		glPopMatrix();
	}

	glColor3f(0.0f, 0.0f, 0.0f);

	project->m_pScreenFont->MakeCurrent();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColor4f(0, 0, 0, 1);
	int cx[6], cy[6], MaxWidth = -1;
	const char* Names[6] = { "TOP", "BOTTOM", "LEFT", "RIGHT", "FRONT", "BACK" };

	for (int i = 0; i < 6; i++)
	{
		project->m_pScreenFont->GetStringDimensions(&cx[i], &cy[i], Names[i]);
		MaxWidth = lcMax(MaxWidth, cx[i]);
		MaxWidth = lcMax(MaxWidth, cy[i]);
	}

	float Scale = 1.5f / MaxWidth;
	float Translation = 1.02f;
	Matrix44 Rotations[6] = 
	{
		Matrix44(Vector4( Scale, 0, 0, 0), Vector4(0, Scale, 0, 0), Vector4(0, 0, 1.0f, 0), Vector4(0, 0,  Translation, 1.0f)),
		Matrix44(Vector4(-Scale, 0, 0, 0), Vector4(0, Scale, 0, 0), Vector4(0, 0, 1.0f, 0), Vector4(0, 0, -Translation, 1.0f)),
		Matrix44(Vector4(0, -Scale, 0, 0), Vector4(0, 0, Scale, 0), Vector4(1.0f, 0, 0, 0), Vector4(-Translation, 0, 0, 1.0f)),
		Matrix44(Vector4(0,  Scale, 0, 0), Vector4(0, 0, Scale, 0), Vector4(1.0f, 0, 0, 0), Vector4( Translation, 0, 0, 1.0f)),
		Matrix44(Vector4( Scale, 0, 0, 0), Vector4(0, 0, Scale, 0), Vector4(0, 1.0f, 0, 0), Vector4(0, -Translation, 0, 1.0f)),
		Matrix44(Vector4(-Scale, 0, 0, 0), Vector4(0, 0, Scale, 0), Vector4(0, 1.0f, 0, 0), Vector4(0,  Translation, 0, 1.0f)),
	};

	// TODO: prebuild the mesh.

	for (int i = 0; i < 6; i++)
	{
		glPushMatrix();
		glMultMatrixf(Rotations[i]);
		project->m_pScreenFont->PrintText(-cx[i]/2.0f, cy[i]/2.0f, 0, Names[i]);
		glPopMatrix();
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glViewport(0, 0, m_nWidth, m_nHeight);
}

// Return a bit to code meaning which face/edge/corner was hit, or zero.
int View::ViewCubeHitTest(int x, int y)
{
	x -= m_nWidth - LC_VIEWPOINT_CUBE_WIDTH ;
	y -= m_nHeight - LC_VIEWPOINT_CUBE_HEIGHT;

	if (x < 0 || y < 0 || x > LC_VIEWPOINT_CUBE_WIDTH || y > LC_VIEWPOINT_CUBE_HEIGHT)
		return 0;

	Matrix44 Projection = CreatePerspectiveMatrix(60.0f, 1.0f, 0.1f, 20.0f);
	Matrix44 WorldView = GetViewpoint()->mWorldView;
	WorldView.m_Rows[3] = Vector4(0, 0, -4, 1);
	int Viewport[4] = { 0, 0, LC_VIEWPOINT_CUBE_WIDTH, LC_VIEWPOINT_CUBE_HEIGHT };

	Vector3 ClickPoints[2] = { Vector3((float)x, (float)y, 0.0f), Vector3((float)x, (float)y, 1.0f) };
	UnprojectPoints(ClickPoints, 2, WorldView, Projection, Viewport);

	BoundingBox Box(Vector3(-1,-1,-1), Vector3(1,1,1));

	Vector3 Intersection;
	float Dist;

	if (!BoundingBoxRayMinIntersectDistance(Box, ClickPoints[0], ClickPoints[1], &Dist, &Intersection))
		return 0;

	int Code = 0;

	if (Intersection[0] < -0.75f)
		Code |= 0x001;
	else if (Intersection[0] < 0.75f)
		Code |= 0x002;
	else
		Code |= 0x004;

	if (Intersection[1] < -0.75f)
		Code |= 0x010;
	else if (Intersection[1] < 0.75f)
		Code |= 0x020;
	else
		Code |= 0x040;

	if (Intersection[2] < -0.75f)
		Code |= 0x100;
	else if (Intersection[2] < 0.75f)
		Code |= 0x200;
	else
		Code |= 0x400;

	return Code;
}

void View::ViewCubeClick()
{
	Project* project = lcGetActiveProject();

	// Center and zoom around points of interest.
	lcObjArray<Vector3> Points;
	Vector3 Center;

	project->m_ActiveModel->GetPointsOfInterest(Points, Center);

	float x = (m_ViewCubeHover & 0x001) / 0x001 * -1.0f + (m_ViewCubeHover & 0x004) / 0x004 * 1.0f;
	float y = (m_ViewCubeHover & 0x010) / 0x010 * -1.0f + (m_ViewCubeHover & 0x040) / 0x040 * 1.0f;
	float z = (m_ViewCubeHover & 0x100) / 0x100 * -1.0f + (m_ViewCubeHover & 0x400) / 0x400 * 1.0f;

	Vector3 CameraPos = Center + Vector3(x, y, z) * 10.0f;

	lcViewpoint Viewpoint = *GetViewpoint();
	SetCamera1(NULL);

	u32 Time = project->m_ActiveModel->m_CurFrame;
	bool AddKey = false;
	float Roll = 0;

	Viewpoint.SetPosition(Time, AddKey, CameraPos);
	Viewpoint.SetTarget(Time, AddKey, Center);
	Viewpoint.SetRoll(Time, AddKey, Roll);
	Viewpoint.CalculateMatrices();
	
	CameraPos = ZoomExtents(CameraPos, Viewpoint.mWorldView, GetProjectionMatrix(), &Points[0], Points.GetSize());

	Viewpoint.SetPosition(Time, AddKey, CameraPos);
	Viewpoint.CalculateMatrices();

	StartViewpointTransition(&Viewpoint);
}

void View::StartViewpointTransition(const lcViewpoint* Viewpoint)
{
	// FIXME: block commands while animating
	mTransitionActive = true;
	mTransitionStart = SystemGetMilliseconds();
	mViewpointStart = mViewpoint;
	mViewpointEnd = *Viewpoint;
	Redraw();
}

void View::UpdateViewpointTransition()
{
	float Elapsed = (SystemGetMilliseconds() - mTransitionStart) / 500.0f;

	if (Elapsed > 1.0f)
	{
		mTransitionActive = false;
		mViewpoint = mViewpointEnd;
		Redraw();
		return;
	}

	mViewpoint.mPosition = mViewpointStart.mPosition + (mViewpointEnd.mPosition - mViewpointStart.mPosition) * Elapsed;
	mViewpoint.mTarget = mViewpointStart.mTarget + (mViewpointEnd.mTarget - mViewpointStart.mTarget) * Elapsed;
	mViewpoint.mRoll = mViewpointStart.mRoll + (mViewpointEnd.mRoll - mViewpointStart.mRoll) * Elapsed;
	mViewpoint.CalculateMatrices();

	Redraw();
}
