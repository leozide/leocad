// FigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "FigDlg.h"
#include "MFWnd.h"
#include "minifig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMinifigDlg dialog

CMinifigDlg::CMinifigDlg(void* param, CWnd* pParent /*=NULL*/)
	: CDialog(CMinifigDlg::IDD, pParent)
{
	m_pParam = param;

	//{{AFX_DATA_INIT(CMinifigDlg)
	//}}AFX_DATA_INIT
}


void CMinifigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMinifigDlg)
	DDX_Control(pDX, IDC_MF_HANDLCOLOR, m_clrHandLeft);
	DDX_Control(pDX, IDC_MF_TORSOCOLOR, m_clrTorso);
	DDX_Control(pDX, IDC_MF_TOOLLCOLOR, m_clrToolLeft);
	DDX_Control(pDX, IDC_MF_SHOELCOLOR, m_clrShoeLeft);
	DDX_Control(pDX, IDC_MF_LEGLCOLOR, m_clrLegLeft);
	DDX_Control(pDX, IDC_MF_HEADCOLOR, m_clrHead);
	DDX_Control(pDX, IDC_MF_ARMLCOLOR, m_clrArmLeft);
	DDX_Control(pDX, IDC_MF_TOOLRCOLOR, m_clrToolRight);
	DDX_Control(pDX, IDC_MF_SHOERCOLOR, m_clrShoeRight);
	DDX_Control(pDX, IDC_MF_LEGRCOLOR, m_clrLegRight);
	DDX_Control(pDX, IDC_MF_HIPSCOLOR, m_clrHips);
	DDX_Control(pDX, IDC_MF_HANDRCOLOR, m_clrHandRight);
	DDX_Control(pDX, IDC_MF_ARMRCOLOR, m_clrArmRight);
	DDX_Control(pDX, IDC_MF_NECKCOLOR, m_clrNeck);
	DDX_Control(pDX, IDC_MF_HATCOLOR, m_clrHat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMinifigDlg, CDialog)
	//{{AFX_MSG_MAP(CMinifigDlg)
	//}}AFX_MSG_MAP
	ON_MESSAGE(CPN_SELENDOK, OnColorSelEndOK)
	ON_CONTROL_RANGE(CBN_SELENDOK, IDC_MF_HAT, IDC_MF_SHOER, OnPieceSelEndOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMinifigDlg message handlers

BOOL CMinifigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ModifyStyle (0, WS_CLIPCHILDREN, 0);

	RECT r;
	//RECT r = { 200, 15, 400, 320 };
	::GetWindowRect (::GetDlgItem(m_hWnd, IDC_PREVIEWSTATIC), &r);
	ScreenToClient (&r);

	m_pMFWnd = new CMinifigWnd;
	m_pMFWnd->m_pFig = (MinifigWizard*)m_pParam;
	m_pMFWnd->Create (NULL, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, r, this, 501);
	m_pMFWnd->InitGL();

	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
		((CColorPicker*)GetDlgItem (IDC_MF_HATCOLOR+i))->SetColorIndex (m_pMFWnd->m_pFig->m_Colors[i]);

	for (i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		CComboBox* pCombo = (CComboBox*)GetDlgItem(i+IDC_MF_HAT);
    char **names;
    int j, count;

    m_pMFWnd->m_pFig->GetDescriptions (i, &names, &count);

    for (j = 0; j < count; j++)
			pCombo->AddString (names[j]);
    free (names);

    if (i == 6) i++;
	}

	for (i = IDC_MF_NECK; i <= IDC_MF_SHOER; i++)
		((CComboBox*)GetDlgItem(i))->SetCurSel(0);
	((CComboBox*)GetDlgItem(IDC_MF_HAT))->SetCurSel(6);
	((CComboBox*)GetDlgItem(IDC_MF_HEAD))->SetCurSel(4);
	((CComboBox*)GetDlgItem(IDC_MF_TORSO))->SetCurSel(18);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMinifigDlg::DestroyWindow() 
{
	m_pMFWnd->DestroyWindow();
	delete m_pMFWnd;
	
	return CDialog::DestroyWindow();
}

LONG CMinifigDlg::OnColorSelEndOK(UINT lParam, LONG wParam)
{
	m_pMFWnd->m_pFig->ChangeColor (wParam-IDC_MF_HATCOLOR, lParam);
	m_pMFWnd->PostMessage(WM_PAINT);

	return TRUE;
}

void CMinifigDlg::OnPieceSelEndOK(UINT nID)
{
  char tmp[65];
  GetDlgItem(nID)->GetWindowText (tmp, 65);

	m_pMFWnd->m_pFig->ChangePiece (nID-IDC_MF_HAT, tmp);
	m_pMFWnd->PostMessage(WM_PAINT);
}
