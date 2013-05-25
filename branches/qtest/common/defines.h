#ifndef _DEFINES_H_
#define _DEFINES_H_

// TODO: cleanup defines and remove this file

#ifdef LC_WINDOWS
#define LC_MAXPATH 260 //_MAX_PATH
#define KEY_CONTROL	VK_CONTROL
#define KEY_ESCAPE	VK_ESCAPE
#define KEY_TAB		VK_TAB
#endif

#ifdef LC_QT
#define LC_MAXPATH 1024//MAXPATHLEN //FILENAME_MAX

#define KEY_CONTROL	Qt::CTRL
#define KEY_ESCAPE	Qt::Key_Escape
#define KEY_TAB	    Qt::Key_Tab

#ifndef WIN32
char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);
#endif

#endif

/////////////////////////////////////////////////////////////////////////////
// LeoCAD constants

#define LC_FOURCC(ch0, ch1, ch2, ch3) (lcuint32)((lcuint32)(lcuint8)(ch0) | ((lcuint32)(lcuint8)(ch1) << 8) | \
                                                ((lcuint32)(lcuint8)(ch2) << 16) | ((lcuint32)(lcuint8)(ch3) << 24 ))

#define LC_FILE_ID LC_FOURCC('L','C','D', 0)

#define LC_STR_VERSION	"LeoCAD 0.7 Project\0\0" // char[20]

#define LC_AUTOSAVE_FLAG		0x100000 // Enable auto-saving

#define LC_SEL_NO_PIECES	0x001 // No pieces in the project
#define LC_SEL_PIECE		0x002 // piece selected
#define LC_SEL_CAMERA		0x004 // camera selected
#define LC_SEL_LIGHT		0x010 // light selected
#define LC_SEL_MULTIPLE		0x020 // multiple pieces selected
#define LC_SEL_UNSELECTED	0x040 // at least 1 piece unselected
#define LC_SEL_HIDDEN		0x080 // at least one piece hidden
#define LC_SEL_GROUP		0x100 // at least one piece selected is grouped
#define LC_SEL_FOCUSGROUP	0x200 // focused piece is grouped
#define LC_SEL_CANGROUP		0x400 // can make a new group

#endif // _DEFINES_H_
