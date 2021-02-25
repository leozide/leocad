#pragma once

#include <QObject>

QString lcFormatValue(float Value, int Precision);
QString lcFormatValueLocalized(float Value);
float lcParseValueLocalized(const QString& Value);

class lcQTreeWidgetColumnStretcher : public QObject
{
	Q_OBJECT

public:
	lcQTreeWidgetColumnStretcher(QTreeWidget* TreeWidget, int ColumnToStretch);

	bool eventFilter(QObject* Object, QEvent* Event) override;

	const int m_columnToStretch;
};

class lcSmallLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	QSize sizeHint() const override
	{
		QFontMetrics FontMetrics(font());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
		int Width = FontMetrics.horizontalAdvance(QLatin1Char('x')) * 10;
#else
		int Width = FontMetrics.width(QLatin1Char('x')) * 10;
#endif

		return QLineEdit::sizeHint() - QSize(Width, 0);
	}
};

class lcTransformLineEdit : public lcSmallLineEdit
{
	Q_OBJECT

protected:
	bool event(QEvent* Event) override
	{
		if (Event->type() == QEvent::ShortcutOverride)
		{
			QKeyEvent* KeyEvent = (QKeyEvent*)Event;
			int Key = KeyEvent->key();

			if (KeyEvent->modifiers() == Qt::NoModifier && Key >= Qt::Key_A && Key <= Qt::Key_Z)
				Event->accept();

			switch (Key)
			{
			case Qt::Key_Down:
			case Qt::Key_Up:
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Home:
			case Qt::Key_End:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
			case Qt::Key_Plus:
			case Qt::Key_Minus:
			case Qt::Key_Enter:
				Event->accept();
				break;
			}
		}

		return QLineEdit::event(Event);
	}
};
