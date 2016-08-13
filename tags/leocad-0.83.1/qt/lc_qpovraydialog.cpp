#include "lc_global.h"
#include "lc_qpovraydialog.h"
#include "ui_lc_qpovraydialog.h"
#include "lc_profile.h"

lcQPOVRayDialog::lcQPOVRayDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcQPOVRayDialog)
{
	ui->setupUi(this);

	mPOVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	mLGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	mRender = lcGetProfileInt(LC_PROFILE_POVRAY_RENDER);

	ui->outputEdit->setText(mFileName);
	ui->povrayEdit->setText(mPOVRayPath);
	ui->lgeoEdit->setText(mLGEOPath);
	ui->render->setChecked(mRender);
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

	mFileName = fileName;
	mPOVRayPath = ui->povrayEdit->text();
	mLGEOPath = ui->lgeoEdit->text();
	mRender = ui->render->isChecked();

	lcSetProfileString(LC_PROFILE_POVRAY_PATH, mPOVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, mLGEOPath);
	lcSetProfileInt(LC_PROFILE_POVRAY_RENDER, mRender);

	QDialog::accept();
}

void lcQPOVRayDialog::on_outputBrowse_clicked()
{
	QString result = QFileDialog::getSaveFileName(this, tr("Save POV-Ray File"), ui->outputEdit->text(), tr("POV-Ray Files (*.pov);;All Files (*.*)"));

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
