#ifndef _LC_BASEWINDOW_H_
#define _LC_BASEWINDOW_H_

#include "lc_math.h"
#include "lc_array.h"
#include "lc_application.h"
#include "lc_model.h"
#include "lc_category.h"
#include "lc_shortcuts.h"
#include "image.h"

enum LC_DIALOG_TYPE
{
	LC_DIALOG_SAVE_IMAGE,
	LC_DIALOG_EXPORT_HTML,
	LC_DIALOG_EXPORT_POVRAY,
	LC_DIALOG_PROPERTIES,
	LC_DIALOG_FIND,
	LC_DIALOG_SELECT_BY_NAME,
	LC_DIALOG_MINIFIG,
	LC_DIALOG_PIECE_ARRAY,
	LC_DIALOG_PIECE_GROUP,
	LC_DIALOG_EDIT_GROUPS,
	LC_DIALOG_PREFERENCES,
	LC_DIALOG_CHECK_UPDATES,
	LC_DIALOG_ABOUT
};

struct lcImageDialogOptions
{
	QString FileName;
	int Width;
	int Height;
	int Start;
	int End;
};

struct lcHTMLDialogOptions
{
	QString PathName;
	LC_IMAGE_FORMAT ImageFormat;
	bool TransparentImages;
	bool SinglePage;
	bool IndexPage;
	int StepImagesWidth;
	int StepImagesHeight;
	bool HighlightNewParts;
	bool PartsListStep;
	bool PartsListEnd;
	bool PartsListImages;
	int PartImagesColor;
	int PartImagesWidth;
	int PartImagesHeight;
};

struct lcPOVRayDialogOptions
{
	QString FileName;
	QString POVRayPath;
	QString LGEOPath;
	bool Render;
};

struct lcPropertiesDialogOptions
{
	lcModelProperties Properties;
	bool SetDefault;

	lcArray<lcPartsListEntry> PartsList;
};

struct lcArrayDialogOptions
{
	int Counts[3];
	lcVector3 Offsets[3];
	lcVector3 Rotations[3];
};

struct lcEditGroupsDialogOptions
{
	QMap<lcPiece*, lcGroup*> PieceParents;
	QMap<lcGroup*, lcGroup*> GroupParents;
	QList<lcGroup*> NewGroups;
	//QList<lcGroup*> DeletedGroups; // todo: support deleting groups in the edit groups dialog
};

struct lcSelectDialogOptions
{
	lcArray<lcObject*> Objects;
};

struct lcPreferencesDialogOptions
{
	lcPreferences Preferences;

	QString DefaultAuthor;
	QString ProjectsPath;
	QString LibraryPath;
	QString POVRayPath;
	QString LGEOPath;
	int CheckForUpdates;

	int AASamples;

	lcArray<lcLibraryCategory> Categories;
	bool CategoriesModified;
	bool CategoriesDefault;

	lcKeyboardShortcuts KeyboardShortcuts;
	bool ShortcutsModified;
	bool ShortcutsDefault;
};

#endif // _LC_BASEWINDOW_H_
