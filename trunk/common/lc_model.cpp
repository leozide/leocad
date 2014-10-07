#include "lc_global.h"
#include "lc_model.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"
#include "lc_library.h"
#include "lc_texture.h"
#include "view.h"
#include "preview.h"

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

void lcModelProperties::SaveLDraw(QTextStream& Stream) const
{
	QLatin1String LineEnding("\r\n");

	if (!mName.isEmpty())
		Stream << QLatin1String("0 !LEOCAD MODEL NAME ") << mName << LineEnding;

	if (!mAuthor.isEmpty())
		Stream << QLatin1String("0 !LEOCAD MODEL AUTHOR ") << mAuthor << LineEnding;

	if (!mDescription.isEmpty())
		Stream << QLatin1String("0 !LEOCAD MODEL DESCRIPTION ") << mDescription << LineEnding;

	if (!mComments.isEmpty())
	{
		QStringList Comments = mComments.split('\n');
		foreach (const QString& Comment, Comments)
			Stream << QLatin1String("0 !LEOCAD MODEL COMMENT ") << Comment << LineEnding;
	}

	for (int BackgroundIdx = 0; BackgroundIdx < LC_NUM_BACKGROUND_TYPES; BackgroundIdx++)
	{
		switch ((mBackgroundType + 1 + BackgroundIdx) % LC_NUM_BACKGROUND_TYPES)
		{
		case LC_BACKGROUND_SOLID:
			Stream << QLatin1String("0 !LEOCAD MODEL BACKGROUND COLOR ") << mBackgroundSolidColor[0] << ' ' << mBackgroundSolidColor[1] << ' ' << mBackgroundSolidColor[2] << LineEnding;
			break;
		case LC_BACKGROUND_GRADIENT:
			Stream << QLatin1String("0 !LEOCAD MODEL BACKGROUND GRADIENT ") << mBackgroundGradientColor1[0] << ' ' << mBackgroundGradientColor1[1] << ' ' << mBackgroundGradientColor1[2] << ' ' << mBackgroundGradientColor2[0] << ' ' << mBackgroundGradientColor2[1] << ' ' << mBackgroundGradientColor2[2] << LineEnding;
			break;
		case LC_BACKGROUND_IMAGE:
			if (!mBackgroundImage.isEmpty())
			{
				Stream << QLatin1String("0 !LEOCAD MODEL BACKGROUND IMAGE ");
				if (mBackgroundImageTile)
					Stream << QLatin1String("TILE ");
				Stream << QLatin1String("NAME ") << mBackgroundImage << LineEnding;
			}
			break;
		}
	}

//	bool mFogEnabled;
//	float mFogDensity;
//	lcVector3 mFogColor;
//	lcVector3 mAmbientColor;
}

void lcModelProperties::ParseLDrawLine(QTextStream& Stream)
{
	QString Token;
	Stream >> Token;

	if (Token == QLatin1String("NAME"))
		Stream >> mName;
	else if (Token == QLatin1String("AUTHOR"))
		Stream >> mAuthor;
	else if (Token == QLatin1String("DESCRIPTION"))
		Stream >> mDescription;
	else if (Token == QLatin1String("COMMENT"))
	{
		QString Comment;
		Stream >> Comment;
		if (!mComments.isEmpty())
			mComments += '\n';
		mComments += Comment;
	}
	else if (Token == QLatin1String("BACKGROUND"))
	{
		Stream >> Token;

		if (Token == QLatin1String("COLOR"))
		{
			mBackgroundType = LC_BACKGROUND_SOLID;
			Stream >> mBackgroundSolidColor[0] >> mBackgroundSolidColor[1] >> mBackgroundSolidColor[2];
		}
		else if (Token == QLatin1String("GRADIENT"))
		{
			mBackgroundType = LC_BACKGROUND_GRADIENT;
			Stream >> mBackgroundGradientColor1[0] >> mBackgroundGradientColor1[1] >> mBackgroundGradientColor1[2] >> mBackgroundGradientColor2[0] >> mBackgroundGradientColor2[1] >> mBackgroundGradientColor2[2];
		}
		else if (Token == QLatin1String("IMAGE"))
		{
			Stream >> Token;

			if (Token == QLatin1String("TILE"))
			{
				mBackgroundImageTile = true;
				Stream >> Token;
			}

			if (Token == QLatin1String("NAME"))
				mBackgroundImage = Stream.readLine();
		}
	}
}

lcModel::lcModel()
{
	mBackgroundTexture = NULL;
	mSavedHistory = NULL;
}

lcModel::~lcModel()
{
	DeleteModel();
	DeleteHistory();
}

void lcModel::DeleteHistory()
{
	mUndoHistory.DeleteAll();
	mRedoHistory.DeleteAll();
}

void lcModel::DeleteModel()
{
	lcReleaseTexture(mBackgroundTexture);
	mBackgroundTexture = NULL;

	const lcArray<View*> Views = gMainWindow->GetViews();

	for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
	{
		View* View = Views[ViewIdx];
		lcCamera* Camera = View->mCamera;

		if (!Camera->IsSimple())
			View->SetCamera(Camera, true);
	}

	mPieces.DeleteAll();
	mCameras.DeleteAll();
	mLights.DeleteAll();
	mGroups.DeleteAll();
}

void lcModel::SaveLDraw(QTextStream& Stream) const
{
	QLatin1String LineEnding("\r\n");

	mProperties.SaveLDraw(Stream);

	lcStep LastStep = GetLastStep();
	if (mCurrentStep != LastStep)
		Stream << QLatin1String("0 !LEOCAD MODEL CURRENT_STEP") << mCurrentStep << LineEnding;

	lcArray<lcGroup*> CurrentGroups;

	for (lcStep Step = 1; Step <= LastStep; Step++)
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->GetStepShow() != Step)
				continue;

			lcGroup* PieceGroup = Piece->GetGroup();

			if (PieceGroup)
			{
				if (CurrentGroups.IsEmpty() || (!CurrentGroups.IsEmpty() && PieceGroup != CurrentGroups[CurrentGroups.GetSize() - 1]))
				{
					lcArray<lcGroup*> PieceParents;

					for (lcGroup* Group = PieceGroup; Group; Group = Group->mGroup)
						PieceParents.InsertAt(0, Group);

					int FoundParent = -1;

					while (!CurrentGroups.IsEmpty())
					{
						lcGroup* Group = CurrentGroups[CurrentGroups.GetSize() - 1];
						int Index = PieceParents.FindIndex(Group);

						if (Index == -1)
						{
							CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
							Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
						}
						else
						{
							FoundParent = Index;
							break;
						}
					}

					for (int ParentIdx = FoundParent + 1; ParentIdx < PieceParents.GetSize(); ParentIdx++)
					{
						lcGroup* Group = PieceParents[ParentIdx];
						CurrentGroups.Add(Group);
						Stream << QLatin1String("0 !LEOCAD GROUP BEGIN ") << Group->m_strName << LineEnding;
					}
				}
			}
			else
			{
				while (CurrentGroups.GetSize())
				{
					CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
					Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
				}
			}

			Piece->SaveLDraw(Stream);
		}

		if (Step != LastStep)
			Stream << QLatin1String("0 STEP\r\n");
	}

	while (CurrentGroups.GetSize())
	{
		CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
		Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->SaveLDraw(Stream);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->SaveLDraw(Stream);
}

void lcModel::LoadLDraw(QTextStream& Stream)
{
	lcPiece* Piece = NULL;
	lcCamera* Camera = NULL;
	lcLight* Light = NULL;
	lcArray<lcGroup*> CurrentGroups;
	int CurrentStep = 1;

	while (!Stream.atEnd())
	{
		QString Line = Stream.readLine().trimmed();
		QTextStream LineStream(&Line, QIODevice::ReadOnly);

		QString Token;
		LineStream >> Token;

		if (Token == QLatin1String("0"))
		{
			LineStream >> Token;

			if (Token == QLatin1String("STEP"))
			{
				CurrentStep++;
				continue;
			}

			if (Token != QLatin1String("!LEOCAD"))
				continue;

			LineStream >> Token;

			if (Token == QLatin1String("MODEL"))
			{
//				if (!strcmp(Tokens[3], "CURRENT_STEP") && Tokens[4])
//					mCurrentStep = atoi(Tokens[4]);

				mProperties.ParseLDrawLine(LineStream);
			}
			else if (Token == QLatin1String("PIECE"))
			{
				if (!Piece)
					Piece = new lcPiece(NULL);

				Piece->ParseLDrawLine(LineStream);
			}
			else if (Token == QLatin1String("CAMERA"))
			{
				if (!Camera)
					Camera = new lcCamera(false);

				if (Camera->ParseLDrawLine(LineStream))
				{
					Camera->CreateName(mCameras);
					mCameras.Add(Camera);
					Camera = NULL;
				}
			}
			else if (Token == QLatin1String("LIGHT"))
			{
			}
			else if (Token == QLatin1String("GROUP"))
			{
				LineStream >> Token;

				if (Token == QLatin1String("BEGIN"))
				{
					QString Name = LineStream.readAll().trimmed();
					QByteArray NameUtf = Name.toUtf8(); // todo: replace with qstring
					lcGroup* Group = GetGroup(NameUtf.constData(), true);
					if (!CurrentGroups.IsEmpty())
						Group->mGroup = CurrentGroups[CurrentGroups.GetSize() - 1];
					else
						Group->mGroup = NULL;
					CurrentGroups.Add(Group);
				}
				else if (Token == QLatin1String("END"))
				{
					if (!CurrentGroups.IsEmpty())
						CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
				}
			}

			continue;
		}
		else if (Token == QLatin1String("1"))
		{
			int ColorCode;
			LineStream >> ColorCode;

			float Matrix[12];
			for (int TokenIdx = 0; TokenIdx < 12; TokenIdx++)
				LineStream >> Matrix[TokenIdx];

			lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[6], Matrix[9], 0.0f), lcVector4(Matrix[4], Matrix[7], Matrix[10], 0.0f),
			                            lcVector4(Matrix[5], Matrix[8], Matrix[11], 0.0f), lcVector4(Matrix[0], Matrix[1], Matrix[2], 1.0f));

			QString File;
			LineStream >> File;

			QString PartID = File.toUpper();
			if (PartID.endsWith(QLatin1String(".DAT")))
				PartID = PartID.left(PartID.size() - 4);

			if (!Piece)
				Piece = new lcPiece(NULL);

			if (!CurrentGroups.IsEmpty())
				Piece->SetGroup(CurrentGroups[CurrentGroups.GetSize() - 1]);

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

	CalculateStep();
	UpdateBackgroundTexture();

	delete Piece;
	delete Camera;
	delete Light;
}

void lcModel::UpdateBackgroundTexture()
{
	lcReleaseTexture(mBackgroundTexture);
	mBackgroundTexture = NULL;

	if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		mBackgroundTexture = lcLoadTexture(mProperties.mBackgroundImage, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV);

		if (!mBackgroundTexture)
			mProperties.mBackgroundType = LC_BACKGROUND_SOLID;
	}
}

void lcModel::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->RayTest(ObjectRayTest);
	}

	if (ObjectRayTest.PiecesOnly)
		return;

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera != ObjectRayTest.ViewCamera && Camera->IsVisible())
			Camera->RayTest(ObjectRayTest);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->RayTest(ObjectRayTest);
}

void lcModel::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->BoxTest(ObjectBoxTest);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera != ObjectBoxTest.ViewCamera && Camera->IsVisible())
			Camera->BoxTest(ObjectBoxTest);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->BoxTest(ObjectBoxTest);
}

void lcModel::SaveCheckpoint(const QString& Description)
{
	lcModelHistoryEntry* ModelHistoryEntry = new lcModelHistoryEntry();

	ModelHistoryEntry->Description = Description;

	QTextStream Stream(&ModelHistoryEntry->File);
	SaveLDraw(Stream);

	mUndoHistory.InsertAt(0, ModelHistoryEntry);
	mRedoHistory.DeleteAll();

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : QString(), !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : QString());
}

void lcModel::LoadCheckPoint(lcModelHistoryEntry* CheckPoint)
{
	DeleteModel();

	QTextStream Stream(CheckPoint->File, QIODevice::ReadOnly);
	LoadLDraw(Stream);

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateCameraMenu();
	UpdateSelection();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateAllViews();
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

void lcModel::RemoveEmptyGroups()
{
	bool Removed;

	do
	{
		Removed = false;

		for (int GroupIdx = 0; GroupIdx < mGroups.GetSize();)
		{
			lcGroup* Group = mGroups[GroupIdx];
			int Ref = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				if (mPieces[PieceIdx]->GetGroup() == Group)
					Ref++;

			for (int ParentIdx = 0; ParentIdx < mGroups.GetSize(); ParentIdx++)
				if (mGroups[ParentIdx]->mGroup == Group)
					Ref++;

			if (Ref > 1)
			{
				GroupIdx++;
				continue;
			}

			if (Ref != 0)
			{
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = mPieces[PieceIdx];

					if (Piece->GetGroup() == Group)
					{
						Piece->SetGroup(Group->mGroup);
						break;
					}
				}

				for (int ParentIdx = 0; ParentIdx < mGroups.GetSize(); ParentIdx++)
				{
					if (mGroups[ParentIdx]->mGroup == Group)
					{
						mGroups[ParentIdx]->mGroup = Group->mGroup;
						break;
					}
				}
			}

			mGroups.RemoveIndex(GroupIdx);
			delete Group;
			Removed = true;
		}
	}
	while (Removed);
}

lcVector3 lcModel::LockVector(const lcVector3& Vector) const
{
	lcVector3 NewVector(Vector);

	if (gMainWindow->GetLockX())
		NewVector[0] = 0;

	if (gMainWindow->GetLockY())
		NewVector[1] = 0;

	if (gMainWindow->GetLockZ())
		NewVector[2] = 0;

	return NewVector;
}

lcVector3 lcModel::SnapPosition(const lcVector3& Distance) const
{
	lcVector3 NewDistance(Distance);

	if (gMainWindow->GetMoveXYSnap())
	{
		float SnapXY = (float)gMainWindow->GetMoveXYSnap();
		int i = (int)(NewDistance[0] / SnapXY);
		float Leftover = NewDistance[0] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
		{
			Leftover -= SnapXY;
			i++;
		}
		else if (Leftover < -SnapXY / 2)
		{
			Leftover += SnapXY;
			i--;
		}

		NewDistance[0] = SnapXY * i;

		i = (int)(NewDistance[1] / SnapXY);
		Leftover = NewDistance[1] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
		{
			Leftover -= SnapXY;
			i++;
		}
		else if (Leftover < -SnapXY / 2)
		{
			Leftover += SnapXY;
			i--;
		}

		NewDistance[1] = SnapXY * i;
	}

	if (gMainWindow->GetMoveZSnap())
	{
		float SnapZ = (float)gMainWindow->GetMoveZSnap();
		int i = (int)(NewDistance[2] / SnapZ);
		float Leftover = NewDistance[2] - (SnapZ * i);

		if (Leftover > SnapZ / 2)
		{
			Leftover -= SnapZ;
			i++;
		}
		else if (Leftover < -SnapZ / 2)
		{
			Leftover += SnapZ;
			i--;
		}

		NewDistance[2] = SnapZ * i;
	}

	return NewDistance;
}

lcVector3 lcModel::SnapRotation(const lcVector3& Angles) const
{
	int AngleSnap = gMainWindow->GetAngleSnap();
	lcVector3 NewAngles(Angles);

	if (AngleSnap)
	{
		int Snap[3];

		for (int i = 0; i < 3; i++)
			Snap[i] = (int)(Angles[i] / (float)AngleSnap);

		NewAngles = lcVector3((float)(AngleSnap * Snap[0]), (float)(AngleSnap * Snap[1]), (float)(AngleSnap * Snap[2]));
	}

	return NewAngles;
}

lcMatrix44 lcModel::GetRelativeRotation() const
{
	const lcPreferences& Preferences = lcGetPreferences();

	if (!Preferences.mForceGlobalTransforms)
	{
		lcObject* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPiece())
		{
			lcMatrix44 WorldMatrix = ((lcPiece*)Focus)->mModelWorld;
			WorldMatrix.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));
			return WorldMatrix;
		}
	}

	return lcMatrix44Identity();
}

bool lcModel::RemoveSelectedObjects()
{
	bool RemovedPiece = false;
	bool RemovedCamera = false;
	bool RemovedLight = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			RemovedPiece = true;
			mPieces.Remove(Piece);
			delete Piece;
		}
		else
			PieceIdx++;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); )
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
			{
				View* View = Views[ViewIdx];

				if (Camera == View->mCamera)
					View->SetCamera(Camera, true);
			}

			RemovedCamera = true;
			mCameras.RemoveIndex(CameraIdx);
			delete Camera;
		}
		else
			CameraIdx++;
	}

	if (RemovedCamera)
		gMainWindow->UpdateCameraMenu();

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); )
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			RemovedLight = true;
			mLights.RemoveIndex(LightIdx);
			delete Light;
		}
		else
			LightIdx++;
	}

	RemoveEmptyGroups();

	return RemovedPiece || RemovedCamera || RemovedLight;
}

bool lcModel::MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance)
{
	lcMatrix44 RelativeRotation = GetRelativeRotation();
	bool Moved = false;

	if (PieceDistance.LengthSquared() >= 0.001f)
	{
		lcVector3 TransformedPieceDistance = lcMul30(PieceDistance, RelativeRotation);

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->IsSelected())
			{
				Piece->Move(mCurrentStep, gMainWindow->GetAddKeys(), TransformedPieceDistance);
				Piece->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}
	}

	if (ObjectDistance.LengthSquared() >= 0.001f)
	{
		lcVector3 TransformedObjectDistance = lcMul30(ObjectDistance, RelativeRotation);

		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		{
			lcCamera* Camera = mCameras[CameraIdx];

			if (Camera->IsSelected())
			{
				Camera->Move(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance);
				Camera->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		{
			lcLight* Light = mLights[LightIdx];

			if (Light->IsSelected())
			{
				Light->Move(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance);
				Light->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}
	}

	return Moved;
}

bool lcModel::RotateSelectedPieces(const lcVector3& Angles)
{
	if (Angles.LengthSquared() < 0.001f)
		return false;

	float Bounds[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	lcPiece* Focus = NULL;
	int SelectedCount = 0;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			if (Piece->IsFocused())
				Focus = Piece;

			Piece->CompareBoundingBox(Bounds);
			SelectedCount++;
		}
	}

	lcVector3 Center;

	if (Focus)
		Center = Focus->mPosition;
	else
		Center = lcVector3((Bounds[0] + Bounds[3]) / 2.0f, (Bounds[1] + Bounds[4]) / 2.0f, (Bounds[2] + Bounds[5]) / 2.0f);

	lcVector4 RotationQuaternion(0, 0, 0, 1);
	lcVector4 WorldToFocusQuaternion, FocusToWorldQuaternion;

	if (Angles[0] != 0.0f)
	{
		lcVector4 q = lcQuaternionRotationX(Angles[0] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (Angles[1] != 0.0f)
	{
		lcVector4 q = lcQuaternionRotationY(Angles[1] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (Angles[2] != 0.0f)
	{
		lcVector4 q = lcQuaternionRotationZ(Angles[2] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mForceGlobalTransforms)
		Focus = NULL;

	if (Focus)
	{
		const lcVector4& Rotation = Focus->mRotation;

		WorldToFocusQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rotation[0], Rotation[1], Rotation[2], -Rotation[3] * LC_DTOR));
		FocusToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rotation[0], Rotation[1], Rotation[2], Rotation[3] * LC_DTOR));

		RotationQuaternion = lcQuaternionMultiply(FocusToWorldQuaternion, RotationQuaternion);
	}

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected())
			continue;

		const lcVector4& Rotation = Piece->mRotation;
		lcVector3 Distance = Piece->mPosition - Center;
		lcVector4 LocalToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rotation[0], Rotation[1], Rotation[2], Rotation[3] * LC_DTOR));
		lcVector4 NewRotation, NewLocalToWorldQuaternion;

		if (Focus)
		{
			lcVector4 LocalToFocusQuaternion = lcQuaternionMultiply(WorldToFocusQuaternion, LocalToWorldQuaternion);
			NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToFocusQuaternion);

			lcVector4 WorldToLocalQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rotation[0], Rotation[1], Rotation[2], -Rotation[3] * LC_DTOR));

			Distance = lcQuaternionMul(Distance, WorldToLocalQuaternion);
			Distance = lcQuaternionMul(Distance, NewLocalToWorldQuaternion);
		}
		else
		{
			NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToWorldQuaternion);

			Distance = lcQuaternionMul(Distance, RotationQuaternion);
		}

		NewRotation = lcQuaternionToAxisAngle(NewLocalToWorldQuaternion);
		NewRotation[3] *= LC_RTOD;

		Piece->SetPosition(Center + Distance, mCurrentStep, gMainWindow->GetAddKeys());
		Piece->SetRotation(NewRotation, mCurrentStep, gMainWindow->GetAddKeys());
		Piece->UpdatePosition(mCurrentStep);
	}

	return true;
}

bool lcModel::AnyPiecesSelected() const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		if (mPieces[PieceIdx]->IsSelected())
			return true;

	return false;
}

bool lcModel::AnyObjectsSelected() const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		if (mPieces[PieceIdx]->IsSelected())
			return true;

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		if (mCameras[CameraIdx]->IsSelected())
			return true;

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsSelected())
			return true;

	return false;
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

lcVector3 lcModel::GetFocusOrSelectionCenter() const
{
	lcVector3 Center;

	if (GetFocusPosition(Center))
		return Center;

	GetSelectionCenter(Center);

	return Center;
}

bool lcModel::GetFocusPosition(lcVector3& Position) const
{
	lcObject* FocusObject = GetFocusObject();

	if (FocusObject)
	{
		Position = FocusObject->GetSectionPosition(FocusObject->GetFocusSection());
		return true;
	}
	else
	{
		Position = lcVector3(0.0f, 0.0f, 0.0f);
		return false;
	}
}

bool lcModel::GetSelectionCenter(lcVector3& Center) const
{
	float Bounds[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	bool Selected = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Bounds);
			Selected = true;
		}
	}

	Center = lcVector3((Bounds[0] + Bounds[3]) * 0.5f, (Bounds[1] + Bounds[4]) * 0.5f, (Bounds[2] + Bounds[5]) * 0.5f);

	return Selected;
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

void lcModel::FindPiece(bool FindFirst, bool SearchForward)
{
	if (mPieces.IsEmpty())
		return;

	int StartIdx = mPieces.GetSize() - 1;
	if (!FindFirst)
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->IsFocused() && Piece->IsVisible(mCurrentStep))
			{
				StartIdx = PieceIdx;
				break;
			}
		}
	}

	int CurrentIdx = StartIdx;
	lcObject* Focus = NULL;
	const lcSearchOptions& SearchOptions = gMainWindow->mSearchOptions;

	for (;;)
	{
		if (SearchForward)
			CurrentIdx++;
		else
			CurrentIdx--;

		if (CurrentIdx < 0)
			CurrentIdx = mPieces.GetSize() - 1;
		else if (CurrentIdx >= mPieces.GetSize())
			CurrentIdx = 0;

		if (CurrentIdx == StartIdx)
			break;

		lcPiece* Current = mPieces[CurrentIdx];

		if (!Current->IsVisible(mCurrentStep))
			continue;

		if ((!SearchOptions.MatchInfo || Current->mPieceInfo == SearchOptions.Info) &&
			(!SearchOptions.MatchColor || Current->mColorIndex == SearchOptions.ColorIndex) &&
			(!SearchOptions.MatchName || strcasestr(Current->GetName(), SearchOptions.Name)))
		{
			Focus = Current;
			break;
		}
	}

	ClearSelectionAndSetFocus(Focus, LC_PIECE_SECTION_POSITION);
}

void lcModel::BeginMouseTool()
{
	mMouseToolDistance = lcVector3(0.0f, 0.0f, 0.0f);
}

void lcModel::EndMouseTool(lcTool Tool, bool Accept)
{
	if (!Accept)
	{
		LoadCheckPoint(mUndoHistory[0]);
		return;
	}

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
		SaveCheckpoint(tr("New SpotLight"));
		break;

	case LC_TOOL_CAMERA:
		gMainWindow->UpdateCameraMenu();
		SaveCheckpoint(tr("New Camera"));
		break;

	case LC_TOOL_SELECT:
		break;

	case LC_TOOL_MOVE:
		SaveCheckpoint(tr("Move"));
		break;

	case LC_TOOL_ROTATE:
		SaveCheckpoint(tr("Rotate"));
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			SaveCheckpoint(tr("Zoom"));
		break;

	case LC_TOOL_PAN:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			SaveCheckpoint(tr("Pan"));
		break;

	case LC_TOOL_ROTATE_VIEW:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			SaveCheckpoint(tr("Orbit"));
		break;

	case LC_TOOL_ROLL:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			SaveCheckpoint(tr("Roll"));
		break;

	case LC_TOOL_ZOOM_REGION:
		break;
	}
}

void lcModel::InsertPieceToolClicked(const lcVector3& Position, const lcVector4& Rotation)
{
	lcPiece* Piece = new lcPiece(gMainWindow->mPreviewWidget->GetCurrentPiece());
	Piece->Initialize(Position, Rotation, mCurrentStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	Piece->UpdatePosition(mCurrentStep);
	Piece->CreateName(mPieces);
	mPieces.Add(Piece);

	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION);

	SaveCheckpoint(tr("Insert"));
}

void lcModel::PointLightToolClicked(const lcVector3& Position)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2]);
	Light->CreateName(mLights);
	mLights.Add(Light);

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_POSITION);
	SaveCheckpoint(tr("New Light"));
}

void lcModel::BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	mLights.Add(Light);

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_TARGET);
}

void lcModel::UpdateSpotLightTool(const lcVector3& Target)
{
	lcLight* Light = mLights[mLights.GetSize() - 1];

	Light->Move(1, false, Target);
	Light->UpdatePosition(1);

	gMainWindow->UpdateFocusObject(Light);
	gMainWindow->UpdateAllViews();
}

void lcModel::BeginCameraTool(const lcVector3& Position, const lcVector3& Target)
{
	lcCamera* Camera = new lcCamera(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	Camera->CreateName(mCameras);
	mCameras.Add(Camera);

	ClearSelectionAndSetFocus(Camera, LC_CAMERA_SECTION_TARGET);
}

void lcModel::UpdateCameraTool(const lcVector3& Target)
{
	lcCamera* Camera = mCameras[mCameras.GetSize() - 1];

	Camera->Move(1, false, Target);
	Camera->UpdatePosition(1);

	gMainWindow->UpdateFocusObject(Camera);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateMoveTool(const lcVector3& Distance)
{
	lcVector3 PieceDistance = LockVector(SnapPosition(Distance) - SnapPosition(mMouseToolDistance));
	lcVector3 ObjectDistance = Distance - mMouseToolDistance;

	MoveSelectedObjects(PieceDistance, ObjectDistance);
	mMouseToolDistance = Distance;

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateRotateTool(const lcVector3& Angles)
{
	lcVector3 Delta = LockVector(SnapRotation(Angles) - SnapRotation(mMouseToolDistance));
	RotateSelectedPieces(Delta);
	mMouseToolDistance = Angles;

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void lcModel::EraserToolClicked(lcObject* Object)
{
	if (!Object)
		return;

	switch (Object->GetType())
	{
	case LC_OBJECT_PIECE:
		mPieces.Remove((lcPiece*)Object);
		RemoveEmptyGroups();
		break;

	case LC_OBJECT_CAMERA:
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
			{
				View* View = Views[ViewIdx];
				lcCamera* Camera = View->mCamera;

				if (Camera == Object)
					View->SetCamera(Camera, true);
			}

			mCameras.Remove((lcCamera*)Object);

			gMainWindow->UpdateCameraMenu();
		}
		break;

	case LC_OBJECT_LIGHT:
		mLights.Remove((lcLight*)Object);
		break;
	}

	delete Object;
	gMainWindow->UpdateFocusObject(GetFocusObject());
	UpdateSelection();
	gMainWindow->UpdateAllViews();
	SaveCheckpoint(tr("Deleting"));
}

void lcModel::PaintToolClicked(lcObject* Object)
{
	if (!Object || Object->GetType() != LC_OBJECT_PIECE)
		return;

	lcPiece* Piece = (lcPiece*)Object;

	if (Piece->mColorIndex != gMainWindow->mColorIndex)
	{
		Piece->SetColorIndex(gMainWindow->mColorIndex);

		SaveCheckpoint(tr("Painting"));
		gMainWindow->UpdateFocusObject(GetFocusObject());
		gMainWindow->UpdateAllViews();
	}
}

void lcModel::UpdateZoomTool(lcCamera* Camera, float Mouse)
{
	Camera->Zoom(Mouse - mMouseToolDistance.x, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdatePanTool(lcCamera* Camera, float MouseX, float MouseY)
{
	Camera->Pan(MouseX - mMouseToolDistance.x, MouseY - mMouseToolDistance.y, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = MouseX;
	mMouseToolDistance.y = MouseY;
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY)
{
	lcVector3 Center;
	GetSelectionCenter(Center);
	Camera->Orbit(MouseX - mMouseToolDistance.x, MouseY - mMouseToolDistance.y, Center, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = MouseX;
	mMouseToolDistance.y = MouseY;
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateRollTool(lcCamera* Camera, float Mouse)
{
	Camera->Roll(Mouse - mMouseToolDistance.x, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;
	gMainWindow->UpdateAllViews();
}

void lcModel::ZoomRegionToolClicked(lcCamera* Camera, const lcVector3* Points, float RatioX, float RatioY)
{
	Camera->ZoomRegion(Points, RatioX, RatioY, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Zoom"));
}
