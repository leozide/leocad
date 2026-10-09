#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_colors.h"
#include "lc_application.h"
#include "pieceinf.h"

class lcFrozenPartsView : public QTableView
{
public:
	explicit lcFrozenPartsView(lcPartsTableWidget* PartsTable)
		: QTableView(PartsTable), mPartsTable(PartsTable)
	{
	}

protected:
	QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& Index, const QEvent* Event = nullptr) const override
	{
		return mPartsTable->TrackSelectionCommand(Index, Event, QTableView::selectionCommand(Index, Event));
	}

	void setSelection(const QRect& Rect, QItemSelectionModel::SelectionFlags Command) override
	{
		if (!mPartsTable->SelectSharedRange(Command))
			QTableView::setSelection(Rect, Command);
	}

private:
	lcPartsTableWidget* mPartsTable;
};

lcPartsTableWidget::lcPartsTableWidget(QWidget* Parent)
	: QTableWidget(Parent)
{
}

void lcPartsTableWidget::FreezePartColumn()
{
	mPartColumnWidth = qMax(sizeHintForColumn(0), horizontalHeader()->sectionSizeHint(0));
	horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	setTextElideMode(Qt::ElideRight);
	for (int Row = 0; Row < rowCount(); Row++)
		if (QTableWidgetItem* Item = item(Row, 0))
			Item->setToolTip(Item->text());

	// Both views share the data and selection, so sorting and row highlights stay in sync.
	mFrozenColumn = new lcFrozenPartsView(this);
	mFrozenColumn->setModel(model());
	mFrozenColumn->setSelectionModel(selectionModel());
	mFrozenColumn->setSelectionBehavior(QAbstractItemView::SelectRows);
	mFrozenColumn->setSelectionMode(selectionMode());
	mFrozenColumn->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mFrozenColumn->setTextElideMode(Qt::ElideRight);
	mFrozenColumn->setFocusPolicy(Qt::NoFocus);
	mFrozenColumn->setFocusProxy(this);
	mFrozenColumn->setFrameShape(QFrame::NoFrame);
	mFrozenColumn->verticalHeader()->hide();
	mFrozenColumn->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	mFrozenColumn->horizontalHeader()->setSectionsClickable(true);
	mFrozenColumn->horizontalHeader()->setSortIndicatorShown(true);
	mFrozenColumn->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mFrozenColumn->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mFrozenColumn->viewport()->installEventFilter(this);
	mFrozenColumn->horizontalHeader()->viewport()->installEventFilter(this);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	mFrozenColumn->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	for (int Column = 1; Column < columnCount(); Column++)
		mFrozenColumn->hideColumn(Column);

	for (int Row = 0; Row < rowCount(); Row++)
		mFrozenColumn->setRowHeight(Row, rowHeight(Row));

	connect(verticalScrollBar(), &QScrollBar::valueChanged, mFrozenColumn->verticalScrollBar(), &QScrollBar::setValue);
	connect(mFrozenColumn->verticalScrollBar(), &QScrollBar::valueChanged, verticalScrollBar(), &QScrollBar::setValue);
	connect(verticalHeader(), &QHeaderView::sectionResized, this, [this](int Row, int, int Height)
	{
		mFrozenColumn->setRowHeight(Row, Height);
	});
	connect(horizontalHeader(), &QHeaderView::sectionResized, this, [this](int Column, int, int)
	{
		if (Column == 0)
			UpdateFrozenGeometry();
	});
	connect(horizontalHeader(), &QHeaderView::sortIndicatorChanged, mFrozenColumn->horizontalHeader(), &QHeaderView::setSortIndicator);
	connect(mFrozenColumn->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int)
	{
		const Qt::SortOrder Order = horizontalHeader()->sortIndicatorSection() == 0 && horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder
			? Qt::DescendingOrder : Qt::AscendingOrder;
		horizontalHeader()->setSortIndicator(0, Order);
	});
	mFrozenColumn->horizontalHeader()->setSortIndicator(horizontalHeader()->sortIndicatorSection(), horizontalHeader()->sortIndicatorOrder());

	viewport()->stackUnder(mFrozenColumn);
	UpdateFrozenGeometry();
	mFrozenColumn->show();
}

bool lcPartsTableWidget::eventFilter(QObject* Object, QEvent* Event)
{
	if (mFrozenColumn && Event->type() == QEvent::Wheel &&
		(Object == mFrozenColumn->viewport() || Object == mFrozenColumn->horizontalHeader()->viewport()))
	{
		QCoreApplication::sendEvent(viewport(), Event);
		return true;
	}

	return QTableWidget::eventFilter(Object, Event);
}

QItemSelectionModel::SelectionFlags lcPartsTableWidget::TrackSelectionCommand(const QModelIndex& Index, const QEvent* Event, QItemSelectionModel::SelectionFlags Command) const
{
	// Qt keeps a separate range anchor in each view, even with a shared selection model.
	mSelectionEnd = Index;
	if (Command.testFlag(QItemSelectionModel::Current))
	{
		if (!mSelectionAnchor.isValid())
			mSelectionAnchor = currentIndex().isValid() ? currentIndex() : Index;
	}
	else if (Command != QItemSelectionModel::NoUpdate || (Event && Event->type() == QEvent::MouseButtonPress))
		mSelectionAnchor = Index;

	return Command;
}

QItemSelectionModel::SelectionFlags lcPartsTableWidget::selectionCommand(const QModelIndex& Index, const QEvent* Event) const
{
	return TrackSelectionCommand(Index, Event, QTableWidget::selectionCommand(Index, Event));
}

bool lcPartsTableWidget::SelectSharedRange(QItemSelectionModel::SelectionFlags Command)
{
	if (!Command.testFlag(QItemSelectionModel::Current) || !mSelectionAnchor.isValid() || !mSelectionEnd.isValid())
		return false;

	const int FirstRow = qMin(mSelectionAnchor.row(), mSelectionEnd.row());
	const int LastRow = qMax(mSelectionAnchor.row(), mSelectionEnd.row());
	selectionModel()->select(QItemSelection(model()->index(FirstRow, 0), model()->index(LastRow, columnCount() - 1)), Command);
	return true;
}

void lcPartsTableWidget::setSelection(const QRect& Rect, QItemSelectionModel::SelectionFlags Command)
{
	if (!SelectSharedRange(Command))
		QTableWidget::setSelection(Rect, Command);
}

void lcPartsTableWidget::UpdateFrozenGeometry()
{
	if (!mFrozenColumn)
		return;

	// Reserve at least half the viewport for counts, while letting short names use less space.
	const int PartWidth = qMin(mPartColumnWidth, qMax(1, viewport()->width() / 2));
	if (columnWidth(0) != PartWidth)
		setColumnWidth(0, PartWidth);

	mFrozenColumn->setColumnWidth(0, columnWidth(0));
	mFrozenColumn->horizontalHeader()->setFixedHeight(horizontalHeader()->height());
	mFrozenColumn->setGeometry(viewport()->geometry().x(), frameWidth(), columnWidth(0), viewport()->height() + horizontalHeader()->height());
}

void lcPartsTableWidget::focusInEvent(QFocusEvent* Event)
{
	const bool HasCurrent = currentIndex().isValid();
	QTableWidget::focusInEvent(Event);

	// Receiving focus should not mark the first cell before the user chooses a row.
	if (!HasCurrent)
		selectionModel()->clearCurrentIndex();
}

void lcPartsTableWidget::resizeEvent(QResizeEvent* Event)
{
	QTableWidget::resizeEvent(Event);
	UpdateFrozenGeometry();
}

void lcPartsTableWidget::EnsureCountVisible(const QModelIndex& Index)
{
	if (mFrozenColumn && Index.column() > 0 && visualRect(Index).left() < columnWidth(0))
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + visualRect(Index).left() - columnWidth(0));
}

QModelIndex lcPartsTableWidget::moveCursor(CursorAction Action, Qt::KeyboardModifiers Modifiers)
{
	const QModelIndex Index = QTableWidget::moveCursor(Action, Modifiers);
	EnsureCountVisible(Index);
	return Index;
}

void lcPartsTableWidget::scrollTo(const QModelIndex& Index, ScrollHint Hint)
{
	if (!mFrozenColumn || Index.column() > 0)
	{
		QTableWidget::scrollTo(Index, Hint);
		EnsureCountVisible(Index);
	}
	else
		mFrozenColumn->scrollTo(Index, Hint);
}

class lcPartsTableWidgetItem : public QTableWidgetItem
{
public:
	explicit lcPartsTableWidgetItem(const QString& Text, int Type = QTableWidgetItem::Type)
		: QTableWidgetItem(Text, Type)
	{
		mLast = false;
	}

	bool operator<(const QTableWidgetItem& Other) const override
	{
		if (mLast)
			return false;

		if (((const lcPartsTableWidgetItem&)Other).mLast)
			return true;

		if (column() > 0)
		{
			int Count = text().toInt();
			int OtherCount = Other.text().toInt();
			return Count < OtherCount;
		}

		return QTableWidgetItem::operator<(Other);
	}

	bool mLast;
};

lcPropertiesDialog::lcPropertiesDialog(QWidget* Parent, lcPropertiesDialogOptions* Options)
	: QDialog(Parent), mOptions(Options), ui(new Ui::lcPropertiesDialog)
{
	ui->setupUi(this);

	setWindowTitle(tr("%1 Properties").arg(mOptions->Properties.mFileName));

	ui->DescriptionEdit->setText(mOptions->Properties.mDescription);
	ui->AuthorEdit->setText(mOptions->Properties.mAuthor);
	ui->CommentsEdit->setText(mOptions->Properties.mComments);

	const lcVector3 Dimensions = Options->BoundingBox.Max - Options->BoundingBox.Min;
	QString Format = tr("%1 x %2 x %3 cm\n%4 x %5 x %6 inches\n%7 x %8 x %9 LDU");
	QString Measurements = Format.arg(QString::number(Dimensions.x * 0.04, 'f', 2), QString::number(Dimensions.y * 0.04, 'f', 2), QString::number(Dimensions.z * 0.04, 'f', 2),
	                                  QString::number(Dimensions.x / 64.0, 'f', 2), QString::number(Dimensions.y / 64.0, 'f', 2), QString::number(Dimensions.z / 64.0, 'f', 2),
	                                  QString::number(Dimensions.x, 'f', 2), QString::number(Dimensions.y, 'f', 2), QString::number(Dimensions.z, 'f', 2));

	ui->MeasurementsLabel->setText(Measurements);

	const lcPartsList& PartsList = mOptions->PartsList;
	QStringList HorizontalLabels;

	std::vector<bool> ColorsUsed(gColorList.size());

	for (const auto& PartIt : PartsList)
		for (const auto& ColorIt : PartIt.second)
			ColorsUsed[ColorIt.first] = true;

	std::vector<int> ColorColumns(gColorList.size());
	int ColorCount = 0;

	HorizontalLabels.append(tr("Part"));

	for (size_t ColorIndex = 0; ColorIndex < gColorList.size(); ColorIndex++)
	{
		if (ColorsUsed[ColorIndex])
		{
			ColorColumns[ColorIndex] = ColorCount++;
			HorizontalLabels.append(gColorList[ColorIndex].Name);
		}
	}

	HorizontalLabels.append(tr("Total"));

	QTableWidget* PartsTable = ui->PartsTable;
	PartsTable->setColumnCount(ColorCount + 2);
	PartsTable->setRowCount((int)PartsList.size() + 1);
	PartsTable->setHorizontalHeaderLabels(HorizontalLabels);
	PartsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	std::vector<int> InfoTotals(PartsList.size());
	std::vector<int> ColorTotals(ColorCount);
	int Row = 0, Total = 0;

	for (const auto& PartIt : PartsList)
	{
		PartsTable->setItem(Row, 0, new lcPartsTableWidgetItem(PartIt.first->m_strDescription));

		for (const auto& ColorIt : PartIt.second)
		{
			int ColorIndex = ColorIt.first;
			int Count = ColorIt.second;

			lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(QString::number(Count));
			Item->setTextAlignment(Qt::AlignCenter);
			PartsTable->setItem(Row, ColorColumns[ColorIndex] + 1, Item);

			InfoTotals[Row] += Count;
			ColorTotals[ColorColumns[ColorIndex]] += Count;
			Total += Count;
		}

		for (int Column = 0; Column <= ColorCount; Column++)
			if (!PartsTable->item(Row, Column))
				PartsTable->setItem(Row, Column, new lcPartsTableWidgetItem(QString()));

		Row++;
	}

	lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(tr("Total"));
	Item->mLast = true;
	PartsTable->setItem((int)InfoTotals.size(), 0, Item);

	for (Row = 0; Row < (int)InfoTotals.size(); Row++)
	{
		Item = new lcPartsTableWidgetItem(QString::number(InfoTotals[Row]));
		Item->setTextAlignment(Qt::AlignCenter);
		PartsTable->setItem(Row, ColorCount + 1, Item);
	}

	for (int ColorIndex = 0; ColorIndex < ColorCount; ColorIndex++)
	{
		Item = new lcPartsTableWidgetItem(QString::number(ColorTotals[ColorIndex]));
		Item->mLast = true;
		Item->setTextAlignment(Qt::AlignCenter);
		PartsTable->setItem((int)InfoTotals.size(), ColorIndex + 1, Item);
	}

	Item = new lcPartsTableWidgetItem(QString::number(Total));
	Item->mLast = true;
	Item->setTextAlignment(Qt::AlignCenter);
	PartsTable->setItem((int)InfoTotals.size(), ColorCount + 1, Item);

	ui->PartsTable->FreezePartColumn();
}

lcPropertiesDialog::~lcPropertiesDialog()
{
	delete ui;
}

void lcPropertiesDialog::accept()
{
	mOptions->Properties.mDescription = ui->DescriptionEdit->text();
	mOptions->Properties.mAuthor = ui->AuthorEdit->text();
	mOptions->Properties.mComments = ui->CommentsEdit->toPlainText();

	QDialog::accept();
}
