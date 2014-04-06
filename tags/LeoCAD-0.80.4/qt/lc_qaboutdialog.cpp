#include "lc_global.h"
#include "lc_qaboutdialog.h"
#include "ui_lc_qaboutdialog.h"

lcQAboutDialog::lcQAboutDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQAboutDialog)
{
	ui->setupUi(this);

	options = (char*)data;

	ui->version->setText(QString(tr("LeoCAD Version ")) + LC_VERSION_TEXT);
	ui->info->setText(options);
}

lcQAboutDialog::~lcQAboutDialog()
{
	delete ui;
}
