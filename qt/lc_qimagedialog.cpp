#include "lc_global.h"
#include "lc_qimagedialog.h"
#include "ui_lc_qimagedialog.h"
#include "lc_basewindow.h"

lcQImageDialog::lcQImageDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQImageDialog)
{
	ui->setupUi(this);

	ui->width->setValidator(new QIntValidator(1, 2048));
	ui->height->setValidator(new QIntValidator(1, 2048));
	ui->firstStep->setValidator(new QIntValidator(1, options->End));
	ui->lastStep->setValidator(new QIntValidator(1, options->End));

	options = (lcImageDialogOptions*)data;
	currentStep = options->Start;
	lastStep = options->End;

	ui->fileName->setText(options->FileName);
	ui->width->setText(QString::number(options->Width));
	ui->height->setText(QString::number(options->Height));
	ui->firstStep->setText(QString::number(1));
	ui->lastStep->setText(QString::number(lastStep));
	ui->rangeCurrent->setChecked(true);
}

lcQImageDialog::~lcQImageDialog()
{
	delete ui;
}

void lcQImageDialog::accept()
{
	QString fileName = ui->fileName->text();

	if (fileName.isEmpty())
	{
		QMessageBox::information(this, tr("Error"), tr("Output File cannot be empty."));
		return;
	}

	int width = ui->width->text().toInt();

	if (width < 1 || width > 2048)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a width between 1 and 2048."));
		return;
	}

	int height = ui->height->text().toInt();

	if (height < 1 || height > 2048)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a height between 1 and 2048."));
		return;
	}

	int start, end;

	if (ui->rangeAll->isChecked())
	{
		start = 1;
		end = lastStep;
	}
	else if (ui->rangeCurrent->isChecked())
	{
		start = currentStep;
		end = currentStep;
	}
	else if (ui->rangeCustom->isChecked())
	{
		start = ui->firstStep->text().toInt();

		if (start < 1 || start > lastStep)
		{
			QMessageBox::information(this, tr("Error"), tr("First step must be between 1 and %1.").arg(QString::number(lastStep)));
			return;
		}

		end = ui->lastStep->text().toInt();

		if (end < 1 || end > lastStep)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be between 1 and %1.").arg(QString::number(lastStep)));
			return;
		}

		if (end < start)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be greater than first step."));
			return;
		}
	}

	options->FileName = fileName;
	options->Width = width;
	options->Height = height;
	options->Start = start;
	options->End = end;

	QDialog::accept();
}

void lcQImageDialog::on_fileNameBrowse_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Save Image File"), ui->fileName->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->fileName->setText(QDir::toNativeSeparators(result));
}
