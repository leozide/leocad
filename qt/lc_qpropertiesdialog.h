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
	explicit lcQPropertiesDialog(QWidget *parent, void *data);
	~lcQPropertiesDialog();

	lcPropertiesDialogOptions *options;

public slots:
	void accept() override;
	void colorClicked();
	void on_imageNameButton_clicked();

private:
	Ui::lcQPropertiesDialog *ui;
};

