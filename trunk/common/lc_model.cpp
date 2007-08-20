#include "lc_global.h"
#include "lc_model.h"

#include "lc_object.h"
#include "lc_pieceobj.h"
#include "lc_modelref.h"
#include "lc_camera.h"
#include "lc_light.h"
#include "system.h"

lcModel::lcModel(const char* Name)
{
	m_Name = Name;
	m_Author = Sys_ProfileLoadString("Default", "User", "");

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
		lcObject* Piece = m_Pieces;
		m_Pieces = (lcPieceObject*)m_Pieces->m_Next;
		delete Piece;
	}

	while (m_Cameras)
	{
		lcObject* Camera = m_Cameras;
		m_Cameras = (lcCamera*)m_Cameras->m_Next;
		delete Camera;
	}

	while (m_Lights)
	{
		lcObject* Light = m_Lights;
		m_Lights = (lcLight*)m_Lights->m_Next;
		delete Light;
	}
}

bool lcModel::IsSubModel(const lcModel* Model) const
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
	{
		// fixme: pivot
		if (Piece->GetType() != LC_OBJECT_MODELREF)
			continue;

		lcModelRef* ModelRef = (lcModelRef*)Piece;

		if ((ModelRef->m_Model == Model) || ModelRef->m_Model->IsSubModel(Model))
			return true;
	}

	return false;
}

void lcModel::GetPieceList(ObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	for (lcPieceObject* Piece = m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
		Piece->GetPieceList(Pieces, Color);
}

void lcModel::SetActive(bool Active)
{
	if (Active)
		Update(m_CurFrame);
	else
	{
		Update(LC_MAX_TIME);

		m_BoundingBox.Reset();

		for (lcPieceObject* Piece = m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
		{
			if (Piece->IsVisible(LC_MAX_TIME))
				Piece->MergeBoundingBox(&m_BoundingBox);
		}
	}
}

void lcModel::Update(u32 Time)
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		Piece->Update(Time);

	for (lcObject* Camera = m_Cameras; Camera; Camera = Camera->m_Next)
		Camera->Update(Time);

	for (lcObject* Light = m_Lights; Light; Light = Light->m_Next)
		Light->Update(Time);
}

bool lcModel::AnyObjectsSelected() const
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsSelected())
			return true;

	for (lcObject* Camera = m_Cameras; Camera; Camera = Camera->m_Next)
		if (Camera->IsSelected() || (Camera->m_Children && Camera->m_Children->IsSelected()))
			return true;

	for (lcObject* Light = m_Lights; Light; Light = Light->m_Next)
		if (Light->IsSelected() || (Light->m_Children && Light->m_Children->IsSelected()))
			return true;

	return false;
}

void lcModel::AddPiece(lcPieceObject* NewPiece)
{
	lcObject* Prev = NULL;
	lcObject* Next = m_Pieces;

	while (Next)
	{
		// TODO: sort pieces by vertex buffer.
//		if (Next->GetPieceInfo() > NewPiece->GetPieceInfo())
			break;

		Prev = Next;
		Next = Next->m_Next;
	}

	NewPiece->m_Next = Next;

	if (Prev)
		Prev->m_Next = NewPiece;
	else
		m_Pieces = NewPiece;
}

void lcModel::RemovePiece(lcPieceObject* Piece)
{
	lcObject* Next = m_Pieces;
	lcObject* Prev = NULL;

	while (Next)
	{
		if (Next == Piece)
		{
			if (Prev != NULL)
				Prev->m_Next = Piece->m_Next;
			else
				m_Pieces = (lcPieceObject*)Piece->m_Next;

			break;
		}

		Prev = Next;
		Next = Next->m_Next;
	}
}

void lcModel::InlineModel(lcModel* Model, const Matrix44& ModelWorld, int Color)
{
	// fixme inline
}

bool lcModel::AnyPiecesSelected() const
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if ((Piece->IsVisible(m_CurFrame)) && Piece->IsSelected())
			return true;

	return false;
}

void lcModel::SelectAllPieces(bool Select)
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsVisible(m_CurFrame))
			Piece->SetSelection(Select, true);
}

void lcModel::SelectInvertAllPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsVisible(m_CurFrame))
			Piece->SetSelection(!Piece->IsSelected(), true);
}

void lcModel::HideSelectedPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsSelected())
			Piece->SetVisible(false);
}

void lcModel::HideUnselectedPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (!Piece->IsSelected())
			Piece->SetVisible(false);
}

void lcModel::UnhideAllPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		Piece->SetVisible(true);
}

bool lcModel::RemoveSelectedPieces()
{
	lcObject* Piece = m_Pieces;
	bool Deleted = false;

	while (Piece)
	{
		if (Piece->IsSelected())
		{
			lcObject* Temp = Piece->m_Next;

			Deleted = true;
			RemovePiece((lcPieceObject*)Piece);
			delete Piece;
			Piece = Temp;
		}
		else
			Piece = Piece->m_Next;
	}

	return Deleted;
}

void lcModel::AddCamera(lcCamera* Camera)
{
	lcObject* LastCamera = m_Cameras;

	while (LastCamera && LastCamera->m_Next)
		LastCamera = LastCamera->m_Next;

	if (LastCamera)
		LastCamera->m_Next = Camera;
	else
		m_Cameras = Camera;

	Camera->m_Next = NULL;
}

void lcModel::ResetCameras()
{
	// Delete all cameras.
	while (m_Cameras)
	{
		lcObject* Camera = m_Cameras;
		m_Cameras = (lcCamera*)m_Cameras->m_Next;
		delete Camera;
	}

	// Create new default cameras.
	lcObject* Last = NULL;
	for (int i = 0; i < LC_CAMERA_USER; i++)
	{
		lcCamera* Camera = new lcCamera();
		Camera->CreateCamera(i, true);
		Camera->Update(1);

		if (Last == NULL)
			m_Cameras = Camera;
		else
			Last->m_Next = Camera;

		Last = Camera;
	}
}

lcCamera* lcModel::GetCamera(int Index) const
{
	lcObject* Camera = m_Cameras;

	while (Camera && Index--)
		Camera = Camera->m_Next;

	return (lcCamera*)Camera;
}

lcCamera* lcModel::GetCamera(const char* Name) const
{
	for (lcObject* Camera = m_Cameras; Camera; Camera = Camera->m_Next)
		if (Camera->m_Name == Name)
			return (lcCamera*)Camera;

	return NULL;
}

void lcModel::AddLight(lcLight* Light)
{
	if (!m_Lights)
		m_Lights = Light;
	else
	{
		lcObject* Prev = m_Lights;

		while (Prev->m_Next)
			Prev = Prev->m_Next;

		Prev->m_Next = Light;
	}
}
