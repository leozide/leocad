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

	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void accept();
	void on_projectsFolderBrowse_clicked();
	void on_partsLibraryBrowse_clicked();
	void on_povrayExecutableBrowse_clicked();
	void on_lgeoPathBrowse_clicked();
	void colorClicked();
	void on_antiAliasing_toggled();
	void on_edgeLines_toggled();
	void on_gridStuds_toggled();
	void on_gridLines_toggled();
	void updateParts();
	void on_newCategory_clicked();
	void on_editCategory_clicked();
	void on_deleteCategory_clicked();
	void on_importCategories_clicked();
	void on_exportCategories_clicked();
	void on_resetCategories_clicked();
	void on_shortcutAssign_clicked();
	void on_shortcutRemove_clicked();
	void on_shortcutsImport_clicked();
	void on_shortcutsExport_clicked();
	void on_shortcutsReset_clicked();
	void commandChanged(QTreeWidgetItem *current);
	void on_mouseAssign_clicked();
	void on_mouseRemove_clicked();
	void on_mouseReset_clicked();
	void MouseTreeItemChanged(QTreeWidgetItem* Current);

private:
	Ui::lcQPreferencesDialog *ui;

	void updateCategories();
	void updateCommandList();
	void UpdateMouseTree();
	void UpdateMouseTreeItem(int ItemIndex);
	void setShortcutModified(QTreeWidgetItem *treeItem, bool modified);
};

#endif // LC_QPREFERENCESDIALOG_H
