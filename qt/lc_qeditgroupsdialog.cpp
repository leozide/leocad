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

	QPushButton *newGroup = ui->buttonBox->addButton(tr("New Group"), QDialogButtonBox::ActionRole);
	connect(newGroup, SIGNAL(clicked()), this, SLOT(on_newGroup_clicked()));

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

void lcQEditGroupsDialog::on_newGroup_clicked()
{
	QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();

	if (currentItem && currentItem->data(0, PieceRole).value<uintptr_t>())
		currentItem = currentItem->parent();

	if (!currentItem)
		currentItem = ui->treeWidget->invisibleRootItem();

	Group *parentGroup = (Group*)currentItem->data(0, GroupRole).value<uintptr_t>();
	Group *newGroup = lcGetActiveProject()->AddGroup(NULL, parentGroup, 0, 0, 0);
	options->GroupParents.Add(NULL);

	QTreeWidgetItem *groupItem = new QTreeWidgetItem(currentItem, QStringList(newGroup->m_strName));
	groupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
	groupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)newGroup));
}

void lcQEditGroupsDialog::updateParents(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);

		Piece *itemPiece = (Piece*)childItem->data(0, PieceRole).value<uintptr_t>();

		if (itemPiece)
		{
			int pieceIndex = 0;
			for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext, pieceIndex++)
			{
				if (itemPiece == piece)
				{
					options->PieceParents[pieceIndex] = parentGroup;
					break;
				}
			}
		}
		else
		{
			Group *itemGroup = (Group*)childItem->data(0, GroupRole).value<uintptr_t>();

			int groupIndex = 0;
			for (Group *group = project->m_pGroups; group; group = group->m_pNext, groupIndex++)
			{
				if (itemGroup == group)
				{
					options->GroupParents[groupIndex] = parentGroup;
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
		groupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
		groupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)group));

		addChildren(groupItem, group);
	}

	for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext)
	{
		if (piece->GetGroup() != parentGroup)
			continue;

		QTreeWidgetItem *pieceItem = new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
		pieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		pieceItem->setData(0, PieceRole, qVariantFromValue<uintptr_t>((uintptr_t)piece));
	}
}
