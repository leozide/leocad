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
	LC_TRACK_NONE, LC_TRACK_START_LEFT, LC_TRACK_LEFT,
	LC_TRACK_START_RIGHT, LC_TRACK_RIGHT
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

#include "file.h"

typedef struct LC_UNDOINFO
{
	FileMem file;
	char strText[21];
	LC_UNDOINFO* pNext;
	LC_UNDOINFO() { pNext = NULL; };
} LC_UNDOINFO;

class Project
{
public:
// Constructors
	Project();
	~Project();
	bool Initialize(int argc, char *argv[], char* binpath, char* libpath);

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
	unsigned short GetCurrentTime ()
		{ return m_bAnimation ? m_nCurFrame : m_nCurStep; }
	unsigned long GetSnapFlags() const
		{ return m_nSnap; }
	void SetCurrentPiece(PieceInfo* pInfo)
		{ m_pCurPiece = pInfo; }
	int GetCurrentColor () const
		{ return m_nCurColor; }
	int GetCurrentGroup () const
		{ return m_nCurGroup; }
	float* GetBackgroundColor()
		{ return m_fBackground; }
	unsigned char GetAction() const
		{ return m_nCurAction; }
	int GetOverlayMode() const
		{ return m_OverlayMode; }
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

	void GetArrays(Piece** ppPiece, Camera** ppCamera, Light** ppLight)
	{
		*ppPiece = m_pPieces;
		*ppCamera = m_pCameras;
		*ppLight = m_pLights;
	}

	PiecesLibrary* GetPiecesLibrary () const
		{ return m_pLibrary; }

	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	// Special notifications
	void DeleteContents(bool bUndo); // delete doc items etc
	void LoadDefaults(bool cameras);

	void Render(bool bToMemory);
	void SetViewSize(int cx, int cy);
	void CheckAutoSave();
	void GetFocusPosition(float* pos);
	Object* GetFocusObject() const;
	Group* AddGroup (const char* name, Group* pParent, float x, float y, float z);

	void AddView (View* pView);
	void RemoveView (View* pView);
	void UpdateAllViews (View* pSender = NULL);

// Implementation
protected:
	// default implementation
	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;    // changed since last saved

	PtrArray<View> m_ViewList;

	char m_strAuthor[101];
	char m_strDescription[101];
	char m_strComments[256];

	// Piece library
	char m_AppPath[LC_MAXPATH];	// path to the LeoCAD executable
	TexFont* m_pScreenFont;
	PiecesLibrary* m_pLibrary;

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
	Camera* m_pViewCameras[4];
	Terrain* m_pTerrain;
	File* m_pClipboard[10];
	unsigned char m_nCurClipboard;

	CONNECTION_TYPE m_pConnections[LC_CONNECTIONS];

	void AddPiece(Piece* pPiece);
	void RemovePiece(Piece* pPiece);
	bool RemoveSelectedObjects();
	void FindObjectFromPoint(int x, int y, LC_CLICKLINE* pLine);
	void FindObjectsInBox(float x1, float y1, float x2, float y2, PtrArray<Object>& Objects);
	void SelectAndFocusNone(bool bFocusOnly);
	bool GetSelectionCenter(Point3& Center) const;
	void CalculateStep();
	void MoveSelectedObjects(const Vector3& Delta);
	void RotateSelectedObjects(const Vector3& Delta);
	void SnapVector(Vector3& Delta, Vector3& Leftover) const;
	void SnapRotationVector(Vector3& Delta, Vector3& Leftover) const;

	// Deprecated compatibility functions.
	// TODO: Update function calls to use the math library.
	void RotateSelectedObjects(float x, float y, float z)
	{ RotateSelectedObjects(Vector3(x, y, z)); };
	inline void MoveSelectedObjects(float x, float y, float z)
	{ MoveSelectedObjects(Vector3(x, y, z)); };
	void SnapPoint(float *point, float *reminder) const
	{
		Vector3 Pt(point[0], point[1], point[2]);
		Vector3 Rem;
		if (reminder) Rem = Vector3(reminder[0], reminder[1], reminder[2]);
		SnapVector(Pt, Rem);
		point[0] = Pt[0]; point[1] = Pt[1]; point[2] = Pt[2];
		if (reminder) { reminder[0] = Rem[0]; reminder[1] = Rem[1]; reminder[2] = Rem[2]; }
	};

	// Rendering
	void RenderScene(bool bShaded, bool bDrawViewports);
	void RenderViewports(bool bBackground, bool bLines);
	void RenderOverlays(int Viewport);
	void RenderBoxes(bool bHilite);
	void RenderInitialize();
	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, char* ext);

	inline bool IsDrawing()
	{
		if (m_bRendering)
			m_bStopRender = true;
		return m_bRendering;
	}

	bool m_bRendering;
	bool m_bStopRender;
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
	float m_OverlayScale[4];
	Point3 m_OverlayCenter;
	Point3 m_OverlayTrackStart;
	Vector3 m_OverlayDelta;
	void MouseUpdateOverlays(int x, int y);
	void ActivateOverlay();
	void UpdateOverlayScale();

	void LoadViewportProjection(int Viewport);
	bool SetActiveViewport(int x, int y);
	bool StopTracking(bool bAccept);
	void StartTracking(int mode);
	void UpdateSelection();
	void RemoveEmptyGroups();

public:
	// Call this functions from each OS
	void OnLeftButtonDown(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(int x, int y, bool bControl, bool bShift);
	void OnMouseMove(int x, int y, bool bControl, bool bShift);
	bool OnKeyDown(char nKey, bool bControl, bool bShift);

	void SetAction(int nAction);
	void HandleNotify(LC_NOTIFY id, unsigned long param);
	void HandleCommand(LC_COMMANDS id, unsigned long nParam);
	void HandleMessage(int Message, void* Data);

protected:
	// State variables
	unsigned char m_nViewportMode;
	unsigned char m_nActiveViewport;
	int m_nViewX;
	int m_nViewY;
	PieceInfo* m_pCurPiece;
	unsigned char m_nCurColor;
	unsigned char m_nCurAction;
	unsigned char m_nCurGroup;
	bool m_bAnimation;
	bool m_bAddKeys;
	unsigned char m_nFPS;
	unsigned char m_nCurStep;
	unsigned short m_nCurFrame;
	unsigned short m_nTotalFrames;

	unsigned long m_nScene;
	unsigned long m_nDetail;
	unsigned long m_nSnap;
	unsigned short m_nMoveSnap;
	unsigned short m_nAngleSnap;
	unsigned short m_nGridSize;
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
	bool DoSave(char* lpszPathName, bool bReplace);
	bool DoFileSave();
	bool FileLoad(File* file, bool bUndo, bool bMerge);
	void FileSave(File* file, bool bUndo);
	void FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, PtrArray<File>& FileArray);
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
