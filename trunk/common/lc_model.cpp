#include "lc_model.h"
#include "piece.h"
#include "system.h"

lcModel::lcModel(const String& Name)
{
	m_Pieces = NULL;
	m_Name = Name;
	m_CurTime = 1;
}

lcModel::~lcModel()
{
	while (m_Pieces)
	{
		Object* piece = m_Pieces;
		m_Pieces = m_Pieces->m_Next;
		delete piece;
	}
}

void lcModel::SelectAllPieces(bool Select, bool FocusOnly)
{
	LC_ASSERT(!(FocusOnly && Select), "Cannot set focus to more than 1 piece.");

	for (Object* piece = m_Pieces; piece; piece = piece->m_Next)
		if (piece->IsVisible(m_CurTime))
			piece->Select(Select, FocusOnly);
}

void lcModel::SelectInvertAllPieces()
{
	for (Object* piece = m_Pieces; piece; piece = piece->m_Next)
	{
		if (piece->IsVisible(m_CurTime))
		{
			if (piece->IsSelected())
				piece->Select(false, false);
			else
				piece->Select(true, false);
		}
	}
}

void lcModel::HideSelectedPieces()
{
	for (Object* piece = m_Pieces; piece; piece = piece->m_Next)
		if (piece->IsSelected())
			piece->SetVisible(false);
}

void lcModel::HideUnselectedPieces()
{
	for (Object* piece = m_Pieces; piece; piece = piece->m_Next)
		if (!piece->IsSelected())
			piece->SetVisible(false);
}

void lcModel::UnhideAllPieces()
{
	for (Object* piece = m_Pieces; piece; piece = piece->m_Next)
		piece->SetVisible();
}

bool lcModel::RemoveSelectedPieces()
{
	Object* piece = m_Pieces;
	bool Deleted = false;

	while (piece)
	{
		if (piece->IsSelected())
		{
			Object* Temp = piece->m_Next;

			Deleted = true;
			RemovePiece(piece);
			delete piece;
			piece = Temp;
		}
		else
			piece = piece->m_Next;
	}

	return Deleted;
}
