#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "typedefs.h"
#include "opengl.h"
#include "array.h"
#include "lc_math.h"

typedef enum 
{
	LC_TRACK_NONE,
	LC_TRACK_START_LEFT,
	LC_TRACK_LEFT,
	LC_TRACK_START_RIGHT,
	LC_TRACK_RIGHT
} LC_MOUSE_TRACK;

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
	LC_OVERLAY_ROTATE_VIEW_XYZ,
};

class Piece;
class Camera;
class Light;
class Group;
class Texture;
class Terrain;
class PieceInfo;
class Matrix;
class View;
class Image;
class PiecesLibrary;
class TexFont;

// Undo support

#include "lc_file.h"

struct LC_UNDOINFO
{
	lcMemFile file;
	char strText[21];
	LC_UNDOINFO* pNext;
	LC_UNDOINFO() { pNext = NULL; };
};

struct LC_FILEENTRY
{
	lcMemFile File;
	char FileName[LC_MAXPATH];
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
	void SetModifiedFlag(bool bModified)
		{ m_bModified = bModified; }

	// Access to protected members
	unsigned char GetLastStep();
	bool IsAnimation()
		{ return m_bAnimation; }
	void SetAnimation(bool Anim)
	{ m_bAnimation = Anim; } // only to be called from lcApplication::Initialize()
	unsigned short GetCurrentTime ()
		{ return m_bAnimation ? m_nCurFrame : m_nCurStep; }
	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	int GetCurrentColor () const
		{ return m_nCurColor; }
	float* GetBackgroundColor()
		{ return m_fBackground; }
	unsigned long GetSnap() const
		{ return m_nSnap; }
	int GetOverlayMode() const
		{ return m_OverlayMode; }
	void GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;
	void GetSnapDistanceText(char* SnapXY, char* SnapZ) const;
	Camera* GetCamera(int i);
	void GetTimeRange(int* from, int* to)
	{
		*from = m_bAnimation ? m_nCurFrame : m_nCurStep;
		*to = m_bAnimation ? m_nTotalFrames : 255;
	}
	unsigned short GetTotalFrames () const
		{ return m_nTotalFrames; }

	void ConvertToUserUnits(lcVector3& Value) const;
	void ConvertFromUserUnits(lcVector3& Value) const;
	void GetArrays(Piece** ppPiece, Camera** ppCamera, Light** ppLight)
	{
		*ppPiece = m_pPieces;
		*ppCamera = m_pCameras;
		*ppLight = m_pLights;
	}

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	// Special notifications
	void DeleteContents(bool bUndo); // delete doc items etc
	void LoadDefaults(bool cameras);
	void BeginPieceDrop(PieceInfo* Info);
	void BeginColorDrop();

	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void Render(View* view, bool bToMemory);
	void CheckAutoSave();
	bool GetSelectionCenter(lcVector3& Center) const;
	bool GetFocusPosition(lcVector3& Position) const;
	Object* GetFocusObject() const;
	Group* AddGroup (const char* name, Group* pParent, float x, float y, float z);
	void TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform);

	void AddView(View* view);
	void RemoveView(View* view);
	void UpdateAllViews();
	bool SetActiveView(View* view);
	View* GetActiveView() const
	{
		return m_ActiveView;
	}

// Implementation
protected:
	// default implementation
	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;    // changed since last saved

	View* m_ActiveView;
	PtrArray<View> m_ViewList;

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

	// Objects
	Piece* m_pPieces;
	Camera* m_pCameras;
	Light* m_pLights;
	Group* m_pGroups;
	Terrain* m_pTerrain;
	lcFile* m_pClipboard[10];
	unsigned char m_nCurClipboard;

	void AddPiece(Piece* pPiece);
	void RemovePiece(Piece* pPiece);
	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation);
	void GetPieceInsertPosition(View* view, int MouseX, int MouseY, lcVector3& Position, lcVector4& Orientation);
	Object* FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly = false);
	void FindObjectsInBox(float x1, float y1, float x2, float y2, PtrArray<Object>& Objects);
	void SelectAndFocusNone(bool bFocusOnly);
	void CalculateStep();

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
	// Call these functions from each OS
	void OnLeftButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(View* view, int x, int y, bool bControl, bool bShift);
	void OnMiddleButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnMiddleButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnMouseMove(View* view, int x, int y, bool bControl, bool bShift);
	bool OnKeyDown(char nKey, bool bControl, bool bShift);

	void SetAction(int Action);
	int GetCurAction() const
	{
		return m_nCurAction;
	}
	int GetAction() const;

	void HandleNotify(LC_NOTIFY id, unsigned long param);
	void HandleCommand(LC_COMMANDS id, unsigned long nParam);
	void HandleMessage(int Message, void* Data);

protected:
	// State variables
	int m_nCurAction;
	int m_PreviousAction;
	bool m_RestoreAction;
	PieceInfo* m_pCurPiece;
	PieceInfo* m_PreviousPiece;
	unsigned char m_nCurColor;
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
	char m_strModelsPath[LC_MAXPATH];
	char m_strBackground[LC_MAXPATH];
	Texture* m_pBackground;

protected:
	// File load/save implementation.
	bool DoSave(char* PathName, bool bReplace);
	bool DoFileSave();
	bool FileLoad(lcFile* file, bool bUndo, bool bMerge);
	void FileSave(lcFile* file, bool bUndo);
	void FileReadLDraw(lcFile* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, PtrArray<LC_FILEENTRY>& FileArray);
	void FileReadMPD(lcFile& MPD, PtrArray<LC_FILEENTRY>& FileArray) const;

public:
	// File helpers
	bool OnNewDocument();
	bool OnOpenDocument(const char* FileName);
	bool OpenProject(const char* FileName);
	bool SaveModified();

protected:
	// mail enabling
//	void OnFileSendMail();
//	void OnUpdateFileSendMail(CCmdUI* pCmdUI);

	// TODO: Fix ! This is a hack to make things work now
	friend class CCADView;
	friend void PrintPiecesThread(void* pv);
	friend void Export3DStudio();
};

#endif // _PROJECT_H_
