#include "lc_global.h"
#include "view.h"
#include "lc_mainwindow.h"
#include "project.h"
#include "lc_camera.h"
#include "system.h"

View::View(Project *project)
{
	m_Project = project;
	mCamera = NULL;
	m_OverlayScale = 1.0f;

	if (gMainWindow->mActiveView)
		SetCamera(gMainWindow->mActiveView->mCamera, false);
	else
		SetDefaultCamera();
}

View::~View()
{
	if (gMainWindow)
	{
		gMainWindow->mViews.Remove(this);

		if (gMainWindow->mActiveView == this)
		{
			if (gMainWindow->mViews.GetSize() > 0)
				gMainWindow->SetActiveView(gMainWindow->mViews[0]);
		}
	}

	if (mCamera && mCamera->IsSimple())
		delete mCamera;
}

void View::SetCamera(lcCamera* Camera, bool ForceCopy)
{
	if (Camera->IsSimple() || ForceCopy)
	{
		if (!mCamera || !mCamera->IsSimple())
			mCamera = new lcCamera(true);

		mCamera->CopySettings(Camera);
	}
	else
	{
		if (mCamera && mCamera->IsSimple())
			delete mCamera;

		mCamera = Camera;
	}
}

void View::SetDefaultCamera()
{
	if (!mCamera || !mCamera->IsSimple())
		mCamera = new lcCamera(true);

	mCamera->SetViewpoint(LC_VIEWPOINT_HOME);
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
			LC_ASSERT(false);
			return LC_CURSOR_DEFAULT;
	}
}

void View::OnDraw()
{
	m_Project->Render(this, false);
}

void View::OnInitialUpdate()
{
	MakeCurrent();

	gMainWindow->mViews.Add(this);

	if (!gMainWindow->mActiveView)
		gMainWindow->SetActiveView(this);

	m_Project->RenderInitialize();
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