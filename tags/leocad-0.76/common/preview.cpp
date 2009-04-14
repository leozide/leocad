//
// Piece Preview window
//

#include "lc_global.h"
#include "preview.h"
#include "project.h"
#include "pieceinf.h"
#include "system.h"
#include "lc_application.h"

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

void PiecePreview::ProcessMessage(lcMessageType Message, void* Data)
{
	if (Message == LC_MSG_COLOR_CHANGED)
	{
		Redraw();
	}
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_DITHER);
	glShadeModel(GL_FLAT);

	float Aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport(0, 0, m_nWidth, m_nHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, Aspect, 1.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Vector3 Eye(0, 0, 1.0f);
	Matrix33 Rot;

	Rot = MatrixFromAxisAngle(Vector4(1, 0, 0, -m_RotateX * LC_DTOR));
	Eye = Mul(Eye, Rot);

	Rot = MatrixFromAxisAngle(Vector4(0, 0, 1, -m_RotateZ * LC_DTOR));
	Eye = Mul(Eye, Rot);

	if (m_AutoZoom)
	{
		Eye = Eye * 100.0f;
		m_Selection->ZoomExtents(30.0f, Aspect, Eye);

		// Update the new camera distance.
		Vector3 d = Eye - m_Selection->GetCenter();
		m_Distance = Length(d);
	}
	else
	{
		Matrix44 WorldToView;
		WorldToView = CreateLookAtMatrix(Eye * m_Distance, m_Selection->GetCenter(), Vector3(0, 0, 1));
		glLoadMatrixf(WorldToView);
	}

	m_Selection->RenderPiece(lcGetActiveProject()->GetCurrentColor());

	glFinish();
	SwapBuffers();
}

void PiecePreview::SetSelection(void* Selection)
{
	MakeCurrent();

	if (m_Selection != NULL)
		m_Selection->DeRef();

	m_Selection = (PieceInfo*)Selection;

	if (m_Selection != NULL)
	{
		m_Selection->AddRef();
		lcGetActiveProject()->SetCurrentPiece(m_Selection);
		Redraw();
	}
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
