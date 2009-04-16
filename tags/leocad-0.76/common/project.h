#ifndef _PROJECT_H_
#define _PROJECT_H_

#include "lc_message.h"

#include "object.h"
#include "defines.h"
#include "typedefs.h"
#include "opengl.h"
#include "lc_array.h"
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

enum LC_TOUCH_PHASE
{
	LC_TOUCH_BEGAN,
	LC_TOUCH_MOVED,
	LC_TOUCH_ENDED
};

struct TouchState
{
	LC_TOUCH_PHASE Phase;
	int TapCount;
	int x, y;
	int StartX, StartY;
};

class lcPiece;
class lcCamera;
class lcLight;
class Group;
class GroupInfo;
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

class Project : public lcListener
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

	// Retrieve a pointer to an existing camera.
	lcCamera* GetCamera(int Index) const;
	lcCamera* GetCamera(const char* Name) const;

	void GetTimeRange(int* from, int* to)
	{
		*from = m_bAnimation ? m_nCurFrame : m_nCurStep;
		*to = m_bAnimation ? m_nTotalFrames : 255;
	}
	unsigned short GetTotalFrames () const
		{ return m_nTotalFrames; }

	void ConvertToUserUnits(Vector3& Value) const;
	void ConvertFromUserUnits(Vector3& Value) const;
	void GetArrays(lcPiece** ppPiece, lcCamera** ppCamera, lcLight** ppLight)
	{
		*ppPiece = m_Pieces;
		*ppCamera = m_Cameras;
		*ppLight = m_Lights;
	}

	void UpdateInterface();
	void SetPathName (const char* lpszPathName, bool bAddToMRU);
	void SetTitle (const char* lpszTitle);

public:
	// Special notifications
	void DeleteContents(bool bUndo); // delete doc items etc
	void LoadDefaults(bool cameras);
	void BeginPieceDrop();

	void CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite);
	void Render(bool bToMemory);
	void SetViewSize(int cx, int cy);
	void CheckAutoSave();
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

// Implementation
protected:
	// default implementation
	char m_strTitle[LC_MAXPATH];
	char m_strPathName[LC_MAXPATH];
	bool m_bModified;    // changed since last saved

	View* m_ActiveView;
	lcPtrArray<View> m_ViewList;

	char m_strAuthor[101];
	char m_strDescription[101];
	char m_strComments[256];

	// Font used to draw text on the screen.
	TexFont* m_pScreenFont;

	// Undo support
	LC_UNDOINFO* m_pUndoList;
	LC_UNDOINFO* m_pRedoList;
	bool m_bUndoOriginal;
	void CheckPoint (const char* text);

	// Objects
	lcPiece* m_Pieces;
	lcCamera* m_Cameras;
	lcLight* m_Lights;
	Group* m_pGroups;
	lcCamera* m_pViewCameras[4];
	Terrain* m_pTerrain;
	File* m_pClipboard[10];
	unsigned char m_nCurClipboard;

	CONNECTION_TYPE m_pConnections[LC_CONNECTIONS];

	void AddPiece(lcPiece* pPiece);
	void RemovePiece(lcPiece* pPiece);
	bool RemoveSelectedObjects();
	void GetPieceInsertPosition(lcPiece* OffsetPiece, Vector3& Position, Vector4& Rotation);
	void GetPieceInsertPosition(int MouseX, int MouseY, Vector3& Position, Vector4& Orientation);
	lcObject* FindObjectFromPoint(int x, int y, bool PiecesOnly = false);
	void FindObjectsInBox(float x1, float y1, float x2, float y2, lcPtrArray<lcObject>& Objects);
	void SelectAndFocusNone(bool bFocusOnly);
	void GetActiveViewportMatrices(Matrix44& ModelView, Matrix44& Projection, int Viewport[4]);
	void CalculateStep();

	// Movement.
	bool MoveSelectedObjects(Vector3& Move, Vector3& Remainder);
	bool RotateSelectedObjects(Vector3& Delta, Vector3& Remainder);
	void SnapVector(Vector3& Delta) const
	{
		Vector3 Dummy;
		SnapVector(Delta, Dummy);
	}
	void SnapVector(Vector3& Delta, Vector3& Leftover) const;
	void SnapRotationVector(Vector3& Delta, Vector3& Leftover) const;

	// Rendering functions.
	void RenderScene(bool bShaded, bool bDrawViewports);
	void RenderViewports(bool bBackground, bool bLines);
	void RenderOverlays(int Viewport);
	void RenderBoxes(bool bHilite);
	void RenderInitialize();
	void DrawGrid();
	void CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext);

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
	Vector3 m_MouseSnapLeftover;
	Vector3 m_MouseTotalDelta;

	lcObjArray<TouchState> m_TouchState;
	int FindTouchIndex(int x, int y) const
	{
		for (int TouchIndex = 0; TouchIndex < m_TouchState.GetSize(); TouchIndex++)
			if ((m_TouchState[TouchIndex].x == x) && (m_TouchState[TouchIndex].y == y))
				return TouchIndex;

		return -1;
	}

	int m_OverlayMode;
	bool m_OverlayActive;
	float m_OverlayScale[4];
	Vector3 m_OverlayCenter;
	Vector3 m_OverlayTrackStart;
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
	// Call these functions from each OS
	void OnLeftButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnLeftButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnLeftButtonDoubleClick(View* view, int x, int y, bool bControl, bool bShift);
	void OnRightButtonDown(View* view, int x, int y, bool bControl, bool bShift);
	void OnRightButtonUp(View* view, int x, int y, bool bControl, bool bShift);
	void OnMouseMove(View* view, int x, int y, bool bControl, bool bShift);
	bool OnKeyDown(char nKey, bool bControl, bool bShift);
	void OnTouch(View* view, LC_TOUCH_PHASE Phase, int TapCount, int x, int y, int PrevX, int PrevY);

	void SetAction(int nAction);
	void HandleNotify(LC_NOTIFY id, unsigned long param);
	void HandleCommand(LC_COMMANDS id, unsigned long nParam);
	void ProcessMessage(lcMessageType Message, void* Data);

protected:
	// State variables
	unsigned char m_nViewportMode;
	unsigned char m_nActiveViewport;
	int m_nViewX;
	int m_nViewY;
	unsigned char m_nCurAction;
	unsigned char m_PreviousAction;
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
	void FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, lcPtrArray<File>& FileArray, const String& FilePath);
	void FileReadMPD(File& MPD, lcPtrArray<File>& FileArray) const;

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

	// VRML/X3DV export
	void exportVRML97File(char *filename);
	void exportX3DVFile(char *filename);
	void exportVRMLFile(char *filename, int dialect);
	template<class type> void writeVRMLShapes(type color, FILE *stream, int coordinateCounter, lcPiece* pPiece, unsigned short group, float *pos, bool beginAndEnd);
	void writeVRMLShapeBegin(FILE *stream, unsigned long currentColor, bool blackLines);
	void writeVRMLShapeMeshBegin(FILE *stream);
	void writeVRMLShapeMeshData(FILE *stream);
	void writeVRMLShapeMeshEnd(FILE *stream);
	void writeVRMLShapeEnd(FILE *stream);
	void writeIndent(FILE *stream);
	int indent;
	int numDEF;
	int VRMLdialect;
	bool firstData;
	int searchForVertex(float *vertex);
	template<class type> void generateMeshData(type* info, float *pos, lcPiece* pPiece, int numVertices, int currentColor);
	template<class type> void getMinMaxData(type* info, lcPiece* pPiece, GroupInfo* groupInfo);
	template<class type> void getMinMax(type col, lcPiece* pPiece, unsigned short group, GroupInfo* groupInfo);
	bool handleAsGroup(lcPiece* piece, GroupInfo groupInfo);
	int numCoords;
	float *coords;
	int numCoordIndices;
	int *coordIndices;
	float centerOfMass[3];
	int numFaceColors;
	int *faceColors;
	float VRMLScale;
};

#endif // _PROJECT_H_
