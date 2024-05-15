#include "lc_global.h"
#include "lc_partpalettedialog.h"
#include "ui_lc_partpalettedialog.h"
#include "lc_partselectionwidget.h"
#include "lc_setsdatabasedialog.h"

lcPartPaletteDialog::lcPartPaletteDialog(QWidget* Parent, std::vector<lcPartPalette>& PartPalettes)
	: QDialog(Parent), ui(new Ui::lcPartPaletteDialog), mPartPalettes(PartPalettes)
{
    ui->setupUi(this);

	for (const lcPartPalette& Palette : PartPalettes)
	{
		QListWidgetItem* Item = new QListWidgetItem(Palette.Name);
		Item->setData(Qt::UserRole, QVariant::fromValue<uintptr_t>(reinterpret_cast<uintptr_t>(&Palette)));
		ui->PaletteList->addItem(Item);
	}

	ui->PaletteList->setCurrentRow(0);
	UpdateButtons();
}

lcPartPaletteDialog::~lcPartPaletteDialog()
{
	for (lcPartPalette* Palette : mImportedPalettes)
		delete Palette;

	delete ui;
}

void lcPartPaletteDialog::UpdateButtons()
{
	int CurrentPalette = ui->PaletteList->currentRow();
	int PaletteCount = ui->PaletteList->count();

	ui->DeleteButton->setEnabled(PaletteCount > 1);
	ui->MoveUpButton->setEnabled(PaletteCount > 1 && CurrentPalette > 0);
	ui->MoveDownButton->setEnabled(PaletteCount > 1 && CurrentPalette < PaletteCount - 1);
}

void lcPartPaletteDialog::accept()
{
	std::vector<lcPartPalette> PartPalettes;
	PartPalettes.reserve(ui->PaletteList->count());

	for (int ItemIdx = 0; ItemIdx < ui->PaletteList->count(); ItemIdx++)
	{
		QListWidgetItem* Item = ui->PaletteList->item(ItemIdx);
		lcPartPalette* OldPalette = reinterpret_cast<lcPartPalette*>(Item->data(Qt::UserRole).value<uintptr_t>());

		lcPartPalette Palette;
		Palette.Name = Item->text();
		if (OldPalette)
			Palette.Parts = std::move(OldPalette->Parts);

		PartPalettes.emplace_back(std::move(Palette));
	}

	mPartPalettes = std::move(PartPalettes);

	QDialog::accept();
}

void lcPartPaletteDialog::on_NewButton_clicked()
{
	bool Ok = false;

	QString Name = QInputDialog::getText(this, tr("New Part Palette"), tr("Palette Name:"), QLineEdit::Normal, QString(), &Ok);

	if (!Ok || Name.isEmpty())
		return;

	QListWidgetItem* Item = new QListWidgetItem(Name);
	Item->setData(Qt::UserRole, QVariant::fromValue<uintptr_t>(0));
	ui->PaletteList->addItem(Item);
	ui->PaletteList->setCurrentRow(ui->PaletteList->count() - 1);
	UpdateButtons();
}

void lcPartPaletteDialog::on_DeleteButton_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->PaletteList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QString Prompt = tr("Are you sure you want to delete the palette '%1'?").arg(SelectedItems[0]->text());
	if (QMessageBox::question(this, tr("Delete Part Palette"), Prompt, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	QListWidgetItem* SelectedItem = SelectedItems.first();

	delete SelectedItem;
	UpdateButtons();
}

void lcPartPaletteDialog::on_RenameButton_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->PaletteList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	bool Ok = false;

	QString Name = SelectedItems[0]->text();
	Name = QInputDialog::getText(this, tr("Rename Part Palette"), tr("Palette Name:"), QLineEdit::Normal, Name, &Ok);

	if (!Ok || Name.isEmpty())
		return;

	if (!Name.isEmpty())
		SelectedItems[0]->setText(Name);
}

void lcPartPaletteDialog::on_ImportButton_clicked()
{
	lcSetsDatabaseDialog Dialog(this);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	lcPartPalette* Palette = new lcPartPalette;
	Palette->Name = Dialog.GetSetDescription();
	mImportedPalettes.push_back(Palette);

	QByteArray Inventory = Dialog.GetSetInventory();
	QJsonDocument Document = QJsonDocument::fromJson(Inventory);
	QJsonObject Root = Document.object();
	QJsonArray Parts = Root["results"].toArray();

	for (const QJsonValue& Part : Parts)
	{
		QJsonObject PartObject = Part.toObject();
		QByteArray PartID = PartObject["part"].toObject()["part_num"].toString().toLatin1();
		QJsonArray PartIDArray = PartObject["part"].toObject()["external_ids"].toObject()["LDraw"].toArray();
		if (!PartIDArray.isEmpty())
			PartID = PartIDArray.first().toString().toLatin1();

		Palette->Parts.push_back(PartID.toUpper().toStdString() + ".DAT");
	}

	QListWidgetItem* Item = new QListWidgetItem(Palette->Name);
	Item->setData(Qt::UserRole, QVariant::fromValue<uintptr_t>(reinterpret_cast<uintptr_t>(Palette)));
	ui->PaletteList->addItem(Item);
	ui->PaletteList->setCurrentRow(ui->PaletteList->count() - 1);
	UpdateButtons();
}

void lcPartPaletteDialog::on_MoveUpButton_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->PaletteList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QListWidgetItem* Item = SelectedItems[0];
	int Row = ui->PaletteList->row(Item);

	if (Row == 0)
		return;

	ui->PaletteList->takeItem(Row);
	ui->PaletteList->insertItem(Row - 1, Item);
	ui->PaletteList->setCurrentItem(Item);
	UpdateButtons();
}

void lcPartPaletteDialog::on_MoveDownButton_clicked()
{
	QList<QListWidgetItem*>	SelectedItems = ui->PaletteList->selectedItems();

	if (SelectedItems.isEmpty())
		return;

	QListWidgetItem* Item = SelectedItems[0];
	int Row = ui->PaletteList->row(Item);

	ui->PaletteList->takeItem(Row);
	ui->PaletteList->insertItem(Row + 1, Item);
	ui->PaletteList->setCurrentItem(Item);
	UpdateButtons();
}

void lcPartPaletteDialog::on_PaletteList_currentRowChanged(int CurrentRow)
{
	Q_UNUSED(CurrentRow);

	UpdateButtons();
}
