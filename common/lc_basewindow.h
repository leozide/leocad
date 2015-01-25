#ifndef _LC_BASEWINDOW_H_
#define _LC_BASEWINDOW_H_

#include "lc_math.h"
#include "lc_array.h"
#include "lc_application.h"
#include "lc_model.h"
#include "lc_category.h"
#include "lc_shortcuts.h"
#include "image.h"

#define LC_OK           1
#define LC_CANCEL       2
#define LC_ABORT        3
#define LC_RETRY        4
#define LC_IGNORE       5
#define LC_YES          6
#define LC_NO           7
 
#define LC_MB_OK                 0x000
#define LC_MB_OKCANCEL           0x001
//#define LC_MB_ABORTRETRYIGNORE 0x002
#define LC_MB_YESNOCANCEL        0x003
#define LC_MB_YESNO              0x004
//#define LC_MB_RETRYCANCEL      0x005

#define LC_MB_ICONERROR          0x010
#define LC_MB_ICONQUESTION       0x020
#define LC_MB_ICONWARNING        0x030
#define LC_MB_ICONINFORMATION    0x040
 
#define LC_MB_TYPEMASK           0x00F
#define LC_MB_ICONMASK           0x0F0

enum LC_DIALOG_TYPE
{
	LC_DIALOG_SAVE_IMAGE,
	LC_DIALOG_EXPORT_3DSTUDIO,
	LC_DIALOG_EXPORT_BRICKLINK,
	LC_DIALOG_EXPORT_CSV,
	LC_DIALOG_EXPORT_HTML,
	LC_DIALOG_EXPORT_POVRAY,
	LC_DIALOG_PROPERTIES,
	LC_DIALOG_PRINT,
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
	char FileName[LC_MAXPATH];
	char POVRayPath[LC_MAXPATH];
	char LGEOPath[LC_MAXPATH];
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

	char DefaultAuthor[101];
	char ProjectsPath[LC_MAXPATH];
	char LibraryPath[LC_MAXPATH];
	char POVRayPath[LC_MAXPATH];
	char LGEOPath[LC_MAXPATH];
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
