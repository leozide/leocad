#include "lc_global.h"
#include "lc_qgroupdialog.h"
#include "ui_lc_qgroupdialog.h"
#include "group.h"

lcQGroupDialog::lcQGroupDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQGroupDialog)
{
	ui->setupUi(this);

	ui->name->setMaxLength(LC_MAX_GROUP_NAME);

	options = (char*)data;

	ui->name->setText(options);
}

lcQGroupDialog::~lcQGroupDialog()
{
	delete ui;
}

void lcQGroupDialog::accept()
{
	QString name = ui->name->text();

	if (name.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Name cannot be empty."));
		return;
	}

	strcpy(options, name.toLocal8Bit().data());

	QDialog::accept();
}
