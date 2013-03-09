#ifndef LC_QPREFERENCESDIALOG_H
#define LC_QPREFERENCESDIALOG_H

#include <QDialog>
struct lcPreferencesDialogOptions;

namespace Ui {
class lcQPreferencesDialog;
}

class lcQPreferencesDialog : public QDialog
{
	Q_OBJECT
    
public:
	explicit lcQPreferencesDialog(QWidget *parent, void *data);
	~lcQPreferencesDialog();

	lcPreferencesDialogOptions *options;

	enum
	{
		CategoryRole = Qt::UserRole
	};

public slots:
	void accept();
	void on_projectsFolderBrowse_clicked();
	void on_partsLibraryBrowse_clicked();
	void on_antiAliasing_toggled();
	void on_edgeLines_toggled();
	void on_baseGrid_toggled();
	void updateParts();
	void on_newCategory_clicked();
	void on_editCategory_clicked();
	void on_deleteCategory_clicked();
	void on_loadCategories_clicked();
	void on_saveCategories_clicked();
	void on_resetCategories_clicked();

private:
	Ui::lcQPreferencesDialog *ui;

	void updateCategories();

	bool needsToSaveCategories;
};

#endif // LC_QPREFERENCESDIALOG_H
