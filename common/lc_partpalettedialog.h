#pragma once

namespace Ui {
class lcPartPaletteDialog;
}

struct lcPartPalette;

class lcPartPaletteDialog : public QDialog
{
    Q_OBJECT

public:
    lcPartPaletteDialog(QWidget* Parent, std::vector<lcPartPalette>& PartPalettes);
    ~lcPartPaletteDialog();

protected slots:
	void accept();
	void on_NewButton_clicked();
	void on_DeleteButton_clicked();
	void on_RenameButton_clicked();
	void on_ImportButton_clicked();
	void on_MoveUpButton_clicked();
	void on_MoveDownButton_clicked();
	void on_PaletteList_currentRowChanged(int CurrentRow);

private:
	void UpdateButtons();

	Ui::lcPartPaletteDialog* ui;
	std::vector<lcPartPalette>& mPartPalettes;
	std::vector<lcPartPalette*> mImportedPalettes;
};
