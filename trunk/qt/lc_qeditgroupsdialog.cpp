#include "lc_global.h"
#include "lc_qeditgroupsdialog.h"
#include "ui_lc_qeditgroupsdialog.h"
#include "lc_application.h"
#include "project.h"
#include "piece.h"
#include "group.h"
#include "lc_basewindow.h"

lcQEditGroupsDialog::lcQEditGroupsDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQEditGroupsDialog)
{
	m_lastItemClicked = NULL;

	ui->setupUi(this);
	connect(ui->treeWidget,SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(onItemClicked(QTreeWidgetItem *,int)));
	connect(ui->treeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem *,int)));

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
	Group *newGroup = lcGetActiveProject()->AddGroup(parentGroup);
	options->GroupParents.Add(NULL);

	QTreeWidgetItem *groupItem = new QTreeWidgetItem(currentItem, QStringList(newGroup->m_strName));
	groupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
	groupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)newGroup));
}

void lcQEditGroupsDialog::onItemClicked(QTreeWidgetItem *item, int column)
{
	if (item->flags() & Qt::ItemIsEditable)
	{
		m_clickTimer.stop();

		if (m_lastItemClicked != item)
		{
			m_lastItemClicked = item;
			m_editableDoubleClicked = false;
		}
		else
		{
			m_clickTimer.start(QApplication::doubleClickInterval() + 50, this);
		}
	}
}

void lcQEditGroupsDialog::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
	if (item->flags() & Qt::ItemIsEditable)
	{
		m_editableDoubleClicked = true;
	}
}

void lcQEditGroupsDialog::timerEvent(QTimerEvent *event)
{
	m_clickTimer.stop();
	if (!m_editableDoubleClicked)
	{
		ui->treeWidget->editItem(m_lastItemClicked);
	}

	m_editableDoubleClicked = false;
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
			int pieceIndex = project->mPieces.FindIndex(itemPiece);
			if (pieceIndex != -1)
				options->PieceParents[pieceIndex] = parentGroup;
		}
		else
		{
			Group *itemGroup = (Group*)childItem->data(0, GroupRole).value<uintptr_t>();

			strncpy(itemGroup->m_strName, childItem->text(0).toLocal8Bit(), sizeof(itemGroup->m_strName));
			itemGroup->m_strName[sizeof(itemGroup->m_strName) - 1] = 0;

			for (int groupIdx = 0; groupIdx < project->mGroups.GetSize(); groupIdx++)
			{
				lcGroup *group = project->mGroups[groupIdx];

				if (itemGroup == group)
				{
					options->GroupParents[groupIdx] = parentGroup;
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

	for (int groupIdx = 0; groupIdx < project->mGroups.GetSize(); groupIdx++)
	{
		lcGroup *group = project->mGroups[groupIdx];

		if (group->mGroup != parentGroup)
			continue;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(parentItem, QStringList(group->m_strName));
		groupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
		groupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)group));

		addChildren(groupItem, group);
	}

	for (int pieceIndex = 0; pieceIndex < project->mPieces.GetSize(); pieceIndex++)
	{
		Piece *piece = project->mPieces[pieceIndex];

		if (piece->GetGroup() != parentGroup)
			continue;

		QTreeWidgetItem *pieceItem = new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
		pieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		pieceItem->setData(0, PieceRole, qVariantFromValue<uintptr_t>((uintptr_t)piece));
	}
}
