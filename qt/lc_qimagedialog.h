#pragma once

#include <QDialog>

namespace Ui {
class lcImageDialog;
}

class lcImageDialog : public QDialog
{
	Q_OBJECT

public:
	lcImageDialog(QWidget* Parent);
	~lcImageDialog();

	QString mFileName;
	int mWidth;
	int mHeight;
	int mStart;
	int mEnd;

public slots:
	void accept() override;

private slots:
	void FileNameBrowseClicked();

private:
	Ui::lcImageDialog *ui;
};

