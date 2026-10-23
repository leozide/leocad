#include "lc_global.h"
#include "lc_qutils.h"
#include "lc_categorydialog.h"
#include "ui_lc_categorydialog.h"
#include "lc_category.h"

lcCategoryDialog::lcCategoryDialog(QWidget* Parent, lcLibraryCategory* Options)
	: QDialog(Parent), ui(new Ui::lcCategoryDialog)
{
	ui->setupUi(this);
	lcSetupDialogHelp(this, ui->buttonBox);

	mOptions = Options;

	if (!mOptions->Name.isEmpty())
		setWindowTitle(tr("Edit Category"));
	else
		setWindowTitle(tr("New Category"));

	ui->name->setText(mOptions->Name);
	ui->keywords->setText(mOptions->Keywords);
}

lcCategoryDialog::~lcCategoryDialog()
{
	delete ui;
}

void lcCategoryDialog::accept()
{
	QString Name = ui->name->text();

	if (Name.isEmpty())
	{
		QMessageBox::information(this, windowTitle(), tr("Name cannot be empty."));
		return;
	}

	QString Keywords = ui->keywords->text();

	if (Keywords.isEmpty())
	{
		QMessageBox::information(this, windowTitle(), tr("Keywords cannot be empty."));
		return;
	}

	mOptions->Name = Name;
	mOptions->Keywords = Keywords.toLatin1();

	QDialog::accept();
}
