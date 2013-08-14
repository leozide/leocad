#include "lc_global.h"
#include "lc_qimagedialog.h"
#include "ui_lc_qimagedialog.h"
#include "basewnd.h"

lcQImageDialog::lcQImageDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQImageDialog)
{
	ui->setupUi(this);

	ui->width->setValidator(new QIntValidator(1, 2048));
	ui->height->setValidator(new QIntValidator(1, 2048));
	ui->firstStep->setValidator(new QIntValidator(1, 9999));
	ui->lastStep->setValidator(new QIntValidator(1, 9999));

	options = (lcImageDialogOptions*)data;

	ui->fileName->setText(options->FileName);
	ui->format->setCurrentIndex(options->Format);
	ui->transparent->setChecked(options->Transparent);
	ui->width->setText(QString::number(options->Width));
	ui->height->setText(QString::number(options->Height));
	ui->firstStep->setText(QString::number(options->Start));
	ui->lastStep->setText(QString::number(options->End));
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
		QMessageBox::information(this, "LeoCAD", tr("Output File cannot be empty."));
		return;
	}

	int width = ui->width->text().toInt();

	if (width < 1 || width > 2048)
	{
		QMessageBox::information(this, "LeoCAD", tr("Please enter a width between 1 and 2048."));
		return;
	}

	int height = ui->height->text().toInt();

	if (height < 1 || height > 2048)
	{
		QMessageBox::information(this, "LeoCAD", tr("Please enter a height between 1 and 2048."));
		return;
	}

	int start = ui->firstStep->text().toInt();

	if (start < 1 || start > 9999)
	{
		QMessageBox::information(this, "LeoCAD", tr("First step must be between 1 and 9999."));
		return;
	}

	int end = ui->lastStep->text().toInt();

	if (end < 1 || end > 9999)
	{
		QMessageBox::information(this, "LeoCAD", tr("Last step must be between 1 and 9999."));
		return;
	}

	if (end < start)
	{
		QMessageBox::information(this, "LeoCAD", tr("Last step must be greater than first step."));
		return;
	}

	strcpy(options->FileName, fileName.toLocal8Bit().data());
	options->Format = (LC_IMAGE_FORMAT)ui->format->currentIndex();
	options->Transparent = ui->transparent->isChecked();
	options->Width = width;
	options->Height = height;
	options->Start = start;
	options->End = end;

	QDialog::accept();
}

void lcQImageDialog::on_fileNameBrowse_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Save Image File"), ui->fileName->text(), tr("Supported Files (*.bmp *.png *.jpg);;BMP Files (*.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->fileName->setText(QDir::toNativeSeparators(result));
}

void lcQImageDialog::on_format_currentIndexChanged(int index)
{
	QString fileName = ui->fileName->text();
	QString extension = QFileInfo(fileName).suffix().toLower();

	QString newExtension;
	switch (index)
	{
	case LC_IMAGE_BMP: newExtension = "bmp";
		break;
	case LC_IMAGE_JPG: newExtension = "jpg";
		break;
	default:
	case LC_IMAGE_PNG: newExtension = "png";
		break;
	}

	if (extension == newExtension)
		return;

	if (extension == "bmp" || extension == "png" || extension == "jpg" || extension == "jpeg")
		fileName = fileName.left(fileName.length() - extension.length()) + newExtension;

	ui->fileName->setText(fileName);
}
