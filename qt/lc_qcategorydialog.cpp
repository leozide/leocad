#include "lc_global.h"
#include "lc_qcategorydialog.h"
#include "ui_lc_qcategorydialog.h"
#include "lc_category.h"

lcQCategoryDialog::lcQCategoryDialog(QWidget *parent, void *data) :
    QDialog(parent),
    ui(new Ui::lcQCategoryDialog)
{
	ui->setupUi(this);

	options = (lcLibraryCategory*)data;

	if (!options->Name.isEmpty())
		setWindowTitle(tr("Edit Category"));
	else
		setWindowTitle(tr("New Category"));

	ui->name->setText(options->Name);
	ui->keywords->setText(options->Keywords);
}

lcQCategoryDialog::~lcQCategoryDialog()
{
	delete ui;
}

void lcQCategoryDialog::accept()
{
	QString name = ui->name->text();

	if (name.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Name cannot be empty."));
		return;
	}

	QString keywords = ui->keywords->text();

	if (keywords.isEmpty())
	{
		QMessageBox::information(this, "LeoCAD", tr("Keywords cannot be empty."));
		return;
	}

	options->Name = name;
	options->Keywords = keywords.toLatin1();

	QDialog::accept();
}
