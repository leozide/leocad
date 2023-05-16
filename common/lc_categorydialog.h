#pragma once

struct lcLibraryCategory;

namespace Ui {
class lcCategoryDialog;
}

class lcCategoryDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit lcCategoryDialog(QWidget* Parent, lcLibraryCategory* Category);
	virtual ~lcCategoryDialog();

	lcLibraryCategory* mOptions;

public slots:
	void accept() override;

private:
	Ui::lcCategoryDialog* ui;
};

