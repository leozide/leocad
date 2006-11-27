#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "system.h"

lcModel::lcModel(const char* Name)
{
	m_Name = Name;

	m_Pieces = NULL;
	m_Cameras = NULL;
	m_Lights = NULL;

	m_CurFrame = 1;
	m_TotalFrames = 1;
}

lcModel::~lcModel()
{
	while (m_Pieces)
	{
		Piece* piece = m_Pieces;
		m_Pieces = (Piece*)m_Pieces->m_Next;
		delete piece;
	}

	while (m_Cameras)
	{
		Camera* camera = m_Cameras;
		m_Cameras = (Camera*)m_Cameras->m_Next;
		delete camera;
	}

	while (m_Lights)
	{
		Light* light = m_Lights;
		m_Lights = (Light*)m_Lights->m_Next;
		delete light;
	}
}

void lcModel::AddPiece(Piece* NewPiece)
{
	Piece* Prev = NULL;
	Piece* Next = m_Pieces;

	while (Next)
	{
		if (Next->GetPieceInfo() > NewPiece->GetPieceInfo())
			break;

		Prev = Next;
		Next = (Piece*)Next->m_Next;
	}

	NewPiece->m_Next = Next;

	if (Prev)
		Prev->m_Next = NewPiece;
	else
		m_Pieces = NewPiece;
}

void lcModel::RemovePiece(Piece* piece)
{
	Piece* Next = m_Pieces;
	Piece* Prev = NULL;

	while (Next)
	{
		if (Next == piece)
		{
			if (Prev != NULL)
				Prev->m_Next = piece->m_Next;
			else
				m_Pieces = (Piece*)piece->m_Next;

			break;
		}

		Prev = Next;
		Next = (Piece*)Next->m_Next;
	}
}

bool lcModel::AnyPiecesSelected() const
{
	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
		if ((piece->IsVisible(m_CurFrame)) && piece->IsSelected())
			return true;

	return false;
}

void lcModel::SelectAllPieces(bool Select, bool FocusOnly)
{
	LC_ASSERT(!(FocusOnly && Select), "Cannot set focus to more than 1 piece.");

	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
		if (piece->IsVisible(m_CurFrame))
			piece->Select(Select, FocusOnly);
}

void lcModel::SelectInvertAllPieces()
{
	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
	{
		if (piece->IsVisible(m_CurFrame))
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
	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
		if (piece->IsSelected())
			piece->Hide();
}

void lcModel::HideUnselectedPieces()
{
	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
		if (!piece->IsSelected())
			piece->Hide();
}

void lcModel::UnhideAllPieces()
{
	for (Piece* piece = m_Pieces; piece; piece = (Piece*)piece->m_Next)
		piece->UnHide();
}

bool lcModel::RemoveSelectedPieces()
{
	Piece* piece = m_Pieces;
	bool Deleted = false;

	while (piece)
	{
		if (piece->IsSelected())
		{
			Piece* Temp = (Piece*)piece->m_Next;

			Deleted = true;
			RemovePiece(piece);
			delete piece;
			piece = Temp;
		}
		else
			piece = (Piece*)piece->m_Next;
	}

	return Deleted;
}

void lcModel::ResetCameras()
{
	// Delete all cameras.
	while (m_Cameras)
	{
		Camera* camera = m_Cameras;
		m_Cameras = (Camera*)m_Cameras->m_Next;
		delete camera;
	}

	// Create new default cameras.
	Camera* camera = NULL;
	for (int i = 0; i < 7; i++)
	{
		camera = new Camera(i, camera);
		if (m_Cameras == NULL)
			m_Cameras = camera;
	}
}

Camera* lcModel::GetCamera(int Index) const
{
	Camera* camera = m_Cameras;

	while (camera && Index--)
		camera = (Camera*)camera->m_Next;

	return camera;
}

Camera* lcModel::GetCamera(const char* Name) const
{
	for (Camera* camera = m_Cameras; camera; camera = (Camera*)camera->m_Next)
		if (!strcmp(camera->GetName(), Name))
			return camera;

	return NULL;
}
