#ifndef _BASEWND_H_
#define _BASEWND_H_

#include "defines.h"
#include "lc_math.h"
#include "array.h"
#include "project.h"
#include "lc_category.h"
#include "image.h"
#include "lc_shortcuts.h"

class Group;

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
	LC_DIALOG_OPEN_PROJECT,
	LC_DIALOG_SAVE_PROJECT,
	LC_DIALOG_MERGE_PROJECT,
	LC_DIALOG_SAVE_IMAGE,
	LC_DIALOG_EXPORT_3DSTUDIO,
	LC_DIALOG_EXPORT_BRICKLINK,
	LC_DIALOG_EXPORT_CSV,
	LC_DIALOG_EXPORT_HTML,
	LC_DIALOG_EXPORT_POVRAY,
	LC_DIALOG_EXPORT_WAVEFRONT,
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
	char FileName[LC_MAXPATH];
	LC_IMAGE_FORMAT Format;
	bool Transparent;
	int Width;
	int Height;
	int Start;
	int End;
};

struct lcHTMLDialogOptions
{
	char PathName[LC_MAXPATH];
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
	const char* Title;

	char Author[101];
	char Description[101];
	char Comments[256];

	int BackgroundType;
	lcVector3 SolidColor;
	lcVector3 GradientColor1;
	lcVector3 GradientColor2;
	char BackgroundFileName[LC_MAXPATH];
	bool BackgroundTile;
	bool FogEnabled;
	float FogDensity;
	lcVector3 FogColor;
	lcVector3 AmbientColor;
	bool DrawFloor;
	bool SetDefault;

	ObjArray<lcPiecesUsedEntry> PartsUsed;
};

struct lcArrayDialogOptions
{
	int Counts[3];
	lcVector3 Offsets[3];
	lcVector3 Rotations[3];
};

struct lcEditGroupsDialogOptions
{
	PtrArray<Group> PieceParents;
	PtrArray<Group> GroupParents;
};

struct lcSelectDialogOptions
{
	ObjArray<bool> Selection;
};

struct lcPreferencesDialogOptions
{
	char DefaultAuthor[101];
	char ProjectsPath[LC_MAXPATH];
	char LibraryPath[LC_MAXPATH];
	int MouseSensitivity;
	int CheckForUpdates;

	lcuint32 Snap;
	lcuint32 Detail;
	float LineWidth;
	int AASamples;
	int GridSize;

	ObjArray<lcLibraryCategory> Categories;
	bool CategoriesModified;
	bool CategoriesDefault;

	lcKeyboardShortcuts KeyboardShortcuts;
	bool ShortcutsModified;
	bool ShortcutsDefault;
};

class lcBaseWindow
{
 public:
	lcBaseWindow()
	{
		mHandle = NULL;
	}

	~lcBaseWindow()
	{
	}

	bool DoDialog(LC_DIALOG_TYPE Type, void* Data);

	int DoMessageBox(const char* Text, int Flags = LC_MB_OK | LC_MB_ICONINFORMATION)
	{
		return DoMessageBox(Text, "LeoCAD", Flags);
	}

	int DoMessageBox(const char* Text, const char* Caption = "LeoCAD", int Flags = LC_MB_OK | LC_MB_ICONINFORMATION);

	void* mHandle;
};

#endif // _BASEWND_H_
