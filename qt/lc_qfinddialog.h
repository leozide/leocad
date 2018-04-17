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
	lcQFindDialog(QWidget* Parent, lcSearchOptions* SearchOptions, lcModel* Model);
	~lcQFindDialog();

	lcSearchOptions* mSearchOptions;
	
public slots:
	void accept();

private:
	Ui::lcQFindDialog *ui;
};

