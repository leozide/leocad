#ifndef _LC_QARRAYDIALOG_H_
#define _LC_QARRAYDIALOG_H_

#include <QDialog>
struct lcArrayDialogOptions;

namespace Ui {
class lcQArrayDialog;
}

class lcQArrayDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQArrayDialog(QWidget *parent, void* data);
	~lcQArrayDialog();

	lcArrayDialogOptions *options;

public slots:
	void accept();

private:
	Ui::lcQArrayDialog *ui;
};

#endif // _LC_QARRAYDIALOG_H_
