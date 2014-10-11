#ifndef LC_QIMAGEDIALOG_H
#define LC_QIMAGEDIALOG_H

#include <QDialog>
struct lcImageDialogOptions;

namespace Ui {
class lcQImageDialog;
}

class lcQImageDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQImageDialog(QWidget *parent, void *data);
	~lcQImageDialog();

	lcImageDialogOptions *options;
	int currentStep;
	int lastStep;

public slots:
	void accept();
	void on_fileNameBrowse_clicked();

private:
	Ui::lcQImageDialog *ui;
};

#endif // LC_QIMAGEDIALOG_H
