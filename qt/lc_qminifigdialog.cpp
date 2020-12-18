#include "lc_global.h"
#include "lc_qminifigdialog.h"
#include "ui_lc_qminifigdialog.h"
#include "lc_viewwidget.h"
#include "lc_qcolorpicker.h"
#include "minifig.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "lc_library.h"

lcQMinifigDialog::lcQMinifigDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::lcQMinifigDialog)
{
	ui->setupUi(this);

	QGridLayout *previewLayout = new QGridLayout(ui->minifigFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	mMinifigWidget = new MinifigWizard();

	lcViewWidget* minifigWidget = new lcViewWidget(nullptr, mMinifigWidget);
	minifigWidget->setMinimumWidth(100);
	previewLayout->addWidget(minifigWidget);

	connect(ui->hatsType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->hats2Type, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->headType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->neckType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->bodyType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->body2Type, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->body3Type, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->rarmType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->larmType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->rhandType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->lhandType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->rhandaType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->lhandaType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->rlegType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->llegType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->rlegaType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(ui->llegaType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));

	connect(ui->hatsColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->hats2Color, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->headColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->neckColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->bodyColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->body2Color, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->body3Color, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->rarmColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->larmColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->rhandColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->lhandColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->rhandaColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->lhandaColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->rlegColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->llegColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->rlegaColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));
	connect(ui->llegaColor, SIGNAL(colorChanged(int)), this, SLOT(colorChanged(int)));

	connect(ui->hatsAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->hats2Angle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->headAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->rarmAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->larmAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->rhandAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->lhandAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->rhandaAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->lhandaAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->rlegAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->llegAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->rlegaAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));
	connect(ui->llegaAngle, SIGNAL(valueChanged(double)), this, SLOT(angleChanged(double)));

	for (int ItemIndex = 0; ItemIndex < LC_MFW_NUMITEMS; ItemIndex++)
	{
		std::vector<lcMinifigPieceInfo>& PartList = mMinifigWidget->mSettings[ItemIndex];
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

		QComboBox* ItemCombo = getTypeComboBox(ItemIndex);

		ItemCombo->blockSignals(true);
		ItemCombo->addItems(ItemStrings);
		for (int SeparatorIndex = Separators.size() - 1; SeparatorIndex >= 0; SeparatorIndex--)
			ItemCombo->insertSeparator(Separators[SeparatorIndex]);
		ItemCombo->setCurrentIndex(mMinifigWidget->GetSelectionIndex(ItemIndex));
		ItemCombo->blockSignals(false);

		lcQColorPicker *colorPicker = getColorPicker(ItemIndex);
		colorPicker->blockSignals(true);
		colorPicker->setCurrentColor(mMinifigWidget->mMinifig.Colors[ItemIndex]);
		colorPicker->blockSignals(false);
	}

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
	ui->TemplateGroup->hide();
#endif

	mMinifigWidget->OnInitialUpdate();
	UpdateTemplateCombo();
}

lcQMinifigDialog::~lcQMinifigDialog()
{
	delete ui;
}

void lcQMinifigDialog::UpdateTemplateCombo()
{
	ui->TemplateComboBox->clear();

	const auto& Templates = mMinifigWidget->GetTemplates();
	for (const auto& Template : Templates)
		ui->TemplateComboBox->addItem(Template.first);
}

void lcQMinifigDialog::on_TemplateComboBox_currentIndexChanged(const QString& TemplateName)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	const auto& Templates = mMinifigWidget->GetTemplates();
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
				for (const lcMinifigPieceInfo& MinifigPieceInfo : mMinifigWidget->mSettings[PartIdx])
				{
					if (Info == MinifigPieceInfo.Info)
					{
						getTypeComboBox(PartIdx)->setCurrentText(MinifigPieceInfo.Description);
						break;
					}
				}
			}
		}
		else
		{
			getTypeComboBox(PartIdx)->setCurrentText("None");
		}

		getColorPicker(PartIdx)->setCurrentColorCode(Template.Colors[PartIdx]);

		QDoubleSpinBox* AngleSpinBox = getAngleEdit(PartIdx);
		if (AngleSpinBox)
			AngleSpinBox->setValue(Template.Angles[PartIdx]);
	}
#endif
}

void lcQMinifigDialog::on_TemplateSaveButton_clicked()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
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
		Template.Parts[PartIdx] = mMinifigWidget->mSettings[PartIdx][getTypeComboBox(PartIdx)->currentIndex()].Info->mFileName;
		Template.Colors[PartIdx] = getColorPicker(PartIdx)->currentColorCode();
		QDoubleSpinBox* AngleSpinBox = getAngleEdit(PartIdx);
		Template.Angles[PartIdx] = AngleSpinBox ? AngleSpinBox->value() : 0.0f;
	}

	mMinifigWidget->SaveTemplate(TemplateName, Template);

	ui->TemplateComboBox->blockSignals(true);
	UpdateTemplateCombo();
	ui->TemplateComboBox->setCurrentText(TemplateName);
	ui->TemplateComboBox->blockSignals(false);
#endif
}

void lcQMinifigDialog::on_TemplateDeleteButton_clicked()
{
	QString Template = ui->TemplateComboBox->currentText();
	QString Question = tr("Are you sure you want to delete the template '%1'?").arg(Template);

	if (QMessageBox::question(this, tr("Delete Template"), Question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	mMinifigWidget->DeleteTemplate(Template);

	UpdateTemplateCombo();
}

void lcQMinifigDialog::on_TemplateImportButton_clicked()
{
	QString FileName = QFileDialog::getOpenFileName(this, tr("Import Templates"), "", tr("Minifig Template Files (*.minifig);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
		return;
	}

	QByteArray FileData = File.readAll();
	mMinifigWidget->AddTemplatesJson(FileData);

	UpdateTemplateCombo();
}

void lcQMinifigDialog::on_TemplateExportButton_clicked()
{
	QString FileName = QFileDialog::getSaveFileName(this, tr("Export Templates"), "", tr("Minifig Template Files (*.minifig);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return;
	}

	QByteArray Templates = mMinifigWidget->GetTemplatesJson();
	File.write(Templates);
}

void lcQMinifigDialog::typeChanged(int index)
{
	mMinifigWidget->SetSelectionIndex(getTypeIndex(sender()), index);
	mMinifigWidget->Redraw();
}

void lcQMinifigDialog::colorChanged(int index)
{
	mMinifigWidget->SetColor(getColorIndex(sender()), index);
	mMinifigWidget->Redraw();
}

void lcQMinifigDialog::angleChanged(double value)
{
	mMinifigWidget->SetAngle(getAngleIndex(sender()), value);
	mMinifigWidget->Redraw();
}

QComboBox *lcQMinifigDialog::getTypeComboBox(int type)
{
	switch (type)
	{
	case LC_MFW_HATS:
		return ui->hatsType;
	case LC_MFW_HATS2:
		return ui->hats2Type;
	case LC_MFW_HEAD:
		return ui->headType;
	case LC_MFW_NECK:
		return ui->neckType;
	case LC_MFW_BODY:
		return ui->bodyType;
	case LC_MFW_BODY2:
		return ui->body2Type;
	case LC_MFW_BODY3:
		return ui->body3Type;
	case LC_MFW_RARM:
		return ui->rarmType;
	case LC_MFW_LARM:
		return ui->larmType;
	case LC_MFW_RHAND:
		return ui->rhandType;
	case LC_MFW_LHAND:
		return ui->lhandType;
	case LC_MFW_RHANDA:
		return ui->rhandaType;
	case LC_MFW_LHANDA:
		return ui->lhandaType;
	case LC_MFW_RLEG:
		return ui->rlegType;
	case LC_MFW_LLEG:
		return ui->llegType;
	case LC_MFW_RLEGA:
		return ui->rlegaType;
	case LC_MFW_LLEGA:
		return ui->llegaType;
	}

	return nullptr;
}

int lcQMinifigDialog::getTypeIndex(QObject *widget)
{
	if (widget == ui->hatsType)
		return LC_MFW_HATS;
	else if (widget == ui->hats2Type)
		return LC_MFW_HATS2;
	else if (widget == ui->headType)
		return LC_MFW_HEAD;
	else if (widget == ui->neckType)
		return LC_MFW_NECK;
	else if (widget == ui->bodyType)
		return LC_MFW_BODY;
	else if (widget == ui->body2Type)
		return LC_MFW_BODY2;
	else if (widget == ui->body3Type)
		return LC_MFW_BODY3;
	else if (widget == ui->rarmType)
		return LC_MFW_RARM;
	else if (widget == ui->larmType)
		return LC_MFW_LARM;
	else if (widget == ui->rhandType)
		return LC_MFW_RHAND;
	else if (widget == ui->lhandType)
		return LC_MFW_LHAND;
	else if (widget == ui->rhandaType)
		return LC_MFW_RHANDA;
	else if (widget == ui->lhandaType)
		return LC_MFW_LHANDA;
	else if (widget == ui->rlegType)
		return LC_MFW_RLEG;
	else if (widget == ui->llegType)
		return LC_MFW_LLEG;
	else if (widget == ui->rlegaType)
		return LC_MFW_RLEGA;
	else if (widget == ui->llegaType)
		return LC_MFW_LLEGA;

	return -1;
}

lcQColorPicker* lcQMinifigDialog::getColorPicker(int type)
{
	switch (type)
	{
	case LC_MFW_HATS:
		return ui->hatsColor;
	case LC_MFW_HATS2:
		return ui->hats2Color;
	case LC_MFW_HEAD:
		return ui->headColor;
	case LC_MFW_NECK:
		return ui->neckColor;
	case LC_MFW_BODY:
		return ui->bodyColor;
	case LC_MFW_BODY2:
		return ui->body2Color;
	case LC_MFW_BODY3:
		return ui->body3Color;
	case LC_MFW_RARM:
		return ui->rarmColor;
	case LC_MFW_LARM:
		return ui->larmColor;
	case LC_MFW_RHAND:
		return ui->rhandColor;
	case LC_MFW_LHAND:
		return ui->lhandColor;
	case LC_MFW_RHANDA:
		return ui->rhandaColor;
	case LC_MFW_LHANDA:
		return ui->lhandaColor;
	case LC_MFW_RLEG:
		return ui->rlegColor;
	case LC_MFW_LLEG:
		return ui->llegColor;
	case LC_MFW_RLEGA:
		return ui->rlegaColor;
	case LC_MFW_LLEGA:
		return ui->llegaColor;
	}

	return nullptr;
}

int lcQMinifigDialog::getColorIndex(QObject *widget)
{
	if (widget == ui->hatsColor)
		return LC_MFW_HATS;
	else if (widget == ui->hats2Color)
		return LC_MFW_HATS2;
	else if (widget == ui->headColor)
		return LC_MFW_HEAD;
	else if (widget == ui->neckColor)
		return LC_MFW_NECK;
	else if (widget == ui->bodyColor)
		return LC_MFW_BODY;
	else if (widget == ui->body2Color)
		return LC_MFW_BODY2;
	else if (widget == ui->body3Color)
		return LC_MFW_BODY;
	else if (widget == ui->rarmColor)
		return LC_MFW_RARM;
	else if (widget == ui->larmColor)
		return LC_MFW_LARM;
	else if (widget == ui->rhandColor)
		return LC_MFW_RHAND;
	else if (widget == ui->lhandColor)
		return LC_MFW_LHAND;
	else if (widget == ui->rhandaColor)
		return LC_MFW_RHANDA;
	else if (widget == ui->lhandaColor)
		return LC_MFW_LHANDA;
	else if (widget == ui->rlegColor)
		return LC_MFW_RLEG;
	else if (widget == ui->llegColor)
		return LC_MFW_LLEG;
	else if (widget == ui->rlegaColor)
		return LC_MFW_RLEGA;
	else if (widget == ui->llegaColor)
		return LC_MFW_LLEGA;

	return -1;
}

QDoubleSpinBox* lcQMinifigDialog::getAngleEdit(int index)
{
	switch (index)
	{
	case LC_MFW_HATS:
		return ui->hatsAngle;
	case LC_MFW_HATS2:
		return ui->hats2Angle;
	case LC_MFW_HEAD:
		return ui->headAngle;
	case LC_MFW_NECK:
		return nullptr;
	case LC_MFW_BODY:
		return nullptr;
	case LC_MFW_BODY2:
		return nullptr;
	case LC_MFW_BODY3:
		return nullptr;
	case LC_MFW_RARM:
		return ui->rarmAngle;
	case LC_MFW_LARM:
		return ui->larmAngle;
	case LC_MFW_RHAND:
		return ui->rhandAngle;
	case LC_MFW_LHAND:
		return ui->lhandAngle;
	case LC_MFW_RHANDA:
		return ui->rhandaAngle;
	case LC_MFW_LHANDA:
		return ui->lhandaAngle;
	case LC_MFW_RLEG:
		return ui->rlegAngle;
	case LC_MFW_LLEG:
		return ui->llegAngle;
	case LC_MFW_RLEGA:
		return ui->rlegaAngle;
	case LC_MFW_LLEGA:
		return ui->llegaAngle;
	}

	return nullptr;
}

int lcQMinifigDialog::getAngleIndex(QObject *widget)
{
	if (widget == ui->hatsAngle)
		return LC_MFW_HATS;
	else if (widget == ui->hats2Angle)
		return LC_MFW_HATS2;
	else if (widget == ui->headAngle)
		return LC_MFW_HEAD;
	else if (widget == ui->rarmAngle)
		return LC_MFW_RARM;
	else if (widget == ui->larmAngle)
		return LC_MFW_LARM;
	else if (widget == ui->rhandAngle)
		return LC_MFW_RHAND;
	else if (widget == ui->lhandAngle)
		return LC_MFW_LHAND;
	else if (widget == ui->rhandaAngle)
		return LC_MFW_RHANDA;
	else if (widget == ui->lhandaAngle)
		return LC_MFW_LHANDA;
	else if (widget == ui->rlegAngle)
		return LC_MFW_RLEG;
	else if (widget == ui->llegAngle)
		return LC_MFW_LLEG;
	else if (widget == ui->rlegaAngle)
		return LC_MFW_RLEGA;
	else if (widget == ui->llegaAngle)
		return LC_MFW_LLEGA;

	return -1;
}
