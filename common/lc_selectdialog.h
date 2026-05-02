#pragma once

namespace Ui {
class lcSelectDialog;
}

class lcSelectDialog : public QDialog
{
	Q_OBJECT

public:
	lcSelectDialog(QWidget* Parent, lcModel* Model);
	~lcSelectDialog();

	std::vector<lcObject*> mObjects;

	enum
	{
		IndexRole = Qt::UserRole
	};

public slots:
	void accept() override;

private slots:
	void SelectAllClicked();
	void SelectNoneClicked();
	void SelectInvertClicked();
	void ItemChanged(QTreeWidgetItem *item, int column);

private:
	Ui::lcSelectDialog* ui;

	void AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup, lcModel* Model);
};

