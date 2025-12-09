#include "lc_global.h"
#include "lc_modelaction.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "pieceinf.h"
#include "lc_colors.h"

void lcModelAction::SaveUndoBuffer(QByteArray& Buffer, const lcModel* Model, bool SelectedOnly)
{
	QDataStream Stream(&Buffer, QIODevice::WriteOnly);

	const std::vector<std::unique_ptr<lcPiece>>& Pieces = Model->GetPieces();
	const std::vector<std::unique_ptr<lcCamera>>& Cameras = Model->GetCameras();
	const std::vector<std::unique_ptr<lcLight>>& Lights = Model->GetLights();

	uint64_t PieceCount = Pieces.size(), CameraCount = Cameras.size(), LightCount = Lights.size();

	Stream << PieceCount;
	Stream << CameraCount;
	Stream << LightCount;
	
	for (const std::unique_ptr<lcPiece>& Piece : Pieces)
		if (!SelectedOnly || Piece->IsSelected())
			Piece->SaveUndoData(Stream);

	for (const std::unique_ptr<lcCamera>& Camera : Cameras)
		if (!SelectedOnly || Camera->IsSelected())
			Camera->SaveUndoData(Stream);

	for (const std::unique_ptr<lcLight>& Light : Lights)
		if (!SelectedOnly || Light->IsSelected())
			Light->SaveUndoData(Stream);
}

void lcModelAction::LoadUndoBuffer(const QByteArray& Buffer, lcModel* Model, bool SelectedOnly)
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
		return;

	for (const std::unique_ptr<lcPiece>& Piece : Pieces)
		if (!SelectedOnly || Piece->IsSelected())
			Piece->LoadUndoData(Stream);

	for (const std::unique_ptr<lcCamera>& Camera : Cameras)
		if (!SelectedOnly || Camera->IsSelected())
			Camera->LoadUndoData(Stream);

	for (const std::unique_ptr<lcLight>& Light : Lights)
		if (!SelectedOnly || Light->IsSelected())
			Light->LoadUndoData(Stream);
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

void lcModelActionObjectEdit::SaveSelectionStartState(const lcModel* Model)
{
	SaveUndoBuffer(mStartBuffer, Model, true);
}

void lcModelActionObjectEdit::LoadSelectionStartState(lcModel* Model) const
{
	LoadUndoBuffer(mStartBuffer, Model, true);
}

void lcModelActionObjectEdit::SaveSelectionEndState(const lcModel* Model)
{
	SaveUndoBuffer(mEndBuffer, Model, true);
}

void lcModelActionObjectEdit::LoadSelectionEndState(lcModel* Model) const
{
	LoadUndoBuffer(mEndBuffer, Model, true);
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

lcModelActionStep::lcModelActionStep(lcModelActionStepMode Mode, lcStep Step)
    : mMode(Mode), mStep(Step)
{
}

void lcModelActionStep::SaveModelState(const lcModel* Model)
{
	SaveUndoBuffer(mUndoBuffer, Model, false);
}

void lcModelActionStep::LoadModelState(lcModel* Model) const
{
	LoadUndoBuffer(mUndoBuffer, Model, false);
}
