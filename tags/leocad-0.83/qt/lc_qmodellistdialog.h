#ifndef _LC_QMODELLISTDIALOG_H_
#define _LC_QMODELLISTDIALOG_H_

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

	int mActiveModel;
	QList<QPair<QString, lcModel*>>& mModels;

public slots:
	void accept();
	void on_NewModel_clicked();
	void on_DeleteModel_clicked();
	void on_RenameModel_clicked();
	void on_MoveUp_clicked();
	void on_MoveDown_clicked();
	void on_ModelList_itemDoubleClicked(QListWidgetItem* Item);

private:
	Ui::lcQModelListDialog* ui;
};

#endif // _LC_QMODELLISTDIALOG_H_
