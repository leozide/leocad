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

private slots:
	void PartsLibraryBrowseClicked();
	void PartsArchiveBrowseClicked();
	void ColorConfigBrowseButtonClicked();
	void MinifigSettingsBrowseButtonClicked();
	void PovrayExecutableBrowseClicked();
	void LgeoPathBrowseClicked();
	void ColorThemeCurrentIndexChanged(int Index);
	void ColorButtonClicked();
	void AutomateEdgeColor();
	void AutomateEdgeColorToggled();
	void BlenderAddonSettingsButtonClicked();
	void StudStyleComboCurrentIndexChanged(int index);
	void AntiAliasingToggled();
	void EdgeLinesToggled();
	void ConditionalLinesCheckBoxToggled();
	void LineWidthSliderValueChanged();
	void MeshLODSliderValueChanged();
	void FadeStepsToggled();
	void HighlightNewPartsToggled();
	void GridStudsToggled();
	void GridLinesToggled();
	void ViewSphereSizeComboCurrentIndexChanged(int Index);
	void PreviewViewSphereSizeComboCurrentIndexChanged(int Index);
	void updateParts();
	void CategoriesDropped(const QModelIndex& Parent, int First, int Last);
	void NewCategoryClicked();
	void EditCategoryClicked();
	void DeleteCategoryClicked();
	void ImportCategoriesClicked();
	void ExportCategoriesClicked();
	void ResetCategoriesClicked();
	void ShortcutAssignClicked();
	void ShortcutRemoveClicked();
	void ShortcutsImportClicked();
	void ShortcutsExportClicked();
	void ShortcutsResetClicked();
	void commandChanged(QTreeWidgetItem *current);
	void KeyboardFilterEditTextEdited(const QString& Text);
	void MouseAssignClicked();
	void MouseRemoveClicked();
	void MouseImportButtonClicked();
	void MouseExportButtonClicked();
	void MouseResetClicked();
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
