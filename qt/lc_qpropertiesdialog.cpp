#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_colors.h"
#include "lc_application.h"
#include "lc_piecedata.h"
#include "lc_library.h"
#include "lc_thumbnailmanager.h"
#include "lc_bricklink.h"
#include "project.h"

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

class lcPartsThumbnailDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

protected:
	void initStyleOption(QStyleOptionViewItem* Option, const QModelIndex& Index) const override
	{
		QStyledItemDelegate::initStyleOption(Option, Index);
		Option->decorationAlignment = Qt::AlignCenter;
	}
};

lcPartsTableWidget::lcPartsTableWidget(QWidget* Parent)
	: QTableWidget(Parent)
{
}

void lcPartsTableWidget::FreezePartColumns()
{
	// The thumbnail sits in a column of its own so copied cells only hold text.
	setItemDelegateForColumn(0, new lcPartsThumbnailDelegate(this));
	setColumnWidth(0, qMax(iconSize().width() + 8, horizontalHeader()->sectionSizeHint(0)));
	setColumnWidth(1, qMax(sizeHintForColumn(1), horizontalHeader()->sectionSizeHint(1)));
	mPartColumnWidth = qMax(sizeHintForColumn(2), horizontalHeader()->sectionSizeHint(2));
	for (int Column = 0; Column < 3; Column++)
		horizontalHeader()->setSectionResizeMode(Column, QHeaderView::Fixed);
	setTextElideMode(Qt::ElideRight);

	// Both views share the data and selection, so sorting and row highlights stay in sync.
	mFrozenColumn = new lcFrozenPartsView(this);
	mFrozenColumn->setModel(model());
	mFrozenColumn->setSelectionModel(selectionModel());
	mFrozenColumn->setSelectionBehavior(QAbstractItemView::SelectRows);
	mFrozenColumn->setSelectionMode(selectionMode());
	mFrozenColumn->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mFrozenColumn->setTextElideMode(Qt::ElideRight);
	mFrozenColumn->setIconSize(iconSize());
	mFrozenColumn->setItemDelegateForColumn(0, new lcPartsThumbnailDelegate(mFrozenColumn));
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

	for (int Column = 3; Column < columnCount(); Column++)
		mFrozenColumn->hideColumn(Column);

	for (int Row = 0; Row < rowCount(); Row++)
		mFrozenColumn->setRowHeight(Row, rowHeight(Row));

	// The total label spans the thumbnail, ID and piece columns, the total row always sorts last.
	if (rowCount() > 0)
	{
		setSpan(rowCount() - 1, 0, 1, 3);
		mFrozenColumn->setSpan(rowCount() - 1, 0, 1, 3);
	}

	connect(verticalScrollBar(), &QScrollBar::valueChanged, mFrozenColumn->verticalScrollBar(), &QScrollBar::setValue);
	connect(mFrozenColumn->verticalScrollBar(), &QScrollBar::valueChanged, verticalScrollBar(), &QScrollBar::setValue);
	connect(verticalHeader(), &QHeaderView::sectionResized, this, [this](int Row, int, int Height)
	{
		mFrozenColumn->setRowHeight(Row, Height);
	});
	connect(horizontalHeader(), &QHeaderView::sectionResized, this, [this](int Column, int, int)
	{
		if (Column < 3)
			UpdateFrozenGeometry();
	});
	connect(horizontalHeader(), &QHeaderView::sortIndicatorChanged, mFrozenColumn->horizontalHeader(), &QHeaderView::setSortIndicator);
	connect(mFrozenColumn->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int Column)
	{
		const Qt::SortOrder Order = horizontalHeader()->sortIndicatorSection() == Column && horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder
			? Qt::DescendingOrder : Qt::AscendingOrder;
		horizontalHeader()->setSortIndicator(Column, Order);
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

	// The piece column always shows the full description.
	if (columnWidth(2) != mPartColumnWidth)
		setColumnWidth(2, mPartColumnWidth);

	for (int Column = 0; Column < 3; Column++)
		mFrozenColumn->setColumnWidth(Column, columnWidth(Column));
	mFrozenColumn->horizontalHeader()->setFixedHeight(horizontalHeader()->height());
	mFrozenColumn->setGeometry(viewport()->geometry().x(), frameWidth(), GetFrozenWidth(), viewport()->height() + horizontalHeader()->height());
}

int lcPartsTableWidget::GetFrozenWidth() const
{
	return columnWidth(0) + columnWidth(1) + columnWidth(2);
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
	if (mFrozenColumn && Index.column() > 2 && visualRect(Index).left() < GetFrozenWidth())
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + visualRect(Index).left() - GetFrozenWidth());
}

QModelIndex lcPartsTableWidget::moveCursor(CursorAction Action, Qt::KeyboardModifiers Modifiers)
{
	const QModelIndex Index = QTableWidget::moveCursor(Action, Modifiers);
	EnsureCountVisible(Index);
	return Index;
}

void lcPartsTableWidget::scrollTo(const QModelIndex& Index, ScrollHint Hint)
{
	if (!mFrozenColumn || Index.column() > 2)
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
		setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	}

	lcPartsTableWidgetItem(int Count, QLocale Locale)
		: QTableWidgetItem(lcFormatIntegerLocalized(Count, Locale)), mLast(false)
	{
		setData(Qt::UserRole, Count);
	}

	bool operator<(const QTableWidgetItem& Other) const override
	{
		// Keep the total row at the bottom in both sort orders.
		const bool Descending = tableWidget() && tableWidget()->horizontalHeader()->sortIndicatorOrder() == Qt::DescendingOrder;

		if (mLast)
			return Descending;

		if (((const lcPartsTableWidgetItem&)Other).mLast)
			return !Descending;

		if (column() == 0)
			return data(Qt::UserRole).toString() < Other.data(Qt::UserRole).toString();

		if (column() > 2)
		{
			int Count = data(Qt::UserRole).toInt();
			int OtherCount = Other.data(Qt::UserRole).toInt();
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
	lcSetupDialogHelp(this, ui->buttonBox);

	setWindowTitle(tr("%1 Properties").arg(mOptions->Properties.mFileName));
	setWindowState(windowState() | Qt::WindowMaximized);

	ui->DescriptionEdit->setText(mOptions->Properties.mDescription);
	ui->AuthorEdit->setText(mOptions->Properties.mAuthor);
	ui->CommentsEdit->setText(mOptions->Properties.mComments);

	const lcVector3 Dimensions = Options->BoundingBox.Max - Options->BoundingBox.Min;
	QString Format = tr("Metric: %1 x %2 x %3 cm - Imperial: %4 x %5 x %6 inches - LDraw: %7 x %8 x %9 LDU");
	QString Measurements = Format.arg(lcFormatDoubleLocalized(Dimensions.x * 0.04, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.y * 0.04, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.z * 0.04, locale(), 'f', 2),
	                                  lcFormatDoubleLocalized(Dimensions.x / 64.0, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.y / 64.0, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.z / 64.0, locale(), 'f', 2),
	                                  lcFormatDoubleLocalized(Dimensions.x, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.y, locale(), 'f', 2), lcFormatDoubleLocalized(Dimensions.z, locale(), 'f', 2));

	ui->MeasurementsLabel->setText(Measurements);

	const lcPartsList& PartsList = mOptions->PartsList;
	QStringList HorizontalLabels;

	std::vector<bool> ColorsUsed(gColorList.size());

	for (const auto& PartIt : PartsList)
		for (const auto& ColorIt : PartIt.second)
			ColorsUsed[ColorIt.first] = true;

	std::vector<int> ColorColumns(gColorList.size());
	int ColorCount = 0;

	HorizontalLabels.append(tr("Preview"));
	HorizontalLabels.append(tr("ID"));
	HorizontalLabels.append(tr("Piece"));

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
	const int ThumbnailSize = 32;
	PartsTable->setIconSize(QSize(ThumbnailSize, ThumbnailSize));
	PartsTable->verticalHeader()->setDefaultSectionSize(qMax(PartsTable->verticalHeader()->defaultSectionSize(), ThumbnailSize + 4));
	PartsTable->setColumnCount(ColorCount + 4);
	PartsTable->setRowCount((int)PartsList.size() + 1);
	PartsTable->setHorizontalHeaderLabels(HorizontalLabels);
	PartsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	std::vector<int> InfoTotals(PartsList.size());
	std::vector<int> ColorTotals(ColorCount);
	int Row = 0, Total = 0, UniqueCount = 0;
	lcThumbnailManager* ThumbnailManager = lcGetPiecesLibrary()->GetThumbnailManager();

	connect(ThumbnailManager, &lcThumbnailManager::ThumbnailReady, this, &lcPropertiesDialog::ThumbnailReady);

	for (const auto& PartIt : PartsList)
	{
		const lcPieceData* PieceData = PartIt.first;
		QString PieceId = PieceData->mFileName;

		if (PieceId.endsWith(QLatin1String(".dat"), Qt::CaseInsensitive))
			PieceId.chop(4);

		lcPartsTableWidgetItem* ThumbnailItem = new lcPartsTableWidgetItem(QString());
		ThumbnailItem->setData(Qt::UserRole, PieceId);
		PartsTable->setItem(Row, 0, ThumbnailItem);
		PartsTable->setItem(Row, 1, new lcPartsTableWidgetItem(PieceId));
		PartsTable->setItem(Row, 2, new lcPartsTableWidgetItem(PieceData->m_strDescription));

		int ThumbnailColor = gDefaultColor, ThumbnailCount = 0;

		for (const auto& ColorIt : PartIt.second)
		{
			int ColorIndex = ColorIt.first;
			int Count = ColorIt.second;

			lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(Count, locale());
			Item->setTextAlignment(Qt::AlignCenter);
			PartsTable->setItem(Row, ColorColumns[ColorIndex] + 3, Item);

			if (Count > ThumbnailCount)
			{
				ThumbnailColor = ColorIndex;
				ThumbnailCount = Count;
			}

			UniqueCount++;
			InfoTotals[Row] += Count;
			ColorTotals[ColorColumns[ColorIndex]] += Count;
			Total += Count;
		}

		// The thumbnail shows the piece in the color it is used the most.
		auto [ThumbnailId, Thumbnail] = ThumbnailManager->RequestThumbnail(const_cast<lcPieceData*>(PieceData), ThumbnailColor, ThumbnailSize, devicePixelRatioF());
		mThumbnails.emplace_back(ThumbnailId, ThumbnailItem);

		if (!Thumbnail.isNull())
			ThumbnailItem->setData(Qt::DecorationRole, Thumbnail);

		for (int Column = 0; Column <= ColorCount + 2; Column++)
			if (!PartsTable->item(Row, Column))
				PartsTable->setItem(Row, Column, new lcPartsTableWidgetItem(QString()));

		Row++;
	}

	lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(tr("Total"));
	Item->mLast = true;
	PartsTable->setItem((int)InfoTotals.size(), 0, Item);

	Item = new lcPartsTableWidgetItem(QString());
	Item->mLast = true;
	PartsTable->setItem((int)InfoTotals.size(), 1, Item);

	Item = new lcPartsTableWidgetItem(QString());
	Item->mLast = true;
	PartsTable->setItem((int)InfoTotals.size(), 2, Item);

	for (Row = 0; Row < (int)InfoTotals.size(); Row++)
	{
		Item = new lcPartsTableWidgetItem(InfoTotals[Row], locale());
		Item->setTextAlignment(Qt::AlignCenter);
		PartsTable->setItem(Row, ColorCount + 3, Item);
	}

	for (int ColorIndex = 0; ColorIndex < ColorCount; ColorIndex++)
	{
		Item = new lcPartsTableWidgetItem(ColorTotals[ColorIndex], locale());
		Item->mLast = true;
		Item->setTextAlignment(Qt::AlignCenter);
		PartsTable->setItem((int)InfoTotals.size(), ColorIndex + 3, Item);
	}

	Item = new lcPartsTableWidgetItem(Total, locale());
	Item->mLast = true;
	Item->setTextAlignment(Qt::AlignCenter);
	PartsTable->setItem((int)InfoTotals.size(), ColorCount + 3, Item);

	ui->PartsTable->FreezePartColumns();

	ui->PiecesCountLabel->setText(tr("Unique pieces: %1 - Total pieces: %2").arg(lcFormatIntegerLocalized(UniqueCount, locale()), lcFormatIntegerLocalized(Total, locale())));

	ui->ExportBrickLinkButton->setEnabled(!PartsList.empty());
	ui->ExportCSVButton->setEnabled(!PartsList.empty());

	connect(ui->ExportBrickLinkButton, &QPushButton::clicked, this, &lcPropertiesDialog::ExportBrickLink);
	connect(ui->ExportCSVButton, &QPushButton::clicked, this, &lcPropertiesDialog::ExportCSV);
}

lcPropertiesDialog::~lcPropertiesDialog()
{
	lcThumbnailManager* ThumbnailManager = lcGetPiecesLibrary()->GetThumbnailManager();

	for (const auto& [ThumbnailId, Item] : mThumbnails)
		ThumbnailManager->ReleaseThumbnail(ThumbnailId);

	delete ui;
}

void lcPropertiesDialog::ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap)
{
	for (const auto& [Id, Item] : mThumbnails)
		if (Id == ThumbnailId)
			Item->setData(Qt::DecorationRole, Pixmap);
}

QString lcPropertiesDialog::GetExportFileName(const QString& Extension, const QString& Title, const QString& Filter)
{
	const Project* Project = lcGetActiveProject();
	const QString ProjectFileName = Project ? Project->GetFileName() : QString();
	QString FileName = QFileInfo(mOptions->Properties.mFileName).completeBaseName();

	if (FileName.isEmpty())
		FileName = tr("Pieces");

	FileName += "." + Extension;

	if (!ProjectFileName.isEmpty())
		FileName = QFileInfo(ProjectFileName).absoluteDir().filePath(FileName);

	return QFileDialog::getSaveFileName(this, Title, FileName, Filter);
}

void lcPropertiesDialog::ExportBrickLink()
{
	const QString FileName = GetExportFileName("xml", tr("Export BrickLink"), tr("XML Files (*.xml);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcResult<void> Result = lcExportBrickLink(FileName, mOptions->PartsList);

	if (!Result && !Result.error().isEmpty())
		QMessageBox::warning(this, tr("Export BrickLink"), Result.error());
}

void lcPropertiesDialog::ExportCSV()
{
	const QString FileName = GetExportFileName("csv", tr("Export CSV"), tr("CSV Files (*.csv);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcResult<void> Result = lcExportCSV(FileName, mOptions->PartsList);

	if (!Result && !Result.error().isEmpty())
		QMessageBox::warning(this, tr("Export CSV"), Result.error());
}

void lcPropertiesDialog::accept()
{
	mOptions->Properties.mDescription = ui->DescriptionEdit->text();
	mOptions->Properties.mAuthor = ui->AuthorEdit->text();
	mOptions->Properties.mComments = ui->CommentsEdit->toPlainText();

	QDialog::accept();
}
