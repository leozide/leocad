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
	void UpdateModels();
	void searchParts(const QString& searchString);
	void setCurrentPart(PieceInfo *part);

	enum
	{
		PieceInfoRole = Qt::UserRole,
		CategoryRole,
		ExpandedOnceRole
	};

protected:
	bool event(QEvent *event);

public slots:
	void itemExpanded(QTreeWidgetItem *item);

private:
	QTreeWidgetItem* mModelListItem;
	QTreeWidgetItem* mSearchResultsItem;
};

#endif // LC_QPARTSTREE_H
