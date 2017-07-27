#pragma once

#include <QObject>

QString lcFormatValue(float Value, int Precision);
QString lcFormatValueLocalized(float Value);
float lcParseValueLocalized(const QString& Value);

class lcQTreeWidgetColumnStretcher : public QObject
{
public:
	lcQTreeWidgetColumnStretcher(QTreeWidget *treeWidget, int columnToStretch);

	virtual bool eventFilter(QObject *obj, QEvent *ev);

	const int m_columnToStretch;
};

