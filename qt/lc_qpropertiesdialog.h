#pragma once

#include <QDialog>
struct lcPropertiesDialogOptions;

namespace Ui {
class lcQPropertiesDialog;
}

class lcQPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	lcQPropertiesDialog(QWidget* Parent, void* Data);
	~lcQPropertiesDialog();

	lcPropertiesDialogOptions *options;

public slots:
	void accept() override;

private:
	Ui::lcQPropertiesDialog *ui;
};

