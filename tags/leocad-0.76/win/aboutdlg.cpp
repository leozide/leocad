// AboutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "AboutDlg.h"
#include "config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

CAboutDlg::~CAboutDlg()
{
	m_Font.DeleteObject();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_ABTDLG_HOMEPAGE, OnHomepage)
	ON_WM_CTLCOLOR()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Initialize the OpenGL information box.
	CString info, tmp;
	PIXELFORMATDESCRIPTOR pfd;
	OpenGLDescribePixelFormat(m_hViewDC, OpenGLGetPixelFormat(m_hViewDC), sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	info = ("Pixel Format: ");
	if (pfd.iPixelType == PFD_TYPE_RGBA)
		info += "RGBA, ";
	else
		info += "Color Index, ";

	if (pfd.dwFlags & PFD_DOUBLEBUFFER)
		info += "Double Buffer ";
	else
		info += "Single Buffer ";

	if ((pfd.dwFlags & (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT)) == 0)
		info += "(Installable Client Driver)\r\n";
	else if ((pfd.dwFlags & (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT)) == (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT))
		info += "(Mini-Client Driver)\r\n";
	else if ((pfd.dwFlags & (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT)) == PFD_GENERIC_FORMAT)
		info += "(Generic Software Driver)\r\n";
	else if ((pfd.dwFlags & (PFD_GENERIC_ACCELERATED|PFD_GENERIC_FORMAT)) == PFD_GENERIC_ACCELERATED)
		info += "(Unknown Driver Type)\r\n";

	tmp.Format("Color bits: %d, Depth Buffer: %d bits\r\nOpenGL Version ", pfd.cColorBits, pfd.cDepthBits);
	info += tmp;
	info += (const char*)glGetString(GL_VERSION);
	info += " (";
	info += (const char*)glGetString(GL_RENDERER);
	info += " - ";
	info += (const char*)glGetString(GL_VENDOR);
	info += ")";

	SetDlgItemText(IDC_ABTDLG_INFO, info);

	// Underline the homepage link.
	LOGFONT lf;
	CFont* pFont = GetDlgItem(IDC_ABTDLG_HOMEPAGE)->GetFont();
	if (pFont != NULL)
	{
		pFont->GetLogFont(&lf);
		lf.lfUnderline = TRUE;
		m_Font.DeleteObject();
		m_Font.CreateFontIndirect(&lf);
		GetDlgItem(IDC_ABTDLG_HOMEPAGE)->SetFont(&m_Font);
	}	

	// The following function appeared in Paul DiLascia's Jan 1998 MSJ articles.
	// It loads a "hand" cursor from "winhlp32.exe" resources

	// Get the windows directory
	CString strWndDir;
	GetWindowsDirectory(strWndDir.GetBuffer(MAX_PATH), MAX_PATH);
	strWndDir.ReleaseBuffer();
	strWndDir += _T("\\winhlp32.exe");

	// This retrieves cursor #106 from winhlp32.exe, which is a hand pointer
	HMODULE hModule = LoadLibrary(strWndDir);
	if (hModule)
	{
		HCURSOR hHandCursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
		m_hLinkCursor = CopyCursor(hHandCursor);
		FreeLibrary(hModule);
	}

	AdjustHomepageWindow();

	// Fix the version number.
	SetDlgItemText(IDC_ABTDLG_VERSION, "LeoCAD Version " LC_VERSION_TEXT LC_VERSION_TAG);

	return TRUE;
}

// Move and resize the window so that its client area has the same size as the hyperlink text.
// This prevents the hyperlink cursor being active when it is not over the text.
void CAboutDlg::AdjustHomepageWindow()
{	
	CWnd *pWnd = GetDlgItem(IDC_ABTDLG_HOMEPAGE);
	ASSERT(::IsWindow(pWnd->GetSafeHwnd()));

	// Get the current window rect
	CRect rcWnd;
	pWnd->GetWindowRect(rcWnd);

	// For a child CWnd object, window rect is relative to the 
	// upper-left corner of the parent window’s client area. 
	ScreenToClient(rcWnd);

	// Get the current client rect
	CRect rcClient;
	pWnd->GetClientRect(rcClient);

	// Calc border size based on window and client rects
	int borderWidth = rcWnd.Width() - rcClient.Width();
	int borderHeight = rcWnd.Height() - rcClient.Height();

	// Get the extent of window text 
	CString strWndText;
	pWnd->GetWindowText(strWndText);
	
	CDC* pDC = pWnd->GetDC();	
	CFont* pOldFont = pDC->SelectObject(&m_Font);
	CSize Extent = pDC->GetTextExtent(strWndText);
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	// Get the text justification style
	DWORD dwStyle = pWnd->GetStyle();

	// Recalc window size and position based on text justification
	if (dwStyle & SS_CENTERIMAGE)
		rcWnd.DeflateRect(0, (rcWnd.Height() - Extent.cy) / 2);
	else
		rcWnd.bottom = rcWnd.top + Extent.cy;

	if (dwStyle & SS_CENTER)
		rcWnd.DeflateRect((rcWnd.Width() - Extent.cx) / 2, 0);
	else if (dwStyle & SS_RIGHT)
		rcWnd.left  = rcWnd.right - Extent.cx;
	else // SS_LEFT
		rcWnd.right = rcWnd.left + Extent.cx;

	// Move and resize the window
	pWnd->MoveWindow(rcWnd.left, rcWnd.top, rcWnd.Width() + borderWidth, rcWnd.Height() + borderHeight);
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_ABTDLG_HOMEPAGE)
		pDC->SetTextColor(RGB(0, 0, 255));

	return hbr;
}

BOOL CAboutDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (pWnd->GetDlgCtrlID() == IDC_ABTDLG_HOMEPAGE)
	{
		if (m_hLinkCursor)
			::SetCursor(m_hLinkCursor);

		return TRUE;
	}

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CAboutDlg::OnHomepage() 
{
	ShellExecute(::GetDesktopWindow(), _T("open"), _T("http://www.leocad.org"), NULL, NULL, SW_NORMAL); 
}
