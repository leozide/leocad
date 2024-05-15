#pragma once

#include <QDialog>

namespace Ui {
class lcAboutDialog;
}

class lcAboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcAboutDialog(QWidget* Parent);
	~lcAboutDialog();

private:
	Ui::lcAboutDialog* ui;
};
