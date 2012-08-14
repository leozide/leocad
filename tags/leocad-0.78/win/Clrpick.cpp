#include "lc_global.h"
#include "leocad.h"
#include "ClrPopup.h"
#include "ClrPick.h"
#include "lc_colors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorPicker

CColorPicker::CColorPicker()
{
	m_bActive = FALSE;
	m_nColor = -1;
	SetColorIndex(-1);
}

CColorPicker::~CColorPicker()
{
}

IMPLEMENT_DYNCREATE(CColorPicker, CButton)

BEGIN_MESSAGE_MAP(CColorPicker, CButton)
	//{{AFX_MSG_MAP(CColorPicker)
	ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(CPN_SELENDOK, OnSelEndOK)
	ON_MESSAGE(CPN_SELENDCANCEL, OnSelEndCancel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorPicker message handlers

LONG CColorPicker::OnSelEndOK(UINT /*lParam*/, LONG wParam)
{
	m_bActive = FALSE;
	SetColorIndex(wParam);

	CWnd *pParent = GetParent();
	if (pParent)
		pParent->SendMessage(CPN_SELENDOK, wParam, (LPARAM)GetDlgCtrlID());

	return TRUE;
}

LONG CColorPicker::OnSelEndCancel(UINT /*lParam*/, LONG wParam)
{
	m_bActive = FALSE;

	CWnd *pParent = GetParent();
	if (pParent)
		pParent->SendMessage(CPN_SELENDCANCEL, (WPARAM)wParam, (LPARAM)GetDlgCtrlID());

	return TRUE;
}

int CColorPicker::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CButton::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetWindowSize();    // resize appropriately
	return 0;
}

// On mouse click, create and show a CColorPopup window for colour selection
BOOL CColorPicker::OnClicked()
{
	m_bActive = TRUE;
	CRect rect;
	GetWindowRect(rect);
	new CColorPopup(CPoint(rect.left, rect.bottom), m_nColor, this, false);

	return TRUE;
}

void CColorPicker::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	ASSERT(lpDrawItemStruct);

	CDC*    pDC     = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect   rect    = lpDrawItemStruct->rcItem;
	UINT    state   = lpDrawItemStruct->itemState;
	DWORD   dwStyle = GetStyle();
	CString m_strText;

	CSize Margins(::GetSystemMetrics(SM_CXEDGE), ::GetSystemMetrics(SM_CYEDGE));

	// Draw arrow
	if (m_bActive) state |= ODS_SELECTED;
	pDC->DrawFrameControl(&m_ArrowRect, DFC_SCROLL, DFCS_SCROLLDOWN  | 
		((state & ODS_SELECTED) ? DFCS_PUSHED : 0) |
		((state & ODS_DISABLED) ? DFCS_INACTIVE : 0));

	pDC->DrawEdge(rect, EDGE_SUNKEN, BF_RECT);

	// Must reduce the size of the "client" area of the button due to edge thickness.
	rect.DeflateRect(Margins.cx, Margins.cy);
	rect.bottom +=1;

	// Fill remaining area with colour
	rect.right -= m_ArrowRect.Width()-1;

	COLORREF brushColor;
	if ((state & ODS_DISABLED) || m_nColor == -1)
		brushColor = ::GetSysColor(COLOR_3DFACE);
	else
	{
		float* Value = gColorList[m_nColor].Value;
		brushColor = RGB(Value[0] * 255.0f, Value[1] * 255.0f, Value[2] * 255.0f);
	}

	CBrush brush(brushColor);
	CBrush* pOldBrush = (CBrush*) pDC->SelectObject(&brush);
	pDC->SelectStockObject(NULL_PEN);
	pDC->Rectangle(rect);
	pDC->SelectObject(pOldBrush);

	if (lcIsColorTranslucent(m_nColor))
	{
		for (int x = rect.left; x < rect.right; x++)
		{
			for (int y = rect.top + x % 4; y < rect.bottom; y += 4)
				pDC->SetPixel(x, y, RGB(255, 255, 255));

			for (int y = rect.bottom - x % 4; y > rect.top; y -= 4)
				pDC->SetPixel(x, y, RGB(255, 255, 255));
		}
	}

	// Draw focus rect
	if (state & ODS_FOCUS) 
	{
		rect.DeflateRect(1, 1);
		pDC->DrawFocusRect(rect);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CColorPicker overrides

void CColorPicker::PreSubclassWindow() 
{
	ModifyStyle(0, BS_OWNERDRAW);        // Make it owner drawn
	CButton::PreSubclassWindow();
	SetWindowSize();                     // resize appropriately
}

/////////////////////////////////////////////////////////////////////////////
// CColorPicker attributes

int CColorPicker::GetColorIndex()
{
	return m_nColor;
}

void CColorPicker::SetColorIndex(int nColor)
{
	if (m_nColor != nColor)
	{
		m_nColor = nColor;
		if (IsWindow(m_hWnd))
			RedrawWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CColorPicker implementation

void CColorPicker::SetWindowSize()
{
	// Get size dimensions of edges
	CSize MarginSize(::GetSystemMetrics(SM_CXEDGE), ::GetSystemMetrics(SM_CYEDGE));

	// Get size of dropdown arrow
	int nArrowWidth = max(::GetSystemMetrics(SM_CXHTHUMB), 5*MarginSize.cx);
	int nArrowHeight = max(::GetSystemMetrics(SM_CYVTHUMB), 5*MarginSize.cy);
	CSize ArrowSize(max(nArrowWidth, nArrowHeight), max(nArrowWidth, nArrowHeight));

	// Get window size
	CRect rect;
	GetWindowRect(rect);

	CWnd* pParent = GetParent();
	if (pParent)
		pParent->ScreenToClient(rect);

	// Set window size at least as wide as 2 arrows, and as high as arrow + margins
	int nWidth = max(rect.Width(), 2*ArrowSize.cx + 2*MarginSize.cx);
	int nHeight = max(rect.Height(), ArrowSize.cy + 2*MarginSize.cy);
	MoveWindow(rect.left, rect.top, nWidth, nHeight, TRUE);

	// Get the new coords of this window
	GetWindowRect(rect);
	ScreenToClient(rect);

	// Get the rect where the arrow goes, and convert to client coords.
	m_ArrowRect.SetRect(rect.right - ArrowSize.cx - MarginSize.cx, 
	                    rect.top + MarginSize.cy, rect.right - MarginSize.cx,
	                    rect.bottom - MarginSize.cy);
}
