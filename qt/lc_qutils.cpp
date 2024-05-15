#include "lc_global.h"
#include "lc_qutils.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_model.h"
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
	: QObject(treeWidget->header()), m_columnToStretch(columnToStretch), m_interactiveResize(false), m_stretchWidth(0)
{
	parent()->installEventFilter(this);
	connect(treeWidget->header(), SIGNAL(sectionResized(int, int, int)), SLOT(sectionResized(int, int, int)));
	QHideEvent fake;
	lcQTreeWidgetColumnStretcher::eventFilter(parent(), &fake);
}

void lcQTreeWidgetColumnStretcher::sectionResized(int LogicalIndex, int OldSize, int NewSize)
{
	Q_UNUSED(OldSize)

	if (LogicalIndex == m_columnToStretch) 
	{ 
		QHeaderView* HeaderView = qobject_cast<QHeaderView*>(parent()); 
 
		if (HeaderView->isVisible()) 
			m_interactiveResize = true; 
 
		m_stretchWidth = NewSize; 
	}
}

bool lcQTreeWidgetColumnStretcher::eventFilter(QObject* Object, QEvent* Event)
{
	if (Object == parent())
	{
		QHeaderView* HeaderView = qobject_cast<QHeaderView*>(Object);

		if (Event->type() == QEvent::Show)
		{
			for (int i = 0; i < HeaderView->count(); ++i)
				HeaderView->setSectionResizeMode(i, QHeaderView::Interactive);

			m_stretchWidth = HeaderView->sectionSize(m_columnToStretch);

		}
		else if (Event->type() == QEvent::Hide)
		{
			if (!m_interactiveResize)
				for (int i = 0; i < HeaderView->count(); ++i)
					HeaderView->setSectionResizeMode(i, i == m_columnToStretch ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
		}
		else if (Event->type() == QEvent::Resize)
		{
			if (HeaderView->sectionResizeMode(m_columnToStretch) == QHeaderView::Interactive) {

				const int StretchWidth = HeaderView->isVisible() ? m_stretchWidth : 32;

				HeaderView->resizeSection(m_columnToStretch, StretchWidth);
			}
		}
	}
	return false;
}

lcPieceIdStringModel::lcPieceIdStringModel(lcModel* Model, QObject* Parent)
	: QAbstractListModel(Parent)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	mSortedPieces.reserve(Library->mPieces.size());

	for (const auto& PartIt : Library->mPieces)
	{
		PieceInfo* Info = PartIt.second;

		if (!Info->IsModel() || !Info->GetModel()->IncludesModel(Model))
			mSortedPieces.push_back(PartIt.second);
	}

	auto PieceCompare = [](PieceInfo* Info1, PieceInfo* Info2)
	{
		return strcmp(Info1->m_strDescription, Info2->m_strDescription) < 0;
	};

	std::sort(mSortedPieces.begin(), mSortedPieces.end(), PieceCompare);
}

QModelIndex lcPieceIdStringModel::Index(PieceInfo* Info) const
{
	for (size_t PieceInfoIndex = 0; PieceInfoIndex < mSortedPieces.size(); PieceInfoIndex++)
		if (mSortedPieces[PieceInfoIndex] == Info)
			return index(static_cast<int>(PieceInfoIndex), 0);

	return QModelIndex();
}

std::vector<bool> lcPieceIdStringModel::GetFilteredRows(const QString& FilterText) const
{
	const std::string Text = FilterText.toStdString();
	std::vector<bool> FilteredRows(mSortedPieces.size());

	for (size_t PieceInfoIndex = 0; PieceInfoIndex < mSortedPieces.size(); PieceInfoIndex++)
	{
		const PieceInfo* Info = mSortedPieces[PieceInfoIndex];

		FilteredRows[PieceInfoIndex] = (strcasestr(Info->m_strDescription, Text.c_str()) || strcasestr(Info->mFileName, Text.c_str()));
	}

	return FilteredRows;
}

int lcPieceIdStringModel::rowCount(const QModelIndex& Parent) const
{
	Q_UNUSED(Parent);

	return static_cast<int>(mSortedPieces.size());
}

QVariant lcPieceIdStringModel::data(const QModelIndex& Index, int Role) const
{
	if (Index.row() < static_cast<int>(mSortedPieces.size()))
	{
		if (Role == Qt::DisplayRole)
			return QString::fromLatin1(mSortedPieces[Index.row()]->m_strDescription);
		else if (Role == Qt::UserRole)
			return QVariant::fromValue(reinterpret_cast<void*>(mSortedPieces[Index.row()]));
	}

	return QVariant();
}

lcPieceIdPickerPopup::lcPieceIdPickerPopup(lcModel* Model, PieceInfo* Current, QWidget* Parent)
	: QWidget(Parent)
{
	QVBoxLayout* Layout = new QVBoxLayout(this);
	Layout->setContentsMargins(0, 0, 0, 0);

	mListView = new QListView(this);
	mListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mListView->setSelectionBehavior(QAbstractItemView::SelectRows);
	mListView->setSelectionMode(QAbstractItemView::SingleSelection);
	mListView->setUniformItemSizes(true);

	mListView->installEventFilter(this);

	lcPieceIdStringModel* StringModel = new lcPieceIdStringModel(Model, mListView);
	mListView->setModel(StringModel);

	if (Current)
		mListView->setCurrentIndex(StringModel->Index(Current));

	Layout->addWidget(mListView);

	connect(mListView, &QListView::doubleClicked, this, &lcPieceIdPickerPopup::ListViewDoubleClicked);

	mFilterEdit = new QLineEdit(this);
	mFilterEdit->setPlaceholderText(tr("Filter"));
	Layout->addWidget(mFilterEdit);

	connect(mFilterEdit, &QLineEdit::textEdited, this, &lcPieceIdPickerPopup::FilterEdited);
}

bool lcPieceIdPickerPopup::eventFilter(QObject* Object, QEvent* Event)
{
	if (Object == mListView && Event->type() == QEvent::KeyPress)
	{
		EmitSelectedEvent(mListView->currentIndex());
		return true;
	}

	return QWidget::eventFilter(Object, Event);
}

void lcPieceIdPickerPopup::EmitSelectedEvent(const QModelIndex& Index)
{
	if (!Index.isValid())
		return;

	lcPieceIdStringModel* StringModel = qobject_cast<lcPieceIdStringModel*>(mListView->model());
	QVariant Variant = StringModel->data(Index, Qt::UserRole);

	if (Variant.isValid())
	{
		emit PieceIdSelected(static_cast<PieceInfo*>(Variant.value<void*>()));
	}

	QMenu* Menu = qobject_cast<QMenu*>(parent());
	Menu->close();
}

void lcPieceIdPickerPopup::ListViewDoubleClicked(const QModelIndex& Index)
{
	EmitSelectedEvent(Index);
}

void lcPieceIdPickerPopup::FilterEdited(const QString& Text)
{
	lcPieceIdStringModel* StringModel = qobject_cast<lcPieceIdStringModel*>(mListView->model());
	std::vector<bool> FilteredRows = StringModel->GetFilteredRows(Text);

	mListView->setUpdatesEnabled(false);

	for (int Row = 0; Row < static_cast<int>(FilteredRows.size()); Row++)
		mListView->setRowHidden(Row, !FilteredRows[Row]);

	mListView->setUpdatesEnabled(true);
}
