//
// OpenGL window
//

#include "stdafx.h"
#include "glwindow.h"
#include "tools.h"

typedef struct
{
	HGLRC m_hrc;
	CClientDC* m_pDC;
	CPalette* m_pPal;
  HWND m_hWnd;
} GLWindowPrivate;

// ============================================================================

BOOL GLWindowPreTranslateMessage (GLWindow *wnd, MSG *pMsg)
{
  switch (pMsg->message)
  {
  case WM_PAINT:
    wnd->OnDraw ();
    break;
  case WM_CREATE:
    wnd->OnInitialUpdate ();
    break;
  case WM_DESTROY:
    wnd->DestroyContext ();
    break;
  case WM_SIZE:
    wnd->OnSize (LOWORD (pMsg->lParam), HIWORD (pMsg->lParam));
    break;
  case WM_LBUTTONDOWN:
    wnd->OnLeftButtonDown (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  case WM_LBUTTONUP:
    wnd->OnLeftButtonUp (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  case WM_LBUTTONDBLCLK:
    wnd->OnLeftButtonDoubleClick (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  case WM_RBUTTONDOWN:
    wnd->OnRightButtonDown (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  case WM_RBUTTONUP:
    wnd->OnRightButtonUp (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  case WM_MOUSEMOVE:
    wnd->OnMouseMove (LOWORD (pMsg->lParam), wnd->GetHeight () - HIWORD (pMsg->lParam) - 1,
      (pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
    break;
  }

  return FALSE;
}

// ============================================================================
// GLWindow class

GLWindow::GLWindow (GLWindow *share)
{
  m_pShare = share;
  m_pData = (GLWindowPrivate*) malloc (sizeof (GLWindowPrivate));
  memset (m_pData, 0, sizeof (GLWindowPrivate));
}

GLWindow::~GLWindow ()
{
  free (m_pData);
}

bool GLWindow::Create (void* data)
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  prv->m_hWnd = (HWND)data;
 	prv->m_pDC = new CClientDC (CWnd::FromHandle (prv->m_hWnd));
  ASSERT (prv->m_pDC != NULL);

  // Fill in the Pixel Format Descriptor
  PIXELFORMATDESCRIPTOR pfd;
  memset (&pfd,0, sizeof(PIXELFORMATDESCRIPTOR));

  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
  pfd.nVersion = 1;
  pfd.dwFlags  =  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 24;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int nPixelFormat = OpenGLChoosePixelFormat (prv->m_pDC->m_hDC, &pfd);
  if (nPixelFormat == 0)
    return false;

  if (!OpenGLSetPixelFormat (prv->m_pDC->m_hDC, nPixelFormat, &pfd))
    return false;

  prv->m_pPal = new CPalette;

  if (CreateRGBPalette (prv->m_pDC->m_hDC, &prv->m_pPal))
  {
    prv->m_pDC->SelectPalette (prv->m_pPal, FALSE);
    prv->m_pDC->RealizePalette ();
  }
  else
  {
    delete prv->m_pPal;
    prv->m_pPal = NULL;
  }

  // Create a rendering context.
  prv->m_hrc = pfnwglCreateContext (prv->m_pDC->m_hDC);
  if (!prv->m_hrc)
    return false;

  if (m_pShare)
  {
    GLWindowPrivate *share = (GLWindowPrivate*)m_pShare->m_pData;
    pfnwglShareLists (share->m_hrc, prv->m_hrc);
  }

  return true;
}

void GLWindow::DestroyContext ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (prv->m_pPal)
	{
    CPalette palDefault;
		palDefault.CreateStockObject (DEFAULT_PALETTE);
		prv->m_pDC->SelectPalette (&palDefault, FALSE);
		delete prv->m_pPal;
    prv->m_pPal = NULL;
	}

	if (prv->m_hrc)
		pfnwglDeleteContext (prv->m_hrc);
  prv->m_hrc = NULL;

  if (prv->m_pDC)
		delete prv->m_pDC;
  prv->m_pDC = NULL;
}

void GLWindow::OnInitialUpdate ()
{
  MakeCurrent ();
  GL_InitializeExtensions ();
}

bool GLWindow::MakeCurrent ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  if (prv->m_pPal)
	{
		prv->m_pDC->SelectPalette (prv->m_pPal, FALSE);
		prv->m_pDC->RealizePalette ();
	}

  return (pfnwglMakeCurrent (prv->m_pDC->m_hDC, prv->m_hrc) != 0);

//	RECT rc;
//	GetClientRect (&rc);
//	m_pFig->Resize (rc.right, rc.bottom);
}

void GLWindow::SwapBuffers ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

  OpenGLSwapBuffers (prv->m_pDC->m_hDC);
}

void GLWindow::Redraw ()
{
  GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;
  InvalidateRect (prv->m_hWnd, NULL, FALSE);
}

