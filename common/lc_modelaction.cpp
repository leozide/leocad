#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"

void lcModelActionSelection::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelActionSelection::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelActionSelection::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelActionSelection::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelActionSelection::StateChanged() const
{
	return mStartState != mEndState;
}

void lcModelActionSelection::SaveState(lcModelActionSelectionState& State, const lcModel* Model)
{
	State = lcModelActionSelectionState();

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	State.PieceSelection.resize(Pieces.size(), false);

	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		lcPiece* Piece = Pieces[PieceIndex].get();

		if (!Piece->IsSelected())
			continue;

		State.PieceSelection[PieceIndex] = true;

		if (Piece->IsFocused())
		{
			State.FocusIndex = PieceIndex;
			State.FocusSection = Piece->GetFocusSection();
			State.FocusObjectType = lcObjectType::Piece;
		}
	}

	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	State.CameraSelection.resize(Cameras.size(), false);

	for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		lcCamera* Camera = Cameras[CameraIndex].get();

		if (!Camera->IsSelected())
			continue;

		State.CameraSelection[CameraIndex] = true;

		if (Camera->IsFocused())
		{
			State.FocusIndex = CameraIndex;
			State.FocusSection = Camera->GetFocusSection();
			State.FocusObjectType = lcObjectType::Camera;
		}
	}

	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	State.LightSelection.resize(Lights.size(), false);

	for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		lcLight* Light = Lights[LightIndex].get();

		if (!Light->IsSelected())
			continue;

		State.LightSelection[LightIndex] = true;

		if (Light->IsFocused())
		{
			State.FocusIndex = LightIndex;
			State.FocusSection = Light->GetFocusSection();
			State.FocusObjectType = lcObjectType::Light;
		}
	}
}

void lcModelActionSelection::LoadState(const lcModelActionSelectionState& State, lcModel* Model)
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	if (Pieces.size() != State.PieceSelection.size() || Cameras.size() != State.CameraSelection.size() || Lights.size() != State.LightSelection.size())
		return;

	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		lcPiece* Piece = Pieces[PieceIndex].get();

		Piece->SetSelected(State.PieceSelection[PieceIndex]);
	
		if (State.FocusObjectType == lcObjectType::Piece && State.FocusIndex == PieceIndex)
			Piece->SetFocused(State.FocusSection, true);
		else
			Piece->SetFocused(State.FocusSection, false);
	}

	for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		lcCamera* Camera = Cameras[CameraIndex].get();

		Camera->SetSelected(State.CameraSelection[CameraIndex]);
	
		if (State.FocusObjectType == lcObjectType::Camera && State.FocusIndex == CameraIndex)
			Camera->SetFocused(State.FocusSection, true);
		else
			Camera->SetFocused(State.FocusSection, false);
	}

	for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		lcLight* Light = Lights[LightIndex].get();

		Light->SetSelected(State.LightSelection[LightIndex]);
	
		if (State.FocusObjectType == lcObjectType::Light && State.FocusIndex == LightIndex)
			Light->SetFocused(State.FocusSection, true);
		else
			Light->SetFocused(State.FocusSection, false);
	}
}

bool operator!=(const lcModelHistoryState& a, const lcModelHistoryState& b)
{
	return a.Groups != b.Groups || a.Pieces != b.Pieces || a.Cameras != b.Cameras || a.Lights != b.Lights;
}

lcModelActionObjectEdit::lcModelActionObjectEdit(lcModelActionEditMerge ModelActionEditMerge)
	: mMerge(ModelActionEditMerge)
{
}

lcModelActionObjectEdit::~lcModelActionObjectEdit()
{
}

void lcModelActionObjectEdit::SaveState(lcModelHistoryState& State, const lcModel* Model)
{
	const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();
	
	for (const std::unique_ptr<lcGroup>& Group : Groups)
		State.Groups.emplace_back(Group->GetHistoryState(Model));
	
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	
	for (const std::unique_ptr<lcPiece>& Piece : Pieces)
		State.Pieces.emplace_back(Piece->GetHistoryState(Model));
	
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	
	for (const std::unique_ptr<lcCamera>& Camera : Cameras)
		State.Cameras.emplace_back(Camera->GetHistoryState(Model));
	
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	
	for (const std::unique_ptr<lcLight>& Light : Lights)
		State.Lights.emplace_back(Light->GetHistoryState(Model));
}

void lcModelActionObjectEdit::LoadState(const lcModelHistoryState& State, lcModel* Model)
{
	Model->LoadHistoryState(State);
}

void lcModelActionObjectEdit::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelActionObjectEdit::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelActionObjectEdit::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelActionObjectEdit::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelActionObjectEdit::StateChanged() const
{
	return mStartState != mEndState;
}

bool lcModelActionObjectEdit::CanMergeWith(const lcModelAction* Other) const
{
	const lcModelActionObjectEdit* OtherModelActionEdit = dynamic_cast<const lcModelActionObjectEdit*>(Other);

	return OtherModelActionEdit && mMerge != lcModelActionEditMerge::None && mMerge == OtherModelActionEdit->mMerge;
}

void lcModelActionObjectEdit::MergeWith(lcModelAction* Other)
{
	lcModelActionObjectEdit* OtherModelActionEdit = dynamic_cast<lcModelActionObjectEdit*>(Other);

	if (OtherModelActionEdit)
		mEndState = std::move(OtherModelActionEdit->mEndState);
}

void lcModelActionProperties::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelActionProperties::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelActionProperties::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelActionProperties::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelActionProperties::StateChanged() const
{
	return mStartState != mEndState;
}

void lcModelActionProperties::SaveState(lcModelProperties& State, const lcModel* Model)
{
	State = Model->GetProperties();
}

void lcModelActionProperties::LoadState(const lcModelProperties& State, lcModel* Model)
{
	Model->SetModelProperties(State);
}
