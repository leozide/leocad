#include "lc_global.h"
#include "lc_qpovraydialog.h"
#include "ui_lc_qpovraydialog.h"
#include "basewnd.h"

lcQPOVRayDialog::lcQPOVRayDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQPOVRayDialog)
{
	ui->setupUi(this);

	options = (lcPOVRayDialogOptions*)data;

	ui->outputEdit->setText(options->FileName);
	ui->povrayEdit->setText(options->POVRayPath);
	ui->lgeoEdit->setText(options->LGEOPath);
	ui->render->setChecked(options->Render);
}

lcQPOVRayDialog::~lcQPOVRayDialog()
{
	delete ui;
}

void lcQPOVRayDialog::accept()
{
	QString fileName = ui->outputEdit->text();

	if (fileName.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Output File cannot be empty."));
		return;
	}

	strcpy(options->FileName, fileName.toLocal8Bit().data());

	QString povrayPath = ui->povrayEdit->text();
	strcpy(options->POVRayPath, povrayPath.toLocal8Bit().data());

	QString lgeoPath = ui->lgeoEdit->text();
	strcpy(options->LGEOPath, lgeoPath.toLocal8Bit().data());

	options->Render = ui->render->isChecked();

	QDialog::accept();
}

void lcQPOVRayDialog::on_outputBrowse_clicked()
{
	QString result = QFileDialog::getOpenFileName(this, tr("Save POV-Ray File"), ui->outputEdit->text(), tr("POV-Ray Files (*.pov);;All Files (*.*)"));

	if (!result.isEmpty())
		ui->outputEdit->setText(QDir::toNativeSeparators(result));
}

void lcQPOVRayDialog::on_povrayBrowse_clicked()
{
#ifdef Q_OS_WIN
	QString filter(tr("Executable Files (*.exe);;All Files (*.*)"));
#else
	QString filter(tr("All Files (*.*)"));
#endif

	QString result = QFileDialog::getOpenFileName(this, tr("Open POV-Ray Executable"), ui->povrayEdit->text(), filter);

	if (!result.isEmpty())
		ui->povrayEdit->setText(QDir::toNativeSeparators(result));
}

void lcQPOVRayDialog::on_lgeoBrowse_clicked()
{
	QString result = QFileDialog::getExistingDirectory(this, tr("Open LGEO Folder"), ui->lgeoEdit->text());

	if (!result.isEmpty())
		ui->lgeoEdit->setText(QDir::toNativeSeparators(result));
}
