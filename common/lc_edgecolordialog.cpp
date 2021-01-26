#include "lc_edgecolordialog.h"
#include "lc_application.h"

lcAutomateEdgeColorDialog::lcAutomateEdgeColorDialog(QWidget* Parent, bool ShowHighContrastDialog)
	:QDialog(Parent)
{
	mStudCylinderColor = lcGetPreferences().mStudCylinderColor;
	mPartEdgeColor = lcGetPreferences().mPartEdgeColor;
	mBlackEdgeColor = lcGetPreferences().mBlackEdgeColor;
	mDarkEdgeColor = lcGetPreferences().mDarkEdgeColor;

	mPartEdgeContrast = lcGetPreferences().mPartEdgeContrast;
	mPartColorValueLDIndex = lcGetPreferences().mPartColorValueLDIndex;

	setWindowTitle(tr("Color Preferences"));
	QVBoxLayout* MainLayout = new  QVBoxLayout(this);

	QGroupBox* EdgeSettingsBox = new QGroupBox(tr("Edge Colors"), this);
	EdgeSettingsBox->setVisible(!ShowHighContrastDialog);
	MainLayout->addWidget(EdgeSettingsBox);
	QGridLayout* EdgeSettingsLayout = new QGridLayout(EdgeSettingsBox);
	EdgeSettingsBox->setLayout(EdgeSettingsLayout);

	QLabel* PartEdgeContrastLabel = new QLabel(tr("Contrast:"), this);
	PartEdgeContrast = new QLabel(this);
	PartEdgeContrastSlider = new QSlider(Qt::Horizontal, this);
	PartEdgeContrastSlider->setRange(0, 100);
	PartEdgeContrastSlider->setValue(mPartEdgeContrast * 100);
	PartEdgeContrastSlider->setToolTip(tr("Set the amount of contrast - 0.50 is midway."));
	connect(PartEdgeContrastSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	emit PartEdgeContrastSlider->valueChanged(PartEdgeContrastSlider->value());

	ResetPartEdgeContrastButton = new QToolButton(this);
	ResetPartEdgeContrastButton->setText(tr("Reset"));
	connect(ResetPartEdgeContrastButton, SIGNAL(clicked()), this, SLOT(ResetSliderButtonClicked()));

	EdgeSettingsLayout->addWidget(PartEdgeContrastLabel,0,0);
	EdgeSettingsLayout->addWidget(PartEdgeContrastSlider,0,1);
	EdgeSettingsLayout->addWidget(PartEdgeContrast,0,2);
	EdgeSettingsLayout->addWidget(ResetPartEdgeContrastButton,0,3);

	QLabel* PartColorValueLDIndexLabel = new QLabel(tr("Light/Dark Value:"), this);
	PartColorValueLDIndex = new QLabel(this);
	PartColorValueLDIndexSlider = new QSlider(Qt::Horizontal, this);
	PartColorValueLDIndexSlider->setRange(0, 100);
	PartColorValueLDIndexSlider->setValue(mPartColorValueLDIndex * 100);
	PartColorValueLDIndexSlider->setToolTip(tr("Set to classify where color values are light or dark - e.g. Dark Bluish Gray (72) is light at 0.39."));
	connect(PartColorValueLDIndexSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	emit PartColorValueLDIndexSlider->valueChanged(PartColorValueLDIndexSlider->value());

	ResetPartColorValueLDIndexButton = new QToolButton(this);
	ResetPartColorValueLDIndexButton->setText(tr("Reset"));
	connect(ResetPartColorValueLDIndexButton, SIGNAL(clicked()), this, SLOT(ResetSliderButtonClicked()));

	EdgeSettingsLayout->addWidget(PartColorValueLDIndexLabel,1,0);
	EdgeSettingsLayout->addWidget(PartColorValueLDIndexSlider,1,1);
	EdgeSettingsLayout->addWidget(PartColorValueLDIndex,1,2);
	EdgeSettingsLayout->addWidget(ResetPartColorValueLDIndexButton,1,3);

	QGroupBox* HighContrastColorBox = new QGroupBox(tr("High Contrast"), this);
	HighContrastColorBox->setVisible(ShowHighContrastDialog);
	MainLayout->addWidget(HighContrastColorBox);
	QGridLayout* HighContrastColorLayout = new QGridLayout(HighContrastColorBox);
	HighContrastColorBox->setLayout(HighContrastColorLayout);

	auto SetButtonPixmap = [](quint32 Color, QToolButton* Button)
	{
		QPixmap Pixmap(12, 12);
		QColor ButtonColor(QColor(LC_RGBA_RED(Color), LC_RGBA_GREEN(Color), LC_RGBA_BLUE(Color)));
		Pixmap.fill(ButtonColor);
		Button->setIcon(Pixmap);
		Button->setToolTip(ButtonColor.name().toUpper());
	};

	QLabel* StudCylinderColorLabel = new QLabel(tr("Stud Cylinder Color:"), this);
	StudCylinderColorButton = new QToolButton(this);
	SetButtonPixmap(mStudCylinderColor, StudCylinderColorButton);
	connect(StudCylinderColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetStudCylinderColorButton = new QToolButton(this);
	ResetStudCylinderColorButton->setText(tr("Reset"));;
	connect(ResetStudCylinderColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	HighContrastColorLayout->addWidget(StudCylinderColorLabel,0,0);
	HighContrastColorLayout->addWidget(StudCylinderColorButton,0,1);
	HighContrastColorLayout->addWidget(ResetStudCylinderColorButton,0,2);

	QLabel* PartEdgeColorLabel = new QLabel(tr("Parts Edge Color:"), this);
	PartEdgeColorButton = new QToolButton(this);
	SetButtonPixmap(mPartEdgeColor, PartEdgeColorButton);
	connect(PartEdgeColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetPartEdgeColorButton = new QToolButton(this);
	ResetPartEdgeColorButton->setText(tr("Reset"));
	connect(ResetPartEdgeColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	HighContrastColorLayout->addWidget(PartEdgeColorLabel,1,0);
	HighContrastColorLayout->addWidget(PartEdgeColorButton,1,1);
	HighContrastColorLayout->addWidget(ResetPartEdgeColorButton,1,2);

	QLabel* BlackEdgeColorLabel = new QLabel(tr("Black Parts Edge Color:"), this);
	BlackEdgeColorButton = new QToolButton(this);
	SetButtonPixmap(mBlackEdgeColor, BlackEdgeColorButton);
	connect(BlackEdgeColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetBlackEdgeColorButton = new QToolButton(this);
	ResetBlackEdgeColorButton->setText(tr("Reset"));
	connect(ResetBlackEdgeColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	HighContrastColorLayout->addWidget(BlackEdgeColorLabel,2,0);
	HighContrastColorLayout->addWidget(BlackEdgeColorButton,2,1);
	HighContrastColorLayout->addWidget(ResetBlackEdgeColorButton,2,2);

	QLabel* DarkEdgeColorLabel = new QLabel(tr("Dark Parts Edge Color:"), this);
	DarkEdgeColorButton = new QToolButton(this);
	SetButtonPixmap(mDarkEdgeColor, DarkEdgeColorButton);
	connect(DarkEdgeColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ResetDarkEdgeColorButton = new QToolButton(this);
	ResetDarkEdgeColorButton->setText(tr("Reset"));
	connect(ResetDarkEdgeColorButton, SIGNAL(clicked()), this, SLOT(ResetColorButtonClicked()));

	HighContrastColorLayout->addWidget(DarkEdgeColorLabel,3,0);
	HighContrastColorLayout->addWidget(DarkEdgeColorButton,3,1);
	HighContrastColorLayout->addWidget(ResetDarkEdgeColorButton,3,2);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	MainLayout->addWidget(buttonBox);
	QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	setMinimumSize(220,100);
}

void lcAutomateEdgeColorDialog::SliderValueChanged(int Value)
{
	if (sender() == PartEdgeContrastSlider)
	{
		mPartEdgeContrast = Value * 0.01f;
		PartEdgeContrast->setText(QString::number(mPartEdgeContrast, 'f', 2));
	}
	else if (sender() == PartColorValueLDIndexSlider)
	{
		mPartColorValueLDIndex = Value * 0.01f;
		PartColorValueLDIndex->setText(QString::number(mPartColorValueLDIndex, 'f', 2));
	}
}

void lcAutomateEdgeColorDialog::ColorButtonClicked()
{
	QObject* Button = sender();
	QString Title;
	quint32* Color = nullptr;
	QColorDialog::ColorDialogOptions DialogOptions;

	if (Button == StudCylinderColorButton)
	{
		Title = tr("Select Stud Cylinder Color");
		Color = &mStudCylinderColor;
	}
	else if (Button == PartEdgeColorButton)
	{
		Title = tr("Select Part Edge Color");
		Color = &mPartEdgeColor;
	}
	else if (Button == BlackEdgeColorButton)
	{
		if (lcGetPreferences().mAutomateEdgeColor)
		{
			QMessageBox msgBox;
			msgBox.setText("Automate edge color appears to be enabled.<br>Black parts edge color will not be accessible.<br>Do you want to continue ?");
			if (msgBox.exec() != QMessageBox::Accepted)
				return;
		}
		Title = tr("Select Black Edge Color");
		Color = &mBlackEdgeColor;
	}
	else if (Button == DarkEdgeColorButton)
	{
		if (lcGetPreferences().mAutomateEdgeColor)
		{
			QMessageBox msgBox;
			msgBox.setText("Automate edge color appears to be enabled.<br>Dark parts edge color will not be accessible.<br>Do you want to continue ?");
			if (msgBox.exec() != QMessageBox::Accepted)
				return;
		}
		Title = tr("Select Dark Edge Color");
		Color = &mDarkEdgeColor;
	}
	else
		return;

	QColor OldColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
	QColor NewColor = QColorDialog::getColor(OldColor, this, Title, DialogOptions);

	if (NewColor == OldColor || !NewColor.isValid())
		return;

	*Color = LC_RGBA(NewColor.red(), NewColor.green(), NewColor.blue(), NewColor.alpha());

	QPixmap Pix(12, 12);
	NewColor.setAlpha(255);
	Pix.fill(NewColor);
	((QToolButton*)Button)->setIcon(Pix);
	((QToolButton*)Button)->setToolTip(NewColor.name().toUpper());
}

void lcAutomateEdgeColorDialog::ResetSliderButtonClicked()
{
	if (sender() == ResetPartEdgeContrastButton)
	{
		PartEdgeContrastSlider->setValue(0.5f * 100);
	}
	else if (sender() == ResetPartColorValueLDIndexButton)
	{
		PartColorValueLDIndexSlider->setValue(0.5f * 100);
	}
}

void lcAutomateEdgeColorDialog::ResetColorButtonClicked()
{
	quint32* Color = nullptr;
	QPixmap Pix(12, 12);
	QColor ResetColor;

	if (sender() == ResetStudCylinderColorButton)
	{
		Color = &mStudCylinderColor;
		*Color = LC_RGBA(27, 42, 52, 255);
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		Pix.fill(ResetColor);
		StudCylinderColorButton->setIcon(Pix);
		StudCylinderColorButton->setToolTip(ResetColor.name().toUpper());
	}
	else if (sender() == ResetPartEdgeColorButton)
	{
		Color = &mPartEdgeColor;
		*Color = LC_RGBA(0, 0, 0, 255);
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		Pix.fill(ResetColor);
		PartEdgeColorButton->setIcon(Pix);
		PartEdgeColorButton->setToolTip(ResetColor.name().toUpper());
	}
	else if (sender() == ResetBlackEdgeColorButton)
	{
		Color = &mBlackEdgeColor;
		*Color = LC_RGBA(255, 255, 255, 255);
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		Pix.fill(ResetColor);
		BlackEdgeColorButton->setIcon(Pix);
		BlackEdgeColorButton->setToolTip(ResetColor.name().toUpper());
	}
	else if (sender() == ResetDarkEdgeColorButton)
	{
		Color = &mDarkEdgeColor;
		*Color = LC_RGBA(27, 42, 52, 255);
		ResetColor = QColor(LC_RGBA_RED(*Color), LC_RGBA_GREEN(*Color), LC_RGBA_BLUE(*Color), LC_RGBA_ALPHA(*Color));
		Pix.fill(ResetColor);
		DarkEdgeColorButton->setIcon(Pix);
		DarkEdgeColorButton->setToolTip(ResetColor.name().toUpper());
	}
}
