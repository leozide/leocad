#include "lc_global.h"
#include "lc_qpropertiesdialog.h"
#include "ui_lc_qpropertiesdialog.h"
#include "basewnd.h"

lcQPropertiesDialog::lcQPropertiesDialog(QWidget *parent, void *data) :
	QDialog(parent),
	ui(new Ui::lcQPropertiesDialog)
{
	ui->setupUi(this);

	connect(ui->color, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient1, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->gradient2, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->fogColor, SIGNAL(clicked()), this, SLOT(colorClicked()));
	connect(ui->ambient, SIGNAL(clicked()), this, SLOT(colorClicked()));

	options = (lcPropertiesDialogOptions*)data;

	setWindowTitle(QString(tr("%1 Properties")).arg(options->Title));

	ui->description->setText(options->Description);
	ui->author->setText(options->Author);
	ui->comments->setText(options->Comments);

	if (options->BackgroundType == 2)
		ui->image->setChecked(true);
	else if (options->BackgroundType == 1)
		ui->gradient->setChecked(true);
	else
		ui->solid->setChecked(true);

	ui->imageName->setText(options->BackgroundFile);
	ui->tile->setChecked(options->BackgroundTile);
	ui->fog->setChecked(options->FogEnabled);
	ui->fogDensity->setText(QString::number(options->FogDensity));
	ui->floor->setChecked(options->DrawFloor);

	QPixmap pix(12, 12);

	pix.fill(QColor(options->SolidColor[0] * 255, options->SolidColor[1] * 255, options->SolidColor[2] * 255));
	ui->color->setIcon(pix);
	pix.fill(QColor(options->GradientColor1[0] * 255, options->GradientColor1[1] * 255, options->GradientColor1[2] * 255));
	ui->gradient1->setIcon(pix);
	pix.fill(QColor(options->GradientColor2[0] * 255, options->GradientColor2[1] * 255, options->GradientColor2[2] * 255));
	ui->gradient2->setIcon(pix);
	pix.fill(QColor(options->FogColor[0] * 255, options->FogColor[1] * 255, options->FogColor[2] * 255));
	ui->fogColor->setIcon(pix);
	pix.fill(QColor(options->AmbientColor[0] * 255, options->AmbientColor[1] * 255, options->AmbientColor[2] * 255));
	ui->ambient->setIcon(pix);
}

lcQPropertiesDialog::~lcQPropertiesDialog()
{
	delete ui;
}

void lcQPropertiesDialog::accept()
{
	QDialog::accept();
}

void lcQPropertiesDialog::colorClicked()
{
//	QObject *button = sender();

	QColor oldColor = QColor(options->SolidColor[0] * 255, options->SolidColor[1] * 255, options->SolidColor[2] * 255);
	QColor newColor = QColorDialog::getColor(oldColor, this, tr("Select Background Color"));

	if (newColor == oldColor || !newColor.isValid())
		return;

}
