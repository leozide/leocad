// Typedefs.
//

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

class Group;
class Piece;
class PieceInfo;
#include "defines.h"

typedef enum {
	LC_COLOR_CHANGED,
	LC_GROUP_CHANGED,
	LC_CAPTURE_LOST,
	LC_ACTIVATE,
	LC_PIECE_MODIFIED,
	LC_CAMERA_MODIFIED,
	LC_LIGHT_MODIFIED
} LC_NOTIFY;

typedef enum {
	LC_FILE_NEW,
	LC_FILE_OPEN,
	LC_FILE_MERGE,
	LC_FILE_SAVE,
	LC_FILE_SAVEAS,
	LC_FILE_PICTURE,
	LC_FILE_3DS,
	LC_FILE_HTML,
	LC_FILE_POVRAY,
	LC_FILE_WAVEFRONT,
	LC_FILE_PROPERTIES,
	LC_FILE_TERRAIN,
	LC_FILE_LIBRARY,
	LC_FILE_RECENT,
	LC_EDIT_UNDO,
	LC_EDIT_REDO,
	LC_EDIT_CUT,
	LC_EDIT_COPY,
	LC_EDIT_PASTE,
	LC_EDIT_SELECT_ALL,
	LC_EDIT_SELECT_NONE,
	LC_EDIT_SELECT_INVERT,
	LC_EDIT_SELECT_BYNAME,
	LC_PIECE_INSERT,
	LC_PIECE_DELETE,
	LC_PIECE_MINIFIG,
	LC_PIECE_ARRAY,
	LC_PIECE_COPYKEYS,
	LC_PIECE_GROUP,
	LC_PIECE_UNGROUP,
	LC_PIECE_GROUP_ADD,
	LC_PIECE_GROUP_REMOVE,
	LC_PIECE_GROUP_EDIT,
	LC_PIECE_HIDE_SELECTED,
	LC_PIECE_HIDE_UNSELECTED,
	LC_PIECE_UNHIDE_ALL,
	LC_PIECE_PREVIOUS,
	LC_PIECE_NEXT,
	LC_VIEW_PREFERENCES,
	LC_VIEW_ZOOMIN,
	LC_VIEW_ZOOMOUT,
	LC_VIEW_ZOOMEXTENTS,
	LC_VIEW_VIEWPORTS,
	LC_VIEW_STEP_NEXT,
	LC_VIEW_STEP_PREVIOUS,
	LC_VIEW_STEP_FIRST,
	LC_VIEW_STEP_LAST,
	LC_VIEW_STEP_CHOOSE,
	LC_VIEW_STEP_SET,
	LC_VIEW_STOP,
	LC_VIEW_PLAY,
	LC_VIEW_CAMERA_MENU,
	LC_VIEW_CAMERA_RESET,
	LC_VIEW_AUTOPAN,
	LC_HELP_ABOUT,
	LC_TOOLBAR_ANIMATION,
	LC_TOOLBAR_ADDKEYS,
	LC_TOOLBAR_SNAPMENU,
	LC_TOOLBAR_LOCKMENU,
	LC_TOOLBAR_SNAPMOVEMENU,
	LC_TOOLBAR_FASTRENDER,
	LC_TOOLBAR_BACKGROUND
} LC_COMMANDS;

typedef enum { 
	LC_ACTION_SELECT = 0, 
	LC_ACTION_INSERT,
	LC_ACTION_LIGHT,
	LC_ACTION_SPOTLIGHT,
	LC_ACTION_CAMERA,
	LC_ACTION_MOVE,
	LC_ACTION_ROTATE,
	LC_ACTION_ERASER,
	LC_ACTION_PAINT,
	LC_ACTION_ZOOM,
	LC_ACTION_ZOOM_REGION,
	LC_ACTION_PAN,
	LC_ACTION_ROTATE_VIEW,
	LC_ACTION_ROLL,
	LC_ACTION_CURVE
} LC_ACTIONS;

// Piece connections (complicated and wastes memory but fast).

typedef struct CONNECTION
{
	unsigned char type;
	float center[3];
	float normal[3];
	CONNECTION* link;
	Piece* owner;
} CONNECTION;

typedef struct
{
	Piece* owner;
	CONNECTION** cons; // pointers to the structures in each piece
	unsigned short numcons;
} CONNECTION_ENTRY;

typedef struct
{
	CONNECTION_ENTRY* entries;
	unsigned short numentries;
} CONNECTION_TYPE;

// Select by Name dialog data

typedef enum
{
	LC_SELDLG_PIECE,
	LC_SELDLG_CAMERA,
	LC_SELDLG_LIGHT,
	LC_SELDLG_GROUP
} LC_SEL_DATA_TYPE;

typedef struct
{
	const char* name;
	unsigned char type;
	bool selected;
	void* pointer;
} LC_SEL_DATA;

typedef struct
{
	void* piece;
	char name[81];
	float pos[3];
	float rot[3];
	int from;
	int to;
	bool hidden;
	int color;
} LC_PIECE_MODIFY;

typedef struct
{
	void* camera;
	char name[81];
	float eye[3];
	float target[3];
	float up[3];
	float fovy;
	float znear;
	float zfar;
	bool hidden;
} LC_CAMERA_MODIFY;

// Image

typedef enum {
	LC_IMAGE_BMP,
	LC_IMAGE_GIF,
	LC_IMAGE_JPG,
	LC_IMAGE_PNG,
	LC_IMAGE_AVI
} LC_IMAGE_FORMATS;


typedef struct
{
	unsigned short width;
	unsigned short height;
	void* bits;
} LC_IMAGE;

typedef struct
{
	unsigned char quality;
	bool interlaced;
	bool transparent;
	bool truecolor;
	unsigned char background[3];
	float pause;
	unsigned char format;
} LC_IMAGE_OPTS;

typedef struct
{
	char filename[LC_MAXPATH];
	unsigned short from;
	unsigned short to;
	bool multiple;
	unsigned short width;
	unsigned short height;
	LC_IMAGE_OPTS imopts;
} LC_IMAGEDLG_OPTS;

typedef enum {
	LC_DLG_FILE_OPEN,
	LC_DLG_FILE_SAVE,
	LC_DLG_FILE_MERGE,
	LC_DLG_PICTURE_SAVE,
	LC_DLG_HTML,
	LC_DLG_POVRAY,
	LC_DLG_WAVEFRONT,
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
	LC_DLG_ABOUT
} LC_DIALOGS;

typedef struct
{
	bool render;
	char povpath[LC_MAXPATH];
	char outpath[LC_MAXPATH];
	char libpath[LC_MAXPATH];
} LC_POVRAYDLG_OPTS;

typedef struct
{
	char path[LC_MAXPATH];
	bool singlepage;
	bool index;
	bool images;
	bool listend;
	bool liststep;
	bool hilite;
	LC_IMAGEDLG_OPTS imdlg;
} LC_HTMLDLG_OPTS;

typedef struct
{
	unsigned short n1DCount;
	unsigned short n2DCount;
	unsigned short n3DCount;
	unsigned char nArrayDimension;
	float f2D[3];
	float f3D[3];
	float fMove[3];
	float fRotate[3];
} LC_ARRAYDLG_OPTS;

typedef struct
{
	char strAuthor[101];
	char strDescription[101];
	char strComments[256];
	char* strTitle;
	char* strFilename;
	char** names;
	unsigned short* count;
	int lines;
} LC_PROPERTIESDLG_OPTS;

typedef struct
{
	int piececount;
	Piece** pieces;
	Group** piecesgroups;
	int groupcount;
	Group** groups;
	Group** groupsgroups;
} LC_GROUPEDITDLG_OPTS;

typedef struct
{
	int nMouse;
	int nSaveInterval;
	char strPath[LC_MAXPATH];
	unsigned long nDetail;
	float fLineWidth;
	unsigned long nSnap;
	unsigned short nAngleSnap;
	unsigned short nGridSize;
	unsigned long nScene;
	float fDensity;
	char strBackground[LC_MAXPATH];
	float fBackground[4];
	float fFog[4];
	float fAmbient[4];
	float fGrad1[3];
	float fGrad2[3];
	char strFooter[256];
	char strHeader[256];
} LC_PREFERENCESDLG_OPTS;

#endif
