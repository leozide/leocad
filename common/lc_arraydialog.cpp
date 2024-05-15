#include "lc_global.h"
#include "lc_arraydialog.h"
#include "ui_lc_arraydialog.h"
#include "lc_qutils.h"

lcArrayDialog::lcArrayDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcArrayDialog)
{
	ui->setupUi(this);

	ui->offset1x->setValidator(new QDoubleValidator(ui->offset1x));
	ui->offset1y->setValidator(new QDoubleValidator(ui->offset1y));
	ui->offset1z->setValidator(new QDoubleValidator(ui->offset1z));
	ui->offset2x->setValidator(new QDoubleValidator(ui->offset2x));
	ui->offset2y->setValidator(new QDoubleValidator(ui->offset2y));
	ui->offset2z->setValidator(new QDoubleValidator(ui->offset2z));
	ui->offset3x->setValidator(new QDoubleValidator(ui->offset3x));
	ui->offset3y->setValidator(new QDoubleValidator(ui->offset3y));
	ui->offset3z->setValidator(new QDoubleValidator(ui->offset3z));
	ui->rotation1x->setValidator(new QDoubleValidator(ui->rotation1x));
	ui->rotation1y->setValidator(new QDoubleValidator(ui->rotation1y));
	ui->rotation1z->setValidator(new QDoubleValidator(ui->rotation1z));
	ui->rotation2x->setValidator(new QDoubleValidator(ui->rotation2x));
	ui->rotation2y->setValidator(new QDoubleValidator(ui->rotation2y));
	ui->rotation2z->setValidator(new QDoubleValidator(ui->rotation2z));
	ui->rotation3x->setValidator(new QDoubleValidator(ui->rotation3x));
	ui->rotation3y->setValidator(new QDoubleValidator(ui->rotation3y));
	ui->rotation3z->setValidator(new QDoubleValidator(ui->rotation3z));

	mCounts[0] = 10;
	mCounts[1] = 1;
	mCounts[2] = 1;
	mOffsets[0] = mOffsets[1] = mOffsets[2] = lcVector3(0.0f, 0.0f, 0.0f);
	mRotations[0] = mRotations[1] = mRotations[2] = lcVector3(0.0f, 0.0f, 0.0f);

	ui->count1->setValue(mCounts[0]);
	ui->count2->setValue(mCounts[1]);
	ui->count3->setValue(mCounts[2]);
	ui->offset1x->setText(lcFormatValueLocalized(mOffsets[0].x));
	ui->offset1y->setText(lcFormatValueLocalized(mOffsets[0].y));
	ui->offset1z->setText(lcFormatValueLocalized(mOffsets[0].z));
	ui->offset2x->setText(lcFormatValueLocalized(mOffsets[1].x));
	ui->offset2y->setText(lcFormatValueLocalized(mOffsets[1].y));
	ui->offset2z->setText(lcFormatValueLocalized(mOffsets[1].z));
	ui->offset3x->setText(lcFormatValueLocalized(mOffsets[2].x));
	ui->offset3y->setText(lcFormatValueLocalized(mOffsets[2].y));
	ui->offset3z->setText(lcFormatValueLocalized(mOffsets[2].z));
	ui->rotation1x->setText(lcFormatValueLocalized(mRotations[0].x));
	ui->rotation1y->setText(lcFormatValueLocalized(mRotations[0].y));
	ui->rotation1z->setText(lcFormatValueLocalized(mRotations[0].z));
	ui->rotation2x->setText(lcFormatValueLocalized(mRotations[1].x));
	ui->rotation2y->setText(lcFormatValueLocalized(mRotations[1].y));
	ui->rotation2z->setText(lcFormatValueLocalized(mRotations[1].z));
	ui->rotation3x->setText(lcFormatValueLocalized(mRotations[2].x));
	ui->rotation3y->setText(lcFormatValueLocalized(mRotations[2].y));
	ui->rotation3z->setText(lcFormatValueLocalized(mRotations[2].z));
}

lcArrayDialog::~lcArrayDialog()
{
	delete ui;
}

void lcArrayDialog::accept()
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

	mCounts[0] = Counts[0];
	mCounts[1] = Counts[1];
	mCounts[2] = Counts[2];
	mOffsets[0].x = lcParseValueLocalized(ui->offset1x->text());
	mOffsets[0].y = lcParseValueLocalized(ui->offset1y->text());
	mOffsets[0].z = lcParseValueLocalized(ui->offset1z->text());
	mOffsets[1].x = lcParseValueLocalized(ui->offset2x->text());
	mOffsets[1].y = lcParseValueLocalized(ui->offset2y->text());
	mOffsets[1].z = lcParseValueLocalized(ui->offset2z->text());
	mOffsets[2].x = lcParseValueLocalized(ui->offset3x->text());
	mOffsets[2].y = lcParseValueLocalized(ui->offset3y->text());
	mOffsets[2].z = lcParseValueLocalized(ui->offset3z->text());
	mRotations[0].x = lcParseValueLocalized(ui->rotation1x->text());
	mRotations[0].y = lcParseValueLocalized(ui->rotation1y->text());
	mRotations[0].z = lcParseValueLocalized(ui->rotation1z->text());
	mRotations[1].x = lcParseValueLocalized(ui->rotation2x->text());
	mRotations[1].y = lcParseValueLocalized(ui->rotation2y->text());
	mRotations[1].z = lcParseValueLocalized(ui->rotation2z->text());
	mRotations[2].x = lcParseValueLocalized(ui->rotation3x->text());
	mRotations[2].y = lcParseValueLocalized(ui->rotation3y->text());
	mRotations[2].z = lcParseValueLocalized(ui->rotation3z->text());

	QDialog::accept();
}
