#pragma once

#include <QDialog>

namespace Ui {
class lcQEditGroupsDialog;
}

class lcQEditGroupsDialog : public QDialog
{
	Q_OBJECT
	
public:
	lcQEditGroupsDialog(QWidget* Parent, const QMap<lcPiece*, lcGroup*>& PieceParents, const QMap<lcGroup*, lcGroup*>& GroupParents, lcModel* Model);
	~lcQEditGroupsDialog();
	
	QMap<lcPiece*, lcGroup*> mPieceParents;
	QMap<lcGroup*, lcGroup*> mGroupParents;
	QList<lcGroup*> mNewGroups;
	//QList<lcGroup*> mDeletedGroups; // todo: support deleting groups in the edit groups dialog

	enum
	{
		PieceRole = Qt::UserRole,
		GroupRole
	};

public slots:
	void accept() override;
	void reject() override;
	void on_newGroup_clicked();
	void onItemClicked(QTreeWidgetItem* Item, int Column);
	void onItemDoubleClicked(QTreeWidgetItem* Item, int Column);

private:
	Ui::lcQEditGroupsDialog *ui;

	void UpdateParents(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup);
	void AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup);

	void timerEvent(QTimerEvent* Event) override;

	lcModel* mModel;
	QTreeWidgetItem* mLastItemClicked;
	bool mEditableDoubleClicked;
	QBasicTimer mClickTimer;
};
