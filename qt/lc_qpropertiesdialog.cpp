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

	setWindowTitle(QString(tr("%1 Properties")).arg(options->Title));

	ui->descriptionEdit->setText(options->Description);
	ui->authorEdit->setText(options->Author);
	ui->commentsEdit->setText(options->Comments);

	if (options->BackgroundType == 2)
		ui->imageRadio->setChecked(true);
	else if (options->BackgroundType == 1)
		ui->gradientRadio->setChecked(true);
	else
		ui->solidRadio->setChecked(true);

	ui->imageNameEdit->setText(options->BackgroundFileName);
	ui->imageTileCheckBox->setChecked(options->BackgroundTile);
	ui->fogCheckBox->setChecked(options->FogEnabled);
	ui->fogDensityEdit->setText(QString::number(options->FogDensity));
	ui->floorCheckBox->setChecked(options->DrawFloor);

	QPixmap pix(12, 12);

	pix.fill(QColor(options->SolidColor[0] * 255, options->SolidColor[1] * 255, options->SolidColor[2] * 255));
	ui->solidColorButton->setIcon(pix);
	pix.fill(QColor(options->GradientColor1[0] * 255, options->GradientColor1[1] * 255, options->GradientColor1[2] * 255));
	ui->gradient1ColorButton->setIcon(pix);
	pix.fill(QColor(options->GradientColor2[0] * 255, options->GradientColor2[1] * 255, options->GradientColor2[2] * 255));
	ui->gradient2ColorButton->setIcon(pix);
	pix.fill(QColor(options->FogColor[0] * 255, options->FogColor[1] * 255, options->FogColor[2] * 255));
	ui->fogColorButton->setIcon(pix);
	pix.fill(QColor(options->AmbientColor[0] * 255, options->AmbientColor[1] * 255, options->AmbientColor[2] * 255));
	ui->ambientColorButton->setIcon(pix);

	lcPiecesLibrary *library = lcGetPiecesLibrary();
	lcArray<lcPiecesUsedEntry>& partsUsed = options->PartsUsed;
	QStringList horizontalLabels, verticalLabels;

	int *colorColumns = new int[gNumUserColors], numColors = 0;
	memset(colorColumns, 0, sizeof(int) * gNumUserColors);

	int *infoRows = new int[library->mPieces.GetSize()], numInfos = 0;
	memset(infoRows, 0, sizeof(int) * library->mPieces.GetSize());

	for (int partIdx = 0; partIdx < partsUsed.GetSize(); partIdx++)
	{
		int colorIndex = partsUsed[partIdx].ColorIndex;
		if (!colorColumns[colorIndex])
		{
			colorColumns[colorIndex] = numColors++;
			horizontalLabels.append(gColorList[colorIndex].Name);
		}

		int infoIndex = library->mPieces.FindIndex(partsUsed[partIdx].Info);
		if (!infoRows[infoIndex])
		{
			infoRows[infoIndex] = numInfos++;
			verticalLabels.append(partsUsed[partIdx].Info->m_strDescription);
		}
	}

	QTableWidget *table = ui->partsTable;
	table->setColumnCount(numColors);
	table->setRowCount(numInfos);
	table->setHorizontalHeaderLabels(horizontalLabels);
	table->setVerticalHeaderLabels(verticalLabels);

	for (int partIdx = 0; partIdx < partsUsed.GetSize(); partIdx++)
	{
		int colorIndex = partsUsed[partIdx].ColorIndex;
		int infoIndex = library->mPieces.FindIndex(partsUsed[partIdx].Info);

		table->setItem(infoRows[infoIndex], colorColumns[colorIndex], new QTableWidgetItem(QString::number(partsUsed[partIdx].Count)));
	}

	delete[] colorColumns;
	delete[] infoRows;
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

void lcQPropertiesDialog::accept()
{
	strcpy(options->Description, ui->descriptionEdit->text().toLocal8Bit().data());
	strcpy(options->Author, ui->authorEdit->text().toLocal8Bit().data());
	strcpy(options->Comments, ui->commentsEdit->toPlainText().toLocal8Bit().data());

	if (ui->imageRadio->isChecked())
		 options->BackgroundType = 2;
	else if (ui->gradientRadio->isChecked())
		 options->BackgroundType = 1;
	else
		 options->BackgroundType = 0;

	strcpy(options->BackgroundFileName, ui->imageNameEdit->text().toLocal8Bit().data());
	options->BackgroundTile = ui->imageTileCheckBox->isChecked();
	options->FogEnabled = ui->fogCheckBox->isChecked();
	options->FogDensity = ui->fogDensityEdit->text().toFloat();
	options->DrawFloor = ui->floorCheckBox->isChecked();
	options->SetDefault = ui->setDefaultCheckBox->isChecked();

	QDialog::accept();
}

void lcQPropertiesDialog::colorClicked()
{
	QObject *button = sender();
	QString title;
	float *color = NULL;

	if (button == ui->solidColorButton)
	{
		color = options->SolidColor;
		title = tr("Select Background Color");
	}
	else if (button == ui->gradient1ColorButton)
	{
		color = options->GradientColor1;
		title = tr("Select Background Top Color");
	}
	else if (button == ui->gradient2ColorButton)
	{
		color = options->GradientColor2;
		title = tr("Select Background Bottom Color");
	}
	else if (button == ui->fogColorButton)
	{
		color = options->FogColor;
		title = tr("Select Fog Color");
	}
	else if (button == ui->ambientColorButton)
	{
		color = options->AmbientColor;
		title = tr("Select Ambient Light Color");
	}

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
