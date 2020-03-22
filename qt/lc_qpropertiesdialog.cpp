#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_basewindow.h"
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

		return QTableWidgetItem::operator<(Other);
	}

	bool mLast;
};

lcQPropertiesDialog::lcQPropertiesDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQPropertiesDialog)
{
	ui->setupUi(this);

	connect(ui->solidColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient1ColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient2ColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));

	options = (lcPropertiesDialogOptions*)data;

	setWindowTitle(tr("%1 Properties").arg(options->Properties.mName));

	ui->descriptionEdit->setText(options->Properties.mDescription);
	ui->authorEdit->setText(options->Properties.mAuthor);
	ui->commentsEdit->setText(options->Properties.mComments);

	if (options->Properties.mBackgroundType == LC_BACKGROUND_IMAGE)
		ui->imageRadio->setChecked(true);
	else if (options->Properties.mBackgroundType == LC_BACKGROUND_GRADIENT)
		ui->gradientRadio->setChecked(true);
	else
		ui->solidRadio->setChecked(true);

	ui->imageNameEdit->setText(options->Properties.mBackgroundImage);
	ui->imageTileCheckBox->setChecked(options->Properties.mBackgroundImageTile);

	QPixmap pix(12, 12);

	pix.fill(QColor(options->Properties.mBackgroundSolidColor[0] * 255, options->Properties.mBackgroundSolidColor[1] * 255, options->Properties.mBackgroundSolidColor[2] * 255));
	ui->solidColorButton->setIcon(pix);
	pix.fill(QColor(options->Properties.mBackgroundGradientColor1[0] * 255, options->Properties.mBackgroundGradientColor1[1] * 255, options->Properties.mBackgroundGradientColor1[2] * 255));
	ui->gradient1ColorButton->setIcon(pix);
	pix.fill(QColor(options->Properties.mBackgroundGradientColor2[0] * 255, options->Properties.mBackgroundGradientColor2[1] * 255, options->Properties.mBackgroundGradientColor2[2] * 255));
	ui->gradient2ColorButton->setIcon(pix);

	const lcPartsList& PartsList = options->PartsList;
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
	options->Properties.mDescription = ui->descriptionEdit->text();
	options->Properties.mAuthor = ui->authorEdit->text();
	options->Properties.mComments = ui->commentsEdit->toPlainText();

	if (ui->imageRadio->isChecked())
		 options->Properties.mBackgroundType = LC_BACKGROUND_IMAGE;
	else if (ui->gradientRadio->isChecked())
		 options->Properties.mBackgroundType = LC_BACKGROUND_GRADIENT;
	else
		 options->Properties.mBackgroundType = LC_BACKGROUND_SOLID;

	options->Properties.mBackgroundImage = ui->imageNameEdit->text();
	options->Properties.mBackgroundImageTile = ui->imageTileCheckBox->isChecked();
	options->SetDefault = ui->setDefaultCheckBox->isChecked();

	QDialog::accept();
}

void lcQPropertiesDialog::colorClicked()
{
	QObject *button = sender();
	QString title;
	float *color = nullptr;

	if (button == ui->solidColorButton)
	{
		color = options->Properties.mBackgroundSolidColor;
		title = tr("Select Background Color");
	}
	else if (button == ui->gradient1ColorButton)
	{
		color = options->Properties.mBackgroundGradientColor1;
		title = tr("Select Background Top Color");
	}
	else if (button == ui->gradient2ColorButton)
	{
		color = options->Properties.mBackgroundGradientColor2;
		title = tr("Select Background Bottom Color");
	}

	if (!color)
		return;

	QColor oldColor = QColor(color[0] * 255, color[1] * 255, color[2] * 255);
	QColor newColor = QColorDialog::getColor(oldColor, this, title);

	if (newColor == oldColor || !newColor.isValid())
		return;

	color[0] = (float)newColor.red() / 255.0f;
	color[1] = (float)newColor.green() / 255.0f;
	color[2] = (float)newColor.blue() / 255.0f;

	QPixmap pix(12, 12);

	pix.fill(newColor);
	((QToolButton*)button)->setIcon(pix);
}

void lcQPropertiesDialog::on_imageNameButton_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Select Background Image"), ui->imageNameEdit->text(), tr("All Image Files (*.png *.jpg *.gif *.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;GIF Files (*.gif);;BMP Files (*.bmp);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->imageNameEdit->setText(QDir::toNativeSeparators(result));
}
