//
// Piece Preview window
//

#include "preview.h"
#include "globals.h"
#include "project.h"
#include "pieceinf.h"

PiecePreview::PiecePreview(GLWindow *share)
	: GLWindow(share)
{
	m_PieceInfo = NULL;
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, aspect, 1.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	m_PieceInfo->ZoomExtents(30.0f, aspect);

	float pos[4] = { 0, 0, 10, 0 }, *bg = project->GetBackgroundColor ();
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_PieceInfo->RenderPiece(project->GetCurrentColor ());

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
		project->SetCurrentPiece(m_PieceInfo);
		Redraw();
	}
}
