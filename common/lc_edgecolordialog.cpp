#include "lc_global.h"
#include "lc_edgecolordialog.h"
#include "lc_qutils.h"
#include "lc_application.h"

lcAutomateEdgeColorDialog::lcAutomateEdgeColorDialog(QWidget* Parent, bool ShowHighContrastDialog)
	:QDialog(Parent)
{
	const lcPreferences& Preferences = lcGetPreferences();
	mStudCylinderColorEnabled = Preferences.mStudCylinderColorEnabled;
	mStudCylinderColor = Preferences.mStudCylinderColor;
	mPartEdgeColorEnabled = Preferences.mPartEdgeColorEnabled;
	mPartEdgeColor = Preferences.mPartEdgeColor;
	mBlackEdgeColorEnabled = Preferences.mBlackEdgeColorEnabled;
	mBlackEdgeColor = Preferences.mBlackEdgeColor;
	mDarkEdgeColorEnabled = Preferences.mDarkEdgeColorEnabled;
	mDarkEdgeColor = Preferences.mDarkEdgeColor;
	mPartEdgeContrast = Preferences.mPartEdgeContrast;
	mPartColorValueLDIndex = Preferences.mPartColorValueLDIndex;

	setWindowTitle(tr("Color Preferences"));
	setWhatsThis(ShowHighContrastDialog ?
		tr("<p><b>High Contrast Edge Colors</b></p><p>Choose custom stud and edge colors for high contrast stud styles. Enable an override with its checkbox, then choose its color. Light/Dark Value sets the threshold for dark-piece edge colors. Automated edge colors take precedence over the piece edge overrides.</p><p>Choose OK to return changes to Preferences, then choose OK in Preferences to keep them. Cancel discards changes in this dialog. Reset restores an individual control's default.</p>") :
		tr("<p><b>Automated Edge Colors</b></p><p>Adjust the contrast and saturation used to calculate piece edge colors. Both controls range from 0.00 to 1.00. Reset restores a control to 0.50.</p><p>Choose OK to return changes to Preferences, then choose OK in Preferences to keep them. Cancel discards changes in this dialog.</p>"));
	QVBoxLayout* MainLayout = new  QVBoxLayout(this);

	QGroupBox* EdgeSettingsBox = new QGroupBox(tr("Edge Colors"), this);
	MainLayout->addWidget(EdgeSettingsBox);
	QGridLayout* EdgeSettingsLayout = new QGridLayout(EdgeSettingsBox);
	EdgeSettingsBox->setLayout(EdgeSettingsLayout);

	int LDIndexRow = 0;
	PartEdgeContrast = nullptr;
	PartEdgeContrastSlider = nullptr;
	if (!ShowHighContrastDialog)
	{
		LDIndexRow = 1;
		QLabel* PartEdgeContrastLabel = new QLabel(tr("Contrast:"), this);
		PartEdgeContrast = new QLabel(this);
		PartEdgeContrastSlider = new QSlider(Qt::Horizontal, this);
		PartEdgeContrastSlider->setRange(0, 100);
		PartEdgeContrastSlider->setValue(mPartEdgeContrast * 100);
		PartEdgeContrastSlider->setToolTip(tr("Set edge contrast from 0.00 to 1.00 (default 0.50)."));
		PartEdgeContrastSlider->setWhatsThis(tr("<p><b>Edge Contrast</b></p><p>Adjust the contrast used by automated edge colors to distinguish edges from piece surfaces. The range is 0.00 to 1.00; Reset restores 0.50.</p>"));
		PartEdgeContrastLabel->setBuddy(PartEdgeContrastSlider);
		PartEdgeContrastLabel->setToolTip(PartEdgeContrastSlider->toolTip());
		PartEdgeContrastLabel->setWhatsThis(PartEdgeContrastSlider->whatsThis());
		connect(PartEdgeContrastSlider, &QSlider::valueChanged, this, &lcAutomateEdgeColorDialog::SliderValueChanged);
		emit PartEdgeContrastSlider->valueChanged(PartEdgeContrastSlider->value());

		ResetPartEdgeContrastButton = new QToolButton(this);
		ResetPartEdgeContrastButton->setText(tr("Reset"));
		ResetPartEdgeContrastButton->setToolTip(tr("Restore edge contrast to 0.50."));
		ResetPartEdgeContrastButton->setWhatsThis(PartEdgeContrastSlider->whatsThis());
		connect(ResetPartEdgeContrastButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetSliderButtonClicked);

		EdgeSettingsLayout->addWidget(PartEdgeContrastLabel,0,0);
		EdgeSettingsLayout->addWidget(PartEdgeContrastSlider,0,1);
		EdgeSettingsLayout->addWidget(PartEdgeContrast,0,2);
		EdgeSettingsLayout->addWidget(ResetPartEdgeContrastButton,0,3);
	}

	QLabel* PartColorValueLDIndexLabel = new QLabel(ShowHighContrastDialog ? tr("Light/Dark Value:") : tr("Saturation:"), this);
	PartColorValueLDIndex = new QLabel(this);
	PartColorValueLDIndexSlider = new QSlider(Qt::Horizontal, this);
	PartColorValueLDIndexSlider->setRange(0, 100);
	PartColorValueLDIndexSlider->setValue(mPartColorValueLDIndex * 100);
	PartColorValueLDIndexSlider->setToolTip(ShowHighContrastDialog ?
		tr("Set the light/dark threshold from 0.00 to 1.00 (default 0.50).") :
		tr("Set edge-color saturation from 0.00 to 1.00 (default 0.50)."));
	PartColorValueLDIndexSlider->setWhatsThis(ShowHighContrastDialog ?
		tr("<p><b>Light/Dark Value</b></p><p>Classify piece colors by brightness to choose which pieces receive the dark-piece edge override. Colors below the threshold are treated as dark. A separate enabled black-piece override takes priority for black pieces. The range is 0.00 to 1.00; Reset restores 0.50.</p>") :
		tr("<p><b>Edge Saturation</b></p><p>Adjust the tint or shade used to calculate automated edge colors from piece colors. The range is 0.00 to 1.00; Reset restores 0.50.</p>"));
	PartColorValueLDIndexLabel->setBuddy(PartColorValueLDIndexSlider);
	PartColorValueLDIndexLabel->setToolTip(PartColorValueLDIndexSlider->toolTip());
	PartColorValueLDIndexLabel->setWhatsThis(PartColorValueLDIndexSlider->whatsThis());
	connect(PartColorValueLDIndexSlider, &QSlider::valueChanged, this, &lcAutomateEdgeColorDialog::SliderValueChanged);
	emit PartColorValueLDIndexSlider->valueChanged(PartColorValueLDIndexSlider->value());

	ResetPartColorValueLDIndexButton = new QToolButton(this);
	ResetPartColorValueLDIndexButton->setText(tr("Reset"));
	ResetPartColorValueLDIndexButton->setToolTip(ShowHighContrastDialog ? tr("Restore the light/dark threshold to 0.50.") : tr("Restore edge-color saturation to 0.50."));
	ResetPartColorValueLDIndexButton->setWhatsThis(PartColorValueLDIndexSlider->whatsThis());
	connect(ResetPartColorValueLDIndexButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetSliderButtonClicked);

	EdgeSettingsLayout->addWidget(PartColorValueLDIndexLabel,LDIndexRow,0);
	EdgeSettingsLayout->addWidget(PartColorValueLDIndexSlider,LDIndexRow,1);
	EdgeSettingsLayout->addWidget(PartColorValueLDIndex,LDIndexRow,2);
	EdgeSettingsLayout->addWidget(ResetPartColorValueLDIndexButton,LDIndexRow,3);

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

	StudCylinderColorEnabledBox = new QCheckBox(tr("Stud Cylinder Color:"), this);
	StudCylinderColorEnabledBox->setChecked(mStudCylinderColorEnabled);
	connect(StudCylinderColorEnabledBox, &QCheckBox::clicked, this, &lcAutomateEdgeColorDialog::ColorCheckBoxClicked);

	StudCylinderColorButton = new QToolButton(this);
	StudCylinderColorButton->setEnabled(mStudCylinderColorEnabled);
	SetButtonPixmap(mStudCylinderColor, StudCylinderColorButton);
	connect(StudCylinderColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ColorButtonClicked);

	ResetStudCylinderColorButton = new QToolButton(this);
	ResetStudCylinderColorButton->setText(tr("Reset"));
	ResetStudCylinderColorButton->setEnabled(mStudCylinderColorEnabled);
	connect(ResetStudCylinderColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetColorButtonClicked);

	HighContrastColorLayout->addWidget(StudCylinderColorEnabledBox,0,0);
	HighContrastColorLayout->addWidget(StudCylinderColorButton,0,1);
	HighContrastColorLayout->addWidget(ResetStudCylinderColorButton,0,2);

	PartEdgeColorEnabledBox = new QCheckBox(tr("Pieces Edge Color:"), this);
	PartEdgeColorEnabledBox->setChecked(mPartEdgeColorEnabled);
	connect(PartEdgeColorEnabledBox, &QCheckBox::clicked, this, &lcAutomateEdgeColorDialog::ColorCheckBoxClicked);

	PartEdgeColorButton = new QToolButton(this);
	PartEdgeColorButton->setEnabled(mPartEdgeColorEnabled);
	SetButtonPixmap(mPartEdgeColor, PartEdgeColorButton);
	connect(PartEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ColorButtonClicked);

	ResetPartEdgeColorButton = new QToolButton(this);
	ResetPartEdgeColorButton->setText(tr("Reset"));
	ResetPartEdgeColorButton->setEnabled(mPartEdgeColorEnabled);
	connect(ResetPartEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetColorButtonClicked);

	HighContrastColorLayout->addWidget(PartEdgeColorEnabledBox,1,0);
	HighContrastColorLayout->addWidget(PartEdgeColorButton,1,1);
	HighContrastColorLayout->addWidget(ResetPartEdgeColorButton,1,2);

	BlackEdgeColorEnabledBox = new QCheckBox(tr("Black Pieces Edge Color:"), this);
	BlackEdgeColorEnabledBox->setChecked(mBlackEdgeColorEnabled);
	connect(BlackEdgeColorEnabledBox, &QCheckBox::clicked, this, &lcAutomateEdgeColorDialog::ColorCheckBoxClicked);

	BlackEdgeColorButton = new QToolButton(this);
	BlackEdgeColorButton->setEnabled(mBlackEdgeColorEnabled);
	SetButtonPixmap(mBlackEdgeColor, BlackEdgeColorButton);
	connect(BlackEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ColorButtonClicked);

	ResetBlackEdgeColorButton = new QToolButton(this);
	ResetBlackEdgeColorButton->setText(tr("Reset"));
	ResetBlackEdgeColorButton->setEnabled(mBlackEdgeColorEnabled);
	connect(ResetBlackEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetColorButtonClicked);

	HighContrastColorLayout->addWidget(BlackEdgeColorEnabledBox,2,0);
	HighContrastColorLayout->addWidget(BlackEdgeColorButton,2,1);
	HighContrastColorLayout->addWidget(ResetBlackEdgeColorButton,2,2);

	DarkEdgeColorEnabledBox = new QCheckBox(tr("Dark Pieces Edge Color:"), this);
	DarkEdgeColorEnabledBox->setChecked(mDarkEdgeColorEnabled);
	connect(DarkEdgeColorEnabledBox, &QCheckBox::clicked, this, &lcAutomateEdgeColorDialog::ColorCheckBoxClicked);

	DarkEdgeColorButton = new QToolButton(this);
	DarkEdgeColorButton->setEnabled(mDarkEdgeColorEnabled);
	SetButtonPixmap(mDarkEdgeColor, DarkEdgeColorButton);
	connect(DarkEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ColorButtonClicked);

	ResetDarkEdgeColorButton = new QToolButton(this);
	ResetDarkEdgeColorButton->setText(tr("Reset"));
	ResetDarkEdgeColorButton->setEnabled(mDarkEdgeColorEnabled);
	connect(ResetDarkEdgeColorButton, &QToolButton::clicked, this, &lcAutomateEdgeColorDialog::ResetColorButtonClicked);

	HighContrastColorLayout->addWidget(DarkEdgeColorEnabledBox,3,0);
	HighContrastColorLayout->addWidget(DarkEdgeColorButton,3,1);
	HighContrastColorLayout->addWidget(ResetDarkEdgeColorButton,3,2);

	auto SetupColorHelp = [](QCheckBox* CheckBox, QToolButton* ColorButton, QToolButton* ResetButton, const QString& Help)
	{
		QString Name = CheckBox->text();
		Name.remove(':');
		CheckBox->setToolTip(tr("Enable the custom color for %1.").arg(Name));
		CheckBox->setWhatsThis(Help);
		ColorButton->setAccessibleName(Name);
		ColorButton->setWhatsThis(Help);
		ResetButton->setToolTip(tr("Restore the default color for %1.").arg(Name));
		ResetButton->setWhatsThis(Help);
	};

	SetupColorHelp(StudCylinderColorEnabledBox, StudCylinderColorButton, ResetStudCylinderColorButton,
		tr("Enable a custom color for stud cylinders in high contrast stud styles, then use the color button to choose it."));
	SetupColorHelp(PartEdgeColorEnabledBox, PartEdgeColorButton, ResetPartEdgeColorButton,
		tr("Enable a custom edge color for pieces not covered by the black or dark overrides. Automated edge colors take precedence."));
	SetupColorHelp(BlackEdgeColorEnabledBox, BlackEdgeColorButton, ResetBlackEdgeColorButton,
		tr("Enable a custom edge color for black pieces. This override takes priority over the dark-piece and general edge colors. Automated edge colors take precedence."));
	SetupColorHelp(DarkEdgeColorEnabledBox, DarkEdgeColorButton, ResetDarkEdgeColorButton,
		tr("Enable a custom edge color for pieces below the light/dark threshold, except black pieces with their own enabled override. Automated edge colors take precedence."));

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal, this);
	MainLayout->addWidget(buttonBox);
	lcSetupDialogHelp(this, buttonBox);
	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &lcAutomateEdgeColorDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &lcAutomateEdgeColorDialog::reject);

	setMinimumSize(220,100);
}

void lcAutomateEdgeColorDialog::ColorCheckBoxClicked()
{
	QObject* CheckBox = sender();
	if (CheckBox == StudCylinderColorEnabledBox)
	{
		mStudCylinderColorEnabled = StudCylinderColorEnabledBox->isChecked();
		StudCylinderColorButton->setEnabled(mStudCylinderColorEnabled);
		ResetStudCylinderColorButton->setEnabled(mStudCylinderColorEnabled);
	}
	else if (CheckBox == PartEdgeColorEnabledBox)
	{
		mPartEdgeColorEnabled = PartEdgeColorEnabledBox->isChecked();
		PartEdgeColorButton->setEnabled(mPartEdgeColorEnabled);
		ResetPartEdgeColorButton->setEnabled(mPartEdgeColorEnabled);
	}
	else if (CheckBox == BlackEdgeColorEnabledBox)
	{
		mBlackEdgeColorEnabled = BlackEdgeColorEnabledBox->isChecked();
		BlackEdgeColorButton->setEnabled(mBlackEdgeColorEnabled);
		ResetBlackEdgeColorButton->setEnabled(mBlackEdgeColorEnabled);
	}
	else if (CheckBox == DarkEdgeColorEnabledBox)
	{
		mDarkEdgeColorEnabled = DarkEdgeColorEnabledBox->isChecked();
		DarkEdgeColorButton->setEnabled(mDarkEdgeColorEnabled);
		ResetDarkEdgeColorButton->setEnabled(mDarkEdgeColorEnabled);
	}
}

void lcAutomateEdgeColorDialog::SliderValueChanged(int Value)
{
	if (sender() == PartEdgeContrastSlider)
	{
		mPartEdgeContrast = Value * 0.01f;
		PartEdgeContrast->setText(lcFormatDoubleLocalized(mPartEdgeContrast, locale(), 'f', 2));
	}
	else if (sender() == PartColorValueLDIndexSlider)
	{
		mPartColorValueLDIndex = Value * 0.01f;
		PartColorValueLDIndex->setText(lcFormatDoubleLocalized(mPartColorValueLDIndex, locale(), 'f', 2));
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
		Title = tr("Select Piece Edge Color");
		Color = &mPartEdgeColor;
	}
	else if (Button == BlackEdgeColorButton)
	{
		if (lcGetPreferences().mAutomateEdgeColor)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle(tr("Select Black Edge Color"));
			msgBox.setText(tr("Automate edge color appears to be enabled.<br>Black pieces edge color will not be accessible.<br>Do you want to continue?"));
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
			msgBox.setWindowTitle(tr("Select Dark Edge Color"));
			msgBox.setText(tr("Automate edge color appears to be enabled.<br>Dark pieces edge color will not be accessible.<br>Do you want to continue?"));
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
