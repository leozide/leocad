#include "lc_global.h"
#include "lc_qeditgroupsdialog.h"
#include "ui_lc_qeditgroupsdialog.h"
#include "lc_application.h"
#include "project.h"
#include "lc_model.h"
#include "piece.h"
#include "group.h"
#include "lc_basewindow.h"

lcQEditGroupsDialog::lcQEditGroupsDialog(QWidget* Parent, void* Data)
	: QDialog(Parent), ui(new Ui::lcQEditGroupsDialog)
{
	mLastItemClicked = NULL;

	ui->setupUi(this);
	connect(ui->treeWidget,SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(onItemClicked(QTreeWidgetItem *,int)));
	connect(ui->treeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem *,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem *,int)));

	QPushButton *newGroup = ui->buttonBox->addButton(tr("New Group"), QDialogButtonBox::ActionRole);
	connect(newGroup, SIGNAL(clicked()), this, SLOT(on_newGroup_clicked()));

	mOptions = (lcEditGroupsDialogOptions*)Data;

	AddChildren(ui->treeWidget->invisibleRootItem(), NULL);
	ui->treeWidget->expandAll();
}

lcQEditGroupsDialog::~lcQEditGroupsDialog()
{
	delete ui;
}

void lcQEditGroupsDialog::accept()
{
	UpdateParents(ui->treeWidget->invisibleRootItem(), NULL);

	QDialog::accept();
}

void lcQEditGroupsDialog::reject()
{
	for (int GroupIdx = 0; GroupIdx < mOptions->NewGroups.size(); GroupIdx++)
		lcGetActiveModel()->RemoveGroup(mOptions->NewGroups[GroupIdx]);

	QDialog::reject();
}

void lcQEditGroupsDialog::on_newGroup_clicked()
{
	QTreeWidgetItem* CurrentItem = ui->treeWidget->currentItem();

	if (CurrentItem && CurrentItem->data(0, PieceRole).value<uintptr_t>())
		CurrentItem = CurrentItem->parent();

	if (!CurrentItem)
		CurrentItem = ui->treeWidget->invisibleRootItem();

	lcGroup* ParentGroup = (lcGroup*)CurrentItem->data(0, GroupRole).value<uintptr_t>();

	lcGroup* NewGroup = lcGetActiveModel()->AddGroup(tr("Group #"), ParentGroup);
	mOptions->GroupParents[NewGroup] = ParentGroup;
	mOptions->NewGroups.append(NewGroup);

	QTreeWidgetItem* GroupItem = new QTreeWidgetItem(CurrentItem, QStringList(NewGroup->mName));
	GroupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
	GroupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)NewGroup));
}

void lcQEditGroupsDialog::onItemClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);

	if (item->flags() & Qt::ItemIsEditable)
	{
		mClickTimer.stop();

		if (mLastItemClicked != item)
		{
			mLastItemClicked = item;
			mEditableDoubleClicked = false;
		}
		else
		{
			mClickTimer.start(QApplication::doubleClickInterval() + 50, this);
		}
	}
}

void lcQEditGroupsDialog::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);

	if (item->flags() & Qt::ItemIsEditable)
	{
		mEditableDoubleClicked = true;
	}
}

void lcQEditGroupsDialog::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event);

	mClickTimer.stop();
	if (!mEditableDoubleClicked)
	{
		ui->treeWidget->editItem(mLastItemClicked);
	}

	mEditableDoubleClicked = false;
}

void lcQEditGroupsDialog::UpdateParents(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup)
{
	for (int ChildIdx = 0; ChildIdx < ParentItem->childCount(); ChildIdx++)
	{
		QTreeWidgetItem* ChildItem = ParentItem->child(ChildIdx);

		lcPiece* Piece = (lcPiece*)ChildItem->data(0, PieceRole).value<uintptr_t>();

		if (Piece)
		{
			mOptions->PieceParents[Piece] = ParentGroup;
		}
		else
		{
			lcGroup* Group = (lcGroup*)ChildItem->data(0, GroupRole).value<uintptr_t>();

			// todo: validate unique group name
			Group->mName = ChildItem->text(0);

			mOptions->GroupParents[Group] = ParentGroup;

			UpdateParents(ChildItem, Group);
		}
	}
}

void lcQEditGroupsDialog::AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup)
{
	for (QMap<lcGroup*, lcGroup*>::const_iterator it = mOptions->GroupParents.constBegin(); it != mOptions->GroupParents.constEnd(); it++)
	{
		lcGroup* Group = it.key();
		lcGroup* Parent = it.value();

		if (Parent != ParentGroup)
			continue;

		QTreeWidgetItem* GroupItem = new QTreeWidgetItem(ParentItem, QStringList(Group->mName));
		GroupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
		GroupItem->setData(0, GroupRole, qVariantFromValue<uintptr_t>((uintptr_t)Group));

		AddChildren(GroupItem, Group);
	}

	for (QMap<lcPiece*, lcGroup*>::const_iterator it = mOptions->PieceParents.constBegin(); it != mOptions->PieceParents.constEnd(); it++)
	{
		lcPiece* Piece = it.key();
		lcGroup* Parent = it.value();

		if (Parent != ParentGroup)
			continue;

		QTreeWidgetItem* PieceItem = new QTreeWidgetItem(ParentItem, QStringList(Piece->GetName()));
		PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		PieceItem->setData(0, PieceRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
	}
}
