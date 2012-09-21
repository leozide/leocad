#include "lc_global.h"
#include "leocad.h"
#include "ProgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

CProgressDlg::CProgressDlg(LPCTSTR pszTitle)
{
	//{{AFX_DATA_INIT(CProgressDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nMinValue = 0;
	m_nMaxValue = 100;
	m_nPrevPos = 0;
	m_nPrevPercent = 0;
    m_strTitle = pszTitle;
	m_bCancel = FALSE;
	m_bParentDisabled = FALSE;
}

CProgressDlg::~CProgressDlg()
{
	if(m_hWnd != NULL)
		DestroyWindow();
}

BOOL CProgressDlg::Create(CWnd* pParent)
{
	m_pParentWnd = CWnd::GetSafeOwner(pParent);

	if((m_pParentWnd != NULL) && m_pParentWnd->IsWindowEnabled())
	{
		m_pParentWnd->EnableWindow(FALSE);
		m_bParentDisabled = TRUE;
	}

	if(!CDialog::Create(CProgressDlg::IDD, pParent))
	{
		ReEnableParent();
		return FALSE;
	}
	
	return TRUE;
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, IDC_PRGDLG_PROGRESS, m_Progress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

void CProgressDlg::OnCancel() 
{
	m_bCancel = TRUE;
}

BOOL CProgressDlg::CheckCancelButton()
{
	// Process all pending messages
	PumpMessages();

	BOOL bResult = m_bCancel;
	m_bCancel = FALSE;

	return bResult;
}

void CProgressDlg::PumpMessages()
{
	// Must call Create() before using the dialog
	ASSERT(m_hWnd!=NULL);
	
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if(!IsDialogMessage(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);  
		}
	}
}

int CProgressDlg::StepIt()
{
	PumpMessages();
	return SetPos(m_nPrevPos + 1);
}

int CProgressDlg::SetPos(int nPos)
{
	CString strTitle;
	int nPercentage;

	m_nPrevPos = nPos;

	if (m_nMaxValue > m_nMinValue)
		nPercentage = (nPos*100)/(m_nMaxValue - m_nMinValue);
	else
		nPercentage = 0;

	if ((nPercentage != m_nPrevPercent) || (nPos == 1))
	{
		m_nPrevPercent = nPercentage;
		strTitle.Format(_T("%s [%d%%]"), m_strTitle, nPercentage);
		SetWindowText(strTitle);
	}

	return m_Progress.SetPos(nPos);
}

void CProgressDlg::SetRange(int nLower, int nUpper)
{
    ASSERT(0 <= nLower && nLower <= 65535);
    ASSERT(0 <= nUpper && nUpper <= 65535);

    m_Progress.SetRange(nLower, nUpper);
    m_nMaxValue = nUpper;
    m_nMinValue = nLower;
}

BOOL CProgressDlg::DestroyWindow() 
{
	ReEnableParent();
	return CDialog::DestroyWindow();
}

void CProgressDlg::ReEnableParent()
{
	if(m_bParentDisabled && (m_pParentWnd != NULL))
		m_pParentWnd->EnableWindow(TRUE);
	m_bParentDisabled = FALSE;
}

BOOL CProgressDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_Progress.ModifyStyle(0, PBS_SMOOTH);
	m_Progress.SetRange(m_nMinValue, m_nMaxValue);
	m_Progress.SetStep(1);
	SetPos(m_nMinValue);
	
	return TRUE;
}

void CProgressDlg::SetStatus(LPCTSTR lpszMessage)
{
	ASSERT(m_hWnd);
	CWnd *pWndStatus = GetDlgItem(IDC_PRGDLG_TEXT);

	ASSERT(pWndStatus != NULL);
	pWndStatus->SetWindowText(lpszMessage);
}

void CProgressDlg::SetSubStatus(LPCTSTR lpszMessage)
{
	ASSERT(m_hWnd);
	CWnd *pWndStatus = GetDlgItem(IDC_PRGDLG_TEXT2);

	ASSERT(pWndStatus != NULL);
	pWndStatus->SetWindowText(lpszMessage);
}
