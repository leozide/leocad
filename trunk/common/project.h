#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "opengl.h"
#include "lc_array.h"
#include "lc_math.h"
#include "lc_commands.h"
#include "str.h"

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
//#define LC_DRAW_CM_UNITS       0x2000 // Use centimeters
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

enum LC_NOTIFY
{
	LC_CAPTURE_LOST
};

enum lcBackgroundType
{
	LC_BACKGROUND_SOLID,
	LC_BACKGROUND_GRADIENT,
	LC_BACKGROUND_IMAGE
};

class lcModelProperties
{
public:
	void LoadDefaults();
	void SaveDefaults();

	bool operator==(const lcModelProperties& Properties)
	{
		if (mName != Properties.mName || mAuthor != Properties.mAuthor ||
			mDescription != Properties.mDescription || mComments != Properties.mComments)
			return false;

		if (mBackgroundType != Properties.mBackgroundType || mBackgroundSolidColor != Properties.mBackgroundSolidColor ||
			mBackgroundGradientColor1 != Properties.mBackgroundGradientColor1 || mBackgroundGradientColor2 != Properties.mBackgroundGradientColor2 ||
			mBackgroundImage != Properties.mBackgroundImage || mBackgroundImageTile != Properties.mBackgroundImageTile)
			return false;

		if (mFogEnabled != Properties.mFogEnabled || mFogDensity != Properties.mFogDensity ||
			mFogColor != Properties.mFogColor || mAmbientColor != Properties.mAmbientColor)
			return false;

		return true;
	}

	String mName;
	String mAuthor;
	String mDescription;
	String mComments;

	lcBackgroundType mBackgroundType;
	lcVector3 mBackgroundSolidColor;
	lcVector3 mBackgroundGradientColor1;
	lcVector3 mBackgroundGradientColor2;
	String mBackgroundImage;
	bool mBackgroundImageTile;

	bool mFogEnabled;
	float mFogDensity;
	lcVector3 mFogColor;
	lcVector3 mAmbientColor;
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
	LC_PIECE_PROPERTY_POSITION,
	LC_PIECE_PROPERTY_ROTATION,
	LC_PIECE_PROPERTY_SHOW,
	LC_PIECE_PROPERTY_HIDE,
	LC_PIECE_PROPERTY_COLOR,
	LC_PIECE_PROPERTY_ID,
	LC_CAMERA_PROPERTY_POSITION,
	LC_CAMERA_PROPERTY_TARGET,
	LC_CAMERA_PROPERTY_UPVECTOR,
	LC_CAMERA_PROPERTY_ORTHO,
	LC_CAMERA_PROPERTY_FOV,
	LC_CAMERA_PROPERTY_NEAR,
	LC_CAMERA_PROPERTY_FAR,
	LC_CAMERA_PROPERTY_NAME
};

enum lcTool
{
	LC_TOOL_INSERT,
	LC_TOOL_LIGHT,
	LC_TOOL_SPOTLIGHT,
	LC_TOOL_CAMERA,
	LC_TOOL_SELECT,
	LC_TOOL_MOVE,
	LC_TOOL_ROTATE,
	LC_TOOL_ERASER,
	LC_TOOL_PAINT,
	LC_TOOL_ZOOM,
	LC_TOOL_PAN,
	LC_TOOL_ROTATE_VIEW,
	LC_TOOL_ROLL,
	LC_TOOL_ZOOM_REGION
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
	unsigned short GetCurrentTime ()
		{ return m_nCurStep; }
	void SetCurrentTime(unsigned short Time)
	{
		m_nCurStep = (unsigned char)Time;
		CalculateStep();
	}
	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	float* GetBackgroundColor() // todo: remove
		{ return mProperties.mBackgroundSolidColor; }
	unsigned long GetSnap() const
		{ return m_nSnap; }
	int GetOverlayMode() const
		{ return m_OverlayMode; }
	void GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const;
	void GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;
	void GetTimeRange(int* from, int* to)
	{
		*from = m_nCurStep;
		*to = 255;
	}

	void ConvertToUserUnits(lcVector3& Value) const;
	void ConvertFromUserUnits(lcVector3& Value) const;

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	void DeleteContents(bool bUndo);
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
	lcArray<Piece*> mPieces;
	lcArray<Camera*> mCameras;
	lcArray<Light*> mLights;
	Group* m_pGroups;
	Terrain* m_pTerrain;

	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;

protected:
	View* m_ActiveView;
	lcArray<View*> m_ViewList;

	// Piece library
	TexFont* m_pScreenFont;

	// Undo support
	LC_UNDOINFO* m_pUndoList;
	LC_UNDOINFO* m_pRedoList;
	bool m_bUndoOriginal;
	void CheckPoint (const char* text);

	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);
	void GetPieceInsertPosition(View* view, int MouseX, int MouseY, lcVector3& Position, lcVector4& Orientation);
	lcObjectSection FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly = false);
	lcArray<lcObjectSection> FindObjectsInBox(View* View, float x1, float y1, float x2, float y2);
	void SelectAndFocusNone(bool bFocusOnly);
	void SelectGroup(Group* TopGroup, bool Select);

	void FocusOrDeselectObject(const lcObjectSection& ObjectSection);
	void ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection)
	{
		ClearSelectionAndSetFocus(ObjectSection.Object, ObjectSection.Section);
	}
	void ClearSelectionAndSetFocus(Object* Object, lcuintptr Section);

	void CalculateStep();
	static int InstanceOfName(const String& existingString, const String& candidateString, String& baseNameOut );

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
	void RenderScenePieces(View* view, bool DrawInterface);
	void RenderSceneObjects(View* view);
	void RenderViewports(View* view);
	void RenderOverlays(View* view);

	void RenderInitialize();
	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext);
	void Export3DStudio();
	void ExportPOVRay(lcFile& File);
	void ZoomExtents(int FirstView, int LastView);

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

	lcuint32 m_nSnap;

protected:
	// State variables
	int mTransformType;
	int m_nCurAction;
	PieceInfo* m_pCurPiece;
	PieceInfo* mDropPiece;
	bool m_bAddKeys;
	unsigned char m_nCurStep;

	lcuint16 m_nMoveSnap;
	lcuint16 m_nAngleSnap;
	char m_strFooter[256];
	char m_strHeader[256];

	lcModelProperties mProperties;

	lcTexture* m_pBackground;
	lcTexture* mGridTexture;

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
