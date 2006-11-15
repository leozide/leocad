#ifndef _LC_MODEL_H_
#define _LC_MODEL_H_

#include "str.h"

class Piece;

class lcModel
{
public:
	lcModel(const char* Name);
	~lcModel();

public:
	// Adds a piece to this model.
	void AddPiece(Piece* NewPiece);

	// Removes a piece from this model.
	void RemovePiece(Piece* piece);

	// Returns true if any pieces are currently selected.
	bool AnyPiecesSelected() const;

	// Selects/Deselects all pieces.
	void SelectAllPieces(bool Select = true, bool FocusOnly = false);

	// Inverts current piece selection.
	void SelectInvertAllPieces();

	// Hides all currently selected pieces.
	void HideSelectedPieces();

	// Hides all pieces not currently selected.
	void HideUnselectedPieces();

	// Shows all hidden pieces.
	void UnhideAllPieces();

	// Deletes all selected pieces and returns true if any pieces were removed.
	bool RemoveSelectedPieces();

public:
	// Making these public for now.
	Piece* m_Pieces;
	int m_CurTime;

protected:
	String m_Name;
};

#endif // _LC_MODEL_H_
