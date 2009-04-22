#ifndef _LC_MODEL_H_
#define _LC_MODEL_H_

#include "str.h"
//#include "config.h"
//#include "algebra.h"
//#include "lc_array.h"

class lcObject;
class lcPiece;
class lcCamera;
class lcLight;
//class lcScene;

class lcModel
{
public:
	lcModel(/*const char* Name*/);
	~lcModel();

public:
	// Check if a given model is referenced by this model.
//	bool IsSubModel(const lcModel* Model) const;

	// Get a list of all actual pieces used by this model.
//	void GetPieceList(lcObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const;

	// Tell this model it's now the active model or no longer active.
	void SetActive(bool Active);

	// Update this model to a given time.
//	void Update(u32 Time);

	// Return true if any objects are currently selected.
//	bool AnyObjectsSelected() const;

	// Add a piece to this model.
//	void AddPiece(lcPieceObject* NewPiece);

	// Remove a piece from this model.
//	void RemovePiece(lcPieceObject* Piece);

	// Inline the contents of another model into this model.
//	void InlineModel(lcModel* Model, const Matrix44& ModelWorld, int Color);

	// Return true if any pieces are currently selected.
//	bool AnyPiecesSelected() const;

	// Select/Deselect all pieces.
//	void SelectAllPieces(bool Select = true);

	// Invert current piece selection.
//	void SelectInvertAllPieces();

	// Hide all currently selected pieces.
//	void HideSelectedPieces();

	// Hide all pieces not currently selected.
//	void HideUnselectedPieces();

	// Show all hidden pieces.
//	void UnhideAllPieces();

	// Delete all selected pieces and returns true if any pieces were removed.
//	bool RemoveSelectedPieces();

	// Add a camera to this model.
//	void AddCamera(lcCamera* Camera);

	// Delete all existing cameras and create new default ones.
//	void ResetCameras();

	// Retrieve a pointer to an existing camera.
//	lcCamera* GetCamera(int Index) const;
//	lcCamera* GetCamera(const char* Name) const;

	// Adds a light to this model.
//	void AddLight(lcLight* Light);

public:
	// Objects contained in this model.
	lcPiece* m_Pieces;
	lcCamera* m_Cameras;
	lcLight* m_Lights;

	// Frame information.
	u32 m_CurFrame;
	u32 m_TotalFrames;

	// Model information.
	String m_Name;
	String m_Author;
	String m_Description;
	String m_Comments;

	// Data used when the model isn't active.
//	BoundingBox m_BoundingBox;
};

#endif // _LC_MODEL_H_
