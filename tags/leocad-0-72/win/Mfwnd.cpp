// GLWindow.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "MFWnd.h"
#include "Tools.h"
#include "project.h"
#include "globals.h"
#include "Matrix.h"
#include "pieceinf.h"

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

    int nPixelFormat = ChoosePixelFormat(m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return -1 ;

	if (!SetPixelFormat(m_pDC->m_hDC, nPixelFormat, &pfd))
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
	m_hrc = wglCreateContext(m_pDC->m_hDC);
	if (!m_hrc)
		return -1;

	HDC oldDC = wglGetCurrentDC();
	HGLRC oldRC = wglGetCurrentContext();
	wglShareLists(oldRC, m_hrc);
	wglMakeCurrent (m_pDC->m_hDC, m_hrc);

	CRect rc;
	GetClientRect (&rc);

	double aspect = (float)rc.right/(float)rc.bottom;
	glViewport(0, 0, rc.right, rc.bottom);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, aspect, 1.0f, 50.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	gluLookAt (0, -9, 4, 0, 5, 1, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float *bg = project->GetBackgroundColor();
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DITHER);
	glShadeModel (GL_FLAT);

	wglMakeCurrent (oldDC, oldRC);

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
		wglDeleteContext(m_hrc);
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

	HDC oldDC = wglGetCurrentDC();
	HGLRC oldRC = wglGetCurrentContext();
	wglMakeCurrent (m_pDC->m_hDC, m_hrc);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = 0; i < 15; i++)
	{
		if (m_pFig->info[i] == NULL)
			continue;

		glPushMatrix();
		Matrix mat;
		float rot[4];
		mat.CreateOld(0,0,0, m_pFig->rot[i][0], m_pFig->rot[i][1], m_pFig->rot[i][2]);
		mat.ToAxisAngle(rot);
		glTranslatef(m_pFig->pos[i][0], m_pFig->pos[i][1], m_pFig->pos[i][2]);
		glRotatef(rot[3], rot[0], rot[1], rot[2]);
		m_pFig->info[i]->RenderPiece(m_pFig->colors[i]);
		glPopMatrix();
	}

	glFinish();
	SwapBuffers(m_pDC->m_hDC);
	wglMakeCurrent (oldDC, oldRC);
}
