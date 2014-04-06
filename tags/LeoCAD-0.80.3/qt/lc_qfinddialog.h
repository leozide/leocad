#ifndef LC_QFINDDIALOG_H
#define LC_QFINDDIALOG_H

#include <QDialog>
struct lcSearchOptions;

namespace Ui {
class lcQFindDialog;
}

class lcQFindDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQFindDialog(QWidget *parent, void *data);
	~lcQFindDialog();

	lcSearchOptions *options;
	
public slots:
	void accept();

private:
	Ui::lcQFindDialog *ui;
};

#endif // LC_QFINDDIALOG_H
