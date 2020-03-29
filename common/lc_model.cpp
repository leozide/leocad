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
#include "lc_synth.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "view.h"
#include "minifig.h"
#include "lc_qarraydialog.h"
#include "lc_qselectdialog.h"
#include "lc_qminifigdialog.h"
#include "lc_qgroupdialog.h"
#include "lc_qeditgroupsdialog.h"
#include "lc_selectbycolordialog.h"
#include "lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_lxf.h"
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtConcurrent>
#endif

void lcModelProperties::LoadDefaults()
{
	mAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);

	mBackgroundType = (lcBackgroundType)lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TYPE);
	mBackgroundSolidColor = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR));
	mBackgroundGradientColor1 = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1));
	mBackgroundGradientColor2 = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2));
	mBackgroundImage = lcGetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE);
	mBackgroundImageTile = lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_TILE);

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
		for (const QString& Comment : Comments)
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
			{
				mBackgroundImage = Stream.readLine().trimmed();
				mBackgroundType = LC_BACKGROUND_IMAGE;
			}
		}
	}
}

lcModel::lcModel(const QString& Name)
{
	mProperties.mName = Name;
	mProperties.LoadDefaults();

	mActive = false;
	mCurrentStep = 1;
	mBackgroundTexture = nullptr;
	mPieceInfo = nullptr;
}

lcModel::~lcModel()
{
	if (mPieceInfo)
	{
		if (gMainWindow && gMainWindow->GetCurrentPieceInfo() == mPieceInfo)
			gMainWindow->SetCurrentPieceInfo(nullptr);

		if (mPieceInfo->GetModel() == this)
			mPieceInfo->SetPlaceholder();

		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		Library->ReleasePieceInfo(mPieceInfo);
	}

	DeleteModel();
	DeleteHistory();
}

bool lcModel::GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& ParentWorldMatrix) const
{
	for (lcPiece* ModelPiece : mPieces)
	{
		if (ModelPiece == Piece)
		{
			ParentWorldMatrix = lcMul(ModelPiece->mModelWorld, ParentWorldMatrix);
			return true;
		}

		PieceInfo* Info = ModelPiece->mPieceInfo;

		if (Info->IsModel())
		{
			lcMatrix44 WorldMatrix = lcMul(ModelPiece->mModelWorld, ParentWorldMatrix);

			if (Info->GetPieceWorldMatrix(Piece, WorldMatrix))
			{
				ParentWorldMatrix = WorldMatrix;
				return true;
			}
		}
	}

	return false;
}

bool lcModel::IncludesModel(const lcModel* Model) const
{
	if (Model == this)
		return true;

	for (lcPiece* Piece : mPieces)
		if (Piece->mPieceInfo->IncludesModel(Model))
			return true;

	return false;
}

void lcModel::DeleteHistory()
{
	for (lcModelHistoryEntry* Entry : mUndoHistory)
		delete Entry;
	mUndoHistory.clear();
	for (lcModelHistoryEntry* Entry : mRedoHistory)
		delete Entry;
	mRedoHistory.clear();
}

void lcModel::DeleteModel()
{
	lcReleaseTexture(mBackgroundTexture);
	mBackgroundTexture = nullptr;

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
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	mPieceInfo = Library->FindPiece(mProperties.mName.toLatin1().constData(), Project, true, false);
	mPieceInfo->SetModel(this, true, Project, true);
	Library->LoadPieceInfo(mPieceInfo, true, true);
}

void lcModel::UpdateMesh()
{
	mPieceInfo->SetModel(this, true, nullptr, false);
}

void lcModel::UpdatePieceInfo(std::vector<lcModel*>& UpdatedModels)
{
	if (std::find(UpdatedModels.begin(), UpdatedModels.end(), this) != UpdatedModels.end())
		return;

	mPieceInfo->SetModel(this, false, nullptr, false);
	UpdatedModels.push_back(this);

	lcMesh* Mesh = mPieceInfo->GetMesh();

	if (mPieces.IsEmpty() && !Mesh)
	{
		mPieceInfo->SetBoundingBox(lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 0.0f));
		return;
	}

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsVisibleInSubModel())
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

	lcArray<lcGroup*> CurrentGroups;
	lcStep Step = 1;
	int CurrentLine = 0;
	int AddedSteps = 0;

	for (lcPiece* Piece : mPieces)
	{
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
			{
				Stream << mFileLines[CurrentLine];
				if (AddedSteps > 0)
					AddedSteps--;
			}
			CurrentLine++;
		}

		while (Piece->GetStepShow() > Step)
		{
			Stream << QLatin1String("0 STEP\r\n");
			AddedSteps++;
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

		if (Piece->mPieceInfo->GetSynthInfo())
		{
			Stream << QLatin1String("0 !LEOCAD SYNTH BEGIN\r\n");

			const lcArray<lcPieceControlPoint>& ControlPoints = Piece->GetControlPoints();
			for (int ControlPointIdx = 0; ControlPointIdx < ControlPoints.GetSize(); ControlPointIdx++)
			{
				const lcPieceControlPoint& ControlPoint = ControlPoints[ControlPointIdx];

				Stream << QLatin1String("0 !LEOCAD SYNTH CONTROL_POINT");

				const float* FloatMatrix = ControlPoint.Transform;
				float Numbers[13] = { FloatMatrix[12], -FloatMatrix[14], FloatMatrix[13], FloatMatrix[0], -FloatMatrix[8], FloatMatrix[4], -FloatMatrix[2], FloatMatrix[10], -FloatMatrix[6], FloatMatrix[1], -FloatMatrix[9], FloatMatrix[5], ControlPoint.Scale };

				for (int NumberIdx = 0; NumberIdx < 13; NumberIdx++)
					Stream << ' ' << lcFormatValue(Numbers[NumberIdx], NumberIdx < 3 ? 4 : 6);

				Stream << LineEnding;
			}
		}

		Piece->SaveLDraw(Stream);

		if (Piece->mPieceInfo->GetSynthInfo())
			Stream << QLatin1String("0 !LEOCAD SYNTH END\r\n");
	}

	while (CurrentLine < mFileLines.size())
	{
		QString Line = mFileLines[CurrentLine];
		QTextStream LineStream(&Line, QIODevice::ReadOnly);

		QString Token;
		LineStream >> Token;
		bool Skip = false;

		if (Token == QLatin1String("0"))
		{
			LineStream >> Token;

			if (Token == QLatin1String("STEP") && AddedSteps-- > 0)
				Skip = true;
		}

		if (!Skip)
			Stream << mFileLines[CurrentLine];
		CurrentLine++;
	}

	while (CurrentGroups.GetSize())
	{
		CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
		Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
	}

	for (lcCamera* Camera : mCameras)
		if (!SelectedOnly || Camera->IsSelected())
			Camera->SaveLDraw(Stream);

	for (lcLight* Light : mLights)
		if (!SelectedOnly || Light->IsSelected())
			Light->SaveLDraw(Stream);

	Stream.flush();
}

int lcModel::SplitMPD(QIODevice& Device)
{
	qint64 ModelPos = Device.pos();

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
				ModelPos = Pos;
			}
			else if (Token == QLatin1String("NOFILE"))
			{
				break;
			}
		}
	}

	return ModelPos;
}

void lcModel::LoadLDraw(QIODevice& Device, Project* Project)
{
	lcPiece* Piece = nullptr;
	lcCamera* Camera = nullptr;
	lcLight* Light = nullptr;
	lcArray<lcGroup*> CurrentGroups;
	lcArray<lcPieceControlPoint> ControlPoints;
	int CurrentStep = 1;
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	mProperties.mAuthor.clear();
	mProperties.mDescription.clear();
	mProperties.mComments.clear();

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
				QString Name = LineStream.readAll().trimmed();

				if (mProperties.mName != Name)
				{
					Device.seek(Pos);
					break;
				}

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
					Piece = new lcPiece(nullptr);

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
					Camera = nullptr;
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
					lcGroup* Group = GetGroup(Name, true);
					if (!CurrentGroups.IsEmpty())
						Group->mGroup = CurrentGroups[CurrentGroups.GetSize() - 1];
					else
						Group->mGroup = nullptr;
					CurrentGroups.Add(Group);
				}
				else if (Token == QLatin1String("END"))
				{
					if (!CurrentGroups.IsEmpty())
						CurrentGroups.RemoveIndex(CurrentGroups.GetSize() - 1);
				}
			}
			else if (Token == QLatin1String("SYNTH"))
			{
				LineStream >> Token;

				if (Token == QLatin1String("BEGIN"))
				{
					ControlPoints.RemoveAll();
				}
				else if (Token == QLatin1String("END"))
				{
					ControlPoints.RemoveAll();
				}
				else if (Token == QLatin1String("CONTROL_POINT"))
				{
					float Numbers[13];
					for (int TokenIdx = 0; TokenIdx < 13; TokenIdx++)
						LineStream >> Numbers[TokenIdx];

					lcPieceControlPoint& PieceControlPoint = ControlPoints.Add();
					PieceControlPoint.Transform = lcMatrix44(lcVector4(Numbers[3], Numbers[9], -Numbers[6], 0.0f), lcVector4(Numbers[5], Numbers[11], -Numbers[8], 0.0f),
					                                         lcVector4(-Numbers[4], -Numbers[10], Numbers[7], 0.0f), lcVector4(Numbers[0], Numbers[2], -Numbers[1], 1.0f));
					PieceControlPoint.Scale = Numbers[12];
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

			QString PartId = LineStream.readAll().trimmed();

			if (PartId.isEmpty())
				continue;

			QByteArray CleanId = PartId.toLatin1().toUpper().replace('\\', '/');

			if (Library->IsPrimitive(CleanId.constData()))
			{
				mFileLines.append(OriginalLine); 
			}
			else
			{
				if (!Piece)
					Piece = new lcPiece(nullptr);

				if (!CurrentGroups.IsEmpty())
					Piece->SetGroup(CurrentGroups[CurrentGroups.GetSize() - 1]);

				PieceInfo* Info = Library->FindPiece(PartId.toLatin1().constData(), Project, true, true);

				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
									 lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(Matrix[12], Matrix[14], -Matrix[13], 1.0f));

				Piece->SetFileLine(mFileLines.size());
				Piece->SetPieceInfo(Info, PartId, false);
				Piece->Initialize(Transform, CurrentStep);
				Piece->SetColorCode(ColorCode);
				Piece->VerifyControlPoints(ControlPoints);
				Piece->SetControlPoints(ControlPoints);
				AddPiece(Piece);
				Piece = nullptr;
			}
		}
		else
			mFileLines.append(OriginalLine); 
	}

	mCurrentStep = CurrentStep;
	CalculateStep(mCurrentStep);
	UpdateBackgroundTexture();
	Library->WaitForLoadQueue();
	Library->mBuffersDirty = true;
	Library->UnloadUnusedParts();

	delete Piece;
	delete Camera;
	delete Light;
}

bool lcModel::LoadBinary(lcFile* file)
{
	qint32 i, count;
	char id[32];
	quint32 rgb;
	float fv = 0.4f;
	quint8 ch;
	quint16 sh;

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
			lcPiece* pPiece = new lcPiece(nullptr);
			pPiece->FileLoad(*file);
			AddPiece(pPiece);
		}
		else
		{
			char name[LC_PIECE_NAME_LEN];
			lcVector3 pos, rot;
			quint8 color, step, group;

			file->ReadFloats(pos, 3);
			file->ReadFloats(rot, 3);
			file->ReadU8(&color, 1);
			file->ReadBuffer(name, 9);
			strcat(name, ".dat");
			file->ReadU8(&step, 1);
			file->ReadU8(&group, 1);

			pos *= 25.0f;
			lcMatrix44 WorldMatrix = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			WorldMatrix.SetTranslation(pos);

			PieceInfo* pInfo = Library->FindPiece(name, nullptr, true, false);
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
			QByteArray Author;
			Author.resize(sh + 1);
			file->ReadBuffer(Author.data(), sh);
			Author[sh] = 0;
			mProperties.mAuthor = QString::fromUtf8(Author);
		}

		file->ReadBuffer(&ch, 1);
		if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
		if (sh > 100)
			file->Seek(sh, SEEK_CUR);
		else
		{
			QByteArray Description;
			Description.resize(sh + 1);
			file->ReadBuffer(Description.data(), sh);
			Description[sh] = 0;
			mProperties.mDescription = QString::fromUtf8(Description);
		}

		file->ReadBuffer(&ch, 1);
		if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
		if (sh > 255)
			file->Seek(sh, SEEK_CUR);
		else
		{
			QByteArray Comments;
			Comments.resize(sh + 1);
			file->ReadBuffer(Comments.data(), sh);
			Comments[sh] = 0;
			mProperties.mComments = QString::fromUtf8(Comments);
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

			i = (qint32)(quintptr)(Group->mGroup);
			Group->mGroup = nullptr;

			if (i > 0xFFFF || i == -1)
				continue;

			Group->mGroup = mGroups[NumGroups + i];
		}

		for (int PieceIdx = FirstNewPiece; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			i = (qint32)(quintptr)(Piece->GetGroup());
			Piece->SetGroup(nullptr);

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
			for (lcCamera* Camera : mCameras)
				Camera->FileLoad(*file);
		}
	}

	if (fv >= 0.7f)
	{
		file->Seek(24, SEEK_CUR);

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

bool lcModel::LoadLDD(const QString& FileData)
{
	lcArray<lcPiece*> Pieces;
	lcArray<lcArray<lcPiece*>> Groups;
	
	if (!lcImportLXFMLFile(FileData, Pieces, Groups))
		return false;

	for (lcPiece* Piece : Pieces)
		AddPiece(Piece);

	for (const lcArray<lcPiece*>& Group : Groups)
	{
		lcGroup* NewGroup = AddGroup(tr("Group #"), nullptr);
		for (lcPiece* Piece : Group)
			Piece->SetGroup(NewGroup);
	}

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	CalculateStep(mCurrentStep);
	Library->WaitForLoadQueue();
	Library->mBuffersDirty = true;
	Library->UnloadUnusedParts();

	return true;
}

bool lcModel::LoadInventory(const QByteArray& Inventory)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	QJsonDocument Document = QJsonDocument::fromJson(Inventory);
	QJsonObject Root = Document.object();
	QJsonArray Parts = Root["results"].toArray();
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	for (const QJsonValue& Part : Parts)
	{
		QJsonObject PartObject = Part.toObject();
		QByteArray PartID = PartObject["part"].toObject()["part_num"].toString().toLatin1();
		QJsonArray PartIDArray = PartObject["part"].toObject()["external_ids"].toObject()["LDraw"].toArray();
		if (!PartIDArray.isEmpty())
			PartID = PartIDArray.first().toString().toLatin1();
		int Quantity = PartObject["quantity"].toInt();
		int ColorCode = 16;
		QJsonArray ColorArray = PartObject["color"].toObject()["external_ids"].toObject()["LDraw"].toObject()["ext_ids"].toArray();
		if (!ColorArray.isEmpty())
			ColorCode = ColorArray.first().toInt();

		PieceInfo* Info = Library->FindPiece(PartID + ".dat", nullptr, true, false);

		while (Quantity--)
		{
			lcPiece* Piece = new lcPiece(nullptr);
			Piece->SetPieceInfo(Info, QString(), false);
			Piece->Initialize(lcMatrix44Identity(), 1);
			Piece->SetColorCode(ColorCode);
			AddPiece(Piece);
		}
	}

	if (mPieces.IsEmpty())
		return false;

	Library->WaitForLoadQueue();
	Library->mBuffersDirty = true;
	Library->UnloadUnusedParts();

	auto RoundBounds = [](float& Value)
	{
		Value = ((Value < 0.0f) ? floor((Value - 5.0f) / 10.0f) : ceil((Value + 5.0f) / 10.0f)) * 10.0f;
	};

	const float TargetHeight = 800.0f;
	float CurrentX = 0.0f;
	float CurrentY = 0.0f;
	float ColumnWidth = 0.0f;

	for (lcPiece* Piece : mPieces)
	{
		lcBoundingBox BoundingBox = Piece->mPieceInfo->GetBoundingBox();
		RoundBounds(BoundingBox.Min.x);
		RoundBounds(BoundingBox.Min.y);
		RoundBounds(BoundingBox.Max.x);
		RoundBounds(BoundingBox.Max.y);

		float PieceWidth = BoundingBox.Max.x - BoundingBox.Min.x;
		float PieceHeight = BoundingBox.Max.y - BoundingBox.Min.y;

		if (CurrentY + PieceHeight > TargetHeight)
		{
			CurrentY = 0.0f;
			CurrentX += ColumnWidth;
			ColumnWidth = 0.0f;
		}

		Piece->SetPosition(lcVector3(CurrentX + PieceWidth / 2.0f, CurrentY + PieceHeight / 2.0f, 0.0f), 1, false);
		CurrentY += PieceHeight;
		ColumnWidth = qMax(ColumnWidth, PieceWidth);
	}

	CalculateStep(mCurrentStep);

	return true;
#else
	return false;
#endif
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
		SaveCheckpoint(tr("Cutting"));
	}
}

void lcModel::Copy()
{
	QByteArray File;
	QTextStream Stream(&File, QIODevice::WriteOnly);

	SaveLDraw(Stream, true);

	gApplication->ExportClipboard(File);
}

void lcModel::Paste()
{
	if (gApplication->mClipboard.isEmpty())
		return;

	lcModel* Model = new lcModel(QString());

	QBuffer Buffer(&gApplication->mClipboard);
	Buffer.open(QIODevice::ReadOnly);
	Model->LoadLDraw(Buffer, lcGetActiveProject());

	const lcArray<lcPiece*>& PastedPieces = Model->mPieces;
	lcArray<lcObject*> SelectedObjects;
	SelectedObjects.AllocGrow(PastedPieces.GetSize());

	for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = PastedPieces[PieceIdx];
		lcStep Step = Piece->GetStepShow();

		Piece->SetFileLine(-1);
		if (Step > mCurrentStep)
			Piece->SetStepShow(mCurrentStep);

		SelectedObjects.Add(Piece);
	}

	Merge(Model);
	SaveCheckpoint(tr("Pasting"));

	if (SelectedObjects.GetSize() == 1)
		ClearSelectionAndSetFocus(SelectedObjects[0], LC_PIECE_SECTION_POSITION, false);
	else
		SetSelectionAndFocus(SelectedObjects, nullptr, 0, false);

	CalculateStep(mCurrentStep);
	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateAllViews();
}

void lcModel::DuplicateSelectedPieces()
{
	lcArray<lcObject*> NewPieces;
	lcPiece* Focus = nullptr;
	std::map<lcGroup*, lcGroup*> GroupMap;

	std::function<lcGroup*(lcGroup*)> GetNewGroup = [this, &GroupMap, &GetNewGroup](lcGroup* Group)
	{
		const auto GroupIt = GroupMap.find(Group);
		if (GroupIt != GroupMap.end())
			return GroupIt->second;
		else
		{
			lcGroup* Parent = Group->mGroup ? GetNewGroup(Group->mGroup) : nullptr;
			QString GroupName = Group->mName;

			while (!GroupName.isEmpty())
			{
				QCharRef Last = GroupName[GroupName.size() - 1];
				if (Last.isDigit())
					GroupName.chop(1);
				else
					break;
			}

			if (GroupName.isEmpty())
				GroupName = Group->mName;

			lcGroup* NewGroup = AddGroup(GroupName, Parent);
			GroupMap[Group] = NewGroup;
			return NewGroup;
		}
	};

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected())
			continue;

		lcPiece* NewPiece = new lcPiece(*Piece);
		NewPiece->UpdatePosition(mCurrentStep);
		NewPieces.Add(NewPiece);

		if (Piece->IsFocused())
			Focus = NewPiece;

		PieceIdx++;
		InsertPiece(NewPiece, PieceIdx);

		lcGroup* Group = Piece->GetGroup();
		if (Group)
			Piece->SetGroup(GetNewGroup(Group));
	}

	if (NewPieces.IsEmpty())
		return;

	gMainWindow->UpdateTimeline(false, false);
	SetSelectionAndFocus(NewPieces, Focus, LC_PIECE_SECTION_POSITION, false);
	SaveCheckpoint(tr("Duplicating Pieces"));
}

void lcModel::GetScene(lcScene& Scene, lcCamera* ViewCamera, bool AllowHighlight, bool AllowFade) const
{
	mPieceInfo->AddRenderMesh(Scene);

	for (const lcPiece* Piece : mPieces)
	{
		if (Piece->IsVisible(mCurrentStep))
		{
			lcStep StepShow = Piece->GetStepShow();
			Piece->AddMainModelRenderMeshes(Scene, AllowHighlight && StepShow == mCurrentStep, AllowFade && StepShow < mCurrentStep);
		}
	}

	if (Scene.GetDrawInterface() && !Scene.GetActiveSubmodelInstance())
	{
		for (lcCamera* Camera : mCameras)
			if (Camera != ViewCamera && Camera->IsVisible())
				Scene.AddInterfaceObject(Camera);

		for (lcLight* Light : mLights)
			if (Light->IsVisible())
				Scene.AddInterfaceObject(Light);
	}
}

void lcModel::AddSubModelRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisibleInSubModel())
			Piece->AddSubModelRenderMeshes(Scene, WorldMatrix, DefaultColorIndex, RenderMeshState, ParentActive);
}

void lcModel::DrawBackground(lcGLWidget* Widget)
{
	if (mProperties.mBackgroundType == LC_BACKGROUND_SOLID)
	{
		glClearColor(mProperties.mBackgroundSolidColor[0], mProperties.mBackgroundSolidColor[1], mProperties.mBackgroundSolidColor[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	lcContext* Context = Widget->mContext;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Context->SetDepthWrite(false);
	glDisable(GL_DEPTH_TEST);

	float ViewWidth = (float)Widget->mWidth;
	float ViewHeight = (float)Widget->mHeight;

	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));

	if (mProperties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		Context->SetSmoothShading(true);

		const lcVector3& Color1 = mProperties.mBackgroundGradientColor1;
		const lcVector3& Color2 = mProperties.mBackgroundGradientColor2;

		float Verts[] =
		{
			ViewWidth, ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      0.0f,       Color2[0], Color2[1], Color2[2], 1.0f,
			ViewWidth, 0.0f,       Color2[0], Color2[1], Color2[2], 1.0f
		};

		Context->SetMaterial(lcMaterialType::UnlitVertexColor);
		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormat(0, 2, 0, 0, 4, false);

		Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

		Context->SetSmoothShading(false);
	}
	else if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		Context->BindTexture2D(mBackgroundTexture->mTexture);

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

		Context->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Context->SetMaterial(lcMaterialType::UnlitTextureDecal);
		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormat(0, 2, 0, 2, 0, false);

		Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);
	}

	glEnable(GL_DEPTH_TEST);
	Context->SetDepthWrite(true);
}

QImage lcModel::GetStepImage(bool Zoom, bool Highlight, int Width, int Height, lcStep Step)
{
	View* ActiveView = gMainWindow->GetActiveView();
	ActiveView->MakeCurrent();
	lcContext* Context = ActiveView->mContext;

	lcStep CurrentStep = mCurrentStep;

	lcCamera* Camera = ActiveView->mCamera;
	if (Zoom)
		ZoomExtents(Camera, (float)Width / (float)Height);

	View View(this);
	View.SetHighlight(Highlight);
	View.SetCamera(Camera, false);
	View.SetContext(Context);

	if (!View.BeginRenderToImage(Width, Height))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return QImage();
	}

	SetTemporaryStep(Step);
	View.OnDraw();

	QImage Image = View.GetRenderImage();

	View.EndRenderToImage();
	Context->ClearResources();

	SetTemporaryStep(CurrentStep);

	if (!mActive)
		CalculateStep(LC_STEP_MAX);

	return Image;
}

QImage lcModel::GetPartsListImage(int MaxWidth, lcStep Step) const
{
	lcPartsList PartsList;

	if (Step == 0)
		GetPartsList(gDefaultColor, true, false, PartsList);
	else
		GetPartsListForStep(Step, gDefaultColor, PartsList);

	if (PartsList.empty())
		return QImage();

	struct lcPartsListImage
	{
		QImage Thumbnail;
		const PieceInfo* Info;
		int ColorIndex;
		int Count;
		QRect Bounds;
		QPoint Position;
	};

	std::vector<lcPartsListImage> Images;

	for (const auto& PartIt : PartsList)
	{
		for (const auto& ColorIt : PartIt.second)
		{
			Images.push_back(lcPartsListImage());
			lcPartsListImage& Image = Images.back();
			Image.Info = PartIt.first;
			Image.ColorIndex = ColorIt.first;
			Image.Count = ColorIt.second;
		}
	}

	auto ImageCompare = [](const lcPartsListImage& Image1, const lcPartsListImage& Image2)
	{
		if (Image1.ColorIndex != Image2.ColorIndex)
			return Image1.ColorIndex < Image2.ColorIndex;

		return strcmp(Image1.Info->m_strDescription, Image2.Info->m_strDescription) < 0;
	};

	std::sort(Images.begin(), Images.end(), ImageCompare);

	View* View = gMainWindow->GetActiveView();
	View->MakeCurrent();
	lcContext* Context = View->mContext;
	const int ThumbnailSize = qMin(MaxWidth, 512);

	std::pair<lcFramebuffer, lcFramebuffer> RenderFramebuffer = Context->CreateRenderFramebuffer(ThumbnailSize, ThumbnailSize);

	if (!RenderFramebuffer.first.IsValid())
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return QImage();
	}

	Context->BindFramebuffer(RenderFramebuffer.first);

	float OrthoSize = 200.0f;

	lcMatrix44 ProjectionMatrix = lcMatrix44Ortho(-OrthoSize, OrthoSize, -OrthoSize, OrthoSize, -5000.0f, 5000.0f);
	lcMatrix44 ViewMatrix = lcMatrix44LookAt(lcVector3(-100.0f, -100.0f, 75.0f), lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f));
	const int Viewport[4] = { 0, 0, ThumbnailSize, ThumbnailSize };

	float ExtraPixels = 0.0f;

	for (lcPartsListImage& Image : Images)
	{
		const PieceInfo* Info = Image.Info;
		const lcBoundingBox& BoundingBox = Info->GetBoundingBox();

		lcVector3 Points[8];
		lcGetBoxCorners(BoundingBox.Min, BoundingBox.Max, Points);

		for (lcVector3& Point : Points)
		{
			Point = lcProjectPoint(Point, ViewMatrix, ProjectionMatrix, Viewport);

			ExtraPixels = qMax(ExtraPixels, -Point.x);
			ExtraPixels = qMax(ExtraPixels, Point.x - ThumbnailSize);
			ExtraPixels = qMax(ExtraPixels, -Point.y);
			ExtraPixels = qMax(ExtraPixels, Point.y - ThumbnailSize);
		}
	}

	if (ExtraPixels)
	{
		OrthoSize += ExtraPixels * (2.0f * OrthoSize / ThumbnailSize);
		ProjectionMatrix = lcMatrix44Ortho(-OrthoSize, OrthoSize, -OrthoSize, OrthoSize, -5000.0f, 5000.0f);
	}

	Context->SetViewport(0, 0, ThumbnailSize, ThumbnailSize);
	Context->SetDefaultState();
	Context->SetProjectionMatrix(ProjectionMatrix);

	for (lcPartsListImage& Image : Images)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lcScene Scene;
		Scene.SetAllowWireframe(false);
		Scene.SetAllowLOD(false);
		Scene.Begin(ViewMatrix);

		Image.Info->AddRenderMeshes(Scene, lcMatrix44Identity(), Image.ColorIndex, lcRenderMeshState::Default, true);

		Scene.End();

		Scene.Draw(Context);

		Image.Thumbnail = Context->GetRenderFramebufferImage(RenderFramebuffer);
	}

	Context->ClearFramebuffer();
	Context->DestroyRenderFramebuffer(RenderFramebuffer);
	Context->ClearResources();

	auto CalculateImageBounds = [](lcPartsListImage& Image)
	{
		QImage& Thumbnail = Image.Thumbnail;
		int Width = Thumbnail.width();
		int Height = Thumbnail.height();

		int MinX = Width;
		int MinY = Height;
		int MaxX = 0;
		int MaxY = 0;

		for (int x = 0; x < Width; x++)
		{
			for (int y = 0; y < Height; y++)
			{
				if (qAlpha(Thumbnail.pixel(x, y)))
				{
					MinX = qMin(x, MinX);
					MinY = qMin(y, MinY);
					MaxX = qMax(x, MaxX);
					MaxY = qMax(y, MaxY);
				}
			}
		}

		Image.Bounds = QRect(QPoint(MinX, MinY), QPoint(MaxX, MaxY));
	};

	QtConcurrent::blockingMap(Images, CalculateImageBounds);

	QImage DummyImage(16, 16, QImage::Format_ARGB32);
	QPainter DummyPainter(&DummyImage);

	QFont Font("helvetica", 20, QFont::Bold);
	DummyPainter.setFont(Font);
	QFontMetrics FontMetrics = DummyPainter.fontMetrics();
	int Ascent = FontMetrics.ascent();

	int CurrentHeight = 0;
	int ImageWidth = MaxWidth;

	for (lcPartsListImage& Image : Images)
		CurrentHeight = qMax(Image.Bounds.height() + Ascent, CurrentHeight);

	for (;;)
	{
		int CurrentWidth = 0;
		int CurrentX = 0;
		int CurrentY = 0;
		int ColumnWidth = 0;
		int Spacing = 20;
		int NextHeightIncrease = INT_MAX;

		for (lcPartsListImage& Image : Images)
		{
			if (CurrentY + Image.Bounds.height() + Ascent > CurrentHeight)
			{
				int NeededSpace = Image.Bounds.height() + Ascent - (CurrentHeight - CurrentY);
				NextHeightIncrease = qMin(NeededSpace, NextHeightIncrease);

				CurrentY = 0;
				CurrentX += ColumnWidth + Spacing;
				ColumnWidth = 0;
			}

			Image.Position = QPoint(CurrentX, CurrentY);
			CurrentY += Image.Bounds.height() + Ascent + Spacing;
			CurrentWidth = qMax(CurrentWidth, CurrentX + Image.Bounds.width());
			ColumnWidth = qMax(ColumnWidth, Image.Bounds.width());
		}

		if (CurrentWidth < MaxWidth)
		{
			ImageWidth = CurrentWidth;
			break;
		}

		CurrentHeight += NextHeightIncrease;
	}

	QImage PainterImage(ImageWidth + 40, CurrentHeight + 40, QImage::Format_ARGB32);
	PainterImage.fill(QColor(255, 255, 255, 0));

	QPainter Painter(&PainterImage);
	Painter.setFont(Font);

	for (lcPartsListImage& Image : Images)
	{
		QPoint Position = Image.Position + QPoint(20, 20);
		Painter.drawImage(Position, Image.Thumbnail, Image.Bounds);
		Painter.drawText(QPoint(Position.x(), Position.y() + Image.Bounds.height() + Ascent), QString::number(Image.Count) + 'x');
	}

	Painter.end();

	return PainterImage;
}

void lcModel::SaveStepImages(const QString& BaseName, bool AddStepSuffix, bool Zoom, bool Highlight, int Width, int Height, lcStep Start, lcStep End)
{
	for (lcStep Step = Start; Step <= End; Step++)
	{
		QString FileName;

		if (AddStepSuffix)
			FileName = BaseName.arg(Step, 2, 10, QLatin1Char('0'));
		else
			FileName = BaseName;

		QImageWriter Writer(FileName);

		if (Writer.format().isEmpty())
			Writer.setFormat("png");

		QImage Image = GetStepImage(Zoom, Highlight, Width, Height, Step);
		if (!Writer.write(Image))
		{
			QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
			break;
		}
	}
}

std::vector<std::pair<lcModel*, lcStep>> lcModel::GetPageLayouts(std::vector<const lcModel*>& AddedModels)
{
	std::vector<std::pair<lcModel*, lcStep>> PageLayouts;

	if (std::find(AddedModels.begin(), AddedModels.end(), this) != AddedModels.end())
		return PageLayouts;

	AddedModels.push_back(this);

	std::map<lcStep, std::vector<lcPiece*>> StepPieces;

	for (lcPiece* Piece : mPieces)
		if (!Piece->IsHidden())
			StepPieces[Piece->GetStepShow()].push_back(Piece);

	lcStep CurrentStep = 1;

	for (const std::pair<lcStep, std::vector<lcPiece*>>& StepIt : StepPieces)
	{
		while (StepIt.first > CurrentStep)
		{
			PageLayouts.emplace_back(std::make_pair(this, CurrentStep));
			CurrentStep++;
		}

		for (lcPiece* Piece : StepIt.second)
		{
			if (Piece->mPieceInfo->IsModel())
			{
				lcModel* SubModel = Piece->mPieceInfo->GetModel();
				std::vector<std::pair<lcModel*, lcStep>> SubModelLayouts = SubModel->GetPageLayouts(AddedModels);
				PageLayouts.insert(PageLayouts.end(), std::make_move_iterator(SubModelLayouts.begin()), std::make_move_iterator(SubModelLayouts.end()));
			}
		}

		PageLayouts.emplace_back(std::make_pair(this, CurrentStep));
		CurrentStep++;
	}

	return PageLayouts;
}

void lcModel::UpdateBackgroundTexture()
{
	lcReleaseTexture(mBackgroundTexture);
	mBackgroundTexture = nullptr;

	if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		mBackgroundTexture = lcLoadTexture(mProperties.mBackgroundImage, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV);

		if (!mBackgroundTexture)
			mProperties.mBackgroundType = LC_BACKGROUND_SOLID;
	}
}

void lcModel::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisible(mCurrentStep) && (!ObjectRayTest.IgnoreSelected || !Piece->IsSelected()))
			Piece->RayTest(ObjectRayTest);

	if (ObjectRayTest.PiecesOnly)
		return;

	for (lcCamera* Camera : mCameras)
		if (Camera != ObjectRayTest.ViewCamera && Camera->IsVisible() && (!ObjectRayTest.IgnoreSelected || !Camera->IsSelected()))
			Camera->RayTest(ObjectRayTest);

	for (lcLight* Light : mLights)
		if (Light->IsVisible() && (!ObjectRayTest.IgnoreSelected || !Light->IsSelected()))
			Light->RayTest(ObjectRayTest);
}

void lcModel::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisible(mCurrentStep))
			Piece->BoxTest(ObjectBoxTest);

	for (lcCamera* Camera : mCameras)
		if (Camera != ObjectBoxTest.ViewCamera && Camera->IsVisible())
			Camera->BoxTest(ObjectBoxTest);

	for (lcLight* Light : mLights)
		if (Light->IsVisible())
			Light->BoxTest(ObjectBoxTest);
}

bool lcModel::SubModelMinIntersectDist(const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance) const
{
	bool MinIntersect = false;

	for (lcPiece* Piece : mPieces)
	{
		lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(Piece->mModelWorld);
		lcVector3 Start = lcMul31(WorldStart, InverseWorldMatrix);
		lcVector3 End = lcMul31(WorldEnd, InverseWorldMatrix);

		if (Piece->IsVisibleInSubModel() && Piece->mPieceInfo->MinIntersectDist(Start, End, MinDistance)) // todo: this should check for piece->mMesh first
			MinIntersect = true;
	}

	return MinIntersect;
}

bool lcModel::SubModelBoxTest(const lcVector4 Planes[6]) const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisibleInSubModel() && Piece->mPieceInfo->BoxTest(Piece->mModelWorld, Planes))
			return true;

	return false;
}

void lcModel::SaveCheckpoint(const QString& Description)
{
	lcModelHistoryEntry* ModelHistoryEntry = new lcModelHistoryEntry();

	ModelHistoryEntry->Description = Description;

	QTextStream Stream(&ModelHistoryEntry->File);
	SaveLDraw(Stream, false);

	mUndoHistory.insert(mUndoHistory.begin(), ModelHistoryEntry);
	for (lcModelHistoryEntry* Entry : mRedoHistory)
		delete Entry;
	mRedoHistory.clear();

	if (!Description.isEmpty())
	{
		gMainWindow->UpdateModified(IsModified());
		gMainWindow->UpdateUndoRedo(mUndoHistory.size() > 1 ? mUndoHistory[0]->Description : QString(), !mRedoHistory.empty() ? mRedoHistory[0]->Description : QString());
	}
}

void lcModel::LoadCheckPoint(lcModelHistoryEntry* CheckPoint)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	std::vector<PieceInfo*> LoadedInfos;

	for (lcPiece* Piece : mPieces)
	{
		PieceInfo* Info = Piece->mPieceInfo;
		Library->LoadPieceInfo(Info, true, true);
		LoadedInfos.push_back(Info);
	}

	DeleteModel();

	QBuffer Buffer(&CheckPoint->File);
	Buffer.open(QIODevice::ReadOnly);
	LoadLDraw(Buffer, lcGetActiveProject());

	gMainWindow->UpdateTimeline(true, false);
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();

	for (PieceInfo* Info : LoadedInfos)
		Library->ReleasePieceInfo(Info);
}

void lcModel::SetActive(bool Active)
{
	CalculateStep(Active ? mCurrentStep : LC_STEP_MAX);
	mActive = Active;
}

void lcModel::CalculateStep(lcStep Step)
{
	for (lcPiece* Piece : mPieces)
	{
		Piece->UpdatePosition(Step);

		if (Piece->IsSelected())
		{
			if (!Piece->IsVisible(Step))
				Piece->SetSelected(false);
			else
				SelectGroup(Piece->GetTopGroup(), true);
		}
	}

	for (lcCamera* Camera : mCameras)
		Camera->UpdatePosition(Step);

	for (lcLight* Light : mLights)
		Light->UpdatePosition(Step);
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

	for (lcPiece* Piece : mPieces)
		Step = lcMax(Step, Piece->GetStepShow());

	return Step;
}

void lcModel::InsertStep(lcStep Step)
{
	for (lcPiece* Piece : mPieces)
	{
		Piece->InsertTime(Step, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (lcCamera* Camera : mCameras)
		Camera->InsertTime(Step, 1);

	for (lcLight* Light : mLights)
		Light->InsertTime(Step, 1);

	SaveCheckpoint(tr("Inserting Step"));
	SetCurrentStep(mCurrentStep);
}

void lcModel::RemoveStep(lcStep Step)
{
	for (lcPiece* Piece : mPieces)
	{
		Piece->RemoveTime(Step, 1);
		if (Piece->IsSelected() && !Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(false);
	}

	for (lcCamera* Camera : mCameras)
		Camera->RemoveTime(Step, 1);

	for (lcLight* Light : mLights)
		Light->RemoveTime(Step, 1);

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
	for (lcGroup* Group : mGroups)
		if (Group->mName == Name)
			return Group;

	if (CreateIfMissing)
	{
		lcGroup* Group = new lcGroup();
		Group->mName = Name;
		mGroups.Add(Group);

		return Group;
	}

	return nullptr;
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

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcPiece* Piece : mPieces)
	{
		lcGroup* Group = Piece->GetGroup();

		if (SelectedGroups.FindIndex(Group) != -1)
			Piece->SetGroup(nullptr);
	}

	for (lcGroup* Group : mGroups)
		if (SelectedGroups.FindIndex(Group->mGroup) != -1)
			Group->mGroup = nullptr;

	SelectedGroups.DeleteAll();

	RemoveEmptyGroups();
	SaveCheckpoint(tr("Ungrouping"));
}

void lcModel::AddSelectedPiecesToGroup()
{
	lcGroup* Group = nullptr;

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsSelected())
		{
			Group = Piece->GetTopGroup();
			if (Group)
				break;
		}
	}

	if (Group)
	{
		for (lcPiece* Piece : mPieces)
		{
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
	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsFocused())
		{
			Piece->SetGroup(nullptr);
			break;
		}
	}

	RemoveEmptyGroups();
	SaveCheckpoint(tr("Ungrouping"));
}

void lcModel::ShowEditGroupsDialog()
{
	QMap<lcPiece*, lcGroup*> PieceParents;
	QMap<lcGroup*, lcGroup*> GroupParents;

	for (lcPiece* Piece : mPieces)
		PieceParents[Piece] = Piece->GetGroup();

	for (lcGroup* Group : mGroups)
		GroupParents[Group] = Group->mGroup;

	lcQEditGroupsDialog Dialog(gMainWindow, PieceParents, GroupParents, this);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	bool Modified = Dialog.mNewGroups.isEmpty();

	for (lcPiece* Piece : mPieces)
	{
		lcGroup* ParentGroup = Dialog.mPieceParents.value(Piece);

		if (ParentGroup != Piece->GetGroup())
		{
			Piece->SetGroup(ParentGroup);
			Modified = true;
		}
	}

	for (lcGroup* Group : mGroups)
	{
		lcGroup* ParentGroup = Dialog.mGroupParents.value(Group);

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

	for (lcGroup* Group : mGroups)
	{
		const QString& Name = Group->mName;

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

			for (lcPiece* Piece : mPieces)
				if (Piece->GetGroup() == Group)
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
				for (lcPiece* Piece : mPieces)
				{
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

lcVector3 lcModel::SnapPosition(const lcVector3& Distance) const
{
	lcVector3 NewDistance(Distance);

	float SnapXY = gMainWindow->GetMoveXYSnap();
	if (SnapXY != 0.0f)
	{
		int i = (int)(NewDistance[0] / SnapXY);
		float Leftover = NewDistance[0] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
			i++;
		else if (Leftover < -SnapXY / 2)
			i--;

		NewDistance[0] = SnapXY * i;

		i = (int)(NewDistance[1] / SnapXY);
		Leftover = NewDistance[1] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
			i++;
		else if (Leftover < -SnapXY / 2)
			i--;

		NewDistance[1] = SnapXY * i;
	}

	float SnapZ = gMainWindow->GetMoveZSnap();
	if (SnapZ != 0.0f)
	{
		int i = (int)(NewDistance[2] / SnapZ);
		float Leftover = NewDistance[2] - (SnapZ * i);

		if (Leftover > SnapZ / 2)
			i++;
		else if (Leftover < -SnapZ / 2)
			i--;

		NewDistance[2] = SnapZ * i;
	}

	return NewDistance;
}

lcVector3 lcModel::SnapRotation(const lcVector3& Angles) const
{
	float AngleSnap = gMainWindow->GetAngleSnap();
	lcVector3 NewAngles(Angles);

	if (AngleSnap != 0.0f)
	{
		int Snap[3];

		for (int i = 0; i < 3; i++)
			Snap[i] = (int)(Angles[i] / AngleSnap);

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
	PieceInfo* CurPiece = gMainWindow->GetCurrentPieceInfo();

	if (!CurPiece)
		return;

	lcPiece* Last = mPieces.IsEmpty() ? nullptr : mPieces[mPieces.GetSize() - 1];

	for (lcPiece* Piece : mPieces)
	{
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
	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION, false);

	SaveCheckpoint(tr("Adding Piece"));
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

		if (Mesh && Mesh->mVertexCacheOffset == -1)
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
	SaveCheckpoint(tr("Resetting Cameras"));
}

void lcModel::DeleteSelectedObjects()
{
	if (RemoveSelectedObjects())
	{
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
		SaveCheckpoint(tr("Deleting"));
	}
}

void lcModel::ResetSelectedPiecesPivotPoint()
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsSelected())
			Piece->ResetPivotPoint();

	gMainWindow->UpdateAllViews();
}

void lcModel::RemoveSelectedPiecesKeyFrames()
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsSelected())
			Piece->RemoveKeyFrames();

	for (lcCamera* Camera : mCameras)
		if (Camera->IsSelected())
			Camera->RemoveKeyFrames();

	for (lcLight* Light : mLights)
		if (Light->IsSelected())
			Light->RemoveKeyFrames();

	gMainWindow->UpdateAllViews();
	SaveCheckpoint(tr("Removing Key Frames"));
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
		SaveCheckpoint(tr("Modifying"));
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
		SaveCheckpoint(tr("Modifying"));
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

	SaveCheckpoint(tr("Modifying"));
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

	SaveCheckpoint(tr("Modifying"));
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
		SaveCheckpoint(tr("Modifying"));
		gMainWindow->UpdateAllViews();
		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(true);
	}
}

void lcModel::RenamePiece(PieceInfo* Info)
{
	for (lcPiece* Piece : mPieces)
		if (Piece->mPieceInfo == Info)
			Piece->UpdateID();
}

void lcModel::MoveSelectionToModel(lcModel* Model)
{
	if (!Model)
		return;

	lcArray<lcPiece*> Pieces;
	lcPiece* ModelPiece = nullptr;
	lcStep FirstStep = LC_STEP_MAX;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			mPieces.RemoveIndex(PieceIdx);
			Piece->SetGroup(nullptr); // todo: copy groups
			Pieces.Add(Piece);
			FirstStep = qMin(FirstStep, Piece->GetStepShow());

			if (!ModelPiece)
			{
				ModelPiece = new lcPiece(Model->mPieceInfo);
				ModelPiece->SetColorIndex(gDefaultColor);
				InsertPiece(ModelPiece, PieceIdx);
				PieceIdx++;
			}
		}
		else
			PieceIdx++;
	}

	lcVector3 ModelCenter = (Min + Max) / 2.0f;
	ModelCenter.z += (Min.z - Max.z) / 2.0f;

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];
		Piece->SetFileLine(-1);
		Piece->SetStepShow(Piece->GetStepShow() - FirstStep + 1);
		Piece->MoveSelected(Piece->GetStepShow(), false, -ModelCenter);
		Model->AddPiece(Piece);
	}

	std::vector<lcModel*> UpdatedModels;
	Model->UpdatePieceInfo(UpdatedModels);
	if (ModelPiece)
	{
		ModelPiece->Initialize(lcMatrix44Translation(ModelCenter), FirstStep);
		ModelPiece->UpdatePosition(mCurrentStep);
	}

	SaveCheckpoint(tr("New Model"));
	gMainWindow->UpdateTimeline(false, false);
	ClearSelectionAndSetFocus(ModelPiece, LC_PIECE_SECTION_POSITION, false);
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

		lcModel* Model = Piece->mPieceInfo->GetModel();

		for (const lcPiece* ModelPiece : Model->mPieces)
		{
			lcPiece* NewPiece = new lcPiece(nullptr);

			// todo: recreate in groups in the current model

			int ColorIndex = ModelPiece->mColorIndex;

			if (ColorIndex == gDefaultColor)
				ColorIndex = Piece->mColorIndex;

			NewPiece->SetPieceInfo(ModelPiece->mPieceInfo, ModelPiece->GetID(), true);
			NewPiece->Initialize(lcMul(ModelPiece->mModelWorld, Piece->mModelWorld), Piece->GetStepShow());
			NewPiece->SetColorIndex(ColorIndex);
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

	SaveCheckpoint(tr("Inlining"));
	gMainWindow->UpdateTimeline(false, false);
	SetSelectionAndFocus(NewPieces, nullptr, 0, false);
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
			for (lcPiece* Piece : mPieces)
			{
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
			for (lcPiece* Piece : mPieces)
			{
				if (Piece->IsSelected())
				{
					Piece->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedPieceDistance);
					Piece->UpdatePosition(mCurrentStep);
					Moved = true;
				}
			}
		}
	}

	if (ObjectDistance.LengthSquared() >= 0.001f && !AlternateButtonDrag)
	{
		lcVector3 TransformedObjectDistance = lcMul(ObjectDistance, RelativeRotation);

		for (lcCamera* Camera : mCameras)
		{
			if (Camera->IsSelected())
			{
				Camera->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance);
				Camera->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}

		for (lcLight* Light : mLights)
		{
			if (Light->IsSelected())
			{
				Light->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance);
				Light->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}
	}

	if (Moved && Update)
	{
		gMainWindow->UpdateAllViews();
		if (Checkpoint)
			SaveCheckpoint(tr("Moving"));
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

		for (lcPiece* Piece : mPieces)
		{
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
			SaveCheckpoint(tr("Rotating"));
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
	quint32 Section = Piece->GetFocusSection();

	if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST)
	{
		int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;
		Piece->SetControlPointScale(ControlPointIndex, Scale);

		if (Update)
		{
			gMainWindow->UpdateAllViews();
			if (Checkpoint)
				SaveCheckpoint(tr("Scaling"));
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

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsSelected() && Piece->mPieceInfo != Info)
		{
			lcPiecesLibrary* Library = lcGetPiecesLibrary();
			Library->ReleasePieceInfo(Piece->mPieceInfo);
			Piece->SetPieceInfo(Info, QString(), true);
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

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcPiece* Piece : mPieces)
	{
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
	for (lcPiece* Piece : mPieces)
		if (Piece->IsSelected())
			return true;

	return false;
}

bool lcModel::AnyObjectsSelected() const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsSelected())
			return true;

	for (lcCamera* Camera : mCameras)
		if (Camera->IsSelected())
			return true;

	for (lcLight* Light : mLights)
		if (Light->IsSelected())
			return true;

	return false;
}

lcObject* lcModel::GetFocusObject() const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsFocused())
			return Piece;

	for (lcCamera* Camera : mCameras)
		if (Camera->IsFocused())
			return Camera;

	for (lcLight* Light : mLights)
		if (Light->IsFocused())
			return Light;

	return nullptr;
}

lcModel* lcModel::GetFirstSelectedSubmodel() const
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsSelected() && Piece->mPieceInfo->IsModel())
			return Piece->mPieceInfo->GetModel();

	return nullptr;
}

void lcModel::GetSubModels(lcArray<lcModel*>& SubModels) const
{
	for (lcPiece* Piece : mPieces)
	{
		if (Piece->mPieceInfo->IsModel())
		{
			lcModel* SubModel = Piece->mPieceInfo->GetModel();
			if (SubModels.FindIndex(SubModel) == -1)
				SubModels.Add(SubModel);
		}
	}
}

bool lcModel::GetMoveRotateTransform(lcVector3& Center, lcMatrix33& RelativeRotation) const
{
	bool Relative = gMainWindow->GetRelativeTransform();
	int NumSelected = 0;

	Center = lcVector3(0.0f, 0.0f, 0.0f);
	RelativeRotation = lcMatrix33Identity();

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcCamera* Camera : mCameras)
	{
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

	for (lcLight* Light : mLights)
	{
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
	lcPiece* Selected = nullptr;
	int NumSelected = 0;

	for (lcPiece* Piece : mPieces)
	{
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
	lcPiece* SelectedPiece = nullptr;
	bool SinglePieceSelected = true;
	bool Selected = false;

	for (lcPiece* Piece : mPieces)
	{
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

	for (lcCamera* Camera : mCameras)
	{
		if (Camera->IsSelected())
		{
			Camera->CompareBoundingBox(Min, Max);
			Selected = true;
			SinglePieceSelected = false;
		}
	}

	for (lcLight* Light : mLights)
	{
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
	bool Valid = false;
	Min = lcVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	Max = lcVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsVisible(mCurrentStep))
		{
			Piece->CompareBoundingBox(Min, Max);
			Valid = true;
		}
	}

	return Valid;
}

void lcModel::GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const
{
	for (lcPiece* Piece : mPieces)
	{
		if (!Piece->IsVisibleInSubModel())
			continue;

		int ColorIndex = Piece->mColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		Piece->mPieceInfo->GetPartsList(ColorIndex, ScanSubModels, AddSubModels, PartsList);
	}
}

void lcModel::GetPartsListForStep(lcStep Step, int DefaultColorIndex, lcPartsList& PartsList) const
{
	for (lcPiece* Piece : mPieces)
	{
		if (Piece->GetStepShow() != Step || Piece->IsHidden())
			continue;

		int ColorIndex = Piece->mColorIndex;

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		Piece->mPieceInfo->GetPartsList(ColorIndex, false, true, PartsList);
	}
}

void lcModel::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const
{
	for (lcPiece* Piece : mPieces)
		Piece->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
}

void lcModel::GetSelectionInformation(int* Flags, lcArray<lcObject*>& Selection, lcObject** Focus) const
{
	*Flags = 0;
	*Focus = nullptr;

	if (mPieces.IsEmpty())
		*Flags |= LC_SEL_NO_PIECES;
	else
	{
		lcGroup* Group = nullptr;
		bool First = true;

		for (lcPiece* Piece : mPieces)
		{
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

				if (Piece->CanAddControlPoint())
					*Flags |= LC_SEL_CAN_ADD_CONTROL_POINT;

				if (Piece->CanRemoveControlPoint())
					*Flags |= LC_SEL_CAN_REMOVE_CONTROL_POINT;

				if (Piece->GetGroup() != nullptr)
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
						if (Group == nullptr)
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

	for (lcCamera* Camera : mCameras)
	{
		if (Camera->IsSelected())
		{
			Selection.Add(Camera);
			*Flags |= LC_SEL_SELECTED;

			if (Camera->IsFocused())
				*Focus = Camera;
		}
	}

	for (lcLight* Light : mLights)
	{
		if (Light->IsSelected())
		{
			Selection.Add(Light);
			*Flags |= LC_SEL_SELECTED;

			if (Light->IsFocused())
				*Focus = Light;
		}
	}
}

lcArray<lcObject*> lcModel::GetSelectionModePieces(lcPiece* SelectedPiece) const
{
	PieceInfo* Info = SelectedPiece->mPieceInfo;
	int ColorIndex = SelectedPiece->mColorIndex;
	lcArray<lcObject*> Pieces;

	switch (gMainWindow->GetSelectionMode())
	{
	case lcSelectionMode::SINGLE:
		break;

	case lcSelectionMode::PIECE:
		for (lcPiece* Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->mPieceInfo == Info && Piece != SelectedPiece)
				Pieces.Add(Piece);
		break;

	case lcSelectionMode::COLOR:
		for (lcPiece* Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->mColorIndex == ColorIndex && Piece != SelectedPiece)
				Pieces.Add(Piece);
		break;

	case lcSelectionMode::PIECE_COLOR:
		for (lcPiece* Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->mPieceInfo == Info && Piece->mColorIndex == ColorIndex && Piece != SelectedPiece)
				Pieces.Add(Piece);
		break;
	}

	return Pieces;
}

void lcModel::ClearSelection(bool UpdateInterface)
{
	for (lcPiece* Piece : mPieces)
		Piece->SetSelected(false);

	for (lcCamera* Camera : mCameras)
		Camera->SetSelected(false);

	for (lcLight* Light : mLights)
		Light->SetSelected(false);

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

	for (lcPiece* Piece : mPieces)
		if (!Piece->IsSelected() && Piece->IsVisible(mCurrentStep) && (Piece->GetTopGroup() == TopGroup))
			Piece->SetSelected(Select);
}

void lcModel::FocusOrDeselectObject(const lcObjectSection& ObjectSection)
{
	lcObject* FocusObject = GetFocusObject();
	lcObject* Object = ObjectSection.Object;
	quint32 Section = ObjectSection.Section;

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
			Object->SetFocused(Section, false);

		bool IsSelected = Object->IsSelected();

		if (Object->IsPiece() && (WasSelected != IsSelected))
		{
			lcPiece* Piece = (lcPiece*)Object;

			if (gMainWindow->GetSelectionMode() == lcSelectionMode::SINGLE)
				SelectGroup(Piece->GetTopGroup(), IsSelected);
			else
			{
				lcArray<lcObject*> Pieces = GetSelectionModePieces(Piece);
				AddToSelection(Pieces, false, false);
			}
		}
	}
	else
	{
		if (FocusObject)
			FocusObject->SetFocused(FocusObject->GetFocusSection(), false);
	}

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::ClearSelectionAndSetFocus(lcObject* Object, quint32 Section, bool EnableSelectionMode)
{
	ClearSelection(false);

	if (Object)
	{
		Object->SetFocused(Section, true);

		if (Object->IsPiece())
		{
			SelectGroup(((lcPiece*)Object)->GetTopGroup(), true);

			if (EnableSelectionMode)
			{
				lcArray<lcObject*> Pieces = GetSelectionModePieces((lcPiece*)Object);
				AddToSelection(Pieces, false, false);
			}
		}
	}

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::ClearSelectionAndSetFocus(const lcObjectSection& ObjectSection, bool EnableSelectionMode)
{
	ClearSelectionAndSetFocus(ObjectSection.Object, ObjectSection.Section, EnableSelectionMode);
}

void lcModel::SetSelectionAndFocus(const lcArray<lcObject*>& Selection, lcObject* Focus, quint32 Section, bool EnableSelectionMode)
{
	ClearSelection(false);

	if (Focus)
	{
		Focus->SetFocused(Section, true);

		if (Focus->IsPiece())
		{
			SelectGroup(((lcPiece*)Focus)->GetTopGroup(), true);

			if (EnableSelectionMode)
			{
				lcArray<lcObject*> Pieces = GetSelectionModePieces((lcPiece*)Focus);
				AddToSelection(Pieces, false, false);
			}
		}
	}

	AddToSelection(Selection, EnableSelectionMode, true);
}

void lcModel::AddToSelection(const lcArray<lcObject*>& Objects, bool EnableSelectionMode, bool UpdateInterface)
{
	for (lcObject* Object : Objects)
	{
		bool WasSelected = Object->IsSelected();
		Object->SetSelected(true);

		if (Object->IsPiece())
		{
			if (!WasSelected)
				SelectGroup(((lcPiece*)Object)->GetTopGroup(), true);

			if (EnableSelectionMode)
			{
				lcArray<lcObject*> Pieces = GetSelectionModePieces((lcPiece*)Object);
				AddToSelection(Pieces, false, false);
			}
		}
	}

	if (UpdateInterface)
	{
		gMainWindow->UpdateSelectedObjects(true);
		gMainWindow->UpdateAllViews();
	}
}

void lcModel::RemoveFromSelection(const lcArray<lcObject*>& Objects)
{
	for (lcObject* SelectedObject : Objects)
	{
		bool WasSelected = SelectedObject->IsSelected();
		SelectedObject->SetSelected(false);

		if (WasSelected && SelectedObject->IsPiece())
		{
			lcPiece* Piece = (lcPiece*)SelectedObject;

			if (gMainWindow->GetSelectionMode() == lcSelectionMode::SINGLE)
				SelectGroup(Piece->GetTopGroup(), false);
			else
			{
				lcArray<lcObject*> Pieces = GetSelectionModePieces(Piece);

				for (lcObject* Object : Pieces)
				{
					if (Object->IsSelected())
					{
						Object->SetSelected(false);
						SelectGroup(((lcPiece*)Object)->GetTopGroup(), false);
					}
				}
			}
		}
	}

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::RemoveFromSelection(const lcObjectSection& ObjectSection)
{
	lcObject* SelectedObject = ObjectSection.Object;

	if (!SelectedObject)
		return;

	bool WasSelected = SelectedObject->IsSelected();

	if (SelectedObject->IsFocused(ObjectSection.Section))
		SelectedObject->SetSelected(ObjectSection.Section, false);
	else
		SelectedObject->SetSelected(false);


	if (SelectedObject->IsPiece() && WasSelected)
	{
		lcPiece* Piece = (lcPiece*)SelectedObject;

		if (gMainWindow->GetSelectionMode() == lcSelectionMode::SINGLE)
			SelectGroup(Piece->GetTopGroup(), false);
		else
		{
			lcArray<lcObject*> Pieces = GetSelectionModePieces(Piece);

			for (lcObject* Object : Pieces)
			{
				if (Object->IsSelected())
				{
					Object->SetSelected(false);
					SelectGroup(((lcPiece*)Object)->GetTopGroup(), false);
				}
			}
		}
	}

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::SelectAllPieces()
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(true);

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::InvertSelection()
{
	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisible(mCurrentStep))
			Piece->SetSelected(!Piece->IsSelected());

	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();
}

void lcModel::HideSelectedPieces()
{
	bool Modified = false;

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsSelected())
		{
			Piece->SetHidden(true);
			Piece->SetSelected(false);
			Modified = true;
		}
	}

	if (!Modified)
		return;

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();

	SaveCheckpoint(tr("Hide"));
}

void lcModel::HideUnselectedPieces()
{
	bool Modified = false;

	for (lcPiece* Piece : mPieces)
	{
		if (!Piece->IsSelected())
		{
			Piece->SetHidden(true);
			Modified = true;
		}
	}

	if (!Modified)
		return;

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();

	SaveCheckpoint(tr("Hide"));
}

void lcModel::UnhideSelectedPieces()
{
	bool Modified = false;

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsSelected() && Piece->IsHidden())
		{
			Piece->SetHidden(false);
			Modified = true;
		}
	}

	if (!Modified)
		return;

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();

	SaveCheckpoint(tr("Unhide"));
}

void lcModel::UnhideAllPieces()
{
	bool Modified = false;

	for (lcPiece* Piece : mPieces)
	{
		if (Piece->IsHidden())
		{
			Piece->SetHidden(false);
			Modified = true;
		}
	}

	if (!Modified)
		return;

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateAllViews();

	SaveCheckpoint(tr("Unhide"));
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
	lcObject* Focus = nullptr;
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

	ClearSelectionAndSetFocus(Focus, LC_PIECE_SECTION_POSITION, false);
}

void lcModel::UndoAction()
{
	if (mUndoHistory.size() < 2)
		return;

	lcModelHistoryEntry* Undo = mUndoHistory.front();
	mUndoHistory.erase(mUndoHistory.begin());
	mRedoHistory.insert(mRedoHistory.begin(), Undo);

	LoadCheckPoint(mUndoHistory[0]);

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(mUndoHistory.size() > 1 ? mUndoHistory[0]->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory[0]->Description : nullptr);
}

void lcModel::RedoAction()
{
	if (mRedoHistory.empty())
		return;

	lcModelHistoryEntry* Redo = mRedoHistory.front();
	mRedoHistory.erase(mRedoHistory.begin());
	mUndoHistory.insert(mUndoHistory.begin(), Redo);

	LoadCheckPoint(Redo);

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(mUndoHistory.size() > 1 ? mUndoHistory[0]->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory[0]->Description : nullptr);
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
	case LC_TOOL_COLOR_PICKER:
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

	case LC_NUM_TOOLS:
		break;
	}
}

void lcModel::InsertPieceToolClicked(const lcMatrix44& WorldMatrix)
{
	lcPiece* Piece = new lcPiece(gMainWindow->GetCurrentPieceInfo());
	Piece->Initialize(WorldMatrix, mCurrentStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	Piece->UpdatePosition(mCurrentStep);
	AddPiece(Piece);

	gMainWindow->UpdateTimeline(false, false);
	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION, false);

	SaveCheckpoint(tr("Insert"));
}

void lcModel::PointLightToolClicked(const lcVector3& Position)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2]);
	Light->CreateName(mLights);
	mLights.Add(Light);

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_POSITION, false);
	SaveCheckpoint(tr("New Light"));
}

void lcModel::BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	mLights.Add(Light);

	mMouseToolDistance = Target;

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_TARGET, false);
}

void lcModel::UpdateSpotLightTool(const lcVector3& Position)
{
	lcLight* Light = mLights[mLights.GetSize() - 1];

	Light->MoveSelected(1, false, Position - mMouseToolDistance);
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

	ClearSelectionAndSetFocus(Camera, LC_CAMERA_SECTION_TARGET, false);
}

void lcModel::UpdateCameraTool(const lcVector3& Position)
{
	lcCamera* Camera = mCameras[mCameras.GetSize() - 1];

	Camera->MoveSelected(1, false, Position - mMouseToolDistance);
	Camera->UpdatePosition(1);

	mMouseToolDistance = Position;

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateMoveTool(const lcVector3& Distance, bool AlternateButtonDrag)
{
	lcVector3 PieceDistance = SnapPosition(Distance) - SnapPosition(mMouseToolDistance);
	lcVector3 ObjectDistance = Distance - mMouseToolDistance;

	MoveSelectedObjects(PieceDistance, ObjectDistance, true, AlternateButtonDrag, true, false);
	mMouseToolDistance = Distance;

	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();
}

void lcModel::UpdateRotateTool(const lcVector3& Angles, bool AlternateButtonDrag)
{
	lcVector3 Delta = SnapRotation(Angles) - SnapRotation(mMouseToolDistance);
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

void lcModel::ColorPickerToolClicked(lcObject* Object)
{
	if (!Object || Object->GetType() != LC_OBJECT_PIECE)
		return;

	lcPiece* Piece = (lcPiece*)Object;

	gMainWindow->SetColorIndex(Piece->mColorIndex);
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

void lcModel::MoveCamera(lcCamera* Camera, const lcVector3& Direction)
{
	Camera->MoveRelative(Direction, mCurrentStep, gMainWindow->GetAddKeys());
	gMainWindow->UpdateSelectedObjects(false);
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		SaveCheckpoint(tr("Moving Camera"));
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

	GetPartsList(gDefaultColor, true, false, Options.PartsList);

	lcQPropertiesDialog Dialog(gMainWindow, &Options);
	if (Dialog.exec() != QDialog::Accepted)
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

	lcQSelectDialog Dialog(gMainWindow, this);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	SetSelectionAndFocus(Dialog.mObjects, nullptr, 0, false);
}

void lcModel::ShowSelectByColorDialog()
{
	if (mPieces.IsEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to select."));
		return;
	}

	int ColorIndex = gMainWindow->mColorIndex;

	lcObject* Focus = GetFocusObject();

	if (Focus && Focus->IsPiece())
		ColorIndex = ((lcPiece*)Focus)->mColorIndex;

	lcSelectByColorDialog Dialog(gMainWindow, ColorIndex);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	ColorIndex = Dialog.mColorIndex;
	lcArray<lcObject*> Selection;

	for (lcPiece* Piece : mPieces)
		if (Piece->IsVisible(mCurrentStep) && Piece->mColorIndex == ColorIndex)
			Selection.Add(Piece);

	SetSelectionAndFocus(Selection, nullptr, 0, false);
}

void lcModel::ShowArrayDialog()
{
	lcVector3 Center;

	if (!GetPieceFocusOrSelectionCenter(Center))
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No pieces selected."));
		return;
	}
	
	lcQArrayDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	if (Dialog.mCounts[0] * Dialog.mCounts[1] * Dialog.mCounts[2] < 2)
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Array only has 1 element or less, no pieces added."));
		return;
	}

	lcArray<lcObject*> NewPieces;

	for (int Step1 = 0; Step1 < Dialog.mCounts[0]; Step1++)
	{
		for (int Step2 = 0; Step2 < Dialog.mCounts[1]; Step2++)
		{
			for (int Step3 = (Step1 == 0 && Step2 == 0) ? 1 : 0; Step3 < Dialog.mCounts[2]; Step3++)
			{
				lcMatrix44 ModelWorld;
				lcVector3 Position;

				lcVector3 RotationAngles = Dialog.mRotations[0] * Step1 + Dialog.mRotations[1] * Step2 + Dialog.mRotations[2] * Step3;
				lcVector3 Offset = Dialog.mOffsets[0] * Step1 + Dialog.mOffsets[1] * Step2 + Dialog.mOffsets[2] * Step3;

				for (lcPiece* Piece : mPieces)
				{
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

					lcPiece* NewPiece = new lcPiece(nullptr);
					NewPiece->SetPieceInfo(Piece->mPieceInfo, Piece->GetID(), true);
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

	AddToSelection(NewPieces, false, true);
	gMainWindow->UpdateTimeline(false, false);
	SaveCheckpoint(tr("Array"));
}

void lcModel::ShowMinifigDialog()
{
	lcQMinifigDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	gMainWindow->GetActiveView()->MakeCurrent();

	lcGroup* Group = AddGroup(tr("Minifig #"), nullptr);
	lcArray<lcObject*> Pieces(LC_MFW_NUMITEMS);
	lcMinifig& Minifig = Dialog.mMinifigWidget->mMinifig;

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (Minifig.Parts[PartIdx] == nullptr)
			continue;

		lcPiece* Piece = new lcPiece(Minifig.Parts[PartIdx]);

		Piece->Initialize(Minifig.Matrices[PartIdx], mCurrentStep);
		Piece->SetColorIndex(Minifig.Colors[PartIdx]);
		Piece->SetGroup(Group);
		AddPiece(Piece);
		Piece->UpdatePosition(mCurrentStep);

		Pieces.Add(Piece);
	}

	SetSelectionAndFocus(Pieces, nullptr, 0, false);
	gMainWindow->UpdateTimeline(false, false);
	SaveCheckpoint(tr("Minifig"));
}

void lcModel::UpdateInterface()
{
	gMainWindow->UpdateTimeline(true, false);
	gMainWindow->UpdateUndoRedo(mUndoHistory.size() > 1 ? mUndoHistory[0]->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory[0]->Description : nullptr);
	gMainWindow->UpdatePaste(!gApplication->mClipboard.isEmpty());
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
	gMainWindow->UpdateShadingMode();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateSelectionMode();
}
