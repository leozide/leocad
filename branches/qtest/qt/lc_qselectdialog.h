#ifndef _LC_QSELECTDIALOG_H_
#define _LC_QSELECTDIALOG_H_

#include <QDialog>
struct lcSelectDialogOptions;
class Group;

namespace Ui {
class lcQSelectDialog;
}

class lcQSelectDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQSelectDialog(QWidget *parent, void *data);
	~lcQSelectDialog();

	lcSelectDialogOptions *options;

	enum
	{
		IndexRole = Qt::UserRole
	};

public slots:
	void accept();
	void on_selectAll_clicked();
	void on_selectNone_clicked();
	void on_selectInvert_clicked();
	void itemChanged(QTreeWidgetItem *item, int column);

private:
	Ui::lcQSelectDialog *ui;

	void setSelection(QTreeWidgetItem *parentItem, bool selected);
	void loadSelection(QTreeWidgetItem *parentItem);
	void saveSelection(QTreeWidgetItem *parentItem);
	void addChildren(QTreeWidgetItem *parentItem, Group *parentGroup);
};

#endif // _LC_QSELECTDIALOG_H_
