#include "lc_global.h"
#include "lc_qarraydialog.h"
#include "ui_lc_qarraydialog.h"
#include "lc_basewindow.h"
#include "lc_qutils.h"

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
	ui->offset1x->setText(lcFormatValueLocalized(options->Offsets[0].x));
	ui->offset1y->setText(lcFormatValueLocalized(options->Offsets[0].y));
	ui->offset1z->setText(lcFormatValueLocalized(options->Offsets[0].z));
	ui->offset2x->setText(lcFormatValueLocalized(options->Offsets[1].x));
	ui->offset2y->setText(lcFormatValueLocalized(options->Offsets[1].y));
	ui->offset2z->setText(lcFormatValueLocalized(options->Offsets[1].z));
	ui->offset3x->setText(lcFormatValueLocalized(options->Offsets[2].x));
	ui->offset3y->setText(lcFormatValueLocalized(options->Offsets[2].y));
	ui->offset3z->setText(lcFormatValueLocalized(options->Offsets[2].z));
	ui->rotation1x->setText(lcFormatValueLocalized(options->Rotations[0].x));
	ui->rotation1y->setText(lcFormatValueLocalized(options->Rotations[0].y));
	ui->rotation1z->setText(lcFormatValueLocalized(options->Rotations[0].z));
	ui->rotation2x->setText(lcFormatValueLocalized(options->Rotations[1].x));
	ui->rotation2y->setText(lcFormatValueLocalized(options->Rotations[1].y));
	ui->rotation2z->setText(lcFormatValueLocalized(options->Rotations[1].z));
	ui->rotation3x->setText(lcFormatValueLocalized(options->Rotations[2].x));
	ui->rotation3y->setText(lcFormatValueLocalized(options->Rotations[2].y));
	ui->rotation3z->setText(lcFormatValueLocalized(options->Rotations[2].z));
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
	options->Offsets[0].x = lcParseValueLocalized(ui->offset1x->text());
	options->Offsets[0].y = lcParseValueLocalized(ui->offset1y->text());
	options->Offsets[0].z = lcParseValueLocalized(ui->offset1z->text());
	options->Offsets[1].x = lcParseValueLocalized(ui->offset2x->text());
	options->Offsets[1].y = lcParseValueLocalized(ui->offset2y->text());
	options->Offsets[1].z = lcParseValueLocalized(ui->offset2z->text());
	options->Offsets[2].x = lcParseValueLocalized(ui->offset3x->text());
	options->Offsets[2].y = lcParseValueLocalized(ui->offset3y->text());
	options->Offsets[2].z = lcParseValueLocalized(ui->offset3z->text());
	options->Rotations[0].x = lcParseValueLocalized(ui->rotation1x->text());
	options->Rotations[0].y = lcParseValueLocalized(ui->rotation1y->text());
	options->Rotations[0].z = lcParseValueLocalized(ui->rotation1z->text());
	options->Rotations[1].x = lcParseValueLocalized(ui->rotation2x->text());
	options->Rotations[1].y = lcParseValueLocalized(ui->rotation2y->text());
	options->Rotations[1].z = lcParseValueLocalized(ui->rotation2z->text());
	options->Rotations[2].x = lcParseValueLocalized(ui->rotation3x->text());
	options->Rotations[2].y = lcParseValueLocalized(ui->rotation3y->text());
	options->Rotations[2].z = lcParseValueLocalized(ui->rotation3z->text());

	QDialog::accept();
}
