// FigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "FigDlg.h"
#include "MFWnd.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"

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
	m_pMFWnd->m_pFig = (LC_MINIFIGDLG_OPTS*)m_pParam;
	m_pMFWnd->Create (NULL, NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, r, this, 501);
	m_pMFWnd->InitGL();

	for (int i = 0; i < 15; i++)
		((CColorPicker*)GetDlgItem (IDC_MF_HATCOLOR+i))->SetColorIndex (m_pMFWnd->m_pFig->colors[i]);

	for (i = 0; i < MFW_PIECES; i++)
	{
		PieceInfo* pInfo = project->FindPieceInfo(mfwpieceinfo[i].name);
		if (pInfo != NULL)
		{
			UINT id = 0;
			switch (mfwpieceinfo[i].type)
			{
				case MF_HAT: id = IDC_MF_HAT; break;
				case MF_HEAD: id = IDC_MF_HEAD; break;
				case MF_TORSO: id = IDC_MF_TORSO; break;
				case MF_NECK: id = IDC_MF_NECK; break;
				case MF_ARML: id = IDC_MF_ARML; break;
				case MF_ARMR: id = IDC_MF_ARMR; break;
				case MF_HAND: id = IDC_MF_HANDL; break;
				case MF_TOOL: id = IDC_MF_TOOLL; break;
				case MF_HIPS: id = IDC_MF_HIPS; break;
				case MF_LEGL: id = IDC_MF_LEGL; break;
				case MF_LEGR: id = IDC_MF_LEGR; break;
				case MF_SHOE: id = IDC_MF_SHOEL; break;
			}

			CComboBox* pCombo = (CComboBox*)GetDlgItem(id);
			int pos;
			if (i != 29)
			{
				pos = pCombo->AddString(mfwpieceinfo[i].description);
				pCombo->SetItemDataPtr(pos, pInfo);
			}

			if (id == IDC_MF_HANDL || id == IDC_MF_TOOLL || id == IDC_MF_SHOEL)
			{
				pCombo = (CComboBox*)GetDlgItem(id+1);
				pos = pCombo->AddString(mfwpieceinfo[i].description);
				pCombo->SetItemDataPtr(pos, pInfo);
			}
			if (i == 6) i++;
		}
	}

	UINT nid[6] = { IDC_MF_HAT, IDC_MF_NECK, IDC_MF_TOOLL, IDC_MF_TOOLR, IDC_MF_SHOEL, IDC_MF_SHOER };
	for (i = 0; i < 6; i++)
	{
		CComboBox* pCombo = (CComboBox*)GetDlgItem(nid[i]);
		pCombo->InsertString(0, "None");
		pCombo->SetItemData (0, 0);
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
	m_pMFWnd->m_pFig->colors[wParam-IDC_MF_HATCOLOR] = lParam;
	m_pMFWnd->PostMessage(WM_PAINT);

	return TRUE;
}

void CMinifigDlg::OnPieceSelEndOK(UINT nID)
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(nID);
	PieceInfo* pInfo = (PieceInfo*)pCombo->GetItemDataPtr(pCombo->GetCurSel());

	if (m_pMFWnd->m_pFig->info[nID-IDC_MF_HAT])
		m_pMFWnd->m_pFig->info[nID-IDC_MF_HAT]->DeRef();
	m_pMFWnd->m_pFig->info[nID-IDC_MF_HAT] = pInfo;
	if (pInfo)
		pInfo->AddRef();

	if (nID == IDC_MF_NECK)
	{
		if (m_pMFWnd->m_pFig->info[3] != NULL)
		{
			m_pMFWnd->m_pFig->pos[0][2] = 3.92f;
			m_pMFWnd->m_pFig->pos[1][2] = 3.92f;

			if (strcmp (pInfo->m_strName,"4498") == 0)
				m_pMFWnd->m_pFig->rot[3][2] = 180.0f;
		}
		else
		{
			m_pMFWnd->m_pFig->pos[0][2] = 3.84f;
			m_pMFWnd->m_pFig->pos[1][2] = 3.84f;
		}
	}

	if (nID == IDC_MF_SHOEL)
	{
		if (pCombo->GetCurSel() == 1)
			m_pMFWnd->m_pFig->pos[13][1] = 0;
		else
			m_pMFWnd->m_pFig->pos[13][1] = -0.12f;
	}

	if (nID == IDC_MF_SHOER)
	{
		if (pCombo->GetCurSel() == 1)
			m_pMFWnd->m_pFig->pos[14][1] = 0;
		else
			m_pMFWnd->m_pFig->pos[14][1] = -0.12f;
	}

	if ((nID == IDC_MF_TOOLL) || (nID == IDC_MF_TOOLR))
	if (pCombo->GetItemData(pCombo->GetCurSel()) != 0)
	{
		float rx = 45, ry = 0, rz = 0, x = 0.92f, y = -0.62f, z = 1.76f;

		if (strcmp (pInfo->m_strName,"4529") == 0)
		{ rx = -45; y = -1.14f; z = 2.36f; }
		if (strcmp (pInfo->m_strName,"3899") == 0)
		{ y = -1.64f; z = 1.38f; }
		if (strcmp (pInfo->m_strName,"4528") == 0)
		{ rx = -45; y = -1.26f; z = 2.36f; }
		if (strcmp (pInfo->m_strName,"4479") == 0)
		{ rz = 90; y = -1.22f; z = 2.44f; }
		if (strcmp (pInfo->m_strName,"3962") == 0)
		{ rz = 90; y = -0.7f; z = 1.62f; }
		if (strcmp (pInfo->m_strName,"4360") == 0)
		{ rz = -90; y = -1.22f; z = 2.44f; }
		if (strncmp (pInfo->m_strName,"6246",4) == 0)
		{ y = -1.82f; z = 2.72f; rz = 90; }
		if (strcmp (pInfo->m_strName,"4349") == 0)
		{ y = -1.16f; z = 2.0f; }
		if (strcmp (pInfo->m_strName,"4479") == 0)
		{ y = -1.42f; z = 2.26f; }
		if (strcmp (pInfo->m_strName,"3959") == 0)
		{ y = -1.0f; z = 1.88f; }
		if (strcmp (pInfo->m_strName,"4522") == 0)
		{ y = -1.64f; z = 2.48f; }
		if (strcmp (pInfo->m_strName,"194") == 0)
		{ rz = 180; y = -1.04f; z = 1.94f; }
		if (strcmp (pInfo->m_strName,"4006") == 0)
		{ rz = 180; y = -1.24f; z = 2.18f; }
		if (strcmp (pInfo->m_strName,"6246C") == 0)
		{ rx = 35; rz = 0; y = -2.36f; z = 1.08f; }
		if (strcmp (pInfo->m_strName,"4497") == 0)
		{ y = -2.16f; z = 3.08f; rz = 90; }
		if (strcmp (pInfo->m_strName,"30092") == 0)
		{ x = 0; rz = 180; }
		if (strcmp (pInfo->m_strName,"37") == 0)
		{ z = 1.52f; y = -0.64f; }
		if (strcmp (pInfo->m_strName,"38") == 0)
		{ z = 1.24f; y = -0.34f; }
		if (strcmp (pInfo->m_strName,"3841") == 0)
		{ z = 2.24f; y = -1.34f; rz = 180; }
		if (strcmp (pInfo->m_strName,"4499") == 0)
		{ rz = 10; z = 1.52f; }
		if (strcmp (pInfo->m_strName,"3852") == 0)
		{ rz = -90; x = 0.90f; y = -0.8f; z = 1.84f; }
		if (strcmp (pInfo->m_strName,"30152") == 0)
		{ z = 3.06f; y = -2.16f; }

		if (nID == IDC_MF_TOOLR)
			x = -x;

		m_pMFWnd->m_pFig->pos[nID-IDC_MF_HAT][0] = x;
		m_pMFWnd->m_pFig->pos[nID-IDC_MF_HAT][1] = y;
		m_pMFWnd->m_pFig->pos[nID-IDC_MF_HAT][2] = z;
		m_pMFWnd->m_pFig->rot[nID-IDC_MF_HAT][0] = rx;
		m_pMFWnd->m_pFig->rot[nID-IDC_MF_HAT][1] = ry;
		m_pMFWnd->m_pFig->rot[nID-IDC_MF_HAT][2] = rz;
	}

	m_pMFWnd->PostMessage(WM_PAINT);
}
