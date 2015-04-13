#ifndef _LC_QGROUPDIALOG_H_
#define _LC_QGROUPDIALOG_H_

#include <QDialog>

namespace Ui {
class lcQGroupDialog;
}

class lcQGroupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQGroupDialog(QWidget *parent, void *data);
	~lcQGroupDialog();

	char *options;

public slots:
	void accept();

private:
	Ui::lcQGroupDialog *ui;
};

#endif // _LC_QGROUPDIALOG_H_
