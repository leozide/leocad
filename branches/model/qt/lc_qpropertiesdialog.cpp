#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "lc_basewindow.h"
#include "lc_colors.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

lcQPropertiesDialog::lcQPropertiesDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQPropertiesDialog)
{
	ui->setupUi(this);

	ui->fogDensityEdit->setValidator(new QDoubleValidator());

	connect(ui->solidColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient1ColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient2ColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->fogColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->ambientColorButton, SIGNAL(clicked()), this, SLOT(colorClicked()));

	options = (lcPropertiesDialogOptions*)data;

	ui->nameEdit->setText((const char*)options->Properties.mName);
	ui->descriptionEdit->setText((const char*)options->Properties.mDescription);
	ui->authorEdit->setText((const char*)options->Properties.mAuthor);
	ui->commentsEdit->setText((const char*)options->Properties.mComments);

	if (options->Properties.mBackgroundType == LC_BACKGROUND_IMAGE)
		ui->imageRadio->setChecked(true);
	else if (options->Properties.mBackgroundType == LC_BACKGROUND_GRADIENT)
		ui->gradientRadio->setChecked(true);
	else
		ui->solidRadio->setChecked(true);

	ui->imageNameEdit->setText((const char*)options->Properties.mBackgroundImage);
	ui->imageTileCheckBox->setChecked(options->Properties.mBackgroundImageTile);
	ui->fogCheckBox->setChecked(options->Properties.mFogEnabled);
	ui->fogDensityEdit->setText(QString::number(options->Properties.mFogDensity));

	QPixmap pix(12, 12);

	lcuint32 color = options->Properties.mBackgroundSolidColor;
	pix.fill(QColor(LC_RGBA_RED(color), LC_RGBA_GREEN(color), LC_RGBA_BLUE(color)));
	ui->solidColorButton->setIcon(pix);
	color = options->Properties.mBackgroundGradientColor1;
	pix.fill(QColor(LC_RGBA_RED(color), LC_RGBA_GREEN(color), LC_RGBA_BLUE(color)));
	ui->gradient1ColorButton->setIcon(pix);
	color = options->Properties.mBackgroundGradientColor2;
	pix.fill(QColor(LC_RGBA_RED(color), LC_RGBA_GREEN(color), LC_RGBA_BLUE(color)));
	ui->gradient2ColorButton->setIcon(pix);
	color = options->Properties.mFogColor;
	pix.fill(QColor(LC_RGBA_RED(color), LC_RGBA_GREEN(color), LC_RGBA_BLUE(color)));
	ui->fogColorButton->setIcon(pix);
	color = options->Properties.mAmbientColor;
	pix.fill(QColor(LC_RGBA_RED(color), LC_RGBA_GREEN(color), LC_RGBA_BLUE(color)));
	ui->ambientColorButton->setIcon(pix);

	lcPiecesLibrary *library = lcGetPiecesLibrary();
	lcArray<lcPiecesUsedEntry>& partsUsed = options->PartsUsed;
	QStringList horizontalLabels, partNames;

	bool *colorsUsed = new bool[gNumUserColors];
	memset(colorsUsed, 0, sizeof(bool) * gNumUserColors);

	int *infoRows = new int[library->mPieces.GetSize()], numInfos = 0;
	memset(infoRows, 0, sizeof(int) * library->mPieces.GetSize());

	for (int partIdx = 0; partIdx < partsUsed.GetSize(); partIdx++)
	{
		colorsUsed[partsUsed[partIdx].ColorIndex] = true;

		int infoIndex = library->mPieces.FindIndex(partsUsed[partIdx].Info);
		if (!infoRows[infoIndex])
		{
			infoRows[infoIndex] = numInfos++;
			partNames.append(partsUsed[partIdx].Info->m_strDescription);
		}
	}

	int *colorColumns = new int[gNumUserColors];
	memset(colorColumns, 0, sizeof(int) * gNumUserColors);
	int numColors = 0;

	horizontalLabels.append(tr("Part"));

	for (int colorIdx = 0; colorIdx < gNumUserColors; colorIdx++)
	{
		if (colorsUsed[colorIdx])
		{
			colorColumns[colorIdx] = numColors++;
			horizontalLabels.append(gColorList[colorIdx].Name);
		}
	}

	QTableWidget *table = ui->partsTable;
	table->setColumnCount(numColors + 1);
	table->setRowCount(numInfos);
	table->setHorizontalHeaderLabels(horizontalLabels);
	table->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);

	for (int rowIdx = 0; rowIdx < partNames.size(); rowIdx++)
		table->setItem(rowIdx, 0, new QTableWidgetItem(partNames[rowIdx]));

	for (int partIdx = 0; partIdx < partsUsed.GetSize(); partIdx++)
	{
		int colorIndex = partsUsed[partIdx].ColorIndex;
		int infoIndex = library->mPieces.FindIndex(partsUsed[partIdx].Info);

		QTableWidgetItem *item = new QTableWidgetItem(QString::number(partsUsed[partIdx].Count));
		item->setTextAlignment(Qt::AlignCenter);
		table->setItem(infoRows[infoIndex], colorColumns[colorIndex] + 1, item);
	}

	delete[] colorColumns;
	delete[] colorsUsed;
	delete[] infoRows;
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

void lcQPropertiesDialog::accept()
{
	options->Properties.mName = ui->nameEdit->text().toLocal8Bit().data();
	options->Properties.mAuthor = ui->authorEdit->text().toLocal8Bit().data();
	options->Properties.mDescription = ui->descriptionEdit->text().toLocal8Bit().data();
	options->Properties.mComments = ui->commentsEdit->toPlainText().toLocal8Bit().data();

	if (ui->imageRadio->isChecked())
		 options->Properties.mBackgroundType = LC_BACKGROUND_IMAGE;
	else if (ui->gradientRadio->isChecked())
		 options->Properties.mBackgroundType = LC_BACKGROUND_GRADIENT;
	else
		 options->Properties.mBackgroundType = LC_BACKGROUND_SOLID;

	options->Properties.mBackgroundImage = ui->imageNameEdit->text().toLocal8Bit().data();
	options->Properties.mBackgroundImageTile = ui->imageTileCheckBox->isChecked();
	options->Properties.mFogEnabled = ui->fogCheckBox->isChecked();
	options->Properties.mFogDensity = ui->fogDensityEdit->text().toFloat();
	options->SetDefault = ui->setDefaultCheckBox->isChecked();

	QDialog::accept();
}

void lcQPropertiesDialog::colorClicked()
{
	QObject *button = sender();
	QString title;
	lcuint32 *color = NULL;

	if (button == ui->solidColorButton)
	{
		color = &options->Properties.mBackgroundSolidColor;
		title = tr("Select Background Color");
	}
	else if (button == ui->gradient1ColorButton)
	{
		color = &options->Properties.mBackgroundGradientColor1;
		title = tr("Select Background Top Color");
	}
	else if (button == ui->gradient2ColorButton)
	{
		color = &options->Properties.mBackgroundGradientColor2;
		title = tr("Select Background Bottom Color");
	}
	else if (button == ui->fogColorButton)
	{
		color = &options->Properties.mFogColor;
		title = tr("Select Fog Color");
	}
	else if (button == ui->ambientColorButton)
	{
		color = &options->Properties.mAmbientColor;
		title = tr("Select Ambient Light Color");
	}

	QColor oldColor = QColor(LC_RGBA_RED(*color), LC_RGBA_GREEN(*color), LC_RGBA_BLUE(*color));
	QColor newColor = QColorDialog::getColor(oldColor, this, title);

	if (newColor == oldColor || !newColor.isValid())
		return;

	*color = LC_RGB(newColor.red(), newColor.green(), newColor.blue());

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
