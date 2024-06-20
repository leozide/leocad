#include "lc_global.h"
#include "lc_qselectdialog.h"
#include "ui_lc_qselectdialog.h"
#include "lc_application.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"

lcQSelectDialog::lcQSelectDialog(QWidget* Parent, lcModel* Model)
	: QDialog(Parent), ui(new Ui::lcQSelectDialog)
{
	ui->setupUi(this);

	AddChildren(ui->treeWidget->invisibleRootItem(), nullptr, Model);
	ui->treeWidget->expandAll();

	connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemChanged(QTreeWidgetItem*, int)));
}

lcQSelectDialog::~lcQSelectDialog()
{
	delete ui;
}

void lcQSelectDialog::accept()
{
	mObjects.clear();

	QList<QTreeWidgetItem*> Items;
	Items.append(ui->treeWidget->invisibleRootItem());

	while (!Items.isEmpty())
	{
		QTreeWidgetItem* Item = Items[0];
		Items.removeFirst();

		if (!Item->childCount())
		{
			if (Item->checkState(0) == Qt::Checked)
			{
				lcObject* Object = (lcObject*)Item->data(0, IndexRole).value<uintptr_t>();
				mObjects.emplace_back(Object);
			}
		}
		else
		{
			for (int ChildIdx = 0; ChildIdx < Item->childCount(); ChildIdx++)
				Items.append(Item->child(ChildIdx));
		}
	}

	QDialog::accept();
}

void lcQSelectDialog::on_selectAll_clicked()
{
	ui->treeWidget->blockSignals(true);

	QList<QTreeWidgetItem*> Items;
	Items.append(ui->treeWidget->invisibleRootItem());

	while (!Items.isEmpty())
	{
		QTreeWidgetItem* Item = Items[0];
		Items.removeFirst();

		if (!Item->childCount())
			Item->setCheckState(0, Qt::Checked);
		else
		{
			for (int ChildIdx = 0; ChildIdx < Item->childCount(); ChildIdx++)
				Items.append(Item->child(ChildIdx));
		}
	}

	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::on_selectNone_clicked()
{
	ui->treeWidget->blockSignals(true);

	QList<QTreeWidgetItem*> Items;
	Items.append(ui->treeWidget->invisibleRootItem());

	while (!Items.isEmpty())
	{
		QTreeWidgetItem* Item = Items[0];
		Items.removeFirst();

		if (!Item->childCount())
			Item->setCheckState(0, Qt::Unchecked);
		else
		{
			for (int ChildIdx = 0; ChildIdx < Item->childCount(); ChildIdx++)
				Items.append(Item->child(ChildIdx));
		}
	}

	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::on_selectInvert_clicked()
{
	ui->treeWidget->blockSignals(true);

	QList<QTreeWidgetItem*> Items;
	Items.append(ui->treeWidget->invisibleRootItem());

	while (!Items.isEmpty())
	{
		QTreeWidgetItem* Item = Items[0];
		Items.removeFirst();

		if (!Item->childCount())
			Item->setCheckState(0, Item->checkState(0) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
		else
		{
			for (int ChildIdx = 0; ChildIdx < Item->childCount(); ChildIdx++)
				Items.append(Item->child(ChildIdx));
		}
	}

	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::itemChanged(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column);

	QTreeWidgetItem* ParentItem = item->parent();

	if (!ParentItem)
		return;

	Qt::CheckState State = item->checkState(0);

	for (;;)
	{
		QTreeWidgetItem* ParentParentItem = ParentItem->parent();

		if (ParentParentItem)
			ParentItem = ParentParentItem;
		else
			break;
	}

	ui->treeWidget->blockSignals(true);

	QList<QTreeWidgetItem*> Items;
	Items.append(ParentItem);

	while (!Items.isEmpty())
	{
		QTreeWidgetItem* Item = Items[0];
		Items.removeFirst();

		if (!Item->childCount())
			Item->setCheckState(0, State);
		else
		{
			for (int ChildIdx = 0; ChildIdx < Item->childCount(); ChildIdx++)
				Items.append(Item->child(ChildIdx));
		}
	}

	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup, lcModel* Model)
{
	const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();

	for (const std::unique_ptr<lcGroup>& Group : Groups)
	{
		if (Group->mGroup != ParentGroup)
			continue;

		QTreeWidgetItem* GroupItem = new QTreeWidgetItem(ParentItem, QStringList(Group->mName));

		AddChildren(GroupItem, Group.get(), Model);
	}

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	lcStep currentStep = Model->GetCurrentStep();

	for (const std::unique_ptr<lcPiece>& Piece : Pieces)
	{
		if (Piece->GetGroup() != ParentGroup || !Piece->IsVisible(currentStep))
			continue;

		QTreeWidgetItem* PieceItem = new QTreeWidgetItem(ParentItem, QStringList(Piece->GetName()));
		PieceItem->setData(0, IndexRole, QVariant::fromValue<uintptr_t>((uintptr_t)Piece.get()));
		PieceItem->setCheckState(0, Piece->IsSelected() ? Qt::Checked : Qt::Unchecked);
	}

	if (!ParentGroup)
	{
		const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();

		for (const std::unique_ptr<lcCamera>& Camera : Cameras)
		{
			if (!Camera->IsVisible())
				continue;

			QTreeWidgetItem *cameraItem = new QTreeWidgetItem(ParentItem, QStringList(Camera->GetName()));
			cameraItem->setData(0, IndexRole, QVariant::fromValue<uintptr_t>((uintptr_t)Camera.get()));
			cameraItem->setCheckState(0, Camera->IsSelected() ? Qt::Checked : Qt::Unchecked);
		}

		const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

		for (const std::unique_ptr<lcLight>& Light : Lights)
		{
			if (!Light->IsVisible())
				continue;

			QTreeWidgetItem *lightItem = new QTreeWidgetItem(ParentItem, QStringList(Light->GetName()));
			lightItem->setData(0, IndexRole, QVariant::fromValue<uintptr_t>((uintptr_t)Light.get()));
			lightItem->setCheckState(0, Light->IsSelected() ? Qt::Checked : Qt::Unchecked);
		}
	}
}
