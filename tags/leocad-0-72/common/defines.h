// Constant definitions.
//

#ifndef _DEFINES_H_
#define _DEFINES_H_

/////////////////////////////////////////////////////////////////////////////
// System specific

#if ! ( defined( LC_WINDOWS ) || defined( LC_LINUX ) || defined( LC_MACINTOSH ))
#error  YOU NEED TO DEFINE YOUR OS
#endif

#ifdef LC_WINDOWS
#define LC_MAXPATH _MAX_PATH
#define KEY_SHIFT	VK_SHIFT
#define KEY_CONTROL	VK_CONTROL
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
#define LC_MAXPATH 1024 //FILENAME_MAX
#define KEY_SHIFT	0x01
#define KEY_CONTROL	0x02
#define KEY_ESCAPE	0x03
#define KEY_TAB	        0x04
#define KEY_INSERT	0x05
#define KEY_DELETE      0x06
#define KEY_UP		0x07
#define KEY_DOWN	0x08
#define KEY_LEFT	0x09
#define KEY_RIGHT	0x0A
#define KEY_PRIOR	0x0B
#define KEY_NEXT        0x0C
#define KEY_PLUS	'+'
#define KEY_MINUS	'-'

char* strupr(char* string);
char* strlwr(char* string);

#endif

// Endianess handling (files are little endian).
#ifdef LC_BIG_ENDIAN
unsigned short SwapSHORT(unsigned short x)
{ return (x>>8) | (x<<8); }
unsigned long SwapLONG(unsigned long x)
{ return (x>>24) | ((x>>8) & 0xff00) | ((x<<8) & 0xff0000) | (x<<24); }
#define SHORT(x) ((short)SwapSHORT((unsigned short) (x)))
#define LONG(x)  ((long)SwapLONG((unsigned long) (x)))
#else
#define SHORT(x) (x)
#define LONG(x)  (x)
#endif

/////////////////////////////////////////////////////////////////////////////
// LeoCAD constants

// Math numbers
#define DTOR 0.017453f
#define RTOD 57.29578f
#define PI  3.14159265
#define PI2 6.28318530

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)	(((a) > 0) ? (a) : -(a))
#endif

#ifndef RGB
#define RGB(r, g, b) ((unsigned long)(((unsigned char) (r) | ((unsigned short) (g) << 8))|(((unsigned long) (unsigned char) (b)) << 16))) 
#endif 

#define FLOATRGB(f) RGB(f[0]*255, f[1]*255, f[2]*255)

#define LC_CONNECTIONS	2		// Different piece connections
#define LC_APP_VERSION	0.71f	// Current version
#define LC_STR_VERSION	"LeoCAD 0.7 Project\0\0" // char[20]

#define LC_MAXCOLORS	28	// Number of colors supported
#define LC_COL_EDGES	28	// Piece edges
#define LC_COL_SELECTED	29	// Selected object
#define LC_COL_FOCUSED	30	// Focused object
#define LC_COL_DEFAULT	31	// Default piece color


//	#define	DET_BACKFACES	0x00001	// Draw backfaces
//	#define	DET_DEPTH		0x00002	// Enable depth test
//	#define	DET_CLEAR		0x00004	// Use clear colors
#define	LC_DET_LIGHTING		0x00008	// Lighting
#define	LC_DET_SMOOTH		0x00010	// Smooth shading
//	#define	DET_STUDS		0x00020	// Draw studs
//	#define	DET_WIREFRAME	0x00040	// Wireframe
#define	LC_DET_ANTIALIAS	0x00080	// Turn on anti-aliasing
#define LC_DET_BRICKEDGES	0x00100	// Draw lines
#define LC_DET_DITHER		0x00200	// Enable dithering
#define LC_DET_BOX_FILL		0x00400	// Filled boxes
#define LC_DET_HIDDEN_LINE	0x00800	// Remove hidden lines
//	#define	DET_STUDS_BOX	0x01000	// Draw studs as boxes
#define LC_DET_LINEAR		0x02000	// Linear filtering
#define LC_DET_FAST			0x04000	// Fast rendering (boxes)
#define LC_DET_BACKGROUND	0x08000	// Background rendering
#define LC_DET_SCREENDOOR	0x10000	// No alpha blending

#define LC_DRAW_AXIS		0x0001	// Orientation icon
#define LC_DRAW_GRID		0x0002	// Grid
#define LC_DRAW_SNAP_A		0x0004	// Snap Angle
#define LC_DRAW_SNAP_X		0x0008	// Snap X
#define LC_DRAW_SNAP_Y		0x0010	// Snap Y
#define LC_DRAW_SNAP_Z		0x0020	// Snap Z
//#define DRAW_COLLISION	0x0040
#define LC_DRAW_MOVE		0x0080	// Switch to move after insert
#define LC_DRAW_LOCK_X		0x0100	// Lock X
#define LC_DRAW_LOCK_Y		0x0200	// Lock Y
#define LC_DRAW_LOCK_Z		0x0400	// Lock Z
#define LC_DRAW_MOVEAXIS	0x0800	// Move on fixed axis
#define LC_DRAW_PREVIEW		0x1000	// Show piece position
#define LC_DRAW_CM_UNITS	0x2000	// Use centimeters
#define LC_DRAW_3DMOUSE		0x4000	// Mouse moves in all directions

//	#define RENDER_FAST			0x001
//	#define RENDER_BACKGROUND	0x002
#define LC_SCENE_FOG			0x004	// Enable fog
//	#define RENDER_FOG_BG		0x008	// Use bg color for fog
#define LC_SCENE_BG				0x010	// Draw bg image
//	#define RENDER_BG_FAST	0x020
#define LC_SCENE_BG_TILE		0x040	// Tile bg image
#define LC_SCENE_FLOOR			0x080	// Render floor
#define LC_SCENE_GRADIENT		0x100	// Draw gradient

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

// Image Options
#define LC_IMAGE_PROGRESSIVE	0x1000
#define LC_IMAGE_TRANSPARENT	0x2000
#define LC_IMAGE_HIGHCOLOR		0x4000
#define LC_IMAGE_MASK			0x7000

// Bounding box owner
typedef enum { LC_PIECE, LC_CAMERA, LC_CAMERA_TARGET,
		LC_LIGHT, LC_LIGHT_TARGET } LC_OBJ_TYPES;

#define LC_UPDATE_OBJECT	0x40
#define LC_UPDATE_TYPE		0x80

// Piece library update
#define LC_UPDATE_DELETE       0x00
#define LC_UPDATE_DESCRIPTION  0x01
#define LC_UPDATE_DRAWINFO     0x02
#define LC_UPDATE_NEWPIECE     0x04

#endif // _DEFINES_H_
