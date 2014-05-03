#include "lc_global.h"
#include <stdlib.h>
#include "lc_mainwindow.h"
#include "project.h"
#include "camera.h"
#include "view.h"
#include "system.h"
#include "tr.h"

View::View(Project *project)
{
	m_Project = project;
	mCamera = NULL;
	m_OverlayScale = 1.0f;

	View* ActiveView = gMainWindow->GetActiveView();
	if (ActiveView)
		SetCamera(ActiveView->mCamera, false);
	else
		SetDefaultCamera();
}

View::~View()
{
	if (gMainWindow)
		gMainWindow->RemoveView(this);

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

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME, 1, false);
}

lcMatrix44 View::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->m_pTR)
		mCamera->m_pTR->BeginTile();

	if (mCamera->IsOrtho())
	{
		// Compute the FOV/plane intersection radius.
		//                d               d
		//   a = 2 atan(------) => ~ a = --- => d = af
		//                2f              f
		float f = (mCamera->mPosition - mCamera->mOrthoTarget).Length();
		float d = (mCamera->m_fovy * f) * (LC_PI / 180.0f);
		float r = d / 2;

		float right = r * AspectRatio;
		return lcMatrix44Ortho(-right, right, -r, r, mCamera->m_zNear, mCamera->m_zFar * 4);
	}
	else
		return lcMatrix44Perspective(mCamera->m_fovy, AspectRatio, mCamera->m_zNear, mCamera->m_zFar);
}

LC_CURSOR_TYPE View::GetCursor() const
{
	// TODO: check if we're the focused window and return just the default arrow if we aren't.

	switch (m_Project->GetAction())
	{
		case LC_TOOL_SELECT:
			if (mInputState.Control)
				return LC_CURSOR_SELECT_GROUP;
			else
				return LC_CURSOR_SELECT;

		case LC_TOOL_INSERT:
			return LC_CURSOR_BRICK;

		case LC_TOOL_LIGHT:
			return LC_CURSOR_LIGHT;

		case LC_TOOL_SPOTLIGHT:
			return LC_CURSOR_SPOTLIGHT;

		case LC_TOOL_CAMERA:
			return LC_CURSOR_CAMERA;

		case LC_TOOL_MOVE:
			return LC_CURSOR_MOVE;

		case LC_TOOL_ROTATE:
			return LC_CURSOR_ROTATE;

		case LC_TOOL_ERASER:
			return LC_CURSOR_DELETE;

		case LC_TOOL_PAINT:
			return LC_CURSOR_PAINT;

		case LC_TOOL_ZOOM:
			return LC_CURSOR_ZOOM;

		case LC_TOOL_ZOOM_REGION:
			return LC_CURSOR_ZOOM_REGION;

		case LC_TOOL_PAN:
			return LC_CURSOR_PAN;

		case LC_TOOL_ROTATE_VIEW:
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

		case LC_TOOL_ROLL:
			return LC_CURSOR_ROLL;

		default:
			LC_ASSERT_FALSE("Unknown cursor type.");
			return LC_CURSOR_DEFAULT;
	}
}

lcObjectSection View::FindObjectUnderPointer(bool PiecesOnly) const
{
	lcVector3 StartEnd[2] =
	{
		lcVector3((float)mInputState.x, (float)mInputState.y, 0.0f),
		lcVector3((float)mInputState.x, (float)mInputState.y, 1.0f)
	};

	UnprojectPoints(StartEnd, 2);

	lcObjectRayTest ObjectRayTest;

	ObjectRayTest.PiecesOnly = PiecesOnly;
	ObjectRayTest.ViewCamera = mCamera;
	ObjectRayTest.Start = StartEnd[0];
	ObjectRayTest.End = StartEnd[1];
	ObjectRayTest.Distance = FLT_MAX;
	ObjectRayTest.ObjectSection.Object = NULL;
	ObjectRayTest.ObjectSection.Section = 0;;

	m_Project->RayTest(ObjectRayTest);

	return ObjectRayTest.ObjectSection;
}

lcArray<lcObjectSection> View::FindObjectsInBox(float x1, float y1, float x2, float y2) const
{
	float Left, Top, Bottom, Right;

	if (x1 < x2)
	{
		Left = x1;
		Right = x2;
	}
	else
	{
		Left = x2;
		Right = x1;
	}

	if (y1 > y2)
	{
		Top = y1;
		Bottom = y2;
	}
	else
	{
		Top = y2;
		Bottom = y1;
	}

	lcVector3 Corners[6] =
	{
		lcVector3(Left, Top, 0),
		lcVector3(Left, Bottom, 0),
		lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0),
		lcVector3(Left, Top, 1),
		lcVector3(Right, Bottom, 1)
	};

	UnprojectPoints(Corners, 6);

	lcVector3 PlaneNormals[6];
	PlaneNormals[0] = lcNormalize(lcCross(Corners[4] - Corners[0], Corners[1] - Corners[0])); // Left
	PlaneNormals[1] = lcNormalize(lcCross(Corners[5] - Corners[2], Corners[3] - Corners[2])); // Right
	PlaneNormals[2] = lcNormalize(lcCross(Corners[3] - Corners[0], Corners[4] - Corners[0])); // Top
	PlaneNormals[3] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[5] - Corners[2])); // Bottom
	PlaneNormals[4] = lcNormalize(lcCross(Corners[1] - Corners[0], Corners[3] - Corners[0])); // Front
	PlaneNormals[5] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[3] - Corners[2])); // Back

	lcObjectBoxTest ObjectBoxTest;
	ObjectBoxTest.ViewCamera = mCamera;
	ObjectBoxTest.Planes[0] = lcVector4(PlaneNormals[0], -lcDot(PlaneNormals[0], Corners[0]));
	ObjectBoxTest.Planes[1] = lcVector4(PlaneNormals[1], -lcDot(PlaneNormals[1], Corners[5]));
	ObjectBoxTest.Planes[2] = lcVector4(PlaneNormals[2], -lcDot(PlaneNormals[2], Corners[0]));
	ObjectBoxTest.Planes[3] = lcVector4(PlaneNormals[3], -lcDot(PlaneNormals[3], Corners[5]));
	ObjectBoxTest.Planes[4] = lcVector4(PlaneNormals[4], -lcDot(PlaneNormals[4], Corners[0]));
	ObjectBoxTest.Planes[5] = lcVector4(PlaneNormals[5], -lcDot(PlaneNormals[5], Corners[5]));

	m_Project->BoxTest(ObjectBoxTest);

	return ObjectBoxTest.ObjectSections;
}


void View::OnDraw()
{
	m_Project->Render(this, false);
}

void View::OnInitialUpdate()
{
	gMainWindow->AddView(this);
}

void View::OnUpdateCursor()
{
	SetCursor(GetCursor());
}

void View::OnLeftButtonDown()
{
	m_Project->OnLeftButtonDown(this);
}

void View::OnLeftButtonUp()
{
	m_Project->OnLeftButtonUp(this);
}

void View::OnLeftButtonDoubleClick()
{
	m_Project->OnLeftButtonDoubleClick(this);
}

void View::OnMiddleButtonDown()
{
	m_Project->OnMiddleButtonDown(this);
}

void View::OnMiddleButtonUp()
{
	m_Project->OnMiddleButtonUp(this);
}

void View::OnRightButtonDown()
{
	m_Project->OnRightButtonDown(this);
}

void View::OnRightButtonUp()
{
	m_Project->OnRightButtonUp(this);
}

void View::OnMouseMove()
{
	m_Project->OnMouseMove(this);
}

void View::OnMouseWheel(float Direction)
{
	m_Project->OnMouseWheel(this, Direction);
}
