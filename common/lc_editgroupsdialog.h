#pragma once

namespace Ui {
class lcEditGroupsDialog;
}

class lcEditGroupsDialog : public QDialog
{
	Q_OBJECT

public:
	lcEditGroupsDialog(QWidget* Parent, const lcModel* Model);
	virtual ~lcEditGroupsDialog();

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

	Ui::lcEditGroupsDialog* ui = nullptr;
	const lcModel* mModel = nullptr;
};
