#include "lc_global.h"
#include "lc_modelhistory.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"

void lcModelHistorySelect::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelHistorySelect::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelHistorySelect::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelHistorySelect::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelHistorySelect::StateChanged() const
{
	return mStartState != mEndState;
}

void lcModelHistorySelect::SaveState(lcModelHistorySelectState& State, const lcModel* Model)
{
	State = lcModelHistorySelectState();

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

void lcModelHistorySelect::LoadState(const lcModelHistorySelectState& State, lcModel* Model)
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

bool operator!=(const lcModelHistoryEditState& a, const lcModelHistoryEditState& b)
{
	return a.Groups != b.Groups || a.Pieces != b.Pieces || a.Cameras != b.Cameras || a.Lights != b.Lights;
}

lcModelHistoryEdit::lcModelHistoryEdit(lcModelHistoryEditMerge ModelHistoryEditMerge)
	: mMerge(ModelHistoryEditMerge)
{
}

lcModelHistoryEdit::~lcModelHistoryEdit()
{
}

void lcModelHistoryEdit::SaveState(lcModelHistoryEditState& State, const lcModel* Model)
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

void lcModelHistoryEdit::LoadState(const lcModelHistoryEditState& State, lcModel* Model)
{
	Model->LoadEditHistoryState(State);
}

void lcModelHistoryEdit::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelHistoryEdit::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelHistoryEdit::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelHistoryEdit::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelHistoryEdit::StateChanged() const
{
	return mStartState != mEndState;
}

bool lcModelHistoryEdit::CanMergeWith(const lcModelHistory* Other) const
{
	const lcModelHistoryEdit* OtherModelHistoryEdit = dynamic_cast<const lcModelHistoryEdit*>(Other);

	return OtherModelHistoryEdit && mMerge != lcModelHistoryEditMerge::None && mMerge == OtherModelHistoryEdit->mMerge;
}

void lcModelHistoryEdit::MergeWith(lcModelHistory* Other)
{
	lcModelHistoryEdit* OtherModelHistoryEdit = dynamic_cast<lcModelHistoryEdit*>(Other);

	if (OtherModelHistoryEdit)
		mEndState = std::move(OtherModelHistoryEdit->mEndState);
}

void lcModelHistoryProperties::SaveStartState(const lcModel* Model)
{
	SaveState(mStartState, Model);
}

void lcModelHistoryProperties::SaveEndState(const lcModel* Model)
{
	SaveState(mEndState, Model);
}

void lcModelHistoryProperties::LoadStartState(lcModel* Model) const
{
	LoadState(mStartState, Model);
}

void lcModelHistoryProperties::LoadEndState(lcModel* Model) const
{
	LoadState(mEndState, Model);
}

bool lcModelHistoryProperties::StateChanged() const
{
	return mStartState != mEndState;
}

void lcModelHistoryProperties::SaveState(lcModelProperties& State, const lcModel* Model)
{
	State = Model->GetProperties();
}

void lcModelHistoryProperties::LoadState(const lcModelProperties& State, lcModel* Model)
{
	Model->LoadModelPropertiesState(State);
}
