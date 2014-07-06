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
	LC_TRACK_LEFT,
	LC_TRACK_RIGHT
};

class Terrain;
class PieceInfo;
class View;
class Image;
class TexFont;

#include "lc_file.h"

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

#include "lc_model.h"

class Project : public lcModel
{
public:
	Project();
	~Project();

	void RayTest(lcObjectRayTest& ObjectRayTest) const;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	void FocusOrDeselectObject(const lcObjectSection& ObjectSection);
	void ClearSelectionAndSetFocus(Object* Object, lcuint32 Section);
	void ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection)
	{
		ClearSelectionAndSetFocus(ObjectSection.Object, ObjectSection.Section);
	}
	void SetSelection(const lcArray<lcObjectSection>& ObjectSections);
	void AddToSelection(const lcArray<lcObjectSection>& ObjectSections);

	const lcVector3& GetMouseToolDistance() const
	{
		return mMouseToolDistance;
	}

	void BeginMouseTool();
	void EndMouseTool(lcTool Tool, bool Accept);
	void InsertPieceToolClicked(const lcVector3& Position, const lcVector4& Rotation);
	void PointLightToolClicked(const lcVector3& Position);
	void BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateSpotLightTool(const lcVector3& Target);
	void BeginCameraTool(const lcVector3& Position, const lcVector3& Target);
	void UpdateCameraTool(const lcVector3& Target);
	void UpdateMoveTool(const lcVector3& Distance);
	void UpdateRotateTool(const lcVector3& Angles);
	void EraserToolClicked(lcObject* Object);
	void PaintToolClicked(lcObject* Object);
	void UpdateZoomTool(lcCamera* Camera, float Mouse);
	void UpdatePanTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY);
	void UpdateRollTool(lcCamera* Camera, float Mouse);
	void ZoomRegionToolClicked(lcCamera* Camera, const lcVector3* Points, float RatioX, float RatioY);

protected:
	lcVector3 mMouseToolDistance;

public:
	void SetCurrentStep(lcStep Step)
	{
		mCurrentStep = Step;
		CalculateStep();
	}

	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	float* GetBackgroundColor() // todo: remove
		{ return mProperties.mBackgroundSolidColor; }
	unsigned long GetSnap() const
		{ return m_nSnap; }
	void GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const;
	void GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;

	int GetGroupIndex(lcGroup* Group) const
	{
		return mGroups.FindIndex(Group);
	}

	void ConvertToUserUnits(lcVector3& Value) const;
	void ConvertFromUserUnits(lcVector3& Value) const;
	lcMatrix44 GetRelativeRotation() const;

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	void DeleteContents(bool bUndo);
	void LoadDefaults(bool cameras);
	void RenderInitialize();

	void GetPiecesUsed(lcArray<lcPiecesUsedEntry>& PiecesUsed) const;
	void CreateImages(Image* images, int width, int height, lcStep from, lcStep to, bool hilite);
	void Render(View* view, bool bToMemory);
	void CheckAutoSave();
	bool GetSelectionCenter(lcVector3& Center) const;
	lcVector3 GetFocusOrSelectionCenter() const;
	bool GetFocusPosition(lcVector3& Position) const;
	Object* GetFocusObject() const;
	bool AnyObjectsSelected(bool PiecesOnly) const;
	lcGroup* AddGroup(lcGroup* Parent);
	void TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform);
	void ModifyObject(Object* Object, lcObjectProperty Property, void* Value);
	void ZoomActiveView(int Amount);

	Terrain* m_pTerrain;

	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];

	void GetPieceInsertPosition(View* view, lcVector3& Position, lcVector4& Orientation);

	TexFont* m_pScreenFont;

protected:
	void CheckPoint(const char* Description);

	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);
	void SelectAndFocusNone(bool bFocusOnly);
	void SelectGroup(Group* TopGroup, bool Select);

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

	void CreateHTMLPieceList(FILE* f, lcStep Step, bool bImages, const char* ext);
	void Export3DStudio();
	void ExportPOVRay(lcFile& File);
	void ZoomExtents(int FirstView, int LastView);

	void UpdateSelection();
	void RemoveEmptyGroups();

public:
	void OnMouseWheel(View* view, float Direction);
	void HandleCommand(LC_COMMANDS id);

	lcuint32 m_nSnap;

protected:
	int mTransformType;
	PieceInfo* m_pCurPiece;

	lcuint16 m_nMoveSnap;
	lcuint16 m_nAngleSnap;
	char m_strFooter[256];
	char m_strHeader[256];

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
};

#endif // _PROJECT_H_
