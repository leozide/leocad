#include "lc_qfinddialog.h"
#include "ui_lc_qfinddialog.h"
#include "project.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

lcQFindDialog::lcQFindDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQFindDialog)
{
	ui->setupUi(this);

	QComboBox *parts = ui->ID;
	parts->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	parts->setMinimumContentsLength(1);

	lcPiecesLibrary* library = lcGetPiecesLibrary();
	for (int partIdx = 0; partIdx < library->mPieces.GetSize(); partIdx++)
		parts->addItem(library->mPieces[partIdx]->m_strDescription, qVariantFromValue((void*)library->mPieces[partIdx]));
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
