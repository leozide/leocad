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

	addChildren(ui->treeWidget->invisibleRootItem(), NULL);

	connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemChanged(QTreeWidgetItem*, int)));
}

lcQSelectDialog::~lcQSelectDialog()
{
	delete ui;
}

void lcQSelectDialog::accept()
{
	saveSelection(ui->treeWidget->invisibleRootItem());

	QDialog::accept();
}

void lcQSelectDialog::on_selectAll_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = true;

	ui->treeWidget->blockSignals(true);
	loadSelection(ui->treeWidget->invisibleRootItem());
	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::on_selectNone_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = false;

	ui->treeWidget->blockSignals(true);
	loadSelection(ui->treeWidget->invisibleRootItem());
	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::on_selectInvert_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = !options->Selection[objectIdx];

	ui->treeWidget->blockSignals(true);
	loadSelection(ui->treeWidget->invisibleRootItem());
	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::itemChanged(QTreeWidgetItem *item, int column)
{
	int itemIndex = item->data(0, IndexRole).value<int>();
	bool selected = (item->checkState(0) == Qt::Checked);

	if (options->Selection[itemIndex] == selected)
		return;

	options->Selection[itemIndex] = selected;

	QTreeWidgetItem *parentItem = item->parent();

	if (!parentItem)
		return;

	for (;;)
	{
		QTreeWidgetItem *parentParentItem = parentItem->parent();

		if (parentParentItem)
			parentItem = parentParentItem;
		else
			break;
	}

	ui->treeWidget->blockSignals(true);
	setSelection(parentItem, selected);
	ui->treeWidget->blockSignals(false);
}

void lcQSelectDialog::setSelection(QTreeWidgetItem *parentItem, bool selected)
{
	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);

		if (childItem->childCount())
			setSelection(childItem, selected);
		else
		{
			int itemIndex = childItem->data(0, IndexRole).value<int>();

			options->Selection[itemIndex] = selected;
			childItem->setCheckState(0, selected ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void lcQSelectDialog::loadSelection(QTreeWidgetItem *parentItem)
{
	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);

		if (childItem->childCount())
			loadSelection(childItem);
		else
		{
			int itemIndex = childItem->data(0, IndexRole).value<int>();

			childItem->setCheckState(0, options->Selection[itemIndex] ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void lcQSelectDialog::saveSelection(QTreeWidgetItem *parentItem)
{
	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);

		if (childItem->childCount())
			saveSelection(childItem);
		else
		{
			int itemIndex = childItem->data(0, IndexRole).value<int>();

			options->Selection[itemIndex] = (childItem->checkState(0) == Qt::Checked);
		}
	}
}

void lcQSelectDialog::addChildren(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (int groupIdx = 0; groupIdx < project->mGroups.GetSize(); groupIdx++)
	{
		lcGroup* group = project->mGroups[groupIdx];

		if (group->mGroup != parentGroup)
			continue;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(parentItem, QStringList(group->m_strName));

		addChildren(groupItem, group);
	}

	int numObjects = 0;

	for (int pieceIdx = 0; pieceIdx < project->mPieces.GetSize(); pieceIdx++, numObjects++)
	{
		lcPiece *piece = project->mPieces[pieceIdx];

		if (piece->GetGroup() != parentGroup)
			continue;

		if (!piece->IsVisible(project->GetCurrentTime()))
			continue;

		QTreeWidgetItem *pieceItem = new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
		pieceItem->setData(0, IndexRole, qVariantFromValue(numObjects));
		pieceItem->setCheckState(0, options->Selection[numObjects] ? Qt::Checked : Qt::Unchecked);
	}

	if (!parentGroup)
	{
		for (int cameraIdx = 0; cameraIdx < project->mCameras.GetSize(); cameraIdx++, numObjects++)
		{
			lcCamera *camera = project->mCameras[cameraIdx];

			if (!camera->IsVisible())
				continue;

			QTreeWidgetItem *cameraItem = new QTreeWidgetItem(parentItem, QStringList(camera->GetName()));
			cameraItem->setData(0, IndexRole, qVariantFromValue(numObjects));
			cameraItem->setCheckState(0, options->Selection[numObjects] ? Qt::Checked : Qt::Unchecked);
		}

		for (int lightIdx = 0; lightIdx < project->mLights.GetSize(); lightIdx++, numObjects++)
		{
			lcLight* light = project->mLights[lightIdx];

			if (!light->IsVisible())
				continue;

			QTreeWidgetItem *lightItem = new QTreeWidgetItem(parentItem, QStringList(light->GetName()));
			lightItem->setData(0, IndexRole, qVariantFromValue(numObjects));
			lightItem->setCheckState(0, options->Selection[numObjects] ? Qt::Checked : Qt::Unchecked);
		}
	}
}
