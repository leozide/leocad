#include "lc_global.h"
#include "preview.h"
#include "project.h"
#include "pieceinf.h"
#include "system.h"
#include "lc_application.h"
#include "lc_mainwnd.h"

PiecePreview::PiecePreview()
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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float aspect = (float)mWidth/(float)mHeight;
	glViewport(0, 0, mWidth, mHeight);

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

	float *bg = lcGetActiveProject()->GetBackgroundColor();
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_PieceInfo->RenderPiece(gMainWindow->mColorIndex);
}

void PiecePreview::SetCurrentPiece(PieceInfo *pInfo)
{
	MakeCurrent();

	if (m_PieceInfo != NULL)
		m_PieceInfo->Release();

	m_PieceInfo = pInfo;

	if (m_PieceInfo != NULL)
	{
		m_PieceInfo->AddRef();
		lcGetActiveProject()->SetCurrentPiece(m_PieceInfo);
		Redraw();
	}
}

void PiecePreview::OnLeftButtonDown()
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = mInputState.x;
		m_DownY = mInputState.y;
		m_Tracking = LC_TRACK_LEFT;
		CaptureMouse();
	}
}

void PiecePreview::OnLeftButtonUp()
{
	if (m_Tracking == LC_TRACK_LEFT)
	{
		m_Tracking = LC_TRACK_NONE;
		ReleaseMouse();
	}
}

void PiecePreview::OnLeftButtonDoubleClick()
{
	m_AutoZoom = true;
	Redraw();
}

void PiecePreview::OnRightButtonDown()
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = mInputState.x;
		m_DownY = mInputState.y;
		m_Tracking = LC_TRACK_RIGHT;
		CaptureMouse();
	}
}

void PiecePreview::OnRightButtonUp()
{
	if (m_Tracking == LC_TRACK_RIGHT)
	{
		m_Tracking = LC_TRACK_NONE;
		ReleaseMouse();
	}
}

void PiecePreview::OnMouseMove()
{
	if (m_Tracking == LC_TRACK_LEFT)
	{
		// Rotate.
		m_RotateZ += mInputState.x - m_DownX;
		m_RotateX += mInputState.y - m_DownY;

		if (m_RotateX > 179.5f)
			m_RotateX = 179.5f;
		else if (m_RotateX < 0.5f)
			m_RotateX = 0.5f;

		m_DownX = mInputState.x;
		m_DownY = mInputState.y;

		Redraw();
	}
	else if (m_Tracking == LC_TRACK_RIGHT)
	{
		// Zoom.
		m_Distance += (float)(m_DownY - mInputState.y) * 0.2f;
		m_AutoZoom = false;

		if (m_Distance < 0.5f)
			m_Distance = 0.5f;

		m_DownX = mInputState.x;
		m_DownY = mInputState.y;

		Redraw();
	}
}
