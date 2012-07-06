#include "lc_global.h"
#include "opengl.h"
#include "glwindow.h"
#include "tools.h"
#include "resource.h"

struct GLWindowPrivate
{
	HGLRC m_hrc;
	CDC* m_pDC;
	CPalette* m_pPal;
	HWND m_hWnd;
	HCURSOR Cursor;
};

// ============================================================================

BOOL GLWindowPreTranslateMessage(GLWindow *wnd, MSG *pMsg)
{
	switch (pMsg->message)
	{
		case WM_PAINT:
			{
				GLWindowPrivate* prv = (GLWindowPrivate*)wnd->GetData();
				PAINTSTRUCT ps;
				BeginPaint(prv->m_hWnd, &ps);
				wnd->OnDraw();
				EndPaint(prv->m_hWnd, &ps);
			} break;
		case WM_SIZE:
			wnd->OnSize(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
			break;
		case WM_LBUTTONDOWN:
			wnd->OnLeftButtonDown((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_LBUTTONUP:
			wnd->OnLeftButtonUp((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_LBUTTONDBLCLK:
			wnd->OnLeftButtonDoubleClick((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_MBUTTONDOWN:
			wnd->OnMiddleButtonDown((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_MBUTTONUP:
			wnd->OnMiddleButtonUp((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_RBUTTONDOWN:
			wnd->OnRightButtonDown((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_RBUTTONUP:
			wnd->OnRightButtonUp((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_MOUSEMOVE:
			wnd->OnMouseMove((SHORT)LOWORD(pMsg->lParam), wnd->GetHeight() - (SHORT)HIWORD(pMsg->lParam) - 1,
				(pMsg->wParam & MK_CONTROL) != 0, (pMsg->wParam & MK_SHIFT) != 0);
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE:
			wnd->OnInitialUpdate();
			break;
		case WM_DESTROY:
			wnd->DestroyContext();
			return FALSE;
			break;
		case WM_SETCURSOR:
			{
				GLWindowPrivate *prv = (GLWindowPrivate*)wnd->GetData();

				if (prv->Cursor)
				{
					SetCursor(prv->Cursor);
					return TRUE;
				}

				return FALSE;
			} break;
		case WM_PALETTECHANGED:
			if ((HWND)pMsg->wParam == pMsg->hwnd)  // Responding to own message.
				break;
		case WM_QUERYNEWPALETTE:
			{
				GLWindowPrivate *prv = (GLWindowPrivate*)wnd->GetData();

				if (prv->m_pPal)
				{
					prv->m_pDC->SelectPalette(prv->m_pPal, FALSE);
					if (prv->m_pDC->RealizePalette() != 0)
					{
						// Some colors changed, so we need to do a repaint.
						InvalidateRect(prv->m_hWnd, NULL, TRUE);
					}
				}
			} break;

		default:
			return FALSE;
	}

	return TRUE;
}

LRESULT CALLBACK GLWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CMapPtrToPtr WindowMap;
	GLWindow *wnd;

	if (uMsg == WM_CREATE)
	{
		LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;

		wnd = (GLWindow*)cs->lpCreateParams;
		wnd->CreateFromWindow(hwnd);

		WindowMap.SetAt(hwnd, wnd);
	}

	wnd = (GLWindow*)WindowMap[hwnd];

	if (wnd)
	{
		MSG msg;
		msg.hwnd = hwnd;
		msg.message = uMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;

		GLWindowPreTranslateMessage(wnd, &msg);

		if (uMsg == WM_DESTROY)
		{
			WindowMap.RemoveKey(hwnd);
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// GLWindow class

GLWindow::GLWindow(GLWindow *share)
{
	m_pShare = share;
	m_pData = (GLWindowPrivate*)malloc(sizeof(GLWindowPrivate));
	memset(m_pData, 0, sizeof(GLWindowPrivate));
}

GLWindow::~GLWindow()
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	delete prv->m_pDC;

	free(m_pData);
}

bool GLWindow::CreateFromWindow(void* data)
{
	GLWindowPrivate* prv = (GLWindowPrivate*)m_pData;

	prv->m_hWnd = (HWND)data;
	prv->m_pDC = new CClientDC(CWnd::FromHandle(prv->m_hWnd));
	ASSERT(prv->m_pDC != NULL);

	// Fill in the Pixel Format Descriptor
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ChoosePixelFormat(prv->m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return false;

	if (!SetPixelFormat(prv->m_pDC->m_hDC, nPixelFormat, &pfd))
		return false;

	prv->m_pPal = new CPalette;

	if (CreateRGBPalette(prv->m_pDC->m_hDC, &prv->m_pPal))
	{
		prv->m_pDC->SelectPalette(prv->m_pPal, FALSE);
		prv->m_pDC->RealizePalette();
	}
	else
	{
		delete prv->m_pPal;
		prv->m_pPal = NULL;
	}

	// Create a rendering context.
	prv->m_hrc = wglCreateContext(prv->m_pDC->m_hDC);
	if (!prv->m_hrc)
		return false;

	if (m_pShare)
	{
		GLWindowPrivate *share = (GLWindowPrivate*)m_pShare->m_pData;
		wglShareLists(share->m_hrc, prv->m_hrc);
	}

	return true;
}

bool GLWindow::CreateFromBitmap(void* Data)
{
	GLWindowPrivate* prv = (GLWindowPrivate*)m_pData;

	prv->m_pDC = new CDC;
	prv->m_pDC->Attach((HDC)Data);
	ASSERT(prv->m_pDC != NULL);

	// Fill in the Pixel Format Descriptor
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ChoosePixelFormat(prv->m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return false;

	if (!SetPixelFormat(prv->m_pDC->m_hDC, nPixelFormat, &pfd))
		return false;

	prv->m_pPal = new CPalette;

	if (CreateRGBPalette(prv->m_pDC->m_hDC, &prv->m_pPal))
	{
		prv->m_pDC->SelectPalette(prv->m_pPal, FALSE);
		prv->m_pDC->RealizePalette();
	}
	else
	{
		delete prv->m_pPal;
		prv->m_pPal = NULL;
	}

	// Create a rendering context.
	prv->m_hrc = wglCreateContext(prv->m_pDC->m_hDC);
	if (!prv->m_hrc)
		return false;

	if (m_pShare)
	{
		GLWindowPrivate *share = (GLWindowPrivate*)m_pShare->m_pData;
		wglShareLists(share->m_hrc, prv->m_hrc);
	}

	return true;
}

void GLWindow::DestroyContext()
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (prv->m_pPal)
	{
		CPalette palDefault;
		palDefault.CreateStockObject(DEFAULT_PALETTE);
		prv->m_pDC->SelectPalette(&palDefault, FALSE);
		delete prv->m_pPal;
		prv->m_pPal = NULL;
	}

	if (prv->m_hrc)
		wglDeleteContext(prv->m_hrc);
	prv->m_hrc = NULL;

	if (prv->m_pDC)
		delete prv->m_pDC;
	prv->m_pDC = NULL;
}

void GLWindow::OnInitialUpdate()
{
	MakeCurrent();
	GL_InitializeExtensions();
}

bool GLWindow::MakeCurrent()
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (prv->m_pPal)
	{
		prv->m_pDC->SelectPalette(prv->m_pPal, FALSE);
		prv->m_pDC->RealizePalette();
	}

	return (wglMakeCurrent(prv->m_pDC->m_hDC, prv->m_hrc) != 0);
}

void GLWindow::SwapBuffers()
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	::SwapBuffers(prv->m_pDC->m_hDC);
}

void GLWindow::Redraw(bool ForceRedraw)
{
	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	InvalidateRect(prv->m_hWnd, NULL, FALSE);

	if (ForceRedraw)
		UpdateWindow(prv->m_hWnd);
}

void GLWindow::CaptureMouse()
{
	GLWindowPrivate* prv = (GLWindowPrivate*)m_pData;
	SetCapture(prv->m_hWnd);
}

void GLWindow::ReleaseMouse()
{
	ReleaseCapture();
}

void GLWindow::SetCursor(LC_CURSOR_TYPE Cursor)
{
	const UINT CursorResources[LC_CURSOR_COUNT] = 
	{
		0,                // LC_CURSOR_DEFAULT
		IDC_BRICK,        // LC_CURSOR_BRICK
		IDC_LIGHT,        // LC_CURSOR_LIGHT
		IDC_SPOTLIGHT,    // LC_CURSOR_SPOTLIGHT
		IDC_CAMERA,       // LC_CURSOR_CAMERA
		IDC_SELECT,       // LC_CURSOR_SELECT
		IDC_SELECT_GROUP, // LC_CURSOR_SELECT_GROUP
		IDC_MOVE,         // LC_CURSOR_MOVE
		IDC_ROTATE,       // LC_CURSOR_ROTATE
		IDC_ROTX,         // LC_CURSOR_ROTATEX
		IDC_ROTY,         // LC_CURSOR_ROTATEY
		IDC_ERASER,       // LC_CURSOR_DELETE
		IDC_PAINT,        // LC_CURSOR_PAINT
		IDC_ZOOM,         // LC_CURSOR_ZOOM
		IDC_ZOOM_REGION,  // LC_CURSOR_ZOOM_REGION
		IDC_PAN,          // LC_CURSOR_PAN
		IDC_ROLL,         // LC_CURSOR_ROLL
		IDC_ANGLE,        // LC_CURSOR_ROTATE_VIEW
	};

	GLWindowPrivate *prv = (GLWindowPrivate*)m_pData;

	if (CursorResources[Cursor])
		prv->Cursor = AfxGetApp()->LoadCursor(CursorResources[Cursor]);
	else
		prv->Cursor = NULL;
}
