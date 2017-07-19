#pragma once

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

