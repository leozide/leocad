#pragma once

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
	void accept() override;
	void on_fileNameBrowse_clicked();

private:
	Ui::lcQImageDialog *ui;
};

