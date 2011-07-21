// modellistdlg.cpp : implementation file
//

#include "lc_global.h"
#include "leocad.h"
#include "modellistdlg.h"
#include "lc_application.h"
#include "lc_model.h"
#include "project.h"
#include "system.h"

// CModelListDlg dialog

IMPLEMENT_DYNAMIC(CModelListDlg, CDialog)

CModelListDlg::CModelListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModelListDlg::IDD, pParent)
{

}

CModelListDlg::~CModelListDlg()
{
}

void CModelListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_List);
}


BEGIN_MESSAGE_MAP(CModelListDlg, CDialog)
	ON_BN_CLICKED(IDC_MODEL_NEW, &CModelListDlg::OnBnClickedNew)
	ON_BN_CLICKED(IDC_MODEL_DELETE, &CModelListDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_MODEL_UP, &CModelListDlg::OnBnClickedUp)
	ON_BN_CLICKED(IDC_MODEL_DOWN, &CModelListDlg::OnBnClickedDown)
	ON_BN_CLICKED(IDC_MODEL_ACTIVATE, &CModelListDlg::OnBnClickedActivate)
END_MESSAGE_MAP()


// CModelListDlg message handlers

BOOL CModelListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Project* project = lcGetActiveProject();

	for (int ModelIndex = 0; ModelIndex < project->m_ModelList.GetSize(); ModelIndex++)
		m_List.AddString(project->m_ModelList[ModelIndex]->m_Name);

	return TRUE;
}

void CModelListDlg::OnBnClickedNew()
{
	Project* project = lcGetActiveProject();

	project->HandleCommand(LC_MODEL_NEW, 0);

	m_List.ResetContent();
	for (int ModelIndex = 0; ModelIndex < project->m_ModelList.GetSize(); ModelIndex++)
		m_List.AddString(project->m_ModelList[ModelIndex]->m_Name);
}

void CModelListDlg::OnBnClickedDelete()
{
	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	Project* project = lcGetActiveProject();

	project->HandleCommand(LC_MODEL_DELETE, Sel);

	m_List.ResetContent();
	for (int ModelIndex = 0; ModelIndex < project->m_ModelList.GetSize(); ModelIndex++)
		m_List.AddString(project->m_ModelList[ModelIndex]->m_Name);
}

void CModelListDlg::OnBnClickedUp()
{
	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR || Sel == 0)
		return;

	Project* project = lcGetActiveProject();

	lcModel* Model = project->m_ModelList[Sel-1];
	project->m_ModelList.SetAt(Sel-1, project->m_ModelList[Sel]);
	project->m_ModelList.SetAt(Sel, Model);
	SystemUpdateModelMenu(project->m_ModelList, project->m_ActiveModel);

	m_List.ResetContent();
	for (int ModelIndex = 0; ModelIndex < project->m_ModelList.GetSize(); ModelIndex++)
		m_List.AddString(project->m_ModelList[ModelIndex]->m_Name);
	m_List.SetCurSel(Sel-1);
}

void CModelListDlg::OnBnClickedDown()
{
	int Sel = m_List.GetCurSel();

	Project* project = lcGetActiveProject();

	if (Sel == LB_ERR || Sel == project->m_ModelList.GetSize()-1)
		return;

	lcModel* Model = project->m_ModelList[Sel+1];
	project->m_ModelList.SetAt(Sel+1, project->m_ModelList[Sel]);
	project->m_ModelList.SetAt(Sel, Model);
	SystemUpdateModelMenu(project->m_ModelList, project->m_ActiveModel);

	m_List.ResetContent();
	for (int ModelIndex = 0; ModelIndex < project->m_ModelList.GetSize(); ModelIndex++)
		m_List.AddString(project->m_ModelList[ModelIndex]->m_Name);
	m_List.SetCurSel(Sel+1);
}

void CModelListDlg::OnBnClickedActivate()
{
	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	lcGetActiveProject()->HandleCommand(LC_MODEL_SET_ACTIVE, Sel);
}
