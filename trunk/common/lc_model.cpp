#include "lc_global.h"
#include "lc_model.h"
#include <locale.h>
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
#include "lc_qgroupdialog.h"

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

	bool TypeChanged = (mBackgroundType != lcGetDefaultProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TYPE));

	switch (mBackgroundType)
	{
	case LC_BACKGROUND_SOLID:
		if (mBackgroundSolidColor != lcVector3FromColor(lcGetDefaultProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR)) || TypeChanged)
			Stream << QLatin1String("0 !LEOCAD MODEL BACKGROUND COLOR ") << mBackgroundSolidColor[0] << ' ' << mBackgroundSolidColor[1] << ' ' << mBackgroundSolidColor[2] << LineEnding;
		break;

	case LC_BACKGROUND_GRADIENT:
		if (mBackgroundGradientColor1 != lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1)) ||
			mBackgroundGradientColor2 != lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2)) || TypeChanged)
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

//	bool mFogEnabled;
//	float mFogDensity;
//	lcVector3 mFogColor;
//	lcVector3 mAmbientColor;
}

void lcModelProperties::ParseLDrawLine(QTextStream& Stream)
{
	QString Token;
	Stream >> Token;

	if (Token == QLatin1String("AUTHOR"))
		mAuthor = Stream.readLine().mid(1);
	else if (Token == QLatin1String("DESCRIPTION"))
		mDescription = Stream.readLine().mid(1);
	else if (Token == QLatin1String("COMMENT"))
	{
		QString Comment = Stream.readLine().mid(1);
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
				mBackgroundImage = Stream.readLine().trimmed();
		}
	}
}

lcModel::lcModel(const QString& Name)
{
	mProperties.mName = Name;
	mProperties.LoadDefaults();

	mActive = false;
	mCurrentStep = 1;
	mBackgroundTexture = NULL;
	mPieceInfo = NULL;
}

lcModel::~lcModel()
{
	if (mPieceInfo)
	{
		if (gMainWindow && gMainWindow->mPreviewWidget->GetCurrentPiece() == mPieceInfo)
			gMainWindow->mPreviewWidget->SetDefaultPiece();

		if (mPieceInfo->GetModel() == this)
			mPieceInfo->SetPlaceholder();
		mPieceInfo->Release(true);
	}

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
		const lcArray<View*>* Views = gMainWindow->GetViewsForModel(this);

		// TODO: this is only needed to avoid a dangling pointer during undo/redo if a camera is set to a view but we should find a better solution instead
		if (Views)
		{
			for (int ViewIdx = 0; ViewIdx < Views->GetSize(); ViewIdx++)
			{
				View* View = (*Views)[ViewIdx];
				lcCamera* Camera = View->mCamera;

				if (!Camera->IsSimple() && mCameras.FindIndex(Camera) != -1)
					View->SetCamera(Camera, true);
			}
		}
	}

	mPieces.DeleteAll();
	mCameras.DeleteAll();
	mLights.DeleteAll();
	mGroups.DeleteAll();
	mFileLines.clear();
}

void lcModel::CreatePieceInfo(Project* Project)
{
	mPieceInfo = lcGetPiecesLibrary()->FindPiece(mProperties.mName.toUpper().toLatin1().constData(), Project, true);
	mPieceInfo->SetModel(this, true);
	mPieceInfo->AddRef();
}

void lcModel::UpdatePieceInfo(lcArray<lcModel*>& UpdatedModels)
{
	if (UpdatedModels.FindIndex(this) != -1)
		return;

	mPieceInfo->SetModel(this, false);
	UpdatedModels.Add(this);

	lcMesh* Mesh = mPieceInfo->GetMesh();

	if (mPieces.IsEmpty() && !Mesh)
	{
		mPieceInfo->SetBoundingBox(lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 0.0f));
		return;
	}

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->GetStepHide() == LC_STEP_MAX)
		{
			Piece->mPieceInfo->UpdateBoundingBox(UpdatedModels);
			Piece->CompareBoundingBox(Min, Max);
		}
	}

	if (Mesh)
	{
		Min = lcMin(Min, Mesh->mBoundingBox.Min);
		Max = lcMax(Max, Mesh->mBoundingBox.Max);
	}

	mPieceInfo->SetBoundingBox(Min, Max);
}

void lcModel::SaveLDraw(QTextStream& Stream, bool SelectedOnly) const
{
	QLatin1String LineEnding("\r\n");

	mProperties.SaveLDraw(Stream);

	if (mCurrentStep != GetLastStep())
		Stream << QLatin1String("0 !LEOCAD MODEL CURRENT_STEP") << mCurrentStep << LineEnding;

	lcArray<lcGroup*> CurrentGroups;
	lcStep Step = 1;
	int CurrentLine = 0;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (SelectedOnly && !Piece->IsSelected())
			continue;

		while (Piece->GetFileLine() > CurrentLine && CurrentLine < mFileLines.size())
		{
			QString Line = mFileLines[CurrentLine];
			QTextStream LineStream(&Line, QIODevice::ReadOnly);

			QString Token;
			LineStream >> Token;
			bool Skip = false;

			if (Token == QLatin1String("0"))
			{
				LineStream >> Token;

				if (Token == QLatin1String("STEP"))
				{
					if (Piece->GetStepShow() > Step)
						Step++;
					else
						Skip = true;
				}
			}

			if (!Skip)
				Stream << mFileLines[CurrentLine];
			CurrentLine++;
		}

		while (Piece->GetStepShow() > Step)
		{
			Stream << QLatin1String("0 STEP\r\n");
			Step++;
		}

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
					Stream << QLatin1String("0 !LEOCAD GROUP BEGIN ") << Group->mName << LineEnding;
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

	while (CurrentLine < mFileLines.size())
	{
		Stream << mFileLines[CurrentLine];
		CurrentLine++;
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

void lcModel::LoadLDraw(QIODevice& Device, Project* Project)
{
	lcPiece* Piece = NULL;
	lcCamera* Camera = NULL;
	lcLight* Light = NULL;
	lcArray<lcGroup*> CurrentGroups;
	int CurrentStep = 1;

	mProperties.mAuthor.clear();

	while (!Device.atEnd())
	{
		qint64 Pos = Device.pos();
		QString OriginalLine = Device.readLine();
		QString Line = OriginalLine.trimmed();
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
					Device.seek(Pos);
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
				mFileLines.append(OriginalLine);
				continue;
			}

			if (Token != QLatin1String("!LEOCAD"))
			{
				mFileLines.append(OriginalLine); 
				continue;
			}

			LineStream >> Token;

			if (Token == QLatin1String("MODEL"))
			{
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

			float IncludeMatrix[12];
			for (int TokenIdx = 0; TokenIdx < 12; TokenIdx++)
				LineStream >> IncludeMatrix[TokenIdx];

			lcMatrix44 IncludeTransform(lcVector4(IncludeMatrix[3], IncludeMatrix[6], IncludeMatrix[9], 0.0f), lcVector4(IncludeMatrix[4], IncludeMatrix[7], IncludeMatrix[10], 0.0f),
										lcVector4(IncludeMatrix[5], IncludeMatrix[8], IncludeMatrix[11], 0.0f), lcVector4(IncludeMatrix[0], IncludeMatrix[1], IncludeMatrix[2], 1.0f));

			QString File = LineStream.readAll().trimmed().toUpper();
			QString PartID = File;
			PartID.replace('\\', '/');

			if (PartID.endsWith(QLatin1String(".DAT")))
				PartID = PartID.left(PartID.size() - 4);

			lcPiecesLibrary* Library = lcGetPiecesLibrary();

			if (Library->IsPrimitive(PartID.toLatin1().constData()))
			{
				mFileLines.append(OriginalLine); 
			}
			else
			{
				if (!Piece)
					Piece = new lcPiece(NULL);

				if (!CurrentGroups.IsEmpty())
					Piece->SetGroup(CurrentGroups[CurrentGroups.GetSize() - 1]);

				PieceInfo* Info = Library->FindPiece(PartID.toLatin1().constData(), Project, false);

				if (!Info)
					Info = Library->FindPiece(File.toLatin1().constData(), Project, true);

				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
									 lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(Matrix[12], Matrix[14], -Matrix[13], 1.0f));

				Piece->SetFileLine(mFileLines.size());
				Piece->SetPieceInfo(Info);
				Piece->Initialize(Transform, CurrentStep);
				Piece->SetColorCode(ColorCode);
				AddPiece(Piece);
				Piece = NULL;
			}
		}
		else
			mFileLines.append(OriginalLine); 
	}

	mCurrentStep = CurrentStep;
	CalculateStep(mCurrentStep);
	UpdateBackgroundTexture();
	lcGetPiecesLibrary()->UnloadUnusedParts();

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

	int FirstNewPiece = mPieces.GetSize();

	while (count--)
	{
		if (fv > 0.4f)
		{
			lcPiece* pPiece = new lcPiece(NULL);
			pPiece->FileLoad(*file);
			AddPiece(pPiece);
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
			lcMatrix44 WorldMatrix = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			WorldMatrix.SetTranslation(pos);

			PieceInfo* pInfo = Library->FindPiece(name, NULL, true);
			lcPiece* pPiece = new lcPiece(pInfo);

			pPiece->Initialize(WorldMatrix, step);
			pPiece->SetColorCode(lcGetColorCodeFromOriginalColor(color));
			AddPiece(pPiece);

//			pPiece->SetGroup((lcGroup*)group);
		}
	}

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
				char Name[LC_MAX_GROUP_NAME + 1];
				file->ReadBuffer(Name, sizeof(Name));
				Group->mName = QString::fromUtf8(Name);
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
	lcGetPiecesLibrary()->UnloadUnusedParts();

	return true;
}

void lcModel::Merge(lcModel* Other)
{
	for (int PieceIdx = 0; PieceIdx < Other->mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Other->mPieces[PieceIdx];
		Piece->SetFileLine(-1);
		AddPiece(Piece);
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

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::Cut()
{
	Copy();

	if (RemoveSelectedObjects())
	{
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(true);
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

	QBuffer Buffer(&g_App->mClipboard);
	Buffer.open(QIODevice::ReadOnly);
	Model->LoadLDraw(Buffer, lcGetActiveProject());

	const lcArray<lcPiece*>& PastedPieces = Model->mPieces;
	lcArray<lcObject*> SelectedObjects;
	SelectedObjects.AllocGrow(PastedPieces.GetSize());

	for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = PastedPieces[PieceIdx];
		lcStep Step = Piece->GetStepShow();

		if (Step > mCurrentStep)
			Piece->SetStepShow(mCurrentStep);

		SelectedObjects.Add(Piece);
	}

	Merge(Model);
	SaveCheckpoint(tr("Pasting"));

	if (PastedPieces.GetSize() == 1)
		ClearSelectionAndSetFocus(PastedPieces[0], LC_PIECE_SECTION_POSITION);
	else
		SetSelectionAndFocus(SelectedObjects, NULL, 0);

	CalculateStep(mCurrentStep);
	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateAllViews();
}

void lcModel::GetScene(lcScene& Scene, lcCamera* ViewCamera, bool DrawInterface) const
{
	Scene.Begin(ViewCamera->mWorldView);

	mPieceInfo->AddRenderMesh(Scene);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->AddRenderMeshes(Scene, DrawInterface);
	}

	if (DrawInterface)
	{
		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		{
			lcCamera* Camera = mCameras[CameraIdx];

			if (Camera != ViewCamera && Camera->IsVisible())
				Scene.mInterfaceObjects.Add(Camera);
		}

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		{
			lcLight* Light = mLights[LightIdx];

			if (Light->IsVisible())
				Scene.mInterfaceObjects.Add(Light);
		}
	}

	Scene.End();
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

	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));

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

		Context->SetProgram(LC_PROGRAM_VERTEX_COLOR);
		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormat(0, 2, 0, 4);

		Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

		Context->ClearVertexBuffer(); // context remove

		glShadeModel(GL_FLAT);
	}
	else if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
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

		Context->SetProgram(LC_PROGRAM_TEXTURE);
		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormat(0, 2, 2, 0);

		Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

		Context->ClearVertexBuffer(); // context remove

		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

void lcModel::SaveStepImages(const QString& BaseName, int Width, int Height, lcStep Start, lcStep End)
{
	gMainWindow->mPreviewWidget->MakeCurrent();
	lcContext* Context = gMainWindow->mPreviewWidget->mContext;

	if (!Context->BeginRenderToTexture(Width, Height))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return;
	}

	lcStep CurrentStep = mCurrentStep;

	View View(this);
	View.SetCamera(gMainWindow->GetActiveView()->mCamera, false);
	View.mWidth = Width;
	View.mHeight = Height;
	View.SetContext(Context);

	for (lcStep Step = Start; Step <= End; Step++)
	{
		SetTemporaryStep(Step);
		View.OnDraw();

		QString FileName;

		if (Start == End)
			FileName = BaseName;
		else
			FileName = BaseName.arg(Step, 2, 10, QLatin1Char('0'));

		if (!Context->SaveRenderToTextureImage(FileName, Width, Height))
			break;
	}

	Context->EndRenderToTexture();

	SetTemporaryStep(CurrentStep);

	if (!mActive)
		CalculateStep(LC_STEP_MAX);
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

		lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(Piece->mModelWorld);
		lcVector3 Start = lcMul31(WorldStart, InverseWorldMatrix);
		lcVector3 End = lcMul31(WorldEnd, InverseWorldMatrix);

		if (Piece->GetStepHide() == LC_STEP_MAX && Piece->mPieceInfo->MinIntersectDist(Start, End, MinDistance)) // todo: this should check for piece->mMesh first
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

	QBuffer Buffer(&CheckPoint->File);
	Buffer.open(QIODevice::ReadOnly);
	LoadLDraw(Buffer, lcGetActiveProject());

	gMainWindow->UpdateTimeline(true, false);
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateSelectedObjects(true);
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

		strncpy(mPieceInfo->m_strName, mProperties.mName.toLatin1().constData(), sizeof(mPieceInfo->m_strName));
		strupr(mPieceInfo->m_strName);
		mPieceInfo->m_strName[sizeof(mPieceInfo->m_strName) - 1] = 0;
		strncpy(mPieceInfo->m_strDescription, mProperties.mName.toLatin1().constData(), sizeof(mPieceInfo->m_strDescription));
		mPieceInfo->m_strDescription[sizeof(mPieceInfo->m_strDescription) - 1] = 0;
	}

	mActive = Active;
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

void lcModel::SetCurrentStep(lcStep Step)
{
	mCurrentStep = Step;
	CalculateStep(Step);

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCurrentStep();
}

void lcModel::ShowFirstStep()
{
	if (mCurrentStep == 1)
		return;

	SetCurrentStep(1);
}

void lcModel::ShowLastStep()
{
	lcStep LastStep = GetLastStep();

	if (mCurrentStep == LastStep)
		return;

	SetCurrentStep(LastStep);
}

void lcModel::ShowPreviousStep()
{
	if (mCurrentStep == 1)
		return;

	SetCurrentStep(mCurrentStep - 1);
}

void lcModel::ShowNextStep()
{
	if (mCurrentStep == LC_STEP_MAX)
		return;

	SetCurrentStep(mCurrentStep + 1);
}

lcStep lcModel::GetLastStep() const
{
	lcStep Step = 1;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		Step = lcMax(Step, mPieces[PieceIdx]->GetStepShow());

	return Step;
}

void lcModel::InsertStep(lcStep Step)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->InsertTime(Step, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->InsertTime(Step, 1);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->InsertTime(Step, 1);

	SaveCheckpoint(tr("Inserting Step"));
	SetCurrentStep(mCurrentStep);
}

void lcModel::RemoveStep(lcStep Step)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];
		Piece->RemoveTime(Step, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->RemoveTime(Step, 1);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->RemoveTime(Step, 1);

	SaveCheckpoint(tr("Removing Step"));
	SetCurrentStep(mCurrentStep);
}

lcGroup* lcModel::AddGroup(const QString& Prefix, lcGroup* Parent)
{
	lcGroup* Group = new lcGroup();
	mGroups.Add(Group);

	Group->mName = GetGroupName(Prefix);
	Group->mGroup = Parent;

	return Group;
}

lcGroup* lcModel::GetGroup(const QString& Name, bool CreateIfMissing)
{
	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];

		if (Group->mName == Name)
			return Group;
	}

	if (CreateIfMissing)
	{
		lcGroup* Group = new lcGroup();
		Group->mName = Name;
		mGroups.Add(Group);

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
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No pieces selected."));
		return;
	}

	lcQGroupDialog Dialog(gMainWindow, GetGroupName(tr("Group #")));
	if (Dialog.exec() != QDialog::Accepted)
		return;

	lcGroup* NewGroup = GetGroup(Dialog.mName, true);

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

QString lcModel::GetGroupName(const QString& Prefix)
{
	int Length = Prefix.length();
	int Max = 0;

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		const QString& Name = mGroups[GroupIdx]->mName;

		if (Name.startsWith(Prefix))
		{
			bool Ok = false;
			int GroupNumber = Name.mid(Length).toInt(&Ok);
			if (Ok && GroupNumber > Max)
				Max = GroupNumber;
		}
	}

	return Prefix + QString::number(Max + 1);
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

	float SnapXY = gMainWindow->GetMoveXYSnap();
	if (SnapXY != 0.0f)
	{
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

	float SnapZ = gMainWindow->GetMoveZSnap();
	if (SnapZ != 0.0f)
	{
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

lcMatrix33 lcModel::GetRelativeRotation() const
{
	if (gMainWindow->GetRelativeTransform())
	{
		lcObject* Focus = GetFocusObject();

		if (Focus && Focus->IsPiece())
			return ((lcPiece*)Focus)->GetRelativeRotation();
	}

	return lcMatrix33Identity();
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

	lcMatrix44 WorldMatrix;

	if (Last)
	{
		const lcBoundingBox& BoundingBox = Last->GetBoundingBox();
		lcVector3 Dist(0, 0, BoundingBox.Max.z - BoundingBox.Min.z);
		Dist = SnapPosition(Dist);

		WorldMatrix = Last->mModelWorld;
		WorldMatrix.SetTranslation(lcMul31(Dist, Last->mModelWorld));
	}
	else
	{
		const lcBoundingBox& BoundingBox = CurPiece->GetBoundingBox();
		WorldMatrix = lcMatrix44Translation(lcVector3(0.0f, 0.0f, -BoundingBox.Min.z));
	}

	lcPiece* Piece = new lcPiece(CurPiece);
	Piece->Initialize(WorldMatrix, mCurrentStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	AddPiece(Piece);
	gMainWindow->UpdateTimeline(false, false);
	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION);

	SaveCheckpoint("Adding Piece");
}

void lcModel::AddPiece(lcPiece* Piece)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		if (mPieces[PieceIdx]->GetStepShow() > Piece->GetStepShow())
		{
			InsertPiece(Piece, PieceIdx);
			return;
		}
	}

	InsertPiece(Piece, mPieces.GetSize());
}

void lcModel::InsertPiece(lcPiece* Piece, int Index)
{
	PieceInfo* Info = Piece->mPieceInfo;

	if (!Info->IsModel())
	{
		lcMesh* Mesh = Info->IsTemporary() ? gPlaceholderMesh : Info->GetMesh();

		if (Mesh->mVertexCacheOffset == -1)
			lcGetPiecesLibrary()->mBuffersDirty = true;
	}

	mPieces.InsertAt(Index, Piece);
}

void lcModel::DeleteAllCameras()
{
	if (mCameras.IsEmpty())
		return;

	mCameras.DeleteAll();

	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
	SaveCheckpoint("Reseting Cameras");
}

void lcModel::DeleteSelectedObjects()
{
	if (RemoveSelectedObjects())
	{
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
		SaveCheckpoint("Deleting");
	}
}

void lcModel::ResetSelectedPiecesPivotPoint()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
			Piece->ResetPivotPoint();
	}

	gMainWindow->UpdateAllViews();
}

void lcModel::InsertControlPoint()
{
	lcObject* Focus = GetFocusObject();

	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* Piece = (lcPiece*)Focus;

	lcVector3 Start, End;
	gMainWindow->GetActiveView()->GetRayUnderPointer(Start, End);

	if (Piece->InsertControlPoint(Start, End))
	{
		SaveCheckpoint("Modifying");
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
	}
}

void lcModel::RemoveFocusedControlPoint()
{
	lcObject* Focus = GetFocusObject();

	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* Piece = (lcPiece*)Focus;

	if (Piece->RemoveFocusedControlPoint())
	{
		SaveCheckpoint("Modifying");
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
	}
}

void lcModel::ShowSelectedPiecesEarlier()
{
	lcArray<lcPiece*> MovedPieces;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step > 1)
			{
				Step--;
				Piece->SetStepShow(Step);

				MovedPieces.Add(Piece);
				mPieces.RemoveIndex(PieceIdx);
				continue;
			}
		}

		PieceIdx++;
	}

	if (MovedPieces.IsEmpty())
		return;

	for (int PieceIdx = 0; PieceIdx < MovedPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = MovedPieces[PieceIdx];
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	SaveCheckpoint("Modifying");
	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::ShowSelectedPiecesLater()
{
	lcArray<lcPiece*> MovedPieces;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step < LC_STEP_MAX)
			{
				Step++;
				Piece->SetStepShow(Step);

				if (!Piece->IsVisible(mCurrentStep))
					Piece->SetSelected(false);

				MovedPieces.Add(Piece);
				mPieces.RemoveIndex(PieceIdx);
				continue;
			}
		}

		PieceIdx++;
	}

	if (MovedPieces.IsEmpty())
		return;

	for (int PieceIdx = 0; PieceIdx < MovedPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = MovedPieces[PieceIdx];
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	SaveCheckpoint("Modifying");
	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::SetPieceSteps(const QList<QPair<lcPiece*, lcStep>>& PieceSteps)
{
	if (PieceSteps.size() != mPieces.GetSize())
		return;

	bool Modified = false;

	for (int PieceIdx = 0; PieceIdx < PieceSteps.size(); PieceIdx++)
	{
		const QPair<lcPiece*, lcStep>& PieceStep = PieceSteps[PieceIdx];
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece != PieceStep.first || Piece->GetStepShow() != PieceStep.second)
		{
			Piece = PieceStep.first;
			lcStep Step = PieceStep.second;

			mPieces[PieceIdx] = Piece;
			Piece->SetStepShow(Step);

			if (!Piece->IsVisible(mCurrentStep))
				Piece->SetSelected(false);

			Modified = true;
		}
	}

	if (Modified)
	{
		SaveCheckpoint("Modifying");
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(true);
	}
}

void lcModel::MoveSelectionToModel(lcModel* Model)
{
	if (!Model)
		return;

	lcArray<lcPiece*> Pieces;
	lcPiece* ModelPiece = NULL;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			mPieces.RemoveIndex(PieceIdx);
			Piece->SetGroup(NULL); // todo: copy groups
			Pieces.Add(Piece);

			if (!ModelPiece)
			{
				ModelPiece = new lcPiece(Model->mPieceInfo);
				ModelPiece->Initialize(lcMatrix44Identity(), Piece->GetStepShow());
				ModelPiece->SetColorIndex(gDefaultColor);
				InsertPiece(ModelPiece, PieceIdx);
				PieceIdx++;
			}
		}
		else
			PieceIdx++;
	}

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
		Model->AddPiece(Pieces[PieceIdx]);

	lcArray<lcModel*> UpdatedModels;
	Model->UpdatePieceInfo(UpdatedModels);
	ModelPiece->UpdatePosition(mCurrentStep);

	SaveCheckpoint("New Model");
	gMainWindow->UpdateTimeline(false, false);
	ClearSelectionAndSetFocus(ModelPiece, LC_PIECE_SECTION_POSITION);
}

void lcModel::InlineSelectedModels()
{
	lcArray<lcObject*> NewPieces;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected() || !Piece->mPieceInfo->IsModel())
		{
			PieceIdx++;
			continue;
		}

		mPieces.RemoveIndex(PieceIdx);

		lcArray<lcModelPartsEntry> ModelParts;
		Piece->mPieceInfo->GetModelParts(Piece->mModelWorld, Piece->mColorIndex, ModelParts);

		for (int InsertIdx = 0; InsertIdx < ModelParts.GetSize(); InsertIdx++)
		{
			lcModelPartsEntry& Entry = ModelParts[InsertIdx];

			lcPiece* NewPiece = new lcPiece(Entry.Info);

			// todo: recreate in groups in the current model

			NewPiece->Initialize(Entry.WorldMatrix, Piece->GetStepShow());
			NewPiece->SetColorIndex(Entry.ColorIndex);
			NewPiece->UpdatePosition(mCurrentStep);

			NewPieces.Add(NewPiece);
			InsertPiece(NewPiece, PieceIdx);
			PieceIdx++;
		}

		delete Piece;
	}

	if (!NewPieces.GetSize())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No models selected."));
		return;
	}

	SaveCheckpoint("Inlining");
	gMainWindow->UpdateTimeline(false, false);
	SetSelectionAndFocus(NewPieces, NULL, 0);
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
			const lcArray<View*>* Views = gMainWindow->GetViewsForModel(this);
			for (int ViewIdx = 0; ViewIdx < Views->GetSize(); ViewIdx++)
			{
				View* View = (*Views)[ViewIdx];

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

void lcModel::MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool Relative, bool AlternateButtonDrag, bool Update, bool Checkpoint)
{
	bool Moved = false;
	lcMatrix33 RelativeRotation;

	if (Relative)
		RelativeRotation = GetRelativeRotation();
	else
		RelativeRotation = lcMatrix33Identity();

	if (PieceDistance.LengthSquared() >= 0.001f)
	{
		lcVector3 TransformedPieceDistance = lcMul(PieceDistance, RelativeRotation);

		if (AlternateButtonDrag)
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsFocused())
				{
					Piece->MovePivotPoint(TransformedPieceDistance);
					Moved = true;
					break;
				}
			}
		}
		else
		{
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
	}

	if (ObjectDistance.LengthSquared() >= 0.001f && !AlternateButtonDrag)
	{
		lcVector3 TransformedObjectDistance = lcMul(ObjectDistance, RelativeRotation);

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
		if (Checkpoint)
			SaveCheckpoint("Moving");
		gMainWindow->UpdateSelectedObjects(false);
	}
}

void lcModel::RotateSelectedPieces(const lcVector3& Angles, bool Relative, bool AlternateButtonDrag, bool Update, bool Checkpoint)
{
	if (Angles.LengthSquared() < 0.001f)
		return;

	lcMatrix33 RotationMatrix = lcMatrix33Identity();
	bool Rotated = false;

	if (Angles[0] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationX(Angles[0] * LC_DTOR), RotationMatrix);

	if (Angles[1] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationY(Angles[1] * LC_DTOR), RotationMatrix);

	if (Angles[2] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationZ(Angles[2] * LC_DTOR), RotationMatrix);

	if (AlternateButtonDrag)
	{
		lcObject* Focus = GetFocusObject();

		if (Focus && Focus->IsPiece())
		{
			((lcPiece*)Focus)->RotatePivotPoint(RotationMatrix);
			Rotated = true;
		}
	}
	else
	{
		lcVector3 Center;
		lcMatrix33 RelativeRotation;

		GetMoveRotateTransform(Center, RelativeRotation);

		lcMatrix33 WorldToFocusMatrix;

		if (Relative)
		{
			WorldToFocusMatrix = lcMatrix33AffineInverse(RelativeRotation);
			RotationMatrix = lcMul(RotationMatrix, RelativeRotation);
		}
		else
			WorldToFocusMatrix = lcMatrix33Identity();

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (!Piece->IsSelected())
				continue;

			Piece->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
			Piece->UpdatePosition(mCurrentStep);
			Rotated = true;
		}
	}

	if (Rotated && Update)
	{
		gMainWindow->UpdateAllViews();
		if (Checkpoint)
			SaveCheckpoint("Rotating");
		gMainWindow->UpdateSelectedObjects(false);
	}
}

void lcModel::ScaleSelectedPieces(const float Scale, bool Update, bool Checkpoint)
{
	if (Scale < 0.001f)
		return;

	lcObject* Focus = GetFocusObject();
	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* Piece = (lcPiece*)Focus;
	lcuint32 Section = Piece->GetFocusSection();

	if (Section >= LC_PIECE_SECTION_CONTROL_POINT_1 && Section <= LC_PIECE_SECTION_CONTROL_POINT_8)
	{
		int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;
		Piece->SetControlPointScale(ControlPointIndex, Scale);

		if (Update)
		{
			gMainWindow->UpdateAllViews();
			if (Checkpoint)
				SaveCheckpoint("Scaling");
			gMainWindow->UpdateSelectedObjects(false);
		}
	}
}

void lcModel::TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform)
{
	switch (TransformType)
	{
	case LC_TRANSFORM_ABSOLUTE_TRANSLATION:
		MoveSelectedObjects(Transform, false, false, true, true);
		break;

	case LC_TRANSFORM_RELATIVE_TRANSLATION:
		MoveSelectedObjects(Transform, true, false, true, true);
		break;

	case LC_TRANSFORM_ABSOLUTE_ROTATION:
		RotateSelectedPieces(Transform, false, false, true, true);
		break;

	case LC_TRANSFORM_RELATIVE_ROTATION:
		RotateSelectedPieces(Transform, true, false, true, true);
		break;
	}
}

void lcModel::SetSelectedPiecesColorIndex(int ColorIndex)
{
	bool Modified = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected() && Piece->mColorIndex != ColorIndex)
		{
			Piece->SetColorIndex(ColorIndex);
			Modified = true;
		}
	}

	if (Modified)
	{
		SaveCheckpoint(tr("Painting"));
		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, true);
	}
}

void lcModel::SetSelectedPiecesPieceInfo(PieceInfo* Info)
{
	bool Modified = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected() && Piece->mPieceInfo != Info)
		{
			Piece->mPieceInfo->Release(true);
			Piece->SetPieceInfo(Info);
			Modified = true;
		}
	}

	if (Modified)
	{
		SaveCheckpoint(tr("Setting Part"));
		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, true);
	}
}

void lcModel::SetSelectedPiecesStepShow(lcStep Step)
{
	bool Modified = false;
	bool SelectionChanged = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected() && Piece->GetStepShow() != Step)
		{
			Piece->SetStepShow(Step);

			if (!Piece->IsVisible(mCurrentStep))
			{
				Piece->SetSelected(false);
				SelectionChanged = true;
			}

			Modified = true;
		}
	}

	if (Modified)
	{
		SaveCheckpoint(tr("Showing Pieces"));
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(SelectionChanged);
	}
}

void lcModel::SetSelectedPiecesStepHide(lcStep Step)
{
	bool Modified = false;
	bool SelectionChanged = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected() && Piece->GetStepHide() != Step)
		{
			Piece->SetStepHide(Step);

			if (!Piece->IsVisible(mCurrentStep))
			{
				Piece->SetSelected(false);
				SelectionChanged = true;
			}

			Modified = true;
		}
	}

	if (Modified)
	{
		SaveCheckpoint(tr("Hiding Pieces"));
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(SelectionChanged);
	}
}

void lcModel::SetCameraOrthographic(lcCamera* Camera, bool Ortho)
{
	if (Camera->IsOrtho() == Ortho)
		return;

	Camera->SetOrtho(Ortho);
	Camera->UpdatePosition(mCurrentStep);

	SaveCheckpoint(tr("Editing Camera"));
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdatePerspective();
}

void lcModel::SetCameraFOV(lcCamera* Camera, float FOV)
{
	if (Camera->m_fovy == FOV)
		return;

	Camera->m_fovy = FOV;
	Camera->UpdatePosition(mCurrentStep);

	SaveCheckpoint(tr("Changing FOV"));
	gMainWindow->UpdateAllViews();
}

void lcModel::SetCameraZNear(lcCamera* Camera, float ZNear)
{
	if (Camera->m_zNear == ZNear)
		return;

	Camera->m_zNear = ZNear;
	Camera->UpdatePosition(mCurrentStep);

	SaveCheckpoint(tr("Editing Camera"));
	gMainWindow->UpdateAllViews();
}

void lcModel::SetCameraZFar(lcCamera* Camera, float ZFar)
{
	if (Camera->m_zFar == ZFar)
		return;

	Camera->m_zFar = ZFar;
	Camera->UpdatePosition(mCurrentStep);

	SaveCheckpoint(tr("Editing Camera"));
	gMainWindow->UpdateAllViews();
}

void lcModel::SetCameraName(lcCamera* Camera, const char* Name)
{
	if (!strcmp(Camera->m_strName, Name))
		return;

	strncpy(Camera->m_strName, Name, sizeof(Camera->m_strName));
	Camera->m_strName[sizeof(Camera->m_strName) - 1] = 0;

	SaveCheckpoint(tr("Renaming Camera"));
	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateCameraMenu();
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

lcModel* lcModel::GetFirstSelectedSubmodel() const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected() && Piece->mPieceInfo->IsModel())
			return Piece->mPieceInfo->GetModel();
	}

	return NULL;
}

bool lcModel::GetMoveRotateTransform(lcVector3& Center, lcMatrix33& RelativeRotation) const
{
	bool Relative = gMainWindow->GetRelativeTransform();
	int NumSelected = 0;

	Center = lcVector3(0.0f, 0.0f, 0.0f);
	RelativeRotation = lcMatrix33Identity();

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected())
			continue;

		if (Piece->IsFocused() && Relative)
		{
			Center = Piece->GetRotationCenter();
			RelativeRotation = Piece->GetRelativeRotation();
			return true;
		}

		Center += Piece->mModelWorld.GetTranslation();
		NumSelected++;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (!Camera->IsSelected())
			continue;

		if (Camera->IsFocused() && Relative)
		{
			Center = Camera->GetSectionPosition(Camera->GetFocusSection());
//			RelativeRotation = Piece->GetRelativeRotation();
			return true;
		}

		Center += Camera->GetSectionPosition(LC_CAMERA_SECTION_POSITION);
		Center += Camera->GetSectionPosition(LC_CAMERA_SECTION_TARGET);
		Center += Camera->GetSectionPosition(LC_CAMERA_SECTION_UPVECTOR);
		NumSelected += 3;
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (!Light->IsSelected())
			continue;

		if (Light->IsFocused() && Relative)
		{
			Center = Light->GetSectionPosition(Light->GetFocusSection());
//			RelativeRotation = Piece->GetRelativeRotation();
			return true;
		}

		Center += Light->GetSectionPosition(LC_LIGHT_SECTION_POSITION);
		NumSelected++;
		if (Light->IsSpotLight() || Light->IsDirectionalLight())
		{
			Center += Light->GetSectionPosition(LC_LIGHT_SECTION_TARGET);
			NumSelected++;
		}
	}

	if (NumSelected)
	{
		Center /= NumSelected;
		return true;
	}

	return false;
}

bool lcModel::GetPieceFocusOrSelectionCenter(lcVector3& Center) const
{
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	lcPiece* Selected = NULL;
	int NumSelected = 0;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
		{
			Center = Piece->mModelWorld.GetTranslation();
			return true;
		}

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			Selected = Piece;
			NumSelected++;
		}
	}

	if (NumSelected == 1)
		Center = Selected->mModelWorld.GetTranslation();
	else if (NumSelected)
		Center = (Min + Max) / 2.0f;
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return NumSelected != 0;
}

lcVector3 lcModel::GetSelectionOrModelCenter() const
{
	lcVector3 Center;

	if (!GetSelectionCenter(Center))
	{
		lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		if (GetPiecesBoundingBox(Min, Max))
			Center = (Min + Max) / 2.0f;
		else
			Center = lcVector3(0.0f, 0.0f, 0.0f);
	}

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
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	lcPiece* SelectedPiece = NULL;
	bool SinglePieceSelected = true;
	bool Selected = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			Selected = true;

			if (!SelectedPiece)
				SelectedPiece = Piece;
			else
				SinglePieceSelected = false;
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			Camera->CompareBoundingBox(Min, Max);
			Selected = true;
			SinglePieceSelected = false;
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			Light->CompareBoundingBox(Min, Max);
			Selected = true;
			SinglePieceSelected = false;
		}
	}

	if (SelectedPiece && SinglePieceSelected)
		Center = SelectedPiece->GetSectionPosition(LC_PIECE_SECTION_POSITION);
	else if (Selected)
		Center = (Min + Max) / 2.0f;
	else
		Center = lcVector3(0.0f, 0.0f, 0.0f);

	return Selected;
}

bool lcModel::GetPiecesBoundingBox(lcVector3& Min, lcVector3& Max) const
{
	Min = lcVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	Max = lcVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	if (mPieces.IsEmpty())
		return false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->CompareBoundingBox(Min, Max);
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

void lcModel::GetPartsListForStep(lcStep Step, int DefaultColorIndex, lcArray<lcPartsListEntry>& PartsList) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->GetStepShow() != Step)
			continue;

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

void lcModel::GetSelectionInformation(int* Flags, lcArray<lcObject*>& Selection, lcObject** Focus) const
{
	*Flags = 0;
	*Focus = NULL;

	if (mPieces.IsEmpty())
		*Flags |= LC_SEL_NO_PIECES;
	else
	{
		lcGroup* Group = NULL;
		bool First = true;

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (Piece->IsSelected())
			{
				Selection.Add(Piece);

				if (Piece->IsFocused())
					*Focus = Piece;

				if (Piece->mPieceInfo->IsModel())
					*Flags |= LC_SEL_MODEL_SELECTED;

				if (Piece->IsHidden())
					*Flags |= LC_SEL_HIDDEN | LC_SEL_HIDDEN_SELECTED;
				else
					*Flags |= LC_SEL_VISIBLE_SELECTED;

				*Flags |= LC_SEL_PIECE | LC_SEL_SELECTED;

				if (Piece->mPieceInfo->GetSynthInfo())
				{
					*Flags |= LC_SEL_CAN_ADD_CONTROL_POINT;

					lcuint32 Section = Piece->GetFocusSection();

					if (Section >= LC_PIECE_SECTION_CONTROL_POINT_1 && Section <= LC_PIECE_SECTION_CONTROL_POINT_8 && Piece->GetControlPoints().GetSize() > 2)
						*Flags |= LC_SEL_CAN_REMOVE_CONTROL_POINT;
				}

				if (Piece->GetGroup() != NULL)
				{
					*Flags |= LC_SEL_GROUPED;
					if (Piece->IsFocused())
						*Flags |= LC_SEL_FOCUS_GROUPED;
				}

				if (First)
				{
					Group = Piece->GetGroup();
					First = false;
				}
				else
				{
					if (Group != Piece->GetGroup())
						*Flags |= LC_SEL_CAN_GROUP;
					else
						if (Group == NULL)
							*Flags |= LC_SEL_CAN_GROUP;
				}
			}
			else
			{
				*Flags |= LC_SEL_UNSELECTED;

				if (Piece->IsHidden())
					*Flags |= LC_SEL_HIDDEN;
			}
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			Selection.Add(Camera);
			*Flags |= LC_SEL_SELECTED;

			if (Camera->IsFocused())
				*Focus = Camera;
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			Selection.Add(Light);
			*Flags |= LC_SEL_SELECTED;

			if (Light->IsFocused())
				*Focus = Light;
		}
	}
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
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
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

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
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

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection)
{
	ClearSelectionAndSetFocus(ObjectSection.Object, ObjectSection.Section);
}

void lcModel::SetSelectionAndFocus(const lcArray<lcObject*>& Selection, lcObject* Focus, lcuint32 Section)
{
	ClearSelection(false);

	if (Focus)
	{
		Focus->SetFocused(Section, true);

		if (Focus->IsPiece())
			SelectGroup(((lcPiece*)Focus)->GetTopGroup(), true);
	}

	AddToSelection(Selection);
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

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::SelectAllPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(true);
	}

	gMainWindow->UpdateSelectedObjects(true);
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

	gMainWindow->UpdateSelectedObjects(true);
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

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
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

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::UnhideSelectedPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
			Piece->SetHidden(false);
	}

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::UnhideAllPieces()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		mPieces[PieceIdx]->SetHidden(false);

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
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

void lcModel::InsertPieceToolClicked(const lcMatrix44& WorldMatrix)
{
	lcPiece* Piece = new lcPiece(gMainWindow->mPreviewWidget->GetCurrentPiece());
	Piece->Initialize(WorldMatrix, mCurrentStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	Piece->UpdatePosition(mCurrentStep);
	AddPiece(Piece);

	gMainWindow->UpdateTimeline(false, false);
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

	gMainWindow->UpdateSelectedObjects(false);
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

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateMoveTool(const lcVector3& Distance, bool AlternateButtonDrag)
{
	lcVector3 PieceDistance = LockVector(SnapPosition(Distance) - SnapPosition(mMouseToolDistance));
	lcVector3 ObjectDistance = Distance - mMouseToolDistance;

	MoveSelectedObjects(PieceDistance, ObjectDistance, true, AlternateButtonDrag, true, false);
	mMouseToolDistance = Distance;

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateRotateTool(const lcVector3& Angles, bool AlternateButtonDrag)
{
	lcVector3 Delta = LockVector(SnapRotation(Angles) - SnapRotation(mMouseToolDistance));
	RotateSelectedPieces(Delta, true, AlternateButtonDrag, false, false);
	mMouseToolDistance = Angles;

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateScaleTool(const float Scale)
{
	ScaleSelectedPieces(Scale, true, false);

	gMainWindow->UpdateSelectedObjects(false);
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
			const lcArray<View*>* Views = gMainWindow->GetViewsForModel(this);
			for (int ViewIdx = 0; ViewIdx < Views->GetSize(); ViewIdx++)
			{
				View* View = (*Views)[ViewIdx];
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
	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
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
		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, true);
	}
}

void lcModel::UpdateZoomTool(lcCamera* Camera, float Mouse)
{
	Camera->Zoom(Mouse - mMouseToolDistance.x, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdatePanTool(lcCamera* Camera, const lcVector3& Distance)
{
	Camera->Pan(Distance - mMouseToolDistance, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance = Distance;
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

void lcModel::ZoomRegionToolClicked(lcCamera* Camera, float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners)
{
	Camera->ZoomRegion(AspectRatio, Position, TargetPosition, Corners, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Zoom"));
}

void lcModel::LookAt(lcCamera* Camera)
{
	lcVector3 Center;

	if (!GetSelectionCenter(Center))
	{
		lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		if (GetPiecesBoundingBox(Min, Max))
			Center = (Min + Max) / 2.0f;
		else
			Center = lcVector3(0.0f, 0.0f, 0.0f);
	}

	Camera->Center(Center, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Look At"));
}

void lcModel::ZoomExtents(lcCamera* Camera, float Aspect)
{
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	if (!GetPiecesBoundingBox(Min, Max))
		return;

	lcVector3 Center = (Min + Max) / 2.0f;

	lcVector3 Points[8];
	lcGetBoxCorners(Min, Max, Points);

	Camera->ZoomExtents(Aspect, Center, Points, 8, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Zoom"));
}

void lcModel::Zoom(lcCamera* Camera, float Amount)
{
	Camera->Zoom(Amount, mCurrentStep, gMainWindow->GetAddKeys());
	gMainWindow->UpdateSelectedObjects(false);
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
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to select."));
		return;
	}

	lcSelectDialogOptions Options;
	if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
		return;

	SetSelectionAndFocus(Options.Objects, NULL, 0);
}

void lcModel::ShowArrayDialog()
{
	lcVector3 Center;

	if (!GetPieceFocusOrSelectionCenter(Center))
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No pieces selected."));
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
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Array only has 1 element or less, no pieces added."));
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
					ModelWorld.SetTranslation(Position + Offset);

					lcPiece* NewPiece = new lcPiece(Piece->mPieceInfo);
					NewPiece->Initialize(ModelWorld, mCurrentStep);
					NewPiece->SetColorIndex(Piece->mColorIndex);

					NewPieces.Add(NewPiece);
				}
			}
		}
	}

	for (int PieceIdx = 0; PieceIdx < NewPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = (lcPiece*)NewPieces[PieceIdx];
		Piece->UpdatePosition(mCurrentStep);
		AddPiece(Piece);
	}

	AddToSelection(NewPieces);
	gMainWindow->UpdateTimeline(false, false);
	SaveCheckpoint(tr("Array"));
}

void lcModel::ShowMinifigDialog()
{
	lcMinifig* Minifig = new lcMinifig();

	if (!gMainWindow->DoDialog(LC_DIALOG_MINIFIG, &Minifig))
		return;

	gMainWindow->mPreviewWidget->MakeCurrent();

	lcGroup* Group = AddGroup(tr("Minifig #"), NULL);
	lcArray<lcObject*> Pieces(LC_MFW_NUMITEMS);

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (Minifig->Parts[PartIdx] == NULL)
			continue;

		lcPiece* Piece = new lcPiece(Minifig->Parts[PartIdx]);

		Piece->Initialize(Minifig->Matrices[PartIdx], mCurrentStep);
		Piece->SetColorIndex(Minifig->Colors[PartIdx]);
		Piece->SetGroup(Group);
		AddPiece(Piece);
		Piece->UpdatePosition(mCurrentStep);

		Pieces.Add(Piece);
	}

	SetSelectionAndFocus(Pieces, NULL, 0);
	gMainWindow->UpdateTimeline(false, false);
	SaveCheckpoint(tr("Minifig"));
}

void lcModel::UpdateInterface()
{
	gMainWindow->UpdateTimeline(true, false);
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
	gMainWindow->UpdatePaste(!g_App->mClipboard.isEmpty());
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle();
	gMainWindow->SetTool(gMainWindow->GetTool());

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->SetTransformType(gMainWindow->GetTransformType());
	gMainWindow->UpdateLockSnap();
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateModels();
	gMainWindow->UpdatePerspective();
	gMainWindow->UpdateCurrentStep();
}
