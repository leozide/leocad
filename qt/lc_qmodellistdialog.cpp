#include "lc_global.h"
#include "lc_qmodellistdialog.h"
#include "ui_lc_qmodellistdialog.h"
#include "project.h"
#include "lc_profile.h"

lcQModelListDialog::lcQModelListDialog(QWidget* Parent, QList<QPair<QString, lcModel*>>& Models)
	: QDialog(Parent), mModels(Models), ui(new Ui::lcQModelListDialog)
{
	mActiveModelItem = nullptr;

	ui->setupUi(this);

	for (QList<QPair<QString, lcModel*>>::iterator it = Models.begin(); it != Models.end(); it++)
	{
		QListWidgetItem* Item = new QListWidgetItem(it->first);
		Item->setData(Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)it->second));
		ui->ModelList->addItem(Item);
	}
	ui->ModelList->setCurrentRow(lcGetActiveProject()->GetActiveModelIndex());
	UpdateButtons();
}

lcQModelListDialog::~lcQModelListDialog()
{
	delete ui;
}

int lcQModelListDialog::GetActiveModelIndex() const
{
	return ui->ModelList->row(mActiveModelItem);
}

void lcQModelListDialog::UpdateButtons()
{
	int CurrentModel = ui->ModelList->currentRow();
	int NumModels = ui->ModelList->count();

	ui->DeleteModel->setEnabled(NumModels > 1);
	ui->MoveUp->setEnabled(NumModels > 1 && CurrentModel > 0);
	ui->MoveDown->setEnabled(NumModels > 1 && CurrentModel < NumModels - 1);
}

void lcQModelListDialog::accept()
{
	mModels.clear();

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
	{
		QListWidgetItem* Item = ui->ModelList->item(ItemIdx);
		mModels.append(QPair<QString, lcModel*>(Item->text(), (lcModel*)Item->data(Qt::UserRole).value<uintptr_t>()));
	}

	QDialog::accept();
}

void lcQModelListDialog::on_NewModel_clicked()
{
	QStringList ModelNames;

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
		ModelNames.append(ui->ModelList->item(ItemIdx)->text());

	QString Name = lcGetActiveProject()->GetNewModelName(this, tr("New Submodel"), QString(), ModelNames);

	if (Name.isEmpty())
		return;

	QListWidgetItem* Item = new QListWidgetItem(Name);
	Item->setData(Qt::UserRole, qVariantFromValue<uintptr_t>(0));
	ui->ModelList->addItem(Item);
	UpdateButtons();
}

void lcQModelListDialog::on_DeleteModel_clicked()
{
	if (ui->ModelList->count() == 1)
	{
		QMessageBox::information(this, tr("Error"), tr("The model cannot be empty."));
		return;
	}

	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QString Prompt = tr("Are you sure you want to delete the submodel '%1'?").arg(SelectedItems[0]->text());
	if (QMessageBox::question(this, tr("Delete Submodel"), Prompt, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	QListWidgetItem* SelectedItem = SelectedItems.first();

	if (mActiveModelItem == SelectedItem)
		mActiveModelItem = nullptr;

	delete SelectedItem;
	UpdateButtons();
}

void lcQModelListDialog::on_RenameModel_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QStringList ModelNames;

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
		ModelNames.append(ui->ModelList->item(ItemIdx)->text());

	QString Name = lcGetActiveProject()->GetNewModelName(this, tr("Rename Submodel"), SelectedItems[0]->text(), ModelNames);

	if (!Name.isEmpty())
		SelectedItems[0]->setText(Name);
}

void lcQModelListDialog::on_ExportModel_clicked()
{
	QListWidgetItem* CurrentItem = ui->ModelList->currentItem();

	if (!CurrentItem)
		return;

	lcModel* Model = (lcModel*)CurrentItem->data(Qt::UserRole).value<uintptr_t>();

	if (!Model)
	{
		QMessageBox::information(this, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), CurrentItem->text()).absoluteFilePath();

	SaveFileName = QFileDialog::getSaveFileName(this, tr("Save Model"), SaveFileName, tr("Supported Files (*.ldr *.dat);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcGetActiveProject()->ExportModel(SaveFileName, Model);

	lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(SaveFileName).absolutePath());
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
	UpdateButtons();
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
	UpdateButtons();
}

void lcQModelListDialog::on_SetActiveModel_clicked()
{
	mActiveModelItem = ui->ModelList->currentItem();
}

void lcQModelListDialog::on_ModelList_itemDoubleClicked(QListWidgetItem* Item)
{
	Q_UNUSED(Item);

	accept();
}

void lcQModelListDialog::on_ModelList_currentRowChanged(int CurrentRow)
{
	Q_UNUSED(CurrentRow);

	UpdateButtons();
}
