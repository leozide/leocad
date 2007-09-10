//
// Piece Preview window
//

#include "lc_global.h"
#include "preview.h"
#include "globals.h"
#include "project.h"
#include "pieceinf.h"
#include "system.h"
#include "lc_application.h"
#include "lc_scene.h"

#include "lc_pieceobj.h"
#include "lc_piece.h"
#include "lc_flexpiece.h"
#include "lc_modelref.h"

PiecePreview::PiecePreview(GLWindow *share)
	: GLWindow(share)
{
	g_App->m_PiecePreview = this;
	m_Selection = NULL;

	m_RotateX = 60.0f;
	m_RotateZ = 225.0f;
	m_Distance = 10.0f;
	m_AutoZoom = true;
	m_Tracking = LC_TRACK_NONE;
}

PiecePreview::~PiecePreview()
{
	delete m_Selection;
	g_App->m_PiecePreview = NULL;
}

void PiecePreview::SetSelection(void* Selection)
{
	MakeCurrent();

	if (lcGetActiveProject()->m_ModelList.FindIndex((lcModel*)Selection) == -1)
	{
		delete m_Selection;
		lcPiece* Piece = new lcPiece((PieceInfo*)Selection);
		m_Selection = Piece;
	}
	else
	{
		delete m_Selection;
		lcModelRef* Model = new lcModelRef((lcModel*)Selection);
		m_Selection = Model;
	}

	m_Selection->m_Color = g_App->m_SelectedColor;
	m_Selection->Update(1);

	Redraw();
}

void PiecePreview::OnDraw()
{
	if (!MakeCurrent())
		return;

	float* bg = lcGetActiveProject()->GetBackgroundColor();
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!m_Selection)
	{
		SwapBuffers();
		return;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_DITHER);
	glShadeModel(GL_FLAT);

	float Aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport(0, 0, m_nWidth, m_nHeight);

	Vector3 Target = m_Selection->m_BoundingBox.GetCenter();
	Vector3 Eye(0, 0, 1.0f);
	Matrix33 Rot;

	Rot = MatrixFromAxisAngle(Vector4(1, 0, 0, -m_RotateX * LC_DTOR));
	Eye = Mul(Eye, Rot);

	Rot = MatrixFromAxisAngle(Vector4(0, 0, 1, -m_RotateZ * LC_DTOR));
	Eye = Mul(Eye, Rot);

	Eye = Target + Eye * m_Distance;

	Matrix44 Projection = CreatePerspectiveMatrix(30.0f, Aspect, 1.0f, 100.0f);
	Matrix44 WorldView = CreateLookAtMatrix(Eye, Target, Vector3(0, 0, 1));

	if (m_AutoZoom)
	{
		Vector3 Points[8];
		m_Selection->m_BoundingBox.GetPoints(Points);

		Eye = ZoomExtents(Eye, WorldView, Projection, Points, 8);

		WorldView = CreateLookAtMatrix(Eye, Target, Vector3(0, 0, 1));
		m_Distance = Length(Eye - Target);
	}

	lcScene Scene(100, 100, 100, 100);
	Scene.m_WorldView = WorldView;

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(WorldView);

	m_Selection->AddToScene(&Scene, g_App->m_SelectedColor);
	Scene.Render();

	SwapBuffers();
}

void PiecePreview::OnLeftButtonDown(int x, int y, bool Control, bool Shift)
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = x;
		m_DownY = y;
		m_Tracking = LC_TRACK_LEFT;
		CaptureMouse();
	}
}

void PiecePreview::OnLeftButtonUp(int x, int y, bool Control, bool Shift)
{
	if (m_Tracking == LC_TRACK_LEFT)
	{
		m_Tracking = LC_TRACK_NONE;
		ReleaseMouse();
	}
}

void PiecePreview::OnLeftButtonDoubleClick(int x, int y, bool Control, bool Shift)
{
	m_AutoZoom = true;
	Redraw();
}

void PiecePreview::OnRightButtonDown(int x, int y, bool Control, bool Shift)
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = x;
		m_DownY = y;
		m_Tracking = LC_TRACK_RIGHT;
		CaptureMouse();
	}
}

void PiecePreview::OnRightButtonUp(int x, int y, bool Control, bool Shift)
{
	if (m_Tracking == LC_TRACK_RIGHT)
	{
		m_Tracking = LC_TRACK_NONE;
		ReleaseMouse();
	}
}

void PiecePreview::OnMouseMove(int x, int y, bool Control, bool Shift)
{
	if (m_Tracking == LC_TRACK_LEFT)
	{
		// Rotate.
		m_RotateZ += x - m_DownX;
		m_RotateX += y - m_DownY;

		m_RotateX = lcClamp(m_RotateX, 0.5f, 179.5f);

		m_DownX = x;
		m_DownY = y;

		Redraw();
	}
	else if (m_Tracking == LC_TRACK_RIGHT)
	{
		// Zoom.
		m_Distance += (float)(y - m_DownY) * 0.2f;
		m_AutoZoom = false;

		m_Distance = lcClamp(m_Distance, 0.5f, 50.0f);

		m_DownX = x;
		m_DownY = y;

		Redraw();
	}
}
