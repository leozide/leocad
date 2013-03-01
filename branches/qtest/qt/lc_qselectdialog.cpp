#include "lc_global.h"
#include "lc_qselectdialog.h"
#include "ui_lc_qselectdialog.h"
#include "lc_application.h"
#include "project.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "basewnd.h"

lcQSelectDialog::lcQSelectDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQSelectDialog)
{
	ui->setupUi(this);

	options = (lcSelectDialogOptions*)data;

	addChildren(ui->treeWidget->invisibleRootItem(), NULL);
}

lcQSelectDialog::~lcQSelectDialog()
{
	delete ui;
}

void lcQSelectDialog::accept()
{
	updateSelection(ui->treeWidget->invisibleRootItem());

	// TODO: enforce selection rules

	QDialog::accept();
}

void lcQSelectDialog::on_selectAll_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = true;

	setSelection(ui->treeWidget->invisibleRootItem());
}

void lcQSelectDialog::on_selectNone_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = false;

	setSelection(ui->treeWidget->invisibleRootItem());
}

void lcQSelectDialog::on_selectInvert_clicked()
{
	for (int objectIdx = 0; objectIdx < options->Selection.GetSize(); objectIdx++)
		options->Selection[objectIdx] = !options->Selection[objectIdx];

	setSelection(ui->treeWidget->invisibleRootItem());
}

void lcQSelectDialog::setSelection(QTreeWidgetItem *parentItem)
{
	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);
		int itemIndex = childItem->data(0, IndexRole).value<int>();

		childItem->setSelected(options->Selection[itemIndex]);

		setSelection(childItem);
	}
}

void lcQSelectDialog::updateSelection(QTreeWidgetItem *parentItem)
{
	for (int childIndex = 0; childIndex < parentItem->childCount(); childIndex++)
	{
		QTreeWidgetItem *childItem = parentItem->child(childIndex);
		int itemIndex = childItem->data(0, IndexRole).value<int>();

		options->Selection[itemIndex] = childItem->isSelected();

		updateSelection(childItem);
	}
}

void lcQSelectDialog::addChildren(QTreeWidgetItem *parentItem, Group *parentGroup)
{
	Project *project = lcGetActiveProject();

	for (Group *group = project->m_pGroups; group; group = group->m_pNext)
	{
		if (group->m_pGroup != parentGroup)
			continue;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(parentItem, QStringList(group->m_strName));
		groupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

		addChildren(groupItem, group);
	}

	int numObjects = 0;

	for (Piece *piece = project->m_pPieces; piece; piece = piece->m_pNext, numObjects++)
	{
		if (piece->GetGroup() != parentGroup)
			continue;

		if (!piece->IsVisible(project->GetCurrentTime(), project->IsAnimation()))
			continue;

		QTreeWidgetItem *pieceItem = new QTreeWidgetItem(parentItem, QStringList(piece->GetName()));
		pieceItem->setData(0, IndexRole, qVariantFromValue(numObjects));
		pieceItem->setSelected(options->Selection[numObjects]);
	}

	if (!parentGroup)
	{
		for (int cameraIdx = 0; cameraIdx < project->mCameras.GetSize(); cameraIdx++, numObjects++)
		{
			Camera *camera = project->mCameras[cameraIdx];

			if (!camera->IsVisible())
				continue;

			QTreeWidgetItem *cameraItem = new QTreeWidgetItem(parentItem, QStringList(camera->GetName()));
			cameraItem->setData(0, IndexRole, qVariantFromValue(numObjects));
			cameraItem->setSelected(options->Selection[numObjects]);
		}

		for (Light* light = project->m_pLights; light; light = light->m_pNext, numObjects++)
		{
			if (!light->IsVisible())
				continue;

			QTreeWidgetItem *lightItem = new QTreeWidgetItem(parentItem, QStringList(light->GetName()));
			lightItem->setData(0, IndexRole, qVariantFromValue(numObjects));
			lightItem->setSelected(options->Selection[numObjects]);
		}
	}
}
