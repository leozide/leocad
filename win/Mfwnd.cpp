// GLWindow.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "MFWnd.h"
#include "Tools.h"
#include "minifig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMinifigWnd

CMinifigWnd::CMinifigWnd()
{
	m_pDC = NULL;
}

CMinifigWnd::~CMinifigWnd()
{
}

BEGIN_MESSAGE_MAP(CMinifigWnd, CWnd)
	//{{AFX_MSG_MAP(CMinifigWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMinifigWnd message handlers

int CMinifigWnd::InitGL() 
{
	m_pDC = new CClientDC(this);
	ASSERT(m_pDC != NULL);

	// Fill in the Pixel Format Descriptor
  PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0, sizeof(PIXELFORMATDESCRIPTOR));

  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
  pfd.nVersion = 1;
	pfd.dwFlags  =  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int nPixelFormat = pfnwglChoosePixelFormat(m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return -1 ;

	if (!pfnwglSetPixelFormat(m_pDC->m_hDC, nPixelFormat, &pfd))
		return -1 ;

	m_pPal = new CPalette;

	if (CreateRGBPalette(m_pDC->m_hDC, &m_pPal))
	{
		m_pDC->SelectPalette(m_pPal, FALSE);
		m_pDC->RealizePalette();
	}
	else
	{
		delete m_pPal;
		m_pPal = NULL;
	}

    // Create a rendering context.
	m_hrc = pfnwglCreateContext(m_pDC->m_hDC);
	if (!m_hrc)
		return -1;

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();
	pfnwglShareLists(oldRC, m_hrc);
	pfnwglMakeCurrent (m_pDC->m_hDC, m_hrc);

	pfnwglMakeCurrent (oldDC, oldRC);

	return 0;
}

BOOL CMinifigWnd::OnEraseBkgnd(CDC* /*pDC*/) 
{
	return TRUE;
}

void CMinifigWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	DrawScene(); 
}

void CMinifigWnd::OnDestroy() 
{
	if (m_pPal)
	{
    CPalette palDefault;
		palDefault.CreateStockObject(DEFAULT_PALETTE);
		m_pDC->SelectPalette(&palDefault, FALSE);
		delete m_pPal;
	}

	if (m_hrc)
		pfnwglDeleteContext(m_hrc);
	if (m_pDC)
		delete m_pDC;

	CWnd::OnDestroy();
}

void CMinifigWnd::DrawScene() 
{
	if (m_pPal)
	{
		m_pDC->SelectPalette(m_pPal, FALSE);
		m_pDC->RealizePalette();
	}

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();
	pfnwglMakeCurrent (m_pDC->m_hDC, m_hrc);

	RECT rc;
	GetClientRect (&rc);

	m_pFig->Resize (rc.right, rc.bottom);
  m_pFig->Redraw ();

	pfnwglSwapBuffers (m_pDC->m_hDC);
	pfnwglMakeCurrent (oldDC, oldRC);
}
