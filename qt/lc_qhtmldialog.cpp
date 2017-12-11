#include "lc_global.h"
#include "lc_qhtmldialog.h"
#include "ui_lc_qhtmldialog.h"
#include "project.h"

lcQHTMLDialog::lcQHTMLDialog(QWidget* Parent, lcHTMLExportOptions* Options)
	: QDialog(Parent), ui(new Ui::lcQHTMLDialog)
{
    ui->setupUi(this);

	ui->stepWidth->setValidator(new QIntValidator(0, 2048, ui->stepWidth));
	ui->stepHeight->setValidator(new QIntValidator(0, 2048, ui->stepHeight));
	ui->partImagesWidth->setValidator(new QIntValidator(0, 2048, ui->partImagesWidth));
	ui->partImagesHeight->setValidator(new QIntValidator(0, 2048, ui->partImagesHeight));

	mOptions = Options;

	ui->outputFolder->setText(QDir::toNativeSeparators(mOptions->PathName));

	if (mOptions->CurrentOnly)
		ui->currentModelOnly->setChecked(true);
	else if (mOptions->SubModels)
		ui->currentModelSubmodels->setChecked(true);
	else
		ui->allModels->setChecked(true);

	ui->transparentImages->setChecked(mOptions->TransparentImages);
	ui->singlePage->setChecked(mOptions->SinglePage);
	ui->oneStepPerPage->setChecked(!mOptions->SinglePage);
	ui->indexPage->setChecked(mOptions->SinglePage);
	ui->stepWidth->setText(QString::number(mOptions->StepImagesWidth));
	ui->stepHeight->setText(QString::number(mOptions->StepImagesHeight));
	ui->highlightNewParts->setChecked(mOptions->HighlightNewParts);
	ui->partsAfterEachStep->setChecked(mOptions->PartsListStep);
	ui->partsAtTheEnd->setChecked(mOptions->PartsListEnd);
	ui->partImages->setChecked(mOptions->PartsListImages);
	ui->partColor->setCurrentColor(mOptions->PartImagesColor);
	ui->partImagesWidth->setText(QString::number(mOptions->PartImagesWidth));
	ui->partImagesHeight->setText(QString::number(mOptions->PartImagesHeight));
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

	mOptions->PathName = pathName;
	mOptions->SubModels = ui->currentModelSubmodels->isChecked();
	mOptions->CurrentOnly = ui->currentModelOnly->isChecked();
	mOptions->TransparentImages = ui->transparentImages->isChecked();
	mOptions->SinglePage = ui->singlePage->isChecked();
	mOptions->IndexPage = ui->indexPage->isChecked();
	mOptions->StepImagesWidth = ui->stepWidth->text().toInt();
	mOptions->StepImagesHeight = ui->stepHeight->text().toInt();
	mOptions->HighlightNewParts = ui->highlightNewParts->isChecked();
	mOptions->PartsListStep = ui->partsAfterEachStep->isChecked();
	mOptions->PartsListEnd = ui->partsAtTheEnd->isChecked();
	mOptions->PartsListImages = ui->partImages->isChecked();
	mOptions->PartImagesColor = ui->partColor->currentColor();
	mOptions->PartImagesWidth = ui->partImagesWidth->text().toInt();
	mOptions->PartImagesHeight = ui->partImagesHeight->text().toInt();

	QDialog::accept();
}

void lcQHTMLDialog::on_outputFolderBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), ui->outputFolder->text());

	if (!result.isEmpty())
		ui->outputFolder->setText(QDir::toNativeSeparators(result));
}
