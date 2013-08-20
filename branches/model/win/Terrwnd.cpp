#include "lc_global.h"
#include "LeoCAD.h"
#include "TerrWnd.h"
#include "Terrain.h"

#include "camera.h"
#include "Tools.h"

lcTerrainView::lcTerrainView(GLWindow* Share, Terrain* pTerrain)
	: GLWindow(Share)
{
	mCamera = new Camera(20, 20, 20, 0, 0, 0);
	mTerrain = pTerrain;
	mAction = TERRAIN_ZOOM;
	mMouseDown = false;
}

lcTerrainView::~lcTerrainView()
{
	delete mCamera;
}

void lcTerrainView::OnInitialUpdate()
{
	MakeCurrent();

	float ambient [] = {0.0f, 0.0f, 0.0f, 1.0f};
	float diffuse [] = {0.8f, 0.9f, 0.6f, 1.0f};
	float specular[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float position[] = {0.0f, 5.0f,15.0f, 0.0f};

	glShadeModel(GL_SMOOTH);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_CULL_FACE);
}

void lcTerrainView::OnDraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport(0, 0, m_nWidth, m_nHeight);

	mCamera->LoadProjection(aspect);

	mTerrain->Render(mCamera, aspect);
}

void lcTerrainView::OnLeftButtonDown(int x, int y, bool Control, bool Shift)
{
	mMouseX = x;
	mMouseY = y;
	mMouseDown = true;

	CaptureMouse();
}

void lcTerrainView::OnLeftButtonUp(int x, int y, bool Control, bool Shift)
{
	mMouseDown = false;

	ReleaseCapture();
}

void lcTerrainView::OnMouseMove(int x, int y, bool Control, bool Shift)
{
	if (!mMouseDown)
		return;

	switch (mAction)
	{
		case TERRAIN_ZOOM:
			mCamera->DoZoom(y - mMouseY, 11, 1, false, false);
			Redraw();
			break;

		case TERRAIN_PAN:
			mCamera->DoPan(x - mMouseX, y - mMouseY, 11, 1, false, false);
			Redraw();
			break;

		case TERRAIN_ROTATE:
			if (mMouseX != x || mMouseY != y)
			{
				float center[3] = { 0,0,0 };
				mCamera->DoRotate(x - mMouseX, y - mMouseY, 11, 1, false, false, center);
				Redraw();
			}
		break;
	}

	mMouseX = x;
	mMouseY = y;
}

void lcTerrainView::LoadTexture()
{
	MakeCurrent();
	mTerrain->LoadTexture();
}

void lcTerrainView::ResetCamera()
{
	delete mCamera;
	mCamera = new Camera(20, 20, 20, 0, 0, 0);
}

void lcTerrainView::SetAction(int Action)
{
	mAction = Action;

	switch (mAction)
	{
	case TERRAIN_ZOOM:
		SetCursor(LC_CURSOR_ZOOM);
		break;
	case TERRAIN_PAN:
		SetCursor(LC_CURSOR_PAN);
		break;
	case TERRAIN_ROTATE:
		SetCursor(LC_CURSOR_ROTATE_VIEW);
		break;
	}
}
