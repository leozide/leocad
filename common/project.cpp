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
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "preview.h"

Project::Project()
{
	LoadDefaults();

	CheckPoint("");
	mSavedHistory = mUndoHistory[0];
}

Project::~Project()
{
	DeleteModel();
	DeleteHistory();
}

bool Project::Load(const QString& FileName)
{
	lcDiskFile file;
	bool bSuccess = false;

	if (!file.Open(FileName.toLatin1().constData(), "rb")) // todo: qstring
		return false;

	QString Extension = QFileInfo(FileName).suffix().toLower();

	// todo: detect using file contents instead
	bool datfile = (Extension == QLatin1String("dat") || Extension == QLatin1String("ldr"));
	bool mpdfile = (Extension == QLatin1String("mpd"));

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

			const lcArray<View*>& Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
				Views[ViewIdx]->ZoomExtents();

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

	CheckPoint("");
	mSavedHistory = mUndoHistory[0];
	mFileName = FileName;

	return bSuccess;
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

void Project::LoadDefaults() // todo: Change the interface in SetProject() instead
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

			if (!bUndo)
				view->ZoomExtents();
		}
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

void Project::SetSaved(const QString& FileName)
{
	mSavedHistory = mUndoHistory[0];
	mFileName = FileName;
}

QString Project::GetTitle() const
{
	return mFileName.isEmpty() ? tr("New Project.ldr") : QFileInfo(mFileName).fileName();
}

void Project::CheckPoint(const char* Description)
{
	SaveCheckpoint(Description);
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
		View.OnDraw();

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
