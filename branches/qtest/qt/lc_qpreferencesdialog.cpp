#include "lc_qpreferencesdialog.h"
#include "ui_lc_qpreferencesdialog.h"

lcQPreferencesDialog::lcQPreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lcQPreferencesDialog)
{
    ui->setupUi(this);
}

lcQPreferencesDialog::~lcQPreferencesDialog()
{
    delete ui;
}
