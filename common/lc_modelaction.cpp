#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_colors.h"

bool lcModelAction::SaveUndoBuffer(QByteArray& Buffer, const lcModel* Model, bool SelectedOnly)
{
	QDataStream Stream(&Buffer, QIODevice::WriteOnly);

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t PieceCount = Pieces.size(), CameraCount = Cameras.size(), LightCount = Lights.size();

	Stream << PieceCount;
	Stream << CameraCount;
	Stream << LightCount;
	
	for (uint64_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		const std::unique_ptr<lcPiece>& Piece = Pieces[PieceIndex];
		
		if (!SelectedOnly || Piece->IsSelected())
		{
			if (Stream.writeRawData(reinterpret_cast<const char*>(&PieceIndex), sizeof(PieceIndex)) != sizeof(PieceIndex))
				return false;
			
			if (!Piece->SaveUndoData(Stream))
				return false;
		}
	}
	
	if (Stream.writeRawData(reinterpret_cast<const char*>(&mEndOfList), sizeof(mEndOfList)) != sizeof(mEndOfList))
		return false;
	
	for (uint64_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		const std::unique_ptr<lcCamera>& Camera = Cameras[CameraIndex];
		
		if (!SelectedOnly || Camera->IsSelected())
		{
			if (Stream.writeRawData(reinterpret_cast<const char*>(&CameraIndex), sizeof(CameraIndex)) != sizeof(CameraIndex))
				return false;
			
			if (!Camera->SaveUndoData(Stream))
				return false;
		}
	}
	
	if (Stream.writeRawData(reinterpret_cast<const char*>(&mEndOfList), sizeof(mEndOfList)) != sizeof(mEndOfList))
		return false;
	
	for (uint64_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		const std::unique_ptr<lcLight>& Light = Lights[LightIndex];
		
		if (!SelectedOnly || Light->IsSelected())
		{
			if (Stream.writeRawData(reinterpret_cast<const char*>(&LightIndex), sizeof(LightIndex)) != sizeof(LightIndex))
				return false;
			
			if (!Light->SaveUndoData(Stream))
				return false;
		}
	}
	
	if (Stream.writeRawData(reinterpret_cast<const char*>(&mEndOfList), sizeof(mEndOfList)) != sizeof(mEndOfList))
		return false;
	
	return true;
}

bool lcModelAction::LoadUndoBuffer(const QByteArray& Buffer, lcModel* Model)
{
	QDataStream Stream(const_cast<QByteArray*>(&Buffer), QIODevice::ReadOnly);

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t PieceCount, CameraCount, LightCount;

	Stream >> PieceCount;
	Stream >> CameraCount;
	Stream >> LightCount;

	if (PieceCount != Pieces.size() || CameraCount != Cameras.size() || LightCount != Lights.size())
		return false;
	
	for (;;)
	{
		uint64_t PieceIndex;
		
		if (Stream.readRawData(reinterpret_cast<char*>(&PieceIndex), sizeof(PieceIndex)) != sizeof(PieceIndex))
			return false;
		
		if (PieceIndex == mEndOfList)
			break;
		
		if (PieceIndex >= Pieces.size())
			return false;
		
		const std::unique_ptr<lcPiece>& Piece = Pieces[PieceIndex];
		
		if (!Piece->LoadUndoData(Stream))
			return false;
	}
	
	for (;;)
	{
		uint64_t CameraIndex;
		
		if (Stream.readRawData(reinterpret_cast<char*>(&CameraIndex), sizeof(CameraIndex)) != sizeof(CameraIndex))
			return false;
		
		if (CameraIndex == mEndOfList)
			break;
		
		if (CameraIndex >= Cameras.size())
			return false;
		
		const std::unique_ptr<lcCamera>& Camera = Cameras[CameraIndex];
		
		if (!Camera->LoadUndoData(Stream))
			return false;
	}
	
	for (;;)
	{
		uint64_t LightIndex;
		
		if (Stream.readRawData(reinterpret_cast<char*>(&LightIndex), sizeof(LightIndex)) != sizeof(LightIndex))
			return false;
		
		if (LightIndex == mEndOfList)
			break;
		
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

void lcModelActionObjectEdit::SaveCameraStartState(const lcCamera* Camera)
{
	QDataStream Stream(&mStartBuffer, QIODevice::WriteOnly);
	
	Camera->SaveUndoData(Stream);

	mCameraName = Camera->GetName();
}

void lcModelActionObjectEdit::LoadCameraStartState(lcCamera* Camera) const
{
	QDataStream Stream(const_cast<QByteArray*>(&mStartBuffer), QIODevice::ReadOnly);
	
	Camera->LoadUndoData(Stream);
}

void lcModelActionObjectEdit::SaveCameraEndState(const lcCamera* Camera)
{
	QDataStream Stream(&mEndBuffer, QIODevice::WriteOnly);
	
	Camera->SaveUndoData(Stream);
}

void lcModelActionObjectEdit::LoadCameraEndState(lcCamera* Camera) const
{
	QDataStream Stream(const_cast<QByteArray*>(&mEndBuffer), QIODevice::ReadOnly);
	
	Camera->LoadUndoData(Stream);
}

void lcModelActionObjectEdit::SaveModelStartState(const lcModel* Model)
{
	SaveUndoBuffer(mStartBuffer, Model, mMode == lcModelActionObjectEditMode::Selection);
}

void lcModelActionObjectEdit::LoadModelStartState(lcModel* Model) const
{
	LoadUndoBuffer(mStartBuffer, Model);
}

void lcModelActionObjectEdit::SaveModelEndState(const lcModel* Model)
{
	SaveUndoBuffer(mEndBuffer, Model, mMode == lcModelActionObjectEditMode::Selection);
}

void lcModelActionObjectEdit::LoadModelEndState(lcModel* Model) const
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

lcModelActionHidePieces::lcModelActionHidePieces(lcModelActionHidePiecesMode Mode)
	: mMode(Mode)
{
}

void lcModelActionHidePieces::SaveHiddenState(const std::vector<std::unique_ptr<lcPiece>>& Pieces)
{
	mHiddenState.resize(Pieces.size());

	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
		mHiddenState[PieceIndex] = Pieces[PieceIndex]->IsHidden();
}
