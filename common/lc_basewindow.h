#pragma once

#include "lc_math.h"
#include "lc_array.h"
#include "lc_application.h"
#include "lc_model.h"
#include "lc_category.h"
#include "lc_shortcuts.h"
#include "image.h"

struct lcPropertiesDialogOptions
{
	lcModelProperties Properties;
	bool SetDefault;

	lcPartsList PartsList;
};

struct lcPreferencesDialogOptions
{
	lcPreferences Preferences;

	QString LibraryPath;
	QString ColorConfigPath;
	QString MinifigSettingsPath;
	QString POVRayPath;
	QString LGEOPath;
	QString DefaultAuthor;
	int CheckForUpdates;

	int AASamples;
	int StudLogo;

	lcArray<lcLibraryCategory> Categories;
	bool CategoriesModified;
	bool CategoriesDefault;

	lcKeyboardShortcuts KeyboardShortcuts;
	bool KeyboardShortcutsModified;
	bool KeyboardShortcutsDefault;

	lcMouseShortcuts MouseShortcuts;
	bool MouseShortcutsModified;
	bool MouseShortcutsDefault;
};

