#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "object.h"
#include "defines.h"
#include "typedefs.h"
#include "opengl.h"
#include "array.h"
#include "algebra.h"

typedef enum 
{
	LC_TRACK_NONE,
	LC_TRACK_START_LEFT,
	LC_TRACK_LEFT,
	LC_TRACK_START_RIGHT,
	LC_TRACK_RIGHT
} LC_MOUSE_TRACK;

// Mouse control overlays.
typedef enum
{
	LC_OVERLAY_XYZ,
	LC_OVERLAY_X,
	LC_OVERLAY_Y,
	LC_OVERLAY_Z,
	LC_OVERLAY_XY,
	LC_OVERLAY_XZ,
	LC_OVERLAY_YZ
} LC_OVERLAY_MODES;

class Group;
class Texture;
class Terrain;
class PieceInfo;
class Matrix;
class View;
class Image;
class PiecesLibrary;
class TexFont;

class lcModel;
class lcScene;

// Undo support

#include "file.h"

typedef struct LC_UNDOINFO
{
	FileMem file;
	char strText[21];
	LC_UNDOINFO* pNext;
	LC_UNDOINFO() { pNext = NULL; };
} LC_UNDOINFO;

#include "lc_model.h" // temp include for GetTime()

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

	// Access to protected members (TODO: clean up all this crap)
	lcModel* GetActiveModel() const
	{ return m_ActiveModel; }
	unsigned char GetLastStep();
	int GetCurrentTime ()
		{ return m_ActiveModel->m_CurFrame; }
	int GetCurrentColor () const
		{ return m_nCurColor; }
	float* GetBackgroundColor()
		{ return m_fBackground; }
	unsigned char GetAction() const
		{ return m_nCurAction; }
	int GetOverlayMode() const
		{ return m_OverlayMode; }
	void GetSnapIndex(int* SnapXY, int* SnapZ) const;
	void GetSnapDistance(float* SnapXY, float* SnapZ) const;
	void GetSnapDistanceText(char* SnapXY, char* SnapZ) const;
	const Vector3& GetOverlayCenter() const
		{ return m_OverlayCenter; }

	void ConvertToUserUnits(Vector3& Value) const;
	void ConvertFromUserUnits(Vector3& Value) const;
	TexFont* GetFont() const
	{ return m_pScreenFont; }

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	// Special notifications
	void DeleteContents(bool bUndo); // delete doc items etc
	void LoadDefaults(bool cameras);
	void BeginPieceDrop(PieceInfo* Info);

	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void Render(View* view, bool AllowFast, bool RenderInterface);
	void CheckAutoSave();
	void CheckAnimation();
	bool GetSelectionCenter(Vector3& Center) const;
	bool GetFocusPosition(Vector3& Position) const;
	lcObject* GetFocusObject() const;
	Group* AddGroup (const char* name, Group* pParent, float x, float y, float z);

	// Views.
	void AddView(View* pView);
	void RemoveView(View* pView);
	void UpdateAllViews();
	View* GetFirstView() const
		{ return m_ViewList.GetSize() ? m_ViewList[0] : NULL; }
	View* GetActiveView() const
		{ return m_ActiveView; }
	bool SetActiveView(View* view);

public:
	lcModel* m_ActiveModel;
	PtrArray<lcModel> m_ModelList;

// Implementation
protected:
	// default implementation
	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;    // changed since last saved

	View* m_ActiveView;
	PtrArray<View> m_ViewList;

	// Font used to draw text on the screen.
	TexFont* m_pScreenFont;

	// Undo support
	LC_UNDOINFO* m_pUndoList;
	LC_UNDOINFO* m_pRedoList;
	bool m_bUndoOriginal;
	void CheckPoint (const char* text);

	// Objects
	Terrain* m_pTerrain;
	File* m_pClipboard[10];
	unsigned char m_nCurClipboard;

	CONNECTION_TYPE m_pConnections[LC_CONNECTIONS];

	void AddModel(lcModel* Model);
	void DeleteModel(lcModel* Model);
	void SetActiveModel(lcModel* Model);

	void AddPiece(Vector3 Pos, Vector4 Rot);
	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(lcPieceObject* OffsetPiece, Vector3& Position, Vector4& Rotation);
	void GetPieceInsertPosition(int MouseX, int MouseY, Vector3& Position, Vector4& Orientation);
	lcObject* FindObjectFromPoint(int x, int y, bool PiecesOnly = false);
	void FindObjectsInBox(float x1, float y1, float x2, float y2, PtrArray<lcObject>& Objects);
	void SelectAndFocusNone(bool bFocusOnly);
	void GetActiveViewportMatrices(Matrix44& ModelView, Matrix44& Projection, int Viewport[4]);
	void CalculateStep();

	// Movement.
	bool MoveSelectedObjects(Vector3& Move, Vector3& Remainder, bool Snap);
	bool RotateSelectedObjects(Vector3& Delta, Vector3& Remainder, bool Snap);
	void SnapVector(Vector3& Delta) const
	{
		Vector3 Dummy;
		SnapVector(Delta, Dummy);
	}
	void SnapVector(Vector3& Delta, Vector3& Leftover) const;
	void SnapRotationVector(Vector3& Delta, Vector3& Leftover) const;

	// Rendering functions.
	void RenderBackground(View* view);
	void RenderScene(View* view);
	void RenderSceneBoxes(View* view);
	void RenderOverlays(View* view);
	void RenderInterface(View* view);

	lcScene* m_Scene;

	// Rendering helper functions.
	void RenderBoxes(bool bHilite);
	void RenderInitialize();

	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, char* ext);

	// Animation playback.
	bool m_PlayingAnimation;
	u64 m_LastFrameTime;

	File* m_pTrackFile;
	bool m_bTrackCancel;
	int m_nTracking;
	int m_nDownX;
	int m_nDownY;
	float m_fTrack[3];
	int m_nMouse;
	Vector3 m_MouseSnapLeftover;
	Vector3 m_MouseTotalDelta;

	int m_OverlayMode;
	bool m_OverlayActive;
	Vector3 m_OverlayCenter;
	Vector3 m_OverlayTrackStart;
	Vector3 m_OverlayDelta;
	void MouseUpdateOverlays(int x, int y);
	void ActivateOverlay();
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
	void OnRightButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnMouseMove(View* view, int x, int y, bool bControl, bool bShift);
	bool OnKeyDown(char nKey, bool bControl, bool bShift);

	void SetAction(int nAction);
	void HandleNotify(LC_NOTIFY id, unsigned long param);
	void HandleCommand(LC_COMMANDS id, unsigned long nParam);
	void HandleMessage(int Message, void* Data);

protected:
	// State variables
	unsigned char m_nCurColor;
	unsigned char m_nCurAction;
	unsigned char m_PreviousAction;
	bool m_bAddKeys;
	unsigned char m_nFPS;

	unsigned long m_nScene;
	unsigned long m_nDetail;
	unsigned long m_nSnap;
	unsigned short m_nMoveSnap;
	unsigned short m_nAngleSnap;
	float m_fLineWidth;
	float m_fFogDensity;
	float m_fFogColor[4];
	float m_fAmbient[4];
	float m_fBackground[4];
	float m_fGradient1[3];
	float m_fGradient2[3];
	char m_strFooter[256];
	char m_strHeader[256];

	unsigned long m_nAutosave;
	unsigned long m_nSaveTimer;
	char m_strModelsPath[LC_MAXPATH];
	char m_strBackground[LC_MAXPATH];
	Texture* m_pBackground;

protected:
	// File load/save implementation.
	bool DoSave(char* lpszPathName, bool bReplace);
	bool DoFileSave();
	bool FileLoad(File* file, bool bUndo, bool bMerge);
	void FileSave(File* file, bool bUndo);
	void FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, PtrArray<File>& FileArray, const String& FilePath);
	void FileReadMPD(File& MPD, PtrArray<File>& FileArray) const;

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
