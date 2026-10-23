#include "lc_global.h"
#include "lc_arraydialog.h"
#include "ui_lc_arraydialog.h"
#include "lc_qutils.h"

lcArrayDialog::lcArrayDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcArrayDialog)
{
	ui->setupUi(this);
	lcSetupDialogHelp(this, ui->buttonBox);

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
	ui->offset1x->setText(lcFormatFloatLocalized(mOffsets[0].x, locale()));
	ui->offset1y->setText(lcFormatFloatLocalized(mOffsets[0].y, locale()));
	ui->offset1z->setText(lcFormatFloatLocalized(mOffsets[0].z, locale()));
	ui->offset2x->setText(lcFormatFloatLocalized(mOffsets[1].x, locale()));
	ui->offset2y->setText(lcFormatFloatLocalized(mOffsets[1].y, locale()));
	ui->offset2z->setText(lcFormatFloatLocalized(mOffsets[1].z, locale()));
	ui->offset3x->setText(lcFormatFloatLocalized(mOffsets[2].x, locale()));
	ui->offset3y->setText(lcFormatFloatLocalized(mOffsets[2].y, locale()));
	ui->offset3z->setText(lcFormatFloatLocalized(mOffsets[2].z, locale()));
	ui->rotation1x->setText(lcFormatFloatLocalized(mRotations[0].x, locale()));
	ui->rotation1y->setText(lcFormatFloatLocalized(mRotations[0].y, locale()));
	ui->rotation1z->setText(lcFormatFloatLocalized(mRotations[0].z, locale()));
	ui->rotation2x->setText(lcFormatFloatLocalized(mRotations[1].x, locale()));
	ui->rotation2y->setText(lcFormatFloatLocalized(mRotations[1].y, locale()));
	ui->rotation2z->setText(lcFormatFloatLocalized(mRotations[1].z, locale()));
	ui->rotation3x->setText(lcFormatFloatLocalized(mRotations[2].x, locale()));
	ui->rotation3y->setText(lcFormatFloatLocalized(mRotations[2].y, locale()));
	ui->rotation3z->setText(lcFormatFloatLocalized(mRotations[2].z, locale()));
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
		QMessageBox::information(this, windowTitle(), tr("Array is empty."));
		return;
	}

	QLineEdit* Edits[2][3][3] =
	{
		{
			{ ui->offset1x, ui->offset1y, ui->offset1z },
			{ ui->offset2x, ui->offset2y, ui->offset2z },
			{ ui->offset3x, ui->offset3y, ui->offset3z }
		},
		{
			{ ui->rotation1x, ui->rotation1y, ui->rotation1z },
			{ ui->rotation2x, ui->rotation2y, ui->rotation2z },
			{ ui->rotation3x, ui->rotation3y, ui->rotation3z }
		}
	};
	lcVector3 Values[2][3];

	for (int Type = 0; Type < 2; Type++)
		for (int Dimension = 0; Dimension < 3; Dimension++)
			for (int Axis = 0; Axis < 3; Axis++)
			{
				QLineEdit* Edit = Edits[Type][Dimension][Axis];
				const std::optional<float> Value = lcParseFloatLocalized(Edit->text(), Edit->validator()->locale());

				if (!Value || !Edit->hasAcceptableInput())
				{
					QMessageBox::information(this, windowTitle(), tr("Please enter a valid value for '%1'.").arg(Edit->accessibleName()));
					Edit->setFocus();
					return;
				}

				Values[Type][Dimension][Axis] = *Value;
			}

	for (int Dimension = 0; Dimension < 3; Dimension++)
	{
		mCounts[Dimension] = Counts[Dimension];
		mOffsets[Dimension] = Values[0][Dimension];
		mRotations[Dimension] = Values[1][Dimension];
	}

	QDialog::accept();
}
