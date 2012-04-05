// TerrWnd.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "TerrWnd.h"
#include "Terrain.h"

#include "camera.h"
#include "Tools.h"
#include "Matrix.h"
#include "Vector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTerrainWnd

CTerrainWnd::CTerrainWnd(Terrain* pTerrain)
{
	m_pCamera = new Camera(20,20,20,0,0,0, NULL);
	m_pTerrain = pTerrain;
	m_pPalette = NULL;
	m_pDC = NULL;
	m_hglRC = 0;
	m_nAction = TERRAIN_ZOOM;
}

CTerrainWnd::~CTerrainWnd()
{
	delete m_pCamera;
}


BEGIN_MESSAGE_MAP(CTerrainWnd, CWnd)
	//{{AFX_MSG_MAP(CTerrainWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTerrainWnd message handlers

BOOL CTerrainWnd::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CTerrainWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();

	if (m_pPalette)
	{
		m_pDC->SelectPalette(m_pPalette, FALSE);
		m_pDC->RealizePalette();
	}

	pfnwglMakeCurrent(m_pDC->m_hDC, m_hglRC);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float aspect = (float)m_szView.cx/(float)m_szView.cy;
	glViewport(0, 0, m_szView.cx, m_szView.cy);

	m_pCamera->LoadProjection(aspect);

	m_pTerrain->Render(m_pCamera, aspect);

  glFlush();
	OpenGLSwapBuffers (dc.m_hDC);
	pfnwglMakeCurrent (oldDC, oldRC);
}

void CTerrainWnd::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	if (cy < 1) cy = 1;
	m_szView.cx = cx;
	m_szView.cy = cy;
}

void CTerrainWnd::OnDestroy() 
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

int CTerrainWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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
		return -1;

	if (!OpenGLSetPixelFormat(m_pDC->m_hDC, nPixelFormat, &pfd))
		return -1;

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
		return -1;

	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();
	pfnwglMakeCurrent (m_pDC->m_hDC, m_hglRC);

	// Initialize OpenGL the way we want it.
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

	pfnwglMakeCurrent(oldDC, oldRC);

	return 0;
}

void CTerrainWnd::LoadTexture(bool linear)
{
	HDC oldDC = pfnwglGetCurrentDC();
	HGLRC oldRC = pfnwglGetCurrentContext();

	pfnwglMakeCurrent(m_pDC->m_hDC, m_hglRC);
	m_pTerrain->LoadTexture(linear);
	pfnwglMakeCurrent(oldDC, oldRC);
}

void CTerrainWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	m_ptMouse = point;
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CTerrainWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (nFlags & MK_LBUTTON)
	{
		switch (m_nAction)
		{
			case TERRAIN_ZOOM:
			{
				m_pCamera->DoZoom(point.y - m_ptMouse.y, 11, 1, false, false);
				InvalidateRect (NULL, FALSE);
			} break;

			case TERRAIN_PAN:
			{
				m_pCamera->DoPan(point.x - m_ptMouse.x, point.y - m_ptMouse.y, 11, 1, false, false);
				InvalidateRect (NULL, FALSE);
			} break;

			case TERRAIN_ROTATE:
			{
				float center[3] = { 0,0,0 };
				if (point == m_ptMouse)
					break;
				m_pCamera->DoRotate(point.x - m_ptMouse.x, point.y - m_ptMouse.y, 11, 1, false, false, center);
				InvalidateRect (NULL, FALSE);
			} break;
		}

		m_ptMouse = point;
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CTerrainWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	CWnd::OnLButtonUp(nFlags, point);
}

BOOL CTerrainWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	UINT c;

	switch (m_nAction)
	{
		case TERRAIN_ZOOM:	c = IDC_ZOOM;	break;
		case TERRAIN_PAN:	c = IDC_PAN;	break;
		case TERRAIN_ROTATE:c = IDC_ANGLE;	break;
		default: 
			return CWnd::OnSetCursor(pWnd, nHitTest, message);
	}
	
	SetCursor(theApp.LoadCursor(c));
	return TRUE;
}

void CTerrainWnd::ResetCamera()
{
	delete m_pCamera;
	m_pCamera = new Camera(20,20,20,0,0,0, NULL);
}
