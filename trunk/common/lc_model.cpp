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
#include "pieceinf.h"
#include "view.h"
#include "preview.h"
#include "minifig.h"

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

lcModel::lcModel(const QString& Name)
{
	mProperties.mName = Name;
	mProperties.LoadDefaults();

	mCurrentStep = 1;
	mBackgroundTexture = NULL;
	mPieceInfo = NULL;
}

lcModel::~lcModel()
{
	DeleteModel();
	DeleteHistory();
}

bool lcModel::IncludesModel(const lcModel* Model) const
{
	if (Model == this)
		return true;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		if (mPieces[PieceIdx]->mPieceInfo->IncludesModel(Model))
			return true;

	return false;
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

	if (gMainWindow)
	{
		const lcArray<View*>& Views = gMainWindow->GetViews();

		for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		{
			View* View = Views[ViewIdx];
			lcCamera* Camera = View->mCamera;

			if (!Camera->IsSimple() && mCameras.FindIndex(Camera) != -1)
				View->SetCamera(Camera, true);
		}
	}

	mPieces.DeleteAll();
	mCameras.DeleteAll();
	mLights.DeleteAll();
	mGroups.DeleteAll();
}

void lcModel::CreatePieceInfo()
{
	QString PartID = mProperties.mName.toUpper();
	mPieceInfo = lcGetPiecesLibrary()->FindPiece(PartID.toLatin1().constData(), true);
	mPieceInfo->SetModel(this);
}

void lcModel::SaveLDraw(QTextStream& Stream, bool SelectedOnly) const
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

			if (Piece->GetStepShow() != Step || (SelectedOnly && !Piece->IsSelected()))
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
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (!SelectedOnly || Camera->IsSelected())
			Camera->SaveLDraw(Stream);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (!SelectedOnly || Light->IsSelected())
			Light->SaveLDraw(Stream);
	}

	Stream.flush();
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
		qint64 Pos = Stream.pos();
		QString Line = Stream.readLine().trimmed();
		QTextStream LineStream(&Line, QIODevice::ReadOnly);

		QString Token;
		LineStream >> Token;

		if (Token == QLatin1String("0"))
		{
			LineStream >> Token;

			if (Token == QLatin1String("FILE"))
			{
				if (!mProperties.mName.isEmpty())
				{
					Stream.seek(Pos);
					break;
				}

				mProperties.mName = LineStream.readAll().trimmed();
				continue;
			}
			else if (Token == QLatin1String("NOFILE"))
			{
				break;
			}
			else if (Token == QLatin1String("STEP"))
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

	CalculateStep(mCurrentStep);
	UpdateBackgroundTexture();

	delete Piece;
	delete Camera;
	delete Light;
}

bool lcModel::LoadBinary(lcFile* file)
{
	lcint32 i, count;
	char id[32];
	lcuint32 rgb;
	float fv = 0.4f;
	lcuint8 ch;
	lcuint16 sh;

	file->Seek(0, SEEK_SET);
	file->ReadBuffer(id, 32);
	sscanf(&id[7], "%f", &fv);

	if (fv == 0.0f)
	{
		lconv *loc = localeconv();
		id[8] = loc->decimal_point[0];
		sscanf(&id[7], "%f", &fv);

		if (fv == 0.0f)
			return false;
	}

	if (fv > 0.4f)
		file->ReadFloats(&fv, 1);

	file->ReadU32(&rgb, 1);
	mProperties.mBackgroundSolidColor[0] = (float)((unsigned char) (rgb))/255;
	mProperties.mBackgroundSolidColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	mProperties.mBackgroundSolidColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

	if (fv < 0.6f) // old view
	{
		double eye[3], target[3];
		file->ReadDoubles(eye, 3);
		file->ReadDoubles(target, 3);
	}

	file->Seek(28, SEEK_CUR);
	file->ReadS32(&i, 1);
	mCurrentStep = i;

	if (fv > 0.8f)
		file->ReadU32();//m_nScene

	file->ReadS32(&count, 1);
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	Library->OpenCache();

	int FirstNewPiece = mPieces.GetSize();

	while (count--)
	{
		if (fv > 0.4f)
		{
			lcPiece* pPiece = new lcPiece(NULL);
			pPiece->FileLoad(*file);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				if (strcmp(mPieces[PieceIdx]->GetName(), pPiece->GetName()) == 0)
				{
					pPiece->CreateName(mPieces);
					break;
				}
			}

			if (strlen(pPiece->GetName()) == 0)
				pPiece->CreateName(mPieces);

			mPieces.Add(pPiece);
		}
		else
		{
			char name[LC_PIECE_NAME_LEN];
			lcVector3 pos, rot;
			lcuint8 color, step, group;

			file->ReadFloats(pos, 3);
			file->ReadFloats(rot, 3);
			file->ReadU8(&color, 1);
			file->ReadBuffer(name, 9);
			file->ReadU8(&step, 1);
			file->ReadU8(&group, 1);

			pos *= 25.0f;
			lcMatrix44 ModelWorld = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			PieceInfo* pInfo = Library->FindPiece(name, true);
			lcPiece* pPiece = new lcPiece(pInfo);

			pPiece->Initialize(pos, AxisAngle, step);
			pPiece->SetColorCode(lcGetColorCodeFromOriginalColor(color));
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);

//			pPiece->SetGroup((lcGroup*)group);
		}
	}

	Library->CloseCache();

	if (fv >= 0.4f)
	{
		file->ReadBuffer(&ch, 1);
		if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
		if (sh > 100)
			file->Seek(sh, SEEK_CUR);
		else
		{
			String Author;
			file->ReadBuffer(Author.GetBuffer(sh), sh);
			Author.Buffer()[sh] = 0;
			mProperties.mAuthor = QString::fromUtf8(Author.Buffer());
		}

		file->ReadBuffer(&ch, 1);
		if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
		if (sh > 100)
			file->Seek(sh, SEEK_CUR);
		else
		{
			String Description;
			file->ReadBuffer(Description.GetBuffer(sh), sh);
			Description.Buffer()[sh] = 0;
			mProperties.mDescription = QString::fromUtf8(Description.Buffer());
		}

		file->ReadBuffer(&ch, 1);
		if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
		if (sh > 255)
			file->Seek(sh, SEEK_CUR);
		else
		{
			String Comments;
			file->ReadBuffer(Comments.GetBuffer(sh), sh);
			Comments.Buffer()[sh] = 0;
			mProperties.mComments = QString::fromUtf8(Comments.Buffer());
			mProperties.mComments.replace(QLatin1String("\r\n"), QLatin1String("\n"));
		}
	}

	if (fv >= 0.5f)
	{
		int NumGroups = mGroups.GetSize();

		file->ReadS32(&count, 1);
		for (i = 0; i < count; i++)
			mGroups.Add(new lcGroup());

		for (int GroupIdx = NumGroups; GroupIdx < mGroups.GetSize(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx];

			if (fv < 1.0f)
			{
				file->ReadBuffer(Group->m_strName, 65);
				file->ReadBuffer(&ch, 1);
				Group->mGroup = (lcGroup*)-1;
			}
			else
				Group->FileLoad(file);
		}

		for (int GroupIdx = NumGroups; GroupIdx < mGroups.GetSize(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx];

			i = LC_POINTER_TO_INT(Group->mGroup);
			Group->mGroup = NULL;

			if (i > 0xFFFF || i == -1)
				continue;

			Group->mGroup = mGroups[NumGroups + i];
		}

		for (int PieceIdx = FirstNewPiece; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			i = LC_POINTER_TO_INT(Piece->GetGroup());
			Piece->SetGroup(NULL);

			if (i > 0xFFFF || i == -1)
				continue;

			Piece->SetGroup(mGroups[NumGroups + i]);
		}

		RemoveEmptyGroups();
	}

	if (fv >= 0.6f)
	{
		if (fv < 1.0f)
			file->Seek(4, SEEK_CUR);
		else
			file->Seek(2, SEEK_CUR);

		file->ReadS32(&count, 1);
		for (i = 0; i < count; i++)
			mCameras.Add(new lcCamera(false));

		if (count < 7)
		{
			lcCamera* pCam = new lcCamera(false);
			for (i = 0; i < count; i++)
				pCam->FileLoad(*file);
			delete pCam;
		}
		else
		{
			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->FileLoad(*file);
		}
	}

	if (fv >= 0.7f)
	{
		file->Seek(16, SEEK_CUR);

		file->ReadU32(&rgb, 1);
		mProperties.mFogColor[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

		if (fv < 1.0f)
		{
			file->ReadU32(&rgb, 1);
			mProperties.mFogDensity = (float)rgb/100;
		}
		else
			file->ReadFloats(&mProperties.mFogDensity, 1);

		if (fv < 1.3f)
		{
			file->ReadU8(&ch, 1);
			if (ch == 0xFF)
				file->ReadU16(&sh, 1);
			sh = ch;
		}
		else
			file->ReadU16(&sh, 1);

		if (sh < LC_MAXPATH)
		{
			char Background[LC_MAXPATH];
			file->ReadBuffer(Background, sh);
			mProperties.mBackgroundImage = Background;
		}
		else
			file->Seek(sh, SEEK_CUR);
	}

	if (fv >= 0.8f)
	{
		file->ReadBuffer(&ch, 1);
		file->Seek(ch, SEEK_CUR);
		file->ReadBuffer(&ch, 1);
		file->Seek(ch, SEEK_CUR);
	}

	if (fv > 0.9f)
	{
		file->ReadU32(&rgb, 1);
		mProperties.mAmbientColor[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mAmbientColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mAmbientColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

		if (fv < 1.3f)
			file->Seek(23, SEEK_CUR);
		else
			file->Seek(11, SEEK_CUR);
	}

	if (fv > 1.0f)
	{
		file->ReadU32(&rgb, 1);
		mProperties.mBackgroundGradientColor1[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mBackgroundGradientColor1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mBackgroundGradientColor1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
		file->ReadU32(&rgb, 1);
		mProperties.mBackgroundGradientColor2[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mBackgroundGradientColor2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mBackgroundGradientColor2[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	}

	UpdateBackgroundTexture();
	CalculateStep(mCurrentStep);

	return true;
}

void lcModel::Merge(lcModel* Other)
{
	for (int PieceIdx = 0; PieceIdx < Other->mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Other->mPieces[PieceIdx];
		Piece->CreateName(mPieces);
		mPieces.Add(Piece);
	}

	Other->mPieces.RemoveAll();

	for (int CameraIdx = 0; CameraIdx < Other->mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = Other->mCameras[CameraIdx];
		Camera->CreateName(mCameras);
		mCameras.Add(Camera);
	}

	Other->mCameras.RemoveAll();

	for (int LightIdx = 0; LightIdx < Other->mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = Other->mLights[LightIdx];
		Light->CreateName(mLights);
		mLights.Add(Light);
	}

	Other->mLights.RemoveAll();

	for (int GroupIdx = 0; GroupIdx < Other->mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = Other->mGroups[GroupIdx];
		Group->CreateName(mGroups);
		mGroups.Add(Group);
	}

	Other->mGroups.RemoveAll();

	delete Other;
}

void lcModel::Cut()
{
	Copy();

	if (RemoveSelectedObjects())
	{
		gMainWindow->UpdateFocusObject(NULL);
		UpdateSelection();
		gMainWindow->UpdateAllViews();
		SaveCheckpoint("Cutting");
	}
}

void lcModel::Copy()
{
	QByteArray File;
	QTextStream Stream(&File, QIODevice::WriteOnly);

	SaveLDraw(Stream, true);

	g_App->ExportClipboard(File);
}

void lcModel::Paste()
{
	if (g_App->mClipboard.isEmpty())
		return;

	lcModel* Model = new lcModel(QString());

	QTextStream Stream(&g_App->mClipboard);
	Model->LoadLDraw(Stream);

	Merge(Model);
	SaveCheckpoint(tr("Pasting"));

	CalculateStep(mCurrentStep);
	gMainWindow->UpdateAllViews();
}

void lcModel::GetScene(lcScene& Scene, lcCamera* ViewCamera, bool DrawInterface) const
{
	Scene.ViewMatrix = ViewCamera->mWorldView;
	Scene.OpaqueMeshes.RemoveAll();
	Scene.OpaqueMeshes.AllocGrow(mPieces.GetSize());
	Scene.TranslucentMeshes.RemoveAll();
	Scene.TranslucentMeshes.AllocGrow(mPieces.GetSize());
	Scene.InterfaceObjects.RemoveAll();

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsVisible(mCurrentStep))
			continue;

		PieceInfo* Info = Piece->mPieceInfo;
		bool Focused, Selected;

		if (DrawInterface)
		{
			Focused = Piece->IsFocused();
			Selected = Piece->IsSelected();
		}
		else
		{
			Focused = false;
			Selected = false;
		}

		Info->AddRenderMeshes(Scene, Piece->mModelWorld, Piece->mColorIndex, Focused, Selected);

		if (Selected)
			Scene.InterfaceObjects.Add(Piece);
	}

	if (DrawInterface)
	{
		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		{
			lcCamera* Camera = mCameras[CameraIdx];

			if (Camera != ViewCamera && Camera->IsVisible())
				Scene.InterfaceObjects.Add(Camera);
		}

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		{
			lcLight* Light = mLights[LightIdx];

			if (Light->IsVisible())
				Scene.InterfaceObjects.Add(Light);
		}
	}

	Scene.OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	Scene.TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);
}

void lcModel::SubModelAddRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, bool Focused, bool Selected) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->GetStepHide() != LC_STEP_MAX)
			continue;

		int ColorIndex = Piece->mColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		PieceInfo* Info = Piece->mPieceInfo;
		Info->AddRenderMeshes(Scene, lcMul(Piece->mModelWorld, WorldMatrix), ColorIndex, Focused, Selected);
	}
}

void lcModel::DrawBackground(lcContext* Context)
{
	if (mProperties.mBackgroundType == LC_BACKGROUND_SOLID)
	{
		glClearColor(mProperties.mBackgroundSolidColor[0], mProperties.mBackgroundSolidColor[1], mProperties.mBackgroundSolidColor[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	float ViewWidth = (float)Context->GetViewportWidth();
	float ViewHeight = (float)Context->GetViewportHeight();

	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));
	Context->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

	if (mProperties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		glShadeModel(GL_SMOOTH);

		const lcVector3& Color1 = mProperties.mBackgroundGradientColor1;
		const lcVector3& Color2 = mProperties.mBackgroundGradientColor2;

		float Verts[] =
		{
			ViewWidth, ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      0.0f,       Color2[0], Color2[1], Color2[2], 1.0f,
			ViewWidth, 0.0f,       Color2[0], Color2[1], Color2[2], 1.0f
		};

		glVertexPointer(2, GL_FLOAT, 6 * sizeof(float), Verts);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 6 * sizeof(float), Verts + 2);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_COLOR_ARRAY);

		glShadeModel(GL_FLAT);
	}

	if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, mBackgroundTexture->mTexture);

		float TileWidth = 1.0f, TileHeight = 1.0f;

		if (mProperties.mBackgroundImageTile)
		{
			TileWidth = ViewWidth / mBackgroundTexture->mWidth;
			TileHeight = ViewHeight / mBackgroundTexture->mHeight;
		}

		float Verts[] =
		{
			0.0f,      ViewHeight, 0.0f,      0.0f,
			ViewWidth, ViewHeight, TileWidth, 0.0f,
			ViewWidth, 0.0f,       TileWidth, TileHeight,
			0.0f,      0.0f,       0.0f,      TileHeight
		};

		glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), Verts + 2);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
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

bool lcModel::SubModelMinIntersectDist(const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance) const
{
	bool MinIntersect = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->GetStepHide() == LC_STEP_MAX && Piece->mPieceInfo->MinIntersectDist(Piece->mModelWorld, WorldStart, WorldEnd, MinDistance))
			MinIntersect = true;
	}

	return MinIntersect;
}

bool lcModel::SubModelBoxTest(const lcVector4 Planes[6]) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->GetStepHide() != LC_STEP_MAX)
			continue;

		if (Piece->mPieceInfo->BoxTest(Piece->mModelWorld, Planes))
			return true;
	}

	return false;
}

void lcModel::SaveCheckpoint(const QString& Description)
{
	lcModelHistoryEntry* ModelHistoryEntry = new lcModelHistoryEntry();

	ModelHistoryEntry->Description = Description;

	QTextStream Stream(&ModelHistoryEntry->File);
	SaveLDraw(Stream, false);

	mUndoHistory.InsertAt(0, ModelHistoryEntry);
	mRedoHistory.DeleteAll();

	if (!Description.isEmpty())
	{
		gMainWindow->UpdateModified(IsModified());
		gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : QString(), !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : QString());
	}
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

void lcModel::SetActive(bool Active)
{
	if (Active)
	{
		CalculateStep(mCurrentStep);
	}
	else
	{
		CalculateStep(LC_STEP_MAX);

		float BoundingBox[6];
		GetPiecesBoundingBox(BoundingBox, true);

		mPieceInfo->m_fDimensions[0] = BoundingBox[3];
		mPieceInfo->m_fDimensions[1] = BoundingBox[4];
		mPieceInfo->m_fDimensions[2] = BoundingBox[5];
		mPieceInfo->m_fDimensions[3] = BoundingBox[0];
		mPieceInfo->m_fDimensions[4] = BoundingBox[1];
		mPieceInfo->m_fDimensions[5] = BoundingBox[2];

		strncpy(mPieceInfo->m_strName, mProperties.mName.toLatin1().constData(), sizeof(mPieceInfo->m_strName));
		mPieceInfo->m_strName[sizeof(mPieceInfo->m_strName) - 1] = 0;
		strncpy(mPieceInfo->m_strDescription, mProperties.mName.toLatin1().constData(), sizeof(mPieceInfo->m_strDescription));
		mPieceInfo->m_strDescription[sizeof(mPieceInfo->m_strDescription) - 1] = 0;
	}
}

void lcModel::CalculateStep(lcStep Step)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->UpdatePosition(Step);

		if (Piece->IsSelected())
		{
			if (!Piece->IsVisible(Step))
				Piece->SetSelected(false);
			else
				SelectGroup(Piece->GetTopGroup(), true);
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->UpdatePosition(Step);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->UpdatePosition(Step);
}

void lcModel::ShowFirstStep()
{
	if (mCurrentStep == 1)
		return;

	mCurrentStep = 1;

	CalculateStep(mCurrentStep);
	UpdateSelection();
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCurrentStep();
}

void lcModel::ShowLastStep()
{
	lcStep LastStep = GetLastStep();

	if (mCurrentStep == LastStep)
		return;

	mCurrentStep = LastStep;

	CalculateStep(mCurrentStep);
	UpdateSelection();
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCurrentStep();
}

void lcModel::ShowPreviousStep()
{
	if (mCurrentStep == 1)
		return;

	mCurrentStep--;

	CalculateStep(mCurrentStep);
	UpdateSelection();
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCurrentStep();
}

void lcModel::ShowNextStep()
{
	if (mCurrentStep == LC_STEP_MAX)
		return;

	mCurrentStep++;

	CalculateStep(mCurrentStep);
	UpdateSelection();
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCurrentStep();
}

lcStep lcModel::GetLastStep() const
{
	lcStep Step = 1;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Step = lcMax(Step, mPieces[PieceIdx]->GetStepShow());

	return Step;
}

void lcModel::InsertStep()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->InsertTime(mCurrentStep, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->InsertTime(mCurrentStep, 1);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->InsertTime(mCurrentStep, 1);

	SaveCheckpoint(tr("Inserting Step"));
	CalculateStep(mCurrentStep);
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	UpdateSelection();
}

void lcModel::RemoveStep()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->RemoveTime(mCurrentStep, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->RemoveTime(mCurrentStep, 1);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->RemoveTime(mCurrentStep, 1);

	SaveCheckpoint(tr("Removing Step"));
	CalculateStep(mCurrentStep);
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
	UpdateSelection();
}

lcGroup* lcModel::AddGroup(const char* Prefix, lcGroup* Parent)
{
	lcGroup* Group = new lcGroup();
	mGroups.Add(Group);

	GetGroupName(Prefix, Group->m_strName);
	Group->mGroup = Parent;

	return Group;
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

void lcModel::RemoveGroup(lcGroup* Group)
{
	mGroups.Remove(Group);
	delete Group;
}

void lcModel::GroupSelection()
{
	if (!AnyPiecesSelected())
	{
		gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	char GroupName[LC_MAX_GROUP_NAME + 1];

	GetGroupName("Group #", GroupName);

	if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_GROUP, GroupName))
		return;

	lcGroup* NewGroup = GetGroup(GroupName, true);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcGroup* Group = Piece->GetTopGroup();

			if (!Group)
				Piece->SetGroup(NewGroup);
			else if (Group != NewGroup)
				Group->mGroup = NewGroup;
		}
	}

	SaveCheckpoint(tr("Grouping"));
}

void lcModel::UngroupSelection()
{
	lcArray<lcGroup*> SelectedGroups;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcGroup* Group = Piece->GetTopGroup();

			if (SelectedGroups.FindIndex(Group) == -1)
			{
				mGroups.Remove(Group);
				SelectedGroups.Add(Group);
			}
		}
	}

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		lcGroup* Group = Piece->GetGroup();

		if (SelectedGroups.FindIndex(Group) != -1)
			Piece->SetGroup(NULL);
	}

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];

		if (SelectedGroups.FindIndex(Group->mGroup) != -1)
			Group->mGroup = NULL;
	}

	SelectedGroups.DeleteAll();

	RemoveEmptyGroups();
	SaveCheckpoint(tr("Ungrouping"));
}

void lcModel::AddSelectedPiecesToGroup()
{
	lcGroup* Group = NULL;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Group = Piece->GetTopGroup();
			if (Group)
				break;
		}
	}

	if (Group)
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->IsFocused())
			{
				Piece->SetGroup(Group);
				break;
			}
		}
	}

	RemoveEmptyGroups();
	SaveCheckpoint(tr("Grouping"));
}

void lcModel::RemoveFocusPieceFromGroup()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
		{
			Piece->SetGroup(NULL);
			break;
		}
	}

	RemoveEmptyGroups();
	SaveCheckpoint(tr("Ungrouping"));
}

void lcModel::ShowEditGroupsDialog()
{
	lcEditGroupsDialogOptions Options;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Options.PieceParents[Piece] = Piece->GetGroup();
	}

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];
		Options.GroupParents[Group] = Group->mGroup;
	}

	if (!gMainWindow->DoDialog(LC_DIALOG_EDIT_GROUPS, &Options))
		return;

	bool Modified = Options.NewGroups.isEmpty();

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		lcGroup* ParentGroup = Options.PieceParents.value(Piece);

		if (ParentGroup != Piece->GetGroup())
		{
			mPieces[PieceIdx]->SetGroup(ParentGroup);
			Modified = true;
		}
	}

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];
		lcGroup* ParentGroup = Options.GroupParents.value(Group);

		if (ParentGroup != Group->mGroup)
		{
			Group->mGroup = ParentGroup;
			Modified = true;
		}
	}

	if (Modified)
	{
		ClearSelection(true);
		SaveCheckpoint(tr("Editing Groups"));
	}
}

void lcModel::GetGroupName(const char* Prefix, char* GroupName)
{
	int Length = strlen(Prefix);
	int Max = 0;

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];
		int GroupNumber;

		if (strncmp(Group->m_strName, Prefix, Length) == 0)
			if (sscanf(Group->m_strName + Length, "%d", &GroupNumber) == 1)
				if (GroupNumber > Max)
					Max = GroupNumber;
	}

	sprintf(GroupName, "%s%.2d", Prefix, Max + 1);
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

void lcModel::AddPiece()
{
	PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();

	if (!CurPiece)
		return;

	lcPiece* Last = mPieces.IsEmpty() ? NULL : mPieces[mPieces.GetSize() - 1];

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
		{
			Last = Piece;
			break;
		}
	}

	lcVector3 Position(0, 0, 0);
	lcVector4 Rotation(0, 0, 1, 0);

	if (Last != NULL)
	{
		lcVector3 Dist(0, 0, Last->mPieceInfo->m_fDimensions[2] - CurPiece->m_fDimensions[5]);
		Dist = SnapPosition(Dist);

		Position = lcMul31(Dist, Last->mModelWorld);
		Rotation = Last->mRotation;
	}
	else
	{
		Position[2] = -CurPiece->m_fDimensions[5];
	}

	lcPiece* Piece = new lcPiece(CurPiece);
	Piece->Initialize(Position, Rotation, mCurrentStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	Piece->CreateName(mPieces);
	mPieces.Add(Piece);
	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION);

	SaveCheckpoint("Adding Piece");
}

void lcModel::DeleteSelectedObjects()
{
	if (RemoveSelectedObjects())
	{
		gMainWindow->UpdateFocusObject(NULL);
		UpdateSelection();
		gMainWindow->UpdateAllViews();
		SaveCheckpoint("Deleting");
	}
}

void lcModel::ShowSelectedPiecesEarlier()
{
	bool Modified = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step > 1)
			{
				Step--;
				Modified = true;
				Piece->SetStepShow(Step);
			}
		}
	}

	if (Modified)
	{
		SaveCheckpoint("Modifying");
		gMainWindow->UpdateAllViews();
		UpdateSelection();
	}
}

void lcModel::ShowSelectedPiecesLater()
{
	bool Modified = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step < LC_STEP_MAX)
			{
				Step++;
				Modified = true;
				Piece->SetStepShow(Step);

				if (Step > mCurrentStep)
					Piece->SetSelected(false);
			}
		}
	}

	if (Modified)
	{
		SaveCheckpoint("Modifying");
		gMainWindow->UpdateAllViews();
		UpdateSelection();
	}
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
			const lcArray<View*>& Views = gMainWindow->GetViews();
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

void lcModel::MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool Update)
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

	if (Moved && Update)
	{
		gMainWindow->UpdateAllViews();
		SaveCheckpoint("Moving");
		gMainWindow->UpdateFocusObject(GetFocusObject());
	}
}

void lcModel::RotateSelectedPieces(const lcVector3& Angles, bool Update)
{
	if (Angles.LengthSquared() < 0.001f)
		return;

	float Bounds[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	lcPiece* Focus = NULL;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			if (Piece->IsFocused())
				Focus = Piece;

			Piece->CompareBoundingBox(Bounds);
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

	bool Rotated = false;

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
		Rotated = true;
	}

	if (Rotated && Update)
	{
		gMainWindow->UpdateAllViews();
		SaveCheckpoint("Rotating");
		gMainWindow->UpdateFocusObject(GetFocusObject());
	}
}

void lcModel::TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform)
{
	switch (TransformType)
	{
	case LC_TRANSFORM_ABSOLUTE_TRANSLATION:
		{
			lcVector3 Center = GetFocusOrSelectionCenter();
			lcVector3 Offset = Transform - Center;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					Piece->UpdatePosition(mCurrentStep);
				}
			}

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				lcCamera* Camera = mCameras[CameraIdx];

				if (Camera->IsSelected())
				{
					Camera->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					Camera->UpdatePosition(mCurrentStep);
				}
			}

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			{
				lcLight* Light = mLights[LightIdx];

				if (Light->IsSelected())
				{
					Light->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					Light->UpdatePosition(mCurrentStep);
				}
			}

			gMainWindow->UpdateAllViews();
			SaveCheckpoint("Moving");
			gMainWindow->UpdateFocusObject(GetFocusObject());
		}
		break;

	case LC_TRANSFORM_RELATIVE_TRANSLATION:
		MoveSelectedObjects(Transform, true);
		break;

	case LC_TRANSFORM_ABSOLUTE_ROTATION:
		{
			lcVector4 RotationQuaternion(0, 0, 0, 1);

			if (Transform[0] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationX(Transform[0] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[1] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationY(Transform[1] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[2] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationZ(Transform[2] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			lcVector4 NewRotation = lcQuaternionToAxisAngle(RotationQuaternion);
			NewRotation[3] *= LC_RTOD;

			bool Rotated = false;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->SetRotation(NewRotation, mCurrentStep, gMainWindow->GetAddKeys());
					Piece->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
			}

			if (Rotated)
			{
				gMainWindow->UpdateAllViews();
				SaveCheckpoint("Rotating");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		}
		break;

	case LC_TRANSFORM_RELATIVE_ROTATION:
		RotateSelectedPieces(Transform, true);
		break;
	}
}

void lcModel::SetObjectProperty(lcObject* Object, lcObjectPropertyType ObjectPropertyType, const void* Value)
{
	QString CheckPointString;

	switch (ObjectPropertyType)
	{
	case LC_PIECE_PROPERTY_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			lcPiece* Piece = (lcPiece*)Object;

			if (Piece->mPosition != Position)
			{
				Piece->SetPosition(Position, mCurrentStep, gMainWindow->GetAddKeys());
				Piece->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Moving");
			}
		} break;

	case LC_PIECE_PROPERTY_ROTATION:
		{
			const lcVector4& Rotation = *(lcVector4*)Value;
			lcPiece* Piece = (lcPiece*)Object;

			if (Rotation != Piece->mRotation)
			{
				Piece->SetRotation(Rotation, mCurrentStep, gMainWindow->GetAddKeys());
				Piece->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Rotating");
			}
		} break;

	case LC_PIECE_PROPERTY_SHOW:
		{
			lcStep Step = *(lcStep*)Value;
			lcPiece* Part = (lcPiece*)Object;

			if (Step != Part->GetStepShow())
			{
				Part->SetStepShow(Step);
				if (Part->IsSelected() && !Part->IsVisible(mCurrentStep))
					Part->SetSelected(false);

				CheckPointString = tr("Showing");
			}
		} break;

	case LC_PIECE_PROPERTY_HIDE:
		{
			lcStep Step = *(lcuint32*)Value;
			lcPiece* Part = (lcPiece*)Object;

			if (Step != Part->GetStepHide())
			{
				Part->SetStepHide(Step);

				CheckPointString = tr("Hiding");
			}
		} break;

	case LC_PIECE_PROPERTY_COLOR:
		{
			int ColorIndex = *(int*)Value;
			lcPiece* Part = (lcPiece*)Object;

			if (ColorIndex != Part->mColorIndex)
			{
				Part->SetColorIndex(ColorIndex);

				CheckPointString = tr("Setting Color");
			}
		} break;

	case LC_PIECE_PROPERTY_ID:
		{
			lcPiece* Part = (lcPiece*)Object;
			PieceInfo* Info = (PieceInfo*)Value;

			if (Info != Part->mPieceInfo)
			{
				Part->mPieceInfo->Release();
				Part->mPieceInfo = Info;
				Part->mPieceInfo->AddRef();

				CheckPointString = tr("Setting Part");
			}
		} break;

	case LC_CAMERA_PROPERTY_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->mPosition != Position)
			{
				Camera->SetPosition(Position, mCurrentStep, gMainWindow->GetAddKeys());
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Moving Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_TARGET:
		{
			const lcVector3& TargetPosition = *(lcVector3*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->mTargetPosition != TargetPosition)
			{
				Camera->SetTargetPosition(TargetPosition, mCurrentStep, gMainWindow->GetAddKeys());
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Moving Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_UPVECTOR:
		{
			const lcVector3& Up = *(lcVector3*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->mUpVector != Up)
			{
				Camera->SetUpVector(Up, mCurrentStep, gMainWindow->GetAddKeys());
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Rotating Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_ORTHO:
		{
			bool Ortho = *(bool*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->IsOrtho() != Ortho)
			{
				Camera->SetOrtho(Ortho);
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Changing Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_FOV:
		{
			float FOV = *(float*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->m_fovy != FOV)
			{
				Camera->m_fovy = FOV;
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Setting FOV");
			}
		} break;

	case LC_CAMERA_PROPERTY_NEAR:
		{
			float Near = *(float*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->m_zNear != Near)
			{
				Camera->m_zNear= Near;
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Setting Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_FAR:
		{
			float Far = *(float*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (Camera->m_zFar != Far)
			{
				Camera->m_zFar = Far;
				Camera->UpdatePosition(mCurrentStep);

				CheckPointString = tr("Setting Camera");
			}
		} break;

	case LC_CAMERA_PROPERTY_NAME:
		{
			const char* Name = (const char*)Value;
			lcCamera* Camera = (lcCamera*)Object;

			if (strcmp(Camera->m_strName, Name))
			{
				strncpy(Camera->m_strName, Name, sizeof(Camera->m_strName));
				Camera->m_strName[sizeof(Camera->m_strName) - 1] = 0;

				gMainWindow->UpdateCameraMenu();

				CheckPointString = tr("Naming Camera");
			}
		}
	}

	if (!CheckPointString.isEmpty())
	{
		SaveCheckpoint(CheckPointString);
		gMainWindow->UpdateFocusObject(GetFocusObject());
		gMainWindow->UpdateAllViews();
	}
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

bool lcModel::GetPieceFocusOrSelectionCenter(lcVector3& Center) const
{
	float Bounds[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	bool Selected = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
		{
			Center = Piece->mPosition;
			return true;
		}

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Bounds);
			Selected = true;
		}
	}

	if (Selected)
		Center = lcVector3((Bounds[0] + Bounds[3]) * 0.5f, (Bounds[1] + Bounds[4]) * 0.5f, (Bounds[2] + Bounds[5]) * 0.5f);
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return Selected;
}

lcVector3 lcModel::GetFocusOrSelectionCenter() const
{
	lcVector3 Center;

	if (GetFocusPosition(Center))
		return Center;

	if (GetSelectionCenter(Center))
		return Center;

	return lcVector3(0.0f, 0.0f, 0.0f);
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

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			Camera->CompareBoundingBox(Bounds);
			Selected = true;
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			Light->CompareBoundingBox(Bounds);
			Selected = true;
		}
	}

	if (Selected)
		Center = lcVector3((Bounds[0] + Bounds[3]) * 0.5f, (Bounds[1] + Bounds[4]) * 0.5f, (Bounds[2] + Bounds[5]) * 0.5f);
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return Selected;
}

bool lcModel::GetPiecesBoundingBox(float BoundingBox[6], bool IncludeHidden) const
{
	BoundingBox[0] = FLT_MAX;
	BoundingBox[1] = FLT_MAX;
	BoundingBox[2] = FLT_MAX;
	BoundingBox[3] = -FLT_MAX;
	BoundingBox[4] = -FLT_MAX;
	BoundingBox[5] = -FLT_MAX;

	if (mPieces.IsEmpty())
		return false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (IncludeHidden || Piece->IsVisible(mCurrentStep))
			Piece->CompareBoundingBox(BoundingBox);
	}

	return true;
}

void lcModel::GetPartsList(int DefaultColorIndex, lcArray<lcPartsListEntry>& PartsList) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		int ColorIndex = Piece->mColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		int UsedIdx;

		for (UsedIdx = 0; UsedIdx < PartsList.GetSize(); UsedIdx++)
		{
			if (PartsList[UsedIdx].Info != Piece->mPieceInfo || PartsList[UsedIdx].ColorIndex != ColorIndex)
				continue;

			PartsList[UsedIdx].Count++;
			break;
		}

		if (UsedIdx == PartsList.GetSize())
			Piece->mPieceInfo->GetPartsList(ColorIndex, PartsList);
	}
}

void lcModel::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcArray<lcModelPartsEntry>& ModelParts) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		int ColorIndex = Piece->mColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		Piece->mPieceInfo->GetModelParts(lcMul(Piece->mModelWorld, WorldMatrix), ColorIndex, ModelParts);
	}
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

void lcModel::SetSelection(const lcArray<lcObject*>& Objects)
{
	ClearSelection(false);
	AddToSelection(Objects);
}

void lcModel::AddToSelection(const lcArray<lcObject*>& Objects)
{
	for (int ObjectIdx = 0; ObjectIdx < Objects.GetSize(); ObjectIdx++)
	{
		lcObject* Object = Objects[ObjectIdx];

		bool WasSelected = Object->IsSelected();
		Object->SetSelected(Objects[ObjectIdx]);

		if (!WasSelected && Object->GetType() == LC_OBJECT_PIECE)
			SelectGroup(((lcPiece*)Object)->GetTopGroup(), true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(GetFocusObject());
}

void lcModel::SelectAllPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
}

void lcModel::InvertSelection()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(!Piece->IsSelected());
	}

	gMainWindow->UpdateFocusObject(GetFocusObject());
	UpdateSelection();
	gMainWindow->UpdateAllViews();
}

void lcModel::HideSelectedPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->SetHidden(true);
			Piece->SetSelected(false);
		}
	}

	UpdateSelection();
	gMainWindow->UpdateFocusObject(NULL);
	gMainWindow->UpdateAllViews();
}

void lcModel::HideUnselectedPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected())
			Piece->SetHidden(true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
}

void lcModel::UnhideAllPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		mPieces[PieceIdx]->SetHidden(false);

	UpdateSelection();
	gMainWindow->UpdateAllViews();
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

void lcModel::UndoAction()
{
	if (mUndoHistory.GetSize() < 2)
		return;

	lcModelHistoryEntry* Undo = mUndoHistory[0];
	mUndoHistory.RemoveIndex(0);
	mRedoHistory.InsertAt(0, Undo);

	LoadCheckPoint(mUndoHistory[0]);

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
}

void lcModel::RedoAction()
{
	if (mRedoHistory.IsEmpty())
		return;

	lcModelHistoryEntry* Redo = mRedoHistory[0];
	mRedoHistory.RemoveIndex(0);
	mUndoHistory.InsertAt(0, Redo);

	LoadCheckPoint(Redo);

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
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

	mMouseToolDistance = Target;

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_TARGET);
}

void lcModel::UpdateSpotLightTool(const lcVector3& Position)
{
	lcLight* Light = mLights[mLights.GetSize() - 1];

	Light->Move(1, false, Position - mMouseToolDistance);
	Light->UpdatePosition(1);

	mMouseToolDistance = Position;

	gMainWindow->UpdateFocusObject(Light);
	gMainWindow->UpdateAllViews();
}

void lcModel::BeginCameraTool(const lcVector3& Position, const lcVector3& Target)
{
	lcCamera* Camera = new lcCamera(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	Camera->CreateName(mCameras);
	mCameras.Add(Camera);

	mMouseToolDistance = Position;

	ClearSelectionAndSetFocus(Camera, LC_CAMERA_SECTION_TARGET);
}

void lcModel::UpdateCameraTool(const lcVector3& Position)
{
	lcCamera* Camera = mCameras[mCameras.GetSize() - 1];

	Camera->Move(1, false, Position - mMouseToolDistance);
	Camera->UpdatePosition(1);

	mMouseToolDistance = Position;

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
	RotateSelectedPieces(Delta, false);
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

void lcModel::LookAt(lcCamera* Camera)
{
	lcVector3 Center;

	if (!GetSelectionCenter(Center))
	{
		float BoundingBox[6];

		if (GetPiecesBoundingBox(BoundingBox, false))
			Center = lcVector3((BoundingBox[0] + BoundingBox[3]) / 2, (BoundingBox[1] + BoundingBox[4]) / 2, (BoundingBox[2] + BoundingBox[5]) / 2);
		else
			Center = lcVector3(0.0f, 0.0f, 0.0f);
	}

	Camera->Center(Center, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Look At"));
}

void lcModel::ZoomExtents(lcCamera* Camera, float Aspect)
{
	float BoundingBox[6];

	if (!GetPiecesBoundingBox(BoundingBox, false))
		return;

	lcVector3 Center((BoundingBox[0] + BoundingBox[3]) / 2, (BoundingBox[1] + BoundingBox[4]) / 2, (BoundingBox[2] + BoundingBox[5]) / 2);

	lcVector3 Points[8] =
	{
		lcVector3(BoundingBox[0], BoundingBox[1], BoundingBox[5]),
		lcVector3(BoundingBox[3], BoundingBox[1], BoundingBox[5]),
		lcVector3(BoundingBox[0], BoundingBox[1], BoundingBox[2]),
		lcVector3(BoundingBox[3], BoundingBox[4], BoundingBox[5]),
		lcVector3(BoundingBox[3], BoundingBox[4], BoundingBox[2]),
		lcVector3(BoundingBox[0], BoundingBox[4], BoundingBox[2]),
		lcVector3(BoundingBox[0], BoundingBox[4], BoundingBox[5]),
		lcVector3(BoundingBox[3], BoundingBox[1], BoundingBox[2])
	};

	Camera->ZoomExtents(Aspect, Center, Points, 8, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Zoom"));
}

void lcModel::Zoom(lcCamera* Camera, float Amount)
{
	Camera->Zoom(Amount, mCurrentStep, gMainWindow->GetAddKeys());
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Zoom"));
}

void lcModel::ShowPropertiesDialog()
{
	lcPropertiesDialogOptions Options;

	Options.Properties = mProperties;
	Options.SetDefault = false;

	GetPartsList(gDefaultColor, Options.PartsList);

	if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
		return;

	if (Options.SetDefault)
		Options.Properties.SaveDefaults();

	if (mProperties == Options.Properties)
		return;

	mProperties = Options.Properties;

	UpdateBackgroundTexture();

	SaveCheckpoint(tr("Changing Properties"));
}

void lcModel::ShowSelectByNameDialog()
{
	if (mPieces.IsEmpty() && mCameras.IsEmpty() && mLights.IsEmpty())
	{
		gMainWindow->DoMessageBox("Nothing to select.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	lcSelectDialogOptions Options;
	if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
		return;

	SetSelection(Options.Objects);
}

void lcModel::ShowArrayDialog()
{
	lcVector3 Center;

	if (!GetPieceFocusOrSelectionCenter(Center))
	{
		gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}
	
	lcArrayDialogOptions Options;

	memset(&Options, 0, sizeof(Options));
	Options.Counts[0] = 10;
	Options.Counts[1] = 1;
	Options.Counts[2] = 1;

	if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_ARRAY, &Options))
		return;

	if (Options.Counts[0] * Options.Counts[1] * Options.Counts[2] < 2)
	{
		gMainWindow->DoMessageBox("Array only has 1 element or less, no pieces added.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	lcArray<lcObject*> NewPieces;

	for (int Step1 = 0; Step1 < Options.Counts[0]; Step1++)
	{
		for (int Step2 = 0; Step2 < Options.Counts[1]; Step2++)
		{
			for (int Step3 = (Step1 == 0 && Step2 == 0) ? 1 : 0; Step3 < Options.Counts[2]; Step3++)
			{
				lcMatrix44 ModelWorld;
				lcVector3 Position;

				lcVector3 RotationAngles = Options.Rotations[0] * Step1 + Options.Rotations[1] * Step2 + Options.Rotations[2] * Step3;
				lcVector3 Offset = Options.Offsets[0] * Step1 + Options.Offsets[1] * Step2 + Options.Offsets[2] * Step3;

				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = mPieces[PieceIdx];

					if (!Piece->IsSelected())
						continue;

					ModelWorld = Piece->mModelWorld;

					ModelWorld.r[3] -= lcVector4(Center, 0.0f);
					ModelWorld = lcMul(ModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
					ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
					ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));
					ModelWorld.r[3] += lcVector4(Center, 0.0f);

					Position = lcVector3(ModelWorld.r[3].x, ModelWorld.r[3].y, ModelWorld.r[3].z);

					lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
					AxisAngle[3] *= LC_RTOD;

					lcPiece* NewPiece = new lcPiece(Piece->mPieceInfo);
					NewPiece->Initialize(Position + Offset, AxisAngle, mCurrentStep);
					NewPiece->SetColorIndex(Piece->mColorIndex);

					NewPieces.Add(NewPiece);
				}
			}
		}
	}

	for (int PieceIdx = 0; PieceIdx < NewPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = (lcPiece*)NewPieces[PieceIdx];
		Piece->CreateName(mPieces);
		Piece->UpdatePosition(mCurrentStep);
		mPieces.Add(Piece);
	}

	AddToSelection(NewPieces);
	SaveCheckpoint(tr("Array"));
}

void lcModel::ShowMinifigDialog()
{
	lcMinifig Minifig;

	if (!gMainWindow->DoDialog(LC_DIALOG_MINIFIG, &Minifig))
		return;

	lcGroup* Group = AddGroup("Minifig #", NULL);
	lcArray<lcObject*> Pieces(LC_MFW_NUMITEMS);

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (Minifig.Parts[PartIdx] == NULL)
			continue;

		lcPiece* Piece = new lcPiece(Minifig.Parts[PartIdx]);

		lcVector3 Position(Minifig.Matrices[PartIdx][3][0], Minifig.Matrices[PartIdx][3][1], Minifig.Matrices[PartIdx][3][2]);
		lcVector4 Rotation = lcMatrix44ToAxisAngle(Minifig.Matrices[PartIdx]);
		Rotation[3] *= LC_RTOD;
		Piece->Initialize(Position, Rotation, mCurrentStep);
		Piece->SetColorIndex(Minifig.Colors[PartIdx]);
		Piece->CreateName(mPieces);
		Piece->SetGroup(Group);
		mPieces.Add(Piece);
		Piece->UpdatePosition(mCurrentStep);

		Pieces.Add(Piece);
	}

	SetSelection(Pieces);
	SaveCheckpoint(tr("Minifig"));
}

void lcModel::UpdateInterface()
{
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
	gMainWindow->UpdatePaste(!g_App->mClipboard.isEmpty());
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle();
	gMainWindow->SetTool(gMainWindow->GetTool());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->SetTransformType(gMainWindow->GetTransformType());
	gMainWindow->UpdateLockSnap();
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateModels();
	gMainWindow->UpdatePerspective();
	gMainWindow->UpdateCurrentStep();

	UpdateSelection();
}
