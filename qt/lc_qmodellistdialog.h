#pragma once

#include <QDialog>

namespace Ui {
class lcQModelListDialog;
}

class lcQModelListDialog : public QDialog
{
	Q_OBJECT

public:
	lcQModelListDialog(QWidget* Parent, QList<QPair<QString, lcModel*>>& Models);
	~lcQModelListDialog();

	int GetActiveModelIndex() const;

	QList<QPair<QString, lcModel*>>& mModels;

public slots:
	void accept();
	void on_NewModel_clicked();
	void on_DeleteModel_clicked();
	void on_RenameModel_clicked();
	void on_ExportModel_clicked();
	void on_MoveUp_clicked();
	void on_MoveDown_clicked();
	void on_SetActiveModel_clicked();
	void on_ModelList_itemDoubleClicked(QListWidgetItem* Item);
	void on_ModelList_currentRowChanged(int CurrentRow);

private:
	QListWidgetItem* mActiveModelItem;
	void UpdateButtons();
	Ui::lcQModelListDialog* ui;
};

