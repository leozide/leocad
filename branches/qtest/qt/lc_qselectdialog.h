#ifndef LC_QSELECTDIALOG_H
#define LC_QSELECTDIALOG_H

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

private:
	Ui::lcQSelectDialog *ui;

	void setSelection(QTreeWidgetItem *parentItem);
	void updateSelection(QTreeWidgetItem *parentItem);
	void addChildren(QTreeWidgetItem *parentItem, Group *parentGroup);
};

#endif // LC_QSELECTDIALOG_H
