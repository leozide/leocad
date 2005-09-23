// PiecePrv.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "PiecePrv.h"
#include "Tools.h"
#include "pieceinf.h"
#include "globals.h"
#include "project.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPiecePreview

CPiecePreview::CPiecePreview()
{
	m_pPalette = NULL;
	m_hglRC = 0;
	m_pDC = NULL;
	m_pPieceInfo = NULL;
}

CPiecePreview::~CPiecePreview()
{
}


BEGIN_MESSAGE_MAP(CPiecePreview, CWnd)
	//{{AFX_MSG_MAP(CPiecePreview)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPiecePreview message handlers

BOOL CPiecePreview::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CPiecePreview::SetPieceInfo(PieceInfo* pInfo)
{
	if (m_pPieceInfo != NULL)
		m_pPieceInfo->DeRef();
	m_pPieceInfo = pInfo;
	m_pPieceInfo->AddRef();
}

void CPiecePreview::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	if (!IsWindowEnabled() || (m_pPieceInfo == NULL))
		return;

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();

	if (m_pPalette)
	{
		m_pDC->SelectPalette(m_pPalette, FALSE);
		m_pDC->RealizePalette();
	}

	pfnwglMakeCurrent(m_pDC->m_hDC, m_hglRC);

	double aspect = (float)m_szView.cx/(float)m_szView.cy;
	glViewport(0, 0, m_szView.cx, m_szView.cy);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, aspect, 1.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt (-5, -5, 4, 0, 0, 0, 0, 0, 1);
	m_pPieceInfo->ZoomExtents(30.0f, (float)aspect);

	float pos[4] = { 0, 0, 10, 0 }, *bg = project->GetBackgroundColor();
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_pPieceInfo->RenderPiece(project->GetCurrentColor());

  glFinish();
	OpenGLSwapBuffers (m_pDC->m_hDC);
	pfnwglMakeCurrent (oldDC, oldRC);
}

void CPiecePreview::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if (cy < 1) cy = 1;
	m_szView.cx = cx;
	m_szView.cy = cy;
}

int CPiecePreview::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pDC = new CClientDC(this);
	ASSERT(m_pDC != NULL);

	// Fill in the Pixel Format Descriptor
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0, sizeof(PIXELFORMATDESCRIPTOR));
		
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);	
	pfd.nVersion = 1;
	pfd.dwFlags  =	PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
		
	int nPixelFormat = OpenGLChoosePixelFormat(m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return 0;

	if (!OpenGLSetPixelFormat(m_pDC->m_hDC, nPixelFormat, &pfd))
		return 0;

	m_pPalette = new CPalette;

	if (CreateRGBPalette(m_pDC->m_hDC, &m_pPalette))
	{
		m_pDC->SelectPalette(m_pPalette, FALSE);
		m_pDC->RealizePalette();
	}
	else
	{
		delete m_pPalette;
		m_pPalette = NULL;
	}
		
	// Create a rendering context.
	m_hglRC = pfnwglCreateContext(m_pDC->m_hDC);
	if (!m_hglRC)
		return 0;

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();
	pfnwglMakeCurrent (m_pDC->m_hDC, m_hglRC);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the material color to follow the current color
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glDisable (GL_DITHER);
	glShadeModel (GL_FLAT);

	pfnwglMakeCurrent (oldDC, oldRC);
	pfnwglShareLists (oldRC, m_hglRC);

  return 0;
}

void CPiecePreview::OnDestroy() 
{
	if (m_pPalette)
	{
		CClientDC dc(this);
	    CPalette palDefault;
		palDefault.CreateStockObject(DEFAULT_PALETTE);
		dc.SelectPalette(&palDefault, FALSE);
		delete m_pPalette;
	}

	if (m_hglRC)
		pfnwglDeleteContext(m_hglRC);
	if (m_pDC)
		delete m_pDC;

	CWnd::OnDestroy();
}
