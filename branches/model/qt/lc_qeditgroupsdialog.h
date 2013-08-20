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

	enum
	{
		PieceRole = Qt::UserRole,
		GroupRole,
	};

public slots:
	void accept();
	void on_newGroup_clicked();

private:
	Ui::lcQEditGroupsDialog *ui;

	void updateParents(QTreeWidgetItem *parentItem, Group *parentGroup);
	void addChildren(QTreeWidgetItem *parentItem, Group *parentGroup);
};

#endif // LC_QEDITGROUPSDIALOG_H
