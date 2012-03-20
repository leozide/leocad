#include "lc_global.h"
#include "leocad.h"
#include "StepDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStepDlg dialog


CStepDlg::CStepDlg(CStepDlg** pointer, CWnd* pParent)
	: CDialog(CStepDlg::IDD, pParent)
{
	m_pView = pParent;
	m_pPointer = pointer;

	//{{AFX_DATA_INIT(CStepDlg)
	m_nStep = 0;
	//}}AFX_DATA_INIT
}


void CStepDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStepDlg)
	DDX_Control(pDX, IDC_STEP_SLIDER, m_Slider);
	DDX_Text(pDX, IDC_STEP_EDIT, m_nStep);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStepDlg, CDialog)
	//{{AFX_MSG_MAP(CStepDlg)
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_STEP_EDIT, OnEditChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
 
/////////////////////////////////////////////////////////////////////////////
// CStepDlg message handlers

void CStepDlg::UpdateRange(int nTime, int nTotal)
{
	m_Slider.SetRange (1, nTotal);
	m_Slider.SetPos (nTime);
	((CSpinButtonCtrl*)GetDlgItem(IDC_STEP_SPIN))->SetRange(1, nTotal);
	m_nStep = nTime;
	UpdateData(FALSE);
}

void CStepDlg::OnCancel() 
{
	DestroyWindow();
}

void CStepDlg::OnOK() 
{
	UpdateData();
	m_pView->PostMessage(WM_LC_SET_STEP, m_nStep);
	DestroyWindow();
}

void CStepDlg::OnApply() 
{
	UpdateData();
	m_pView->PostMessage(WM_LC_SET_STEP, m_nStep);
}

void CStepDlg::PostNcDestroy() 
{
	*m_pPointer = NULL;
	delete this;
}

void CStepDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	m_nStep = m_Slider.GetPos();
	UpdateData(FALSE);
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CStepDlg::OnEditChange()
{
	if (m_Slider.m_hWnd)
	{
		UpdateData();
		m_Slider.SetPos(m_nStep);
	}
}

void CStepDlg::OnDestroy() 
{
	RECT rc;
	GetWindowRect(&rc);
	char buf[30];
	sprintf(buf, "%d, %d", rc.top, rc.left);
	AfxGetApp()->WriteProfileString ("Settings", "Step Dialog", buf);

	CDialog::OnDestroy();
}
/*
void CStepDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	CRect rc = lpncsp->rgrc[0];
	rc.DeflateRect(2, 2, 2, 2);
	rc.top += 10;
//	rc.bottom -= 10;
	lpncsp->rgrc[0] = rc;
}

void CStepDlg::OnNcPaint() 
{
	CWindowDC dc(this);
	CRect rectClient;
	GetClientRect(rectClient);
	CRect rectWindow;
	GetWindowRect(rectWindow);
	ScreenToClient(rectWindow);

	rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top+2);
	rectWindow.bottom = rectWindow.top + 10;
	::FillRect(dc.m_hDC, rectWindow, (HBRUSH)(COLOR_ACTIVECAPTION+1));

	CRect rectBorder;
	GetWindowRect(rectBorder);
	rectBorder.OffsetRect(-rectBorder.left, -rectBorder.top);

	dc.Draw3dRect(rectBorder, ::GetSysColor(COLOR_BTNFACE), ::GetSysColor(COLOR_3DDKSHADOW));
	rectBorder.DeflateRect(1, 1);
	dc.Draw3dRect(rectBorder, ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_BTNSHADOW));

	rectClient.OffsetRect(-rectWindow.left, -rectWindow.top);
	dc.ExcludeClipRect(rectClient);

	// draw borders in non-client area
	rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);

	// erase parts not drawn
	dc.IntersectClipRect(rectWindow);

	// erase NC background the hard way
//	HBRUSH hbr = (HBRUSH)GetClassLong(m_hWnd, GCL_HBRBACKGROUND);
//	::FillRect(dc.m_hDC, rectWindow, hbr);


    ReleaseDC(&dc);
}
*/
