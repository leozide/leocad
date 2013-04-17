#ifndef LC_QPARTSTREE_H
#define LC_QPARTSTREE_H

#include <QTreeWidget>
class PieceInfo;

class lcQPartsTree : public QTreeWidget
{
	Q_OBJECT

public:
	explicit lcQPartsTree(QWidget *parent = 0);
	
	QSize sizeHint() const;
	void startDrag(Qt::DropActions supportedActions);

	void updateCategories();
	void searchParts(const QString& searchString);
	void setCurrentPart(PieceInfo *part);

	enum
	{
		PartInfoRole = Qt::UserRole,
		CategoryRole,
		ExpandedOnceRole
	};

public slots:
	void itemExpanded(QTreeWidgetItem *item);

private:
	QTreeWidgetItem *searchResultsItem;
};

#endif // LC_QPARTSTREE_H
