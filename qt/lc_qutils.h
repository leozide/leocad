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

class lcTransformLineEdit : public QLineEdit
{
public:
	lcTransformLineEdit()
		: QLineEdit()
	{
	}

	virtual QSize sizeHint() const
	{
		QFontMetrics FontMetrics(font());

		int Width = FontMetrics.width(QLatin1Char('x')) * 10;

		return QLineEdit::sizeHint() - QSize(Width, 0);
	}
};
