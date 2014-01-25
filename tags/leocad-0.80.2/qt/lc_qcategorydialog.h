#ifndef LC_QCATEGORYDIALOG_H
#define LC_QCATEGORYDIALOG_H

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
	void accept();

private:
	Ui::lcQCategoryDialog *ui;
};

#endif // LC_QCATEGORYDIALOG_H
