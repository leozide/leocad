#pragma once

#include <QDialog>
struct lcLibraryCategory;

namespace Ui {
class lcQCategoryDialog;
}

class lcQCategoryDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcQCategoryDialog(QWidget *parent, void *data);
	~lcQCategoryDialog();

	lcLibraryCategory *options;

public slots:
	void accept() override;

private:
	Ui::lcQCategoryDialog *ui;
};

