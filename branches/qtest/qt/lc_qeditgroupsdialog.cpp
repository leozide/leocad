#include "lc_global.h"
#include "lc_qeditgroupsdialog.h"
#include "ui_lc_qeditgroupsdialog.h"
#include "lc_application.h"
#include "project.h"
#include "piece.h"
#include "group.h"
#include "basewnd.h"

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
	updateParents(ui->treeWidget->invisibleRootItem(), NULL);

	QDialog::accept();
}

void lcQEditGroupsDialog::updateParents(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);

		Piece *itemPiece = (Piece*)childItem->data(0, PieceRole).value<void*>();

		if (itemPiece)
		{
			int pieceIndex = 0;
			for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext, pieceIndex++)
			{
				if (itemPiece == piece)
				{
					options->PieceParents.SetAt(pieceIndex, parentGroup);
					break;
				}
			}
		}
		else
		{
			Group *itemGroup = (Group*)childItem->data(0, GroupRole).value<void*>();

			int groupIndex = 0;
			for (Group *group = project->m_pGroups; group; group = group->m_pNext, groupIndex++)
			{
				if (itemGroup == group)
				{
					options->GroupParents.SetAt(groupIndex, parentGroup);
					break;
				}
			}

			updateParents(childItem, itemGroup);
		}
	}
}

void lcQEditGroupsDialog::addChildren(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (Group *group = project->m_pGroups; group; group = group->m_pNext)
	{
		if (group->m_pGroup != parentGroup)
			continue;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(parentItem, QStringList(group->m_strName));
		groupItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
		groupItem->setData(0, GroupRole, qVariantFromValue((void*)group));

		addChildren(groupItem, group);
	}

	for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext)
	{
		if (piece->GetGroup() != parentGroup)
			continue;

		QTreeWidgetItem *pieceItem = new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
		pieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		pieceItem->setData(0, PieceRole, qVariantFromValue((void*)piece));
	}
}

/*

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
