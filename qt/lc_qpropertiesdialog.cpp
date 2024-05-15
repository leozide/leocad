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
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

void lcQPropertiesDialog::accept()
{
	mOptions->Properties.mDescription = ui->DescriptionEdit->text();
	mOptions->Properties.mAuthor = ui->AuthorEdit->text();
	mOptions->Properties.mComments = ui->CommentsEdit->toPlainText();

	QDialog::accept();
}
