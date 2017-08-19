#include "lc_setsdatabasedialog.h"
#include "ui_lc_setsdatabasedialog.h"

lcSetsDatabaseDialog::lcSetsDatabaseDialog(QWidget* Parent)
	: QDialog(Parent),
    ui(new Ui::lcSetsDatabaseDialog)
{
	ui->setupUi(this);
}

lcSetsDatabaseDialog::~lcSetsDatabaseDialog()
{
	delete ui;
}
