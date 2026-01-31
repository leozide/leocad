#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"

bool lcModelAction::SaveHistoryBuffer(QByteArray& Buffer, const lcModel* Model)
{
	QDataStream Stream(&Buffer, QIODevice::WriteOnly);
	
	const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t ObjectCount[4] = { Groups.size(), Pieces.size(), Cameras.size(), Lights.size() };
	
	if (Stream.writeRawData(reinterpret_cast<const char*>(ObjectCount), sizeof(ObjectCount)) != sizeof(ObjectCount))
		return false;
	
	for (size_t GroupIndex : mGroupIndices)
	{
		const lcGroup* Group = Groups[GroupIndex].get();
		uint64_t ParentIndex = UINT64_MAX;
		
		if (Group->mGroup)
			for (ParentIndex = 0; ParentIndex < Groups.size(); ParentIndex++)
				if (Group->mGroup == Groups[ParentIndex].get())
					break;
		
		Stream << Group->mName;
		Stream << ParentIndex;
	}
	
	for (size_t PieceIndex : mPieceIndices)
		if (!Pieces[PieceIndex]->SaveUndoData(Stream, Model))
            return false;
	
	for (size_t CameraIndex : mCameraIndices)
		if (!Cameras[CameraIndex]->SaveUndoData(Stream, Model))
			return false;
	
	for (size_t LightIndex : mLightIndices)
		if (!Lights[LightIndex]->SaveUndoData(Stream, Model))
			return false;
	
	return true;
}

bool lcModelAction::LoadHistoryBuffer(const QByteArray& Buffer, lcModel* Model, bool CreateObjects) const
{
	QDataStream Stream(const_cast<QByteArray*>(&Buffer), QIODevice::ReadOnly);
	
	const std::vector<std::unique_ptr<lcGroup>>& Groups = Model->GetGroups();
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t ObjectCount[4];
	
	if (Stream.readRawData(reinterpret_cast<char*>(ObjectCount), sizeof(ObjectCount)) != sizeof(ObjectCount))
		return false;
	
	if (CreateObjects)
	{
		ObjectCount[0] -= mGroupIndices.size();
		ObjectCount[1] -= mPieceIndices.size();
		ObjectCount[2] -= mCameraIndices.size();
		ObjectCount[3] -= mLightIndices.size();
	}
	
	if (ObjectCount[0] != Groups.size() || ObjectCount[1] != Pieces.size() || ObjectCount[2] != Cameras.size() || ObjectCount[3] != Lights.size())
		return false;
	
	if (CreateObjects)
	{
		for (size_t GroupIndex : mGroupIndices)
		{
			std::unique_ptr<lcGroup> Group(new lcGroup());
			QString Name;
			uint64_t ParentIndex;
			
			Stream >> Name;
			Stream >> ParentIndex;
			
			Group->mName = Name;
			Group->mGroup = ParentIndex < Groups.size() ? Groups[ParentIndex].get() : nullptr;
			
			Model->AddGroup(std::move(Group), GroupIndex);			
		}
		
		for (size_t PieceIndex : mPieceIndices)
		{
			if (PieceIndex > Pieces.size())
				return false;
			
			std::unique_ptr<lcPiece> Piece(new lcPiece(nullptr));
			
			if (!Piece->LoadUndoData(Stream, Model))
				return false;
			
			Model->AddPiece(std::move(Piece), PieceIndex);
		}
		
		for (size_t CameraIndex : mCameraIndices)
		{
            if (CameraIndex > Cameras.size())
                return false;
            
            std::unique_ptr<lcCamera> Camera(new lcCamera(false));
            
            if (!Camera->LoadUndoData(Stream, Model))
                return false;
            
            Model->AddCamera(std::move(Camera), CameraIndex);
		}
		
		for (size_t LightIndex : mLightIndices)
		{
			if (LightIndex > Lights.size())
				return false;
			
			std::unique_ptr<lcLight> Light(new lcLight(lcVector3(0.0f, 0.0f, 0.0f), lcLightType::Point));
			
			if (!Light->LoadUndoData(Stream, Model))
				return false;
			
			Model->AddLight(std::move(Light), LightIndex);
		}
	}
	else
	{
		for (size_t GroupIndex : mGroupIndices)
		{
			lcGroup* Group = Groups[GroupIndex].get();
			QString Name;
			uint64_t ParentIndex;
			
			Stream >> Name;
			Stream >> ParentIndex;
			
			Group->mName = Name;
			Group->mGroup = ParentIndex < Groups.size() ? Groups[ParentIndex].get() : nullptr;
		}
		
		for (size_t PieceIndex : mPieceIndices)
		{
			if (PieceIndex >= Pieces.size())
				return false;
			
			const std::unique_ptr<lcPiece>& Piece = Pieces[PieceIndex];
			
			if (!Piece->LoadUndoData(Stream, Model))
				return false;
		}
		
		for (size_t CameraIndex : mCameraIndices)
		{
			if (CameraIndex >= Cameras.size())
				return false;
			
			const std::unique_ptr<lcCamera>& Camera = Cameras[CameraIndex];
			
			if (!Camera->LoadUndoData(Stream, Model))
				return false;
		}
		
		for (size_t LightIndex : mLightIndices)
		{
			if (LightIndex >= Lights.size())
				return false;
			
			const std::unique_ptr<lcLight>& Light = Lights[LightIndex];

			if (!Light->LoadUndoData(Stream, Model))
				return false;
		}
	}

	return true;
}

lcModelActionSelection::lcModelActionSelection(lcModelActionSelectionMode Mode)
    : mMode(Mode)
{
}

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

lcModelActionObjectEdit::lcModelActionObjectEdit(lcModelActionObjectEditMode Mode)
	: mMode(Mode)
{
}

bool lcModelActionObjectEdit::SaveStartState(const lcModel* Model, const lcCamera* Camera)
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	
	switch (mMode)
	{
	case lcModelActionObjectEditMode::EditAllObjects:
		mPieceIndices.resize(Pieces.size());
		std::iota(mPieceIndices.begin(), mPieceIndices.end(), 0);
		
		mCameraIndices.resize(Cameras.size());
		std::iota(mCameraIndices.begin(), mCameraIndices.end(), 0);
		
		mLightIndices.resize(Lights.size());
		std::iota(mLightIndices.begin(), mLightIndices.end(), 0);
		break;
		
	case lcModelActionObjectEditMode::EditAllPieces:
		mPieceIndices.resize(Pieces.size());
		std::iota(mPieceIndices.begin(), mPieceIndices.end(), 0);
		break;
		
	case lcModelActionObjectEditMode::EditSelectedObjects:
		for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
			if (Pieces[PieceIndex]->IsSelected())
				mPieceIndices.push_back(PieceIndex);
		
		for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
			if (Cameras[CameraIndex]->IsSelected())
				mCameraIndices.push_back(CameraIndex);
		
		for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
			if (Lights[LightIndex]->IsSelected())
				mLightIndices.push_back(LightIndex);
		break;
		
	case lcModelActionObjectEditMode::EditSelectedPieces:
		for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
			if (Pieces[PieceIndex]->IsSelected())
				mPieceIndices.push_back(PieceIndex);
		break;
		
	case lcModelActionObjectEditMode::EditUnselectedPieces:
		for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
			if (!Pieces[PieceIndex]->IsSelected())
				mPieceIndices.push_back(PieceIndex);
		break;
		
	case lcModelActionObjectEditMode::EditCamera:
		for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
		{
			if (Cameras[CameraIndex].get() == Camera)
			{
				mCameraIndices.push_back(CameraIndex);
				break;
			}
		}
		break;
	
	case lcModelActionObjectEditMode::CreatePieces:
	case lcModelActionObjectEditMode::CreateCamera:
	case lcModelActionObjectEditMode::CreateLight:
		break;
	};
	
	return SaveHistoryBuffer(mStartBuffer, Model);
}

bool lcModelActionObjectEdit::SaveEndState(const lcModel* Model, std::vector<size_t>&& ObjectIndices, std::vector<size_t>&& GroupIndices)
{
	switch (mMode)
	{
	case lcModelActionObjectEditMode::EditAllObjects:
	case lcModelActionObjectEditMode::EditAllPieces:
	case lcModelActionObjectEditMode::EditSelectedObjects:
	case lcModelActionObjectEditMode::EditSelectedPieces:
	case lcModelActionObjectEditMode::EditUnselectedPieces:
	case lcModelActionObjectEditMode::EditCamera:
		break;
	
	case lcModelActionObjectEditMode::CreatePieces:
		mPieceIndices = std::move(ObjectIndices);
		mGroupIndices = std::move(GroupIndices);
		break;
		
	case lcModelActionObjectEditMode::CreateCamera:
		mCameraIndices = std::move(ObjectIndices);
		break;
		
	case lcModelActionObjectEditMode::CreateLight:
		mLightIndices = std::move(ObjectIndices);
		break;
	};
	
	return SaveHistoryBuffer(mEndBuffer, Model);	
}

void lcModelActionObjectEdit::LoadStartState(lcModel* Model) const
{
	switch (mMode)
	{
	case lcModelActionObjectEditMode::EditAllObjects:
	case lcModelActionObjectEditMode::EditAllPieces:
	case lcModelActionObjectEditMode::EditSelectedObjects:
	case lcModelActionObjectEditMode::EditSelectedPieces:
	case lcModelActionObjectEditMode::EditUnselectedPieces:
	case lcModelActionObjectEditMode::EditCamera:
		LoadHistoryBuffer(mStartBuffer, Model, false);
		break;
	
	case lcModelActionObjectEditMode::CreatePieces:
		Model->RemovePieces(mPieceIndices);
		break;
		
	case lcModelActionObjectEditMode::CreateCamera:
   		Model->RemoveCameras(mCameraIndices);
		break;
		
	case lcModelActionObjectEditMode::CreateLight:
		Model->RemoveLights(mLightIndices);
		break;
	};
}

void lcModelActionObjectEdit::LoadEndState(lcModel* Model) const
{
	switch (mMode)
	{
	case lcModelActionObjectEditMode::EditAllObjects:
	case lcModelActionObjectEditMode::EditAllPieces:
	case lcModelActionObjectEditMode::EditSelectedObjects:
	case lcModelActionObjectEditMode::EditSelectedPieces:
	case lcModelActionObjectEditMode::EditUnselectedPieces:
	case lcModelActionObjectEditMode::EditCamera:
		LoadHistoryBuffer(mEndBuffer, Model, false);
		break;
	
	case lcModelActionObjectEditMode::CreatePieces:
	case lcModelActionObjectEditMode::CreateCamera:
	case lcModelActionObjectEditMode::CreateLight:
		LoadHistoryBuffer(mEndBuffer, Model, true);
		break;
	};
}

lcModelActionGroupPieces::lcModelActionGroupPieces(lcModelActionGroupPiecesMode Mode, const QString& GroupName)
	: mMode(Mode), mGroupName(GroupName)
{
}
