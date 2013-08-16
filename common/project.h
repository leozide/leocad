#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "opengl.h"
#include "lc_array.h"
#include "lc_math.h"
#include "lc_commands.h"

//#define DET_BACKFACES	0x00001	// Draw backfaces
//#define DET_DEPTH		0x00002	// Enable depth test
//#define DET_CLEAR		0x00004	// Use clear colors
#define	LC_DET_LIGHTING		0x00008	// Lighting
//#define	LC_DET_SMOOTH		0x00010	// Smooth shading
//#define DET_STUDS		0x00020	// Draw studs
//#define DET_WIREFRAME	0x00040	// Wireframe
//#define LC_DET_ANTIALIAS	0x00080	// Turn on anti-aliasing
#define LC_DET_BRICKEDGES	0x00100	// Draw lines
//#define LC_DET_DITHER		0x00200	// Enable dithering
//#define LC_DET_BOX_FILL		0x00400	// Filled boxes
//#define LC_DET_HIDDEN_LINE	0x00800	// Remove hidden lines
//#define DET_STUDS_BOX	0x01000	// Draw studs as boxes
//#define LC_DET_LINEAR		0x02000	// Linear filtering
#define LC_DET_FAST			0x04000	// Fast rendering (boxes)
//#define LC_DET_BACKGROUND	0x08000	// Background rendering
//#define LC_DET_SCREENDOOR	0x10000	// No alpha blending

#define LC_DRAW_AXIS           0x0001 // Orientation icon
#define LC_DRAW_GRID           0x0002 // Grid
#define LC_DRAW_SNAP_A         0x0004 // Snap Angle
#define LC_DRAW_SNAP_X         0x0008 // Snap X
#define LC_DRAW_SNAP_Y         0x0010 // Snap Y
#define LC_DRAW_SNAP_Z         0x0020 // Snap Z
#define LC_DRAW_SNAP_XYZ       (LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z)
#define LC_DRAW_GLOBAL_SNAP    0x0040 // Don't allow relative snap.
//#define LC_DRAW_MOVE           0x0080 // Switch to move after insert
#define LC_DRAW_LOCK_X         0x0100 // Lock X
#define LC_DRAW_LOCK_Y         0x0200 // Lock Y
#define LC_DRAW_LOCK_Z         0x0400 // Lock Z
#define LC_DRAW_LOCK_XYZ       (LC_DRAW_LOCK_X | LC_DRAW_LOCK_Y | LC_DRAW_LOCK_Z)
#define LC_DRAW_MOVEAXIS       0x0800 // Move on fixed axis
//#define LC_DRAW_PREVIEW        0x1000 // Show piece position
#define LC_DRAW_CM_UNITS       0x2000 // Use centimeters
//#define LC_DRAW_3DMOUSE        0x4000 // Mouse moves in all directions

//	#define RENDER_FAST			0x001
//	#define RENDER_BACKGROUND	0x002
#define LC_SCENE_FOG			0x004	// Enable fog
//	#define RENDER_FOG_BG		0x008	// Use bg color for fog
#define LC_SCENE_BG				0x010	// Draw bg image
//	#define RENDER_BG_FAST	0x020
#define LC_SCENE_BG_TILE		0x040	// Tile bg image
#define LC_SCENE_FLOOR			0x080	// Render floor
#define LC_SCENE_GRADIENT		0x100	// Draw gradient

#define LC_HTML_SINGLEPAGE      0x01
#define LC_HTML_INDEX           0x02
#define LC_HTML_IMAGES          0x04
#define LC_HTML_LISTEND         0x08
#define LC_HTML_LISTSTEP        0x10
#define LC_HTML_HIGHLIGHT       0x20
//#define LC_HTML_HTMLEXT         0x40
//#define LC_HTML_LISTID          0x80

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

enum LC_NOTIFY
{
	LC_CAPTURE_LOST
};

enum LC_TRANSFORM_TYPE
{
	LC_TRANSFORM_ABSOLUTE_TRANSLATION,
	LC_TRANSFORM_RELATIVE_TRANSLATION,
	LC_TRANSFORM_ABSOLUTE_ROTATION,
	LC_TRANSFORM_RELATIVE_ROTATION
};

enum LC_MOUSE_TRACK
{
	LC_TRACK_NONE,
	LC_TRACK_START_LEFT,
	LC_TRACK_LEFT,
	LC_TRACK_START_RIGHT,
	LC_TRACK_RIGHT
};

// Mouse control overlays.
enum LC_OVERLAY_MODES
{
	LC_OVERLAY_NONE,
	LC_OVERLAY_MOVE_X,
	LC_OVERLAY_MOVE_Y,
	LC_OVERLAY_MOVE_Z,
	LC_OVERLAY_MOVE_XY,
	LC_OVERLAY_MOVE_XZ,
	LC_OVERLAY_MOVE_YZ,
	LC_OVERLAY_MOVE_XYZ,
	LC_OVERLAY_ROTATE_X,
	LC_OVERLAY_ROTATE_Y,
	LC_OVERLAY_ROTATE_Z,
	LC_OVERLAY_ROTATE_XY,
	LC_OVERLAY_ROTATE_XZ,
	LC_OVERLAY_ROTATE_YZ,
	LC_OVERLAY_ROTATE_XYZ,
	LC_OVERLAY_ZOOM,
	LC_OVERLAY_PAN,
	LC_OVERLAY_ROTATE_VIEW_X,
	LC_OVERLAY_ROTATE_VIEW_Y,
	LC_OVERLAY_ROTATE_VIEW_Z,
	LC_OVERLAY_ROTATE_VIEW_XYZ
};

class Piece;
class Camera;
class Light;
class Group;
class Terrain;
class PieceInfo;
class View;
class Image;
class TexFont;

// Undo support

#include "lc_file.h"

struct LC_UNDOINFO
{
	lcMemFile file;
	char strText[21];
	LC_UNDOINFO* pNext;
	LC_UNDOINFO() { pNext = NULL; }
};

struct LC_FILEENTRY
{
	lcMemFile File;
	char FileName[LC_MAXPATH];
};

struct lcPiecesUsedEntry
{
	PieceInfo* Info;
	int ColorIndex;
	int Count;
};

struct lcSearchOptions
{
	bool MatchInfo;
	bool MatchColor;
	bool MatchName;
	PieceInfo* Info;
	int ColorIndex;
	char Name[256];
};

enum lcObjectProperty
{
	LC_PART_POSITION,
	LC_PART_ROTATION,
	LC_PART_SHOW,
	LC_PART_HIDE,
	LC_PART_COLOR,
	LC_PART_ID,
	LC_CAMERA_POSITION,
	LC_CAMERA_TARGET,
	LC_CAMERA_UP,
	LC_CAMERA_FOV,
	LC_CAMERA_NEAR,
	LC_CAMERA_FAR,
	LC_CAMERA_NAME
};

enum LC_ACTIONS
{
	LC_ACTION_INSERT,
	LC_ACTION_LIGHT,
	LC_ACTION_SPOTLIGHT,
	LC_ACTION_CAMERA,
	LC_ACTION_SELECT,
	LC_ACTION_MOVE,
	LC_ACTION_ROTATE,
	LC_ACTION_ERASER,
	LC_ACTION_PAINT,
	LC_ACTION_ZOOM,
	LC_ACTION_PAN,
	LC_ACTION_ROTATE_VIEW,
	LC_ACTION_ROLL,
	LC_ACTION_ZOOM_REGION
};

class Project
{
public:
// Constructors
	Project();
	~Project();

// Attributes
public:
	bool IsModified()
		{ return m_bModified; }

	// Access to protected members
	unsigned char GetLastStep();
	bool IsAnimation()
		{ return m_bAnimation; }
	void SetAnimation(bool Anim)
	{ m_bAnimation = Anim; } // only to be called from lcApplication::Initialize()
	unsigned short GetCurrentTime ()
		{ return m_bAnimation ? m_nCurFrame : m_nCurStep; }
	void SetCurrentTime(unsigned short Time)
	{
		if (m_bAnimation)
			m_nCurFrame = Time;
		else
			m_nCurStep = (unsigned char)Time;
		CalculateStep();
	}
	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	float* GetBackgroundColor()
		{ return m_fBackground; }
	unsigned long GetSnap() const
		{ return m_nSnap; }
	int GetOverlayMode() const
		{ return m_OverlayMode; }
	void GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const;
	void GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;
	void GetTimeRange(int* from, int* to)
	{
		*from = m_bAnimation ? m_nCurFrame : m_nCurStep;
		*to = m_bAnimation ? m_nTotalFrames : 255;
	}
	unsigned short GetTotalFrames () const
		{ return m_nTotalFrames; }

	void ConvertToUserUnits(lcVector3& Value) const;
	void ConvertFromUserUnits(lcVector3& Value) const;

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	// Special notifications
	void DeleteContents(bool bUndo); // delete doc items etc
	void LoadDefaults(bool cameras);
	void BeginPieceDrop(PieceInfo* Info);
	void OnPieceDropMove(int x, int y);
	void EndPieceDrop(bool Accept);
	void BeginColorDrop();

	void GetPiecesUsed(lcArray<lcPiecesUsedEntry>& PiecesUsed) const;
	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void Render(View* view, bool bToMemory);
	void CheckAutoSave();
	bool GetSelectionCenter(lcVector3& Center) const;
	bool GetFocusPosition(lcVector3& Position) const;
	Object* GetFocusObject() const;
	Group* AddGroup (const char* name, Group* pParent, float x, float y, float z);
	void TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform);
	void ModifyObject(Object* Object, lcObjectProperty Property, void* Value);
	void ZoomActiveView(int Amount);

	void AddView(View* view);
	void RemoveView(View* view);
	void UpdateAllViews();
	bool SetActiveView(View* view);
	View* GetActiveView() const
	{
		return m_ActiveView;
	}

	// Objects
	Piece* m_pPieces;
	lcArray<Camera*> mCameras;
	Light* m_pLights;
	Group* m_pGroups;
	Terrain* m_pTerrain;

	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;

// Implementation
protected:
	View* m_ActiveView;
	lcArray<View*> m_ViewList;

	char m_strAuthor[101];
	char m_strDescription[101];
	char m_strComments[256];

	// Piece library
	TexFont* m_pScreenFont;

	// Undo support
	LC_UNDOINFO* m_pUndoList;
	LC_UNDOINFO* m_pRedoList;
	bool m_bUndoOriginal;
	void CheckPoint (const char* text);

	void AddPiece(Piece* pPiece);
	void RemovePiece(Piece* pPiece);
	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);
	void GetPieceInsertPosition(View* view, int MouseX, int MouseY, lcVector3& Position, lcVector4& Orientation);
	Object* FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly = false);
	void FindObjectsInBox(float x1, float y1, float x2, float y2, lcArray<Object*>& Objects);
	void SelectAndFocusNone(bool bFocusOnly);
	void CalculateStep();

	void FindPiece(bool FindFirst, bool SearchForward);
	lcSearchOptions mSearchOptions;

	// Movement.
	bool MoveSelectedObjects(lcVector3& Move, lcVector3& Remainder, bool Snap, bool Lock);
	bool RotateSelectedObjects(lcVector3& Delta, lcVector3& Remainder, bool Snap, bool Lock);
	void SnapVector(lcVector3& Delta) const
	{
		lcVector3 Dummy;
		SnapVector(Delta, Dummy);
	}
	void SnapVector(lcVector3& Delta, lcVector3& Leftover) const;
	void SnapRotationVector(lcVector3& Delta, lcVector3& Leftover) const;

	// Rendering functions.
	void RenderBackground(View* view);
	void RenderScenePieces(View* view);
	void RenderSceneBoxes(View* view);
	void RenderSceneObjects(View* view);
	void RenderViewports(View* view);
	void RenderOverlays(View* view);

	void RenderInitialize();
	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext);
	void Export3DStudio();
	void ZoomExtents(int FirstView, int LastView);

	bool m_bStopRender;
	lcFile* m_pTrackFile;
	bool m_bTrackCancel;
	int m_nTracking;
	int m_nDownX;
	int m_nDownY;
	float m_fTrack[3];
	int m_nMouse;
	lcVector3 m_MouseSnapLeftover;
	lcVector3 m_MouseTotalDelta;

	int m_OverlayMode;
	bool m_OverlayActive;
	lcVector3 m_OverlayCenter;
	lcVector3 m_OverlayTrackStart;
	lcVector3 m_OverlayDelta;
	void MouseUpdateOverlays(View* view, int x, int y);
	void ActivateOverlay(View* view, int Action, int OverlayMode);
	void UpdateOverlayScale();

	bool StopTracking(bool bAccept);
	void StartTracking(int mode);
	void UpdateSelection();
	void RemoveEmptyGroups();

public:
	void OnLeftButtonDown(View* view);
	void OnLeftButtonUp(View* view);
	void OnLeftButtonDoubleClick(View* view);
	void OnMiddleButtonDown(View* view);
	void OnMiddleButtonUp(View* view);
	void OnRightButtonDown(View* view);
	void OnRightButtonUp(View* view);
	void OnMouseMove(View* view);
	void OnMouseWheel(View* view, float Direction);

	void SetAction(int Action);
	int GetCurAction() const
	{
		return m_nCurAction;
	}
	int GetAction() const;

	void HandleNotify(LC_NOTIFY id, unsigned long param);
	void HandleCommand(LC_COMMANDS id);

protected:
	// State variables
	int mTransformType;
	int m_nCurAction;
	PieceInfo* m_pCurPiece;
	PieceInfo* mDropPiece;
	bool m_bAnimation;
	bool m_bAddKeys;
	unsigned char m_nFPS;
	unsigned char m_nCurStep;
	lcuint16 m_nCurFrame;
	lcuint16 m_nTotalFrames;

	lcuint32 m_nScene;
	lcuint32 m_nDetail;
	lcuint32 m_nSnap;
	lcuint16 m_nMoveSnap;
	lcuint16 m_nAngleSnap;
	lcuint16 m_nGridSize;
	float m_fLineWidth;
	float m_fFogDensity;
	float m_fFogColor[4];
	float m_fAmbient[4];
	float m_fBackground[4];
	float m_fGradient1[3];
	float m_fGradient2[3];
	char m_strFooter[256];
	char m_strHeader[256];

	GLuint m_nGridList;
	unsigned long m_nAutosave;
	unsigned long m_nSaveTimer;
	char m_strBackground[LC_MAXPATH];
	lcTexture* m_pBackground;

protected:
	// File load/save implementation.
	bool DoSave(const char* FileName);
	bool FileLoad(lcFile* file, bool bUndo, bool bMerge);
	void FileSave(lcFile* file, bool bUndo);
	void FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray);
	void FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const;

public:
	// File helpers
	bool OnNewDocument();
	bool OnOpenDocument(const char* FileName);
	bool OpenProject(const char* FileName);
	bool SaveModified();
	void SetModifiedFlag(bool Modified);
};

#endif // _PROJECT_H_
