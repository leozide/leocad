#include "lc_global.h"
#include "lc_selectbycolordialog.h"
#include "lc_qcolorpicker.h"

lcSelectByColorDialog::lcSelectByColorDialog(QWidget* Parent, int ColorIndex)
	: QDialog(Parent), mColorIndex(ColorIndex)
{
	setWindowTitle(tr("Select By Color"));

	QVBoxLayout* MainLayout = new QVBoxLayout(this);
	
	QHBoxLayout* ColorLayout = new QHBoxLayout();
	ColorLayout->setContentsMargins(0, 0, 0, 0);

	QLabel* ColorLabel = new QLabel(tr("Color:"), this);
	ColorLayout->addWidget(ColorLabel);

	mColorPicker = new lcQColorPicker(this);
	mColorPicker->setCurrentColor(mColorIndex);
	ColorLayout->addWidget(mColorPicker);

	MainLayout->addLayout(ColorLayout);
	
	QDialogButtonBox* ButtonBox = new QDialogButtonBox(this);
	ButtonBox->setOrientation(Qt::Horizontal);
	ButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	MainLayout->addWidget(ButtonBox);

	QObject::connect(ButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(ButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

lcSelectByColorDialog::~lcSelectByColorDialog()
{
}

void lcSelectByColorDialog::accept()
{
	mColorIndex = mColorPicker->currentColor();

	QDialog::accept();
}
