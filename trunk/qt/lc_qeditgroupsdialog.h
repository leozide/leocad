#ifndef LC_QEDITGROUPSDIALOG_H
#define LC_QEDITGROUPSDIALOG_H

#include <QDialog>
struct lcEditGroupsDialogOptions;

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
		GroupRole
	};

public slots:
	void accept();
	void on_newGroup_clicked();
	void onItemClicked(QTreeWidgetItem *item, int col);
	void onItemDoubleClicked(QTreeWidgetItem *item, int col);

private:
	Ui::lcQEditGroupsDialog *ui;

	void updateParents(QTreeWidgetItem *parentItem, Group *parentGroup);
	void addChildren(QTreeWidgetItem *parentItem, Group *parentGroup);

	void timerEvent(QTimerEvent *event);

	QTreeWidgetItem* m_lastItemClicked;
	bool m_editableDoubleClicked;
	QBasicTimer m_clickTimer;
};

#endif // LC_QEDITGROUPSDIALOG_H
