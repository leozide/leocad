#pragma once

#include <QDialog>

namespace Ui {
class lcSetsDatabaseDialog;
}

class lcSetsDatabaseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcSetsDatabaseDialog(QWidget* Parent = nullptr);
	~lcSetsDatabaseDialog();

private:
	Ui::lcSetsDatabaseDialog* ui;
};
