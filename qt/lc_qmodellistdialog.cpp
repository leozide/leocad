#include "lc_global.h"
#include "lc_qmodellistdialog.h"
#include "ui_lc_qmodellistdialog.h"
#include "project.h"
#include "lc_profile.h"
#include "lc_model.h"

enum class lcModelListRole
{
	ExistingModel = Qt::UserRole,
	DuplicateModel
};

lcQModelListDialog::lcQModelListDialog(QWidget* Parent, const lcArray<lcModel*> Models)
	: QDialog(Parent), ui(new Ui::lcQModelListDialog)
{
	mActiveModelItem = nullptr;

	ui->setupUi(this);

	for (const lcModel* Model : Models)
	{
		QListWidgetItem* Item = new QListWidgetItem(Model->GetProperties().mFileName);
		Item->setData(static_cast<int>(lcModelListRole::ExistingModel), QVariant::fromValue<uintptr_t>((uintptr_t)Model));
		ui->ModelList->addItem(Item);
	}

	ui->ModelList->setCurrentRow(lcGetActiveProject()->GetActiveModelIndex());

	QSettings Settings;
	ui->SetActiveModel->setChecked(Settings.value("Settings/ModelListSetActive", true).toBool());

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

std::vector<lcModelListDialogEntry> lcQModelListDialog::GetResults() const
{
	std::vector<lcModelListDialogEntry> Models;
	Models.reserve(ui->ModelList->count());

	for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
	{
		QListWidgetItem* Item = ui->ModelList->item(ItemIdx);

		lcModelListDialogEntry Entry;

		Entry.Name = Item->text();
		Entry.ExistingModel = reinterpret_cast<lcModel*>(Item->data(static_cast<int>(lcModelListRole::ExistingModel)).value<uintptr_t>());
		Entry.DuplicateSource = reinterpret_cast<lcModel*>(Item->data(static_cast<int>(lcModelListRole::DuplicateModel)).value<uintptr_t>());

		Models.emplace_back(Entry);
	}

	return Models;
}

void lcQModelListDialog::UpdateButtons()
{
	int ModelCount = ui->ModelList->count();

	ui->DeleteModel->setEnabled(ModelCount > 1);
	ui->SetActiveModel->setEnabled(ui->ModelList->currentItem() != nullptr);

	bool MoveUp = false;
	bool MoveDown = false;

	for (int Row = 0; Row < ui->ModelList->count(); Row++)
	{
		QListWidgetItem* Item = ui->ModelList->item(Row);

		if (!Item->isSelected())
			continue;

		if (Row > 0 && !ui->ModelList->item(Row - 1)->isSelected())
			MoveUp = true;

		if (Row < ModelCount - 1 && !ui->ModelList->item(Row + 1)->isSelected())
			MoveDown = true;
	}

	ui->MoveUp->setEnabled(MoveUp);
	ui->MoveDown->setEnabled(MoveDown);
}

void lcQModelListDialog::accept()
{
	if (ui->SetActiveModel->isChecked())
		mActiveModelItem = ui->ModelList->currentItem();

	QSettings Settings;
	Settings.setValue("Settings/ModelListSetActive", ui->SetActiveModel->isChecked());

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
	ui->ModelList->addItem(Item);
	UpdateButtons();
}

void lcQModelListDialog::on_DeleteModel_clicked()
{
	if (ui->ModelList->count() == 1)
	{
		QMessageBox::information(this, tr("Delete Submodel"), tr("The model cannot be empty."));
		return;
	}

	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
	{
		QMessageBox::information(this, tr("Delete Submodel"), tr("No submodel selected."));
		return;
	}

	QString Prompt;
	if (SelectedItems.size() == 1)
		Prompt = tr("Are you sure you want to delete the submodel '%1'?").arg(SelectedItems[0]->text());
	else
		Prompt = tr("Are you sure you want to delete %1 submodels?").arg(QString::number(SelectedItems.size()));

	if (QMessageBox::question(this, tr("Delete Submodel"), Prompt, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	for (QListWidgetItem* SelectedItem : SelectedItems)
	{
		if (mActiveModelItem == SelectedItem)
			mActiveModelItem = nullptr;

		delete SelectedItem;
	}

	UpdateButtons();
}

void lcQModelListDialog::on_RenameModel_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
	{
		QMessageBox::information(this, tr("Rename Submodel"), tr("No submodel selected."));
		return;
	}

	for (QListWidgetItem* CurrentItem : SelectedItems)
	{
		QStringList ModelNames;

		for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
			ModelNames.append(ui->ModelList->item(ItemIdx)->text());

		QString Name = lcGetActiveProject()->GetNewModelName(this, tr("Rename Submodel"), CurrentItem->text(), ModelNames);

		if (!Name.isEmpty())
			CurrentItem->setText(Name);
	}
}

void lcQModelListDialog::on_ExportModel_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
	{
		QMessageBox::information(this, tr("Export Submodel"), tr("No submodel selected."));
		return;
	}

	if (SelectedItems.size() == 1)
	{
		QListWidgetItem* CurrentItem = SelectedItems[0];
		lcModel* Model = (lcModel*)CurrentItem->data(static_cast<int>(lcModelListRole::ExistingModel)).value<uintptr_t>();

		if (!Model)
		{
			Model = (lcModel*)CurrentItem->data(static_cast<int>(lcModelListRole::DuplicateModel)).value<uintptr_t>();

			if (!Model)
			{
				QMessageBox::information(this, tr("LeoCAD"), tr("Nothing to export."));
				return;
			}
		}

		QString SaveFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), CurrentItem->text()).absoluteFilePath();

		SaveFileName = QFileDialog::getSaveFileName(this, tr("Export Model"), SaveFileName, tr("Supported Files (*.ldr *.dat);;All Files (*.*)"));

		if (SaveFileName.isEmpty())
			return;

		lcGetActiveProject()->ExportModel(SaveFileName, Model);

		lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(SaveFileName).absolutePath());
	}
	else
	{
		QString Folder = QFileDialog::getExistingDirectory(this, tr("Select Export Folder"), lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

		for (QListWidgetItem* CurrentItem : SelectedItems)
		{
			lcModel* Model = (lcModel*)CurrentItem->data(static_cast<int>(lcModelListRole::ExistingModel)).value<uintptr_t>();

			if (!Model)
				Model = (lcModel*)CurrentItem->data(static_cast<int>(lcModelListRole::DuplicateModel)).value<uintptr_t>();

			if (Model)
			{
				QString SaveFileName = QFileInfo(QDir(Folder), CurrentItem->text()).absoluteFilePath();
				lcGetActiveProject()->ExportModel(SaveFileName, Model);
			}
		}

		lcSetProfileString(LC_PROFILE_PROJECTS_PATH, Folder);
	}
}

void lcQModelListDialog::on_DuplicateModel_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->ModelList->selectedItems();

	if (SelectedItems.isEmpty())
	{
		QMessageBox::information(this, tr("Duplicate Submodel"), tr("No submodel selected."));
		return;
	}

	for (QListWidgetItem* CurrentItem : SelectedItems)
	{
		QStringList ModelNames;

		for (int ItemIdx = 0; ItemIdx < ui->ModelList->count(); ItemIdx++)
			ModelNames.append(ui->ModelList->item(ItemIdx)->text());

		QString Name = CurrentItem->text();

		do
		{
			Name = tr("Copy of ") + Name;
		} while (ModelNames.contains(Name, Qt::CaseInsensitive));

		Name = lcGetActiveProject()->GetNewModelName(this, tr("Duplicate Submodel"), Name, ModelNames);

		if (Name.isEmpty())
			continue;

		QListWidgetItem* Item = new QListWidgetItem(Name);
		uintptr_t ExistingModel = CurrentItem->data(static_cast<int>(lcModelListRole::ExistingModel)).value<uintptr_t>();
		if (!ExistingModel)
			ExistingModel = CurrentItem->data(static_cast<int>(lcModelListRole::DuplicateModel)).value<uintptr_t>();
		Item->setData(static_cast<int>(lcModelListRole::DuplicateModel), QVariant::fromValue<uintptr_t>(ExistingModel));
		ui->ModelList->addItem(Item);
	}

	UpdateButtons();
}

void lcQModelListDialog::on_MoveUp_clicked()
{
	bool Blocked = ui->ModelList->blockSignals(true);

	for (int Row = 1; Row < ui->ModelList->count(); Row++)
	{
		QListWidgetItem* Item = ui->ModelList->item(Row);

		if (!Item->isSelected())
			continue;

		if (ui->ModelList->item(Row - 1)->isSelected())
			continue;

		ui->ModelList->takeItem(Row);
		ui->ModelList->insertItem(Row - 1, Item);
		Item->setSelected(true);
	}

	ui->ModelList->blockSignals(Blocked);
	UpdateButtons();
}

void lcQModelListDialog::on_MoveDown_clicked()
{
	bool Blocked = ui->ModelList->blockSignals(true);

	for (int Row = ui->ModelList->count() - 2; Row >= 0; Row--)
	{
		QListWidgetItem* Item = ui->ModelList->item(Row);

		if (!Item->isSelected())
			continue;

		if (ui->ModelList->item(Row + 1)->isSelected())
			continue;

		ui->ModelList->takeItem(Row);
		ui->ModelList->insertItem(Row + 1, Item);
		Item->setSelected(true);
	}

	ui->ModelList->blockSignals(Blocked);
	UpdateButtons();
}

void lcQModelListDialog::on_ModelList_itemDoubleClicked(QListWidgetItem* Item)
{
	mActiveModelItem = Item;

	accept();
}

void lcQModelListDialog::on_ModelList_itemSelectionChanged()
{
	UpdateButtons();
}
