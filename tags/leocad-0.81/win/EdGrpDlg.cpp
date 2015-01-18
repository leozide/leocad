#include "lc_global.h"
#include "leocad.h"
#include "EdGrpDlg.h"
#include "globals.h"
#include "project.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditGroupsDlg dialog


CEditGroupsDlg::CEditGroupsDlg(void* param, CWnd* pParent /*=NULL*/)
	: CDialog(CEditGroupsDlg::IDD, pParent)
{
	m_Tree.opts = (LC_GROUPEDITDLG_OPTS*)param;

	//{{AFX_DATA_INIT(CEditGroupsDlg)
	//}}AFX_DATA_INIT
}


void CEditGroupsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditGroupsDlg)
	DDX_Control(pDX, IDC_TREE, m_Tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditGroupsDlg, CDialog)
	//{{AFX_MSG_MAP(CEditGroupsDlg)
	ON_BN_CLICKED(IDC_EDITGRP_NEWGROUP, OnEditgrpNewgroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditGroupsDlg message handlers


BOOL CEditGroupsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_TreeImages.Create(IDB_PARTICONS, 16, 0, RGB (0,128,128));
	m_Tree.SetImageList(&m_TreeImages, TVSIL_NORMAL);

	m_Tree.DeleteAllItems();
	m_Tree.AddChildren(NULL, NULL);

  return TRUE;
}

void CEditGroupsDlg::OnEditgrpNewgroup() 
{
  HTREEITEM hItem, hParent = NULL;
  Group *pGroup, *pParent = NULL;
  TVITEM item;
  
  hItem = m_Tree.GetSelectedItem ();

  if (hItem != NULL)
  {
    item.hItem = hItem;
    item.mask = TVIF_HANDLE | TVIF_PARAM;

    if (m_Tree.GetItem (&item))
    {
      if (item.lParam < 0xFFFF)
        hParent = m_Tree.GetParentItem (hItem);
      else
        hParent = hItem;
    }
  }

  if (hParent)
  {
    item.hItem = hParent;
    item.mask = TVIF_HANDLE | TVIF_PARAM;

    if (m_Tree.GetItem (&item))
      pParent = m_Tree.opts->groups[item.lParam - 0xFFFF];
  }

  pGroup = lcGetActiveProject()->AddGroup (NULL, pParent, 0, 0, 0);

  m_Tree.opts->groupcount++;
  m_Tree.opts->groups = (Group**)realloc(m_Tree.opts->groups, m_Tree.opts->groupcount*sizeof(Group*));
  m_Tree.opts->groupsgroups = (Group**)realloc(m_Tree.opts->groupsgroups, m_Tree.opts->groupcount*sizeof(Group*));

  m_Tree.opts->groups[m_Tree.opts->groupcount-1] = pGroup;
  m_Tree.opts->groupsgroups[m_Tree.opts->groupcount-1] = pParent;

  m_Tree.DeleteAllItems();
	m_Tree.AddChildren(NULL, NULL);
}
