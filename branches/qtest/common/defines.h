#ifndef _DEFINES_H_
#define _DEFINES_H_

#define LC_RGB(r,g,b) ((lcuint32)(((lcuint8) (r) | ((lcuint16) (g) << 8))|(((lcuint32) (lcuint8) (b)) << 16))) 

// ============================================================================
// Old defines (mostly deprecated).

// TODO: cleanup defines and remove this file

#ifdef WIN32
#define isnan _isnan
#endif

#ifndef LC_QT

#ifdef LC_WINDOWS
#define LC_MAXPATH 260 //_MAX_PATH
#define KEY_SHIFT	VK_SHIFT
#define KEY_CONTROL	VK_CONTROL
#define KEY_ALT		VK_MENU
#define KEY_ESCAPE	VK_ESCAPE
#define KEY_TAB		VK_TAB
#define KEY_INSERT	VK_INSERT
#define KEY_DELETE	VK_DELETE
#define KEY_UP		VK_UP
#define KEY_DOWN	VK_DOWN
#define KEY_LEFT	VK_LEFT
#define KEY_RIGHT	VK_RIGHT
#define KEY_PRIOR	VK_PRIOR
#define KEY_NEXT	VK_NEXT
#define KEY_PLUS	VK_ADD
#define KEY_MINUS	VK_SUBTRACT
#endif

#ifdef LC_LINUX
#include <unistd.h>

#define LC_MAXPATH 1024 //FILENAME_MAX
#define KEY_SHIFT	0x01
#define KEY_CONTROL	0x02
#define KEY_ALT		0x03
#define KEY_ESCAPE	0x04
#define KEY_TAB	        0x05
#define KEY_INSERT	0x06
#define KEY_DELETE      0x07
#define KEY_UP		0x08
#define KEY_DOWN	0x09
#define KEY_LEFT	0x0A
#define KEY_RIGHT	0x0B
#define KEY_PRIOR	0x0C
#define KEY_NEXT        0x0D
#define KEY_PLUS	'+'
#define KEY_MINUS	'-'

char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);

#endif

#endif

#ifdef LC_QT
#define LC_MAXPATH 1024//MAXPATHLEN //FILENAME_MAX

#define KEY_SHIFT	Qt::SHIFT
#define KEY_CONTROL	Qt::CTRL
#define KEY_ALT		Qt::ALT
#define KEY_ESCAPE	Qt::Key_Escape
#define KEY_TAB	    Qt::Key_Tab
#define KEY_INSERT	Qt::Key_Insert
#define KEY_DELETE  Qt::Key_Delete
#define KEY_UP		Qt::Key_Up
#define KEY_DOWN	Qt::Key_Down
#define KEY_LEFT	Qt::Key_Left
#define KEY_RIGHT	Qt::Key_Right
#define KEY_PRIOR	Qt::Key_PageUp
#define KEY_NEXT    Qt::Key_PageDown
#define KEY_PLUS	Qt::Key_Plus
#define KEY_MINUS	Qt::Key_Minus

#ifndef WIN32
char* strupr(char* string);
char* strlwr(char* string);
int stricmp(const char* str1, const char* str2);
#endif

#endif


/////////////////////////////////////////////////////////////////////////////
// LeoCAD constants

#ifndef WIN32
#define RGB(r, g, b) ((unsigned long)(((unsigned char) (r) | ((unsigned short) (g) << 8))|(((unsigned long) (unsigned char) (b)) << 16))) 
#endif 

#define FLOATRGB(f) RGB(f[0]*255, f[1]*255, f[2]*255)

#define LC_FOURCC(ch0, ch1, ch2, ch3) (lcuint32)((lcuint32)(lcuint8)(ch0) | ((lcuint32)(lcuint8)(ch1) << 8) | \
                                                ((lcuint32)(lcuint8)(ch2) << 16) | ((lcuint32)(lcuint8)(ch3) << 24 ))

#define LC_FILE_ID LC_FOURCC('L','C','D', 0)

#define LC_STR_VERSION	"LeoCAD 0.7 Project\0\0" // char[20]

#define LC_TERRAIN_FLAT			0x01	// Flat terrain
#define LC_TERRAIN_TEXTURE		0x02	// Use texture
#define LC_TERRAIN_SMOOTH		0x04	// Smooth shading

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
