#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_colors.h"
#include "lc_application.h"
#include "pieceinf.h"

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

lcQPropertiesDialog::lcQPropertiesDialog(QWidget* Parent, lcPropertiesDialogOptions* Options)
	: QDialog(Parent), mOptions(Options), ui(new Ui::lcQPropertiesDialog)
{
	ui->setupUi(this);

	setWindowTitle(tr("%1 Properties").arg(mOptions->Properties.mFileName));

	ui->descriptionEdit->setText(mOptions->Properties.mDescription);
	ui->authorEdit->setText(mOptions->Properties.mAuthor);
	ui->commentsEdit->setText(mOptions->Properties.mComments);

	const lcPartsList& PartsList = mOptions->PartsList;
	QStringList horizontalLabels;

	QVector<bool> ColorsUsed(gNumUserColors);

	for (const auto& PartIt : PartsList)
		for (const auto& ColorIt : PartIt.second)
			ColorsUsed[ColorIt.first] = true;

	QVector<int> ColorColumns(gNumUserColors);
	int NumColors = 0;

	horizontalLabels.append(tr("Part"));

	for (int colorIdx = 0; colorIdx < gNumUserColors; colorIdx++)
	{
		if (ColorsUsed[colorIdx])
		{
			ColorColumns[colorIdx] = NumColors++;
			horizontalLabels.append(gColorList[colorIdx].Name);
		}
	}

	horizontalLabels.append(tr("Total"));

	QTableWidget *table = ui->partsTable;
	table->setColumnCount(NumColors + 2);
	table->setRowCount((int)PartsList.size() + 1);
	table->setHorizontalHeaderLabels(horizontalLabels);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
	table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif

	std::vector<int> InfoTotals(PartsList.size());
	std::vector<int> ColorTotals(NumColors);
	int Row = 0, Total = 0;

	for (const auto& PartIt : PartsList)
	{
		table->setItem(Row, 0, new lcPartsTableWidgetItem(PartIt.first->m_strDescription));

		for (const auto& ColorIt : PartIt.second)
		{
			int ColorIndex = ColorIt.first;
			int Count = ColorIt.second;

			lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(QString::number(Count));
			Item->setTextAlignment(Qt::AlignCenter);
			table->setItem(Row, ColorColumns[ColorIndex] + 1, Item);

			InfoTotals[Row] += Count;
			ColorTotals[ColorColumns[ColorIndex]] += Count;
			Total += Count;
		}

		for (int Column = 0; Column <= NumColors; Column++)
			if (!table->item(Row, Column))
				table->setItem(Row, Column, new lcPartsTableWidgetItem(QString()));

		Row++;
	}

	lcPartsTableWidgetItem* Item = new lcPartsTableWidgetItem(tr("Total"));
	Item->mLast = true;
	table->setItem((int)InfoTotals.size(), 0, Item);

	for (Row = 0; Row < (int)InfoTotals.size(); Row++)
	{
		lcPartsTableWidgetItem *item = new lcPartsTableWidgetItem(QString::number(InfoTotals[Row]));
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem(Row, NumColors + 1, item);
	}

	for (int colorIdx = 0; colorIdx < NumColors; colorIdx++)
	{
		lcPartsTableWidgetItem *item = new lcPartsTableWidgetItem(QString::number(ColorTotals[colorIdx]));
		item->mLast = true;
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem((int)InfoTotals.size(), colorIdx + 1, item);
	}

	lcPartsTableWidgetItem *item = new lcPartsTableWidgetItem(QString::number(Total));
	item->mLast = true;
	item->setTextAlignment(Qt::AlignCenter);
	table->setItem((int)InfoTotals.size(), NumColors + 1, item);
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

void lcQPropertiesDialog::accept()
{
	mOptions->Properties.mDescription = ui->descriptionEdit->text();
	mOptions->Properties.mAuthor = ui->authorEdit->text();
	mOptions->Properties.mComments = ui->commentsEdit->toPlainText();

	QDialog::accept();
}
