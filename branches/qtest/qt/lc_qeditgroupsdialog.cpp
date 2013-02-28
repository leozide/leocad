#include "lc_global.h"
#include "lc_qeditgroupsdialog.h"
#include "ui_lc_qeditgroupsdialog.h"
#include "lc_application.h"
#include "project.h"
#include "piece.h"
#include "group.h"

lcQEditGroupsDialog::lcQEditGroupsDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQEditGroupsDialog)
{
	ui->setupUi(this);

	options = (lcEditGroupsDialogOptions*)data;

	addChildren(ui->treeWidget->invisibleRootItem(), NULL);
}

lcQEditGroupsDialog::~lcQEditGroupsDialog()
{
	delete ui;
}

void lcQEditGroupsDialog::accept()
{
	QDialog::accept();
}

void lcQEditGroupsDialog::addChildren(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (Group *group = project->m_pGroups; group; group = group->m_pNext)
	{
		if (group->m_pGroup != parentGroup)
			continue;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(parentItem, QStringList(group->m_strName));
		addChildren(groupItem, group);
	}

	for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext)
	{
		if (piece->GetGroup() != parentGroup)
			continue;

		new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
	}
	/*
	int i;
	TV_INSERTSTRUCT tvstruct;
	tvstruct.hParent = hParent;
	tvstruct.hInsertAfter = TVI_SORT;

	for (int GroupIdx = 0; GroupIdx < options->->groupcount; i++)
		if (opts->groupsgroups[i] == pGroup)
		{
			tvstruct.item.lParam = i + 0xFFFF;
			tvstruct.item.iImage = 0;
			tvstruct.item.iSelectedImage = 1;
			tvstruct.item.pszText = opts->groups[i]->m_strName;
			tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

			HTREEITEM hti = InsertItem(&tvstruct);
			AddChildren(hti, opts->groups[i]);
		}

	for (i = 0; i < opts->piececount; i++)
		if (opts->piecesgroups[i] == pGroup)
		{
			tvstruct.item.lParam = i;
			tvstruct.item.iImage = 2;
			tvstruct.item.iSelectedImage = 2;
			tvstruct.item.pszText = (char*)opts->pieces[i]->GetName();
			tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			InsertItem(&tvstruct);
		}
		*/
}

/*

BOOL CEditGroupsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_TreeImages.Create(IDB_PARTICONS, 16, 0, RGB (0,128,128));
	m_Tree.SetImageList(&m_TreeImages, TVSIL_NORMAL);

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
  */
