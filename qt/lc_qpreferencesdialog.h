#pragma once

#include "lc_application.h"
#include "lc_shortcuts.h"
#include "lc_category.h"

struct lcPreferencesDialogOptions
{
	lcPreferences Preferences;

	QString LibraryPath;
	QString ColorConfigPath;
	QString MinifigSettingsPath;
	QString POVRayPath;
	QString LGEOPath;
	QString DefaultAuthor;
	QString Language;
	int CheckForUpdates;

	int AASamples;
	lcStudStyle StudStyle;

	std::vector<lcLibraryCategory> Categories;
	bool CategoriesModified;
	bool CategoriesDefault;

	lcKeyboardShortcuts KeyboardShortcuts;
	bool KeyboardShortcutsModified;
	bool KeyboardShortcutsDefault;

	lcMouseShortcuts MouseShortcuts;
	bool MouseShortcutsModified;
	bool MouseShortcutsDefault;
};

namespace Ui
{
class lcQPreferencesDialog;
}

class lcQPreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	lcQPreferencesDialog(QWidget* Parent, lcPreferencesDialogOptions* Options);
	~lcQPreferencesDialog();

	lcPreferencesDialogOptions* mOptions;

	enum
	{
		CategoryRole = Qt::UserRole
	};

	bool eventFilter(QObject* Object, QEvent* Event) override;

public slots:
	void accept() override;
	void on_partsLibraryBrowse_clicked();
	void on_partsArchiveBrowse_clicked();
	void on_ColorConfigBrowseButton_clicked();
	void on_MinifigSettingsBrowseButton_clicked();
	void on_povrayExecutableBrowse_clicked();
	void on_lgeoPathBrowse_clicked();
	void on_ColorTheme_currentIndexChanged(int Index);
	void ColorButtonClicked();
	void AutomateEdgeColor();
	void on_AutomateEdgeColor_toggled();
	void on_antiAliasing_toggled();
	void on_edgeLines_toggled();
	void on_LineWidthSlider_valueChanged();
	void on_MeshLODSlider_valueChanged();
	void on_FadeSteps_toggled();
	void on_HighlightNewParts_toggled();
	void on_gridStuds_toggled();
	void on_gridLines_toggled();
	void on_ViewSphereSizeCombo_currentIndexChanged(int Index);
	void on_PreviewViewSphereSizeCombo_currentIndexChanged(int Index);
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
	void on_KeyboardFilterEdit_textEdited(const QString& Text);
	void on_mouseAssign_clicked();
	void on_mouseRemove_clicked();
	void on_MouseImportButton_clicked();
	void on_MouseExportButton_clicked();
	void on_mouseReset_clicked();
	void MouseTreeItemChanged(QTreeWidgetItem* Current);

private:
	Ui::lcQPreferencesDialog *ui;

	void updateCategories();
	void updateCommandList();
	void UpdateMouseTree();
	void UpdateMouseTreeItem(int ItemIndex);
	void setShortcutModified(QTreeWidgetItem *treeItem, bool modified);

	float mLineWidthRange[2];
	float mLineWidthGranularity;
	static constexpr float mMeshLODMultiplier = 25.0f;
};
