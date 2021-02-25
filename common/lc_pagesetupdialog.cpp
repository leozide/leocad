#include "lc_global.h"
#include "lc_pagesetupdialog.h"
#include "ui_lc_pagesetupdialog.h"
#include "lc_instructions.h"

lcPageSetupDialog::lcPageSetupDialog(QWidget* Parent, lcInstructionsPageSetup* PageSetup)
	: QDialog(Parent), ui(new Ui::lcPageSetupDialog), mPageSetup(PageSetup)
{
	ui->setupUi(this);

	ui->WidthEdit->setText(QString::number(PageSetup->Width));
	ui->HeightEdit->setText(QString::number(PageSetup->Height));
	ui->LeftEdit->setText(QString::number(PageSetup->MarginLeft));
	ui->RightEdit->setText(QString::number(PageSetup->MarginRight));
	ui->TopEdit->setText(QString::number(PageSetup->MarginTop));
	ui->BottomEdit->setText(QString::number(PageSetup->MarginBottom));
}

lcPageSetupDialog::~lcPageSetupDialog()
{
	delete ui;
}

void lcPageSetupDialog::accept()
{
	mPageSetup->Width = ui->WidthEdit->text().toFloat();
	mPageSetup->Height = ui->HeightEdit->text().toFloat();
	mPageSetup->MarginLeft = ui->LeftEdit->text().toFloat();
	mPageSetup->MarginRight = ui->RightEdit->text().toFloat();
	mPageSetup->MarginTop = ui->TopEdit->text().toFloat();
	mPageSetup->MarginBottom = ui->BottomEdit->text().toFloat();

	QDialog::accept();
}
