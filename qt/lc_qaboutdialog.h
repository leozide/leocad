#ifndef _LC_QABOUTDIALOG_H_
#define _LC_QABOUTDIALOG_H_

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

#endif // _LC_QABOUTDIALOG_H_
