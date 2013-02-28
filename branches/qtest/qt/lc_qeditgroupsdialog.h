#ifndef LC_QEDITGROUPSDIALOG_H
#define LC_QEDITGROUPSDIALOG_H

#include <QDialog>
struct lcEditGroupsDialogOptions;
class Group;

namespace Ui {
class lcQEditGroupsDialog;
}

class lcQEditGroupsDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQEditGroupsDialog(QWidget *parent, void *data);
	~lcQEditGroupsDialog();
	
	lcEditGroupsDialogOptions *options;

public slots:
	void accept();

private:
	Ui::lcQEditGroupsDialog *ui;

	void addChildren(QTreeWidgetItem *parentItem, Group *parentGroup);
};

#endif // LC_QEDITGROUPSDIALOG_H
