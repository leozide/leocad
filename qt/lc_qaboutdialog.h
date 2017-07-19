#pragma once

#include <QDialog>

namespace Ui {
class lcQAboutDialog;
}

class lcQAboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQAboutDialog(QWidget *parent);
	~lcQAboutDialog();

private:
	Ui::lcQAboutDialog *ui;
};

