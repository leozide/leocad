#include "lc_qmodellistdialog.h"
#include "ui_lc_qmodellistdialog.h"
#include "project.h"

lcQModelListDialog::lcQModelListDialog(QWidget* Parent, QList<QPair<QString, lcModel*>>& Models)
	: QDialog(Parent), mModels(Models), ui(new Ui::lcQModelListDialog)
{
	ui->setupUi(this);

	for (QList<QPair<QString, lcModel*>>::iterator it = Models.begin(); it != Models.end(); it++)
	{
		QListWidgetItem* Item = new QListWidgetItem(it->first);
		Item->setData(Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)it->second));
		ui->ModelList->addItem(Item);
	}
	ui->ModelList->setCurrentRow(0);
}

lcQModelListDialog::~lcQModelListDialog()
{
	delete ui;
}

void lcQModelListDialog::accept()
{
	mModels.clear();

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
	{
		QListWidgetItem* Item = ui->ModelList->item(ItemIdx);
		mModels.append(QPair<QString, lcModel*>(Item->text(), (lcModel*)Item->data(Qt::UserRole).value<uintptr_t>()));
	}

	mActiveModel = ui->ModelList->currentRow();
	if (mActiveModel < 0)
		mActiveModel = 0;

	QDialog::accept();
}

void lcQModelListDialog::on_NewModel_clicked()
{
	const QString Prefix = tr("Model #");
	int Max = 0;

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
	{
		QString Name = ui->ModelList->item(ItemIdx)->text();

		if (Name.startsWith(Prefix))
		{
			QString NumberString = Name.mid(Prefix.length());
			QTextStream Stream(&NumberString);
			int Number;
			Stream >> Number;
			Max = qMax(Max, Number);
		}
	}

	QString Name = Prefix + QString::number(Max + 1);

	if (Name.isEmpty())
		return;

	QListWidgetItem* Item = new QListWidgetItem(Name);
	Item->setData(Qt::UserRole, qVariantFromValue<uintptr_t>(0));
	ui->ModelList->addItem(Item);
}

void lcQModelListDialog::on_DeleteModel_clicked()
{
	if (ui->ModelList->count() == 1)
	{
		QMessageBox::information(this, tr("Error"), tr("The project must have at least 1 model."));
		return;
	}

	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QString Prompt = tr("Are you sure you want to delete the model '%1'?").arg(SelectedItems[0]->text());
	if (QMessageBox::question(this, tr("Delete Model"), Prompt, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	delete SelectedItems[0];
}

void lcQModelListDialog::on_RenameModel_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QString Name = SelectedItems[0]->text();
	bool Ok = false;

	for (;;)
	{
		Name = QInputDialog::getText(this, tr("Rename Model"), tr("Name:"), QLineEdit::Normal, Name, &Ok);

		if (!Ok)
			return;

		if (Name.isEmpty())
		{
			QMessageBox::information(this, tr("Empty Name"), tr("The model name cannot be empty."));
			continue;
		}

		QList<QListWidgetItem*> MatchedItems = ui->ModelList->findItems(Name, Qt::MatchFixedString);

		if (MatchedItems.isEmpty() || (MatchedItems.size() == 1 && MatchedItems[0] == SelectedItems[0]))
			break;

		QMessageBox::information(this, tr("Duplicate Model"), tr("A model named '%1' already exists in this project, please enter an unique name.").arg(Name));
	}

	SelectedItems[0]->setText(Name);
}

void lcQModelListDialog::on_MoveUp_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QListWidgetItem* Item = SelectedItems[0];
	int Row = ui->ModelList->row(Item);

	if (Row == 0)
		return;

	ui->ModelList->takeItem(Row);
	ui->ModelList->insertItem(Row - 1, Item);
	ui->ModelList->setCurrentItem(Item);
}

void lcQModelListDialog::on_MoveDown_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QListWidgetItem* Item = SelectedItems[0];
	int Row = ui->ModelList->row(Item);

	ui->ModelList->takeItem(Row);
	ui->ModelList->insertItem(Row + 1, Item);
	ui->ModelList->setCurrentItem(Item);
}
