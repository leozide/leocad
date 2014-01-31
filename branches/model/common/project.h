#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "opengl.h"
#include "lc_array.h"
#include "lc_math.h"
#include "lc_commands.h"

#define LC_DRAW_SNAP_A         0x0004 // Snap Angle
#define LC_DRAW_SNAP_X         0x0008 // Snap X
#define LC_DRAW_SNAP_Y         0x0010 // Snap Y
#define LC_DRAW_SNAP_Z         0x0020 // Snap Z
#define LC_DRAW_SNAP_XYZ       (LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z)
#define LC_DRAW_GLOBAL_SNAP    0x0040 // Don't allow relative snap.
#define LC_DRAW_LOCK_X         0x0100 // Lock X
#define LC_DRAW_LOCK_Y         0x0200 // Lock Y
#define LC_DRAW_LOCK_Z         0x0400 // Lock Z
#define LC_DRAW_LOCK_XYZ       (LC_DRAW_LOCK_X | LC_DRAW_LOCK_Y | LC_DRAW_LOCK_Z)
#define LC_DRAW_MOVEAXIS       0x0800 // Move on fixed axis
//#define LC_DRAW_PREVIEW        0x1000 // Show piece position
#define LC_DRAW_CM_UNITS       0x2000 // Use centimeters
//#define LC_DRAW_3DMOUSE        0x4000 // Mouse moves in all directions

#define LC_SCENE_FOG			0x004	// Enable fog
#define LC_SCENE_BG				0x010	// Draw bg image
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

enum LC_MOUSE_TRACK
{
	LC_TRACK_NONE,
	LC_TRACK_LEFT,
	LC_TRACK_RIGHT
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
	LC_PROPERTY_PART_POSITION,
	LC_PROPERTY_PART_ROTATION,
	LC_PROPERTY_PART_SHOW,
	LC_PROPERTY_PART_HIDE,
	LC_PROPERTY_PART_COLOR,
	LC_PROPERTY_PART_ID,
	LC_PROPERTY_CAMERA_POSITION,
	LC_PROPERTY_CAMERA_TARGET,
	LC_PROPERTY_CAMERA_UP,
	LC_PROPERTY_CAMERA_FOV,
	LC_PROPERTY_CAMERA_NEAR,
	LC_PROPERTY_CAMERA_FAR,
	LC_PROPERTY_CAMERA_NAME
};

class Project
{
public:
	Project();
	~Project();

public:
	bool IsModified()
		{ return m_bModified; }

	unsigned char GetLastStep();
	bool IsAnimation()
		{ return m_bAnimation; }
	unsigned short GetCurrentTime ()
		{ return m_nCurStep; }
	void SetCurrentTime(unsigned short Time)
	{
		m_nCurStep = (unsigned char)Time;
		CalculateStep();
	}
	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	unsigned long GetSnap() const
		{ return m_nSnap; }
	void GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const;
	void GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;
	unsigned short GetTotalFrames () const
		{ return m_nTotalFrames; }

	void ConvertToUserUnits(lcVector3& Value) const;
	void ConvertFromUserUnits(lcVector3& Value) const;

	void UpdateInterface();
	void SetPathName(const char* lpszPathName, bool bAddToMRU);
	void SetTitle(const char* lpszTitle);

public:
	void DeleteContents(bool bUndo);
	void LoadDefaults(bool cameras);
	void BeginColorDrop();

	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void Render(View* view, bool bToMemory);
	bool GetSelectionCenter(lcVector3& Center) const;
	Object* GetFocusObject() const;
	Group* AddGroup (const char* name, Group* pParent, float x, float y, float z);
//	void TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform);
	void ModifyObject(Object* Object, lcObjectProperty Property, void* Value);
	void ZoomActiveView(int Amount);
	void RenderInitialize();

	lcModel* mActiveModel;
	lcArray<lcModel*> mModels;

	// Objects
	Piece* m_pPieces;
	lcArray<Camera*> mCameras;
	Light* m_pLights;
	Group* m_pGroups;
	Terrain* m_pTerrain;

	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;

public:
	char m_strAuthor[101];
	char m_strDescription[101];
	char m_strComments[256];

	// Undo support
	LC_UNDOINFO* m_pUndoList;
	LC_UNDOINFO* m_pRedoList;
	bool m_bUndoOriginal;
	void CheckPoint (const char* text);

	void AddPiece(Piece* pPiece);
	void GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);
	void GetPieceInsertPosition(View* view, int MouseX, int MouseY, lcVector3& Position, lcVector4& Orientation);
	Object* FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly = false);
	void SelectAndFocusNone(bool bFocusOnly);
	void CalculateStep();

	void FindPiece(bool FindFirst, bool SearchForward);
	lcSearchOptions mSearchOptions;

	// Movement.
	lcVector3 SnapVector(const lcVector3& Distance) const;
	lcVector3 GetMoveDistance(const lcVector3& Distance, bool Snap, bool Lock);
	bool RotateSelectedObjects(lcVector3& Delta, lcVector3& Remainder, bool Snap, bool Lock);
	void SnapRotationVector(lcVector3& Delta, lcVector3& Leftover) const;

	// Rendering functions.
	void RenderSceneObjects(View* view);

	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext);
	void Export3DStudio();
	void ExportBrickLink();
	void ExportCSV();
	void ExportPOVRay(lcFile& File);
	void ZoomExtents(int FirstView, int LastView);

	lcVector3 m_MouseSnapLeftover;
	lcVector3 m_MouseTotalDelta;

	lcVector3 m_OverlayTrackStart;
	lcVector3 m_OverlayDelta;

	void UpdateSelection();
	void RemoveEmptyGroups();

public:
	void HandleCommand(LC_COMMANDS id);

	lcuint32 m_nSnap;

public:
	PieceInfo* m_pCurPiece;
	bool m_bAnimation;
	unsigned char m_nCurStep;
	lcuint16 m_nTotalFrames;

	lcuint16 m_nMoveSnap;
	lcuint16 m_nAngleSnap;
	char m_strFooter[256];
	char m_strHeader[256];

	char m_strBackground[LC_MAXPATH];
	lcTexture* m_pBackground;

protected:
	bool DoSave(const char* FileName);
	bool FileLoad(lcFile* file, bool bUndo, bool bMerge);
	void FileSave(lcFile* file, bool bUndo);
	void FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray);
	void FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const;

public:
	bool OnNewDocument();
	bool OnOpenDocument(const char* FileName);
	bool OpenProject(const char* FileName);
	bool SaveModified();
	void SetModifiedFlag(bool Modified);
};

#endif // _PROJECT_H_