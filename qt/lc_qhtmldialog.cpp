#include "lc_global.h"
#include "lc_qhtmldialog.h"
#include "ui_lc_qhtmldialog.h"
#include "lc_basewindow.h"

lcQHTMLDialog::lcQHTMLDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQHTMLDialog)
{
    ui->setupUi(this);

	ui->stepWidth->setValidator(new QIntValidator(0, 2048));
	ui->stepHeight->setValidator(new QIntValidator(0, 2048));
	ui->partImagesWidth->setValidator(new QIntValidator(0, 2048));
	ui->partImagesHeight->setValidator(new QIntValidator(0, 2048));

	options = (lcHTMLDialogOptions*)data;

	ui->outputFolder->setText(options->PathName);
	ui->imageFormat->setCurrentIndex(options->ImageFormat);
	ui->transparentImages->setChecked(options->TransparentImages);
	ui->singlePage->setChecked(options->SinglePage);
	ui->oneStepPerPage->setChecked(!options->SinglePage);
	ui->indexPage->setChecked(options->SinglePage);
	ui->stepWidth->setText(QString::number(options->StepImagesWidth));
	ui->stepHeight->setText(QString::number(options->StepImagesHeight));
	ui->highlightNewParts->setChecked(options->HighlightNewParts);
	ui->partsAfterEachStep->setChecked(options->PartsListStep);
	ui->partsAtTheEnd->setChecked(options->PartsListEnd);
	ui->partImages->setChecked(options->PartsListImages);
	ui->partColor->setCurrentColor(options->PartImagesColor);
	ui->partImagesWidth->setText(QString::number(options->PartImagesWidth));
	ui->partImagesHeight->setText(QString::number(options->PartImagesHeight));
}

lcQHTMLDialog::~lcQHTMLDialog()
{
    delete ui;
}

void lcQHTMLDialog::accept()
{
	QString pathName = ui->outputFolder->text();

	if (pathName.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Output folder cannot be empty."));
		return;
	}

	options->PathName = pathName;
	options->ImageFormat = (LC_IMAGE_FORMAT)ui->imageFormat->currentIndex();
	options->TransparentImages = ui->transparentImages->isChecked();
	options->SinglePage = ui->singlePage->isChecked();
	options->IndexPage = ui->indexPage->isChecked();
	options->StepImagesWidth = ui->stepWidth->text().toInt();
	options->StepImagesHeight = ui->stepHeight->text().toInt();
	options->HighlightNewParts = ui->highlightNewParts->isChecked();
	options->PartsListStep = ui->partsAfterEachStep->isChecked();
	options->PartsListEnd = ui->partsAtTheEnd->isChecked();
	options->PartsListImages = ui->partImages->isChecked();
	options->PartImagesColor = ui->partColor->currentColor();
	options->PartImagesWidth = ui->partImagesWidth->text().toInt();
	options->PartImagesHeight = ui->partImagesHeight->text().toInt();

	QDialog::accept();
}

void lcQHTMLDialog::on_outputFolderBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), ui->outputFolder->text());

	if (!result.isEmpty())
		ui->outputFolder->setText(QDir::toNativeSeparators(result));
}
