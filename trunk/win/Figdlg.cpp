#include "lc_global.h"
#include "LeoCAD.h"
#include "FigDlg.h"
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
  m_pMinifig = (MinifigWizard*)param;
	m_pMinifigWnd = NULL;

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
	ON_CONTROL_RANGE(EN_CHANGE, IDC_MF_HATANGLE, IDC_MF_SHOERANGLE, OnChangeAngle)
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

	HINSTANCE hInst = AfxGetInstanceHandle();
	WNDCLASS wndcls;
LRESULT CALLBACK GLWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define OPENGL_CLASSNAME _T("LeoCADOpenGLClass")
#define MINIFIG_CLASSNAME _T("LeoCADMinifigOpenGLClass")

  // check if our class is registered
	if(!(GetClassInfo (hInst, MINIFIG_CLASSNAME, &wndcls)))
	{
  	if (GetClassInfo (hInst, OPENGL_CLASSNAME, &wndcls))
	  {
      // set our class name
	  	wndcls.lpszClassName = MINIFIG_CLASSNAME;
      wndcls.lpfnWndProc = GLWindowProc;

  		// register class
	  	if (!AfxRegisterClass (&wndcls))
		  	AfxThrowResourceException();
  	}
		else
			AfxThrowResourceException();
  }

	m_pMinifigWnd = new CWnd;
	m_pMinifigWnd->CreateEx (0, MINIFIG_CLASSNAME, "LeoCAD", WS_BORDER | WS_CHILD | WS_VISIBLE, r, this, 0, m_pMinifig);

	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
		((CColorPicker*)GetDlgItem (IDC_MF_HATCOLOR+i))->SetColorIndex (m_pMinifig->m_Colors[i]);

	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		CComboBox* pCombo = (CComboBox*)GetDlgItem(i+IDC_MF_HAT);
		ObjArray<lcMinifigPieceInfo>& Pieces = m_pMinifig->mSettings[i];

	    for (int j = 0; j < Pieces.GetSize(); j++)
			pCombo->AddString(Pieces[j].Description);
	}

	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		CComboBox* pCombo = (CComboBox*)GetDlgItem(i+IDC_MF_HAT);
		pCombo->SetCurSel(m_pMinifig->GetSelectionIndex(i));
	}

	for (int i = IDC_MF_HATSPIN; i <= IDC_MF_SHOERSPIN; i++)
		((CSpinButtonCtrl*)GetDlgItem(i))->SetRange(-360, 360);

  return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMinifigDlg::DestroyWindow() 
{
	if (m_pMinifigWnd)
	{
		m_pMinifigWnd->DestroyWindow();
		delete m_pMinifigWnd;
		m_pMinifigWnd = NULL;
	}
	
	return CDialog::DestroyWindow();
}

LONG CMinifigDlg::OnColorSelEndOK(UINT lParam, LONG wParam)
{
	m_pMinifig->SetColor(wParam-IDC_MF_HATCOLOR, lParam);
	m_pMinifig->Redraw ();

	return TRUE;
}

void CMinifigDlg::OnPieceSelEndOK(UINT nID)
{
	CComboBox* Combo = (CComboBox*)GetDlgItem(nID);
	m_pMinifig->SetSelectionIndex(nID - IDC_MF_HAT, Combo->GetCurSel());
	m_pMinifig->Redraw();
}

void CMinifigDlg::OnChangeAngle(UINT nID) 
{
	int index[] = { LC_MFW_HAT, LC_MFW_HEAD, LC_MFW_LEFT_ARM, LC_MFW_RIGHT_ARM, LC_MFW_LEFT_HAND, LC_MFW_RIGHT_HAND,
	                LC_MFW_LEFT_TOOL, LC_MFW_RIGHT_TOOL, LC_MFW_LEFT_LEG, LC_MFW_RIGHT_LEG, LC_MFW_LEFT_SHOE, LC_MFW_RIGHT_SHOE };

	char tmp[65];
	GetDlgItem(nID)->GetWindowText(tmp, 65);

	if (m_pMinifigWnd)
	{
		m_pMinifig->SetAngle(index[nID-IDC_MF_HATANGLE], (float)strtod(tmp, NULL));
		m_pMinifig->Redraw();
	}
}
