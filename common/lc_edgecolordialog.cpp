#include "lc_edgecolordialog.h"
#include "lc_application.h"

#define MIN_GAMMA 1.0f

lcAutomateEdgeColorDialog::lcAutomateEdgeColorDialog(QWidget* Parent)
	:QDialog(Parent)
{
	mStudColor = lcGetPreferences().mStudColor;
	mStudEdgeColor = lcGetPreferences().mStudEdgeColor;
	mPartEdgeContrast = lcGetPreferences().mPartEdgeContrast;
	mPartEdgeGamma = lcGetPreferences().mPartEdgeGamma;
	mPartColorToneIndex = lcGetPreferences().mPartColorToneIndex;

	setWindowTitle(QString("Edge Color Preferences"));
	QVBoxLayout* mainLayout = new  QVBoxLayout(this);

	QGroupBox* EdgeSettingsBox = new QGroupBox("Part Edge Color Settings",this);
	mainLayout->addWidget(EdgeSettingsBox);
	QGridLayout* EdgeSettingsLayout = new QGridLayout(EdgeSettingsBox);
	EdgeSettingsBox->setLayout(EdgeSettingsLayout);

	QLabel* PartEdgeContrastLabel = new QLabel(tr("Contrast:"), this);
	PartEdgeContrast = new QLabel(this);
	PartEdgeContrastSlider = new QSlider(Qt::Horizontal, this);
	PartEdgeContrastSlider->setRange(0, 10);
	PartEdgeContrastSlider->setValue(mPartEdgeContrast * 10);
	PartEdgeContrastSlider->setToolTip(tr("Set the amount of contrast - 0.5 is midway."));
	connect(PartEdgeContrastSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	emit PartEdgeContrastSlider->valueChanged(PartEdgeContrastSlider->value());

	EdgeSettingsLayout->addWidget(PartEdgeContrastLabel,0,0);
	EdgeSettingsLayout->addWidget(PartEdgeContrastSlider,0,1);
	EdgeSettingsLayout->addWidget(PartEdgeContrast,0,2);

	QLabel* PartEdgeGammaLabel = new QLabel(tr("Brightness:"), this);
	PartEdgeGamma = new QLabel(this);
	PartEdgeGammaSlider = new QSlider(Qt::Horizontal, this);
	PartEdgeGammaSlider->setRange(0, 20);
	PartEdgeGammaSlider->setValue(qRound((mPartEdgeGamma - MIN_GAMMA) * 10));
	PartEdgeGammaSlider->setToolTip(tr("Set the brightness (gamma) - the normal range is 1.8 to 2.8."));
	connect(PartEdgeGammaSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	emit PartEdgeGammaSlider->valueChanged(PartEdgeGammaSlider->value());

	EdgeSettingsLayout->addWidget(PartEdgeGammaLabel,1,0);
	EdgeSettingsLayout->addWidget(PartEdgeGammaSlider,1,1);
	EdgeSettingsLayout->addWidget(PartEdgeGamma,1,2);

	QLabel* PartColorToneIndexLabel = new QLabel(tr("Tone Index:"), this);
	PartColorToneIndex = new QLabel(this);
	PartColorToneIndexSlider = new QSlider(Qt::Horizontal, this);
	PartColorToneIndexSlider->setRange(0, 10);
	PartColorToneIndexSlider->setValue(mPartColorToneIndex * 10);
	PartColorToneIndexSlider->setToolTip(tr("Set to classify where colors are either light or dark - e.g. Dark Bluish Gray (72) is classified as a light color at 0.4."));
	connect(PartColorToneIndexSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	emit PartColorToneIndexSlider->valueChanged(PartColorToneIndexSlider->value());

	EdgeSettingsLayout->addWidget(PartColorToneIndexLabel,2,0);
	EdgeSettingsLayout->addWidget(PartColorToneIndexSlider,2,1);
	EdgeSettingsLayout->addWidget(PartColorToneIndex,2,2);

	QGroupBox* StudColorBox = new QGroupBox("High Contrast Style", this);
	mainLayout->addWidget(StudColorBox);
	QGridLayout* StudColorLayout = new QGridLayout(StudColorBox);
	StudColorBox->setLayout(StudColorLayout);

	auto SetButtonPixmap = [](quint32 Color, QToolButton* Button)
	{
		QPixmap Pixmap(12, 12);
		QColor ButtonColor(QColor(LC_RGBA_RED(Color), LC_RGBA_GREEN(Color), LC_RGBA_BLUE(Color)));
		Pixmap.fill(ButtonColor);
		Button->setIcon(Pixmap);
		Button->setToolTip(ButtonColor.name().toUpper());
	};

	QLabel* StudColorLabel = new QLabel(tr("Stud Color:"), this);
	StudColorButton = new QToolButton(this);
	SetButtonPixmap(mStudColor, StudColorButton);
	connect(StudColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetStudColorButton = new QToolButton(this);
	ResetStudColorButton->setText(tr("..."));
	ResetStudColorButton->setToolTip(tr("Reset"));
	connect(ResetStudColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	StudColorLayout->addWidget(StudColorLabel,0,0);
	StudColorLayout->addWidget(StudColorButton,0,1);
	StudColorLayout->addWidget(ResetStudColorButton,0,2);

	QLabel* StudEdgeColorLabel = new QLabel(tr("Stud Edge Color:"), this);
	StudEdgeColorButton = new QToolButton(this);
	SetButtonPixmap(mStudEdgeColor, StudEdgeColorButton);
	connect(StudEdgeColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetStudEdgeColorButton = new QToolButton(this);
	ResetStudEdgeColorButton->setText(tr("..."));
	ResetStudEdgeColorButton->setToolTip(tr("Reset"));
	connect(ResetStudEdgeColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	StudColorLayout->addWidget(StudEdgeColorLabel,1,0);
	StudColorLayout->addWidget(StudEdgeColorButton,1,1);
	StudColorLayout->addWidget(ResetStudEdgeColorButton,1,2);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
									 Qt::Horizontal, this);
	mainLayout->addWidget(buttonBox);
	QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	setMinimumSize(220,100);
}

void lcAutomateEdgeColorDialog::SliderValueChanged(int Value)
{
	float Result;
	if (sender() == PartEdgeContrastSlider)
	{
		Result = Value * 0.1f;
		PartEdgeContrast->setText(QString::number(Result));
	}
	else if (sender() == PartEdgeGammaSlider)
	{
		Result = (Value * 0.1f) + MIN_GAMMA;
		PartEdgeGamma->setText(QString::number(Result));
	}
	else if (sender() == PartColorToneIndexSlider)
	{
		Result = Value * 0.1f;
		PartColorToneIndex->setText(QString::number(Result));
	}
}

void lcAutomateEdgeColorDialog::ColorButtonClicked()
{
	QObject* Button = sender();
	QString Title;
	quint32* Color = nullptr;
	QColorDialog::ColorDialogOptions DialogOptions;

	if (Button == StudColorButton)
	{
		Title = tr("Select Stud Color");
		Color = &mStudColor;
	}
	else if (Button == StudEdgeColorButton)
	{
		Title = tr("Select Stud Edge Color");
		Color = &mStudEdgeColor;
	}
	else
		return;

	QColor oldColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
	QColor NewColor = QColorDialog::getColor(oldColor, this, Title, DialogOptions);

	if (NewColor == oldColor || !NewColor.isValid())
		return;

	*Color = LC_RGBA(NewColor.red(), NewColor.green(), NewColor.blue(), NewColor.alpha());

	QPixmap pix(12, 12);

	NewColor.setAlpha(255);
	pix.fill(NewColor);
	((QToolButton*)Button)->setIcon(pix);
	((QToolButton*)Button)->setToolTip(NewColor.name().toUpper());
}

void lcAutomateEdgeColorDialog::ResetColorButtonClicked()
{
	quint32* Color = nullptr;
	QPixmap pix(12, 12);
	QColor ResetColor;

	if (sender() == StudColorButton)
	{
	   *Color = LC_RGBA(5, 19, 29, 128);
		if (mStudColor == *Color)
			return;
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		pix.fill(ResetColor);
		StudColorButton->setIcon(pix);
		StudColorButton->setToolTip(ResetColor.name().toUpper());
	}
	else if (sender() == StudEdgeColorButton)
	{
	   *Color = LC_RGBA(255, 255, 255, 255);
		if (mStudEdgeColor == *Color)
			return;
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		pix.fill(ResetColor);
		StudEdgeColorButton->setIcon(pix);
		StudEdgeColorButton->setToolTip(ResetColor.name().toUpper());
	}
}
