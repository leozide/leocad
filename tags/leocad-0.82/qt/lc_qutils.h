#ifndef _LC_QUTILS_H_
#define _LC_QUTILS_H_

#include <QObject>

QString lcFormatValue(float Value);

class lcQTreeWidgetColumnStretcher : public QObject
{
public:
	lcQTreeWidgetColumnStretcher(QTreeWidget *treeWidget, int columnToStretch);

	virtual bool eventFilter(QObject *obj, QEvent *ev);

	const int m_columnToStretch;
};

class lcQPartsListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	lcQPartsListModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
};

#endif // _LC_QUTILS_H_
