#pragma once

#include "lc_array.h"

namespace Ui
{
	class lcModelListDialog;
}

struct lcModelListDialogEntry
{
	QString Name;
	lcModel* ExistingModel;
	lcModel* DuplicateSource;
};

class lcModelListDialog : public QDialog
{
	Q_OBJECT

public:
	lcModelListDialog(QWidget* Parent, const lcArray<lcModel*> Models);
	~lcModelListDialog();

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
	Ui::lcModelListDialog* ui;
};

