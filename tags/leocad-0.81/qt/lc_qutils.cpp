#include "lc_global.h"
#include "lc_qutils.h"
#include "lc_application.h"
#include "lc_library.h"
#include "pieceinf.h"

// Resize all columns to content except for one stretching column. (taken from QT creator)
lcQTreeWidgetColumnStretcher::lcQTreeWidgetColumnStretcher(QTreeWidget *treeWidget, int columnToStretch)
	: QObject(treeWidget->header()), m_columnToStretch(columnToStretch)
{
	parent()->installEventFilter(this);
	QHideEvent fake;
	lcQTreeWidgetColumnStretcher::eventFilter(parent(), &fake);
}

bool lcQTreeWidgetColumnStretcher::eventFilter(QObject *obj, QEvent *ev)
{
	if (obj == parent())
	{
		if (ev->type() == QEvent::Show)
		{
			QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
			for (int i = 0; i < hv->count(); ++i)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
				hv->setSectionResizeMode(i, QHeaderView::Interactive);
#else
				hv->setResizeMode(i, QHeaderView::Interactive);
#endif
		}
		else if (ev->type() == QEvent::Hide)
		{
			QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
			for (int i = 0; i < hv->count(); ++i)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
				hv->setSectionResizeMode(i, i == m_columnToStretch ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
#else
				hv->setResizeMode(i, i == m_columnToStretch ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
#endif
		}
		else if (ev->type() == QEvent::Resize)
		{
			QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
			if (hv->sectionResizeMode(m_columnToStretch) == QHeaderView::Interactive)
#else
			if (hv->resizeMode(m_columnToStretch) == QHeaderView::Interactive)
#endif
			{
				QResizeEvent *re = static_cast<QResizeEvent*>(ev);
				int diff = re->size().width() - re->oldSize().width() ;
				hv->resizeSection(m_columnToStretch, qMax(32, hv->sectionSize(1) + diff));
			}
		}
	}
	return false;
}

lcQPartsListModel::lcQPartsListModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

int lcQPartsListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return lcGetPiecesLibrary()->mPieces.GetSize() * 2;
}

QVariant lcQPartsListModel::data(const QModelIndex &index, int role) const
{
	int partIndex = index.row() / 2;

	if (partIndex < 0 || partIndex >= lcGetPiecesLibrary()->mPieces.GetSize())
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		if (index.row() & 1)
			return QString(lcGetPiecesLibrary()->mPieces[partIndex]->m_strName);
		else
			return QString(lcGetPiecesLibrary()->mPieces[partIndex]->m_strDescription);
	}

	return QVariant();
}
