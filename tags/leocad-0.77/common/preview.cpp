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

PiecePreview::PiecePreview(GLWindow *share)
	: GLWindow(share)
{
	m_PieceInfo = NULL;
	m_RotateX = 60.0f;
	m_RotateZ = 225.0f;
	m_Distance = 10.0f;
	m_AutoZoom = true;
	m_Tracking = LC_TRACK_NONE;
}

PiecePreview::~PiecePreview()
{
}

void PiecePreview::OnDraw()
{
	if (m_PieceInfo == NULL)
		return;

	if (!MakeCurrent())
		return;

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_DITHER);
	glShadeModel(GL_FLAT);

	float aspect = (float)m_nWidth/(float)m_nHeight;
	glViewport(0, 0, m_nWidth, m_nHeight);

	lcVector3 Eye(0.0f, 0.0f, 1.0f);

	Eye = lcMul30(Eye, lcMatrix44RotationX(-m_RotateX * LC_DTOR));
	Eye = lcMul30(Eye, lcMatrix44RotationZ(-m_RotateZ * LC_DTOR));

	if (m_AutoZoom)
	{
		Eye = Eye * 100.0f;
		m_PieceInfo->ZoomExtents(30.0f, aspect, Eye);

		// Update the new camera distance.
		lcVector3 d = Eye - m_PieceInfo->GetCenter();
		m_Distance = d.Length();
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(lcMatrix44Perspective(30.0f, aspect, 1.0f, 100.0f));
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(lcMatrix44LookAt(Eye * m_Distance, m_PieceInfo->GetCenter(), lcVector3(0, 0, 1)));
	}

	float pos[4] = { 0, 0, 10, 0 }, *bg = lcGetActiveProject()->GetBackgroundColor();
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_PieceInfo->RenderPiece(lcGetActiveProject()->GetCurrentColor());

	glFinish();
	SwapBuffers();
}

void PiecePreview::SetCurrentPiece(PieceInfo *pInfo)
{
	MakeCurrent();

	if (m_PieceInfo != NULL)
		m_PieceInfo->DeRef();

	m_PieceInfo = pInfo;

	if (m_PieceInfo != NULL)
	{
		m_PieceInfo->AddRef();
		lcGetActiveProject()->SetCurrentPiece(m_PieceInfo);
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

		if (m_RotateX > 179.5f)
			m_RotateX = 179.5f;
		else if (m_RotateX < 0.5f)
			m_RotateX = 0.5f;

		m_DownX = x;
		m_DownY = y;

		Redraw();
	}
	else if (m_Tracking == LC_TRACK_RIGHT)
	{
		// Zoom.
		m_Distance += (float)(y - m_DownY) * 0.2f;
		m_AutoZoom = false;

		if (m_Distance < 0.5f)
			m_Distance = 0.5f;

		m_DownX = x;
		m_DownY = y;

		Redraw();
	}
}
