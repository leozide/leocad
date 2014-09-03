#include "lc_global.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"
#include "lc_library.h"

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

void lcModelProperties::SaveLDraw(lcFile& File) const
{
	char Line[1024];

	if (!mName.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL NAME %s\r\n", mName);
		File.WriteBuffer(Line, strlen(Line));
	}

	if (!mAuthor.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL AUTHOR %s\r\n", mAuthor);
		File.WriteBuffer(Line, strlen(Line));
	}

	if (!mDescription.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL DESCRIPTION %s\r\n", mDescription);
		File.WriteBuffer(Line, strlen(Line));
	}

	if (!mComments.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL COMMENTS %s\r\n", mComments);
		File.WriteBuffer(Line, strlen(Line));
	}

	switch (mBackgroundType)
	{
	case LC_BACKGROUND_SOLID:
		strcpy(Line, "0 !LEOCAD MODEL BACKGROUND_TYPE SOLID\r\n");
		File.WriteBuffer(Line, strlen(Line));
		break;
	case LC_BACKGROUND_GRADIENT:
		strcpy(Line, "0 !LEOCAD MODEL BACKGROUND_TYPE GRADIENT\r\n");
		File.WriteBuffer(Line, strlen(Line));
		break;
	case LC_BACKGROUND_IMAGE:
		strcpy(Line, "0 !LEOCAD MODEL BACKGROUND_TYPE IMAGE\r\n");
		File.WriteBuffer(Line, strlen(Line));
		break;
	}

	sprintf(Line, "0 !LEOCAD MODEL BACKGROUND SOLID_COLOR %.2f %.2f %.2f\r\n", mBackgroundSolidColor[0], mBackgroundSolidColor[1], mBackgroundSolidColor[2]);
	File.WriteBuffer(Line, strlen(Line));

	sprintf(Line, "0 !LEOCAD MODEL BACKGROUND GRADIENT_COLORS %.2f %.2f %.2f %.2f %.2f %.2f\r\n", mBackgroundGradientColor1[0], mBackgroundGradientColor1[1], mBackgroundGradientColor1[2], mBackgroundGradientColor2[0], mBackgroundGradientColor2[1], mBackgroundGradientColor2[2]);
	File.WriteBuffer(Line, strlen(Line));

	if (!mBackgroundImage.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL BACKGROUND IMAGE_NAME %s\r\n", mBackgroundImage);
		File.WriteBuffer(Line, strlen(Line));
	}

	if (mBackgroundImageTile)
	{
		strcat(Line, "0 !LEOCAD MODEL BACKGROUND IMAGE_TILE\r\n");
		File.WriteBuffer(Line, strlen(Line));
	}

//	bool mFogEnabled;
//	float mFogDensity;
//	lcVector3 mFogColor;
//	lcVector3 mAmbientColor;
}

void lcModelProperties::ParseLDrawLine(char** Tokens)
{
	if (!Tokens[4])
		return;

	strupr(Tokens[4]);

	if (!strcmp(Tokens[4], "NAME"))
	{
		if (Tokens[5])
		{
			strncpy(mName, Tokens[5], sizeof(mName));
			mName[sizeof(mName) - 1] = 0;
		}
		else
			mName[0] = 0;
	}
	else if (!strcmp(Tokens[4], "AUTHOR"))
	{
		if (Tokens[5])
		{
			strncpy(mAuthor, Tokens[5], sizeof(mAuthor));
			mAuthor[sizeof(mAuthor) - 1] = 0;
		}
		else
			mAuthor[0] = 0;
	}
	else if (!strcmp(Tokens[4], "DESCRIPTION"))
	{
		if (Tokens[5])
		{
			strncpy(mDescription, Tokens[5], sizeof(mDescription));
			mDescription[sizeof(mDescription) - 1] = 0;
		}
		else
			mDescription[0] = 0;
	}
	else if (!strcmp(Tokens[4], "COMMENTS"))
	{
		if (Tokens[5])
		{
			strncpy(mComments, Tokens[5], sizeof(mComments));
			mComments[sizeof(mComments) - 1] = 0;
		}
		else
			mComments[0] = 0;
	}
	else if (!strcmp(Tokens[4], "BACKGROUND_TYPE"))
	{
		if (Tokens[5])
		{
			strupr(Tokens[5]);

			if (!strcmp(Tokens[5], "SOLID"))
				mBackgroundType = LC_BACKGROUND_SOLID;
			else if (!strcmp(Tokens[5], "GRADIENT"))
				mBackgroundType = LC_BACKGROUND_GRADIENT;
			else if (!strcmp(Tokens[5], "IMAGE"))
				mBackgroundType = LC_BACKGROUND_IMAGE;
		}
	}
	else if (!strcmp(Tokens[4], "BACKGROUND"))
	{
		if (Tokens[5])
		{
			strupr(Tokens[5]);

		}
	}

/*
	sprintf(Line, "0 !LEOCAD MODEL BACKGROUND SOLID_COLOR %.2f %.2f %.2f\r\n", mBackgroundSolidColor[0], mBackgroundSolidColor[1], mBackgroundSolidColor[2]);
	File.WriteBuffer(Line, strlen(Line));

	sprintf(Line, "0 !LEOCAD MODEL BACKGROUND GRADIENT_COLORS %.2f %.2f %.2f %.2f %.2f %.2f\r\n", mBackgroundGradientColor1[0], mBackgroundGradientColor1[1], mBackgroundGradientColor1[2], mBackgroundGradientColor2[0], mBackgroundGradientColor2[1], mBackgroundGradientColor2[2]);
	File.WriteBuffer(Line, strlen(Line));

	if (!mBackgroundImage.IsEmpty())
	{
		sprintf(Line, "0 !LEOCAD MODEL BACKGROUND IMAGE_NAME %s\r\n", mBackgroundImage);
		File.WriteBuffer(Line, strlen(Line));
	}

	if (mBackgroundImageTile)
	{
		strcat(Line, "0 !LEOCAD MODEL BACKGROUND IMAGE_TILE\r\n");
		File.WriteBuffer(Line, strlen(Line));
	}
*/
}

QJsonObject lcModelProperties::Save() const
{
	QJsonObject Properties;

	if (!mName.IsEmpty())
		Properties[QStringLiteral("Name")] = QString::fromLatin1(mName); // todo: qstring
	if (!mAuthor.IsEmpty())
		Properties[QStringLiteral("Author")] = QString::fromLatin1(mAuthor);
	if (!mDescription.IsEmpty())
		Properties[QStringLiteral("Description")] = QString::fromLatin1(mDescription);
	if (!mComments.IsEmpty())
		Properties[QStringLiteral("Comments")] = QString::fromLatin1(mComments);

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
		Properties[QStringLiteral("BackgroundImage")] = QString::fromLatin1(mBackgroundImage); // todo: qstring
	if (mBackgroundImageTile)
		Properties[QStringLiteral("BackgroundImageTile")] = QStringLiteral("true");

	if (mFogEnabled)
		Properties[QStringLiteral("FogEnabled")] = QStringLiteral("true");
	Properties[QStringLiteral("FogDensity")] = QString::number(mFogDensity);
	Properties[QStringLiteral("FogColor")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundSolidColor[0]), QString::number(mBackgroundSolidColor[1]), QString::number(mBackgroundSolidColor[2]));
	Properties[QStringLiteral("AmbientColor")] = QStringLiteral("%1 %2 %3").arg(QString::number(mBackgroundSolidColor[0]), QString::number(mBackgroundSolidColor[1]), QString::number(mBackgroundSolidColor[2]));

	return Properties;
}

bool lcModelProperties::Load(const QJsonObject& Properties)
{
	return true;
}

lcModel::lcModel()
{
	mSavedHistory = NULL;
}

lcModel::~lcModel()
{
}

void lcModel::SaveLDraw(lcFile& File) const
{
	mProperties.SaveLDraw(File);

	lcStep LastStep = GetLastStep();

	if (mCurrentStep != LastStep)
	{
		char Line[1024];
		sprintf(Line, "0 !LEOCAD MODEL CURRENT_STEP %d\r\n", mCurrentStep);
		File.WriteBuffer(Line, strlen(Line));
	}

	for (lcStep Step = 1; Step <= LastStep; Step++)
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->GetStepShow() == Step)
				Piece->SaveLDraw(File);
		}

		if (Step != LastStep)
			File.WriteBuffer("0 STEP\r\n", 8);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->SaveLDraw(File);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->SaveLDraw(File);

//	lcArray<lcGroup*> mGroups;
}

void lcModel::LoadLDraw(const QStringList& Lines, const lcMatrix44& CurrentTransform, int DefaultColorCode, int& CurrentStep)
{
	lcPiece* Piece = NULL;
	lcCamera* Camera = NULL;
	lcLight* Light = NULL;

	for (int LineIdx = 0; LineIdx < Lines.size(); LineIdx++)
	{
		QString Line = Lines[LineIdx].trimmed();
		QTextStream Stream(&Line, QIODevice::ReadOnly);

		QString Token;
		Stream >> Token;

		if (Token == QStringLiteral("0"))
		{
			Stream >> Token;

			if (Token == QStringLiteral("STEP"))
			{
				CurrentStep++;
				continue;
			}

			if (Token != QStringLiteral("!LEOCAD"))
				continue;

			Stream >> Token;

			if (Token == QStringLiteral("MODEL"))
			{
//				if (!strcmp(Tokens[3], "CURRENT_STEP") && Tokens[4])
//					mCurrentStep = atoi(Tokens[4]);

//				mProperties.ParseLDrawLine(Tokens);
			}
			else if (Token == QStringLiteral("PIECE"))
			{
				if (!Piece)
					Piece = new lcPiece(NULL);

				Piece->ParseLDrawLine(Stream, this);
			}
			else if (Token == QStringLiteral("CAMERA"))
			{
				if (!Camera)
					Camera = new lcCamera(false);

				if (Camera->ParseLDrawLine(Stream))
				{
					Camera->CreateName(mCameras);
					mCameras.Add(Camera);
					Camera = NULL;
				}
			}
			else if (Token == QStringLiteral("LIGHT"))
			{
			}
			else if (Token == QStringLiteral("GROUP"))
			{
			}

			continue;
		}
		else if (Token == QStringLiteral("1"))
		{
			int ColorCode;
			Stream >> ColorCode;
			if (ColorCode == 16)
				ColorCode = DefaultColorCode;

			float Matrix[12];
			for (int TokenIdx = 0; TokenIdx < 12; TokenIdx++)
				Stream >> Matrix[TokenIdx];

			lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[6], Matrix[9], 0.0f), lcVector4(Matrix[4], Matrix[7], Matrix[10], 0.0f),
			                            lcVector4(Matrix[5], Matrix[8], Matrix[11], 0.0f), lcVector4(Matrix[0], Matrix[1], Matrix[2], 1.0f));

			IncludeTransform = lcMul(IncludeTransform, CurrentTransform);

			QString File;
			Stream >> File;

			QString PartID = File.toUpper();
			if (PartID.endsWith(QStringLiteral(".DAT")))
				PartID = PartID.left(PartID.size() - 4);

			if (!Piece)
				Piece = new lcPiece(NULL);

			PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(PartID.toLatin1().constData(), false);
			if (Info != NULL)
			{
				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
				                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
				AxisAngle[3] *= LC_RTOD;

				Piece->SetPieceInfo(Info);
				Piece->Initialize(lcVector3(IncludeTransform[3].x, IncludeTransform[3].z, -IncludeTransform[3].y), AxisAngle, CurrentStep);
				Piece->SetColorCode(ColorCode);
				Piece->CreateName(mPieces);
				mPieces.Add(Piece);
				Piece = NULL;
				continue;
			}

			// todo: mpd
			// todo: load from disk

			Info = lcGetPiecesLibrary()->FindPiece(PartID.toLatin1().constData(), true);
			if (Info != NULL)
			{
				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
				                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
				AxisAngle[3] *= LC_RTOD;

				Piece->SetPieceInfo(Info);
				Piece->Initialize(lcVector3(IncludeTransform[3].x, IncludeTransform[3].z, -IncludeTransform[3].y), AxisAngle, CurrentStep);
				Piece->SetColorCode(ColorCode);
				Piece->CreateName(mPieces);
				mPieces.Add(Piece);
				Piece = NULL;
				continue;
			}
		}
	}

	delete Piece;
	delete Camera;
	delete Light;
}

QJsonObject lcModel::SaveJson() const
{
	QJsonObject Model;

	Model["Properties"] = mProperties.Save();
	Model["CurrentStep"] = (int)mCurrentStep;

	QJsonArray Pieces;
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Pieces.append(mPieces[PieceIdx]->SaveJson());
	Model["Pieces"] = Pieces;

	QJsonArray Cameras;
	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		Cameras.append(mCameras[CameraIdx]->SaveJson());
	Model["Cameras"] = Cameras;

	QJsonArray Lights;
	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		Lights.append(mLights[LightIdx]->Save());
	Model["Lights"] = Lights;

//	lcArray<lcGroup*> mGroups;

	return Model;
}

bool lcModel::LoadJson(const QJsonObject& Model)
{
	QJsonValue Properties = Model.value(QStringLiteral("Properties"));
	if (!Properties.isUndefined())
		mProperties.Load(Properties.toObject());

	QJsonValue CurrentStep = Model.value(QStringLiteral("CurrentStep"));
	if (!CurrentStep.isUndefined())
		mCurrentStep = CurrentStep.toInt();

	QJsonValue Pieces = Model.value(QStringLiteral("Pieces"));
	if (!Pieces.isUndefined())
	{
		QJsonArray PiecesArray = Pieces.toArray();

		for (int PieceIdx = 0; PieceIdx < PiecesArray.size(); PieceIdx++)
		{
			lcPiece* Piece = new lcPiece(NULL);

			if (Piece->LoadJson(PiecesArray[PieceIdx].toObject()))
				mPieces.Add(Piece);
			else
				delete Piece;
		}
	}

	QJsonValue Cameras = Model.value(QStringLiteral("Cameras"));
	if (!Cameras.isUndefined())
	{
		QJsonArray CamerasArray = Cameras.toArray();

		for (int CameraIdx = 0; CameraIdx < CamerasArray.size(); CameraIdx++)
		{
			lcCamera* Camera = new lcCamera(false);

			if (Camera->LoadJson(CamerasArray[CameraIdx].toObject()))
				mCameras.Add(Camera);
			else
				delete Camera;
		}
	}

	QJsonValue Lights = Model.value(QStringLiteral("Lights"));
	if (!Lights.isUndefined())
	{
		QJsonArray LightsArray = Lights.toArray();

		for (int LightIdx = 0; LightIdx < LightsArray.size(); LightIdx++)
		{
/*			lcLight* Light = new lcLight();

			if (Light->Load(LightsArray[LightIdx].toObject()))
				mLights.Add(Light);
			else
				delete Light;
*/		}
	}

// groups

	CalculateStep();
/*
	if (!bMerge)
		gMainWindow->UpdateFocusObject(GetFocusObject());

	if (!bMerge)
	{
		for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		{
			View* view = Views[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}

		if (!bUndo)
			ZoomExtents(0, Views.GetSize());
	}
*/
	gMainWindow->UpdateCameraMenu();
	UpdateSelection();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateAllViews();

	return true;
}

void lcModel::CalculateStep()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->UpdatePosition(mCurrentStep);

		if (Piece->IsSelected())
		{
			if (!Piece->IsVisible(mCurrentStep))
				Piece->SetSelected(false);
			else
				SelectGroup(Piece->GetTopGroup(), true);
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->UpdatePosition(mCurrentStep);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->UpdatePosition(mCurrentStep);
}

lcStep lcModel::GetLastStep() const
{
	lcStep Step = 1;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Step = lcMax(Step, mPieces[PieceIdx]->GetStepShow());

	return Step;
}

lcGroup* lcModel::GetGroup(const char* Name, bool CreateIfMissing)
{
	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];

		if (!strcmp(Group->m_strName, Name))
			return Group;
	}

	if (CreateIfMissing)
	{
		lcGroup* Group = new lcGroup();
		mGroups.Add(Group);

		strncpy(Group->m_strName, Name, sizeof(Group->m_strName));
		Group->m_strName[sizeof(Group->m_strName) - 1] = 0;

		return Group;
	}

	return NULL;
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
