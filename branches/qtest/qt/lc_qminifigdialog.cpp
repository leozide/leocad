#include "lc_global.h"
#include "lc_qminifigdialog.h"
#include "ui_lc_qminifigdialog.h"
#include "lc_glwidget.h"

lcQMinifigDialog::lcQMinifigDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQMinifigDialog)
{
    ui->setupUi(this);

	QGridLayout *previewLayout = new QGridLayout(ui->minifigFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	lcGLWidget *minifig = new lcGLWidget(NULL, NULL, (GLWindow*)data);
	previewLayout->addWidget(minifig);
}

lcQMinifigDialog::~lcQMinifigDialog()
{
    delete ui;
}
