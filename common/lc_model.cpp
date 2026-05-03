#include "lc_global.h"
#include "lc_model.h"
#include "lc_modelhistory.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "project.h"
#include "lc_mainwindow.h"
#include "lc_profile.h"
#include "lc_library.h"
#include "lc_scene.h"
#include "lc_traintrack.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_view.h"
#include "minifig.h"
#include "lc_arraydialog.h"
#include "lc_minifigdialog.h"
#include "lc_groupdialog.h"
#include "lc_editgroupsdialog.h"
#include "lc_qpropertiesdialog.h"
#include "lc_qutils.h"
#include "lc_lxf.h"
#include "lc_previewwidget.h"
#include "lc_string.h"
#include <locale.h>

void lcModelProperties::LoadDefaults()
{
	mAuthor = lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME);
	mAmbientColor = lcVector3FromColor(lcGetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR));
}

void lcModelProperties::SaveDefaults()
{
	lcSetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR, lcColorFromVector3(mAmbientColor));
}

void lcModelProperties::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");

	Stream << QLatin1String("0 ") << mDescription << LineEnding;
	Stream << QLatin1String("0 Name: ") << mModelName << LineEnding;
	Stream << QLatin1String("0 Author: ") << mAuthor << LineEnding;

	if (!mComments.isEmpty())
	{
		const QStringList Comments = mComments.split('\n');
		for (const QString& Comment : Comments)
			Stream << QLatin1String("0 !LEOCAD MODEL COMMENT ") << Comment << LineEnding;
	}

//	lcVector3 mAmbientColor;
}

bool lcModelProperties::ParseLDrawHeader(QString Line, bool FirstLine)
{
	QTextStream LineStream(&Line, QIODevice::ReadOnly);

	QString Token;
	LineStream >> Token;
	const int StartPos = LineStream.pos();
	LineStream >> Token;

	if (Token == QLatin1String("!LEOCAD"))
		return false;

	if (Token == QLatin1String("Name:"))
	{
		mModelName = LineStream.readLine().mid(1);
		return true;
	}

	if (Token == QLatin1String("Author:"))
	{
		mAuthor = LineStream.readLine().mid(1);
		return true;
	}

	if (FirstLine)
	{
		LineStream.seek(StartPos);
		mDescription = LineStream.readLine().mid(1);
		return true;
	}

	return false;
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
}

void lcPOVRayOptions::ParseLDrawLine(QTextStream& LineStream)
{
	QString Token;
	LineStream >> Token;

	if (Token == QLatin1String("HEADER_INCLUDE_FILE"))
	{
		LineStream >> HeaderIncludeFile;
		if (!QFileInfo(HeaderIncludeFile).isReadable())
			HeaderIncludeFile.clear();
	}
	else if (Token == QLatin1String("FOOTER_INCLUDE_FILE"))
	{
		LineStream >> FooterIncludeFile;
		if (!QFileInfo(FooterIncludeFile).isReadable())
			FooterIncludeFile.clear();
	}
	else if (Token == QLatin1String("FLOOR_AXIS"))
	{
		LineStream >> FloorAxis;
		if (FloorAxis < 0 || FloorAxis > 2)
			FloorAxis = 1; // y
	}
	else if (Token == QLatin1String("FLOOR_COLOR_RGB"))
		LineStream >> FloorColor[0] >> FloorColor[1] >> FloorColor[2];
	else if (Token == QLatin1String("FLOOR_AMBIENT"))
		LineStream >> FloorAmbient;
	else if (Token == QLatin1String("FLOOR_DIFFUSE"))
		LineStream >> FloorDiffuse;
	else if (Token == QLatin1String("EXCLUDE_FLOOR"))
		ExcludeFloor = true;
	else if (Token == QLatin1String("EXCLUDE_BACKGROUND"))
		ExcludeBackground = true;
	else if (Token == QLatin1String("NO_REFLECTION"))
		NoReflection = true;
	else if (Token == QLatin1String("NO_SHADOWS"))
		NoShadow = true;
	else if (Token == QLatin1String("USE_LGEO"))
		UseLGEO = true;
}

void lcPOVRayOptions::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");
	if (!HeaderIncludeFile.isEmpty())
		Stream << QLatin1String("0 !LEOCAD POV_RAY HEADER_INCLUDE_FILE ") << QDir::toNativeSeparators(HeaderIncludeFile) << LineEnding;
	if (!FooterIncludeFile.isEmpty())
		Stream << QLatin1String("0 !LEOCAD POV_RAY FOOTER_INCLUDE_FILE ") << QDir::toNativeSeparators(FooterIncludeFile) << LineEnding;
	if (FloorAxis != 1)
		Stream << QLatin1String("0 !LEOCAD POV_RAY FLOOR_AXIS ") << FloorAxis << LineEnding;
	if (FloorColor != lcVector3(0.8f,0.8f,0.8f))
		Stream << QLatin1String("0 !LEOCAD POV_RAY FLOOR_COLOR_RGB ") << FloorColor[0] << ' ' << FloorColor[1] << ' ' << FloorColor[2] << LineEnding;
	if (FloorAmbient != 0.4f)
		Stream << QLatin1String("0 !LEOCAD POV_RAY FLOOR_AMBIENT ") << FloorAmbient << LineEnding;
	if (FloorDiffuse != 0.4f)
		Stream << QLatin1String("0 !LEOCAD POV_RAY FLOOR_DIFFUSE ") << FloorDiffuse << LineEnding;
	if (ExcludeFloor)
		Stream << QLatin1String("0 !LEOCAD POV_RAY EXCLUDE_FLOOR") << LineEnding;
	if (ExcludeBackground)
		Stream << QLatin1String("0 !LEOCAD POV_RAY EXCLUDE_BACKGROUND") << LineEnding;
	if (NoReflection)
		Stream << QLatin1String("0 !LEOCAD POV_RAY NO_REFLECTION") << LineEnding;
	if (NoShadow)
		Stream << QLatin1String("0 !LEOCAD POV_RAY NO_SHADOWS") << LineEnding;
	if (UseLGEO)
		Stream << QLatin1String("0 !LEOCAD POV_RAY USE_LGEO") << LineEnding;
}

lcModel::lcModel(const QString& FileName, Project* Project, bool Preview)
	: mProject(Project), mIsPreview(Preview)
{
	mProperties.mModelName = FileName;
	mProperties.mFileName = FileName;
	mProperties.LoadDefaults();

	mActive = false;
	mCurrentStep = 1;
	mPieceInfo = nullptr;
}

lcModel::~lcModel()
{
	if (mPieceInfo)
	{
		if (!mIsPreview && gMainWindow && gMainWindow->GetCurrentPieceInfo() == mPieceInfo)
			gMainWindow->SetCurrentPieceInfo(nullptr);

		if (mPieceInfo->GetModel() == this)
			mPieceInfo->SetPlaceholder();

		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		Library->ReleasePieceInfo(mPieceInfo);
	}

	DeleteModel();
}

bool lcModel::GetPieceWorldMatrix(lcPiece* Piece, lcMatrix44& ParentWorldMatrix) const
{
	for (const std::unique_ptr<lcPiece>& ModelPiece : mPieces)
	{
		if (ModelPiece.get() == Piece)
		{
			ParentWorldMatrix = lcMul(ModelPiece->mModelWorld, ParentWorldMatrix);
			return true;
		}

		const PieceInfo* Info = ModelPiece->mPieceInfo;

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

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->mPieceInfo->IncludesModel(Model))
			return true;

	return false;
}

void lcModel::DeleteModel()
{
	if (gMainWindow)
	{
		std::vector<lcView*> Views = lcView::GetModelViews(this);

		// TODO: this is only needed to avoid a dangling pointer during undo/redo if a camera is set to a view but we should find a better solution instead
		for (lcView* View : Views)
		{
			lcCamera* ViewCamera = View->GetCamera();

			if (!ViewCamera->IsSimple())
			{
				for (const std::unique_ptr<lcCamera>& Camera : mCameras)
				{
					if (Camera.get() == ViewCamera)
					{
						View->SetCamera(ViewCamera, true);
						break;
					}
				}
			}
		}
	}

	mPieces.clear();
	mCameras.clear();
	mLights.clear();
	mGroups.clear();
	mFileLines.clear();
}

void lcModel::CreatePieceInfo(Project* Project)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	mPieceInfo = Library->FindPiece(mProperties.mFileName.toLatin1().constData(), Project, true, false);
	mPieceInfo->SetModel(this, true, Project, true);
	Library->LoadPieceInfo(mPieceInfo, true, true);
}

void lcModel::UpdateMesh()
{
	mPieceInfo->SetModel(this, true, nullptr, false);
}

void lcModel::UpdateAllViews() const
{
	lcView::UpdateProjectViews(mProject);
}

void lcModel::UpdatePieceInfo(std::vector<lcModel*>& UpdatedModels)
{
	if (std::find(UpdatedModels.begin(), UpdatedModels.end(), this) != UpdatedModels.end())
		return;

	mPieceInfo->SetModel(this, false, nullptr, false);
	UpdatedModels.push_back(this);

	const lcMesh* Mesh = mPieceInfo->GetMesh();

	if (mPieces.empty() && !Mesh)
	{
		mPieceInfo->SetBoundingBox(lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 0.0f));
		return;
	}

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
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

void lcModel::SaveLDraw(QTextStream& Stream, bool SelectedOnly, lcStep LastStep) const
{
	const QLatin1String LineEnding("\r\n");

	mProperties.SaveLDraw(Stream);

	std::vector<lcGroup*> CurrentGroups;
	lcStep Step = 1;
	int CurrentLine = 0;
	int AddedSteps = 0;
	bool SavedStep = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (SelectedOnly && !Piece->IsSelected())
			continue;

		if ((SavedStep = (LastStep != 0 && Piece->GetStepShow() > LastStep)))
			break;

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
			if (CurrentGroups.empty() || (!CurrentGroups.empty() && PieceGroup != CurrentGroups[CurrentGroups.size() - 1]))
			{
				std::deque<lcGroup*> PieceParents;

				for (lcGroup* Group = PieceGroup; Group; Group = Group->mGroup)
					PieceParents.push_front(Group);

				std::deque<lcGroup*>::iterator ParentsToAdd = PieceParents.begin();

				while (!CurrentGroups.empty())
				{
					lcGroup* Group = CurrentGroups.back();
					const std::deque<lcGroup*>::iterator ParentFound = std::find(PieceParents.begin(), PieceParents.end(), Group);

					if (ParentFound == PieceParents.end())
					{
						CurrentGroups.pop_back();
						Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
					}
					else
					{
						ParentsToAdd = ParentFound + 1;
						break;
					}
				}

				for (std::deque<lcGroup*>::iterator ParentIt = ParentsToAdd; ParentIt != PieceParents.end(); ParentIt++)
				{
					lcGroup* Group = *ParentIt;
					CurrentGroups.emplace_back(Group);
					Stream << QLatin1String("0 !LEOCAD GROUP BEGIN ") << Group->mName << LineEnding;
				}
			}
		}
		else
		{
			while (CurrentGroups.size())
			{
				CurrentGroups.pop_back();
				Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
			}
		}

		if (Piece->mPieceInfo->GetSynthInfo())
		{
			Stream << QLatin1String("0 !LEOCAD SYNTH BEGIN\r\n");

			const std::vector<lcPieceControlPoint>& ControlPoints = Piece->GetControlPoints();
			for (const lcPieceControlPoint& ControlPoint : ControlPoints)
			{
				Stream << QLatin1String("0 !LEOCAD SYNTH CONTROL_POINT");

				const float* FloatMatrix = ControlPoint.Transform.GetFloats();
				const float Numbers[13] = { FloatMatrix[12], -FloatMatrix[14], FloatMatrix[13], FloatMatrix[0], -FloatMatrix[8], FloatMatrix[4], -FloatMatrix[2], FloatMatrix[10], -FloatMatrix[6], FloatMatrix[1], -FloatMatrix[9], FloatMatrix[5], ControlPoint.Scale };

				for (int NumberIdx = 0; NumberIdx < 13; NumberIdx++)
					Stream << ' ' << lcFormatValue(Numbers[NumberIdx], NumberIdx < 3 ? 4 : 6);

				Stream << LineEnding;
			}
		}

		Piece->SaveLDraw(Stream);

		if (Piece->mPieceInfo->GetSynthInfo())
			Stream << QLatin1String("0 !LEOCAD SYNTH END\r\n");
	}

	while (!SavedStep && CurrentLine < mFileLines.size())
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

	while (CurrentGroups.size())
	{
		CurrentGroups.pop_back();
		Stream << QLatin1String("0 !LEOCAD GROUP END\r\n");
	}

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (!SelectedOnly || Camera->IsSelected())
			Camera->SaveLDraw(Stream);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (!SelectedOnly || Light->IsSelected())
			Light->SaveLDraw(Stream);

	Stream.flush();
}

int lcModel::SplitMPD(QIODevice& Device)
{
	qint64 ModelPos = Device.pos();

	while (!Device.atEnd())
	{
		const qint64 Pos = Device.pos();
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
				if (!mProperties.mFileName.isEmpty())
				{
					Device.seek(Pos);
					break;
				}

				SetFileName(LineStream.readAll().trimmed());
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
	std::vector<lcGroup*> CurrentGroups;
	std::vector<lcPieceControlPoint> ControlPoints;
	int CurrentStep = 1;
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	mProperties.mAuthor.clear();
	mProperties.mDescription.clear();
	mProperties.mComments.clear();
	bool ReadingHeader = true;
	bool FirstLine = true;

	while (!Device.atEnd())
	{
		const qint64 Pos = Device.pos();
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

				if (mProperties.mFileName != Name)
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

			if (ReadingHeader)
			{
				ReadingHeader = mProperties.ParseLDrawHeader(Line, FirstLine);
				FirstLine = false;

				if (ReadingHeader)
					continue;
			}

			if (Token == QLatin1String("STEP"))
			{
				delete Piece;
				Piece = nullptr;
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
					mCameras.emplace_back(Camera);
					Camera = nullptr;
				}
			}
			else if (Token == QLatin1String("LIGHT"))
			{
				if (!Light)
					Light = new lcLight(lcVector3(0.0f, 0.0f, 0.0f), lcLightType::Point);

				if (Light->ParseLDrawLine(LineStream))
				{
					Light->CreateName(mLights);
					mLights.emplace_back(Light);
					Light = nullptr;
				}
			}
			else if (Token == QLatin1String("POV_RAY"))
			{
				mPOVRayOptions.ParseLDrawLine(LineStream);
			}
			else if (Token == QLatin1String("GROUP"))
			{
				LineStream >> Token;

				if (Token == QLatin1String("BEGIN"))
				{
					QString Name = LineStream.readAll().trimmed();
					lcGroup* Group = GetGroup(Name, true);
					if (!CurrentGroups.empty())
						Group->mGroup = CurrentGroups[CurrentGroups.size() - 1];
					else
						Group->mGroup = nullptr;
					CurrentGroups.emplace_back(Group);
				}
				else if (Token == QLatin1String("END"))
				{
					if (!CurrentGroups.empty())
						CurrentGroups.pop_back();
				}
			}
			else if (Token == QLatin1String("SYNTH"))
			{
				LineStream >> Token;

				if (Token == QLatin1String("BEGIN"))
				{
					ControlPoints.clear();
				}
				else if (Token == QLatin1String("END"))
				{
					ControlPoints.clear();
				}
				else if (Token == QLatin1String("CONTROL_POINT"))
				{
					float Numbers[13];
					for (int TokenIdx = 0; TokenIdx < 13; TokenIdx++)
						LineStream >> Numbers[TokenIdx];

					lcPieceControlPoint& PieceControlPoint = ControlPoints.emplace_back();
					PieceControlPoint.Transform = lcMatrix44(lcVector4(Numbers[3], Numbers[9], -Numbers[6], 0.0f), lcVector4(Numbers[5], Numbers[11], -Numbers[8], 0.0f),
					                                         lcVector4(-Numbers[4], -Numbers[10], Numbers[7], 0.0f), lcVector4(Numbers[0], Numbers[2], -Numbers[1], 1.0f));
					PieceControlPoint.Scale = Numbers[12];
				}
			}

			continue;
		}
		else if (Token == QLatin1String("1"))
		{
			ReadingHeader = false;
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

				if (!CurrentGroups.empty())
					Piece->SetGroup(CurrentGroups[CurrentGroups.size() - 1]);

				PieceInfo* Info = Library->FindPiece(PartId.toLatin1().constData(), Project, true, true);

				const float* Matrix = IncludeTransform.GetFloats();
				const lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
									       lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(Matrix[12], Matrix[14], -Matrix[13], 1.0f));

				Piece->SetFileLine(mFileLines.size());
				Piece->SetPieceInfo(Info, PartId, false, true);
				Piece->Initialize(Transform, CurrentStep);
				Piece->SetColorCode(ColorCode);
				Piece->VerifyControlPoints(ControlPoints);
				Piece->SetControlPoints(ControlPoints);
				ControlPoints.clear();

				if (Piece->mPieceInfo->IsModel() && Piece->mPieceInfo->GetModel()->IncludesModel(this))
				{
					delete Piece;
					Piece = nullptr;
					continue;
				}

				AddPiece(Piece);
				Piece = nullptr;
			}
		}
		else
		{
			ReadingHeader = false;
			mFileLines.append(OriginalLine);
		}

		FirstLine = false;
	}

	mCurrentStep = CurrentStep;
	CalculateStep(mCurrentStep);
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

	if (memcmp(id, "LeoCAD ", 7))
		return false;

	if (fv == 0.0f)
	{
		const lconv *loc = localeconv();
		id[8] = loc->decimal_point[0];
		sscanf(&id[7], "%f", &fv);

		if (fv == 0.0f)
			return false;
	}

	if (fv > 0.4f)
		file->ReadFloats(&fv, 1);

	file->ReadU32(&rgb, 1);

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

	const size_t FirstNewPiece = mPieces.size();

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

			file->ReadFloats(pos.GetFloats(), 3);
			file->ReadFloats(rot.GetFloats(), 3);
			file->ReadU8(&color, 1);
			file->ReadBuffer(name, 9);
			lcstrcat(name, ".dat");
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
		const size_t NumGroups = mGroups.size();

		file->ReadS32(&count, 1);
		for (i = 0; i < count; i++)
			mGroups.emplace_back(new lcGroup());

		for (size_t GroupIdx = NumGroups; GroupIdx < mGroups.size(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx].get();

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

		for (size_t GroupIdx = NumGroups; GroupIdx < mGroups.size(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx].get();

			i = (qint32)(quintptr)(Group->mGroup);
			Group->mGroup = nullptr;

			if (i > 0xFFFF || i == -1)
				continue;

			Group->mGroup = mGroups[NumGroups + i].get();
		}

		for (size_t PieceIndex = FirstNewPiece; PieceIndex < mPieces.size(); PieceIndex++)
		{
			lcPiece* Piece = mPieces[PieceIndex].get();

			i = (qint32)(quintptr)(Piece->GetGroup());
			Piece->SetGroup(nullptr);

			if (i > 0xFFFF || i == -1)
				continue;

			Piece->SetGroup(mGroups[NumGroups + i].get());
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
			lcCamera::FileLoad(*file);
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

		file->Seek(sh, SEEK_CUR); // Background
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
		file->ReadU32(&rgb, 1);
	}

	CalculateStep(mCurrentStep);
	lcGetPiecesLibrary()->UnloadUnusedParts();

	return true;
}

bool lcModel::LoadLDD(const QString& FileData)
{
	std::vector<lcPiece*> Pieces;
	std::vector<std::vector<lcPiece*>> Groups;

	if (!lcImportLXFMLFile(FileData, Pieces, Groups))
		return false;

	for (lcPiece* Piece : Pieces)
		AddPiece(Piece);

	for (const std::vector<lcPiece*>& Group : Groups)
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

bool lcModel::LoadInventory(const std::vector<lcSetInventoryItem>& SetInventory)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	for (const lcSetInventoryItem& SetInventoryItem : SetInventory)
	{
		PieceInfo* Info = Library->FindPiece(SetInventoryItem.PartID + ".dat", nullptr, true, false);
		int Quantity = SetInventoryItem.Quantity;

		while (Quantity--)
		{
			lcPiece* Piece = new lcPiece(nullptr);
			Piece->SetPieceInfo(Info, QString(), false, true);
			Piece->Initialize(lcMatrix44Identity(), 1);
			Piece->SetColorCode(SetInventoryItem.ColorCode);
			AddPiece(Piece);
		}
	}

	if (mPieces.empty())
		return false;

	Library->WaitForLoadQueue();
	Library->mBuffersDirty = true;
	Library->UnloadUnusedParts();

	auto RoundBounds = [](float& Value)
	{
		Value = ((Value < 0.0f) ? floor((Value - 5.0f) / 10.0f) : ceil((Value + 5.0f) / 10.0f)) * 10.0f;
	};

	constexpr float TargetHeight = 800.0f;
	float CurrentX = 0.0f;
	float CurrentY = 0.0f;
	float ColumnWidth = 0.0f;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		lcBoundingBox BoundingBox = Piece->mPieceInfo->GetBoundingBox();
		RoundBounds(BoundingBox.Min.x);
		RoundBounds(BoundingBox.Min.y);
		RoundBounds(BoundingBox.Max.x);
		RoundBounds(BoundingBox.Max.y);

		const float PieceWidth = BoundingBox.Max.x - BoundingBox.Min.x;
		const float PieceHeight = BoundingBox.Max.y - BoundingBox.Min.y;

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
}

void lcModel::Merge(std::unique_ptr<lcModel> Other)
{
	for (std::unique_ptr<lcPiece>& Piece : Other->mPieces)
	{
		Piece->SetFileLine(-1);
		AddPiece(Piece.release());
	}

	Other->mPieces.clear();

	for (std::unique_ptr<lcCamera>& Camera : Other->mCameras)
	{
		Camera->CreateName(mCameras);
		mCameras.emplace_back(std::move(Camera));
	}

	Other->mCameras.clear();

	for (std::unique_ptr<lcLight>& Light : Other->mLights)
	{
		Light->CreateName(mLights);
		mLights.emplace_back(std::move(Light));
	}

	Other->mLights.clear();

	for (std::vector<std::unique_ptr<lcGroup>>::iterator GroupIt = Other->mGroups.begin(); GroupIt != Other->mGroups.end(); GroupIt++)
	{
		std::unique_ptr<lcGroup>& Group = *GroupIt;
		Group->CreateName(mGroups);
		mGroups.emplace_back(std::move(Group));
	}

	Other->mGroups.clear();

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::Cut()
{
	if (!AnyObjectsSelected())
		return;

	Copy();

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	RemoveSelectedObjects();

	EndEditHistory();
	EndHistorySequence(tr("Cut"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::Copy()
{
	QByteArray File;
	QTextStream Stream(&File, QIODevice::WriteOnly);

	SaveLDraw(Stream, true, 0);

	gApplication->ExportClipboard(File);
}

void lcModel::Paste(bool PasteToCurrentStep)
{
	if (gApplication->mClipboard.isEmpty())
		return;

	std::unique_ptr<lcModel> Model(new lcModel(QString(), nullptr, false));

	QBuffer Buffer(&gApplication->mClipboard);
	Buffer.open(QIODevice::ReadOnly);
	Model->LoadLDraw(Buffer, lcGetActiveProject());

	const std::vector<std::unique_ptr<lcPiece>>& PastedPieces = Model->mPieces;
	std::vector<lcObject*> SelectedObjects;
	SelectedObjects.reserve(PastedPieces.size());

	for (const std::unique_ptr<lcPiece>& Piece : PastedPieces)
	{
		Piece->SetFileLine(-1);

		if (PasteToCurrentStep)
		{
			Piece->SetStepShow(mCurrentStep);
			SelectedObjects.emplace_back(Piece.get());
		}
		else
		{
			if (Piece->GetStepShow() <= mCurrentStep)
				SelectedObjects.emplace_back(Piece.get());
		}
	}

	if (PastedPieces.empty())
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	Merge(std::move(Model));

	CalculateStep(mCurrentStep);

	EndEditHistory();

	if (SelectedObjects.size() == 1)
		SetSelectionAndFocus(std::vector<lcObject*>(), SelectedObjects.front(), LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	else
		SetSelectionAndFocus(SelectedObjects, nullptr, 0, lcSelectionMode::Single);

	EndHistorySequence(tr("Paste"));

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::DuplicateSelectedPieces()
{
	if (!AnyPiecesSelected())
	{
		QMessageBox::information(gMainWindow, tr("Duplicate Pieces"), tr("No pieces selected."));
		return;
	}

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcObject*> NewPieces;
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
				const QChar Last = GroupName[GroupName.size() - 1];
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

	for (size_t PieceIndex = 0; PieceIndex < mPieces.size(); PieceIndex++)
	{
		const lcPiece* Piece = mPieces[PieceIndex].get();

		if (!Piece->IsSelected())
			continue;

		lcPiece* NewPiece = new lcPiece(*Piece);
		NewPiece->UpdatePosition(mCurrentStep);
		NewPieces.emplace_back(NewPiece);

		if (Piece->IsFocused())
			Focus = NewPiece;

		PieceIndex++;
		AddPiece(std::unique_ptr<lcPiece>(NewPiece), PieceIndex);

		lcGroup* Group = Piece->GetGroup();
		if (Group)
			NewPiece->SetGroup(GetNewGroup(Group));
	}

	EndEditHistory();

	SetSelectionAndFocus(NewPieces, Focus, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(tr("Duplicate"));

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::PaintSelectedPieces()
{
	SetSelectedPiecesColorIndex(gMainWindow->mColorIndex);
}

void lcModel::GetScene(lcScene* Scene, const lcCamera* ViewCamera, bool AllowHighlight, bool AllowFade) const
{
	if (mPieceInfo)
		mPieceInfo->AddRenderMesh(*Scene);

	lcPiece* FocusPiece = nullptr;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() || Piece->IsVisible(mCurrentStep))
		{
			if (Piece->IsFocused())
				FocusPiece = Piece.get();

			const lcStep StepShow = Piece->GetStepShow();
			Piece->AddMainModelRenderMeshes(Scene, AllowHighlight && StepShow == mCurrentStep, AllowFade && StepShow < mCurrentStep);
		}
	}

	if (Scene->GetDrawInterface() && !Scene->GetActiveSubmodelInstance())
	{
		if (FocusPiece)
			UpdateTrainTrackConnections(FocusPiece, false);

		for (const std::unique_ptr<lcCamera>& Camera : mCameras)
			if (Camera.get() != ViewCamera && Camera->IsVisible())
				Scene->AddInterfaceObject(Camera.get());

		for (const std::unique_ptr<lcLight>& Light : mLights)
			if (Light->IsVisible())
				Scene->AddInterfaceObject(Light.get());
	}
}

void lcModel::AddSubModelRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsVisibleInSubModel())
		{
			if (Piece->IsFocused())
				UpdateTrainTrackConnections(Piece.get(), false);

			Piece->AddSubModelRenderMeshes(Scene, WorldMatrix, DefaultColorIndex, RenderMeshState, ParentActive);
		}
	}
}

QImage lcModel::GetStepImage(bool Zoom, int Width, int Height, lcStep Step)
{
	const lcView* ActiveView = gMainWindow->GetActiveView();
	const lcStep CurrentStep = mCurrentStep;
	lcCamera* Camera = ActiveView->GetCamera();

	lcView View(lcViewType::View, this);
	View.SetCamera(Camera, true);
	View.SetOffscreenContext();
	View.MakeCurrent();

	if (!View.BeginRenderToImage(Width, Height))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return QImage();
	}

	SetTemporaryStep(Step);

	if (Zoom)
		ZoomExtents(Camera, (float)Width / (float)Height, lcMatrix44Identity());

	View.OnDraw();

	QImage Image = View.GetRenderImage();

	View.EndRenderToImage();

	SetTemporaryStep(CurrentStep);

	if (!mActive)
		CalculateStep(LC_STEP_MAX);

	return Image;
}

QImage lcModel::GetPartsListImage(int MaxWidth, lcStep Step, quint32 BackgroundColor, QFont Font, QColor TextColor) const
{
	lcPartsList PartsList;

	if (Step == 0)
		GetPartsList(gDefaultColor, true, false, PartsList);
	else
		GetPartsListForStep(Step, gDefaultColor, PartsList, false);

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

	lcView View(lcViewType::PartsList, nullptr);
	View.SetOffscreenContext();
	View.MakeCurrent();
	lcContext* Context = View.mContext;
	const int ThumbnailSize = qMin(MaxWidth, 512);

	View.SetSize(ThumbnailSize, ThumbnailSize);

	if (!View.BeginRenderToImage(ThumbnailSize, ThumbnailSize))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return QImage();
	}

	float OrthoSize = 200.0f;

	lcMatrix44 ProjectionMatrix = lcMatrix44Ortho(-OrthoSize, OrthoSize, -OrthoSize, OrthoSize, -5000.0f, 5000.0f);
	const lcMatrix44 ViewMatrix = lcMatrix44LookAt(lcVector3(-100.0f, -100.0f, 75.0f), lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f));
	const int Viewport[4] = { 0, 0, ThumbnailSize, ThumbnailSize };

	float ExtraPixels = 0.0f;

	for (const lcPartsListImage& Image : Images)
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
		View.BindRenderFramebuffer();
		Context->ClearColorAndDepth(lcVector4(lcVector3FromColor(BackgroundColor), 0.0f));

		lcScene Scene;

		const lcPreferences& Preferences = lcGetPreferences();
		lcShadingMode ShadingMode = Preferences.mShadingMode;
		if (ShadingMode == lcShadingMode::Wireframe)
			ShadingMode = lcShadingMode::Flat;

		Scene.SetShadingMode(ShadingMode);
		Scene.SetAllowLOD(false);
		Scene.Begin(ViewMatrix);

		Image.Info->AddRenderMeshes(&Scene, lcMatrix44Identity(), Image.ColorIndex, lcRenderMeshState::Default, true);

		Scene.End();

		Scene.Draw(Context);

		View.UnbindRenderFramebuffer();
		Image.Thumbnail = View.GetRenderFramebufferImage().convertToFormat(QImage::Format_ARGB32);
	}

	View.EndRenderToImage();
	Context->ClearResources();

	auto CalculateImageBounds = [](lcPartsListImage& Image)
	{
		const QImage& Thumbnail = Image.Thumbnail;
		const int Width = Thumbnail.width();
		const int Height = Thumbnail.height();

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

	DummyPainter.setFont(Font);
	QFontMetrics FontMetrics = DummyPainter.fontMetrics();
	const int Ascent = FontMetrics.ascent();

	int CurrentHeight = 0;
	int ImageWidth = MaxWidth;

	for (const lcPartsListImage& Image : Images)
		CurrentHeight = qMax(Image.Bounds.height() + Ascent, CurrentHeight);

	for (;;)
	{
		int CurrentWidth = 0;
		int CurrentX = 0;
		int CurrentY = 0;
		int ColumnWidth = 0;
		constexpr int Spacing = 20;
		int NextHeightIncrease = INT_MAX;

		for (lcPartsListImage& Image : Images)
		{
			if (CurrentY + Image.Bounds.height() + Ascent > CurrentHeight)
			{
				const int NeededSpace = Image.Bounds.height() + Ascent - (CurrentHeight - CurrentY);
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

		if (CurrentWidth <= MaxWidth)
		{
			ImageWidth = CurrentWidth;
			break;
		}

		CurrentHeight += NextHeightIncrease;
	}

	QImage PainterImage(ImageWidth + 40, CurrentHeight + 40, QImage::Format_ARGB32);
	PainterImage.fill(lcQColorFromRGBA(BackgroundColor));

	QPainter Painter(&PainterImage);
	Painter.setFont(Font);
	Painter.setPen(TextColor);

	for (const lcPartsListImage& Image : Images)
	{
		const QPoint Position = Image.Position + QPoint(20, 20);
		Painter.drawImage(Position, Image.Thumbnail, Image.Bounds);
		Painter.drawText(QPoint(Position.x(), Position.y() + Image.Bounds.height() + Ascent), QString::number(Image.Count) + 'x');
	}

	Painter.end();

	return PainterImage;
}

void lcModel::SaveStepImages(const QString& BaseName, bool AddStepSuffix, bool Zoom, int Width, int Height, lcStep Start, lcStep End)
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

		QImage Image = GetStepImage(Zoom, Width, Height, Step);
		if (!Writer.write(Image))
		{
			QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
			break;
		}
	}
}

void lcModel::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if ((Piece->IsSelected() || Piece->IsVisible(mCurrentStep)) && (!ObjectRayTest.IgnoreSelected || !Piece->IsSelected()))
			Piece->RayTest(ObjectRayTest);

	if (ObjectRayTest.PiecesOnly)
		return;

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (Camera.get() != ObjectRayTest.ViewCamera && Camera->IsVisible() && (!ObjectRayTest.IgnoreSelected || !Camera->IsSelected()))
			Camera->RayTest(ObjectRayTest);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (Light->IsVisible() && (!ObjectRayTest.IgnoreSelected || !Light->IsSelected()))
			Light->RayTest(ObjectRayTest);
}

void lcModel::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected() || Piece->IsVisible(mCurrentStep))
			Piece->BoxTest(ObjectBoxTest);

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (Camera.get() != ObjectBoxTest.ViewCamera && Camera->IsVisible())
			Camera->BoxTest(ObjectBoxTest);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (Light->IsVisible())
			Light->BoxTest(ObjectBoxTest);
}

bool lcModel::SubModelMinIntersectDist(const lcVector3& WorldStart, const lcVector3& WorldEnd, float& MinDistance, lcPieceInfoRayTest& PieceInfoRayTest) const
{
	bool MinIntersect = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		const lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(Piece->mModelWorld);
		const lcVector3 Start = lcMul31(WorldStart, InverseWorldMatrix);
		const lcVector3 End = lcMul31(WorldEnd, InverseWorldMatrix);

		if (Piece->IsVisibleInSubModel())
		{
			if (Piece->mPieceInfo->MinIntersectDist(Start, End, MinDistance, PieceInfoRayTest)) // todo: this should check for piece->mMesh first
			{
				MinIntersect = true;
				PieceInfoRayTest.Transform = lcMul(PieceInfoRayTest.Transform, Piece->mModelWorld);
			}
		}
	}

	return MinIntersect;
}

bool lcModel::SubModelBoxTest(const lcVector4 Planes[6]) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsVisibleInSubModel() && Piece->mPieceInfo->BoxTest(Piece->mModelWorld, Planes))
			return true;

	return false;
}

void lcModel::SubModelCompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsVisibleInSubModel())
			Piece->SubModelCompareBoundingBox(WorldMatrix, Min, Max);
}

void lcModel::SubModelAddBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsVisibleInSubModel())
			Piece->SubModelAddBoundingBoxPoints(WorldMatrix, Points);
}

void lcModel::AddSelectionHistory(std::function<void()> Callback)
{
	std::unique_ptr<lcModelHistorySelect> ModelHistorySelect = std::make_unique<lcModelHistorySelect>();

	ModelHistorySelect->SaveStartState(this);

	Callback();

	ModelHistorySelect->SaveEndState(this);

	if (ModelHistorySelect->StateChanged())
		mHistorySequence.emplace_back(std::move(ModelHistorySelect));
}

void lcModel::ClearSelection()
{
	AddSelectionHistory([this](){ DeselectAllObjects(); });
}

void lcModel::SetFocus(lcObject* FocusObject, uint32_t FocusSection, lcSelectionMode SelectionMode)
{
	AddSelectionHistory([this, FocusObject, FocusSection, SelectionMode](){ SetFocusedObject(FocusObject, FocusSection, SelectionMode); });
}

void lcModel::SetSelectionAndFocus(const std::vector<lcObject*>& Objects, lcObject* FocusObject, uint32_t FocusSection, lcSelectionMode SelectionMode)
{
	AddSelectionHistory([this, &Objects, FocusObject, FocusSection, SelectionMode]()
	{
		DeselectAllObjects();
		SetObjectsSelected(Objects, true);
		SetFocusedObject(FocusObject, FocusSection, SelectionMode);
	});
}

void lcModel::SelectAllPieces()
{
	AddSelectionHistory([this]()
	{
		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep))
				Piece->SetSelected(true);
	});
}

void lcModel::InvertPieceSelection()
{
	AddSelectionHistory([this]()
	{
		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			if (Piece->IsSelected() || Piece->IsVisible(mCurrentStep))
				Piece->SetSelected(!Piece->IsSelected());
	});
}

void lcModel::AddToSelection(const std::vector<lcObject*>& Objects)
{
	AddSelectionHistory([this, &Objects](){ SetObjectsSelected(Objects, true); });
}

void lcModel::RemoveFromSelection(const std::vector<lcObject*>& Objects)
{
	AddSelectionHistory([this, &Objects](){ SetObjectsSelected(Objects, false); });
}

template<typename StateType, typename ObjectType>
void lcModel::LoadObjectHistoryState(const std::vector<StateType>& ObjectStates, std::vector<std::unique_ptr<ObjectType>>& Objects)
{
	std::vector<std::unique_ptr<ObjectType>> NewObjects;

	NewObjects.reserve(ObjectStates.size());

	for (const StateType& ObjectState : ObjectStates)
	{
		auto ObjectId = ObjectState.Id;
		bool Found = false;

		for (auto ObjectIt = Objects.begin(); ObjectIt != Objects.end(); ++ObjectIt)
		{
			ObjectType* Object = ObjectIt->get();

			if (Object->GetId() == ObjectId)
			{
				NewObjects.emplace_back(std::move(*ObjectIt));
				Objects.erase(ObjectIt);
				Found = true;
				break;
			}
		}

		if (!Found)
			NewObjects.emplace_back(new ObjectType());
	}

	if constexpr(std::is_same_v<ObjectType, lcCamera>)
		for (const std::unique_ptr<lcCamera>& Camera : Objects)
			RemoveCameraFromViews(Camera.get());

	Objects = std::move(NewObjects);

	for (size_t ObjectIndex = 0; ObjectIndex < Objects.size(); ObjectIndex++)
		Objects[ObjectIndex]->SetHistoryState(ObjectStates[ObjectIndex], this);
}

void lcModel::LoadEditHistoryState(const lcModelHistoryEditState& HistoryState)
{
	LoadObjectHistoryState(HistoryState.Groups, mGroups);
	LoadObjectHistoryState(HistoryState.Pieces, mPieces);
	LoadObjectHistoryState(HistoryState.Cameras, mCameras);
	LoadObjectHistoryState(HistoryState.Lights, mLights);
}

void lcModel::BeginEditHistory(lcModelHistoryEditMerge ModelHistoryEditMerge)
{
	std::unique_ptr<lcModelHistoryEdit> ModelHistoryEdit = std::make_unique<lcModelHistoryEdit>(ModelHistoryEditMerge);

	ModelHistoryEdit->SaveStartState(this);

	mHistorySequence.emplace_back(std::move(ModelHistoryEdit));
}

void lcModel::EndEditHistory()
{
	if (mHistorySequence.empty())
		return;

	lcModelHistoryEdit* ModelHistoryEdit = dynamic_cast<lcModelHistoryEdit*>(mHistorySequence.back().get());

	if (!ModelHistoryEdit)
		return;

	ModelHistoryEdit->SaveEndState(this);

	if (!ModelHistoryEdit->StateChanged())
		mHistorySequence.pop_back();
}

void lcModel::LoadModelPropertiesState(const lcModelProperties& ModelProperties)
{
	mProperties = ModelProperties;

	if (gMainWindow)
		gMainWindow->GetPreviewWidget()->UpdatePreview();
}

void lcModel::SetModelProperties(const lcModelProperties& ModelProperties)
{
	std::unique_ptr<lcModelHistoryProperties> ModelHistoryProperties = std::make_unique<lcModelHistoryProperties>();
	ModelHistoryProperties->SaveStartState(this);

	LoadModelPropertiesState(ModelProperties);

	ModelHistoryProperties->SaveEndState(this);

	if (ModelHistoryProperties->StateChanged())
		mHistorySequence.emplace_back(std::move(ModelHistoryProperties));
}

void lcModel::RunHistorySequence(const std::vector<std::unique_ptr<lcModelHistory>>& HistorySequence, bool Apply)
{
	bool SelectionChanged = false;

	auto RunAction=[this, &SelectionChanged](const lcModelHistory* ModelHistory, bool Apply)
	{
		if (!ModelHistory)
			return;

		if (Apply)
			ModelHistory->LoadEndState(this);
		else
			ModelHistory->LoadStartState(this);

		if (dynamic_cast<const lcModelHistorySelect*>(ModelHistory))
		{
			SelectionChanged = true;
		}
		else if (dynamic_cast<const lcModelHistoryEdit*>(ModelHistory))
		{
			SetCurrentStep(mCurrentStep);
		}
		else if (dynamic_cast<const lcModelHistoryProperties*>(ModelHistory))
		{
		}
	};

	if (Apply)
	{
		for (auto ModelHistory = HistorySequence.begin(); ModelHistory != HistorySequence.end(); ++ModelHistory)
			RunAction(ModelHistory->get(), true);
	}
	else
	{
		for (auto ModelHistory = HistorySequence.rbegin(); ModelHistory != HistorySequence.rend(); ++ModelHistory)
			RunAction(ModelHistory->get(), false);
	}

	if (SelectionChanged)
		gMainWindow->UpdateSelectedObjects(true);

	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateTimeline(true, false);

	UpdateAllViews();
}

void lcModel::BeginHistorySequence()
{
	mHistorySequence.clear();
}

void lcModel::EndHistorySequence(const QString& Description)
{
	if (mHistorySequence.empty())
		return;

	if (mIsPreview)
	{
		mHistorySequence.clear();

		return;
	}

	bool CanMerge = false;

	if (mHistorySequence.size() == 1 && !mUndoHistory.empty() && mUndoHistory.front()->HistorySequence.size() == 1)
		CanMerge = mHistorySequence.front()->CanMergeWith(mUndoHistory.front()->HistorySequence.front().get());

	if (!CanMerge)
	{
		std::unique_ptr<lcModelHistoryEntry> ModelHistoryEntry = std::make_unique<lcModelHistoryEntry>(lcModelHistoryEntry());

		ModelHistoryEntry->Description = Description;
		ModelHistoryEntry->HistorySequence = std::move(mHistorySequence);

		mUndoHistory.insert(mUndoHistory.begin(), std::move(ModelHistoryEntry));
	}
	else
	{
		lcModelHistoryEdit* ModelHistoryEdit = dynamic_cast<lcModelHistoryEdit*>(mHistorySequence.front().get());
		lcModelHistoryEntry* LastHistoryEntry = mUndoHistory.front().get();
		lcModelHistoryEdit* LastModelHistoryEdit = dynamic_cast<lcModelHistoryEdit*>(LastHistoryEntry->HistorySequence.front().get());

		LastModelHistoryEdit->MergeWith(ModelHistoryEdit);

		mHistorySequence.clear();
	}

	mRedoHistory.clear();

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(!mUndoHistory.empty() ? mUndoHistory.front()->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory.front()->Description : nullptr);
	gMainWindow->UpdateTimeline(true, false);

	for (const std::unique_ptr<lcModelHistory>& ModelHistory : mUndoHistory.front()->HistorySequence)
	{
		if (dynamic_cast<const lcModelHistorySelect*>(ModelHistory.get()))
		{
			gMainWindow->UpdateSelectedObjects(true);
			break;
		}
	}

	UpdateAllViews();
}

void lcModel::DiscardHistorySequence()
{
	mHistorySequence.clear();
}

void lcModel::RevertHistorySequence()
{
	RunHistorySequence(mHistorySequence, false);

	mHistorySequence.clear();
}

bool lcModel::IsModified() const
{
	const lcModelHistoryEntry* FirstModifyAction = GetFirstUndoChange();

	if (!FirstModifyAction)
		return mSavedHistory != nullptr;
	else
		return mSavedHistory != FirstModifyAction;
}

void lcModel::SetSaved()
{
	mSavedHistory = GetFirstUndoChange();
}

void lcModel::RemoveFirstUndoIfUnchanged()
{
	if (mUndoHistory.empty())
		return;

	for (const std::unique_ptr<lcModelHistory>& ModelHistory : mUndoHistory.front()->HistorySequence)
		if (ModelHistory->StateChanged())
			return;

	mUndoHistory.erase(mUndoHistory.begin());

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(!mUndoHistory.empty() ? mUndoHistory.front()->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory.front()->Description : nullptr);
}

const lcModelHistoryEntry* lcModel::GetFirstUndoChange() const
{
	for (const std::unique_ptr<lcModelHistoryEntry>& UndoEntry : mUndoHistory)
		if (UndoEntry->HistorySequence.size() != 1 || !dynamic_cast<lcModelHistorySelect*>(UndoEntry->HistorySequence.front().get()))
			return UndoEntry.get();

	return nullptr;
}

void lcModel::SetActive(bool Active)
{
	CalculateStep(Active ? mCurrentStep : LC_STEP_MAX);
	mActive = Active;
}

void lcModel::CalculateStep(lcStep Step)
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Piece->UpdatePosition(Step);

	for (std::unique_ptr<lcCamera>& Camera : mCameras)
		Camera->UpdatePosition(Step);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		Light->UpdatePosition(Step);
}

void lcModel::SetCurrentStep(lcStep Step)
{
	mCurrentStep = Step;
	CalculateStep(Step);

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
	UpdateAllViews();
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
	const lcStep LastStep = GetLastStep();

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

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Step = lcMax(Step, Piece->GetStepShow());

	return Step;
}

void lcModel::InsertStep(lcStep Step)
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Piece->InsertTime(Step, 1);

	for (std::unique_ptr<lcCamera>& Camera : mCameras)
		Camera->InsertTime(Step, 1);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		Light->InsertTime(Step, 1);
}

void lcModel::InsertStepAction(lcStep Step)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	InsertStep(Step);

	EndEditHistory();
	EndHistorySequence(tr("Insert Step"));

	SetCurrentStep(mCurrentStep);
}

void lcModel::RemoveStepAction(lcStep Step)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Piece->RemoveTime(Step, 1);

	for (std::unique_ptr<lcCamera>& Camera : mCameras)
		Camera->RemoveTime(Step, 1);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		Light->RemoveTime(Step, 1);

	EndEditHistory();
	EndHistorySequence(tr("Remove Step"));

	SetCurrentStep(mCurrentStep);
}

lcGroup* lcModel::AddGroup(const QString& Prefix, lcGroup* Parent)
{
	lcGroup* Group = new lcGroup();
	mGroups.emplace_back(Group);

	Group->mName = GetGroupName(Prefix);
	Group->mGroup = Parent;

	return Group;
}

lcGroup* lcModel::GetGroup(const QString& Name, bool CreateIfMissing)
{
	for (const std::unique_ptr<lcGroup>& Group : mGroups)
		if (Group->mName == Name)
			return Group.get();

	if (CreateIfMissing)
	{
		lcGroup* Group = new lcGroup();
		Group->mName = Name;
		mGroups.emplace_back(Group);

		return Group;
	}

	return nullptr;
}

void lcModel::RemoveGroup(lcGroup* Group)
{
	for (std::vector<std::unique_ptr<lcGroup>>::iterator GroupIt = mGroups.begin(); GroupIt != mGroups.end(); GroupIt++)
	{
		if (GroupIt->get() == Group)
		{
			mGroups.erase(GroupIt);
			break;
		}
	}
}

void lcModel::GroupSelection()
{
	if (!AnyPiecesSelected())
	{
		QMessageBox::information(gMainWindow, tr("Group Selection"), tr("No pieces selected."));
		return;
	}

	lcGroupDialog Dialog(gMainWindow, GetGroupName(tr("Group #")), mGroups);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	lcGroup* NewGroup = GetGroup(Dialog.mName, true);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
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

	EndEditHistory();
	EndHistorySequence(tr("Group"));

	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::UngroupSelection()
{
	if (!AnyPiecesSelected())
	{
		QMessageBox::information(gMainWindow, tr("Ungroup Selection"), tr("No pieces selected."));
		return;
	}

	std::set<lcGroup*> SelectedGroups;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected())
		{
			lcGroup* Group = Piece->GetTopGroup();

			if (SelectedGroups.insert(Group).second)
			{
				for (std::vector<std::unique_ptr<lcGroup>>::iterator GroupIt = mGroups.begin(); GroupIt != mGroups.end(); GroupIt++)
				{
					if (GroupIt->get() == Group)
					{
						GroupIt->release();
						mGroups.erase(GroupIt);
						break;
					}
				}
			}
		}
	}

	if (SelectedGroups.empty())
	{
		DiscardHistorySequence();

		QMessageBox::information(gMainWindow, tr("Ungroup Selection"), tr("No groups selected."));

		return;
	}

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		lcGroup* Group = Piece->GetGroup();

		if (SelectedGroups.find(Group) != SelectedGroups.end())
			Piece->SetGroup(nullptr);
	}

	for (const std::unique_ptr<lcGroup>& Group : mGroups)
		if (SelectedGroups.find(Group->mGroup) != SelectedGroups.end())
			Group->mGroup = nullptr;

	for (lcGroup* Group : SelectedGroups)
		delete Group;

	RemoveEmptyGroups();

	EndEditHistory();
	EndHistorySequence(tr("Ungroup"));

	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::AddSelectedPiecesToGroup()
{
	lcGroup* Group = nullptr;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected())
		{
			Group = Piece->GetTopGroup();
			if (Group)
				break;
		}
	}

	if (!Group)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsFocused())
		{
			Piece->SetGroup(Group);
			break;
		}
	}

	RemoveEmptyGroups();

	EndEditHistory();
	EndHistorySequence(tr("Group"));

	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::RemoveFocusPieceFromGroup()
{
	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsFocused())
		{
			Piece->SetGroup(nullptr);

			Modified = true;

			break;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	RemoveEmptyGroups();

	EndEditHistory();
	EndHistorySequence(tr("Ungroup"));
}

void lcModel::ShowEditGroupsDialog()
{
	lcEditGroupsDialog Dialog(gMainWindow, this);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::function<void(const lcEditGroupsDialog::GroupInfo&, lcGroup*)> UpdateGroups=[this, &UpdateGroups](const lcEditGroupsDialog::GroupInfo& GroupInfo, lcGroup* ParentGroup)
	{
		lcGroup* Group = GroupInfo.Group;

		if (!Group)
		{
			Group = new lcGroup();
			mGroups.emplace_back(Group);
		}

		Group->mName = GroupInfo.Name;
		Group->mGroup = ParentGroup;

		for (const lcEditGroupsDialog::GroupInfo& ChildGroupInfo : GroupInfo.ChildGroups)
			UpdateGroups(ChildGroupInfo, Group);

		for (lcPiece* Piece : GroupInfo.ChildPieces)
			Piece->SetGroup(Group);
	};

	lcEditGroupsDialog::GroupInfo GroupInfo = Dialog.GetGroups();

	for (const lcEditGroupsDialog::GroupInfo& ChildGroupInfo : GroupInfo.ChildGroups)
		UpdateGroups(ChildGroupInfo, nullptr);

	for (lcPiece* Piece : GroupInfo.ChildPieces)
		Piece->SetGroup(nullptr);

	RemoveEmptyGroups();

	EndEditHistory();

	if (mHistorySequence.empty())
	{
		DiscardHistorySequence();
		return;
	}

	ClearSelection();

	EndHistorySequence(tr("Edit Groups"));

	gMainWindow->UpdateSelectedObjects(true);
}

QString lcModel::GetGroupName(const QString& Prefix)
{
	const qsizetype Length = Prefix.length();
	int Max = 0;

	for (const std::unique_ptr<lcGroup>& Group : mGroups)
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

		for (std::vector<std::unique_ptr<lcGroup>>::iterator GroupIt = mGroups.begin(); GroupIt != mGroups.end();)
		{
			lcGroup* Group = GroupIt->get();
			int Ref = 0;

			for (const std::unique_ptr<lcPiece>& Piece : mPieces)
				if (Piece->GetGroup() == Group)
					Ref++;

			for (size_t ParentIdx = 0; ParentIdx < mGroups.size(); ParentIdx++)
				if (mGroups[ParentIdx]->mGroup == Group)
					Ref++;

			if (Ref > 1)
			{
				GroupIt++;
				continue;
			}

			if (Ref != 0)
			{
				for (const std::unique_ptr<lcPiece>& Piece : mPieces)
				{
					if (Piece->GetGroup() == Group)
					{
						Piece->SetGroup(Group->mGroup);
						break;
					}
				}

				for (size_t ParentIdx = 0; ParentIdx < mGroups.size(); ParentIdx++)
				{
					if (mGroups[ParentIdx]->mGroup == Group)
					{
						mGroups[ParentIdx]->mGroup = Group->mGroup;
						break;
					}
				}
			}

			GroupIt = mGroups.erase(GroupIt);
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
		const float Leftover = NewDistance[2] - (SnapZ * i);

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
	const float AngleSnap = gMainWindow->GetAngleSnap();
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
		const lcObject* Focus = GetFocusObject();

		if (Focus)
		{
			if (Focus->IsPiece())
				return ((lcPiece*)Focus)->GetRelativeRotation();

			if (Focus->IsLight())
				return ((lcLight*)Focus)->GetRelativeRotation();
		}
	}

	return lcMatrix33Identity();
}

lcPiece* lcModel::AddPiece(PieceInfo* Info, quint32 Section)
{
	if (!Info)
		Info = gMainWindow->GetCurrentPieceInfo();

	if (!Info)
		return nullptr;

	lcPiece* Last = mPieces.empty() ? nullptr : mPieces.back().get();

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsFocused())
		{
			Last = Piece.get();
			break;
		}
	}

	const lcBoundingBox& PieceInfoBoundingBox = Info->GetBoundingBox();
	lcPiece* Piece = nullptr;

	auto CreatePiece=[this, &Piece](PieceInfo* Info, const lcMatrix44& WorldMatrix, int ColorIndex)
	{
		Piece = new lcPiece(Info);
		Piece->Initialize(WorldMatrix, mCurrentStep);
		Piece->SetColorIndex(ColorIndex);
		AddPiece(Piece);
	};

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	if (Last)
	{
		std::vector<lcInsertPieceInfo> TrainTracks;

		if (Info->GetTrainTrackInfo() && Last->mPieceInfo->GetTrainTrackInfo())
		{
			quint32 FocusSection = Last->GetFocusSection();

			if (FocusSection != LC_PIECE_SECTION_INVALID && FocusSection >= LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
				Section = FocusSection;

			if (!Last->IsFocused())
				UpdateTrainTrackConnections(Last, false);

			TrainTracks = lcTrainTrackInfo::GetInsertPieceInfo(Last, Info, nullptr, gMainWindow->mColorIndex, Section, true, std::nullopt);

			for (const lcInsertPieceInfo& TrainTrack : TrainTracks)
				CreatePiece(TrainTrack.Info, TrainTrack.Transform, TrainTrack.ColorIndex);
		}

		if (TrainTracks.empty())
		{
			const lcBoundingBox& LastBoundingBox = Last->GetBoundingBox();
			lcVector3 Dist(0, 0, LastBoundingBox.Max.z - PieceInfoBoundingBox.Min.z);
			Dist = SnapPosition(Dist);

			lcMatrix44 WorldMatrix = Last->mModelWorld;
			WorldMatrix.SetTranslation(lcMul31(Dist, Last->mModelWorld));

			CreatePiece(Info, WorldMatrix, gMainWindow->mColorIndex);
		}
	}
	else
	{
		lcMatrix44 WorldMatrix = lcMatrix44Translation(lcVector3(0.0f, 0.0f, -PieceInfoBoundingBox.Min.z));

		CreatePiece(Info, WorldMatrix, gMainWindow->mColorIndex);
	}

	if (!Piece)
	{
		DiscardHistorySequence();

		return nullptr;
	}

	EndEditHistory();

	SetSelectionAndFocus(std::vector<lcObject*>(), Piece, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(tr("Add Piece"));

	gMainWindow->UpdateTimeline(false, false);

	return Piece;
}

void lcModel::AddPiece(std::unique_ptr<lcPiece> Piece, size_t PieceIndex)
{
	const PieceInfo* Info = Piece->mPieceInfo;

	if (!Info->IsModel())
	{
		const lcMesh* Mesh = Info->GetMesh();

		if (Mesh && Mesh->mVertexCacheOffset == -1)
			lcGetPiecesLibrary()->mBuffersDirty = true;
	}

	mPieces.insert(mPieces.begin() + PieceIndex, std::move(Piece));
}

size_t lcModel::AddPiece(lcPiece* Piece)
{
	size_t PieceIndex;

	for (PieceIndex = 0; PieceIndex < mPieces.size(); PieceIndex++)
	{
		if (mPieces[PieceIndex]->GetStepShow() > Piece->GetStepShow())
		{
			AddPiece(std::unique_ptr<lcPiece>(Piece), PieceIndex);

			return PieceIndex;
		}
	}

	AddPiece(std::unique_ptr<lcPiece>(Piece), PieceIndex);

	return PieceIndex;
}

void lcModel::FocusNextTrainTrack()
{
	const lcObject* Focus = GetFocusObject();

	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* FocusPiece = (lcPiece*)Focus;
	const lcTrainTrackInfo* TrainTrackInfo = FocusPiece->mPieceInfo->GetTrainTrackInfo();

	if (!TrainTrackInfo)
		return;

	quint32 FocusSection = FocusPiece->GetFocusSection();
	int ConnectionIndex = 0;

	if (FocusSection != LC_PIECE_SECTION_INVALID && FocusSection != LC_PIECE_SECTION_POSITION)
	{
		ConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
		ConnectionIndex = (ConnectionIndex + 1) % TrainTrackInfo->GetConnections().size();
	}

	BeginHistorySequence();
	SetFocus(FocusPiece, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex, lcSelectionMode::Single);
	EndHistorySequence(tr("Selection"));
}

void lcModel::FocusPreviousTrainTrack()
{
	const lcObject* Focus = GetFocusObject();

	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* FocusPiece = (lcPiece*)Focus;
	const lcTrainTrackInfo* TrainTrackInfo = FocusPiece->mPieceInfo->GetTrainTrackInfo();

	if (!TrainTrackInfo)
		return;

	quint32 FocusSection = FocusPiece->GetFocusSection();
	int ConnectionIndex = 0;

	if (FocusSection != LC_PIECE_SECTION_INVALID && FocusSection != LC_PIECE_SECTION_POSITION)
	{
		ConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
		ConnectionIndex = (ConnectionIndex + static_cast<int>(TrainTrackInfo->GetConnections().size()) - 1) % TrainTrackInfo->GetConnections().size();
	}

	BeginHistorySequence();
	SetFocus(FocusPiece, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + ConnectionIndex, lcSelectionMode::Single);
	EndHistorySequence(tr("Selection"));
}

void lcModel::RotateFocusedTrainTrack(int Direction)
{
	const lcObject* Focus = GetFocusObject();

	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* FocusPiece = (lcPiece*)Focus;
	const lcTrainTrackInfo* TrainTrackInfo = FocusPiece->mPieceInfo->GetTrainTrackInfo();

	if (!TrainTrackInfo)
		return;

	quint32 FocusSection = FocusPiece->GetFocusSection();
	std::optional<lcMatrix44> Transform;
	int FirstConnectionIndex = 0, RotateConnectionIndex = 0, TracksConnected = 0;

	for (int ConnectionIndex = 0; ConnectionIndex < static_cast<int>(TrainTrackInfo->GetConnections().size()); ConnectionIndex++)
	{
		if (FocusPiece->IsTrainTrackConnected(ConnectionIndex))
		{
			if (!TracksConnected)
				FirstConnectionIndex = ConnectionIndex;

			TracksConnected++;
		}
	}

	if (FocusSection != LC_PIECE_SECTION_INVALID && FocusSection != LC_PIECE_SECTION_POSITION)
	{
		RotateConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}
	else
	{
		RotateConnectionIndex = FirstConnectionIndex;

		if (TracksConnected > 1)
		{
			// todo: find most recent connection
		}
	}

	lcMatrix44 ConnectionTransform = lcMul(TrainTrackInfo->GetConnections()[RotateConnectionIndex].Transform, FocusPiece->mModelWorld);
	int NewConnectionIndex = (RotateConnectionIndex + Direction + static_cast<int>(TrainTrackInfo->GetConnections().size())) % TrainTrackInfo->GetConnections().size();

	Transform = lcTrainTrackInfo::CalculateTransformToConnection(ConnectionTransform, FocusPiece->mPieceInfo, NewConnectionIndex);

	if (!Transform)
		return;

	BeginHistorySequence();

	BeginEditHistory(lcModelHistoryEditMerge::None);

	FocusPiece->SetPosition(Transform.value().GetTranslation(), mCurrentStep, gMainWindow->GetAddKeys());
	FocusPiece->SetRotation(lcMatrix33(Transform.value()), mCurrentStep, gMainWindow->GetAddKeys());
	FocusPiece->UpdatePosition(mCurrentStep);

	EndEditHistory();

	if ((FocusSection != LC_PIECE_SECTION_INVALID && FocusSection != LC_PIECE_SECTION_POSITION) || !TracksConnected)
		SetFocus(FocusPiece, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + NewConnectionIndex, lcSelectionMode::Single);

	EndHistorySequence(tr("Rotate"));

	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::UpdateTrainTrackConnections(lcPiece* TrackPiece, bool IgnoreSelected) const
{
	if (!TrackPiece)
		return;

	const lcTrainTrackInfo* TrainTrackInfo = TrackPiece->mPieceInfo->GetTrainTrackInfo();

	if (!TrainTrackInfo)
		return;

	const int ConnectionCount = static_cast<int>(TrainTrackInfo->GetConnections().size());
	std::vector<bool> Connections(ConnectionCount, false);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece.get() == TrackPiece || !Piece->mPieceInfo->GetTrainTrackInfo() || (IgnoreSelected && Piece->IsSelected()))
			continue;

		for (int ConnectionIndex = 0; ConnectionIndex < ConnectionCount; ConnectionIndex++)
			if (!Connections[ConnectionIndex] && lcTrainTrackInfo::GetPieceConnectionIndex(TrackPiece, ConnectionIndex, Piece.get()) != -1)
				Connections[ConnectionIndex] = true;
	}

	TrackPiece->SetTrainTrackConnections(std::move(Connections));
}

void lcModel::UpdateSelectedPiecesTrainTrackConnections()
{
	std::vector<lcPiece *> TrackPieces;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->mPieceInfo->GetTrainTrackInfo() && Piece->IsSelected())
			TrackPieces.push_back(Piece.get());

	for (lcPiece* TrackPiece : TrackPieces)
	{
		const lcTrainTrackInfo* TrainTrackInfo = TrackPiece->mPieceInfo->GetTrainTrackInfo();
		const int ConnectionCount = static_cast<int>(TrainTrackInfo->GetConnections().size());
		std::vector<bool> Connections(ConnectionCount, false);

		for (const lcPiece* CheckPiece : TrackPieces)
		{
			if (CheckPiece == TrackPiece)
				continue;

			for (int ConnectionIndex = 0; ConnectionIndex < ConnectionCount; ConnectionIndex++)
				if (!Connections[ConnectionIndex] && lcTrainTrackInfo::GetPieceConnectionIndex(TrackPiece, ConnectionIndex, CheckPiece) != -1)
					Connections[ConnectionIndex] = true;
		}

		TrackPiece->SetTrainTrackConnections(std::move(Connections));
	}
}

void lcModel::DeleteSelectedObjects()
{
	if (mIsPreview)
	{
		RemoveSelectedObjects();

		return;
	}

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = RemoveSelectedObjects();

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Delete"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
	gMainWindow->UpdateInUseCategory();
}

void lcModel::ResetSelectedPiecesPivotPoint()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected())
			Piece->ResetPivotPoint();

	EndEditHistory();
	EndHistorySequence(tr("Reset Pivot Point"));
}

void lcModel::RemoveSelectedObjectsKeyFrames()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected())
			Piece->RemoveKeyFrames();

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (Camera->IsSelected())
			Camera->RemoveKeyFrames();

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (Light->IsSelected())
			Light->RemoveKeyFrames();

	EndEditHistory();
	EndHistorySequence(tr("Remove Key Frames"));
}

void lcModel::InsertControlPoint()
{
	lcPiece* Piece = dynamic_cast<lcPiece*>(GetFocusObject());

	if (!Piece)
		return;

	lcVector3 Start, End;

	gMainWindow->GetActiveView()->GetRayUnderPointer(Start, End);

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = Piece->InsertControlPoint(Start, End);

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Add Control Point"));

	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::RemoveFocusedControlPoint()
{
	lcPiece* Piece = dynamic_cast<lcPiece*>(GetFocusObject());

	if (!Piece)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = Piece->RemoveFocusedControlPoint();

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Remove Control Point"));

	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::ShowSelectedPiecesEarlier()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcPiece*> MovedPieces;

	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); )
	{
		lcPiece* Piece = PieceIt->get();

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step > 1)
			{
				Step--;
				Piece->SetStepShow(Step);

				MovedPieces.emplace_back(PieceIt->release());
				PieceIt = mPieces.erase(PieceIt);
				continue;
			}
		}

		PieceIt++;
	}

	if (MovedPieces.empty())
	{
		DiscardHistorySequence();

		return;
	}

	for (lcPiece* Piece : MovedPieces)
	{
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	EndEditHistory();
	EndHistorySequence(tr("Show Earlier"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::ShowSelectedPiecesLater()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcPiece*> MovedPieces;

	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); )
	{
		lcPiece* Piece = PieceIt->get();

		if (Piece->IsSelected())
		{
			lcStep Step = Piece->GetStepShow();

			if (Step < LC_STEP_MAX)
			{
				Step++;
				Piece->SetStepShow(Step);

				MovedPieces.emplace_back(PieceIt->release());
				PieceIt = mPieces.erase(PieceIt);

				continue;
			}
		}

		PieceIt++;
	}

	if (MovedPieces.empty())
	{
		DiscardHistorySequence();

		return;
	}

	for (lcPiece* Piece : MovedPieces)
	{
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	EndEditHistory();
	EndHistorySequence(tr("Show Later"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::InsertStepAndMoveSelectedPieces(lcStep Step)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	InsertStep(Step);

	std::vector<lcPiece*> MovedPieces;

	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); )
	{
		lcPiece* Piece = PieceIt->get();

		if (Piece->IsSelected())
		{
			Piece->SetStepShow(Step);

			MovedPieces.emplace_back(PieceIt->release());
			PieceIt = mPieces.erase(PieceIt);

			continue;
		}

		PieceIt++;
	}

	for (lcPiece* Piece : MovedPieces)
	{
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	EndEditHistory();
	EndHistorySequence(tr("Show Pieces"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::SetPieceSteps(const std::vector<std::pair<lcPiece*, lcStep>>& PieceSteps)
{
	if (PieceSteps.size() != mPieces.size())
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (size_t PieceIdx = 0; PieceIdx < PieceSteps.size(); PieceIdx++)
	{
		const std::pair<lcPiece*, lcStep>& PieceStep = PieceSteps[PieceIdx];
		lcPiece* Piece = mPieces[PieceIdx].get();

		if (Piece != PieceStep.first || Piece->GetStepShow() != PieceStep.second)
		{
			Piece = PieceStep.first;
			const lcStep Step = PieceStep.second;

			mPieces[PieceIdx].release();
			mPieces[PieceIdx] = std::unique_ptr<lcPiece>(Piece);
			Piece->SetStepShow(Step);

			Modified = true;
		}
	}

	if (Modified)
	{
		EndEditHistory();
		EndHistorySequence(tr("Change Step"));

		gMainWindow->UpdateTimeline(false, false);
		gMainWindow->UpdateSelectedObjects(false);
	}
	else
	{
		DiscardHistorySequence();
	}
}

void lcModel::RenamePiece(PieceInfo* Info)
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->mPieceInfo == Info)
			Piece->UpdateID();
}

void lcModel::MoveSelectionToModel(lcModel* Model)
{
	if (!Model)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcPiece*> Pieces;
	lcPiece* ModelPiece = nullptr;
	lcStep FirstStep = LC_STEP_MAX;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (size_t PieceIndex = 0; PieceIndex < mPieces.size(); )
	{
		lcPiece* Piece = mPieces[PieceIndex].get();

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			mPieces[PieceIndex].release();
			mPieces.erase(mPieces.begin() + PieceIndex);
			Piece->SetGroup(nullptr); // todo: copy groups
			Pieces.emplace_back(Piece);
			FirstStep = qMin(FirstStep, Piece->GetStepShow());

			if (!ModelPiece)
			{
				ModelPiece = new lcPiece(Model->mPieceInfo);
				ModelPiece->SetColorIndex(gDefaultColor);
				AddPiece(std::unique_ptr<lcPiece>(ModelPiece), PieceIndex);
				PieceIndex++;
			}
		}
		else
			PieceIndex++;
	}

	if (Pieces.empty())
	{
		DiscardHistorySequence();

		return;
	}

	lcVector3 ModelCenter = (Min + Max) / 2.0f;
	ModelCenter.z += (Min.z - Max.z) / 2.0f;

	for (lcPiece* Piece : Pieces)
	{
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

	EndEditHistory();

	gMainWindow->UpdateTimeline(false, false);

	SetSelectionAndFocus(std::vector<lcObject*>(), ModelPiece, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(tr("Move to Model"));
}

void lcModel::InlineSelectedModels()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcObject*> NewPieces;
	bool Modified = false;

	for (size_t PieceIndex = 0; PieceIndex < mPieces.size(); )
	{
		lcPiece* Piece = mPieces[PieceIndex].get();

		if (!Piece->IsSelected() || !Piece->mPieceInfo->IsModel())
		{
			PieceIndex++;
			continue;
		}

		mPieces[PieceIndex].release();
		mPieces.erase(mPieces.begin() + PieceIndex);

		lcModel* Model = Piece->mPieceInfo->GetModel();

		Modified = true;

		for (const std::unique_ptr<lcPiece>& ModelPiece : Model->mPieces)
		{
			lcPiece* NewPiece = new lcPiece(nullptr);

			// todo: recreate in groups in the current model

			int ColorIndex = ModelPiece->GetColorIndex();

			if (ColorIndex == gDefaultColor)
				ColorIndex = Piece->GetColorIndex();

			NewPiece->SetPieceInfo(ModelPiece->mPieceInfo, ModelPiece->GetID(), true, true);
			NewPiece->Initialize(lcMul(ModelPiece->mModelWorld, Piece->mModelWorld), Piece->GetStepShow());
			NewPiece->SetColorIndex(ColorIndex);
			NewPiece->UpdatePosition(mCurrentStep);

			NewPieces.emplace_back(NewPiece);
			AddPiece(std::unique_ptr<lcPiece>(NewPiece), PieceIndex);
			PieceIndex++;
		}

		delete Piece;
	}

	if (!Modified)
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No models selected."));

		DiscardHistorySequence();

		return;
	}

	EndEditHistory();

	SetSelectionAndFocus(NewPieces, nullptr, 0, lcSelectionMode::Single);

	EndHistorySequence(tr("Inline Model"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateInUseCategory();
}

void lcModel::RemoveCameraFromViews(lcCamera* Camera)
{
	std::vector<lcView*> Views = lcView::GetModelViews(this);

	for (lcView* View : Views)
		if (Camera == View->GetCamera())
			View->SetCamera(Camera, true);
}

bool lcModel::RemoveSelectedObjects()
{
	bool RemovedPiece = false;
	bool RemovedCamera = false;
	bool RemovedLight = false;

	for (std::vector<std::unique_ptr<lcPiece>>::iterator PieceIt = mPieces.begin(); PieceIt != mPieces.end(); )
	{
		lcPiece* Piece = PieceIt->get();

		if (Piece->IsSelected())
		{
			RemovedPiece = true;
			PieceIt = mPieces.erase(PieceIt);
		}
		else
			PieceIt++;
	}

	for (std::vector<std::unique_ptr<lcCamera>>::iterator CameraIt = mCameras.begin(); CameraIt != mCameras.end(); )
	{
		lcCamera* Camera = CameraIt->get();

		if (Camera->IsSelected())
		{
			RemoveCameraFromViews(Camera);

			RemovedCamera = true;
			CameraIt = mCameras.erase(CameraIt);
		}
		else
			CameraIt++;
	}

	for (std::vector<std::unique_ptr<lcLight>>::iterator LightIt = mLights.begin(); LightIt != mLights.end(); )
	{
		lcLight* Light = LightIt->get();

		if (Light->IsSelected())
		{
			RemovedLight = true;
			LightIt = mLights.erase(LightIt);
		}
		else
			LightIt++;
	}

	RemoveEmptyGroups();

	return RemovedPiece || RemovedCamera || RemovedLight;
}

void lcModel::MoveSelectedObjects(const lcVector3& PieceDistance, const lcVector3& ObjectDistance, bool AllowRelative, bool AlternateButtonDrag, bool Checkpoint, bool FirstMove, lcModelHistoryEditMerge ModelHistoryEditMerge)
{
	if (Checkpoint)
	{
		BeginHistorySequence();
		BeginEditHistory(ModelHistoryEditMerge);
	}

	bool Moved = false;
	lcMatrix33 RelativeRotation;

	if (AllowRelative)
		RelativeRotation = GetRelativeRotation();
	else
		RelativeRotation = lcMatrix33Identity();

	if (PieceDistance.LengthSquared() >= 0.001f)
	{
		lcVector3 TransformedPieceDistance = lcMul(PieceDistance, RelativeRotation);

		if (AlternateButtonDrag)
		{
			for (const std::unique_ptr<lcPiece>& Piece : mPieces)
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
			for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			{
				if (Piece->IsSelected())
				{
					if (gMainWindow->GetSeparateTransform())
						TransformedPieceDistance = lcMul(PieceDistance, Piece->GetRelativeRotation());

					Piece->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedPieceDistance);
					Piece->UpdatePosition(mCurrentStep);
					Moved = true;
				}
			}
		}
	}

	if (ObjectDistance.LengthSquared() >= 0.001f && !AlternateButtonDrag)
	{
		const lcVector3 TransformedObjectDistance = lcMul(ObjectDistance, RelativeRotation);

		for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		{
			if (Camera->IsSelected())
			{
				Camera->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance);
				Camera->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}

		for (const std::unique_ptr<lcLight>& Light : mLights)
		{
			if (Light->IsSelected())
			{
				Light->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), TransformedObjectDistance, FirstMove);
				Light->UpdatePosition(mCurrentStep);
				Moved = true;
			}
		}
	}

	if (!Moved)
	{
		if (Checkpoint)
			DiscardHistorySequence();

		return;
	}

	if (Checkpoint)
	{
		EndEditHistory();
		EndHistorySequence(tr("Move"));

		RemoveFirstUndoIfUnchanged();
	}

	UpdateAllViews();
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::RotateSelectedObjects(const lcVector3& Angles, bool Relative, bool RotatePivotPoint, bool Checkpoint, lcModelHistoryEditMerge ModelHistoryEditMerge)
{
	if (Angles.LengthSquared() < 0.001f)
		return;

	if (Checkpoint)
	{
		BeginHistorySequence();
		BeginEditHistory(ModelHistoryEditMerge);
	}

	lcMatrix33 RotationMatrix = lcMatrix33Identity();
	bool Rotated = false;

	if (Angles[0] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationX(Angles[0] * LC_DTOR), RotationMatrix);

	if (Angles[1] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationY(Angles[1] * LC_DTOR), RotationMatrix);

	if (Angles[2] != 0.0f)
		RotationMatrix = lcMul(lcMatrix33RotationZ(Angles[2] * LC_DTOR), RotationMatrix);

	if (RotatePivotPoint)
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
		int Flags;
		std::vector<lcObject*> Selection;
		lcObject* Focus;

		GetSelectionInformation(&Flags, Selection, &Focus);

		if (!gMainWindow->GetSeparateTransform())
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

			for (lcObject* Object : Selection)
			{
				if (Object->IsPiece())
				{
					lcPiece* Piece = (lcPiece*)Object;

					Piece->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
					Piece->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
				else if (Object->IsCamera())
				{
					lcCamera* Camera = (lcCamera*)Object;

					Camera->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
					Camera->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
				else if (Object->IsLight())
				{
					lcLight* Light = (lcLight*)Object;

					Light->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
					Light->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
			}
		}
		else
		{
			for (lcObject* Object : Selection)
			{
				if (Object->IsPiece())
				{
					lcPiece* Piece = (lcPiece*)Object;

					const lcVector3 Center = Piece->GetRotationCenter();
					lcMatrix33 WorldToFocusMatrix;
					lcMatrix33 RelativeRotationMatrix;

					if (Relative)
					{
						const lcMatrix33 RelativeRotation = Piece->GetRelativeRotation();
						WorldToFocusMatrix = lcMatrix33AffineInverse(RelativeRotation);
						RelativeRotationMatrix = lcMul(RotationMatrix, RelativeRotation);
					}
					else
					{
						WorldToFocusMatrix = lcMatrix33Identity();
						RelativeRotationMatrix = RotationMatrix;
					}

					Piece->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RelativeRotationMatrix, Center, WorldToFocusMatrix);
					Piece->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
				else if (Object->IsCamera())
				{
					lcCamera* Camera = (lcCamera*)Object;

					const lcVector3 Center = Camera->GetRotationCenter();
					lcMatrix33 WorldToFocusMatrix;
					lcMatrix33 RelativeRotationMatrix;

					if (Relative)
					{
						const lcMatrix33 RelativeRotation = Camera->GetRelativeRotation();
						WorldToFocusMatrix = lcMatrix33AffineInverse(RelativeRotation);
						RelativeRotationMatrix = lcMul(RotationMatrix, RelativeRotation);
					}
					else
					{
						WorldToFocusMatrix = lcMatrix33Identity();
						RelativeRotationMatrix = RotationMatrix;
					}

					Camera->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
					Camera->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
				else if (Object->IsLight())
				{
					lcLight* Light = (lcLight*)Object;

					const lcVector3 Center = Light->GetRotationCenter();
					lcMatrix33 WorldToFocusMatrix;
					lcMatrix33 RelativeRotationMatrix;

					if (Relative)
					{
						const lcMatrix33 RelativeRotation = Light->GetRelativeRotation();
						WorldToFocusMatrix = lcMatrix33AffineInverse(RelativeRotation);
						RelativeRotationMatrix = lcMul(RotationMatrix, RelativeRotation);
					}
					else
					{
						WorldToFocusMatrix = lcMatrix33Identity();
						RelativeRotationMatrix = RotationMatrix;
					}

					Light->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
					Light->UpdatePosition(mCurrentStep);
					Rotated = true;
				}
			}
		}
	}

	if (!Rotated)
	{
		if (Checkpoint)
			DiscardHistorySequence();

		return;
	}

	if (Checkpoint)
	{
		EndEditHistory();
		EndHistorySequence(tr("Rotate"));

		RemoveFirstUndoIfUnchanged();
	}

	UpdateAllViews();
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::ScaleSelectedPieces(const float Scale)
{
	if (Scale < 0.001f)
		return;

	lcObject* Focus = GetFocusObject();
	if (!Focus || !Focus->IsPiece())
		return;

	lcPiece* Piece = (lcPiece*)Focus;
	const quint32 Section = Piece->GetFocusSection();

	if (Section >= LC_PIECE_SECTION_CONTROL_POINT_FIRST && Section <= LC_PIECE_SECTION_CONTROL_POINT_LAST)
	{
		const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;
		Piece->SetControlPointScale(ControlPointIndex, Scale);
	}
}

void lcModel::TransformSelectedObjects(lcTransformType TransformType, const lcVector3& Transform)
{
	switch (TransformType)
	{
	case lcTransformType::AbsoluteTranslation:
		MoveSelectedObjects(Transform, false, false, true, true, lcModelHistoryEditMerge::None);
		break;

	case lcTransformType::RelativeTranslation:
		MoveSelectedObjects(Transform, true, false, true, true, lcModelHistoryEditMerge::None);
		break;

	case lcTransformType::AbsoluteRotation:
		RotateSelectedObjects(Transform, false, false, true, lcModelHistoryEditMerge::None);
		break;

	case lcTransformType::RelativeRotation:
		RotateSelectedObjects(Transform, true, false, true, lcModelHistoryEditMerge::None);
		break;

	case lcTransformType::Count:
		break;
	}
}

void lcModel::SetObjectsKeyFrame(const std::vector<lcObject*>& Objects, lcObjectPropertyId PropertyId, bool KeyFrame)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (lcObject* Object : Objects)
	{
		Modified |= Object->SetKeyFrame(PropertyId, mCurrentStep, KeyFrame);
		Object->UpdatePosition(mCurrentStep);
	}

	if (Modified)
	{
		EndEditHistory();
		EndHistorySequence(KeyFrame ? tr("Add KeyFrame") : tr("Remove KeyFrame"));

		gMainWindow->UpdateSelectedObjects(false);
	}
	else
	{
		DiscardHistorySequence();
	}
}

void lcModel::SetSelectedPiecesColorIndex(int ColorIndex)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() && Piece->GetColorIndex() != ColorIndex)
		{
			Piece->SetColorIndex(ColorIndex);
			Modified = true;
		}
	}

	if (Modified)
	{
		EndEditHistory();
		EndHistorySequence(tr("Paint"));

		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateTimeline(false, true);
	}
	else
	{
		DiscardHistorySequence();
	}
}

void lcModel::SetSelectedPiecesStepShow(lcStep Step)
{
	std::vector<lcPiece*> MovedPieces;

	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); )
	{
		lcPiece* Piece = PieceIt->get();

		if (Piece->IsSelected() && Piece->GetStepShow() != Step)
		{
			Piece->SetStepShow(Step);

			MovedPieces.emplace_back(PieceIt->release());
			PieceIt = mPieces.erase(PieceIt);
			continue;
		}

		PieceIt++;
	}

	if (MovedPieces.empty())
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (lcPiece* Piece : MovedPieces)
	{
		Piece->SetFileLine(-1);
		AddPiece(Piece);
	}

	EndEditHistory();
	EndHistorySequence(tr("Set Show Step"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::SetSelectedPiecesStepHide(lcStep Step)
{
	bool Modified = false;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() && Piece->GetStepHide() != Step)
		{
			Piece->SetStepHide(Step);

			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Set Hide Step"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::SetCameraProjection(lcCamera* Camera, lcCameraProjection CameraProjection)
{
	if (Camera->GetProjection() == CameraProjection)
		return;

	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::None);
	}

	Camera->SetProjection(CameraProjection);
	Camera->UpdatePosition(mCurrentStep);

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Change Projection"));
	}

	UpdateAllViews();
	gMainWindow->UpdateSelectedObjects(false);
}

void lcModel::SetObjectsProperty(const std::vector<lcObject*>& Objects, lcObjectPropertyId PropertyId, QVariant Value)
{
	BeginHistorySequence();
	BeginEditHistory(static_cast<lcModelHistoryEditMerge>(static_cast<uint32_t>(lcModelHistoryEditMerge::PropertiesEdit) | static_cast<uint32_t>(PropertyId)));

	bool Modified = false;

	for (lcObject* Object : Objects)
	{
		bool ObjectModified = Object->SetPropertyValue(PropertyId, mCurrentStep, gMainWindow->GetAddKeys(), Value);

		if (ObjectModified)
		{
			Object->UpdatePosition(mCurrentStep);
			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(lcObject::GetCheckpointString(PropertyId));

	RemoveFirstUndoIfUnchanged();

	gMainWindow->UpdateSelectedObjects(false);

	// todo: fix hacky timeline update
	if (PropertyId == lcObjectPropertyId::PieceId || PropertyId == lcObjectPropertyId::PieceColor)
	{
		gMainWindow->UpdateTimeline(false, true);
	}

	// todo: fix hacky category update
	if (PropertyId == lcObjectPropertyId::PieceId)
	{
		gMainWindow->UpdateInUseCategory();
	}
}

void lcModel::EndPropertyEdit(lcObjectPropertyId PropertyId, bool Accept)
{
	// todo: right clicking or pressing esc while dragging the spinbox doesn't cancel
	// we need to handle the shortcut override and undo the last undo history if it matches the property

	if (!Accept)
	{
		RevertHistorySequence();
		return;
	}
}

bool lcModel::AnyPiecesSelected() const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected())
			return true;

	return false;
}

bool lcModel::AnyObjectsSelected() const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected())
			return true;

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (Camera->IsSelected())
			return true;

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (Light->IsSelected())
			return true;

	return false;
}

lcObject* lcModel::GetFocusObject() const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsFocused())
			return Piece.get();

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		if (Camera->IsFocused())
			return Camera.get();

	for (const std::unique_ptr<lcLight>& Light : mLights)
		if (Light->IsFocused())
			return Light.get();

	return nullptr;
}

lcModel* lcModel::GetFirstSelectedSubmodel() const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected() && Piece->mPieceInfo->IsModel())
			return Piece->mPieceInfo->GetModel();

	return nullptr;
}

void lcModel::GetSubModels(std::set<lcModel*>& SubModels) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->mPieceInfo->IsModel())
		{
			lcModel* SubModel = Piece->mPieceInfo->GetModel();

			SubModels.insert(SubModel);
		}
	}
}

bool lcModel::GetMoveRotateTransform(lcVector3& Center, lcMatrix33& RelativeRotation) const
{
	const bool Relative = gMainWindow->GetRelativeTransform();
	int NumSelected = 0;

	Center = lcVector3(0.0f, 0.0f, 0.0f);
	RelativeRotation = lcMatrix33Identity();

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
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

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
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

	for (const std::unique_ptr<lcLight>& Light : mLights)
	{
		if (!Light->IsSelected())
			continue;

		if (Light->IsFocused())
		{
			Center = Light->GetRotationCenter();

			if (Relative)
				RelativeRotation = Light->GetRelativeRotation();

			return true;
		}

		Center += Light->GetSectionPosition(LC_LIGHT_SECTION_POSITION);
		NumSelected++;
	}

	if (NumSelected)
	{
		Center /= NumSelected;
		return true;
	}

	return false;
}

bool lcModel::CanRotateSelection() const
{
	int Flags;
	std::vector<lcObject*> Selection;
	lcObject* Focus;

	GetSelectionInformation(&Flags, Selection, &Focus);

	if (Flags & LC_SEL_PIECE)
	{
		if ((Flags & (LC_SEL_CAMERA | LC_SEL_LIGHT)) == 0)
			return true;
	}

	if ((Flags & (LC_SEL_PIECE | LC_SEL_CAMERA)) == 0)
	{
		if (Focus && Focus->IsLight())
		{
			lcLight* Light = (lcLight*)Focus;

			return (Light->GetAllowedTransforms() & LC_OBJECT_TRANSFORM_ROTATE_XYZ) != 0;
		}
	}

	return false;
}

bool lcModel::GetPieceFocusOrSelectionCenter(lcVector3& Center) const
{
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	lcPiece* Selected = nullptr;
	int NumSelected = 0;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsFocused())
		{
			Center = Piece->mModelWorld.GetTranslation();
			return true;
		}

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			Selected = Piece.get();
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

		if (GetVisiblePiecesBoundingBox(Min, Max))
			Center = (Min + Max) / 2.0f;
		else
			Center = lcVector3(0.0f, 0.0f, 0.0f);
	}

	return Center;
}

bool lcModel::GetFocusPosition(lcVector3& Position) const
{
	const lcObject* FocusObject = GetFocusObject();

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

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(Min, Max);
			Selected = true;

			if (!SelectedPiece)
				SelectedPiece = Piece.get();
			else
				SinglePieceSelected = false;
		}
	}

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
	{
		if (Camera->IsSelected())
		{
			Camera->CompareBoundingBox(Min, Max);
			Selected = true;
			SinglePieceSelected = false;
		}
	}

	for (const std::unique_ptr<lcLight>& Light : mLights)
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

lcBoundingBox lcModel::GetAllPiecesBoundingBox() const
{
	lcBoundingBox Box;

	if (!mPieces.empty())
	{
		Box.Min = lcVector3(FLT_MAX, FLT_MAX, FLT_MAX);
		Box.Max = lcVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			Piece->CompareBoundingBox(Box.Min, Box.Max);
	}
	else
		Box.Min = Box.Max = lcVector3(0.0f, 0.0f, 0.0f);

	return Box;
}

bool lcModel::GetVisiblePiecesBoundingBox(lcVector3& Min, lcVector3& Max) const
{
	bool Valid = false;
	Min = lcVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	Max = lcVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() || Piece->IsVisible(mCurrentStep))
		{
			Piece->CompareBoundingBox(Min, Max);
			Valid = true;
		}
	}

	return Valid;
}

std::vector<lcVector3> lcModel::GetPiecesBoundingBoxPoints() const
{
	std::vector<lcVector3> Points;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (Piece->IsSelected() || Piece->IsVisible(mCurrentStep))
			Piece->SubModelAddBoundingBoxPoints(lcMatrix44Identity(), Points);

	return Points;
}

void lcModel::GetPartsList(int DefaultColorIndex, bool ScanSubModels, bool AddSubModels, lcPartsList& PartsList) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (!Piece->IsVisibleInSubModel())
			continue;

		int ColorIndex = Piece->GetColorIndex();

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		Piece->mPieceInfo->GetPartsList(ColorIndex, ScanSubModels, AddSubModels, PartsList);
	}
}

void lcModel::GetPartsListForStep(lcStep Step, int DefaultColorIndex, lcPartsList& PartsList, bool Cumulative) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Cumulative ? Piece->GetStepShow() > Step : Piece->GetStepShow() != Step || Piece->IsHidden())
			continue;

		int ColorIndex = Piece->GetColorIndex();

		if (ColorIndex == gDefaultColor)
			ColorIndex = DefaultColorIndex;

		Piece->mPieceInfo->GetPartsList(ColorIndex, false, true, PartsList);
	}
}

void lcModel::GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Piece->GetModelParts(WorldMatrix, DefaultColorIndex, ModelParts);
}

void lcModel::GetSelectionInformation(int* Flags, std::vector<lcObject*>& Selection, lcObject** Focus) const
{
	*Flags = 0;
	*Focus = nullptr;

	if (mPieces.empty())
		*Flags |= LC_SEL_NO_PIECES;
	else
	{
		lcGroup* Group = nullptr;
		bool First = true;

		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		{
			if (Piece->IsSelected())
			{
				Selection.emplace_back(Piece.get());

				if (Piece->IsFocused())
					*Focus = Piece.get();

				if (Piece->mPieceInfo->IsModel())
					*Flags |= LC_SEL_MODEL_SELECTED;

				if (Piece->IsHidden())
					*Flags |= LC_SEL_HIDDEN | LC_SEL_HIDDEN_SELECTED;
				else
					*Flags |= LC_SEL_VISIBLE_SELECTED;

				*Flags |= LC_SEL_PIECE | LC_SEL_SELECTED;

				if (Piece->AreTrainTrackConnectionsVisible())
					*Flags |= LC_SEL_TRAIN_TRACK_VISIBLE;

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

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
	{
		if (Camera->IsSelected())
		{
			Selection.emplace_back(Camera.get());
			*Flags |= LC_SEL_SELECTED | LC_SEL_CAMERA;

			if (Camera->IsFocused())
				*Focus = Camera.get();
		}
	}

	for (const std::unique_ptr<lcLight>& Light : mLights)
	{
		if (Light->IsSelected())
		{
			Selection.emplace_back(Light.get());
			*Flags |= LC_SEL_SELECTED | LC_SEL_LIGHT;

			if (Light->IsFocused())
				*Focus = Light.get();
		}
	}
}

void lcModel::ClearSelectionAction()
{
	BeginHistorySequence();
	ClearSelection();
	EndHistorySequence(tr("Selection"));
}

void lcModel::SetSelectionAndFocusAction(const std::vector<lcObject*>& Objects, lcObject* Focus, quint32 Section, lcSelectionMode SelectionMode)
{
	BeginHistorySequence();
	SetSelectionAndFocus(Objects, Focus, Section, SelectionMode);
	EndHistorySequence(tr("Selection"));
}

void lcModel::FocusOrDeselectObjectAction(lcObject* Object, uint32_t Section, lcSelectionMode SelectionMode)
{
	BeginHistorySequence();

	if (Object)
	{
		if (!Object->IsFocused(Section))
			SetFocus(Object, Section, SelectionMode);
		else
			SetFocus(nullptr, ~0U, SelectionMode);
	}
	else
	{
		SetFocus(nullptr, 0, SelectionMode);
	}

	EndHistorySequence(tr("Selection"));
}

void lcModel::SelectAllPiecesAction()
{
	BeginHistorySequence();
	SelectAllPieces();
	EndHistorySequence(tr("Selection"));
}

void lcModel::InvertPieceSelectionAction()
{
	BeginHistorySequence();
	InvertPieceSelection();
	EndHistorySequence(tr("Selection"));
}

void lcModel::AddToSelectionAction(const std::vector<lcObject*>& Objects)
{
	BeginHistorySequence();
	AddToSelection(Objects);
	EndHistorySequence(tr("Selection"));
}

void lcModel::RemoveFromSelectionAction(const std::vector<lcObject*>& Objects)
{
	BeginHistorySequence();
	RemoveFromSelection(Objects);
	EndHistorySequence(tr("Selection"));
}

std::vector<lcObject*> lcModel::GetSelectionModePieces(lcSelectionMode SelectionMode, const lcPiece* SelectedPiece) const
{
	const PieceInfo* Info = SelectedPiece->mPieceInfo;
	const int ColorIndex = SelectedPiece->GetColorIndex();
	std::vector<lcObject*> Pieces;

	switch (SelectionMode)
	{
	case lcSelectionMode::Single:
		break;

	case lcSelectionMode::Piece:
		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->mPieceInfo == Info && Piece.get() != SelectedPiece)
				Pieces.emplace_back(Piece.get());
		break;

	case lcSelectionMode::Color:
		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->GetColorIndex() == ColorIndex && Piece.get() != SelectedPiece)
				Pieces.emplace_back(Piece.get());
		break;

	case lcSelectionMode::PieceColor:
		for (const std::unique_ptr<lcPiece>& Piece : mPieces)
			if (Piece->IsVisible(mCurrentStep) && Piece->mPieceInfo == Info && Piece->GetColorIndex() == ColorIndex && Piece.get() != SelectedPiece)
				Pieces.emplace_back(Piece.get());
		break;
	}

	return Pieces;
}

void lcModel::DeselectAllObjects()
{
	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		Piece->SetSelected(false);

	for (const std::unique_ptr<lcCamera>& Camera : mCameras)
		Camera->SetSelected(false);

	for (const std::unique_ptr<lcLight>& Light : mLights)
		Light->SetSelected(false);
}

void lcModel::SetFocusedObject(lcObject* FocusObject, uint32_t FocusSection, lcSelectionMode SelectionMode)
{
	lcObject* PreviousFocus = GetFocusObject();

	if (PreviousFocus)
		PreviousFocus->SetFocused(PreviousFocus->GetFocusSection(), false);

	if (FocusObject)
	{
		lcPiece* Piece = dynamic_cast<lcPiece*>(FocusObject);

		if (Piece)
		{
			if (!Piece->IsSelected())
				SetObjectsSelected({ Piece }, true);

			std::vector<lcObject*> Pieces = GetSelectionModePieces(SelectionMode, Piece);

			SetObjectsSelected(Pieces, true);
		}

		FocusObject->SetFocused(FocusSection, true);
	}
}

void lcModel::SetObjectsSelected(const std::vector<lcObject*>& Objects, bool Selected)
{
	for (lcObject* Object : Objects)
	{
		if (Object->IsSelected() == Selected)
			continue;

		Object->SetSelected(Selected);

		if (Object->IsPiece())
		{
			lcPiece* Piece = dynamic_cast<lcPiece*>(Object);

			SelectGroup(Piece->GetTopGroup(), Selected);
		}
	}
}

void lcModel::SelectGroup(lcGroup* TopGroup, bool Select)
{
	if (!TopGroup)
		return;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
		if (!Piece->IsSelected() && (Piece->GetTopGroup() == TopGroup))
			Piece->SetSelected(Select);
}

void lcModel::HideSelectedPieces()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() && !Piece->IsHidden())
		{
			Piece->SetHidden(true);

			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();

	ClearSelection();

	EndHistorySequence(tr("Hide Pieces"));

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::HideUnselectedPieces()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (!Piece->IsSelected() && !Piece->IsHidden())
		{
			Piece->SetHidden(true);

			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Hide Pieces"));

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::UnhideSelectedPieces()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsSelected() && Piece->IsHidden())
		{
			Piece->SetHidden(false);

			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Unhide Pieces"));

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::UnhideAllPieces()
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	bool Modified = false;

	for (const std::unique_ptr<lcPiece>& Piece : mPieces)
	{
		if (Piece->IsHidden())
		{
			Piece->SetHidden(false);

			Modified = true;
		}
	}

	if (!Modified)
	{
		DiscardHistorySequence();

		return;
	}

	EndEditHistory();
	EndHistorySequence(tr("Unhide Pieces"));

	gMainWindow->UpdateTimeline(false, true);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::FindReplacePiece(bool SearchForward, bool FindAll, bool Replace)
{
	if (mPieces.empty())
		return;

	const lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	const bool ReplacePieceInfo = Replace && Params.ReplacePieceInfo;
	const bool ReplaceColor = Replace && lcGetColorCode(Params.ReplaceColorIndex) != LC_COLOR_NOCOLOR;

	// Check if we are supposed to actually replace something
	const bool Replacing = (ReplaceColor || ReplacePieceInfo);

	auto PieceMatches = [&Params](const lcPiece* Piece)
	{
		if (Params.FindInfo && Params.FindInfo != Piece->mPieceInfo)
			return false;

		if (!Params.FindString.isEmpty() && !lcstrcasestr(Piece->mPieceInfo->m_strDescription, Params.FindString.toLatin1()))
			return false;

		return (lcGetColorCode(Params.FindColorIndex) == LC_COLOR_NOCOLOR) || (Piece->GetColorIndex() == Params.FindColorIndex);
	};

	auto ReplacePiece = [&Params, ReplacePieceInfo, ReplaceColor](lcPiece* Piece)
	{
		if (ReplaceColor)
			Piece->SetColorIndex(Params.ReplaceColorIndex);

		if (ReplacePieceInfo)
			Piece->SetPieceInfo(Params.ReplacePieceInfo, QString(), true, true);
	};

	size_t StartIndex = mPieces.size() - 1;
	int ReplacedCount = 0;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	if (!FindAll)
	{
		// We have to find the currently focused piece, in order to find next/prev match and (optionally) to replace it
		lcPiece* const FocusedPiece = dynamic_cast<lcPiece*>(GetFocusObject());

		if (FocusedPiece)
		{
			for (size_t PieceIndex = 0; PieceIndex < mPieces.size(); PieceIndex++)
			{
				if (FocusedPiece == mPieces[PieceIndex].get())
				{
					StartIndex = PieceIndex;
					break;
				}
			}

			if (Replacing && PieceMatches(FocusedPiece))
			{
				ReplacePiece(FocusedPiece);
				ReplacedCount++;
			}
		}
	}

	size_t CurrentIndex = StartIndex;
	lcPiece* Focus = nullptr;
	std::vector<lcObject*> Selection;

	for (;;)
	{
		if (SearchForward)
		{
			CurrentIndex++;

			if (CurrentIndex >= mPieces.size())
				CurrentIndex = 0;
		}
		else
		{
			if (CurrentIndex == 0)
				CurrentIndex = mPieces.size();

			CurrentIndex--;
		}

		lcPiece* Current = mPieces[CurrentIndex].get();

		if (Current->IsVisible(mCurrentStep) && PieceMatches(Current))
		{
			if (FindAll)
			{
				Selection.emplace_back(Current);
				if (Replacing)
				{
					ReplacePiece(Current);
					ReplacedCount++;
				}
			}
			else
			{
				Focus = Current;
				break;
			}
		}

		if (CurrentIndex == StartIndex)
			break;
	}

	EndEditHistory();

	if (FindAll)
		SetSelectionAndFocus(Selection, nullptr, 0, lcSelectionMode::Single);
	else
		SetSelectionAndFocus(std::vector<lcObject*>(), Focus, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	if (ReplacedCount)
	{
		EndHistorySequence(tr("Replace Piece(s)", "", ReplacedCount));

		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateTimeline(false, true);
	}
	else
		EndHistorySequence(tr("Selection"));
}

void lcModel::UndoAction()
{
	if (mUndoHistory.empty())
		return;

	std::unique_ptr<lcModelHistoryEntry> Undo = std::move(mUndoHistory.front());

	RunHistorySequence(Undo->HistorySequence, false);

	mUndoHistory.erase(mUndoHistory.begin());
	mRedoHistory.insert(mRedoHistory.begin(), std::move(Undo));

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(!mUndoHistory.empty() ? mUndoHistory.front()->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory.front()->Description : nullptr);
}

void lcModel::RedoAction()
{
	if (mRedoHistory.empty())
		return;

	std::unique_ptr<lcModelHistoryEntry> Redo = std::move(mRedoHistory.front());

	RunHistorySequence(Redo->HistorySequence, true);

	mRedoHistory.erase(mRedoHistory.begin());
	mUndoHistory.insert(mUndoHistory.begin(), std::move(Redo));

	gMainWindow->UpdateModified(IsModified());
	gMainWindow->UpdateUndoRedo(!mUndoHistory.empty() ? mUndoHistory.front()->Description : nullptr, !mRedoHistory.empty() ? mRedoHistory.front()->Description : nullptr);
}

void lcModel::BeginMouseTool(lcTool Tool, lcView* View)
{
	switch (Tool)
	{
		case lcTool::Insert:
		case lcTool::PointLight:
		case lcTool::SpotLight:
		case lcTool::DirectionalLight:
		case lcTool::AreaLight:
		case lcTool::Camera:
		case lcTool::Select:
			break;

		case lcTool::Move:
		case lcTool::Rotate:
			BeginHistorySequence();
			BeginEditHistory(lcModelHistoryEditMerge::None);
			break;

		case lcTool::Eraser:
		case lcTool::Paint:
		case lcTool::ColorPicker:
			break;

		case lcTool::Pan:
		case lcTool::Zoom:
		case lcTool::RotateView:
		case lcTool::Roll:
			if (!View->GetCamera()->IsSimple())
			{
				BeginHistorySequence();
				BeginEditHistory(lcModelHistoryEditMerge::None);
			}
			break;

		case lcTool::ZoomRegion:
			break;

		case lcTool::Count:
			break;
	}

	mMouseToolDistance = lcVector3(0.0f, 0.0f, 0.0f);
	mMouseToolFirstMove = true;
}

void lcModel::EndMouseTool(lcTool Tool, lcView* View, bool Accept)
{
	if (!Accept)
	{
		RevertHistorySequence();
		return;
	}

	const lcCamera* Camera = View->GetCamera();

	switch (Tool)
	{
	case lcTool::Insert:
	case lcTool::PointLight:
	case lcTool::SpotLight:
	case lcTool::DirectionalLight:
	case lcTool::AreaLight:
	case lcTool::Camera:
	case lcTool::Select:
		break;

	case lcTool::Move:
		EndEditHistory();
		EndHistorySequence(tr("Move"));
		break;

	case lcTool::Rotate:
		EndEditHistory();
		EndHistorySequence(tr("Rotate"));
		break;

	case lcTool::Eraser:
	case lcTool::Paint:
	case lcTool::ColorPicker:
		break;

	case lcTool::Zoom:
		if (!Camera->IsSimple())
		{
			EndEditHistory();
			EndHistorySequence(tr("Zoom"));
		}
		break;

	case lcTool::Pan:
		if (!Camera->IsSimple())
		{
			EndEditHistory();
			EndHistorySequence(tr("Pan"));
		}
		break;

	case lcTool::RotateView:
		if (!Camera->IsSimple())
		{
			EndEditHistory();
			EndHistorySequence(tr("Orbit"));
		}
		break;

	case lcTool::Roll:
		if (!Camera->IsSimple())
		{
			EndEditHistory();
			EndHistorySequence(tr("Roll"));
		}
		break;

	case lcTool::ZoomRegion:
		break;

	case lcTool::Count:
		break;
	}
}

void lcModel::InsertPieceToolClicked(const std::vector<lcInsertPieceInfo>& PieceInfoTransforms)
{
	if (PieceInfoTransforms.empty())
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	lcPiece* Piece = nullptr;

	for (const lcInsertPieceInfo& PieceInfoTransform : PieceInfoTransforms)
	{
		Piece = new lcPiece(PieceInfoTransform.Info);

		Piece->Initialize(PieceInfoTransform.Transform, mCurrentStep);
		Piece->SetColorIndex(PieceInfoTransform.ColorIndex);
		Piece->UpdatePosition(mCurrentStep);

		AddPiece(Piece);
	}

	EndEditHistory();

	SetSelectionAndFocus(std::vector<lcObject*>(), Piece, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(tr("Add Piece"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateInUseCategory();

	UpdateTrainTrackConnections(Piece, false);
}

void lcModel::InsertCameraToolClicked(const lcVector3& Position)
{
	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	lcCamera* Camera = new lcCamera(false, Position, GetSelectionOrModelCenter());

	Camera->CreateName(mCameras);
	mCameras.emplace_back(Camera);

	EndEditHistory();

	SetSelectionAndFocus(std::vector<lcObject*>(), Camera, LC_CAMERA_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(tr("Add Camera"));
}

void lcModel::InsertLightToolClicked(const lcVector3& Position, lcLightType LightType)
{
	QString ActionName;

	switch (LightType)
	{
	case lcLightType::Point:
		ActionName = tr("Add Point Light");
		break;

	case lcLightType::Spot:
		ActionName = tr("Add Spot Light");
		break;

	case lcLightType::Directional:
		ActionName = tr("Add Directional Light");
		break;

	case lcLightType::Area:
		ActionName = tr("Add Area Light");
		break;

	case lcLightType::Count:
		return;
	}

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	lcLight* Light = new lcLight(Position, LightType);

	Light->CreateName(mLights);
	mLights.emplace_back(Light);

	EndEditHistory();

	SetSelectionAndFocus(std::vector<lcObject*>(), Light, LC_LIGHT_SECTION_POSITION, lcSelectionMode::Single);

	EndHistorySequence(ActionName);
}

void lcModel::UpdateMoveTool(const lcVector3& Distance, bool AllowRelative, bool AlternateButtonDrag)
{
	const lcVector3 PieceDistance = SnapPosition(Distance) - SnapPosition(mMouseToolDistance);
	const lcVector3 ObjectDistance = Distance - mMouseToolDistance;

	MoveSelectedObjects(PieceDistance, ObjectDistance, AllowRelative, AlternateButtonDrag, false, mMouseToolFirstMove, lcModelHistoryEditMerge::None);

	mMouseToolDistance = Distance;
	mMouseToolFirstMove = false;

	gMainWindow->UpdateSelectedObjects(false);
	UpdateAllViews();
}

void lcModel::UpdateFreeMoveTool(lcPiece* MousePiece, const lcMatrix44& StartTransform, const lcMatrix44& NewTransform, bool IsConnection, bool AlternateButtonDrag)
{
	lcMatrix33 NewRotation = IsConnection ? lcMatrix33(NewTransform) : lcMatrix33(StartTransform);
	lcMatrix33 CurrentRotation = lcMatrix33(MousePiece->mModelWorld);

	if (!lcMatrix33Similar(CurrentRotation, NewRotation))
	{
		const lcVector3 Distance = NewTransform.GetTranslation() - MousePiece->mModelWorld.GetTranslation();

		lcMatrix33 RotationMatrix = lcMul(lcMatrix33AffineInverse(CurrentRotation), NewRotation);

		const lcVector3& Center = NewTransform.GetTranslation();
		lcMatrix33 RelativeRotation = MousePiece->GetRelativeRotation();

		lcMatrix33 WorldToFocusMatrix = lcMatrix33AffineInverse(RelativeRotation);
		RotationMatrix = lcMul(RotationMatrix, RelativeRotation);

		int Flags;
		std::vector<lcObject*> Selection;
		lcObject* Focus;

		GetSelectionInformation(&Flags, Selection, &Focus);

		for (lcObject* Object : Selection)
		{
			if (Object->IsPiece())
			{
				lcPiece* Piece = (lcPiece*)Object;

				Piece->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), Distance);
				Piece->UpdatePosition(mCurrentStep);

				Piece->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
				Piece->UpdatePosition(mCurrentStep);
			}
			else if (Object->IsCamera())
			{
				lcCamera* Camera = (lcCamera*)Object;

				Camera->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), Distance);
				Camera->UpdatePosition(mCurrentStep);

				Camera->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
				Camera->UpdatePosition(mCurrentStep);
			}
			else if (Object->IsLight())
			{
				lcLight* Light = (lcLight*)Object;

				Light->MoveSelected(mCurrentStep, gMainWindow->GetAddKeys(), Distance, mMouseToolFirstMove);
				Light->UpdatePosition(mCurrentStep);

				Light->Rotate(mCurrentStep, gMainWindow->GetAddKeys(), RotationMatrix, Center, WorldToFocusMatrix);
				Light->UpdatePosition(mCurrentStep);
			}
		}

		mMouseToolDistance = NewTransform.GetTranslation() - StartTransform.GetTranslation();
		mMouseToolFirstMove = false;

		gMainWindow->UpdateSelectedObjects(false);
		UpdateAllViews();
	}
	else
	{
		lcVector3 Distance = NewTransform.GetTranslation() - StartTransform.GetTranslation();

		UpdateMoveTool(Distance, false, AlternateButtonDrag);
	}
}

void lcModel::UpdateRotateTool(const lcVector3& Angles, bool AlternateButtonDrag)
{
	const lcVector3 Delta = SnapRotation(Angles) - SnapRotation(mMouseToolDistance);
	RotateSelectedObjects(Delta, true, AlternateButtonDrag, false, lcModelHistoryEditMerge::None);

	mMouseToolDistance = Angles;
	mMouseToolFirstMove = false;
}

void lcModel::UpdateScaleTool(const float Scale)
{
	ScaleSelectedPieces(Scale);

	gMainWindow->UpdateSelectedObjects(false);
	UpdateAllViews();
}

void lcModel::EraserToolClicked(lcObject* Object)
{
	if (!Object)
		return;

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	switch (Object->GetType())
	{
	case lcObjectType::Piece:
		for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); ++PieceIt)
			if (PieceIt->get() == Object)
			{
				mPieces.erase(PieceIt);
				RemoveEmptyGroups();
				break;
			}
		break;

	case lcObjectType::Camera:
		{
			std::vector<lcView*> Views = lcView::GetModelViews(this);

			for (lcView* View : Views)
			{
				lcCamera* Camera = View->GetCamera();

				if (Camera == Object)
					View->SetCamera(Camera, true);
			}

			for (std::vector<std::unique_ptr<lcCamera>>::iterator CameraIt = mCameras.begin(); CameraIt != mCameras.end(); CameraIt++)
			{
				if (CameraIt->get() == Object)
				{
					mCameras.erase(CameraIt);
					break;
				}
			}
		}
		break;

	case lcObjectType::Light:
		for (std::vector<std::unique_ptr<lcLight>>::iterator LightIt = mLights.begin(); LightIt != mLights.end(); LightIt++)
		{
			if (LightIt->get() == Object)
			{
				mLights.erase(LightIt);
				break;
			}
		}
		break;
	}

	EndEditHistory();
	EndHistorySequence(tr("Delete"));

	gMainWindow->UpdateTimeline(false, false);
	gMainWindow->UpdateSelectedObjects(true);
}

void lcModel::PaintToolClicked(lcObject* Object)
{
	if (!Object || !Object->IsPiece())
		return;

	lcPiece* Piece = (lcPiece*)Object;

	if (Piece->GetColorIndex() != gMainWindow->mColorIndex)
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::None);

		Piece->SetColorIndex(gMainWindow->mColorIndex);

		EndEditHistory();
		EndHistorySequence(tr("Paint"));

		gMainWindow->UpdateSelectedObjects(false);
		gMainWindow->UpdateTimeline(false, true);
	}
}

void lcModel::ColorPickerToolClicked(const lcObject* Object)
{
	if (!Object || !Object->IsPiece())
		return;

	const lcPiece* Piece = (lcPiece*)Object;

	gMainWindow->SetColorIndex(Piece->GetColorIndex());
}

void lcModel::UpdateZoomTool(lcCamera* Camera, float Mouse)
{
	Camera->Zoom(Mouse - mMouseToolDistance.x, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;

	UpdateAllViews();
}

void lcModel::UpdatePanTool(lcCamera* Camera, const lcVector3& Distance)
{
	Camera->Pan(Distance, mCurrentStep, gMainWindow->GetAddKeys());

	UpdateAllViews();
}

void lcModel::UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY)
{
	lcVector3 Center;
	GetSelectionCenter(Center);

	Camera->Orbit(MouseX - mMouseToolDistance.x, MouseY - mMouseToolDistance.y, Center, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = MouseX;
	mMouseToolDistance.y = MouseY;

	UpdateAllViews();
}

void lcModel::UpdateRollTool(lcCamera* Camera, float Mouse)
{
	Camera->Roll(Mouse - mMouseToolDistance.x, mCurrentStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;

	UpdateAllViews();
}

void lcModel::ZoomRegionToolClicked(lcView* View, float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners)
{
	lcCamera* Camera = View->GetCamera();

	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::None);
	}

	Camera->ZoomRegion(AspectRatio, Position, TargetPosition, Corners, mCurrentStep, gMainWindow->GetAddKeys());

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Zoom"));
	}

	gMainWindow->UpdateSelectedObjects(false);
	UpdateAllViews();
}

void lcModel::LookAt(lcCamera* Camera)
{
	lcVector3 Center;

	if (!GetSelectionCenter(Center))
	{
		lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		if (GetVisiblePiecesBoundingBox(Min, Max))
			Center = (Min + Max) / 2.0f;
		else
			Center = lcVector3(0.0f, 0.0f, 0.0f);
	}

	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::None);
	}

	Camera->Center(Center, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateSelectedObjects(false);
	UpdateAllViews();

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Look At"));
	}
}

void lcModel::MoveCamera(lcCamera* Camera, const lcVector3& Direction)
{
	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::KeyboardMoveCamera);
	}

	Camera->MoveRelative(Direction, mCurrentStep, gMainWindow->GetAddKeys());

	gMainWindow->UpdateSelectedObjects(false);
	UpdateAllViews();

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Move"));
	}
}

void lcModel::ZoomExtents(lcCamera* Camera, float Aspect, const lcMatrix44& WorldMatrix)
{
	std::vector<lcVector3> Points = GetPiecesBoundingBoxPoints();

	if (Points.empty())
		return;

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (lcVector3& Point : Points)
	{
		Point = lcMul31(Point, WorldMatrix);

		Min = lcMin(Point, Min);
		Max = lcMax(Point, Max);
	}

	const lcVector3 Center = (Min + Max) / 2.0f;

	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::None);
	}

	Camera->ZoomExtents(Aspect, Center, Points, mCurrentStep, gMainWindow ? gMainWindow->GetAddKeys() : false);

	if (!mIsPreview && gMainWindow)
		gMainWindow->UpdateSelectedObjects(false);

	UpdateAllViews();

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Zoom Extents"));
	}
}

void lcModel::Zoom(lcCamera* Camera, float Amount)
{
	if (!Camera->IsSimple())
	{
		BeginHistorySequence();
		BeginEditHistory(lcModelHistoryEditMerge::KeyboardZoom);
	}

	Camera->Zoom(Amount, mCurrentStep, gMainWindow->GetAddKeys());

	if (!mIsPreview)
		gMainWindow->UpdateSelectedObjects(false);

	UpdateAllViews();

	if (!Camera->IsSimple())
	{
		EndEditHistory();
		EndHistorySequence(tr("Zoom"));
	}
}

void lcModel::ShowPropertiesDialog()
{
	lcPropertiesDialogOptions Options;

	Options.Properties = mProperties;
	Options.BoundingBox = GetAllPiecesBoundingBox();

	GetPartsList(gDefaultColor, true, false, Options.PartsList);

	lcPropertiesDialog Dialog(gMainWindow, &Options);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	if (mProperties == Options.Properties)
		return;

	BeginHistorySequence();
	SetModelProperties(Options.Properties);
	EndHistorySequence(tr("Change Model Properties"));
}

void lcModel::ShowArrayDialog()
{
	lcVector3 Center;

	if (!GetPieceFocusOrSelectionCenter(Center))
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("No pieces selected."));
		return;
	}

	lcArrayDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	if (Dialog.mCounts[0] * Dialog.mCounts[1] * Dialog.mCounts[2] < 2)
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Array only has 1 element or less, no pieces added."));
		return;
	}

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	std::vector<lcObject*> NewPieces;

	for (int Step1 = 0; Step1 < Dialog.mCounts[0]; Step1++)
	{
		for (int Step2 = 0; Step2 < Dialog.mCounts[1]; Step2++)
		{
			for (int Step3 = (Step1 == 0 && Step2 == 0) ? 1 : 0; Step3 < Dialog.mCounts[2]; Step3++)
			{
				lcMatrix44 ModelWorld;
				lcVector3 Position;

				lcVector3 RotationAngles = Dialog.mRotations[0] * Step1 + Dialog.mRotations[1] * Step2 + Dialog.mRotations[2] * Step3;
				const lcVector3 Offset = Dialog.mOffsets[0] * Step1 + Dialog.mOffsets[1] * Step2 + Dialog.mOffsets[2] * Step3;

				for (const std::unique_ptr<lcPiece>& Piece : mPieces)
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
					NewPiece->SetPieceInfo(Piece->mPieceInfo, Piece->GetID(), true, true);
					NewPiece->Initialize(ModelWorld, mCurrentStep);
					NewPiece->SetColorIndex(Piece->GetColorIndex());

					NewPieces.emplace_back(NewPiece);
				}
			}
		}
	}

	for (size_t PieceIdx = 0; PieceIdx < NewPieces.size(); PieceIdx++)
	{
		lcPiece* Piece = (lcPiece*)NewPieces[PieceIdx];
		Piece->UpdatePosition(mCurrentStep);
		AddPiece(Piece);
	}

	EndEditHistory();

	AddToSelection(NewPieces);

	EndHistorySequence(tr("Piece Array"));

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::ShowMinifigDialog()
{
	lcMinifigDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	gMainWindow->GetActiveView()->MakeCurrent();

	BeginHistorySequence();
	BeginEditHistory(lcModelHistoryEditMerge::None);

	lcGroup* Group = AddGroup(tr("Minifig #"), nullptr);
	std::vector<lcObject*> Pieces;
	lcMinifig& Minifig = Dialog.mMinifigWizard->mMinifig;

	Pieces.reserve(LC_MFW_NUMITEMS);

	for (int PartIndex = 0; PartIndex < LC_MFW_NUMITEMS; PartIndex++)
	{
		if (!Minifig.Parts[PartIndex])
			continue;

		lcPiece* Piece = new lcPiece(Minifig.Parts[PartIndex]);

		Piece->Initialize(Minifig.Matrices[PartIndex], mCurrentStep);
		Piece->SetColorIndex(Minifig.ColorIndices[PartIndex]);
		Piece->SetGroup(Group);
		AddPiece(Piece);
		Piece->UpdatePosition(mCurrentStep);

		Pieces.emplace_back(Piece);
	}

	EndEditHistory();

	SetSelectionAndFocus(Pieces, nullptr, 0, lcSelectionMode::Single);

	EndHistorySequence(tr("Add Minifig"));

	gMainWindow->UpdateTimeline(false, false);
}

void lcModel::SetMinifig(const lcMinifig& Minifig)
{
	DeleteModel();

	std::vector<lcObject*> Pieces;
	Pieces.reserve(LC_MFW_NUMITEMS);

	for (int PartIdx = 0; PartIdx < LC_MFW_NUMITEMS; PartIdx++)
	{
		if (!Minifig.Parts[PartIdx])
			continue;

		lcPiece* Piece = new lcPiece(Minifig.Parts[PartIdx]);

		Piece->Initialize(Minifig.Matrices[PartIdx], 1);
		Piece->SetColorIndex(Minifig.ColorIndices[PartIdx]);
		AddPiece(Piece);
		Piece->UpdatePosition(1);

		Pieces.emplace_back(Piece);
	}
}

void lcModel::SetPreviewPieceInfo(PieceInfo* Info, int ColorIndex)
{
	DeleteModel();

	lcPiece* Piece = new lcPiece(Info);

	Piece->Initialize(lcMatrix44Identity(), 1);
	Piece->SetColorIndex(ColorIndex);
	AddPiece(Piece);
	Piece->UpdatePosition(1);

	mCurrentStep = LC_STEP_MAX;
	CalculateStep(LC_STEP_MAX);
}

void lcModel::UpdateInterface()
{
	if (!gMainWindow)
		return;

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
	gMainWindow->UpdateModels();
	gMainWindow->UpdateShadingMode();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateSelectionMode();
}
