#ifndef LC_QHTMLDIALOG_H
#define LC_QHTMLDIALOG_H

#include <QDialog>
struct lcHTMLDialogOptions;

namespace Ui {
class lcQHTMLDialog;
}

class lcQHTMLDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcQHTMLDialog(QWidget *parent, void *data);
	~lcQHTMLDialog();

	lcHTMLDialogOptions *options;

public slots:
	void accept();
	void on_outputFolderBrowse_clicked();

private:
	Ui::lcQHTMLDialog *ui;
};

#endif // LC_QHTMLDIALOG_H
