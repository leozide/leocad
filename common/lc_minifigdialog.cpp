#include "lc_global.h"
#include "lc_minifigdialog.h"
#include "ui_lc_minifigdialog.h"
#include "lc_viewwidget.h"
#include "lc_colorpicker.h"
#include "minifig.h"
#include "lc_application.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_view.h"
#include "camera.h"
#include "lc_doublespinbox.h"
#include "lc_qutils.h"
#include "lc_partselectionpopup.h"

lcMinifigDialog::lcMinifigDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcMinifigDialog)
{
	ui->setupUi(this);

	QGridLayout* MinifigLayout = ui->MinifigLayout;

	bool HasSpinBox[LC_MFW_NUMITEMS] = { true, true, true, false, false, false, false, true, true, true, true, true, true, true, true, true, true };
	bool IsLeft[LC_MFW_NUMITEMS] = { true, true, true, true, false, false, false, false, true, false, true, false, true, false, true, false, true };
	int LeftRow = 0, RightRow = 2;

	QString Labels[LC_MFW_NUMITEMS] =
	{
		tr("Hat"), tr("Hat Accessory"), tr("Head"), tr("Neck"), tr("Torso"), tr("Hip"), tr("Hip Accessory"), tr("Left Arm"), tr("Right Arm"), tr("Left Hand"), tr("Right Hand"),
		tr("Left Hand Accessory"), tr("Right Hand Accessory"), tr("Left Leg"), tr("Right Leg"), tr("Left Leg Accessory"), tr("Right Leg Accessory")
	};

	QSizePolicy PieceButtonSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	PieceButtonSizePolicy.setHorizontalStretch(2);
	PieceButtonSizePolicy.setVerticalStretch(0);

	QPixmap Pixmap(1, 1);
	Pixmap.fill(QColor::fromRgba64(0, 0, 0, 0));

	for (int ItemIndex = 0; ItemIndex < LC_MFW_NUMITEMS; ItemIndex++)
	{
		bool Left = IsLeft[ItemIndex];

		lcElidableToolButton* PieceButton = new lcElidableToolButton(this);
		mPieceButtons[ItemIndex] = PieceButton;
		PieceButton->setSizePolicy(PieceButtonSizePolicy);
		PieceButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		PieceButton->setIcon(Pixmap);

		connect(PieceButton, &QToolButton::clicked, this, &lcMinifigDialog::PieceButtonClicked);

		lcColorPicker* ColorPicker = new lcColorPicker(this);
		mColorPickers[ItemIndex] = ColorPicker;

		connect(ColorPicker, &lcColorPicker::ColorChanged, this, &lcMinifigDialog::ColorChanged);

		lcDoubleSpinBox* SpinBox = HasSpinBox[ItemIndex] ? new lcDoubleSpinBox(this) : nullptr;
		mSpinBoxes[ItemIndex] = SpinBox;

		if (SpinBox)
		{
			SpinBox->setRange(-360.0, 360.0);
			SpinBox->SetSnap(lcFloatPropertySnap::Rotation);
			SpinBox->setSingleStep(5.0);

			connect(SpinBox, QOverload<double>::of(&lcDoubleSpinBox::valueChanged), this, &lcMinifigDialog::AngleChanged);
		}

		QWidget* Label = new QLabel(Labels[ItemIndex], this);

		auto AddRow=[MinifigLayout, Label, PieceButton, ColorPicker, SpinBox](int& Row, int Column)
		{
			if (Row)
			{
				MinifigLayout->setRowMinimumHeight(Row++, 2);
			}

			MinifigLayout->addWidget(Label, Row++, Column + 0, 1, 3);
			MinifigLayout->addWidget(PieceButton, Row, Column + 0);
			MinifigLayout->addWidget(ColorPicker, Row, Column + 1);

			if (SpinBox)
				MinifigLayout->addWidget(SpinBox, Row, Column + 2);

			Row++;
		};

		if (Left)
			AddRow(LeftRow, 0);
		else
			AddRow(RightRow, 4);
	}

	QFrame* PreviewFrame = new QFrame(ui->widget);
	QSizePolicy PreviewSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	PreviewSizePolicy.setHorizontalStretch(3);
	PreviewSizePolicy.setVerticalStretch(0);
	PreviewFrame->setSizePolicy(PreviewSizePolicy);
	PreviewFrame->setFrameShape(QFrame::NoFrame);
	PreviewFrame->setFrameShadow(QFrame::Plain);

	MinifigLayout->addWidget(PreviewFrame, 0, 3, -1, 1);

	QGridLayout* PreviewLayout = new QGridLayout(PreviewFrame);
	PreviewLayout->setContentsMargins(0, 0, 0, 0);

	mMinifigWizard = new MinifigWizard();
	mView = new lcView(lcViewType::Minifig, mMinifigWizard->GetModel());

	lcViewWidget* ViewWidget = new lcViewWidget(nullptr, mView);
	ViewWidget->setMinimumWidth(100);
	PreviewLayout->addWidget(ViewWidget);

	mView->MakeCurrent();
	mMinifigWizard->LoadDefault();

	for (int ItemIndex = 0; ItemIndex < LC_MFW_NUMITEMS; ItemIndex++)
	{
		const std::vector<lcMinifigPieceInfo>& PieceList = mMinifigWizard->mSettings[ItemIndex];
		int PieceIndex = mMinifigWizard->GetSelectionIndex(ItemIndex);
		QToolButton* PieceButton = mPieceButtons[ItemIndex];

		PieceButton->setText(PieceList[PieceIndex].Description);

		lcColorPicker* ColorPicker = mColorPickers[ItemIndex];
		QSignalBlocker ColorPickerBlocker(ColorPicker);

		ColorPicker->SetCurrentColor(mMinifigWizard->mMinifig.ColorIndices[ItemIndex]);
	}

	UpdateTemplateCombo();

	mMinifigWizard->Calculate();
	mView->GetCamera()->SetViewpoint(lcVector3(0.0f, -270.0f, 90.0f));
	mView->ZoomExtents();
}

lcMinifigDialog::~lcMinifigDialog()
{
	delete mMinifigWizard;
	delete ui;
}

void lcMinifigDialog::UpdateTemplateCombo()
{
	QSignalBlocker Blocker(ui->TemplateComboBox);

	ui->TemplateComboBox->clear();

	if (mCurrentTemplateName.isEmpty())
		ui->TemplateComboBox->addItem(QString());

	QString CurrentName = mCurrentTemplateName;

	for (const auto& [Name, Template] : mMinifigWizard->GetTemplates())
	{
		if (mCurrentTemplateModified && Name == mCurrentTemplateName)
		{
			CurrentName += " *";
			ui->TemplateComboBox->addItem(CurrentName);
		}
		else
			ui->TemplateComboBox->addItem(Name);
	}

	ui->TemplateComboBox->setCurrentText(CurrentName);
}

void lcMinifigDialog::on_TemplateComboBox_currentIndexChanged(const QString& TemplateName)
{
	const auto& Templates = mMinifigWizard->GetTemplates();
	const auto& Position = Templates.find(TemplateName);

	if (Position == Templates.end())
		return;

	mCurrentTemplateName = TemplateName;
	mCurrentTemplateModified = false;

	UpdateTemplateCombo();

	const lcMinifigTemplate& Template = Position->second;
	
	mView->MakeCurrent();

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (!Template.Parts[PartIdx].isEmpty())
		{
			PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(Template.Parts[PartIdx].toLatin1(), nullptr, false, false);

			mMinifigWizard->SetPieceInfo(PartIdx, Info);

			if (Info)
			{
				for (const lcMinifigPieceInfo& MinifigPieceInfo : mMinifigWizard->mSettings[PartIdx])
				{
					if (Info == MinifigPieceInfo.Info)
					{
						mPieceButtons[PartIdx]->setText(MinifigPieceInfo.Description);
						break;
					}
				}
			}
		}
		else
		{
			mMinifigWizard->SetPieceInfo(PartIdx, nullptr);

			mPieceButtons[PartIdx]->setText(tr("None"));
		}

		QSignalBlocker ColorBlocker(mColorPickers[PartIdx]);

		mMinifigWizard->SetColorIndex(PartIdx, lcGetColorIndex(Template.ColorCodes[PartIdx]));
		mColorPickers[PartIdx]->SetCurrentColorCode(Template.ColorCodes[PartIdx]);

		QDoubleSpinBox* AngleSpinBox = mSpinBoxes[PartIdx];

		if (AngleSpinBox)
		{
			QSignalBlocker AngleBlocker(AngleSpinBox);

			mMinifigWizard->SetAngle(PartIdx, Template.Angles[PartIdx]);
			AngleSpinBox->setValue(Template.Angles[PartIdx]);
		}
	}

	mMinifigWizard->Calculate();

	mView->Redraw();
}

void lcMinifigDialog::on_TemplateSaveButton_clicked()
{
	QString CurrentName = mCurrentTemplateName;
	bool Ok;
	QString TemplateName = QInputDialog::getText(this, tr("Save Template"), tr("Template Name:"), QLineEdit::Normal, CurrentName, &Ok);

	if (!Ok)
		return;

	if (TemplateName.isEmpty())
	{
		QMessageBox::information(this, tr("Save Template"), tr("Template name cannot be empty."));
		return;
	}

	if (TemplateName == CurrentName)
	{
		QString Question = tr("Are you sure you want to overwrite the template '%1'?").arg(TemplateName);
		if (QMessageBox::question(this, tr("Overwrite Template"), Question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;
	}

	lcMinifigTemplate Template;

	for (int ItemIndex = 0; ItemIndex < LC_MFW_NUMITEMS; ItemIndex++)
	{
		Template.Parts[ItemIndex] = mMinifigWizard->mSettings[ItemIndex][mMinifigWizard->GetSelectionIndex(ItemIndex)].Info->mFileName;
		Template.ColorCodes[ItemIndex] = mColorPickers[ItemIndex]->GetCurrentColorCode();
		QDoubleSpinBox* AngleSpinBox = mSpinBoxes[ItemIndex];
		Template.Angles[ItemIndex] = AngleSpinBox ? AngleSpinBox->value() : 0.0f;
	}

	mMinifigWizard->SaveTemplate(TemplateName, Template);

	mCurrentTemplateName = TemplateName;
	mCurrentTemplateModified = false;

	UpdateTemplateCombo();
}

void lcMinifigDialog::on_TemplateDeleteButton_clicked()
{
	if (mCurrentTemplateName.isEmpty())
		return;

	QString Template = mCurrentTemplateName;
	QString Question = tr("Are you sure you want to delete the template '%1'?").arg(Template);

	if (QMessageBox::question(this, tr("Delete Template"), Question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mMinifigWizard->DeleteTemplate(Template);

	mCurrentTemplateName.clear();
	mCurrentTemplateModified = false;

	UpdateTemplateCombo();
}

void lcMinifigDialog::on_TemplateImportButton_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import Templates"), "", tr("Minifig Template Files (*.minifig);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
		return;
	}

	QByteArray FileData = File.readAll();
	mMinifigWizard->AddTemplatesJson(FileData);

	UpdateTemplateCombo();
}

void lcMinifigDialog::on_TemplateExportButton_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export Templates"), "", tr("Minifig Template Files (*.minifig);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(this, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return;
	}

	QByteArray Templates = mMinifigWizard->GetTemplatesJson();
	File.write(Templates);
}

void lcMinifigDialog::PieceButtonClicked()
{
	QToolButton* PieceButton = qobject_cast<QToolButton*>(sender());

	if (!PieceButton)
		return;

	std::array<QToolButton*, LC_MFW_NUMITEMS>::iterator Search = std::find(mPieceButtons.begin(), mPieceButtons.end(), PieceButton);

	if (Search == mPieceButtons.end())
		return;

	int ItemIndex = std::distance(mPieceButtons.begin(), Search);
	PieceInfo* CurrentInfo = mMinifigWizard->mMinifig.Parts[ItemIndex];

	QPoint Position = PieceButton->mapToGlobal(PieceButton->rect().bottomLeft());
	std::vector<std::pair<PieceInfo*, std::string>> Parts;

	Parts.reserve(mMinifigWizard->mSettings[ItemIndex].size());
	Parts.emplace_back(nullptr, tr("None").toStdString());

	for (const lcMinifigPieceInfo& Setting : mMinifigWizard->mSettings[ItemIndex])
		if (Setting.Info)
			Parts.emplace_back(Setting.Info, Setting.Description);

	int ColorIndex = mMinifigWizard->mMinifig.ColorIndices[ItemIndex];

	std::optional<PieceInfo*> Result = lcShowPartSelectionPopup(CurrentInfo, Parts, ColorIndex, PieceButton, PieceButton->mapToGlobal(PieceButton->rect().bottomLeft()));

	if (!Result.has_value())
		return;

	PieceInfo* NewPiece = Result.value();
	
	mCurrentTemplateModified = true;

	UpdateTemplateCombo();

	mView->MakeCurrent();
	mMinifigWizard->SetPieceInfo(ItemIndex, NewPiece);
	mView->Redraw();

	if (NewPiece)
	{
		const std::vector<lcMinifigPieceInfo>& PieceList = mMinifigWizard->mSettings[ItemIndex];
		int PieceIndex = mMinifigWizard->GetSelectionIndex(ItemIndex);

		PieceButton->setText(PieceList[PieceIndex].Description);
	}
	else
	{
		PieceButton->setText("None");
	}
}

void lcMinifigDialog::ColorChanged(int Index)
{
	std::array<lcColorPicker*, LC_MFW_NUMITEMS>::iterator Search = std::find(mColorPickers.begin(), mColorPickers.end(), sender());

	if (Search == mColorPickers.end())
		return;

	mCurrentTemplateModified = true;

	UpdateTemplateCombo();

	mMinifigWizard->SetColorIndex(std::distance(mColorPickers.begin(), Search), Index);
	mView->Redraw();
}

void lcMinifigDialog::AngleChanged(double Value)
{
	std::array<QDoubleSpinBox*, LC_MFW_NUMITEMS>::iterator Search = std::find(mSpinBoxes.begin(), mSpinBoxes.end(), sender());

	if (Search == mSpinBoxes.end())
		return;

	mCurrentTemplateModified = true;

	UpdateTemplateCombo();

	mMinifigWizard->SetAngle(std::distance(mSpinBoxes.begin(), Search), Value);
	mView->Redraw();
}
