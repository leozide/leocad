#include "lc_global.h"
#include "lc_groupdialog.h"
#include "ui_lc_groupdialog.h"
#include "group.h"

lcGroupDialog::lcGroupDialog(QWidget* Parent, const QString& Name)
	: QDialog(Parent), ui(new Ui::lcGroupDialog)
{
	ui->setupUi(this);

	ui->name->setText(Name);
}

lcGroupDialog::~lcGroupDialog()
{
	delete ui;
}

void lcGroupDialog::accept()
{
	QString Name = ui->name->text();

	if (Name.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Name cannot be empty."));
		return;
	}

	mName = Name;

	QDialog::accept();
}
