#include "lc_global.h"
#include "lc_qutils.h"
#include "lc_application.h"
#include "lc_library.h"
#include "pieceinf.h"

QString lcFormatValue(float Value, int Precision)
{
	QString String = QString::number(Value, 'f', Precision);
	const int Dot = String.indexOf('.');

	if (Dot != -1)
	{
		while (String.endsWith('0'))
			String.chop(1);

		if (String.endsWith('.'))
			String.chop(1);
	}

	return String;
}

QString lcFormatValueLocalized(float Value)
{
	QLocale Locale = QLocale::system();
	Locale.setNumberOptions(QLocale::OmitGroupSeparator);
	QString DecimalPoint = Locale.decimalPoint();
	QString String = Locale.toString(Value, 'f', 4);

	if (String.indexOf(DecimalPoint) != -1)
	{
		while (String.endsWith('0'))
			String.chop(1);

		if (String.endsWith(DecimalPoint))
			String.chop(1);
	}

	return String;
}

float lcParseValueLocalized(const QString& Value)
{
	return QLocale::system().toFloat(Value);
}

// Resize all columns to content except for one stretching column. (taken from QT creator)
lcQTreeWidgetColumnStretcher::lcQTreeWidgetColumnStretcher(QTreeWidget *treeWidget, int columnToStretch)
	: QObject(treeWidget->header()), m_columnToStretch(columnToStretch)
{
	parent()->installEventFilter(this);
	QHideEvent fake;
	lcQTreeWidgetColumnStretcher::eventFilter(parent(), &fake);
}

bool lcQTreeWidgetColumnStretcher::eventFilter(QObject* Object, QEvent* Event)
{
	if (Object == parent())
	{
		if (Event->type() == QEvent::Show)
		{
			QHeaderView* HeaderView = qobject_cast<QHeaderView*>(Object);

			for (int i = 0; i < HeaderView->count(); ++i)
				HeaderView->setSectionResizeMode(i, QHeaderView::Interactive);
		}
		else if (Event->type() == QEvent::Hide)
		{
			QHeaderView* HeaderView = qobject_cast<QHeaderView*>(Object);

			for (int i = 0; i < HeaderView->count(); ++i)
				HeaderView->setSectionResizeMode(i, i == m_columnToStretch ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
		}
		else if (Event->type() == QEvent::Resize)
		{
			QHeaderView* HeaderView = qobject_cast<QHeaderView*>(Object);

			if (HeaderView->sectionResizeMode(m_columnToStretch) == QHeaderView::Interactive)
			{
				const QResizeEvent* ResizeEvent = reinterpret_cast<QResizeEvent*>(Event);
				const int Diff = ResizeEvent->size().width() - ResizeEvent->oldSize().width() ;
				HeaderView->resizeSection(m_columnToStretch, qMax(32, HeaderView->sectionSize(1) + Diff));
			}
		}
	}
	return false;
}
