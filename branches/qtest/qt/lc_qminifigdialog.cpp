#include "lc_qminifigdialog.h"
#include "ui_lc_qminifigdialog.h"

lcQMinifigDialog::lcQMinifigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lcQMinifigDialog)
{
    ui->setupUi(this);
}

lcQMinifigDialog::~lcQMinifigDialog()
{
    delete ui;
}
