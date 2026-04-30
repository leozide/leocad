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
	void accept() override;

private slots:
	void NewButtonClicked();
	void DeleteButtonClicked();
	void RenameButtonClicked();
	void ImportButtonClicked();
	void MoveUpButtonClicked();
	void MoveDownButtonClicked();
	void PaletteListCurrentRowChanged(int CurrentRow);

private:
	void UpdateButtons();

	Ui::lcPartPaletteDialog* ui;
	std::vector<lcPartPalette>& mPartPalettes;
	std::vector<lcPartPalette*> mImportedPalettes;
};
