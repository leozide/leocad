#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_colors.h"

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

lcModelActionStep::lcModelActionStep(lcModelActionStepMode Mode, lcStep Step)
    : mMode(Mode), mStep(Step)
{
}

void lcModelActionStep::SaveState(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights)
{
	mPieceStates.resize(Pieces.size());
	
	for (size_t PieceIndex = 0; PieceIndex < Pieces.size(); PieceIndex++)
	{
		lcModelActionStepPieceState& PieceState = mPieceStates[PieceIndex];
		const lcPiece* Piece = Pieces[PieceIndex].get();
		
		PieceState.StepShow = Piece->GetStepShow();
		PieceState.StepHide = Piece->GetStepHide();
		
		QDataStream Stream(&PieceState.KeyFrames, QIODevice::WriteOnly);
		
		Piece->SaveKeyFrames(Stream);
	}

	mCameraStates.resize(Cameras.size());
	
	for (size_t CameraIndex = 0; CameraIndex < Cameras.size(); CameraIndex++)
	{
		lcModelActionStepCameraState& CameraState = mCameraStates[CameraIndex];
		const lcCamera* Camera = Cameras[CameraIndex].get();
		
		QDataStream Stream(&CameraState.KeyFrames, QIODevice::WriteOnly);
		
		Camera->SaveKeyFrames(Stream);
	}

	mLightStates.resize(Lights.size());
	
	for (size_t LightIndex = 0; LightIndex < Lights.size(); LightIndex++)
	{
		lcModelActionStepLightState& LightState = mLightStates[LightIndex];
		const lcLight* Light = Lights[LightIndex].get();
		
		QDataStream Stream(&LightState.KeyFrames, QIODevice::WriteOnly);
		
		Light->SaveKeyFrames(Stream);
	}
}
