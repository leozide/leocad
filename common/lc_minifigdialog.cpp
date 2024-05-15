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

lcMinifigDialog::lcMinifigDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcMinifigDialog)
{
	ui->setupUi(this);

	mComboBoxes =
	{
		ui->hatsType, ui->hats2Type, ui->headType, ui->neckType, ui->bodyType, ui->body2Type, ui->body3Type, ui->rarmType, ui->larmType,
		ui->rhandType, ui->lhandType, ui->rhandaType, ui->lhandaType, ui->rlegType, ui->llegType, ui->rlegaType, ui->llegaType
	};

	mColorPickers =
	{
		ui->hatsColor, ui->hats2Color, ui->headColor, ui->neckColor, ui->bodyColor, ui->body2Color, ui->body3Color, ui->rarmColor, ui->larmColor,
		ui->rhandColor, ui->lhandColor, ui->rhandaColor, ui->lhandaColor, ui->rlegColor, ui->llegColor, ui->rlegaColor, ui->llegaColor
	};

	mSpinBoxes =
	{
		ui->hatsAngle, ui->hats2Angle, ui->headAngle, nullptr, nullptr, nullptr, nullptr, ui->rarmAngle, ui->larmAngle,
		ui->rhandAngle, ui->lhandAngle, ui->rhandaAngle, ui->lhandaAngle, ui->rlegAngle, ui->llegAngle, ui->rlegaAngle, ui->llegaAngle
	};

	for (QComboBox* ComboBox : mComboBoxes)
		connect(ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(TypeChanged(int)));

	for (lcColorPicker* ColorPicker : mColorPickers)
		connect(ColorPicker, &lcColorPicker::ColorChanged, this, &lcMinifigDialog::ColorChanged);

	for (QDoubleSpinBox* SpinBox : mSpinBoxes)
		if (SpinBox)
			connect(SpinBox, SIGNAL(valueChanged(double)), this, SLOT(AngleChanged(double)));

	QGridLayout* PreviewLayout = new QGridLayout(ui->minifigFrame);
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
		const std::vector<lcMinifigPieceInfo>& PartList = mMinifigWizard->mSettings[ItemIndex];
		QStringList ItemStrings;
		QVector<int> Separators;

		for (const lcMinifigPieceInfo& MinifigPieceInfo : PartList)
		{
			const char* Description = MinifigPieceInfo.Description;

			if (Description[0] != '-' || Description[1] != '-')
				ItemStrings.append(Description);
			else
				Separators.append(ItemStrings.size());
		}

		QComboBox* ItemCombo = mComboBoxes[ItemIndex];

		ItemCombo->blockSignals(true);
		ItemCombo->addItems(ItemStrings);
		for (int SeparatorIndex = Separators.size() - 1; SeparatorIndex >= 0; SeparatorIndex--)
			ItemCombo->insertSeparator(Separators[SeparatorIndex]);
		ItemCombo->setCurrentIndex(mMinifigWizard->GetSelectionIndex(ItemIndex));
		ItemCombo->blockSignals(false);

		lcColorPicker* ColorPicker = mColorPickers[ItemIndex];

		ColorPicker->blockSignals(true);
		ColorPicker->SetCurrentColor(mMinifigWizard->mMinifig.Colors[ItemIndex]);
		ColorPicker->blockSignals(false);
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
	ui->TemplateComboBox->clear();

	const auto& Templates = mMinifigWizard->GetTemplates();
	for (const auto& Template : Templates)
		ui->TemplateComboBox->addItem(Template.first);
}

void lcMinifigDialog::on_TemplateComboBox_currentIndexChanged(const QString& TemplateName)
{
	const auto& Templates = mMinifigWizard->GetTemplates();
	const auto& Position = Templates.find(TemplateName);
	if (Position == Templates.end())
		return;

	const lcMinifigTemplate& Template = Position->second;

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (!Template.Parts[PartIdx].isEmpty())
		{
			PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(Template.Parts[PartIdx].toLatin1(), nullptr, false, false);

			if (Info)
			{
				for (const lcMinifigPieceInfo& MinifigPieceInfo : mMinifigWizard->mSettings[PartIdx])
				{
					if (Info == MinifigPieceInfo.Info)
					{
						mComboBoxes[PartIdx]->setCurrentText(MinifigPieceInfo.Description);
						break;
					}
				}
			}
		}
		else
		{
			mComboBoxes[PartIdx]->setCurrentText("None");
		}

		mColorPickers[PartIdx]->SetCurrentColorCode(Template.Colors[PartIdx]);

		QDoubleSpinBox* AngleSpinBox = mSpinBoxes[PartIdx];
		if (AngleSpinBox)
			AngleSpinBox->setValue(Template.Angles[PartIdx]);
	}

	mMinifigWizard->Calculate();
}

void lcMinifigDialog::on_TemplateSaveButton_clicked()
{
	QString CurrentName = ui->TemplateComboBox->currentText();
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

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		Template.Parts[PartIdx] = mMinifigWizard->mSettings[PartIdx][mComboBoxes[PartIdx]->currentIndex()].Info->mFileName;
		Template.Colors[PartIdx] = mColorPickers[PartIdx]->GetCurrentColorCode();
		QDoubleSpinBox* AngleSpinBox = mSpinBoxes[PartIdx];
		Template.Angles[PartIdx] = AngleSpinBox ? AngleSpinBox->value() : 0.0f;
	}

	mMinifigWizard->SaveTemplate(TemplateName, Template);

	ui->TemplateComboBox->blockSignals(true);
	UpdateTemplateCombo();
	ui->TemplateComboBox->setCurrentText(TemplateName);
	ui->TemplateComboBox->blockSignals(false);
}

void lcMinifigDialog::on_TemplateDeleteButton_clicked()
{
	QString Template = ui->TemplateComboBox->currentText();
	QString Question = tr("Are you sure you want to delete the template '%1'?").arg(Template);

	if (QMessageBox::question(this, tr("Delete Template"), Question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mMinifigWizard->DeleteTemplate(Template);

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

void lcMinifigDialog::TypeChanged(int Index)
{
	std::array<QComboBox*, LC_MFW_NUMITEMS>::iterator Search = std::find(mComboBoxes.begin(), mComboBoxes.end(), sender());

	if (Search == mComboBoxes.end())
		return;

	mView->MakeCurrent();
	mMinifigWizard->SetSelectionIndex(std::distance(mComboBoxes.begin(), Search), Index);
	mView->Redraw();
}

void lcMinifigDialog::ColorChanged(int Index)
{
	std::array<lcColorPicker*, LC_MFW_NUMITEMS>::iterator Search = std::find(mColorPickers.begin(), mColorPickers.end(), sender());

	if (Search == mColorPickers.end())
		return;

	mMinifigWizard->SetColor(std::distance(mColorPickers.begin(), Search), Index);
	mView->Redraw();
}

void lcMinifigDialog::AngleChanged(double Value)
{
	std::array<QDoubleSpinBox*, LC_MFW_NUMITEMS>::iterator Search = std::find(mSpinBoxes.begin(), mSpinBoxes.end(), sender());

	if (Search == mSpinBoxes.end())
		return;

	mMinifigWizard->SetAngle(std::distance(mSpinBoxes.begin(), Search), Value);
	mView->Redraw();
}
