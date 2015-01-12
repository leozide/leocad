#include "lc_global.h"
#include "preview.h"
#include "project.h"
#include "lc_model.h"
#include "pieceinf.h"
#include "system.h"
#include "lc_application.h"
#include "lc_mainwindow.h"

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
	if (m_PieceInfo)
		m_PieceInfo->Release();
}

void PiecePreview::OnDraw()
{
	if (m_PieceInfo == NULL)
		return;

	mContext->SetDefaultState();

	float aspect = (float)mWidth/(float)mHeight;
	mContext->SetViewport(0, 0, mWidth, mHeight);

	lcGetActiveModel()->DrawBackground(mContext);

	lcVector3 Eye(0.0f, 0.0f, 1.0f);

	Eye = lcMul30(Eye, lcMatrix44RotationX(-m_RotateX * LC_DTOR));
	Eye = lcMul30(Eye, lcMatrix44RotationZ(-m_RotateZ * LC_DTOR));

	lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(30.0f, aspect, 1.0f, 2500.0f);
	lcMatrix44 ViewMatrix;

	if (m_AutoZoom)
	{
		Eye = Eye * 100.0f;
		m_PieceInfo->ZoomExtents(ProjectionMatrix, ViewMatrix, Eye);

		// Update the new camera distance.
		lcVector3 d = Eye - m_PieceInfo->GetCenter();
		m_Distance = d.Length();
	}
	else
	{
		ViewMatrix = lcMatrix44LookAt(Eye * m_Distance, m_PieceInfo->GetCenter(), lcVector3(0, 0, 1));
	}

	mContext->SetProjectionMatrix(ProjectionMatrix);

	lcScene Scene;
	Scene.ViewMatrix = ViewMatrix;

	m_PieceInfo->AddRenderMeshes(Scene, lcMatrix44Identity(), gMainWindow->mColorIndex, false, false);

	Scene.OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	Scene.TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);

	mContext->DrawOpaqueMeshes(ViewMatrix, Scene.OpaqueMeshes);
	mContext->DrawTranslucentMeshes(ViewMatrix, Scene.TranslucentMeshes);

	mContext->UnbindMesh(); // context remove
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
	}
}

void PiecePreview::OnLeftButtonUp()
{
	if (m_Tracking == LC_TRACK_LEFT)
		m_Tracking = LC_TRACK_NONE;
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
	}
}

void PiecePreview::OnRightButtonUp()
{
	if (m_Tracking == LC_TRACK_RIGHT)
		m_Tracking = LC_TRACK_NONE;
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
