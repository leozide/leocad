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

lcModelActionSelection::lcModelActionSelection(lcModelActionSelectionMode Mode, lcStep Step)
    : mMode(Mode), mStep(Step)
{
}

bool lcModelActionSelection::Initialize(const lcModel* Model, const std::vector<lcObject*>& Objects, lcObject* FocusObject, uint32_t FocusSection, lcSelectionMode SelectionMode)
{
	if (!HasChanges(Model, Objects, FocusObject, FocusSection))
		return false;

	mSelectionMode = SelectionMode;

	SavePreviousSelection(Model);
	SaveNewObjects(Objects, Model);
	SaveNewFocusObject(Model, FocusObject, FocusSection);
	
	return true;
}

size_t lcModelActionSelection::GetObjectIndex(const lcObject* Object, const lcModel* Model)
{
	switch (Object->GetType())
	{
		case lcObjectType::Piece:
		{
			const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
		
			for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
				if (Pieces[PieceIndex].get() == Object)
					return PieceIndex;
		}
		break;
		
	case lcObjectType::Camera:
		{
			const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
		
			for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
				if (Cameras[CameraIndex].get() == Object)
					return CameraIndex;
		}
		break;
		
	case lcObjectType::Light:
		{
			const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
		
			for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
				if (Lights[LightIndex].get() == Object)
					return LightIndex;
		}
		break;
	}

	return SIZE_T_MAX;
}

bool lcModelActionSelection::HasChanges(const lcModel* Model, const std::vector<lcObject*>& Objects, lcObject* FocusObject, uint32_t FocusSection) const
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	
	auto HasSelectionChanges=[&Objects, FocusObject, FocusSection](lcObject* CheckObject)
	{
		if (CheckObject->IsFocused())
		{
			if (CheckObject != FocusObject || CheckObject->GetFocusSection() != FocusSection)
				return true;
		}
		else
		{
			if (CheckObject == FocusObject)
				return true;
		}
		
		if (CheckObject == FocusObject || std::find(Objects.begin(), Objects.end(), CheckObject) != Objects.end())
		{
			if (!CheckObject->IsSelected())
				return true;
		}
		else
		{
			if (CheckObject->IsSelected())
				return true;
		}
		
		return false;
	};
	
	switch (mMode)
	{
	case lcModelActionSelectionMode::ClearSelection:
		return Model->AnyObjectsSelected();
		
	case lcModelActionSelectionMode::ClearSelectionAndSetFocus:
		if (FocusObject && !FocusObject->IsFocused(FocusSection))
			return true;
		
		for (const std::unique_ptr<lcPiece>& Piece : Pieces)
			if (Piece->IsSelected() && Piece.get() != FocusObject)
				return true;
		
		for (const std::unique_ptr<lcCamera>& Camera : Cameras)
			if (Camera->IsSelected() && Camera.get() != FocusObject)
				return true;
		
		for (const std::unique_ptr<lcLight>& Light : Lights)
			if (Light->IsSelected() && Light.get() != FocusObject)
				return true;
		
		return false;
		
	case lcModelActionSelectionMode::SetFocus:
		if (FocusObject)
		{
			if (!FocusObject->IsFocused(FocusSection))
				return true;
		}
		else
		{
			if (Model->GetFocusObject())
				return true;
		}
		
		return false;
		
	case lcModelActionSelectionMode::SetSelectionAndFocus:
		if (FocusObject && !FocusObject->IsFocused(FocusSection))
			return true;

		for (const std::unique_ptr<lcPiece>& Piece : Pieces)
			if (HasSelectionChanges(Piece.get()))
				return true;
		
		for (const std::unique_ptr<lcCamera>& Camera : Cameras)
			if (HasSelectionChanges(Camera.get()))
				return true;
		
		for (const std::unique_ptr<lcLight>& Light : Lights)
			if (HasSelectionChanges(Light.get()))
				return true;
		
		return false;

	case lcModelActionSelectionMode::SelectAllPieces:
		for (const std::unique_ptr<lcPiece>& Piece : Pieces)
			if (Piece->IsVisible(mStep) && !Piece->IsSelected())
				return true;
		
		return false;
		
	case lcModelActionSelectionMode::InvertPieceSelection:
		for (const std::unique_ptr<lcPiece>& Piece : Pieces)
			if (Piece->IsVisible(mStep))
				return true;
		
		return false;
		
	case lcModelActionSelectionMode::AddToSelection:
		for (lcObject* Object : Objects)
			if (!Object->IsSelected())
				return true;
		
		return false;
		
	case lcModelActionSelectionMode::RemoveFromSelection:
		for (lcObject* Object : Objects)
			if (Object->IsSelected())
				return true;
		
		return false;
		
	case lcModelActionSelectionMode::Set:
	case lcModelActionSelectionMode::Save:
	case lcModelActionSelectionMode::Restore:
		break;
	}
	
	return true;
}

void lcModelActionSelection::SavePreviousSelection(const lcModel* Model)
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	
	mPreviousSelectedPieces.clear();
	mPreviousSelectedCameras.clear();
	mPreviousSelectedLights.clear();

	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		lcPiece* Piece = Pieces[PieceIndex].get();

		if (!Piece->IsSelected())
			continue;

		mPreviousSelectedPieces.push_back(PieceIndex);

		if (Piece->IsFocused())
		{
			mPreviousFocusIndex = PieceIndex;
			mPreviousFocusSection = Piece->GetFocusSection();
			mPreviousFocusObjectType = lcObjectType::Piece;
		}
	}

	for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		lcCamera* Camera = Cameras[CameraIndex].get();

		if (!Camera->IsSelected())
			continue;

		mPreviousSelectedCameras.push_back(CameraIndex);

		if (Camera->IsFocused())
		{
			mPreviousFocusIndex = CameraIndex;
			mPreviousFocusSection = Camera->GetFocusSection();
			mPreviousFocusObjectType = lcObjectType::Camera;
		}
	}

	for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		lcLight* Light = Lights[LightIndex].get();

		if (!Light->IsSelected())
			continue;

		if (Light->IsFocused())
		{
			mPreviousFocusIndex = LightIndex;
			mPreviousFocusSection = Light->GetFocusSection();
			mPreviousFocusObjectType = lcObjectType::Light;
		}
	}
}

void lcModelActionSelection::SaveNewObjects(const std::vector<lcObject*>& Objects, const lcModel* Model)
{
	mNewObjects.clear();
	mNewObjects.reserve(Objects.size());
	
	for (lcObject* Object : Objects)
		mNewObjects.emplace_back(GetObjectIndex(Object, Model), Object->GetType());
}

void lcModelActionSelection::SaveNewFocusObject(const lcModel* Model, lcObject* FocusObject, uint32_t FocusSection)
{
	if (!FocusObject)
		return;

	mNewFocusSection = FocusSection;
	mNewFocusObjectType = FocusObject->GetType();
	mNewFocusIndex = GetObjectIndex(FocusObject, Model);
}

std::vector<lcObject*> lcModelActionSelection::GetNewObjects(const lcModel* Model) const
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	std::vector<lcObject*> Objects;
	
	for (auto [ObjectIndex, ObjectType] : mNewObjects)
	{
		switch (ObjectType)
		{
		case lcObjectType::Piece:
			if (ObjectIndex < Pieces.size())
				Objects.push_back(Pieces[ObjectIndex].get());
			break;
			
		case lcObjectType::Camera:
			if (ObjectIndex < Cameras.size())
				Objects.push_back(Cameras[ObjectIndex].get());
			break;
			
		case lcObjectType::Light:
			if (ObjectIndex < Lights.size())
				Objects.push_back(Lights[ObjectIndex].get());
			break;
		}
	}
	
	return Objects;
}

lcObject* lcModelActionSelection::GetNewFocusObject(const lcModel* Model) const
{
	switch (mNewFocusObjectType)
	{
	case lcObjectType::Piece:
		{
			const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
		
			if (mNewFocusIndex < Pieces.size())
				return Pieces[mNewFocusIndex].get();
		}
		break;
		
	case lcObjectType::Camera:
		{
			const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
		
			if (mNewFocusIndex < Cameras.size())
				return Cameras[mNewFocusIndex].get();
		}
		break;
		
	case lcObjectType::Light:
		{
			const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
		
			if (mNewFocusIndex < Lights.size())
				return Lights[mNewFocusIndex].get();
		}
		break;
	}
	
	return nullptr;
}

std::tuple<std::vector<lcObject*>, lcObject*, uint32_t> lcModelActionSelection::GetPreviousSelection(const lcModel* Model) const
{
	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();
	
	std::vector<lcObject*> SelectedObjects;
	lcObject* FocusObject = nullptr;

	for (size_t PieceIndex : mPreviousSelectedPieces)
		if (PieceIndex < Pieces.size())
			SelectedObjects.push_back(Pieces[PieceIndex].get());

	for (size_t CameraIndex : mPreviousSelectedCameras)
		if (CameraIndex < Cameras.size())
			SelectedObjects.push_back(Cameras[CameraIndex].get());

	for (size_t LightIndex : mPreviousSelectedLights)
		if (LightIndex < Lights.size())
			SelectedObjects.push_back(Lights[LightIndex].get());

	if (mPreviousFocusIndex != SIZE_MAX)
	{
		switch (mPreviousFocusObjectType)
		{
		case lcObjectType::Piece:
			if (mPreviousFocusIndex < Pieces.size())
				FocusObject = Pieces[mPreviousFocusIndex].get();
			break;
		case lcObjectType::Camera:
			if (mPreviousFocusIndex < Cameras.size())
				FocusObject = Cameras[mPreviousFocusIndex].get();
			break;
		case lcObjectType::Light:
			if (mPreviousFocusIndex < Lights.size())
				FocusObject = Lights[mPreviousFocusIndex].get();
			break;
		}
	}

	return { SelectedObjects, FocusObject, mPreviousFocusSection };
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
		for (uint64_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
			if (Pieces[PieceIndex]->IsSelected())
				mPieceIndices.push_back(PieceIndex);
		
		for (uint64_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
			if (Cameras[CameraIndex]->IsSelected())
				mCameraIndices.push_back(CameraIndex);
		
		for (uint64_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
			if (Lights[LightIndex]->IsSelected())
				mLightIndices.push_back(LightIndex);
		break;
		
	case lcModelActionObjectEditMode::EditSelectedPieces:
		for (uint64_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
			if (Pieces[PieceIndex]->IsSelected())
				mPieceIndices.push_back(PieceIndex);
		break;
		
	case lcModelActionObjectEditMode::EditUnselectedPieces:
		for (uint64_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
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
