#ifndef _LC_QSELECTDIALOG_H_
#define _LC_QSELECTDIALOG_H_

#include <QDialog>
#include "lc_array.h"

namespace Ui {
class lcQSelectDialog;
}

class lcQSelectDialog : public QDialog
{
	Q_OBJECT
	
public:
	lcQSelectDialog(QWidget* Parent);
	~lcQSelectDialog();

	lcArray<lcObject*> mObjects;

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

	void AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup);
};

#endif // _LC_QSELECTDIALOG_H_
