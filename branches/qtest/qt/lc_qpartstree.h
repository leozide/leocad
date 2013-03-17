#ifndef LC_QPARTSTREE_H
#define LC_QPARTSTREE_H

#include <QTreeWidget>

class lcQPartsTree : public QTreeWidget
{
	Q_OBJECT

public:
	explicit lcQPartsTree(QWidget *parent = 0);
	
	QSize sizeHint() const;

	enum
	{
		PartInfoRole = Qt::UserRole,
		CategoryRole,
		ExpandedOnceRole
	};

signals:
	
public slots:
	void itemExpanded(QTreeWidgetItem *item);
};

#endif // LC_QPARTSTREE_H
