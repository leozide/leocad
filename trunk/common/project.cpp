#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <locale.h>
#include "opengl.h"
#include "pieceinf.h"
#include "lc_texture.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "project.h"
#include "image.h"
#include "system.h"
#include "minifig.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "texfont.h"
#include "debug.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_context.h"
#include "preview.h"

Project::Project()
{
}

Project::~Project()
{
}

void Project::UpdateInterface()
{
	// Update all user interface elements.
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
	gMainWindow->UpdatePaste(g_App->mClipboard != NULL);
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle(GetTitle(), IsModified());
	gMainWindow->SetTool(gMainWindow->GetTool());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->SetTransformType(gMainWindow->GetTransformType());
	gMainWindow->UpdateLockSnap();
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdatePerspective();
	gMainWindow->UpdateCurrentStep();

	UpdateSelection();
}

void Project::LoadDefaults()
{
	mProperties.LoadDefaults();

	gMainWindow->SetColorIndex(lcGetColorIndex(4));
	gMainWindow->SetTool(LC_TOOL_SELECT);
	gMainWindow->SetAddKeys(false);
	gMainWindow->UpdateUndoRedo(NULL, NULL);
	gMainWindow->UpdateLockSnap();
	gMainWindow->UpdateSnap();
	mCurrentStep = 1;
	gMainWindow->UpdateCurrentStep();

	const lcArray<View*>& Views = gMainWindow->GetViews();
	for (int i = 0; i < Views.GetSize(); i++)
		if (!Views[i]->mCamera->IsSimple())
			Views[i]->SetDefaultCamera();

	gMainWindow->UpdateCameraMenu();

	UpdateSelection();
	gMainWindow->UpdateFocusObject(NULL);
}

bool Project::FileLoad(lcFile* file, bool bUndo, bool bMerge)
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

	// Fix the ugly floating point reading on computers with different decimal points.
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
	if (!bMerge)
	{
		mProperties.mBackgroundSolidColor[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mBackgroundSolidColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mBackgroundSolidColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	}

	if (fv < 0.6f) // old view
	{
		double eye[3], target[3];
		file->ReadDoubles(eye, 3);
		file->ReadDoubles(target, 3);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		file->Seek(28, SEEK_CUR);
		file->ReadS32(&i, 1);
		mCurrentStep = i;
	}

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

			if (bMerge)
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
					if (strcmp(mPieces[PieceIdx]->GetName(), pPiece->GetName()) == 0)
					{
						pPiece->CreateName(mPieces);
						break;
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

	if (!bMerge)
	{
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
	}
	else
	{
		if (fv >= 0.4f)
		{
			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);
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

			if (bMerge)
			{
				// Ensure a unique group name

				int max = -1;
				String baseName;

				for (int Existing = 0; Existing < GroupIdx; Existing++)
					max = lcMax(max, InstanceOfName(mGroups[Existing]->m_strName, Group->m_strName, baseName));

				if (max > -1)
				{
					int baseReserve = sizeof(Group->m_strName) - 5; // space, #, 2-digits, and terminating 0
					for (int num = max; (num > 99); num /= 10) { baseReserve--; }
					sprintf(Group->m_strName, "%s #%.2d", (const char*)(baseName.Left(baseReserve)), max+1);
				}
			}
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

	if (!bMerge)
	{
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
	}

	UpdateBackgroundTexture();
	CalculateStep();

	if (!bUndo)
		ClearSelection(false);

	if (!bMerge)
		gMainWindow->UpdateFocusObject(GetFocusObject());

	if (!bMerge)
	{
		const lcArray<View*>& Views = gMainWindow->GetViews();
		for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		{
			View* view = Views[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}

		if (!bUndo)
			ZoomExtents(0, Views.GetSize());
	}

	gMainWindow->UpdateLockSnap();
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	UpdateSelection();
	gMainWindow->UpdateCurrentStep();
	gMainWindow->UpdateAllViews();

	return true;
}

void Project::FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const
{
	LC_FILEENTRY* CurFile = NULL;
	char Buf[1024];

	while (MPD.ReadLine(Buf, 1024))
	{
		String Line(Buf);

		Line.TrimLeft();

		if (Line[0] != '0')
		{
			// Copy current line.
			if (CurFile != NULL)
				CurFile->File.WriteBuffer(Buf, strlen(Buf));

			continue;
		}

		Line.TrimRight();
		Line = Line.Right(Line.GetLength() - 1);
		Line.TrimLeft();

		// Find where a subfile starts.
		if (Line.CompareNoCase("FILE", 4) == 0)
		{
			Line = Line.Right(Line.GetLength() - 4);
			Line.TrimLeft();

			// Create a new file.
			CurFile = new LC_FILEENTRY();
			strncpy(CurFile->FileName, Line, sizeof(CurFile->FileName));
			FileArray.Add(CurFile);
		}
		else if (Line.CompareNoCase("ENDFILE", 7) == 0)
		{
			// File ends here.
			CurFile = NULL;
		}
		else if (CurFile != NULL)
		{
			// Copy current line.
			CurFile->File.WriteBuffer(Buf, strlen(Buf));
		}
	}
}

void Project::FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray)
{
	char Line[1024];

	// Save file offset.
	lcuint32 Offset = file->GetPosition();
	file->Seek(0, SEEK_SET);

	while (file->ReadLine(Line, 1024))
	{
		bool read = true;
		char* Ptr = Line;
		char* Tokens[15];

		for (int TokenIdx = 0; TokenIdx < 15; TokenIdx++)
		{
			Tokens[TokenIdx] = 0;

			while (*Ptr && *Ptr <= 32)
			{
				*Ptr = 0;
				Ptr++;
			}

			Tokens[TokenIdx] = Ptr;

			while (*Ptr > 32)
				Ptr++;
		}

		if (!Tokens[0])
			continue;

		int LineType = atoi(Tokens[0]);

		if (LineType == 0)
		{
			if (Tokens[1])
			{
				strupr(Tokens[1]);

				if (!strcmp(Tokens[1], "STEP"))
					(*nStep)++;
			}

			continue;
		}

		if (LineType != 1)
			continue;

		bool Error = false;
		for (int TokenIdx = 1; TokenIdx < 15; TokenIdx++)
		{
			if (!Tokens[TokenIdx])
			{
				Error = true;
				break;
			}
		}

		if (Error)
			continue;

		int ColorCode = atoi(Tokens[1]);

		float Matrix[12];
		for (int TokenIdx = 2; TokenIdx < 14; TokenIdx++)
			Matrix[TokenIdx - 2] = atof(Tokens[TokenIdx]);

		lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[6], Matrix[9], 0.0f), lcVector4(Matrix[4], Matrix[7], Matrix[10], 0.0f),
		                            lcVector4(Matrix[5], Matrix[8], Matrix[11], 0.0f), lcVector4(Matrix[0], Matrix[1], Matrix[2], 1.0f));

		IncludeTransform = lcMul(IncludeTransform, CurrentTransform);

		if (ColorCode == 16)
			ColorCode = DefColor;

		char* IncludeName = Tokens[14];
		for (Ptr = IncludeName; *Ptr; Ptr++)
			if (*Ptr == '\r' || *Ptr == '\n')
				*Ptr = 0;

		// See if it's a piece in the library
		if (strlen(IncludeName) < LC_PIECE_NAME_LEN)
		{
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, false);
			if (pInfo != NULL)
			{
				lcPiece* pPiece = new lcPiece(pInfo);
				read = false;

				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
				                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
				AxisAngle[3] *= LC_RTOD;

				pPiece->Initialize(lcVector3(IncludeTransform[3].x, IncludeTransform[3].z, -IncludeTransform[3].y), AxisAngle, *nStep);
				pPiece->SetColorCode(ColorCode);
				pPiece->CreateName(mPieces);
				mPieces.Add(pPiece);
				(*nOk)++;
			}
		}

		// Check for MPD files first.
		if (read)
		{
			for (int i = 0; i < FileArray.GetSize(); i++)
			{
				if (stricmp(FileArray[i]->FileName, IncludeName) == 0)
				{
					FileReadLDraw(&FileArray[i]->File, IncludeTransform, nOk, ColorCode, nStep, FileArray);
					read = false;
					break;
				}
			}
		}

		// Try to read the file from disk.
		if (read)
		{
			lcDiskFile tf;

			if (tf.Open(IncludeName, "rt"))
			{
				FileReadLDraw(&tf, IncludeTransform, nOk, ColorCode, nStep, FileArray);
				read = false;
			}
		}

		if (read)
		{
			// Create a placeholder.
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* Info = lcGetPiecesLibrary()->CreatePlaceholder(name);

			lcPiece* pPiece = new lcPiece(Info);
			read = false;

			float* Matrix = IncludeTransform;
			lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
			                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
			AxisAngle[3] *= LC_RTOD;

			pPiece->Initialize(lcVector3(IncludeTransform[3].x, IncludeTransform[3].z, -IncludeTransform[3].y), AxisAngle, *nStep);
			pPiece->SetColorCode(ColorCode);
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);
			(*nOk)++;
		}
	}

	// Restore file offset.
	file->Seek(Offset, SEEK_SET);
}

bool Project::DoSave(const QString& FileName)
{
	QString SaveFileName;

	if (!FileName.isEmpty())
		SaveFileName = FileName;
	else
	{
		if (!mFileName.isEmpty())
			SaveFileName = mFileName;
		else
			SaveFileName = QFileInfo(QDir(QLatin1String(lcGetProfileString(LC_PROFILE_PROJECTS_PATH))), GetTitle()).absoluteFilePath();

		SaveFileName = QFileDialog::getSaveFileName(gMainWindow->mHandle, tr("Save Project"), SaveFileName, tr("Supported Files (*.ldr *.dat);;All Files (*.*)"));

		if (SaveFileName.isEmpty())
			return false;
	}

	if (QFileInfo(SaveFileName).suffix().toLower() == QLatin1String("lcd"))
	{
		QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Saving files in LCD format is no longer supported, please use the LDR format instead."));
		return false;
	}

	QFile File(SaveFileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(SaveFileName, File.errorString()));
		return false;
	}

	QTextStream Stream(&File);
	SaveLDraw(Stream);

	mSavedHistory = mUndoHistory[0];

	SetFileName(SaveFileName);

	return true;
}

bool Project::SaveIfModified()
{
	if (!IsModified())
		return true;

	switch (QMessageBox::question(gMainWindow->mHandle, tr("Save Project"), tr("Save changes to '%1'?").arg(GetTitle()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
	{
	default:
	case QMessageBox::Cancel:
		return false;

	case QMessageBox::Yes:
		if (!DoSave(mFileName))
			return false;
		break;

	case QMessageBox::No:
		break;
	}

	return true;
}

bool Project::OnNewDocument()
{
	DeleteModel();
	DeleteHistory();
	LoadDefaults();

	CheckPoint("");
	mSavedHistory = mUndoHistory[0];

	SetFileName(QString());

	return true;
}

bool Project::OpenProject(const QString& FileName)
{
	if (!SaveIfModified())
		return false;

	if (!OnOpenDocument(FileName))
		return false;

	SetFileName(FileName);

	return true;
}

bool Project::OnOpenDocument(const QString& FileName)
{
	lcDiskFile file;
	bool bSuccess = false;

	if (!file.Open(FileName.toLatin1().constData(), "rb")) // todo: qstring
	{
//		MessageBox("Failed to open file.");
		return false;
	}

	QString Extension = QFileInfo(FileName).suffix().toLower();

	bool datfile = (Extension == QLatin1String("dat") || Extension == QLatin1String("ldr"));
	bool mpdfile = (Extension == QLatin1String("mpd"));

	DeleteModel();
	DeleteHistory();
	LoadDefaults();

	if (file.GetLength() != 0)
	{
		lcArray<LC_FILEENTRY*> FileArray;

		// Unpack the MPD file.
		if (mpdfile)
		{
			FileReadMPD(file, FileArray);

			if (FileArray.GetSize() == 0)
			{
				file.Seek(0, SEEK_SET);
				mpdfile = false;
				datfile = true;
			}
		}

		if (datfile || mpdfile)
		{
			int ok = 0, step = 1;
			lcMatrix44 mat = lcMatrix44Identity();

			if (mpdfile)
				FileReadLDraw(&FileArray[0]->File, mat, &ok, 16, &step, FileArray);
			else
//				FileReadLDraw(&file, mat, &ok, 16, &step, FileArray);
			{
				QFile File(FileName);

				if (!File.open(QIODevice::ReadOnly))
				{
					QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
					return false;
				}

				QTextStream Stream(&File);
				LoadLDraw(Stream);
			}

			mCurrentStep = step;
			gMainWindow->UpdateCurrentStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();

			ZoomExtents(0, gMainWindow->GetViews().GetSize());
			gMainWindow->UpdateAllViews();

			bSuccess = true;
		}
		else
		{
			// Load a LeoCAD file.
			bSuccess = FileLoad(&file, false, false);
		}

		FileArray.DeleteAll();
	}

	file.Close();

	if (bSuccess == false)
	{
		OnNewDocument();
//		MessageBox("Failed to load.");
		return false;
	}

	CheckPoint("");
	mSavedHistory = mUndoHistory[0];

	return true;
}

void Project::SetFileName(const QString& FileName)
{
	mFileName = FileName;

	if (!FileName.isEmpty())
		gMainWindow->AddRecentFile(FileName);
	gMainWindow->UpdateTitle(GetTitle(), IsModified());
}

QString Project::GetTitle() const
{
	return mFileName.isEmpty() ? tr("New Project.ldr") : QFileInfo(mFileName).fileName();
}

void Project::CheckPoint(const char* Description)
{
	SaveCheckpoint(Description);
}

void Project::Render(View* View, bool ToMemory)
{
	lcContext* Context = View->mContext;

	Context->SetDefaultState();
	Context->SetViewport(0, 0, View->mWidth, View->mHeight);

	DrawBackground(Context);

	RenderScenePieces(View, !ToMemory);

	if (!ToMemory)
	{
		RenderSceneObjects(View);
	}
}

void Project::RenderScenePieces(View* view, bool DrawInterface)
{
	const lcPreferences& Preferences = lcGetPreferences();
	lcContext* Context = view->mContext;

	Context->SetProjectionMatrix(view->GetProjectionMatrix());

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);

		GLfloat mat_translucent[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat mat_opaque[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat medium_shininess[] = { (GLfloat)64.0 };

		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, medium_shininess);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_opaque);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_translucent);

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lcVector4(mProperties.mAmbientColor, 1.0f));

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			mLights[LightIdx]->Setup(LightIdx);

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, mProperties.mFogDensity);
		glFogfv(GL_FOG_COLOR, lcVector4(mProperties.mFogColor, 1.0f));
		glEnable(GL_FOG);
	}

	lcArray<lcRenderMesh> OpaqueMeshes(mPieces.GetSize());
	lcArray<lcRenderMesh> TranslucentMeshes;
	const lcMatrix44& ViewMatrix = view->mCamera->mWorldView;

	Context->SetLineWidth(Preferences.mLineWidth);

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

		Info->AddRenderMeshes(ViewMatrix, &Piece->mModelWorld, Piece->mColorIndex, Focused, Selected, OpaqueMeshes, TranslucentMeshes);
	}

	OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	Context->DrawOpaqueMeshes(ViewMatrix, OpaqueMeshes);

	TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);
	Context->DrawTranslucentMeshes(ViewMatrix, TranslucentMeshes);

	Context->UnbindMesh(); // context remove

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
		glDisable(GL_FOG);

	if (DrawInterface)
	{
		Context->SetLineWidth(2.0f * Preferences.mLineWidth);

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if (!Piece->IsVisible(mCurrentStep) || !Piece->IsSelected())
				continue;

			PieceInfo* PieceInfo = Piece->mPieceInfo;

			lcVector3 Min(PieceInfo->m_fDimensions[3], PieceInfo->m_fDimensions[4], PieceInfo->m_fDimensions[5]);
			lcVector3 Max(PieceInfo->m_fDimensions[0], PieceInfo->m_fDimensions[1], PieceInfo->m_fDimensions[2]);
			lcVector3 Edge((Max - Min) * 0.33f);

			float Verts[48][3] =
			{
				{ Max[0], Max[1], Max[2] }, { Max[0] - Edge[0], Max[1], Max[2] },
				{ Max[0], Max[1], Max[2] }, { Max[0], Max[1] - Edge[1], Max[2] },
				{ Max[0], Max[1], Max[2] }, { Max[0], Max[1], Max[2] - Edge[2] },

				{ Min[0], Max[1], Max[2] }, { Min[0] + Edge[0], Max[1], Max[2] },
				{ Min[0], Max[1], Max[2] }, { Min[0], Max[1] - Edge[1], Max[2] },
				{ Min[0], Max[1], Max[2] }, { Min[0], Max[1], Max[2] - Edge[2] },

				{ Max[0], Min[1], Max[2] }, { Max[0] - Edge[0], Min[1], Max[2] },
				{ Max[0], Min[1], Max[2] }, { Max[0], Min[1] + Edge[1], Max[2] },
				{ Max[0], Min[1], Max[2] }, { Max[0], Min[1], Max[2] - Edge[2] },

				{ Min[0], Min[1], Max[2] }, { Min[0] + Edge[0], Min[1], Max[2] },
				{ Min[0], Min[1], Max[2] }, { Min[0], Min[1] + Edge[1], Max[2] },
				{ Min[0], Min[1], Max[2] }, { Min[0], Min[1], Max[2] - Edge[2] },

				{ Max[0], Max[1], Min[2] }, { Max[0] - Edge[0], Max[1], Min[2] },
				{ Max[0], Max[1], Min[2] }, { Max[0], Max[1] - Edge[1], Min[2] },
				{ Max[0], Max[1], Min[2] }, { Max[0], Max[1], Min[2] + Edge[2] },

				{ Min[0], Max[1], Min[2] }, { Min[0] + Edge[0], Max[1], Min[2] },
				{ Min[0], Max[1], Min[2] }, { Min[0], Max[1] - Edge[1], Min[2] },
				{ Min[0], Max[1], Min[2] }, { Min[0], Max[1], Min[2] + Edge[2] },

				{ Max[0], Min[1], Min[2] }, { Max[0] - Edge[0], Min[1], Min[2] },
				{ Max[0], Min[1], Min[2] }, { Max[0], Min[1] + Edge[1], Min[2] },
				{ Max[0], Min[1], Min[2] }, { Max[0], Min[1], Min[2] + Edge[2] },

				{ Min[0], Min[1], Min[2] }, { Min[0] + Edge[0], Min[1], Min[2] },
				{ Min[0], Min[1], Min[2] }, { Min[0], Min[1] + Edge[1], Min[2] },
				{ Min[0], Min[1], Min[2] }, { Min[0], Min[1], Min[2] + Edge[2] },
			};

			Context->SetWorldViewMatrix(lcMul(Piece->mModelWorld, ViewMatrix));

			if (Piece->IsFocused())
				lcSetColorFocused();
			else
				lcSetColorSelected();

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, 48);
		}

		Context->SetLineWidth(Preferences.mLineWidth); // context remove
	}
}

void Project::RenderSceneObjects(View* view)
{
	const lcPreferences& Preferences = lcGetPreferences();
	const lcMatrix44& ViewMatrix = view->mCamera->mWorldView;
	lcContext* Context = view->mContext;

	Context->SetProjectionMatrix(view->GetProjectionMatrix());

#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif

	if (view->mTrackTool == LC_TRACKTOOL_INSERT)
	{
		lcVector3 Position;
		lcVector4 Rotation;
		GetPieceInsertPosition(view, Position, Rotation);

		lcMatrix44 WorldMatrix = lcMatrix44FromAxisAngle(lcVector3(Rotation[0], Rotation[1], Rotation[2]), Rotation[3] * LC_DTOR);
		WorldMatrix.SetTranslation(Position);

		Context->SetWorldViewMatrix(lcMul(WorldMatrix, ViewMatrix));

		Context->SetLineWidth(2.0f * Preferences.mLineWidth);
		gMainWindow->mPreviewWidget->GetCurrentPiece()->RenderPiece(gMainWindow->mColorIndex);
	}

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
		glDisable(GL_LIGHTING);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* pCamera = mCameras[CameraIdx];

		if ((pCamera == view->mCamera) || !pCamera->IsVisible())
			continue;

		pCamera->Render(view);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->Render(view);

	Context->SetLineWidth(Preferences.mLineWidth); // context remove
}

void Project::ZoomExtents(int FirstView, int LastView)
{
	float BoundingBox[6];

	if (!GetPiecesBoundingBox(BoundingBox))
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

	const lcArray<View*> Views = gMainWindow->GetViews();
	for (int vp = FirstView; vp < LastView; vp++)
	{
		View* view = Views[vp];

		view->mCamera->ZoomExtents(view, Center, Points, 8, mCurrentStep, gMainWindow->GetAddKeys());
	}

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void Project::SaveImage()
{
	lcImageDialogOptions Options;
	lcStep LastStep = GetLastStep();

	Options.Width = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	Options.Height = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	Options.Start = mCurrentStep;
	Options.End = LastStep;

	if (!mFileName.isEmpty())
	{
		Options.FileName = mFileName;
		QString Extension = QFileInfo(Options.FileName).suffix();
		Options.FileName = Options.FileName.left(Options.FileName.length() - Extension.length());
	}
	else
		Options.FileName = QLatin1String("image");

	Options.FileName += lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);

	if (!gMainWindow->DoDialog(LC_DIALOG_SAVE_IMAGE, &Options))
		return;
	
	QString Extension = QFileInfo(Options.FileName).suffix();

	if (!Extension.isEmpty())
		lcSetProfileString(LC_PROFILE_IMAGE_EXTENSION, Options.FileName.right(Extension.length() + 1));

	lcSetProfileInt(LC_PROFILE_IMAGE_WIDTH, Options.Width);
	lcSetProfileInt(LC_PROFILE_IMAGE_HEIGHT, Options.Height);

	if (Options.Start != Options.End)
		Options.FileName = Options.FileName.insert(Options.FileName.length() - Extension.length() - 1, QLatin1String("%1"));

	SaveStepImages(Options.FileName, Options.Width, Options.Height, Options.Start, Options.End);
}

void Project::SaveStepImages(const QString& BaseName, int Width, int Height, lcStep Start, lcStep End)
{
	gMainWindow->mPreviewWidget->MakeCurrent();
	lcContext* Context = gMainWindow->mPreviewWidget->mContext;

	if (!Context->BeginRenderToTexture(Width, Height))
	{
		gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
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
		SetCurrentStep(Step);
		Render(&View, true);

		QString FileName = BaseName.arg(Step, 2, 10, QLatin1Char('0'));
		if (!Context->SaveRenderToTextureImage(FileName, Width, Height))
			break;
	}

	SetCurrentStep(CurrentStep);

	Context->EndRenderToTexture();
}

void Project::CreateHTMLPieceList(QTextStream& Stream, lcStep Step, bool Images, const QString& ImageExtension)
{
	int* ColorsUsed = new int[gColorList.GetSize()];
	memset(ColorsUsed, 0, sizeof(ColorsUsed[0]) * gColorList.GetSize());
	int* PiecesUsed = new int[gColorList.GetSize()];
	int NumColors = 0;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if ((Piece->GetStepShow() == Step) || (Step == 0))
			ColorsUsed[Piece->mColorIndex]++;
	}

	Stream << QLatin1String("<br><table border=1><tr><td><center>Piece</center></td>\r\n");

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		if (ColorsUsed[ColorIdx])
		{
			ColorsUsed[ColorIdx] = NumColors;
			NumColors++;
			Stream << QString("<td><center>%1</center></td>\n").arg(gColorList[ColorIdx].Name);
		}
	}
	NumColors++;
	Stream << QLatin1String("</tr>\n");

	PieceInfo* pInfo;
	for (int j = 0; j < lcGetPiecesLibrary()->mPieces.GetSize(); j++)
	{
		bool Add = false;
		memset(PiecesUsed, 0, sizeof(PiecesUsed[0]) * gColorList.GetSize());
		pInfo = lcGetPiecesLibrary()->mPieces[j];

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];

			if ((Piece->mPieceInfo == pInfo) && ((Piece->GetStepShow() == Step) || (Step == 0)))
			{
				PiecesUsed[Piece->mColorIndex]++;
				Add = true;
			}
		}

		if (Add)
		{
			if (Images)
				Stream << QString("<tr><td><IMG SRC=\"%1%2\" ALT=\"%3\"></td>\n").arg(pInfo->m_strName, ImageExtension, pInfo->m_strDescription);
			else
				Stream << QString("<tr><td>%1</td>\r\n").arg(pInfo->m_strDescription);

			int curcol = 1;
			for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
			{
				if (PiecesUsed[ColorIdx])
				{
					while (curcol != ColorsUsed[ColorIdx] + 1)
					{
						Stream << QLatin1String("<td><center>-</center></td>\r\n");
						curcol++;
					}

					Stream << QString("<td><center>%1</center></td>\r\n").arg(QString::number(PiecesUsed[ColorIdx]));
					curcol++;
				}
			}

			while (curcol != NumColors)
			{
				Stream << QLatin1String("<td><center>-</center></td>\r\n");
				curcol++;
			}

			Stream << QLatin1String("</tr>\r\n");
		}
	}
	Stream << QLatin1String("</table>\r\n<br>");

	delete[] PiecesUsed;
	delete[] ColorsUsed;
}

void Project::ExportHTML()
{
	lcHTMLDialogOptions Options;

	if (!mFileName.isEmpty())
		Options.PathName = QFileInfo(mFileName).canonicalPath();

	int ImageOptions = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS);
	int HTMLOptions = lcGetProfileInt(LC_PROFILE_HTML_OPTIONS);

	Options.ImageFormat = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));
	Options.TransparentImages = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
	Options.SinglePage = (HTMLOptions & LC_HTML_SINGLEPAGE) != 0;
	Options.IndexPage = (HTMLOptions & LC_HTML_INDEX) != 0;
	Options.StepImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH);
	Options.StepImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT);
	Options.HighlightNewParts = (HTMLOptions & LC_HTML_HIGHLIGHT) != 0;
	Options.PartsListStep = (HTMLOptions & LC_HTML_LISTSTEP) != 0;
	Options.PartsListEnd = (HTMLOptions & LC_HTML_LISTEND) != 0;
	Options.PartsListImages = (HTMLOptions & LC_HTML_IMAGES) != 0;
	Options.PartImagesColor = lcGetColorIndex(lcGetProfileInt(LC_PROFILE_HTML_PARTS_COLOR));
	Options.PartImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH);
	Options.PartImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT);

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_HTML, &Options))
		return;

	HTMLOptions = 0;

	if (Options.SinglePage)
		HTMLOptions |= LC_HTML_SINGLEPAGE;
	if (Options.IndexPage)
		HTMLOptions |= LC_HTML_INDEX;
	if (Options.HighlightNewParts)
		HTMLOptions |= LC_HTML_HIGHLIGHT;
	if (Options.PartsListStep)
		HTMLOptions |= LC_HTML_LISTSTEP;
	if (Options.PartsListEnd)
		HTMLOptions |= LC_HTML_LISTEND;
	if (Options.PartsListImages)
		HTMLOptions |= LC_HTML_IMAGES;

	ImageOptions = Options.ImageFormat;

	if (Options.TransparentImages)
		ImageOptions |= LC_IMAGE_TRANSPARENT;

	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, ImageOptions);
	lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, Options.StepImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, Options.StepImagesHeight);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_COLOR, lcGetColorCode(Options.PartImagesColor));
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH, Options.PartImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT, Options.PartImagesHeight);

	QDir Dir(Options.PathName);
	Dir.mkpath(QLatin1String("."));

	QString Title = GetTitle();
	QString BaseName = Title.left(Title.length() - QFileInfo(Title).suffix().length() - 1);
	QString HTMLExtension = QLatin1String(".html");
	QString ImageExtension;
	lcStep LastStep = GetLastStep();

	switch (Options.ImageFormat)
	{
	case LC_IMAGE_BMP:
		ImageExtension = QLatin1String(".bmp");
		break;
	case LC_IMAGE_JPG:
		ImageExtension = QLatin1String(".jpg");
		break;
	default:
	case LC_IMAGE_PNG:
		ImageExtension = QLatin1String(".png");
		break;
	}
	
	if (Options.SinglePage)
	{
		QString FileName = QFileInfo(Dir, BaseName + HTMLExtension).absoluteFilePath();
		QFile File(FileName);

		if (!File.open(QIODevice::WriteOnly))
		{
			QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
			return;
		}

		QTextStream Stream(&File);

		Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(Title);

		for (lcStep Step = 1; Step <= LastStep; Step++)
		{
			QString StepString = QString("%1").arg(Step, 2, 10, QLatin1Char('0'));
			Stream << QString("<IMG SRC=\"%1-%2%3\" ALT=\"Step %4\" WIDTH=%5 HEIGHT=%6><BR><BR>\r\n").arg(BaseName, StepString, ImageExtension, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

			if (Options.PartsListStep)
				CreateHTMLPieceList(Stream, Step, Options.PartsListImages, ImageExtension);
		}

		if (Options.PartsListEnd)
			CreateHTMLPieceList(Stream, 0, Options.PartsListImages, ImageExtension);

		Stream << QLatin1String("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
	}
	else
	{
		if (Options.IndexPage)
		{
			QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-index") + HTMLExtension).absoluteFilePath();
			QFile File(FileName);

			if (!File.open(QIODevice::WriteOnly))
			{
				QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(Title);

			for (lcStep Step = 1; Step <= LastStep; Step++)
				Stream << QString("<A HREF=\"%1-%2.html\">Step %3<BR>\r\n</A>").arg(BaseName, QString("%1").arg(Step, 2, 10, QLatin1Char('0')), QString::number(Step));

			if (Options.PartsListEnd)
				Stream << QString("<A HREF=\"%1-pieces.html\">Pieces Used</A><BR>\r\n").arg(BaseName);

			Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
		}

		for (lcStep Step = 1; Step <= LastStep; Step++)
		{
			QString StepString = QString("%1").arg(Step, 2, 10, QLatin1Char('0'));
			QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-") + StepString + HTMLExtension).absoluteFilePath();
			QFile File(FileName);

			if (!File.open(QIODevice::WriteOnly))
			{
				QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>%1 - Step %2</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(Title, QString::number(Step));
			Stream << QString("<IMG SRC=\"%1-%2%3\" ALT=\"Step %4\" WIDTH=%5 HEIGHT=%6><BR><BR>\r\n").arg(BaseName, StepString, ImageExtension, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

			if (Options.PartsListStep)
				CreateHTMLPieceList(Stream, Step, Options.PartsListImages, ImageExtension);

			Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR>");
			if (Step != 1)
				Stream << QString("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(Step - 1, 2, 10, QLatin1Char('0')));

			if (Options.IndexPage)
				Stream << QString("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

			if (Step != LastStep)
				Stream << QString("<A HREF=\"%1-%2.html\">Next</A>").arg(BaseName, QString("%1").arg(Step + 1, 2, 10, QLatin1Char('0')));
			else if (Options.PartsListEnd)
				Stream << QString("<A HREF=\"%1-pieces.html\">Pieces Used</A>").arg(BaseName);

			Stream << QLatin1String("<BR></HTML>\r\n");
		}

		if (Options.PartsListEnd)
		{
			QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-pieces") + HTMLExtension).absoluteFilePath();
			QFile File(FileName);

			if (!File.open(QIODevice::WriteOnly))
			{
				QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>Pieces used by %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\n").arg(Title);

			CreateHTMLPieceList(Stream, 0, Options.PartsListImages, ImageExtension);

			Stream << QLatin1String("</CENTER>\n<BR><HR><BR>");
			Stream << QString("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(LastStep, 2, 10, QLatin1Char('0')));

			if (Options.IndexPage)
				Stream << QString("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

			Stream << QLatin1String("<BR></HTML>\r\n");
		}
	}

	QString StepImageBaseName = QFileInfo(Dir, BaseName + QLatin1String("-%1") + ImageExtension).absoluteFilePath();
	SaveStepImages(StepImageBaseName, Options.StepImagesWidth, Options.StepImagesHeight, 1, LastStep);

	if (Options.PartsListImages)
	{
		gMainWindow->mPreviewWidget->MakeCurrent();
		lcContext* Context = gMainWindow->mPreviewWidget->mContext;
		int Width = Options.PartImagesWidth;
		int Height = Options.PartImagesHeight;

		if (!Context->BeginRenderToTexture(Width, Height))
		{
			gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
			return;
		}

		float aspect = (float)Width/(float)Height;
		Context->SetViewport(0, 0, Width, Height);

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			lcPiece* Piece = mPieces[PieceIdx];
			bool Skip = false;
			PieceInfo* Info = Piece->mPieceInfo;

			for (int CheckIdx = 0; CheckIdx < PieceIdx; CheckIdx++)
			{
				if (mPieces[CheckIdx]->mPieceInfo == Info)
				{
					Skip = true;
					break;
				}
			}

			if (Skip)
				continue;

			glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Info->ZoomExtents(30.0f, aspect);
			Info->RenderPiece(Options.PartImagesColor);
			glFinish();

			QString FileName = QFileInfo(Dir, Info->m_strName + ImageExtension).absoluteFilePath();
			if (!Context->SaveRenderToTextureImage(FileName, Width, Height))
				break;
		}
		Context->EndRenderToTexture();
	}
}

void Project::HandleCommand(LC_COMMANDS id)
{
	switch (id)
	{
		case LC_FILE_NEW:
		{
			if (!SaveIfModified())
				return;

			OnNewDocument();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_FILE_OPEN:
		{
			QString FileName;

			if (!mFileName.isEmpty())
				FileName = mFileName;
			else
				FileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

			FileName = QFileDialog::getOpenFileName(gMainWindow->mHandle, tr("Open Project"), FileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

			if (!FileName.isEmpty())
				OpenProject(FileName);
		} break;

		case LC_FILE_MERGE:
		{
			QString FileName;

			if (!mFileName.isEmpty())
				FileName = mFileName;
			else
				FileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

			FileName = QFileDialog::getOpenFileName(gMainWindow->mHandle, tr("Merge Project"), FileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

			if (!FileName.isEmpty())
			{
				// todo: detect format
				lcDiskFile file;
				if (file.Open(FileName.toLatin1().constData(), "rb")) // todo: qstring
				{
					if (file.GetLength() != 0)
					{
						FileLoad(&file, false, true);
						CheckPoint("Merging");
					}
					file.Close();
				}
			}
		} break;

	case LC_FILE_SAVE:
		DoSave(mFileName);
		break;

	case LC_FILE_SAVEAS:
		DoSave(QString());
		break;

	case LC_FILE_SAVE_IMAGE:
		SaveImage();
		break;

	case LC_FILE_EXPORT_3DS:
		Export3DStudio();
		break;

	case LC_FILE_EXPORT_HTML:
		ExportHTML();
		break;

	case LC_FILE_EXPORT_BRICKLINK:
		ExportBrickLink();
		break;

	case LC_FILE_EXPORT_CSV:
		ExportCSV();
		break;

	case LC_FILE_EXPORT_POVRAY:
		ExportPOVRay();
		break;

	case LC_FILE_EXPORT_WAVEFRONT:
		ExportWavefront();
		break;

		case LC_FILE_PROPERTIES:
		{
			lcPropertiesDialogOptions Options;

			Options.Properties = mProperties;
			Options.Title = GetTitle();
			Options.SetDefault = false;

			GetPartsList(Options.PartsList);

			if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
				break;

			if (Options.SetDefault)
				Options.Properties.SaveDefaults();

			if (mProperties == Options.Properties)
				break;

			mProperties = Options.Properties;

			UpdateBackgroundTexture();

			CheckPoint("Properties");
		} break;

		case LC_FILE_PRINT_PREVIEW:
			gMainWindow->TogglePrintPreview();
			break;

		case LC_FILE_PRINT:
			gMainWindow->DoDialog(LC_DIALOG_PRINT, NULL);
			break;

	// TODO: printing
	case LC_FILE_PRINT_BOM:
		break;

	case LC_FILE_RECENT1:
	case LC_FILE_RECENT2:
	case LC_FILE_RECENT3:
	case LC_FILE_RECENT4:
		if (!OpenProject(gMainWindow->mRecentFiles[id - LC_FILE_RECENT1]))
			gMainWindow->RemoveRecentFile(id - LC_FILE_RECENT1);
		break;

	case LC_FILE_EXIT:
		gMainWindow->Close();
		break;

	case LC_EDIT_UNDO:
		UndoAction();
		break;

	case LC_EDIT_REDO:
		RedoAction();
		break;

	case LC_EDIT_CUT:
	case LC_EDIT_COPY:
		{
			lcMemFile* Clipboard = new lcMemFile();

			int i = 0;
//			lcLight* pLight;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				if (mPieces[PieceIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
					Piece->FileSave(*Clipboard);
			}

			i = mGroups.GetSize();
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				mGroups[GroupIdx]->FileSave(Clipboard, mGroups);

			i = 0;
			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				lcCamera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
					pCamera->FileSave(*Clipboard);
			}
/*
			for (i = 0, pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					i++;
			Clipboard->Write(&i, sizeof(i));

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					pLight->FileSave(Clipboard);
*/
			if (id == LC_EDIT_CUT)
			{
				RemoveSelectedObjects();
				gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				CheckPoint("Cutting");
			}

			g_App->ExportClipboard(Clipboard);
		} break;

		case LC_EDIT_PASTE:
		{
			lcFile* file = g_App->mClipboard;
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			ClearSelection(false);

			lcArray<lcPiece*> PastedPieces;
			int NumPieces;
			file->ReadBuffer(&NumPieces, sizeof(NumPieces));

			while (NumPieces--)
			{
				lcPiece* Piece = new lcPiece(NULL);
				Piece->FileLoad(*file);
				PastedPieces.Add(Piece);
			}

			lcArray<lcGroup*> Groups;
			int NumGroups;
			file->ReadBuffer(&NumGroups, sizeof(NumGroups));

			while (NumGroups--)
			{
				lcGroup* Group = new lcGroup();
				Group->FileLoad(file);
				Groups.Add(Group);
			}

			for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = PastedPieces[PieceIdx];
				Piece->CreateName(mPieces);
				Piece->SetStepShow(mCurrentStep);
				mPieces.Add(Piece);
				Piece->SetSelected(true);

				int GroupIndex = LC_POINTER_TO_INT(Piece->GetGroup());
				if (GroupIndex != -1)
					Piece->SetGroup(Groups[GroupIndex]);
				else
					Piece->SetGroup(NULL);
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				int GroupIndex = LC_POINTER_TO_INT(Group->mGroup);
				Group->mGroup = (GroupIndex != -1) ? Groups[GroupIndex] : NULL;
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				bool Add = false;

				for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = PastedPieces[PieceIdx];
					
					for (lcGroup* PieceGroup = Piece->GetGroup(); PieceGroup; PieceGroup = PieceGroup->mGroup)
					{
						if (PieceGroup == Group)
						{
							Add = true;
							break;
						}
					}

					if (Add)
						break;
				}

				if (Add)
				{
					int a, max = 0;

					for (int SearchGroupIdx = 0; SearchGroupIdx < mGroups.GetSize(); SearchGroupIdx++)
					{
						lcGroup* SearchGroup = mGroups[SearchGroupIdx];

						if (strncmp("Pasted Group #", SearchGroup ->m_strName, 14) == 0)
							if (sscanf(SearchGroup ->m_strName + 14, "%d", &a) == 1)
								if (a > max)
									max = a;
					}

					sprintf(Group->m_strName, "Pasted Group #%.2d", max+1);
					mGroups.Add(Group);
				}
				else
					delete Group;
			}

			int NumCameras;
			file->ReadBuffer(&NumCameras, sizeof(NumCameras));

			while (NumCameras--)
			{
				lcCamera* pCamera = new lcCamera(false);
				pCamera->FileLoad(*file);
				pCamera->CreateName(mCameras);
				pCamera->SetSelected(true);
				mCameras.Add(pCamera);
			}

			// TODO: lights
			CalculateStep();
			CheckPoint("Pasting");
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

	case LC_EDIT_FIND:
		if (gMainWindow->DoDialog(LC_DIALOG_FIND, &gMainWindow->mSearchOptions))
			FindPiece(true, true);
		break;

	case LC_EDIT_FIND_NEXT:
		FindPiece(false, true);
		break;

	case LC_EDIT_FIND_PREVIOUS:
		FindPiece(false, false);
		break;

	case LC_EDIT_SELECT_ALL:
		SelectAllPieces();
		break;

	case LC_EDIT_SELECT_NONE:
		ClearSelection(true);
		break;

		case LC_EDIT_SELECT_INVERT:
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
		} break;

		case LC_EDIT_SELECT_BY_NAME:
		{
			lcSelectDialogOptions Options;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				Options.Selection.Add(mPieces[PieceIdx]->IsSelected());

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsVisible())
					Options.Selection.Add(mCameras[CameraIdx]->IsSelected());

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				if (mLights[LightIdx]->IsVisible())
					Options.Selection.Add(mLights[LightIdx]->IsSelected());

			if (Options.Selection.GetSize() == 0)
			{
				gMainWindow->DoMessageBox("Nothing to select.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
				break;

			ClearSelection(false);

			int ObjectIndex = 0;
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mPieces[PieceIdx]->SetSelected(true);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mCameras[CameraIdx]->SetSelected(true);

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				if (Options.Selection[ObjectIndex])
					mLights[LightIdx]->SetSelected(true);

			UpdateSelection();
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateFocusObject(GetFocusObject());
		} break;

		case LC_VIEW_SPLIT_HORIZONTAL:
			gMainWindow->SplitHorizontal();
			break;

		case LC_VIEW_SPLIT_VERTICAL:
			gMainWindow->SplitVertical();
			break;

		case LC_VIEW_REMOVE_VIEW:
			gMainWindow->RemoveView();
			break;

		case LC_VIEW_RESET_VIEWS:
			gMainWindow->ResetViews();
			break;

		case LC_VIEW_FULLSCREEN:
			gMainWindow->ToggleFullScreen();
			break;

	case LC_VIEW_PROJECTION_PERSPECTIVE:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = ActiveView->mCamera;

			Camera->SetOrtho(false);

			if (Camera->IsFocused())
				gMainWindow->UpdateFocusObject(Camera);
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdatePerspective();
		}
		break;

	case LC_VIEW_PROJECTION_ORTHO:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = ActiveView->mCamera;

			Camera->SetOrtho(true);

			if (Camera->IsFocused())
				gMainWindow->UpdateFocusObject(Camera);
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdatePerspective();
		}
		break;

	case LC_VIEW_PROJECTION_FOCUS:
		{
			lcVector3 FocusVector;
			GetSelectionCenter(FocusVector);
			gMainWindow->GetActiveView()->mCamera->SetFocalPoint(FocusVector, mCurrentStep, gMainWindow->GetAddKeys());
			gMainWindow->UpdateAllViews();
		}
		break;

		case LC_PIECE_INSERT:
		{
			PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();

			if (!CurPiece)
				break;

			lcPiece* Last = mPieces.IsEmpty() ? NULL : mPieces[mPieces.GetSize() - 1];
			lcPiece* pPiece = new lcPiece(CurPiece);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsFocused())
				{
					Last = Piece;
					break;
				}
			}

			if (Last != NULL)
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(Last, Pos, Rot);

				pPiece->Initialize(Pos, Rot, mCurrentStep);
				pPiece->UpdatePosition(mCurrentStep);
			}
			else
				pPiece->Initialize(lcVector3(0, 0, 0), lcVector4(0, 0, 1, 0), mCurrentStep);

			pPiece->SetColorIndex(gMainWindow->mColorIndex);
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);
			ClearSelectionAndSetFocus(pPiece, LC_PIECE_SECTION_POSITION);

			CheckPoint("Inserting");
		} break;

		case LC_PIECE_DELETE:
		{
			if (RemoveSelectedObjects())
			{
				gMainWindow->UpdateFocusObject(NULL);
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				CheckPoint("Deleting");
			}
		} break;

		case LC_PIECE_MOVE_PLUSX:
		case LC_PIECE_MOVE_MINUSX:
		case LC_PIECE_MOVE_PLUSY:
		case LC_PIECE_MOVE_MINUSY:
		case LC_PIECE_MOVE_PLUSZ:
		case LC_PIECE_MOVE_MINUSZ:
		case LC_PIECE_ROTATE_PLUSX:
		case LC_PIECE_ROTATE_MINUSX:
		case LC_PIECE_ROTATE_PLUSY:
		case LC_PIECE_ROTATE_MINUSY:
		case LC_PIECE_ROTATE_PLUSZ:
		case LC_PIECE_ROTATE_MINUSZ:
		{
			lcVector3 axis;
			bool Rotate = id >= LC_PIECE_ROTATE_PLUSX && id <= LC_PIECE_ROTATE_MINUSZ;

			if (Rotate)
			{
				axis[0] = axis[1] = axis[2] = lcMax(gMainWindow->GetAngleSnap(), 1);
			}
			else
			{
				axis[0] = axis[1] = gMainWindow->GetMoveXYSnap();
				axis[2] = gMainWindow->GetMoveZSnap();

				if (!axis[0])// || bControl)
					axis[0] = 0.01f;
				if (!axis[1])// || bControl)
					axis[1] = 0.01f;
				if (!axis[2])// || bControl)
					axis[2] = 0.01f;
			}

			if (id == LC_PIECE_MOVE_PLUSX || id == LC_PIECE_ROTATE_PLUSX)
				axis = lcVector3(axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_MINUSX || id == LC_PIECE_ROTATE_MINUSX)
				axis = lcVector3(-axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_PLUSY || id == LC_PIECE_ROTATE_PLUSY)
				axis = lcVector3(0, axis[1], 0);
			else if (id == LC_PIECE_MOVE_MINUSY || id == LC_PIECE_ROTATE_MINUSY)
				axis = lcVector3(0, -axis[1], 0);
			else if (id == LC_PIECE_MOVE_PLUSZ || id == LC_PIECE_ROTATE_PLUSZ)
				axis = lcVector3(0, 0, axis[2]);
			else if (id == LC_PIECE_MOVE_MINUSZ || id == LC_PIECE_ROTATE_MINUSZ)
				axis = lcVector3(0, 0, -axis[2]);

			if (!lcGetPreferences().mFixedAxes)
			{
				// TODO: rewrite this
			
				lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
				gMainWindow->GetActiveView()->UnprojectPoints(Pts, 3);

				float ax, ay;
				lcVector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
				vx.Normalize();
				lcVector3 x(1, 0, 0);
				ax = acosf(lcDot(vx, x));

				lcVector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
				vy.Normalize();
				lcVector3 y(0, -1, 0);
				ay = acosf(lcDot(vy, y));

				if (ax > 135)
					axis[0] = -axis[0];

				if (ay < 45)
					axis[1] = -axis[1];

				if (ax >= 45 && ax <= 135)
				{
					float tmp = axis[0];

					ax = acosf(lcDot(vx, y));
					if (ax > 90)
					{
						axis[0] = -axis[1];
						axis[1] = tmp;
					}
					else
					{
						axis[0] = axis[1];
						axis[1] = -tmp;
					}
				}
			}

			if (Rotate)
				RotateSelectedPieces(axis);
			else
				MoveSelectedObjects(axis, axis);

			gMainWindow->UpdateAllViews();
			CheckPoint(Rotate ? "Rotating" : "Moving");
			gMainWindow->UpdateFocusObject(GetFocusObject());
		} break;

		case LC_PIECE_MINIFIG_WIZARD:
		{
			lcMinifig Minifig;
			int i;

			if (!gMainWindow->DoDialog(LC_DIALOG_MINIFIG, &Minifig))
				break;

			ClearSelection(false);

			for (i = 0; i < LC_MFW_NUMITEMS; i++)
			{
				if (Minifig.Parts[i] == NULL)
					continue;

				lcPiece* pPiece = new lcPiece(Minifig.Parts[i]);

				lcVector3 Position(Minifig.Matrices[i][3][0], Minifig.Matrices[i][3][1], Minifig.Matrices[i][3][2]);
				lcVector4 Rotation = lcMatrix44ToAxisAngle(Minifig.Matrices[i]);
				Rotation[3] *= LC_RTOD;
				pPiece->Initialize(Position, Rotation, mCurrentStep);
				pPiece->SetColorIndex(Minifig.Colors[i]);
				pPiece->CreateName(mPieces);
				mPieces.Add(pPiece);
				pPiece->SetSelected(true);
				pPiece->UpdatePosition(mCurrentStep);
			}

			int Max = 0;

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (strncmp(Group->m_strName, "Minifig #", 9) == 0)
					if (sscanf(Group->m_strName, "Minifig #%d", &i) == 1)
						if (i > Max)
							Max = i;
			}

			lcGroup* Group = AddGroup(NULL);
			sprintf(Group->m_strName, "Minifig #%.2d", Max+1);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
					Piece->SetGroup(Group);
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			CheckPoint("Minifig");
		} break;

		case LC_PIECE_ARRAY:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			int sel = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->CompareBoundingBox(bs);
					sel++;
				}
			}

			if (!sel)
			{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			lcArrayDialogOptions Options;

			memset(&Options, 0, sizeof(Options));
			Options.Counts[0] = 10;
			Options.Counts[1] = 1;
			Options.Counts[2] = 1;

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_ARRAY, &Options))
				break;

			if (Options.Counts[0] * Options.Counts[1] * Options.Counts[2] < 2)
			{
				gMainWindow->DoMessageBox("Array only has 1 element or less, no pieces added.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			lcArray<lcObjectSection> NewPieces;

			for (int Step1 = 0; Step1 < Options.Counts[0]; Step1++)
			{
				for (int Step2 = 0; Step2 < Options.Counts[1]; Step2++)
				{
					for (int Step3 = 0; Step3 < Options.Counts[2]; Step3++)
					{
						if (Step1 == 0 && Step2 == 0 && Step3 == 0)
							continue;

						lcMatrix44 ModelWorld;
						lcVector3 Position;

						lcVector3 RotationAngles = Options.Rotations[0] * Step1 + Options.Rotations[1] * Step2 + Options.Rotations[2] * Step3;
						lcVector3 Offset = Options.Offsets[0] * Step1 + Options.Offsets[1] * Step2 + Options.Offsets[2] * Step3;

						for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
						{
							lcPiece* pPiece = mPieces[PieceIdx];

							if (!pPiece->IsSelected())
								continue;

							if (sel == 1)
							{
								ModelWorld = lcMul(pPiece->mModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));

								Position = pPiece->mPosition;
							}
							else
							{
								lcVector4 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2, 0.0f);
								ModelWorld = pPiece->mModelWorld;

								ModelWorld.r[3] -= Center;
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));
								ModelWorld.r[3] += Center;

								Position = lcVector3(ModelWorld.r[3].x, ModelWorld.r[3].y, ModelWorld.r[3].z);
							}

							lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
							AxisAngle[3] *= LC_RTOD;

							lcPiece* NewPiece = new lcPiece(pPiece->mPieceInfo);
							NewPiece->Initialize(Position + Offset, AxisAngle, mCurrentStep);
							NewPiece->SetColorIndex(pPiece->mColorIndex);

							lcObjectSection ObjectSection;
							ObjectSection.Object = NewPiece;
							ObjectSection.Section = LC_PIECE_SECTION_POSITION;
							NewPieces.Add(ObjectSection);
						}
					}
				}
			}

			for (int PieceIdx = 0; PieceIdx < NewPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = (lcPiece*)NewPieces[PieceIdx].Object;
				Piece->CreateName(mPieces);
				Piece->UpdatePosition(mCurrentStep);
				mPieces.Add(Piece);
			}

			AddToSelection(NewPieces);
			CheckPoint("Array");
		} break;

		case LC_PIECE_GROUP:
		{
			int i, Max = 0;
			char name[65];
			int Selected = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				if (mPieces[PieceIdx]->IsSelected())
				{
					Selected++;

					if (Selected > 1)
						break;
				}
			}

			if (!Selected)
			{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (strncmp(Group->m_strName, "Group #", 7) == 0)
					if (sscanf(Group->m_strName, "Group #%d", &i) == 1)
						if (i > Max)
							Max = i;
			}

			sprintf(name, "Group #%.2d", Max + 1);

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_GROUP, name))
				break;

			lcGroup* NewGroup = new lcGroup();
			strcpy(NewGroup->m_strName, name);
			mGroups.Add(NewGroup);

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

			RemoveEmptyGroups();
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_UNGROUP:
		{
			lcArray<lcGroup*> Groups;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					lcGroup* Group = Piece->GetTopGroup();

					if (Groups.FindIndex(Group) == -1)
					{
						mGroups.Remove(Group);
						Groups.Add(Group);
					}
				}
			}

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];
				lcGroup* Group = Piece->GetGroup();

				if (Groups.FindIndex(Group) != -1)
					Piece->SetGroup(NULL);
			}

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (Groups.FindIndex(Group->mGroup) != -1)
					Group->mGroup = NULL;
			}

			Groups.DeleteAll();

			RemoveEmptyGroups();
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			lcGroup* pGroup = NULL;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					pGroup = Piece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}
			}

			if (pGroup != NULL)
			{
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = mPieces[PieceIdx];

					if (Piece->IsFocused())
					{
						Piece->SetGroup(pGroup);
						break;
					}
				}
			}

			RemoveEmptyGroups();
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_GROUP_REMOVE:
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
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_EDIT:
		{
			lcEditGroupsDialogOptions Options;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				Options.PieceParents.Add(mPieces[PieceIdx]->GetGroup());

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				Options.GroupParents.Add(mGroups[GroupIdx]->mGroup);

			if (!gMainWindow->DoDialog(LC_DIALOG_EDIT_GROUPS, &Options))
				break;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				mPieces[PieceIdx]->SetGroup(Options.PieceParents[PieceIdx]);

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				mGroups[GroupIdx]->mGroup = Options.GroupParents[GroupIdx];

			RemoveEmptyGroups();
			ClearSelection(true);
			CheckPoint("Editing");
		} break;

		case LC_PIECE_HIDE_SELECTED:
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
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (!Piece->IsSelected())
					Piece->SetHidden(true);
			}

			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				mPieces[PieceIdx]->SetHidden(false);
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_PIECE_SHOW_EARLIER:
		{
			bool Redraw = false;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					lcStep Step = Piece->GetStepShow();

					if (Step > 1)
					{
						Redraw = true;
						Piece->SetStepShow(Step - 1);
					}
				}
			}

			if (Redraw)
			{
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
				UpdateSelection();
			}
		} break;

		case LC_PIECE_SHOW_LATER:
		{
			bool Redraw = false;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					lcStep Step = Piece->GetStepShow();

					if (Step < LC_STEP_MAX)
					{
						Step++;
						Redraw = true;
						Piece->SetStepShow(Step);

						if (Step > mCurrentStep)
							Piece->SetSelected(false);
					}
				}
			}

			if (Redraw)
			{
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
				UpdateSelection();
			}
		} break;

		case LC_VIEW_PREFERENCES:
			g_App->ShowPreferencesDialog();
			break;

		case LC_VIEW_ZOOM_IN:
		{
			ZoomActiveView(-1);
		} break;

		case LC_VIEW_ZOOM_OUT:
		{
			ZoomActiveView(1);
		} break;

		case LC_VIEW_ZOOM_EXTENTS:
		{
			int FirstView, LastView;

			// TODO: Find a way to let users zoom extents all views
//			if (Sys_KeyDown(KEY_CONTROL))
//			{
//				FirstView = 0;
//				LastView = m_ViewList.GetSize();
//			}
//			else
			{
				FirstView = gMainWindow->GetViews().FindIndex(gMainWindow->GetActiveView());
				LastView = FirstView + 1;
			}

			ZoomExtents(FirstView, LastView);
		} break;

		case LC_VIEW_LOOK_AT:
		{
			lcVector3 Center;

			if (!GetSelectionCenter(Center))
			{
				float BoundingBox[6];

				if (GetPiecesBoundingBox(BoundingBox))
					Center = lcVector3((BoundingBox[0] + BoundingBox[3]) / 2, (BoundingBox[1] + BoundingBox[4]) / 2, (BoundingBox[2] + BoundingBox[5]) / 2);
				else
					Center = lcVector3(0.0f, 0.0f, 0.0f);
			}

			gMainWindow->GetActiveView()->mCamera->Center(Center, mCurrentStep, gMainWindow->GetAddKeys());
			gMainWindow->UpdateAllViews();
			break;
		}

		case LC_VIEW_TIME_NEXT:
		{
			if (mCurrentStep == LC_STEP_MAX)
				break;

			mCurrentStep++;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateCurrentStep();
		} break;

		case LC_VIEW_TIME_PREVIOUS:
		{
			if (mCurrentStep == 1)
				break;
			mCurrentStep--;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateCurrentStep();
		} break;

		case LC_VIEW_TIME_FIRST:
		{
			mCurrentStep = 1;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateCurrentStep();
		} break;

		case LC_VIEW_TIME_LAST:
		{
			mCurrentStep = GetLastStep();

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateCurrentStep();
		} break;

		case LC_VIEW_TIME_INSERT:
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

			CheckPoint("Adding Step");
			CalculateStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_TIME_DELETE:
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

			CheckPoint("Removing Step");
			CalculateStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_VIEWPOINT_FRONT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_FRONT, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_BACK:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_BACK, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_TOP:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_TOP, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_BOTTOM:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_BOTTOM, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_LEFT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_LEFT, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_RIGHT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_RIGHT, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_HOME:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_HOME, mCurrentStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_NONE:
		case LC_VIEW_CAMERA1:
		case LC_VIEW_CAMERA2:
		case LC_VIEW_CAMERA3:
		case LC_VIEW_CAMERA4:
		case LC_VIEW_CAMERA5:
		case LC_VIEW_CAMERA6:
		case LC_VIEW_CAMERA7:
		case LC_VIEW_CAMERA8:
		case LC_VIEW_CAMERA9:
		case LC_VIEW_CAMERA10:
		case LC_VIEW_CAMERA11:
		case LC_VIEW_CAMERA12:
		case LC_VIEW_CAMERA13:
		case LC_VIEW_CAMERA14:
		case LC_VIEW_CAMERA15:
		case LC_VIEW_CAMERA16:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = NULL;

			if (id == LC_VIEW_CAMERA_NONE)
			{
				Camera = ActiveView->mCamera;

				if (!Camera->IsSimple())
				{
					ActiveView->SetCamera(Camera, true);
					Camera = ActiveView->mCamera;
				}
			}
			else
			{
				if (id - LC_VIEW_CAMERA1 < mCameras.GetSize())
				{
					Camera = mCameras[id - LC_VIEW_CAMERA1];
					ActiveView->SetCamera(Camera, false);
				}
				else
					break;
			}

			gMainWindow->UpdateCurrentCamera(mCameras.FindIndex(ActiveView->mCamera));
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_RESET:
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
				Views[ViewIdx]->SetDefaultCamera();

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				delete mCameras[CameraIdx];
			mCameras.RemoveAll();

			gMainWindow->UpdateCameraMenu();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
		} break;

		case LC_HELP_HOMEPAGE:
			g_App->OpenURL("http://www.leocad.org/");
			break;

		case LC_HELP_EMAIL:
			g_App->OpenURL("mailto:leozide@gmail.com?subject=LeoCAD");
			break;

		case LC_HELP_UPDATES:
			gMainWindow->DoDialog(LC_DIALOG_CHECK_UPDATES, NULL);
			break;

		case LC_HELP_ABOUT:
		{
			String Info;
			char Text[256];

			gMainWindow->GetActiveView()->MakeCurrent();

			GLint Red, Green, Blue, Alpha, Depth, Stencil;
			GLboolean DoubleBuffer, RGBA;

			glGetIntegerv(GL_RED_BITS, &Red);
			glGetIntegerv(GL_GREEN_BITS, &Green);
			glGetIntegerv(GL_BLUE_BITS, &Blue);
			glGetIntegerv(GL_ALPHA_BITS, &Alpha);
			glGetIntegerv(GL_DEPTH_BITS, &Depth);
			glGetIntegerv(GL_STENCIL_BITS, &Stencil);
			glGetBooleanv(GL_DOUBLEBUFFER, &DoubleBuffer);
			glGetBooleanv(GL_RGBA_MODE, &RGBA);

			Info = "OpenGL Version ";
			Info += (const char*)glGetString(GL_VERSION);
			Info += "\n";
			Info += (const char*)glGetString(GL_RENDERER);
			Info += " - ";
			Info += (const char*)glGetString(GL_VENDOR);
			sprintf(Text, "\n\nColor Buffer: %d bits %s %s", Red + Green + Blue + Alpha, RGBA ? "RGBA" : "indexed", DoubleBuffer ? "double buffered" : "");
			Info += Text;
			sprintf(Text, "\nDepth Buffer: %d bits", Depth);
			Info += Text;
			sprintf(Text, "\nStencil Buffer: %d bits", Stencil);
			Info += Text;
			Info += "\nGL_ARB_vertex_buffer_object extension: ";
			Info += GL_HasVertexBufferObject() ? "supported" : "not supported";
			Info += "\nGL_ARB_framebuffer_object extension: ";
			Info += GL_HasFramebufferObjectARB() ? "supported" : "not supported";
			Info += "\nGL_EXT_framebuffer_object extension: ";
			Info += GL_HasFramebufferObjectEXT() ? "supported" : "not supported";
			Info += "\nGL_EXT_texture_filter_anisotropic extension: ";
			if (GL_SupportsAnisotropic)
			{
				sprintf(Text, "supported (max %d)", (int)GL_MaxAnisotropy);
				Info += Text;
			}
			else
				Info += "not supported";

			gMainWindow->DoDialog(LC_DIALOG_ABOUT, (char*)Info);
		} break;

		case LC_VIEW_TIME_ADD_KEYS:
			gMainWindow->SetAddKeys(!gMainWindow->GetAddKeys());
			break;

	case LC_EDIT_SNAP_RELATIVE:
		{
			lcPreferences& Preferences = lcGetPreferences();
			Preferences.SetForceGlobalTransforms(!Preferences.mForceGlobalTransforms);
		} break;

	case LC_EDIT_LOCK_X:
		gMainWindow->SetLockX(!gMainWindow->GetLockX());
		break;

	case LC_EDIT_LOCK_Y:
		gMainWindow->SetLockY(!gMainWindow->GetLockY());
		break;

	case LC_EDIT_LOCK_Z:
		gMainWindow->SetLockZ(!gMainWindow->GetLockZ());
		break;

	case LC_EDIT_LOCK_NONE:
		gMainWindow->SetLockX(false);
		gMainWindow->SetLockY(false);
		gMainWindow->SetLockZ(false);
		break;

	case LC_EDIT_SNAP_MOVE_XY0:
	case LC_EDIT_SNAP_MOVE_XY1:
	case LC_EDIT_SNAP_MOVE_XY2:
	case LC_EDIT_SNAP_MOVE_XY3:
	case LC_EDIT_SNAP_MOVE_XY4:
	case LC_EDIT_SNAP_MOVE_XY5:
	case LC_EDIT_SNAP_MOVE_XY6:
	case LC_EDIT_SNAP_MOVE_XY7:
	case LC_EDIT_SNAP_MOVE_XY8:
	case LC_EDIT_SNAP_MOVE_XY9:
		gMainWindow->SetMoveXYSnapIndex(id - LC_EDIT_SNAP_MOVE_XY0);
		break;

	case LC_EDIT_SNAP_MOVE_Z0:
	case LC_EDIT_SNAP_MOVE_Z1:
	case LC_EDIT_SNAP_MOVE_Z2:
	case LC_EDIT_SNAP_MOVE_Z3:
	case LC_EDIT_SNAP_MOVE_Z4:
	case LC_EDIT_SNAP_MOVE_Z5:
	case LC_EDIT_SNAP_MOVE_Z6:
	case LC_EDIT_SNAP_MOVE_Z7:
	case LC_EDIT_SNAP_MOVE_Z8:
	case LC_EDIT_SNAP_MOVE_Z9:
		gMainWindow->SetMoveZSnapIndex(id - LC_EDIT_SNAP_MOVE_Z0);
		break;

	case LC_EDIT_SNAP_ANGLE0:
	case LC_EDIT_SNAP_ANGLE1:
	case LC_EDIT_SNAP_ANGLE2:
	case LC_EDIT_SNAP_ANGLE3:
	case LC_EDIT_SNAP_ANGLE4:
	case LC_EDIT_SNAP_ANGLE5:
	case LC_EDIT_SNAP_ANGLE6:
	case LC_EDIT_SNAP_ANGLE7:
	case LC_EDIT_SNAP_ANGLE8:
	case LC_EDIT_SNAP_ANGLE9:
		gMainWindow->SetAngleSnapIndex(id - LC_EDIT_SNAP_ANGLE0);
		break;

		case LC_EDIT_TRANSFORM:
			TransformSelectedObjects(gMainWindow->GetTransformType(), gMainWindow->GetTransformAmount());
			break;

		case LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION:
		case LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION:
		case LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION:
		case LC_EDIT_TRANSFORM_RELATIVE_ROTATION:
			gMainWindow->SetTransformType((lcTransformType)(id - LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION));
			break;

		case LC_EDIT_ACTION_SELECT:
			gMainWindow->SetTool(LC_TOOL_SELECT);
			break;

		case LC_EDIT_ACTION_INSERT:
			gMainWindow->SetTool(LC_TOOL_INSERT);
			break;

		case LC_EDIT_ACTION_LIGHT:
			gMainWindow->SetTool(LC_TOOL_LIGHT);
			break;

		case LC_EDIT_ACTION_SPOTLIGHT:
			gMainWindow->SetTool(LC_TOOL_SPOTLIGHT);
			break;

		case LC_EDIT_ACTION_CAMERA:
			gMainWindow->SetTool(LC_TOOL_CAMERA);
			break;

		case LC_EDIT_ACTION_MOVE:
			gMainWindow->SetTool(LC_TOOL_MOVE);
			break;

		case LC_EDIT_ACTION_ROTATE:
			gMainWindow->SetTool(LC_TOOL_ROTATE);
			break;

		case LC_EDIT_ACTION_DELETE:
			gMainWindow->SetTool(LC_TOOL_ERASER);
			break;

		case LC_EDIT_ACTION_PAINT:
			gMainWindow->SetTool(LC_TOOL_PAINT);
			break;

		case LC_EDIT_ACTION_ZOOM:
			gMainWindow->SetTool(LC_TOOL_ZOOM);
			break;

		case LC_EDIT_ACTION_ZOOM_REGION:
			gMainWindow->SetTool(LC_TOOL_ZOOM_REGION);
			break;

		case LC_EDIT_ACTION_PAN:
			gMainWindow->SetTool(LC_TOOL_PAN);
			break;

		case LC_EDIT_ACTION_ROTATE_VIEW:
			gMainWindow->SetTool(LC_TOOL_ROTATE_VIEW);
			break;

		case LC_EDIT_ACTION_ROLL:
			gMainWindow->SetTool(LC_TOOL_ROLL);
			break;

		case LC_EDIT_CANCEL:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			if (ActiveView && ActiveView->mTrackButton != LC_TRACKBUTTON_NONE)
				ActiveView->StopTracking(false);
			else
				ClearSelection(true);
		} break;

		case LC_NUM_COMMANDS:
			break;
	}
}

lcGroup* Project::AddGroup(lcGroup* Parent)
{
	lcGroup* NewGroup = new lcGroup();

	int i, Max = 0;

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];

		if (strncmp(Group->m_strName, "Group #", 7) == 0)
			if (sscanf(Group->m_strName, "Group #%d", &i) == 1)
				if (i > Max)
					Max = i;
	}

	sprintf(NewGroup->m_strName, "Group #%.2d", Max + 1);
	mGroups.Add(NewGroup);

	NewGroup->mGroup = Parent;

	return NewGroup;
}

// Find a good starting position/orientation relative to an existing piece.
void Project::GetPieceInsertPosition(lcPiece* OffsetPiece, lcVector3& Position, lcVector4& Rotation)
{
	PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();
	lcVector3 Dist(0, 0, OffsetPiece->mPieceInfo->m_fDimensions[2] - CurPiece->m_fDimensions[5]);
	Dist = SnapPosition(Dist);

	Position = lcMul31(Dist, OffsetPiece->mModelWorld);
	Rotation = OffsetPiece->mRotation;
}

// Try to find a good starting position/orientation for a new piece.
void Project::GetPieceInsertPosition(View* view, lcVector3& Position, lcVector4& Rotation)
{
	// Check if the mouse is over a piece.
	lcPiece* HitPiece = (lcPiece*)view->FindObjectUnderPointer(true).Object;

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	lcVector3 ClickPoints[2] = { lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 0.0f), lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 1.0f) };
	view->UnprojectPoints(ClickPoints, 2);

	PieceInfo* CurPiece = gMainWindow->mPreviewWidget->GetCurrentPiece();
	lcVector3 Intersection;

	if (lcLinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, CurPiece->m_fDimensions[5])))
	{
		Intersection = SnapPosition(Intersection);
		Position = Intersection;
		Rotation = lcVector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position = view->UnprojectPoint(lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 0.9f));
	Rotation = lcVector4(0, 0, 1, 0);
}

void Project::TransformSelectedObjects(lcTransformType Type, const lcVector3& Transform)
{
	switch (Type)
	{
	case LC_TRANSFORM_ABSOLUTE_TRANSLATION:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			lcVector3 Center;
			int nSel = 0;
			lcPiece* pFocus = NULL;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					if (Piece->IsFocused())
						pFocus = Piece;

					Piece->CompareBoundingBox(bs);
					nSel++;
				}
			}

			if (pFocus != NULL)
				Center = pFocus->mPosition;
			else
				Center = lcVector3((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

			lcVector3 Offset = Transform - Center;

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				lcCamera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
				{
					pCamera->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					pCamera->UpdatePosition(mCurrentStep);
				}
			}

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			{
				lcLight* pLight = mLights[LightIdx];

				if (pLight->IsSelected())
				{
					pLight->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					pLight->UpdatePosition (mCurrentStep);
				}
			}

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->Move(mCurrentStep, gMainWindow->GetAddKeys(), Offset);
					Piece->UpdatePosition(mCurrentStep);
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				CheckPoint("Moving");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;

	case LC_TRANSFORM_RELATIVE_TRANSLATION:
		{
/*			lcVector3 Move(Transform);

			if (MoveSelectedObjects(Move, false, false))
			{
				gMainWindow->UpdateAllViews();
				CheckPoint("Moving");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}*/
		} break;

	case LC_TRANSFORM_ABSOLUTE_ROTATION:
		{
			// Create the rotation matrix.
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

			int nSel = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->SetRotation(NewRotation, mCurrentStep, gMainWindow->GetAddKeys());
					Piece->UpdatePosition(mCurrentStep);
					nSel++;
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;

	case LC_TRANSFORM_RELATIVE_ROTATION:
		{
			lcVector3 Rotate(Transform);

			if (RotateSelectedPieces(Rotate))
			{
				gMainWindow->UpdateAllViews();
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;
	}
}

void Project::ModifyObject(lcObject* Object, lcObjectProperty Property, void* Value)
{
	const char* CheckPointString = NULL;

	switch (Property)
	{
	case LC_PIECE_PROPERTY_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			lcPiece* Piece = (lcPiece*)Object;

			if (Piece->mPosition != Position)
			{
				Piece->SetPosition(Position, mCurrentStep, gMainWindow->GetAddKeys());
				Piece->UpdatePosition(mCurrentStep);

				CheckPointString = "Moving";
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

				CheckPointString = "Rotating";
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

				CheckPointString = "Show";
			}
		} break;

	case LC_PIECE_PROPERTY_HIDE:
		{
			lcStep Step = *(lcuint32*)Value;
			lcPiece* Part = (lcPiece*)Object;

			if (Step != Part->GetStepHide())
			{
				Part->SetStepHide(Step);

				CheckPointString = "Hide";
			}
		} break;

	case LC_PIECE_PROPERTY_COLOR:
		{
			int ColorIndex = *(int*)Value;
			lcPiece* Part = (lcPiece*)Object;

			if (ColorIndex != Part->mColorIndex)
			{
				Part->SetColorIndex(ColorIndex);

				CheckPointString = "Color";
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

				CheckPointString = "Part";
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

				CheckPointString = "Camera";
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

				CheckPointString = "Camera";
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

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_ORTHO:
		{
			bool Ortho = *(bool*)Value;
			lcCamera* camera = (lcCamera*)Object;

			if (camera->IsOrtho() != Ortho)
			{
				camera->SetOrtho(Ortho);
				camera->UpdatePosition(mCurrentStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_FOV:
		{
			float FOV = *(float*)Value;
			lcCamera* camera = (lcCamera*)Object;

			if (camera->m_fovy != FOV)
			{
				camera->m_fovy = FOV;
				camera->UpdatePosition(mCurrentStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_NEAR:
		{
			float Near = *(float*)Value;
			lcCamera* camera = (lcCamera*)Object;

			if (camera->m_zNear != Near)
			{
				camera->m_zNear= Near;
				camera->UpdatePosition(mCurrentStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_FAR:
		{
			float Far = *(float*)Value;
			lcCamera* camera = (lcCamera*)Object;

			if (camera->m_zFar != Far)
			{
				camera->m_zFar = Far;
				camera->UpdatePosition(mCurrentStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_NAME:
		{
			const char* Name = (const char*)Value;
			lcCamera* camera = (lcCamera*)Object;

			if (strcmp(camera->m_strName, Name))
			{
				strncpy(camera->m_strName, Name, sizeof(camera->m_strName));
				camera->m_strName[sizeof(camera->m_strName) - 1] = 0;

				gMainWindow->UpdateCameraMenu();

				CheckPointString = "Camera";
			}
		}
	}

	if (CheckPointString)
	{
		CheckPoint(CheckPointString);
		gMainWindow->UpdateFocusObject(GetFocusObject());
		gMainWindow->UpdateAllViews();
	}
}

void Project::ZoomActiveView(int Amount)
{
	float ScaledAmount = 2.0f * Amount / (21 - lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY));

	gMainWindow->GetActiveView()->mCamera->Zoom(ScaledAmount, mCurrentStep, gMainWindow->GetAddKeys());
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

// Indicates if the existing string represents an instance of the candidate
// string in the form "<basename> (#<instance>)".
//
// Returns:
//	-1  if existing is not an instance of candidate.
//	0  if existing is an instance but not numbered.
//	>= 1  indicates the existing instance number.
//
int Project::InstanceOfName(const String& existingString, const String& candidateString, String& baseNameOut)
{
	int einst = 0;
	String estr = existingString;
	estr.TrimLeft();
	estr.TrimRight();

	int div = estr.ReverseFind('#');
	if (-1 != div)
	{
		char* endptr;
		einst = strtol(estr.Mid(div + 1), &endptr, 10);
		if (!*endptr)
		{
			estr = estr.Left(div);
			estr.TrimRight();
		}
	}

	String cstr = candidateString;
	cstr.TrimLeft();
	cstr.TrimRight();

	div = cstr.ReverseFind('#');
	if (-1 != div)
	{
		char* endptr;
        int Value = strtol(cstr.Mid(div + 1), &endptr, 10);
        (void)Value;
		if (!*endptr)
		{
			cstr = cstr.Left(div);
			cstr.TrimRight();
		}
	}

	if (estr.CompareNoCase(cstr))
		return -1;

	baseNameOut = estr;
	return einst;
}
