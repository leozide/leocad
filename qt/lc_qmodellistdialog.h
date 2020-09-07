#pragma once

#include "lc_array.h"

namespace Ui
{
	class lcQModelListDialog;
}

struct lcModelListDialogEntry
{
	QString Name;
	lcModel* ExistingModel;
	lcModel* DuplicateSource;
};

class lcQModelListDialog : public QDialog
{
	Q_OBJECT

public:
	lcQModelListDialog(QWidget* Parent, const lcArray<lcModel*> Models);
	~lcQModelListDialog();

	int GetActiveModelIndex() const;
	std::vector<lcModelListDialogEntry> GetResults() const;

public slots:
	void accept() override;
	void on_NewModel_clicked();
	void on_DeleteModel_clicked();
	void on_RenameModel_clicked();
	void on_ExportModel_clicked();
	void on_DuplicateModel_clicked();
	void on_MoveUp_clicked();
	void on_MoveDown_clicked();
	void on_ModelList_itemDoubleClicked(QListWidgetItem* Item);
	void on_ModelList_itemSelectionChanged();

private:
	QListWidgetItem* mActiveModelItem;
	void UpdateButtons();
	Ui::lcQModelListDialog* ui;
};

