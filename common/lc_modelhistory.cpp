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

lcModelHistoryEdit::lcModelHistoryEdit(lcModelHistoryEditMergeKey MergeKey)
	: mMergeKey(MergeKey)
{
}

lcModelHistoryEdit::~lcModelHistoryEdit()
{
}

void lcModelHistoryEdit::SaveState(lcModelHistoryEditState& State, const lcModel* Model)
{
	State.Groups.Save(Model->GetGroups());
	State.Pieces.Save(Model->GetPieces());
	State.Cameras.Save(Model->GetCameras());
	State.Lights.Save(Model->GetLights());
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

	lcObjectHistory<lcGroupHistoryState>::Compact(mStartState.Groups, mEndState.Groups);
	lcObjectHistory<lcPieceHistoryState>::Compact(mStartState.Pieces, mEndState.Pieces);
	lcObjectHistory<lcCameraHistoryState>::Compact(mStartState.Cameras, mEndState.Cameras);
	lcObjectHistory<lcLightHistoryState>::Compact(mStartState.Lights, mEndState.Lights);
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
	return !mStartState.IsEmpty() || !mEndState.IsEmpty();
}

void lcModelHistoryEdit::RenamePieces(const std::vector<lcModelPieceRename>& Renames, const lcPieceData* ModelPieceData)
{
	Q_UNUSED(ModelPieceData);

	auto RenamePieceIds = [&Renames](lcModelHistoryEditState& State)
	{
		for (lcPieceHistoryState& Piece : State.Pieces.States)
		{
			for (const lcModelPieceRename& Rename : Renames)
			{
				if (Piece.PieceId.compare(Rename.OldName, Qt::CaseInsensitive) != 0)
					continue;

				Piece.PieceId = Rename.NewName;
				break;
			}
		}
	};

	RenamePieceIds(mStartState);
	RenamePieceIds(mEndState);
}

bool lcModelHistoryEdit::CanMergeWith(const lcModelHistory* Other) const
{
	const lcModelHistoryEdit* OtherModelHistoryEdit = dynamic_cast<const lcModelHistoryEdit*>(Other);

	return OtherModelHistoryEdit && mMergeKey.Operation != lcModelHistoryEditMerge::None && mMergeKey == OtherModelHistoryEdit->mMergeKey;
}

void lcModelHistoryEdit::MergeWith(lcModelHistory* Other)
{
	lcModelHistoryEdit* OtherModelHistoryEdit = dynamic_cast<lcModelHistoryEdit*>(Other);

	if (!OtherModelHistoryEdit)
		return;

	lcObjectHistory<lcGroupHistoryState>::Merge(mStartState.Groups, mEndState.Groups, OtherModelHistoryEdit->mStartState.Groups, OtherModelHistoryEdit->mEndState.Groups);
	lcObjectHistory<lcPieceHistoryState>::Merge(mStartState.Pieces, mEndState.Pieces, OtherModelHistoryEdit->mStartState.Pieces, OtherModelHistoryEdit->mEndState.Pieces);
	lcObjectHistory<lcCameraHistoryState>::Merge(mStartState.Cameras, mEndState.Cameras, OtherModelHistoryEdit->mStartState.Cameras, OtherModelHistoryEdit->mEndState.Cameras);
	lcObjectHistory<lcLightHistoryState>::Merge(mStartState.Lights, mEndState.Lights, OtherModelHistoryEdit->mStartState.Lights, OtherModelHistoryEdit->mEndState.Lights);
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

void lcModelHistoryProperties::RenamePieces(const std::vector<lcModelPieceRename>& Renames, const lcPieceData* ModelPieceData)
{
	auto RenameModel = [&Renames, ModelPieceData](lcModelProperties& Properties)
	{
		for (const lcModelPieceRename& Rename : Renames)
		{
			if (Rename.PieceData != ModelPieceData || Properties.mFileName.compare(Rename.OldName, Qt::CaseInsensitive) != 0)
				continue;

			if (Properties.mModelName == Properties.mFileName)
				Properties.mModelName = Rename.NewName;

			Properties.mFileName = Rename.NewName;
			break;
		}
	};

	RenameModel(mStartState);
	RenameModel(mEndState);
}

void lcModelHistoryProperties::SaveState(lcModelProperties& State, const lcModel* Model)
{
	State = Model->GetProperties();
}

void lcModelHistoryProperties::LoadState(const lcModelProperties& State, lcModel* Model)
{
	Model->LoadModelPropertiesState(State);
}
