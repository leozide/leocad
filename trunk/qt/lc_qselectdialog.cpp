#include "lc_global.h"
#include "lc_qselectdialog.h"
#include "ui_lc_qselectdialog.h"
#include "lc_application.h"
#include "project.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "lc_basewindow.h"

lcQSelectDialog::lcQSelectDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQSelectDialog)
{
	ui->setupUi(this);

	options = (lcSelectDialogOptions*)data;

	AddChildren(ui->treeWidget->invisibleRootItem(), NULL);
	ui->treeWidget->expandAll();

	connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemChanged(QTreeWidgetItem*, int)));
}

lcQSelectDialog::~lcQSelectDialog()
{
	delete ui;
}

void lcQSelectDialog::accept()
{
	options->Objects.RemoveAll();

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
				options->Objects.Add(Object);
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

void lcQSelectDialog::AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup)
{
	lcModel* Model = lcGetActiveModel();
	const lcArray<lcGroup*>& Groups = Model->GetGroups();

	for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = Groups[GroupIdx];

		if (Group->mGroup != ParentGroup)
			continue;

		QTreeWidgetItem* GroupItem = new QTreeWidgetItem(ParentItem, QStringList(Group->m_strName));

		AddChildren(GroupItem, Group);
	}

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();
	lcStep currentStep = Model->GetCurrentStep();

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];

		if (Piece->GetGroup() != ParentGroup || !Piece->IsVisible(currentStep))
			continue;

		QTreeWidgetItem* PieceItem = new QTreeWidgetItem(ParentItem, QStringList(Piece->GetName()));
		PieceItem->setData(0, IndexRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
		PieceItem->setCheckState(0, Piece->IsSelected() ? Qt::Checked : Qt::Unchecked);
	}

	if (!ParentGroup)
	{
		const lcArray<lcCamera*>& Cameras = Model->GetCameras();

		for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
		{
			lcCamera* Camera = Cameras[CameraIdx];

			if (!Camera->IsVisible())
				continue;

			QTreeWidgetItem *cameraItem = new QTreeWidgetItem(ParentItem, QStringList(Camera->GetName()));
			cameraItem->setData(0, IndexRole, qVariantFromValue<uintptr_t>((uintptr_t)Camera));
			cameraItem->setCheckState(0, Camera->IsSelected() ? Qt::Checked : Qt::Unchecked);
		}

		const lcArray<lcLight*>& Lights = Model->GetLights();

		for (int LightIdx = 0; LightIdx < Lights.GetSize(); LightIdx++)
		{
			lcLight* Light = Lights[LightIdx];

			if (!Light->IsVisible())
				continue;

			QTreeWidgetItem *lightItem = new QTreeWidgetItem(ParentItem, QStringList(Light->GetName()));
			lightItem->setData(0, IndexRole, qVariantFromValue<uintptr_t>((uintptr_t)Light));
			lightItem->setCheckState(0, Light->IsSelected() ? Qt::Checked : Qt::Unchecked);
		}
	}
}
