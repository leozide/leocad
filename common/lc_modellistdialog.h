#pragma once

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
	lcModelListDialog(QWidget* Parent, const std::vector<std::unique_ptr<lcModel>>& Models);
	~lcModelListDialog();

	int GetActiveModelIndex() const;
	std::vector<lcModelListDialogEntry> GetResults() const;

public slots:
	void accept() override;

private slots:
	void NewModelClicked();
	void DeleteModelClicked();
	void RenameModelClicked();
	void ExportModelClicked();
	void DuplicateModelClicked();
	void MoveUpClicked();
	void MoveDownClicked();
	void ModelListItemDoubleClicked(QListWidgetItem* Item);
	void ModelListItemSelectionChanged();

private:
	QListWidgetItem* mActiveModelItem;
	void UpdateButtons();
	Ui::lcModelListDialog* ui;
};

