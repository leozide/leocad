#include "lc_global.h"
#include "lc_qminifigdialog.h"
#include "ui_lc_qminifigdialog.h"
#include "lc_qglwidget.h"
#include "lc_qcolorpicker.h"
#include "minifig.h"
#include "mainwnd.h"
#include "preview.h"

lcQMinifigDialog::lcQMinifigDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQMinifigDialog)
{
	ui->setupUi(this);

	QGridLayout *previewLayout = new QGridLayout(ui->minifigFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	wizard = new MinifigWizard((lcMinifig*)data);

	lcQGLWidget *minifigWidget = new lcQGLWidget(NULL, (lcQGLWidget*)gMainWindow->mPreviewWidget->mWidget, wizard, false);
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

	options = (lcMinifig*)data;

	for (int itemIndex = 0; itemIndex < LC_MFW_NUMITEMS; itemIndex++)
	{
		ObjArray<lcMinifigPieceInfo>& parts = wizard->mSettings[itemIndex];
		QStringList typeList;

		for (int partIndex = 0; partIndex < parts.GetSize(); partIndex++)
			typeList.append(parts[partIndex].Description);

		QComboBox *itemType = getTypeComboBox(itemIndex);

		itemType->blockSignals(true);
		itemType->addItems(typeList);
		itemType->setCurrentIndex(wizard->GetSelectionIndex(itemIndex));
		itemType->blockSignals(false);

		lcQColorPicker *colorPicker = getColorPicker(itemIndex);
		colorPicker->blockSignals(true);
		colorPicker->setCurrentColor(options->Colors[itemIndex]);
		colorPicker->blockSignals(false);
	}
}

lcQMinifigDialog::~lcQMinifigDialog()
{
	delete wizard;
	delete ui;
}

void lcQMinifigDialog::accept()
{
	QDialog::accept();
}

void lcQMinifigDialog::typeChanged(int index)
{
	wizard->SetSelectionIndex(getTypeIndex(sender()), index);
	wizard->Redraw();
}

void lcQMinifigDialog::colorChanged(int index)
{
	wizard->SetColor(getColorIndex(sender()), index);
	wizard->Redraw();
}

void lcQMinifigDialog::angleChanged(double value)
{
	wizard->SetAngle(getAngleIndex(sender()), value);
	wizard->Redraw();
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

	return NULL;
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
		return LC_MFW_BODY;
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

	return NULL;
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
