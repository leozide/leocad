#include "lc_global.h"
#include "lc_imagedialog.h"
#include "ui_lc_imagedialog.h"
#include "project.h"
#include "lc_qutils.h"

lcImageDialog::lcImageDialog(QWidget* Parent, lcImageDialogOptions* Options)
    : QDialog(Parent), mOptions(Options), ui(new Ui::lcImageDialog)
{
	ui->setupUi(this);
	lcSetupDialogHelp(this, ui->buttonBox);

	connect(ui->fileNameBrowse, &QPushButton::clicked, this, &lcImageDialog::FileNameBrowseClicked);

	ui->width->setValidator(new QIntValidator(1, LC_MAX_IMAGE_DIMENSION, this));
	ui->height->setValidator(new QIntValidator(1, LC_MAX_IMAGE_DIMENSION, this));
	ui->firstStep->setValidator(new QIntValidator(1, mOptions->End, this));
	ui->lastStep->setValidator(new QIntValidator(1, mOptions->End, this));

	ui->fileName->setText(mOptions->FilePath);
	ui->width->setText(lcFormatIntegerLocalized(mOptions->Width, ui->width->validator()->locale()));
	ui->height->setText(lcFormatIntegerLocalized(mOptions->Height, ui->height->validator()->locale()));
	ui->firstStep->setText(lcFormatIntegerLocalized(mOptions->Start, ui->firstStep->validator()->locale()));
	ui->lastStep->setText(lcFormatIntegerLocalized(mOptions->End, ui->lastStep->validator()->locale()));
	ui->rangeCurrent->setChecked(true);
}

lcImageDialog::~lcImageDialog()
{
	delete ui;
}

void lcImageDialog::accept()
{
	QString FilePath = ui->fileName->text();

	if (FilePath.isEmpty())
	{
		QMessageBox::information(this, windowTitle(), tr("Output file cannot be empty."));
		return;
	}

	bool Ok;
	int Width = ui->width->validator()->locale().toInt(ui->width->text(), &Ok);

	if (!Ok || Width < 1 || Width > LC_MAX_IMAGE_DIMENSION)
	{
		QMessageBox::information(this, windowTitle(), tr("Please enter a width between 1 and 32768."));
		return;
	}

	int Height = ui->height->validator()->locale().toInt(ui->height->text(), &Ok);

	if (!Ok || Height < 1 || Height > LC_MAX_IMAGE_DIMENSION)
	{
		QMessageBox::information(this, windowTitle(), tr("Please enter a height between 1 and 32768."));
		return;
	}

	int Start = mOptions->Start, End = mOptions->Start;

	if (ui->rangeAll->isChecked())
	{
		Start = 1;
		End = mOptions->End;
	}
	else if (ui->rangeCurrent->isChecked())
	{
		Start = mOptions->Start;
		End = mOptions->Start;
	}
	else if (ui->rangeCustom->isChecked())
	{
		Start = ui->firstStep->validator()->locale().toInt(ui->firstStep->text(), &Ok);

		if (!Ok || Start < 1 || Start > mOptions->End)
		{
			QMessageBox::information(this, windowTitle(), tr("First step must be between 1 and %1.").arg(QString::number(mOptions->End)));
			return;
		}

		End = ui->lastStep->validator()->locale().toInt(ui->lastStep->text(), &Ok);

		if (!Ok || End < 1 || End > mOptions->End)
		{
			QMessageBox::information(this, windowTitle(), tr("Last step must be between 1 and %1.").arg(QString::number(mOptions->End)));
			return;
		}

		if (End < Start)
		{
			QMessageBox::information(this, windowTitle(), tr("Last step must be greater than first step."));
			return;
		}
	}

	mOptions->FilePath = FilePath;
	mOptions->Width = Width;
	mOptions->Height = Height;
	mOptions->Start = Start;
	mOptions->End = End;

	QDialog::accept();
}

void lcImageDialog::FileNameBrowseClicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Save Image File"), ui->fileName->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->fileName->setText(QDir::toNativeSeparators(result));
}
