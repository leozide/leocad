#pragma once

namespace Ui {
class lcAboutDialog;
}

class lcAboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcAboutDialog(QWidget* Parent);
	virtual ~lcAboutDialog();

private:
	Ui::lcAboutDialog* ui;
};
