#include "lc_qhtmldialog.h"
#include "ui_lc_qhtmldialog.h"

lcQHTMLDialog::lcQHTMLDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lcQHTMLDialog)
{
    ui->setupUi(this);
}

lcQHTMLDialog::~lcQHTMLDialog()
{
    delete ui;
}
