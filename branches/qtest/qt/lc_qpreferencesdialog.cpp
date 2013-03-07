#include "lc_global.h"
#include "lc_qpreferencesdialog.h"
#include "ui_lc_qpreferencesdialog.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

lcQPreferencesDialog::lcQPreferencesDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQPreferencesDialog)
{
    ui->setupUi(this);

	connect(ui->categoriesTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateParts()));

	updateCategories();
}

lcQPreferencesDialog::~lcQPreferencesDialog()
{
    delete ui;
}

void lcQPreferencesDialog::updateCategories()
{
	lcPiecesLibrary *library = lcGetPiecesLibrary();
	QTreeWidgetItem *categoryItem;
	QTreeWidget *tree = ui->categoriesTree;

	while (int itemIndex = tree->topLevelItemCount())
		delete tree->takeTopLevelItem(itemIndex - 1);

	for (int categoryIndex = 0; categoryIndex < library->mCategories.GetSize(); categoryIndex++)
	{
		categoryItem = new QTreeWidgetItem(tree, QStringList((const char*)library->mCategories[categoryIndex].Name));
		categoryItem->setData(0, CategoryRole, QVariant(categoryIndex));
	}

	categoryItem = new QTreeWidgetItem(tree, QStringList(tr("Unassigned")));
	categoryItem->setData(0, CategoryRole, QVariant(-1));
}

void lcQPreferencesDialog::updateParts()
{
	lcPiecesLibrary *library = lcGetPiecesLibrary();
	QTreeWidget *tree = ui->partsTree;

	while (tree->topLevelItemCount())
		delete tree->takeTopLevelItem(0);

	QTreeWidgetItem *categoryItem = ui->categoriesTree->selectedItems().first();
	int categoryIndex = categoryItem->data(0, CategoryRole).toInt();

	if (categoryIndex != -1)
	{
		PtrArray<PieceInfo> singleParts, groupedParts;

		library->GetCategoryEntries(categoryIndex, false, singleParts, groupedParts);

		for (int partIndex = 0; partIndex < singleParts.GetSize(); partIndex++)
		{
			PieceInfo *info = singleParts[partIndex];

			QStringList rowList(info->m_strDescription);
			rowList.append(info->m_strName);

			new QTreeWidgetItem(tree, rowList);
		}
	}
	else
	{
		for (int partIndex = 0; partIndex < library->mPieces.GetSize(); partIndex++)
		{
			PieceInfo *info = library->mPieces[partIndex];

			for (categoryIndex = 0; categoryIndex < library->mCategories.GetSize(); categoryIndex++)
			{
				if (library->PieceInCategory(info, library->mCategories[categoryIndex].Keywords))
					break;
			}

			if (categoryIndex == library->mCategories.GetSize())
			{
				QStringList rowList(info->m_strDescription);
				rowList.append(info->m_strName);

				new QTreeWidgetItem(tree, rowList);
			}
		}
	}

	tree->resizeColumnToContents(0);
	tree->resizeColumnToContents(1);
}
