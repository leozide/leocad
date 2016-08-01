#ifndef LC_QIMAGEDIALOG_H
#define LC_QIMAGEDIALOG_H

#include <QDialog>

namespace Ui {
class lcQImageDialog;
}

class lcQImageDialog : public QDialog
{
	Q_OBJECT
	
public:
	lcQImageDialog(QWidget* Parent);
	~lcQImageDialog();

	QString mFileName;
	int mWidth;
	int mHeight;
	int mStart;
	int mEnd;

public slots:
	void accept();
	void on_fileNameBrowse_clicked();

private:
	Ui::lcQImageDialog *ui;
};

#endif // LC_QIMAGEDIALOG_H
