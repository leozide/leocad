#include "lc_global.h"
#include "lc_qimagedialog.h"
#include "ui_lc_qimagedialog.h"
#include "lc_application.h"
#include "project.h"
#include "lc_model.h"
#include "lc_profile.h"

lcQImageDialog::lcQImageDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcQImageDialog)
{
	ui->setupUi(this);

	ui->width->setValidator(new QIntValidator(1, 32768, this));
	ui->height->setValidator(new QIntValidator(1, 32768, this));
	ui->firstStep->setValidator(new QIntValidator(this));
	ui->lastStep->setValidator(new QIntValidator(this));

	Project* Project = lcGetActiveProject();
	lcModel* Model = Project->GetActiveModel();
	mWidth = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	mHeight = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	mStart = Model->GetCurrentStep();
	mEnd = Model->GetLastStep();
	mFileName = Project->GetFileName();

	if (!mFileName.isEmpty())
		mFileName = QFileInfo(mFileName).completeBaseName();
	else
		mFileName = QLatin1String("image");

	mFileName += lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);

	ui->fileName->setText(mFileName);
	ui->width->setText(QString::number(mWidth));
	ui->height->setText(QString::number(mHeight));
	ui->firstStep->setText(QString::number(mStart));
	ui->lastStep->setText(QString::number(mEnd));
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

	if (width < 1 || width > 32768)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a width between 1 and 32768."));
		return;
	}

	int height = ui->height->text().toInt();

	if (height < 1 || height > 32768)
	{
		QMessageBox::information(this, tr("Error"), tr("Please enter a height between 1 and 32768."));
		return;
	}

	int start = mStart, end = mStart;

	if (ui->rangeAll->isChecked())
	{
		start = 1;
		end = mEnd;
	}
	else if (ui->rangeCurrent->isChecked())
	{
		start = mStart;
		end = mStart;
	}
	else if (ui->rangeCustom->isChecked())
	{
		start = ui->firstStep->text().toInt();

		if (start < 1 || start > mEnd)
		{
			QMessageBox::information(this, tr("Error"), tr("First step must be between 1 and %1.").arg(QString::number(mEnd)));
			return;
		}

		end = ui->lastStep->text().toInt();

		if (end < 1 || end > mEnd)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be between 1 and %1.").arg(QString::number(mEnd)));
			return;
		}

		if (end < start)
		{
			QMessageBox::information(this, tr("Error"), tr("Last step must be greater than first step."));
			return;
		}
	}

	mFileName = fileName;
	mWidth = width;
	mHeight = height;
	mStart = start;
	mEnd = end;

	lcSetProfileInt(LC_PROFILE_IMAGE_WIDTH, mWidth);
	lcSetProfileInt(LC_PROFILE_IMAGE_HEIGHT, mHeight);

	QDialog::accept();
}

void lcQImageDialog::on_fileNameBrowse_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Save Image File"), ui->fileName->text(), tr("Supported Image Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->fileName->setText(QDir::toNativeSeparators(result));
}
