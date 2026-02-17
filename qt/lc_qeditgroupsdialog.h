#pragma once

namespace Ui {
class lcQEditGroupsDialog;
}

class lcQEditGroupsDialog : public QDialog
{
	Q_OBJECT
	
public:
	lcQEditGroupsDialog(QWidget* Parent, const lcModel* Model);
	virtual ~lcQEditGroupsDialog();
	
	struct GroupInfo
	{
		QString Name;
		lcGroup* Group;
		std::vector<GroupInfo> ChildGroups;
		std::vector<lcPiece*> ChildPieces;
	};
	
	GroupInfo GetGroups() const;
	bool CanRenameGroup(const QString& Text) const;
	
protected slots:
	void NewGroupClicked();

protected:
	enum
	{
		PieceRole = Qt::UserRole,
		GroupRole
	};
	
	GroupInfo GetGroupInfo(QTreeWidgetItem* ParentItem) const;
	void PopulateTree();
	
	Ui::lcQEditGroupsDialog* ui = nullptr;
	const lcModel* mModel = nullptr;
};
