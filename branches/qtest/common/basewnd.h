#ifndef _BASEWND_H_
#define _BASEWND_H_

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
	LC_DIALOG_FILE_OPEN_PROJECT,
	LC_DIALOG_FILE_SAVE_PROJECT,
	LC_DIALOG_FILE_MERGE_PROJECT,
	LC_DIALOG_FILE_EXPORT_BRICKLINK,
	LC_DIALOG_FILE_EXPORT_WAVEFRONT,

	// TODO: update dialogs
	LC_DLG_FILE_OPEN,
	LC_DLG_FILE_SAVE,
	LC_DLG_DIRECTORY_BROWSE,
	LC_DLG_PICTURE_SAVE,
	LC_DLG_HTML,
	LC_DLG_POVRAY,
	LC_DLG_MINIFIG,
	LC_DLG_ARRAY,
	LC_DLG_PREFERENCES,
	LC_DLG_PROPERTIES,
	LC_DLG_TERRAIN,
	LC_DLG_LIBRARY,
	LC_DLG_SELECTBYNAME,
	LC_DLG_STEPCHOOSE,
	LC_DLG_EDITGROUPS,
	LC_DLG_GROUP,
	LC_DLG_EDITCATEGORY,
	LC_DLG_ABOUT
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
