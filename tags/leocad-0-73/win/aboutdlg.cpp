// AboutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "AboutDlg.h"

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

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

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
	info += glGetString(GL_VERSION);
	info += " (";
	info += glGetString(GL_RENDERER);
	info += " - ";
	info += glGetString(GL_VENDOR);
	info += ")";

	SetDlgItemText(IDC_ABTDLG_INFO, info);
	return TRUE;
}
