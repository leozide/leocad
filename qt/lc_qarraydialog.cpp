#include "lc_global.h"
#include "lc_qarraydialog.h"
#include "ui_lc_qarraydialog.h"
#include "basewnd.h"

lcQArrayDialog::lcQArrayDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQArrayDialog)
{
	ui->setupUi(this);

	ui->offset1x->setValidator(new QDoubleValidator());
	ui->offset1y->setValidator(new QDoubleValidator());
	ui->offset1z->setValidator(new QDoubleValidator());
	ui->offset2x->setValidator(new QDoubleValidator());
	ui->offset2y->setValidator(new QDoubleValidator());
	ui->offset2z->setValidator(new QDoubleValidator());
	ui->offset3x->setValidator(new QDoubleValidator());
	ui->offset3y->setValidator(new QDoubleValidator());
	ui->offset3z->setValidator(new QDoubleValidator());
	ui->rotation1x->setValidator(new QDoubleValidator());
	ui->rotation1y->setValidator(new QDoubleValidator());
	ui->rotation1z->setValidator(new QDoubleValidator());
	ui->rotation2x->setValidator(new QDoubleValidator());
	ui->rotation2y->setValidator(new QDoubleValidator());
	ui->rotation2z->setValidator(new QDoubleValidator());
	ui->rotation3x->setValidator(new QDoubleValidator());
	ui->rotation3y->setValidator(new QDoubleValidator());
	ui->rotation3z->setValidator(new QDoubleValidator());

	options = (lcArrayDialogOptions*)data;

	ui->count1->setValue(options->Counts[0]);
	ui->count2->setValue(options->Counts[1]);
	ui->count3->setValue(options->Counts[2]);
	ui->offset1x->setText(QString::number(options->Offsets[0].x));
	ui->offset1y->setText(QString::number(options->Offsets[0].y));
	ui->offset1z->setText(QString::number(options->Offsets[0].z));
	ui->offset2x->setText(QString::number(options->Offsets[1].x));
	ui->offset2y->setText(QString::number(options->Offsets[1].y));
	ui->offset2z->setText(QString::number(options->Offsets[1].z));
	ui->offset3x->setText(QString::number(options->Offsets[2].x));
	ui->offset3y->setText(QString::number(options->Offsets[2].y));
	ui->offset3z->setText(QString::number(options->Offsets[2].z));
	ui->rotation1x->setText(QString::number(options->Rotations[0].x));
	ui->rotation1y->setText(QString::number(options->Rotations[0].y));
	ui->rotation1z->setText(QString::number(options->Rotations[0].z));
	ui->rotation2x->setText(QString::number(options->Rotations[1].x));
	ui->rotation2y->setText(QString::number(options->Rotations[1].y));
	ui->rotation2z->setText(QString::number(options->Rotations[1].z));
	ui->rotation3x->setText(QString::number(options->Rotations[2].x));
	ui->rotation3y->setText(QString::number(options->Rotations[2].y));
	ui->rotation3z->setText(QString::number(options->Rotations[2].z));
}

lcQArrayDialog::~lcQArrayDialog()
{
	delete ui;
}

void lcQArrayDialog::accept()
{
	int Counts[3];

	Counts[0] = ui->count1->value();
	Counts[1] = ui->count2->value();
	Counts[2] = ui->count3->value();

	if (Counts[0] * Counts[1] * Counts[2] < 2)
	{
		QMessageBox::information(this, "LeoCAD", tr("Array is empty."));
		return;
	}

	options->Counts[0] = Counts[0];
	options->Counts[1] = Counts[1];
	options->Counts[2] = Counts[2];
	options->Offsets[0].x = ui->offset1x->text().toFloat();
	options->Offsets[0].y = ui->offset1y->text().toFloat();
	options->Offsets[0].z = ui->offset1z->text().toFloat();
	options->Offsets[1].x = ui->offset2x->text().toFloat();
	options->Offsets[1].y = ui->offset2y->text().toFloat();
	options->Offsets[1].z = ui->offset2z->text().toFloat();
	options->Offsets[2].x = ui->offset3x->text().toFloat();
	options->Offsets[2].y = ui->offset3y->text().toFloat();
	options->Offsets[2].z = ui->offset3z->text().toFloat();
	options->Rotations[0].x = ui->rotation1x->text().toFloat();
	options->Rotations[0].y = ui->rotation1y->text().toFloat();
	options->Rotations[0].z = ui->rotation1z->text().toFloat();
	options->Rotations[1].x = ui->rotation2x->text().toFloat();
	options->Rotations[1].y = ui->rotation2y->text().toFloat();
	options->Rotations[1].z = ui->rotation2z->text().toFloat();
	options->Rotations[2].x = ui->rotation3x->text().toFloat();
	options->Rotations[2].y = ui->rotation3y->text().toFloat();
	options->Rotations[2].z = ui->rotation3z->text().toFloat();

	QDialog::accept();
}
