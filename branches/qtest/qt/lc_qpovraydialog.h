#ifndef _LC_QPOVRAYDIALOG_H_
#define _LC_QPOVRAYDIALOG_H_

#include <QDialog>
struct lcPOVRayDialogOptions;

namespace Ui {
class lcQPOVRayDialog;
}

class lcQPOVRayDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQPOVRayDialog(QWidget *parent, void* data);
	~lcQPOVRayDialog();

	lcPOVRayDialogOptions *options;

public slots:
	void accept();
	void on_outputBrowse_clicked();
	void on_povrayBrowse_clicked();
	void on_lgeoBrowse_clicked();

private:
	Ui::lcQPOVRayDialog *ui;
};

#endif // _LC_QPOVRAYDIALOG_H_
