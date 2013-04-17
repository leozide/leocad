#include "lc_global.h"
#include "lc_qpartstree.h"
#include "lc_application.h"
#include "lc_category.h"
#include "lc_library.h"
#include "pieceinf.h"

static int lcQPartsTreeSortFunc(const PieceInfo* a, const PieceInfo* b, void* sortData)
{
	if (a->IsSubPiece())
	{
		if (b->IsSubPiece())
			return strcmp(a->m_strDescription, b->m_strDescription);
		else
			return 1;
	}
	else
	{
		if (b->IsSubPiece())
			return -1;
		else
			return strcmp(a->m_strDescription, b->m_strDescription);
	}

	return 0;
}

lcQPartsTree::lcQPartsTree(QWidget *parent) :
    QTreeWidget(parent)
{
	setDragEnabled(true);
	setHeaderHidden(true);
	connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(itemExpanded(QTreeWidgetItem*)));

	updateCategories();
}

QSize lcQPartsTree::sizeHint() const
{
	QSize sizeHint = QTreeWidget::sizeHint();

	sizeHint.setWidth(200);

	return sizeHint;
}

void lcQPartsTree::updateCategories()
{
	clear();

	for (int categoryIndex = 0; categoryIndex < gCategories.GetSize(); categoryIndex++)
	{
		QTreeWidgetItem* categoryItem = new QTreeWidgetItem(this, QStringList((const char*)gCategories[categoryIndex].Name));
		categoryItem->setData(0, ExpandedOnceRole, QVariant(false));
		categoryItem->setData(0, CategoryRole, QVariant(categoryIndex));
		new QTreeWidgetItem(categoryItem);
	}

	searchResultsItem = new QTreeWidgetItem(this, QStringList(tr("Search Results")));
}

void lcQPartsTree::searchParts(const QString& searchString)
{
	while (QTreeWidgetItem *item = searchResultsItem->child(0))
		delete item;

	lcPiecesLibrary* library = lcGetPiecesLibrary();
	PtrArray<PieceInfo> singleParts, groupedParts;

	library->SearchPieces(searchString.toLocal8Bit().data(), false, singleParts, groupedParts);
	singleParts.Sort(lcQPartsTreeSortFunc, NULL);

	for (int partIndex = 0; partIndex < singleParts.GetSize(); partIndex++)
	{
		PieceInfo* partInfo = singleParts[partIndex];

		QTreeWidgetItem* partItem = new QTreeWidgetItem(searchResultsItem, QStringList(partInfo->m_strDescription));
		partItem->setData(0, PartInfoRole, qVariantFromValue((void*)partInfo));
		partItem->setToolTip(0, QString("%1 (%2)").arg(partInfo->m_strDescription, partInfo->m_strName));
	}

	setCurrentItem(searchResultsItem);
	expandItem(searchResultsItem);
	scrollToItem(searchResultsItem, PositionAtTop);
}

void lcQPartsTree::itemExpanded(QTreeWidgetItem *expandedItem)
{
	QTreeWidgetItem *parent = expandedItem->parent();

	if (parent || expandedItem == searchResultsItem)
		return;

	if (expandedItem->data(0, ExpandedOnceRole).toBool())
		return;

	QTreeWidgetItem *child = expandedItem->child(0);
	expandedItem->removeChild(child);
	delete child;

	int categoryIndex = expandedItem->data(0, CategoryRole).toInt();

	lcPiecesLibrary* library = lcGetPiecesLibrary();
	PtrArray<PieceInfo> singleParts, groupedParts;

	library->GetCategoryEntries(categoryIndex, true, singleParts, groupedParts);

	singleParts += groupedParts;
	singleParts.Sort(lcQPartsTreeSortFunc, NULL);

	for (int partIndex = 0; partIndex < singleParts.GetSize(); partIndex++)
	{
		PieceInfo* partInfo = singleParts[partIndex];

		QTreeWidgetItem* partItem = new QTreeWidgetItem(expandedItem, QStringList(partInfo->m_strDescription));
		partItem->setData(0, PartInfoRole, qVariantFromValue((void*)partInfo));
		partItem->setToolTip(0, QString("%1 (%2)").arg(partInfo->m_strDescription, partInfo->m_strName));

		if (groupedParts.FindIndex(partInfo) != -1)
		{
			PtrArray<PieceInfo> patterns;
			library->GetPatternedPieces(partInfo, patterns);

			for (int patternIndex = 0; patternIndex < patterns.GetSize(); patternIndex++)
			{
				PieceInfo* patternedInfo = patterns[patternIndex];

				if (!library->PieceInCategory(patternedInfo, gCategories[categoryIndex].Keywords))
					continue;

				const char* desc = patternedInfo->m_strDescription;
				int len = strlen(partInfo->m_strDescription);

				if (!strncmp(patternedInfo->m_strDescription, partInfo->m_strDescription, len))
					desc += len;

				QTreeWidgetItem* patternedItem = new QTreeWidgetItem(partItem, QStringList(desc));
				patternedItem->setData(0, PartInfoRole, qVariantFromValue((void*)patternedInfo));
				patternedItem->setToolTip(0, QString("%1 (%2)").arg(patternedInfo->m_strDescription, patternedInfo->m_strName));
			}
		}
	}

	expandedItem->setData(0, ExpandedOnceRole, QVariant(true));
}

void lcQPartsTree::setCurrentPart(PieceInfo *part)
{
	lcPiecesLibrary* library = lcGetPiecesLibrary();
	int categoryIndex;

	for (categoryIndex = 0; categoryIndex < gCategories.GetSize(); categoryIndex++)
		if (library->PieceInCategory(part, gCategories[categoryIndex].Keywords))
			break;

	if (categoryIndex == gCategories.GetSize())
		return;

	QTreeWidgetItem *categoryItem = topLevelItem(categoryIndex);

	expandItem(categoryItem);

	if (part->IsPatterned())
	{
		char parentName[LC_PIECE_NAME_LEN];
		strcpy(parentName, part->m_strName);
		*strchr(parentName, 'P') = '\0';

		PieceInfo *parentPart = library->FindPiece(parentName, false);

		if (parentPart)
		{
			for (int itemIndex = 0; itemIndex < categoryItem->childCount(); itemIndex++)
			{
				QTreeWidgetItem *item = categoryItem->child(itemIndex);
				PieceInfo *info = (PieceInfo*)item->data(0, lcQPartsTree::PartInfoRole).value<void*>();

				if (info == parentPart)
				{
					expandItem(item);
					categoryItem = item;
					break;
				}
			}
		}
	}

	for (int itemIndex = 0; itemIndex < categoryItem->childCount(); itemIndex++)
	{
		QTreeWidgetItem *item = categoryItem->child(itemIndex);
		PieceInfo *info = (PieceInfo*)item->data(0, lcQPartsTree::PartInfoRole).value<void*>();

		if (info == part)
		{
			setCurrentItem(item);
			scrollToItem(item);
			break;
		}
	}
}

void lcQPartsTree::startDrag(Qt::DropActions supportedActions)
{
	PieceInfo *info = (PieceInfo*)currentItem()->data(0, lcQPartsTree::PartInfoRole).value<void*>();

	if (!info)
		return;

	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << QString(info->m_strName);

	QMimeData *mimeData = new QMimeData;
	mimeData->setData("application/vnd.leocad-part", itemData);

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);

	drag->exec(Qt::CopyAction);
}
