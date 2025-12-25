#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_colors.h"

bool lcModelAction::SaveUndoBuffer(QByteArray& Buffer, const lcModel* Model)
{
	QDataStream Stream(&Buffer, QIODevice::WriteOnly);

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t ObjectCount[3] = { Pieces.size(), Cameras.size(), Lights.size() };
	
	if (Stream.writeRawData(reinterpret_cast<const char*>(ObjectCount), sizeof(ObjectCount)) != sizeof(ObjectCount))
		return false;
	
	for (size_t PieceIndex : mPieceIndices)
		if (!Pieces[PieceIndex]->SaveUndoData(Stream))
            return false;
	
	for (size_t CameraIndex : mCameraIndices)
		if (!Cameras[CameraIndex]->SaveUndoData(Stream))
			return false;
	
	for (size_t LightIndex : mLightIndices)
		if (!Lights[LightIndex]->SaveUndoData(Stream))
			return false;
	
	return true;
}

bool lcModelAction::LoadUndoBuffer(const QByteArray& Buffer, lcModel* Model) const
{
	QDataStream Stream(const_cast<QByteArray*>(&Buffer), QIODevice::ReadOnly);

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t ObjectCount[3];
	
	if (Stream.readRawData(reinterpret_cast<char*>(ObjectCount), sizeof(ObjectCount)) != sizeof(ObjectCount))
		return false;
	
	if (ObjectCount[0] != Pieces.size() || ObjectCount[1] != Cameras.size() || ObjectCount[2] != Lights.size())
		return false;
	
	for (size_t PieceIndex : mPieceIndices)
	{
		if (PieceIndex >= Pieces.size())
			return false;
		
		const std::unique_ptr<lcPiece>& Piece = Pieces[PieceIndex];
		
		if (!Piece->LoadUndoData(Stream))
			return false;
	}
	
	for (size_t CameraIndex : mCameraIndices)
	{
		if (CameraIndex >= Cameras.size())
			return false;
		
		const std::unique_ptr<lcCamera>& Camera = Cameras[CameraIndex];
		
		if (!Camera->LoadUndoData(Stream))
			return false;
	}
	
	for (size_t LightIndex : mLightIndices)
	{
		if (LightIndex >= Lights.size())
			return false;
		
		const std::unique_ptr<lcLight>& Light = Lights[LightIndex];
		
		if (!Light->LoadUndoData(Stream))
			return false;
	}
	
	return true;	
}

lcModelActionSelection::lcModelActionSelection(lcModelActionSelectionMode Mode)
	: mMode(Mode)
{
}

void lcModelActionSelection::SetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights)
{
	mSelectedPieces.clear();
	mSelectedCameras.clear();
	mSelectedLights.clear();

	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		lcPiece* Piece = Pieces[PieceIndex].get();

		if (!Piece->IsSelected())
			continue;

		mSelectedPieces.push_back(PieceIndex);

		if (Piece->IsFocused())
		{
			mFocusIndex = PieceIndex;
			mFocusSection = Piece->GetFocusSection();
			mFocusType = lcObjectType::Piece;
		}
	}

	for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		lcCamera* Camera = Cameras[CameraIndex].get();

		if (!Camera->IsSelected())
			continue;

		mSelectedCameras.push_back(CameraIndex);

		if (Camera->IsFocused())
		{
			mFocusIndex = CameraIndex;
			mFocusSection = Camera->GetFocusSection();
			mFocusType = lcObjectType::Camera;
		}
	}

	for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		lcLight* Light = Lights[LightIndex].get();

		if (!Light->IsSelected())
			continue;

		if (Light->IsFocused())
		{
			mFocusIndex = LightIndex;
			mFocusSection = Light->GetFocusSection();
			mFocusType = lcObjectType::Light;
		}
	}
}

std::tuple<std::vector<lcObject*>, lcObject*, uint32_t> lcModelActionSelection::GetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights) const
{
	std::vector<lcObject*> SelectedObjects;
	lcObject* FocusObject = nullptr;

	for (size_t PieceIndex : mSelectedPieces)
		if (PieceIndex < Pieces.size())
			SelectedObjects.push_back(Pieces[PieceIndex].get());

	for (size_t CameraIndex : mSelectedCameras)
		if (CameraIndex < Cameras.size())
			SelectedObjects.push_back(Cameras[CameraIndex].get());

	for (size_t LightIndex : mSelectedLights)
		if (LightIndex < Lights.size())
			SelectedObjects.push_back(Lights[LightIndex].get());

	if (mFocusIndex != SIZE_MAX)
	{
		switch (mFocusType)
		{
		case lcObjectType::Piece:
			if (mFocusIndex < Pieces.size())
				FocusObject = Pieces[mFocusIndex].get();
			break;
		case lcObjectType::Camera:
			if (mFocusIndex < Cameras.size())
				FocusObject = Cameras[mFocusIndex].get();
			break;
		case lcObjectType::Light:
			if (mFocusIndex < Lights.size())
				FocusObject = Lights[mFocusIndex].get();
			break;
		}
	}

	return { SelectedObjects, FocusObject, mFocusSection };
}

lcModelActionObjectEdit::lcModelActionObjectEdit(lcModelActionObjectEditMode Mode)
	: mMode(Mode)
{
}

void lcModelActionObjectEdit::SaveStartState(const lcModel* Model, const lcCamera* Camera)
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
	};	
	
	SaveUndoBuffer(mStartBuffer, Model);
}

void lcModelActionObjectEdit::SaveEndState(const lcModel* Model, const lcCamera* Camera)
{
	SaveUndoBuffer(mEndBuffer, Model);	
}

void lcModelActionObjectEdit::LoadStartState(lcModel* Model) const
{
	LoadUndoBuffer(mStartBuffer, Model);
}

void lcModelActionObjectEdit::LoadEndState(lcModel* Model) const
{
	LoadUndoBuffer(mEndBuffer, Model);
}

lcModelActionAddPieces::lcModelActionAddPieces(lcStep Step, lcModelActionAddPieceSelectionMode SelectionMode)
	: mStep(Step), mSelectionMode(SelectionMode)
{
}

void lcModelActionAddPieces::SetPieceData(const std::vector<lcInsertPieceInfo>& PieceInfoTransforms)
{
	mPieceData.clear();
	mPieceData.reserve(PieceInfoTransforms.size());

	for (const lcInsertPieceInfo& PieceInfoTransform : PieceInfoTransforms)
	{
		lcModelActionAddPieces::PieceData& PieceData = mPieceData.emplace_back();

		PieceData.PieceId = PieceInfoTransform.Info->mFileName;
		PieceData.Transform = PieceInfoTransform.Transform;
		PieceData.ColorCode = lcGetColorCode(PieceInfoTransform.ColorIndex);
	}
}

lcModelActionAddCamera::lcModelActionAddCamera(const lcVector3& Position, const lcVector3& TargetPosition)
    : mPosition(Position), mTargetPosition(TargetPosition)
{
}

lcModelActionAddLight::lcModelActionAddLight(const lcVector3& Position, lcLightType LightType)
	: mPosition(Position), mLightType(LightType)
{
}

lcModelActionGroupPieces::lcModelActionGroupPieces(lcModelActionGroupPiecesMode Mode, const QString& GroupName)
	: mMode(Mode), mGroupName(GroupName)
{
}

lcModelActionDuplicatePieces::lcModelActionDuplicatePieces(lcStep Step)
	: mStep(Step)
{
}
