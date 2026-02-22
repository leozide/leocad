#include "lc_global.h"
#include "lc_qeditgroupsdialog.h"
#include "ui_lc_qeditgroupsdialog.h"
#include "lc_model.h"
#include "piece.h"
#include "group.h"

constexpr uintptr_t LC_GROUPDIALOG_NEW_GROUP = ~0U;

class lcEditGroupsDialogDelegate : public QItemDelegate
{
public:
	lcEditGroupsDialogDelegate(QObject* Parent)
	    : QItemDelegate(Parent)
	{
	}
	
	void setModelData(QWidget* Editor, QAbstractItemModel* Model, const QModelIndex& Index) const
	{
		QLineEdit* LineEdit = qobject_cast<QLineEdit *>(Editor);
		
		if (!LineEdit->isModified())
			return;

		QString Text = LineEdit->text().trimmed();
		
		lcQEditGroupsDialog* Dialog = qobject_cast<lcQEditGroupsDialog*>(parent());
		
		if (Dialog && !Dialog->CanRenameGroup(Text))
			return;
		
		QItemDelegate::setModelData(Editor, Model, Index);
	}
};

lcQEditGroupsDialog::lcQEditGroupsDialog(QWidget* Parent, const lcModel* Model)
    : QDialog(Parent), mModel(Model)
{
	ui = new Ui::lcQEditGroupsDialog;

	ui->setupUi(this);
		
	QPushButton* NewGroup = ui->buttonBox->addButton(tr("&New Group"), QDialogButtonBox::ActionRole);

	connect(NewGroup, &QPushButton::clicked, this, &lcQEditGroupsDialog::NewGroupClicked);

	PopulateTree();
	
	ui->treeWidget->setItemDelegate(new lcEditGroupsDialogDelegate(this));
	ui->treeWidget->expandAll();
}

lcQEditGroupsDialog::~lcQEditGroupsDialog()
{
	delete ui;
}

bool lcQEditGroupsDialog::CanRenameGroup(const QString& Text) const
{
	if (Text.isEmpty())
		return false;
	
	std::function<bool(QTreeWidgetItem*)> ScanGroups = [&ScanGroups, &Text](QTreeWidgetItem* ParentItem)
	{
		for (int ChildIndex = 0; ChildIndex < ParentItem->childCount(); ChildIndex++)
		{
			QTreeWidgetItem* ChildItem = ParentItem->child(ChildIndex);
			
			if (ChildItem->data(0, GroupRole).value<uintptr_t>())
			{
				if (ChildItem->text(0) == Text || !ScanGroups(ChildItem))
					return false;
			}
		}
		
		return true;
	};
	
	return ScanGroups(ui->treeWidget->invisibleRootItem());
}

void lcQEditGroupsDialog::NewGroupClicked()
{
	QTreeWidgetItem* CurrentItem = ui->treeWidget->currentItem();
	
	if (CurrentItem && CurrentItem->data(0, PieceRole).value<uintptr_t>())
		CurrentItem = CurrentItem->parent();
	
	if (!CurrentItem)
		CurrentItem = ui->treeWidget->invisibleRootItem();
	
	QString Prefix = tr("Group #");
	int MaxIndex = 0;
	
	std::function<void(QTreeWidgetItem*)> ScanGroups = [&ScanGroups, &Prefix, &MaxIndex](QTreeWidgetItem* ParentItem)
	{
		for (int ChildIndex = 0; ChildIndex < ParentItem->childCount(); ChildIndex++)
		{
			QTreeWidgetItem* ChildItem = ParentItem->child(ChildIndex);
			
			if (ChildItem->data(0, GroupRole).value<uintptr_t>())
			{
				QString Name = ChildItem->text(0);
				
				if (Name.startsWith(Prefix))
				{
					bool Ok = false;
					int GroupNumber = Name.mid(Prefix.length()).toInt(&Ok);
					
					if (Ok && GroupNumber > MaxIndex)
						MaxIndex = GroupNumber;
				}
				
				ScanGroups(ChildItem);
			}
		}		
	};
	
	ScanGroups(ui->treeWidget->invisibleRootItem());
	
	QString Name = Prefix + QString::number(MaxIndex + 1);
	
	QTreeWidgetItem* GroupItem = new QTreeWidgetItem(CurrentItem, QStringList(Name));
	GroupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
	GroupItem->setData(0, GroupRole, QVariant::fromValue<uintptr_t>(LC_GROUPDIALOG_NEW_GROUP));
	GroupItem->setExpanded(true);
}

lcQEditGroupsDialog::GroupInfo lcQEditGroupsDialog::GetGroupInfo(QTreeWidgetItem* ParentItem) const
{
	lcQEditGroupsDialog::GroupInfo GroupInfo;
	
	GroupInfo.Name = ParentItem->text(0);
	
	uintptr_t GroupPointer = ParentItem->data(0, GroupRole).value<uintptr_t>();
	GroupInfo.Group = (GroupPointer == LC_GROUPDIALOG_NEW_GROUP) ? nullptr : reinterpret_cast<lcGroup*>(GroupPointer);
	
	for (int ChildIndex = 0; ChildIndex < ParentItem->childCount(); ChildIndex++)
	{
		QTreeWidgetItem* ChildItem = ParentItem->child(ChildIndex);
		
		if (ChildItem->data(0, GroupRole).value<uintptr_t>())
			GroupInfo.ChildGroups.emplace_back(GetGroupInfo(ChildItem));
		else
		{
			lcPiece* Piece = reinterpret_cast<lcPiece*>(ChildItem->data(0, PieceRole).value<uintptr_t>());
			
			if (Piece)
				GroupInfo.ChildPieces.push_back(Piece);
		}
	}	
	
	return GroupInfo;
}

lcQEditGroupsDialog::GroupInfo lcQEditGroupsDialog::GetGroups() const
{
	return GetGroupInfo(ui->treeWidget->invisibleRootItem());
}

void lcQEditGroupsDialog::PopulateTree()
{
	const std::vector<std::unique_ptr<lcGroup>>& Groups = mModel->GetGroups();
	std::unordered_map<lcGroup*, QTreeWidgetItem*> AddedGroups;
	std::vector<lcGroup*> GroupsToAdd;
	
	AddedGroups[nullptr] = ui->treeWidget->invisibleRootItem();
	
	auto CreateGroupItem = [&AddedGroups](lcGroup* Group)
	{
		QTreeWidgetItem* ParentItem = AddedGroups.find(Group->mGroup)->second;
		
		if (!ParentItem)
			return false;
		
		QTreeWidgetItem* GroupItem = new QTreeWidgetItem(ParentItem, QStringList(Group->mName));
		GroupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
		GroupItem->setData(0, GroupRole, QVariant::fromValue<uintptr_t>((uintptr_t)Group));
		
		AddedGroups[Group] = GroupItem;
		
		return true;
	};
	
	for (const std::unique_ptr<lcGroup>& Group : Groups)
		if (!CreateGroupItem(Group.get()))
			GroupsToAdd.push_back(Group.get());
	
	while (!GroupsToAdd.empty())
	{
		size_t GroupCount = GroupsToAdd.size();
		
		for (auto GroupIt = GroupsToAdd.begin(); GroupIt != GroupsToAdd.end();)
		{
			if (CreateGroupItem(*GroupIt))
				GroupIt = GroupsToAdd.erase(GroupIt);
			else
				++GroupIt;
		}
		
		if (GroupCount == GroupsToAdd.size())
			break;
	}
	
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = mModel->GetPieces();
	
	for (const std::unique_ptr<lcPiece>& Piece : Pieces)
	{
		QTreeWidgetItem* ParentItem = AddedGroups.find(Piece->GetGroup())->second;
		
		if (ParentItem)
		{
			QTreeWidgetItem* PieceItem = new QTreeWidgetItem(ParentItem, QStringList(Piece->GetName()));
			PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
			PieceItem->setData(0, PieceRole, QVariant::fromValue<uintptr_t>((uintptr_t)Piece.get()));
		}
	}
}
