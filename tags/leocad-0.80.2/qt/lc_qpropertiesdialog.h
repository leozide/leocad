#ifndef _LC_QPROPERTIESDIALOG_H_
#define _LC_QPROPERTIESDIALOG_H_

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
	void accept();
	void colorClicked();
	void on_imageNameButton_clicked();

private:
	Ui::lcQPropertiesDialog *ui;
};

#endif // _LC_QPROPERTIESDIALOG_H_
