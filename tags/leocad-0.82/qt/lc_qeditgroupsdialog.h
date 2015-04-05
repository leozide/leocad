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
	explicit lcQEditGroupsDialog(QWidget* Parent, void* Data);
	~lcQEditGroupsDialog();
	
	lcEditGroupsDialogOptions* mOptions;

	enum
	{
		PieceRole = Qt::UserRole,
		GroupRole
	};

public slots:
	void accept();
	void reject();
	void on_newGroup_clicked();
	void onItemClicked(QTreeWidgetItem* Item, int Column);
	void onItemDoubleClicked(QTreeWidgetItem* Item, int Column);

private:
	Ui::lcQEditGroupsDialog *ui;

	void UpdateParents(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup);
	void AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup);

	void timerEvent(QTimerEvent* Event);

	QTreeWidgetItem* mLastItemClicked;
	bool mEditableDoubleClicked;
	QBasicTimer mClickTimer;
};

#endif // LC_QEDITGROUPSDIALOG_H
