#ifndef _LC_MODEL_H_
#define _LC_MODEL_H_

#include "str.h"

class lcObject;

class lcModel
{
public:
	lcModel(const String& Name);
	~lcModel();

public:
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

protected:
	String m_Name;
	Object* m_Pieces;

	int m_CurTime;
};

#endif // _LC_MODEL_H_
