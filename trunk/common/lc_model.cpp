#include "lc_global.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"

void lcModelProperties::LoadDefaults()
{
	mAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);

	mBackgroundType = (lcBackgroundType)lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TYPE);
	mBackgroundSolidColor = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR));
	mBackgroundGradientColor1 = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1));
	mBackgroundGradientColor2 = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2));
	mBackgroundImage = lcGetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE);
	mBackgroundImageTile = lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE);

	mFogEnabled = lcGetProfileInt(LC_PROFILE_DEFAULT_FOG_ENABLED);
	mFogDensity = lcGetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY);
	mFogColor = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR));
	mAmbientColor = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR));
}

void lcModelProperties::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TYPE, mBackgroundType);
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR, lcColorFromVector3(mBackgroundSolidColor));
	lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1, lcColorFromVector3(mBackgroundGradientColor1));
	lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2, lcColorFromVector3(mBackgroundGradientColor2));
	lcSetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE, mBackgroundImage);
	lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE, mBackgroundImageTile);

	lcSetProfileInt(LC_PROFILE_DEFAULT_FOG_ENABLED, mFogEnabled);
	lcSetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY, mFogDensity);
	lcSetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR, lcColorFromVector3(mFogColor));
	lcSetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR, lcColorFromVector3(mAmbientColor));
}

QJsonObject lcModelProperties::Save()
{
	QJsonObject Properties;

	if (!mName.IsEmpty())
		Properties[QStringLiteral("Name")] = QString::fromLatin1(mName.Buffer());
	if (!mAuthor.IsEmpty())
		Properties[QStringLiteral("Author")] = QString::fromLatin1(mAuthor.Buffer());
	if (!mDescription.IsEmpty())
		Properties[QStringLiteral("Description")] = QString::fromLatin1(mDescription.Buffer());
	if (!mComments.IsEmpty())
		Properties[QStringLiteral("Comments")] = QString::fromLatin1(mComments.Buffer());

	switch (mBackgroundType)
	{
	case LC_BACKGROUND_SOLID:
		Properties[QStringLiteral("Background")] = QStringLiteral("Solid");
		break;
	case LC_BACKGROUND_GRADIENT:
		Properties[QStringLiteral("Background")] = QStringLiteral("Gradient");
		break;
	case LC_BACKGROUND_IMAGE:
		Properties[QStringLiteral("Background")] = QStringLiteral("Image");
		break;
	}

	Properties[QStringLiteral("BackgroundSolidColor")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundSolidColor[0]), QString::number(mBackgroundSolidColor[1]), QString::number(mBackgroundSolidColor[2]));
	Properties[QStringLiteral("BackgroundGradientColor1")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundGradientColor1[0]), QString::number(mBackgroundGradientColor1[1]), QString::number(mBackgroundGradientColor1[2]));
	Properties[QStringLiteral("BackgroundGradientColor2")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundGradientColor2[0]), QString::number(mBackgroundGradientColor2[1]), QString::number(mBackgroundGradientColor2[2]));
	if (!mBackgroundImage.IsEmpty())
		Properties[QStringLiteral("BackgroundImage")] = QString::fromLatin1(mBackgroundImage.Buffer());
	if (mBackgroundImageTile)
		Properties[QStringLiteral("BackgroundImageTile")] = QStringLiteral("true");

	if (mFogEnabled)
		Properties[QStringLiteral("FogEnabled")] = QStringLiteral("true");
	Properties[QStringLiteral("FogDensity")] = QString::number(mFogDensity);
	Properties[QStringLiteral("FogColor")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundSolidColor[0]), QString::number(mBackgroundSolidColor[1]), QString::number(mBackgroundSolidColor[2]));
	Properties[QStringLiteral("AmbientColor")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundSolidColor[0]), QString::number(mBackgroundSolidColor[1]), QString::number(mBackgroundSolidColor[2]));

	return Properties;
}

void lcModelProperties::Load(QJsonObject Properties)
{
}

lcModel::lcModel()
{
	mSavedHistory = NULL;
}

lcModel::~lcModel()
{
}

QJsonObject lcModel::Save()
{
	QJsonObject Model;

	Model["Properties"] = mProperties.Save();
	Model["CurrentStep"] = QString::number(mCurrentStep);

	QJsonArray Pieces;
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Pieces.append(mPieces[PieceIdx]->Save());
	Model["Pieces"] = Pieces;

	QJsonArray Cameras;
	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		Cameras.append(mCameras[CameraIdx]->Save());
	Model["Cameras"] = Cameras;

	QJsonArray Lights;
	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		Lights.append(mLights[LightIdx]->Save());
	Model["Lights"] = Lights;

//	lcArray<lcGroup*> mGroups;

	return Model;
}

void lcModel::Load(QJsonObject Model)
{

}

lcStep lcModel::GetLastStep() const
{
	lcStep Step = 1;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Step = lcMax(Step, mPieces[PieceIdx]->GetStepShow());

	return Step;
}

lcObject* lcModel::GetFocusObject() const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
			return Piece;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsFocused())
			return Camera;
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsFocused())
			return Light;
	}

	return NULL;
}

void lcModel::UpdateSelection() const
{
	int Flags = 0;
	int SelectedCount = 0;
	lcObject* Focus = NULL;

	if (mPieces.IsEmpty())
		Flags |= LC_SEL_NO_PIECES;
	else
	{
		lcGroup* pGroup = NULL;
		bool first = true;

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->IsSelected())
			{
				SelectedCount++;

				if (Piece->IsFocused())
					Focus = Piece;

				Flags |= LC_SEL_PIECE | LC_SEL_SELECTED;

				if (Piece->GetGroup() != NULL)
				{
					Flags |= LC_SEL_GROUPED;
					if (Piece->IsFocused())
						Flags |= LC_SEL_FOCUS_GROUPED;
				}

				if (first)
				{
					pGroup = Piece->GetGroup();
					first = false;
				}
				else
				{
					if (pGroup != Piece->GetGroup())
						Flags |= LC_SEL_CAN_GROUP;
					else
						if (pGroup == NULL)
							Flags |= LC_SEL_CAN_GROUP;
				}
			}
			else
			{
				Flags |= LC_SEL_UNSELECTED;

				if (Piece->IsHidden())
					Flags |= LC_SEL_HIDDEN;
			}
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			Flags |= LC_SEL_SELECTED;
			SelectedCount++;

			if (Camera->IsFocused())
				Focus = Camera;
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			Flags |= LC_SEL_SELECTED;
			SelectedCount++;

			if (Light->IsFocused())
				Focus = Light;
		}
	}

	gMainWindow->UpdateSelectedObjects(Flags, SelectedCount, Focus);
}

void lcModel::ClearSelection(bool UpdateInterface)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		mPieces[PieceIdx]->SetSelected(false);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->SetSelected(false);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->SetSelected(false);

	if (UpdateInterface)
	{
		UpdateSelection();
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateFocusObject(NULL);
	}
}

void lcModel::SelectGroup(lcGroup* TopGroup, bool Select)
{
	if (!TopGroup)
		return;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected() && Piece->IsVisible(mCurrentStep) && (Piece->GetTopGroup() == TopGroup))
			Piece->SetSelected(Select);
	}
}

void lcModel::FocusOrDeselectObject(const lcObjectSection& ObjectSection)
{
	lcObject* FocusObject = GetFocusObject();
	lcObject* Object = ObjectSection.Object;
	lcuint32 Section = ObjectSection.Section;

	if (Object)
	{
		bool WasSelected = Object->IsSelected();

		if (!Object->IsFocused(Section))
		{
			if (FocusObject)
				FocusObject->SetFocused(FocusObject->GetFocusSection(), false);

			Object->SetFocused(Section, true);
		}
		else
			Object->SetSelected(Section, false);

		bool IsSelected = Object->IsSelected();

		if (Object->IsPiece() && (WasSelected != IsSelected))
			SelectGroup(((lcPiece*)Object)->GetTopGroup(), IsSelected);
	}
	else
	{
		if (FocusObject)
			FocusObject->SetFocused(FocusObject->GetFocusSection(), false);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(GetFocusObject());
}

void lcModel::ClearSelectionAndSetFocus(lcObject* Object, lcuint32 Section)
{
	ClearSelection(false);

	if (Object)
	{
		Object->SetFocused(Section, true);

		if (Object->IsPiece())
			SelectGroup(((lcPiece*)Object)->GetTopGroup(), true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(Object);
}

void lcModel::ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection)
{
	ClearSelectionAndSetFocus(ObjectSection.Object, ObjectSection.Section);
}

void lcModel::SetSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	ClearSelection(false);
	AddToSelection(ObjectSections);
}

void lcModel::AddToSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	for (int ObjectIdx = 0; ObjectIdx < ObjectSections.GetSize(); ObjectIdx++)
	{
		lcObject* Object = ObjectSections[ObjectIdx].Object;

		bool WasSelected = Object->IsSelected();
		Object->SetSelected(ObjectSections[ObjectIdx].Section, true);

		if (!WasSelected && Object->GetType() == LC_OBJECT_PIECE)
			SelectGroup(((lcPiece*)Object)->GetTopGroup(), true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(GetFocusObject());
}
