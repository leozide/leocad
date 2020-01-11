#include "lc_global.h"
#include "lc_qfinddialog.h"
#include "ui_lc_qfinddialog.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "lc_colors.h"
#include "lc_model.h"

lcQFindDialog::lcQFindDialog(QWidget* Parent, lcSearchOptions* SearchOptions, lcModel* Model)
	: QDialog(Parent), ui(new Ui::lcQFindDialog)
{
	ui->setupUi(this);

	QComboBox *parts = ui->ID;
	parts->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	parts->setMinimumContentsLength(1);

	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		parts->addItem(PartIt.first->m_strDescription, qVariantFromValue((void*)PartIt.first));
	parts->model()->sort(0);

	mSearchOptions = SearchOptions;

	ui->findColor->setChecked(mSearchOptions->MatchColor);
	ui->color->setCurrentColor(mSearchOptions->ColorIndex);
	ui->findID->setChecked(mSearchOptions->MatchInfo);
	parts->setCurrentIndex(parts->findData(qVariantFromValue((void*)mSearchOptions->Info)));
	ui->findName->setChecked(mSearchOptions->MatchName);
	ui->name->setText(mSearchOptions->Name);
}

lcQFindDialog::~lcQFindDialog()
{
	delete ui;
}

void lcQFindDialog::accept()
{
	mSearchOptions->MatchColor = ui->findColor->isChecked();
	mSearchOptions->ColorIndex = ui->color->currentColor();
	mSearchOptions->MatchInfo= ui->findID->isChecked();
	mSearchOptions->Info = (PieceInfo*)ui->ID->itemData(ui->ID->currentIndex()).value<void*>();
	mSearchOptions->MatchName = ui->findName->isChecked();
	QString name = ui->name->text();
	strcpy(mSearchOptions->Name, name.toLocal8Bit().data());

	QDialog::accept();
}
