#include "lc_global.h"
#include "IPEdit.h"
#include "TerrCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit

CInPlaceEdit::CInPlaceEdit(CWnd* pParent, CRect& rect, DWORD dwStyle, UINT nID,
						   float* pHeight, CString sInitText, 
						   UINT nFirstChar)
{
	m_sInitText 	= sInitText;
	m_pHeight		= pHeight;
	m_nLastChar 	= 0;
	// If mouse click brought us here,
	// then no exit on arrows
	m_bExitOnArrows = (nFirstChar != VK_LBUTTON);
	
	DWORD dwEditStyle = WS_BORDER|WS_CHILD|WS_VISIBLE| ES_AUTOHSCROLL //|ES_MULTILINE
		| dwStyle;
	if (!Create(dwEditStyle, rect, pParent, nID)) return;
	
	SetFont(pParent->GetFont());
	
	SetWindowText(sInitText);
	SetFocus();
	ResizeControl();
	
	switch (nFirstChar)
	{
	case VK_LBUTTON: 
	case VK_RETURN:   SetSel((int)_tcslen(m_sInitText), -1); return;
	case VK_BACK:	  SetSel((int)_tcslen(m_sInitText), -1); break;
	case VK_DOWN: 
	case VK_UP:   
	case VK_RIGHT:
	case VK_LEFT:  
	case VK_NEXT:  
	case VK_PRIOR: 
	case VK_HOME:
	case VK_SPACE:
	case VK_END:	  SetSel(0,-1); return;
	default:		  SetSel(0,-1);
	}
	
	SendMessage(WM_CHAR, nFirstChar);
}

CInPlaceEdit::~CInPlaceEdit()
{
}

BEGIN_MESSAGE_MAP(CInPlaceEdit, CEdit)
	//{{AFX_MSG_MAP(CInPlaceEdit)
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit message handlers

// If an arrow key (or associated) is pressed, then exit if
//	a) The Ctrl key was down, or
//	b) m_bExitOnArrows == TRUE
void CInPlaceEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if ((nChar == VK_PRIOR || nChar == VK_NEXT ||
		nChar == VK_DOWN  || nChar == VK_UP   ||
		nChar == VK_RIGHT || nChar == VK_LEFT) &&
		(m_bExitOnArrows || GetKeyState(VK_CONTROL) < 0))
	{
		m_nLastChar = nChar;
		GetParent()->SetFocus();
		return;
	}
	
	if (nChar == VK_ESCAPE) 
	{
		SetWindowText(m_sInitText); // restore previous text
		m_nLastChar = nChar;
		GetParent()->SetFocus();
		return;
	}
	
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

// Need to keep a lookout for Tabs, Esc and Returns. These send a 
// "KeyUp" message, but no "KeyDown". That's why I didn't put their
// code in OnKeyDown. (I will never understand windows...)
void CInPlaceEdit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE)
	{
		m_nLastChar = nChar;
		GetParent()->SetFocus(); // This will destroy this window
		return;
	}
	
	CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
}

// As soon as this edit loses focus, kill it.
void CInPlaceEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	EndEdit();
}

void CInPlaceEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Prevent beeping
	if (nChar != VK_TAB && nChar != VK_RETURN && nChar != VK_ESCAPE)
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	
		// Resize edit control if needed
		ResizeControl();
	}
}

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit overrides

// Stoopid win95 accelerator key problem workaround - Matt Weagle.
BOOL CInPlaceEdit::PreTranslateMessage(MSG* pMsg) 
{
	// Make sure that the keystrokes continue to the appropriate handlers
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP)
	{
		::TranslateMessage(pMsg);
		::DispatchMessage(pMsg);
		return TRUE;
	}	
	
	// Catch the Alt key so we don't choke if focus is going to an owner drawn button
	if (pMsg->message == WM_SYSCHAR)
		return TRUE;
	
	return CWnd::PreTranslateMessage(pMsg);
}

// Auto delete
void CInPlaceEdit::PostNcDestroy() 
{
	CEdit::PostNcDestroy();
	
	delete this;	
}

////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit implementation

void CInPlaceEdit::EndEdit()
{
	CString str;
	GetWindowText(str);
	BOOL bModified = FALSE;

	float f;
	if (sscanf(str, "%f", &f))
	{
		if (*m_pHeight != f)
		{
			*m_pHeight = f;
			bModified = TRUE;
		}
	}

	CWnd* pOwner = GetOwner();
	if (pOwner)
		pOwner->SendMessage(WM_LC_EDIT_CLOSED, m_nLastChar, bModified);

	// Close this window (PostNcDestroy will delete this)
	PostMessage(WM_CLOSE, 0, 0);
}

void CInPlaceEdit::ResizeControl()
{
	// Get text extent
	CString str;
	GetWindowText(str);
	str += "0";
	
	CWindowDC dc(this);
	CFont *pFontDC = dc.SelectObject(GetFont());
	CSize size = dc.GetTextExtent(str);
	dc.SelectObject(pFontDC);
	
	size.cx += 5;	// add some extra buffer
	
	// Get client rect
	CRect rect, parentrect;
	GetClientRect(&rect);
	GetParent()->GetClientRect(&parentrect);
	
	// Transform rect to parent coordinates
	ClientToScreen(&rect);
	GetParent()->ScreenToClient(&rect);
	
	// Check whether control needs to be resized
	// and whether there is space to grow
	if (size.cx > rect.Width())
	{
		if (size.cx + rect.left < parentrect.right)
			rect.right = rect.left + size.cx;
		else
			rect.right = parentrect.right;
		rect.bottom = rect.top + size.cy + 4;
		MoveWindow(&rect);
	}
}
