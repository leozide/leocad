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

public slots:
	void accept();
	void on_fileNameBrowse_clicked();
	void on_format_currentIndexChanged(int index);

private:
	Ui::lcQImageDialog *ui;
};

#endif // LC_QIMAGEDIALOG_H
