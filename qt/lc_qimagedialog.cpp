#include "lc_global.h"
#include "lc_qimagedialog.h"
#include "ui_lc_qimagedialog.h"
#include "project.h"

lcImageDialog::lcImageDialog(QWidget* Parent, lcImageDialogOptions* Options)
    : QDialog(Parent), mOptions(Options), ui(new Ui::lcImageDialog)
{
	ui->setupUi(this);

	connect(ui->fileNameBrowse, &QPushButton::clicked, this, &lcImageDialog::FileNameBrowseClicked);

	ui->width->setValidator(new QIntValidator(1, 32768, this));
	ui->height->setValidator(new QIntValidator(1, 32768, this));
	ui->firstStep->setValidator(new QIntValidator(this));
	ui->lastStep->setValidator(new QIntValidator(this));

	ui->fileName->setText(mOptions->FilePath);
	ui->width->setText(QString::number(mOptions->Width));
	ui->height->setText(QString::number(mOptions->Height));
	ui->firstStep->setText(QString::number(mOptions->Start));
	ui->lastStep->setText(QString::number(mOptions->End));
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
		QMessageBox::information(this, tr("Error"), tr("Output File cannot be empty."));
		return;
	}

	int Width = ui->width->text().toInt();

	if (Width < 1 || Width > 32768)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a width between 1 and 32768."));
		return;
	}

	int Height = ui->height->text().toInt();

	if (Height < 1 || Height > 32768)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a height between 1 and 32768."));
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
		Start = ui->firstStep->text().toInt();

		if (Start < 1 || Start > mOptions->End)
		{
			QMessageBox::information(this, tr("Error"), tr("First step must be between 1 and %1.").arg(QString::number(mOptions->End)));
			return;
		}

		End = ui->lastStep->text().toInt();

		if (End < 1 || End > mOptions->End)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be between 1 and %1.").arg(QString::number(mOptions->End)));
			return;
		}

		if (End < Start)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be greater than first step."));
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
