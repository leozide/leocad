// SelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "SelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LC_SELDLG_CAMERAS	0x01
#define LC_SELDLG_GROUPS	0x02
#define LC_SELDLG_LIGHTS	0x04
#define LC_SELDLG_PIECES	0x08
#define LC_SELDLG_KEY		"Settings"
#define LC_SELDLG_KEY_NAME	"Selection Dialog"

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg dialog


CSelectDlg::CSelectDlg(void* pData, CWnd* pParent /*=NULL*/)
	: CDialog(CSelectDlg::IDD, pParent)
{
	m_pData = (LC_SEL_DATA*)pData;

	//{{AFX_DATA_INIT(CSelectDlg)
	m_bCameras = TRUE;
	m_bGroups = TRUE;
	m_bLights = TRUE;
	m_bPieces = TRUE;
	//}}AFX_DATA_INIT
}


void CSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDlg)
	DDX_Control(pDX, IDC_SELDLG_LIST, m_List);
	DDX_Check(pDX, IDC_SELDLG_CAMERAS, m_bCameras);
	DDX_Check(pDX, IDC_SELDLG_GROUPS, m_bGroups);
	DDX_Check(pDX, IDC_SELDLG_LIGHTS, m_bLights);
	DDX_Check(pDX, IDC_SELDLG_PIECES, m_bPieces);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectDlg)
	ON_BN_CLICKED(IDC_SELDLG_ALL, OnSeldlgAll)
	ON_BN_CLICKED(IDC_SELDLG_NONE, OnSeldlgNone)
	ON_BN_CLICKED(IDC_SELDLG_INVERT, OnSeldlgInvert)
	ON_LBN_SELCHANGE(IDC_SELDLG_LIST, OnSelChange)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SELDLG_CAMERAS, OnSeldlgToggle)
	ON_BN_CLICKED(IDC_SELDLG_GROUPS, OnSeldlgToggle)
	ON_BN_CLICKED(IDC_SELDLG_LIGHTS, OnSeldlgToggle)
	ON_BN_CLICKED(IDC_SELDLG_PIECES, OnSeldlgToggle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDlg message handlers

BOOL CSelectDlg::OnInitDialog() 
{
	UINT u = theApp.GetProfileInt(LC_SELDLG_KEY, LC_SELDLG_KEY_NAME, 
			LC_SELDLG_CAMERAS|LC_SELDLG_GROUPS|LC_SELDLG_LIGHTS|LC_SELDLG_PIECES);

	m_bCameras = (u & LC_SELDLG_CAMERAS) != 0;
	m_bGroups = (u & LC_SELDLG_GROUPS) != 0;
	m_bLights = (u & LC_SELDLG_LIGHTS) != 0;
	m_bPieces = (u & LC_SELDLG_PIECES) != 0;

	CDialog::OnInitDialog();

	UpdateList(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectDlg::OnSeldlgAll() 
{
	for (int i = 0; i < m_List.GetCount(); i++)
	{
		m_List.SetSel(i);
		((LC_SEL_DATA*)m_List.GetItemDataPtr(i))->selected = true;
	}
}

void CSelectDlg::OnSeldlgNone() 
{
	for (int i = 0; i < m_List.GetCount(); i++)
	{
		m_List.SetSel(i, FALSE);
		((LC_SEL_DATA*)m_List.GetItemDataPtr(i))->selected = false;
	}
}

void CSelectDlg::OnSeldlgInvert() 
{
	for (int i = 0; i < m_List.GetCount(); i++)
	{
		m_List.SetSel(i, !m_List.GetSel(i));
		((LC_SEL_DATA*)m_List.GetItemDataPtr(i))->selected = (m_List.GetSel(i) != 0);
	}
}

void CSelectDlg::OnOK() 
{
	UINT u = 0;
	UpdateData(TRUE);
	if (m_bCameras) u |= LC_SELDLG_CAMERAS;
	if (m_bGroups) u |= LC_SELDLG_GROUPS;
	if (m_bLights) u |= LC_SELDLG_LIGHTS;
	if (m_bPieces) u |= LC_SELDLG_PIECES;
	theApp.WriteProfileInt(LC_SELDLG_KEY, LC_SELDLG_KEY_NAME, u);
	
	CDialog::OnOK();
}

void CSelectDlg::OnSeldlgToggle() 
{
	UpdateList(FALSE);
}

void CSelectDlg::UpdateList(BOOL bFirst)
{
	BOOL bPieces = m_bPieces;
	BOOL bCameras = m_bCameras;
	BOOL bLights = m_bLights;
	BOOL bGroups = m_bGroups;
	UpdateData(TRUE);
	int i, idx;

	if ((m_bPieces != bPieces) || bFirst)
	{
		if (m_bPieces)
		{
			for (i = 0; m_pData[i].pointer != NULL; i++)
				if (m_pData[i].type == LC_SELDLG_PIECE)
				{
					idx = m_List.AddString(m_pData[i].name);
					m_List.SetItemDataPtr(idx, &m_pData[i]);
				}
		}
		else
		{
			for (idx = 0; idx < m_List.GetCount(); idx++)
				if (((LC_SEL_DATA*)m_List.GetItemDataPtr(idx))->type == LC_SELDLG_PIECE)
				{
					m_List.DeleteString(idx);
					idx--;
				}
		}
	}

	if ((m_bCameras != bCameras) || bFirst)
	{
		if (m_bCameras)
		{
			for (i = 0; m_pData[i].pointer != NULL; i++)
				if (m_pData[i].type == LC_SELDLG_CAMERA)
				{
					idx = m_List.AddString(m_pData[i].name);
					m_List.SetItemDataPtr(idx, &m_pData[i]);
				}
		}
		else
		{
			for (idx = 0; idx < m_List.GetCount(); idx++)
				if (((LC_SEL_DATA*)m_List.GetItemDataPtr(idx))->type == LC_SELDLG_CAMERA)
				{
					m_List.DeleteString(idx);
					idx--;
				}
		}
	}

	if ((m_bLights != bLights) || bFirst)
	{
		if (m_bLights)
		{
			for (i = 0; m_pData[i].pointer != NULL; i++)
				if (m_pData[i].type == LC_SELDLG_LIGHT)
				{
					idx = m_List.AddString(m_pData[i].name);
					m_List.SetItemDataPtr(idx, &m_pData[i]);
				}
		}
		else
		{
			for (idx = 0; idx < m_List.GetCount(); idx++)
				if (((LC_SEL_DATA*)m_List.GetItemDataPtr(idx))->type == LC_SELDLG_LIGHT)
				{
					m_List.DeleteString(idx);
					idx--;
				}
		}
	}

	if ((m_bGroups != bGroups) || bFirst)
	{
		if (m_bGroups)
		{
			for (i = 0; m_pData[i].pointer != NULL; i++)
				if (m_pData[i].type == LC_SELDLG_GROUP)
				{
					idx = m_List.AddString(m_pData[i].name);
					m_List.SetItemDataPtr(idx, &m_pData[i]);
				}
		}
		else
		{
			for (idx = 0; idx < m_List.GetCount(); idx++)
				if (((LC_SEL_DATA*)m_List.GetItemDataPtr(idx))->type == LC_SELDLG_GROUP)
				{
					m_List.DeleteString(idx);
					idx--;
				}
		}
	}

	for (idx = 0; idx < m_List.GetCount(); idx++)
		m_List.SetSel(idx, ((LC_SEL_DATA*)m_List.GetItemData(idx))->selected);
}

void CSelectDlg::OnSelChange() 
{
	for (int idx = 0; idx < m_List.GetCount(); idx++)
		((LC_SEL_DATA*)m_List.GetItemDataPtr(idx))->selected = (m_List.GetSel(idx) != 0);
}
