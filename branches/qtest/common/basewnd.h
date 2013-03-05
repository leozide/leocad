#ifndef _BASEWND_H_
#define _BASEWND_H_

#include "defines.h"
#include "lc_math.h"
#include "array.h"
#include "project.h"

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
	LC_DIALOG_OPEN_CATEGORIES,
	LC_DIALOG_SAVE_CATEGORIES,
	LC_DIALOG_EXPORT_BRICKLINK,
	LC_DIALOG_EXPORT_POVRAY,
	LC_DIALOG_EXPORT_WAVEFRONT,
	LC_DIALOG_PROPERTIES,
	LC_DIALOG_SELECT_BY_NAME,
	LC_DIALOG_PIECE_ARRAY,
	LC_DIALOG_PIECE_GROUP,
	LC_DIALOG_EDIT_GROUPS,
	LC_DIALOG_ABOUT,

	// TODO: update dialogs
	LC_DLG_DIRECTORY_BROWSE,
	LC_DLG_PICTURE_SAVE,
	LC_DLG_HTML,
	LC_DLG_MINIFIG,
	LC_DLG_PREFERENCES,
//	LC_DLG_PROPERTIES,
	LC_DLG_TERRAIN,
	LC_DLG_LIBRARY,
	LC_DLG_STEPCHOOSE,
	LC_DLG_EDITCATEGORY,
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
