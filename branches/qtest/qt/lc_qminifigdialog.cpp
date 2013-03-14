#include "lc_global.h"
#include "lc_qminifigdialog.h"
#include "ui_lc_qminifigdialog.h"
#include "lc_glwidget.h"
#include "minifig.h"

lcQMinifigDialog::lcQMinifigDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQMinifigDialog)
{
    ui->setupUi(this);

	QGridLayout *previewLayout = new QGridLayout(ui->minifigFrame);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	lcGLWidget *minifigWidget = new lcGLWidget(NULL, NULL, (GLWindow*)data); // TODO: share lists
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

	options = (MinifigWizard*)data;

	for (int itemIndex = 0; itemIndex < LC_MFW_NUMITEMS; itemIndex++)
	{
		ObjArray<lcMinifigPieceInfo>& parts = options->mSettings[itemIndex];
		QStringList typeList;

		for (int partIndex = 0; partIndex < parts.GetSize(); partIndex++)
			typeList.append(parts[partIndex].Description);

		QComboBox *itemType = getTypeComboBox(itemIndex);

		itemType->blockSignals(true);
		itemType->addItems(typeList);
		itemType->setCurrentIndex(options->GetSelectionIndex(itemIndex));
		itemType->blockSignals(false);
	}

	// TODO: color picker
	/*
	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
		((CColorPicker*)GetDlgItem(IDC_MF_HATSCOLOR+i))->SetColorIndex(m_pMinifig->m_Colors[i]);
	*/
}

lcQMinifigDialog::~lcQMinifigDialog()
{
    delete ui;
}

void lcQMinifigDialog::accept()
{
	QDialog::accept();
}

void lcQMinifigDialog::typeChanged(int index)
{
	options->SetSelectionIndex(getTypeIndex(sender()), index);
	options->Redraw();
}
/*
LONG CMinifigDlg::OnColorSelEndOK(UINT lParam, LONG wParam)
{
	m_pMinifig->SetColor(wParam - IDC_MF_HATSCOLOR, lParam);
	m_pMinifig->Redraw();

	return TRUE;
}
*/
void lcQMinifigDialog::angleChanged(double value)
{
	options->SetAngle(getAngleIndex(sender()), value);
	options->Redraw();
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
