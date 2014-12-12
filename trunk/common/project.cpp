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
#include "lc_qmodellistdialog.h"

Project::Project()
{
	mModified = false;
	mActiveModel = new lcModel(tr("Model #1"));
	mModels.Add(mActiveModel);
}

Project::~Project()
{
	mModels.DeleteAll();
}

bool Project::IsModified() const
{
	if (mModified)
		return true;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		if (mModels[ModelIdx]->IsModified())
			return true;

	return false;
}

QString Project::GetTitle() const
{
	return mFileName.isEmpty() ? tr("New Project.ldr") : QFileInfo(mFileName).fileName();
}

void Project::SetActiveModel(int ModelIndex)
{
	if (ModelIndex < 0 || ModelIndex >= mModels.GetSize())
		return;

	mActiveModel = mModels[ModelIndex];
	mActiveModel->UpdateInterface();
	gMainWindow->UpdateModels();

	const lcArray<View*>& Views = gMainWindow->GetViews();
	for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		Views[ViewIdx]->SetModel(lcGetActiveModel());
}

bool Project::IsModelNameValid(const QString& Name) const
{
	if (Name.isEmpty())
		return false;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		if (mModels[ModelIdx]->GetProperties().mName == Name)
			return false;

	return true;
}

void Project::CreateNewModel()
{
	const QString Prefix = tr("Model #");
	int Max = 0;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		QString Name = mModels[ModelIdx]->GetProperties().mName;

		if (Name.startsWith(Prefix))
		{
			QString NumberString = Name.mid(Prefix.length());
			QTextStream Stream(&NumberString);
			int Number;
			Stream >> Number;
			Max = qMax(Max, Number);
		}
	}

	QString Name = Prefix + QString::number(Max + 1);

	for (;;)
	{
		bool Ok = false;

		Name = QInputDialog::getText(gMainWindow->mHandle, tr("New Model"), tr("Name:"), QLineEdit::Normal, Name, &Ok);

		if (!Ok)
			return;

		if (IsModelNameValid(Name))
			break;

		if (Name.isEmpty())
			QMessageBox::information(gMainWindow->mHandle, tr("Empty Name"), tr("The model name cannot be empty."));
		else
			QMessageBox::information(gMainWindow->mHandle, tr("Duplicate Model"), tr("A model named '%1' already exists in this project, please enter an unique name.").arg(Name));
	}

	if (!Name.isEmpty())
	{
		mModified = true;
		mModels.Add(new lcModel(Name));
		SetActiveModel(mModels.GetSize() - 1);
		gMainWindow->UpdateTitle();
	}
}

void Project::ShowModelListDialog()
{
	QList<QPair<QString, lcModel*>> Models;
	Models.reserve(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];
		Models.append(QPair<QString, lcModel*>(Model->GetProperties().mName, Model));
	}

	lcQModelListDialog Dialog(gMainWindow->mHandle, Models);

	if (Dialog.exec() != QDialog::Accepted || Models.isEmpty())
		return;

	lcArray<lcModel*> NewModels;

	for (QList<QPair<QString, lcModel*>>::iterator it = Models.begin(); it != Models.end(); it++)
	{
		lcModel* Model = it->second;

		if (!Model)
		{
			Model = new lcModel(it->first);
			mModified = true;
		}
		else if (Model->GetProperties().mName != it->first)
		{
			Model->SetName(it->first);
			mModified = true;
		}

		NewModels.Add(Model);
	}

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];

		if (NewModels.FindIndex(Model) == -1)
		{
			delete Model;
			mModified = true;
		}
	}

	mModels = NewModels;

	SetActiveModel(Dialog.mActiveModel);
	gMainWindow->UpdateTitle();
}

bool Project::Load(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	mModels.DeleteAll();
	QString Extension = QFileInfo(FileName).suffix().toLower();

	if (Extension == QLatin1String("dat") || Extension == QLatin1String("ldr") || Extension == QLatin1String("mpd"))
	{
		QTextStream Stream(&File);

		while (!Stream.atEnd())
		{
			QString Name = tr("Model%1").arg(QString::number(mModels.GetSize() + 1));
			lcModel* Model = new lcModel(Name);
			mModels.Add(Model);
			Model->LoadLDraw(Stream);
		}
	}
	else
	{
		lcMemFile MemFile;
		QByteArray FileData = File.readAll();
		MemFile.WriteBuffer(FileData.constData(), FileData.size());
		MemFile.Seek(0, SEEK_SET);

		lcModel* Model = new lcModel(tr("Model1"));

		if (Model->LoadBinary(&MemFile))
			mModels.Add(Model);
		else
			delete Model;
	}

	// todo: validate model names

	if (mModels.IsEmpty())
		return false;

	mActiveModel = mModels[0];


/*
		if (datfile || mpdfile)
		{
			gMainWindow->UpdateCurrentStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();

			const lcArray<View*>& Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
				Views[ViewIdx]->ZoomExtents();

			Success = true;
		}
		else
		{
			Success = FileLoad(&file, false, false);

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
		}
*/

	mFileName = FileName;
	mModified = false;

	return true;
}

bool Project::Save(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow->mHandle, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	QTextStream Stream(&File);
	bool MPD = mModels.GetSize() > 1;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];

		if (MPD)
			Stream << QLatin1String("0 FILE ") << Model->GetProperties().mName << QLatin1String("\r\n");

		Model->SaveLDraw(Stream);
		Model->SetSaved();

		if (MPD)
			Stream << QLatin1String("0 ENDFILE\r\n");
	}

	mFileName = FileName;
	mModified = false;

	return true;
}

/*
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
*/

void Project::SaveImage()
{
	/*
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
	*/
}

void Project::SaveStepImages(const QString& BaseName, int Width, int Height, lcStep Start, lcStep End)
{
	/*
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
	*/
}
/*
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
*/
