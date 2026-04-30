#pragma once

#include <QDialog>

namespace Ui {
class lcQSelectDialog;
}

class lcQSelectDialog : public QDialog
{
	Q_OBJECT

public:
	lcQSelectDialog(QWidget* Parent, lcModel* Model);
	~lcQSelectDialog();

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
	Ui::lcQSelectDialog *ui;

	void AddChildren(QTreeWidgetItem* ParentItem, lcGroup* ParentGroup, lcModel* Model);
};

