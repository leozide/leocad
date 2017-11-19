#include "lc_global.h"
#include "lc_qfinddialog.h"
#include "ui_lc_qfinddialog.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "project.h"
#include "lc_colors.h"
#include "lc_model.h"

lcQFindDialog::lcQFindDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQFindDialog)
{
	ui->setupUi(this);

	QComboBox *parts = ui->ID;
	parts->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	parts->setMinimumContentsLength(1);

	lcPartsList PartsList;
	lcGetActiveModel()->GetPartsList(gDefaultColor, false, PartsList);

	for (const auto& PartIt : PartsList)
		parts->addItem(PartIt.first->m_strDescription, qVariantFromValue((void*)PartIt.first));
	parts->model()->sort(0);

	options = (lcSearchOptions*)data;

	ui->findColor->setChecked(options->MatchColor);
	ui->color->setCurrentColor(options->ColorIndex);
	ui->findID->setChecked(options->MatchInfo);
	parts->setCurrentIndex(parts->findData(qVariantFromValue((void*)options->Info)));
	ui->findName->setChecked(options->MatchName);
	ui->name->setText(options->Name);
}

lcQFindDialog::~lcQFindDialog()
{
	delete ui;
}

void lcQFindDialog::accept()
{
	options->MatchColor = ui->findColor->isChecked();
	options->ColorIndex = ui->color->currentColor();
	options->MatchInfo= ui->findID->isChecked();
	options->Info = (PieceInfo*)ui->ID->itemData(ui->ID->currentIndex()).value<void*>();
	options->MatchName = ui->findName->isChecked();
	QString name = ui->name->text();
	strcpy(options->Name, name.toLocal8Bit().data());

	QDialog::accept();
}
