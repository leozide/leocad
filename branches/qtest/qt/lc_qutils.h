#ifndef _LC_QUTILS_H_
#define _LC_QUTILS_H_

#include <QObject>

class lcQTreeWidgetColumnStretcher : public QObject
{
public:
	lcQTreeWidgetColumnStretcher(QTreeWidget *treeWidget, int columnToStretch);

	virtual bool eventFilter(QObject *obj, QEvent *ev);

	const int m_columnToStretch;
};

#endif // _LC_QUTILS_H_
