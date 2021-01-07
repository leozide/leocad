#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <locale.h>
#include "pieceinf.h"
#include "camera.h"
#include "project.h"
#include "lc_instructions.h"
#include "image.h"
#include "lc_mainwindow.h"
#include "lc_view.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_file.h"
#include "lc_zipfile.h"
#include "lc_qimagedialog.h"
#include "lc_qmodellistdialog.h"

lcHTMLExportOptions::lcHTMLExportOptions(const Project* Project)
{
	QString FileName = Project->GetFileName();

	if (!FileName.isEmpty())
		PathName = QFileInfo(FileName).canonicalPath();

	int ImageOptions = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS);
	int HTMLOptions = lcGetProfileInt(LC_PROFILE_HTML_OPTIONS);

	TransparentImages = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
	SubModels = (HTMLOptions & (LC_HTML_SUBMODELS)) != 0;
	CurrentOnly = (HTMLOptions & LC_HTML_CURRENT_ONLY) != 0;
	SinglePage = (HTMLOptions & LC_HTML_SINGLEPAGE) != 0;
	IndexPage = (HTMLOptions & LC_HTML_INDEX) != 0;
	StepImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH);
	StepImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT);
	PartsListStep = (HTMLOptions & LC_HTML_LISTSTEP) != 0;
	PartsListEnd = (HTMLOptions & LC_HTML_LISTEND) != 0;
}

void lcHTMLExportOptions::SaveDefaults()
{
	int HTMLOptions = 0;

	if (SubModels)
		HTMLOptions |= LC_HTML_SUBMODELS;
	if (CurrentOnly)
		HTMLOptions |= LC_HTML_CURRENT_ONLY;
	if (SinglePage)
		HTMLOptions |= LC_HTML_SINGLEPAGE;
	if (IndexPage)
		HTMLOptions |= LC_HTML_INDEX;
	if (PartsListStep)
		HTMLOptions |= LC_HTML_LISTSTEP;
	if (PartsListEnd)
		HTMLOptions |= LC_HTML_LISTEND;

	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, TransparentImages ? LC_IMAGE_TRANSPARENT : 0);
	lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, StepImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, StepImagesHeight);
}

Project::Project(bool IsPreview)
	: mIsPreview(IsPreview)
{
	mModified = false;
	mActiveModel = new lcModel(tr(mIsPreview ? "Preview.ldr" : "New Model.ldr"), this, mIsPreview);
	mActiveModel->CreatePieceInfo(this);
	mActiveModel->SetSaved();
	mModels.Add(mActiveModel);

	if (!mIsPreview && gMainWindow)
		QObject::connect(&mFileWatcher, SIGNAL(fileChanged(const QString&)), gMainWindow, SLOT(ProjectFileChanged(const QString&)));
}

Project::~Project()
{
	mModels.DeleteAll();
}

lcModel* Project::GetModel(const QString& FileName) const
{
	for (lcModel* Model : mModels)
		if (Model->GetProperties().mFileName == FileName)
			return Model;

	return nullptr;
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
	if (!mFileName.isEmpty())
		return QFileInfo(mFileName).fileName();

	return mModels.GetSize() == 1 ? tr("New Model.ldr") : tr("New Model.mpd");
}

QString Project::GetImageFileName(bool AllowCurrentFolder) const
{
	QString FileName = GetFileName();

	if (!FileName.isEmpty())
	{
		QString Extension = QFileInfo(FileName).suffix();
		if (!Extension.isEmpty())
			FileName = FileName.left(FileName.length() - Extension.length() - 1);
	}
	else
	{
		if (AllowCurrentFolder)
			FileName = QLatin1String("image");
		else
		{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
			QStringList cachePathList = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
			FileName = cachePathList.first();
#else
			FileName = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
			FileName = QDir(FileName).absoluteFilePath(QLatin1String("image"));
		}
	}

	return QDir::toNativeSeparators(FileName) + lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
}

void Project::SetActiveModel(int ModelIndex)
{
	if (ModelIndex < 0 || ModelIndex >= mModels.GetSize())
		return;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->SetActive(ModelIdx == ModelIndex);

	std::vector<lcModel*> UpdatedModels;
	UpdatedModels.reserve(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->UpdatePieceInfo(UpdatedModels);

	mActiveModel = mModels[ModelIndex];

	if (!mIsPreview && gMainWindow)
	{
		gMainWindow->SetCurrentModelTab(mActiveModel);
		mActiveModel->UpdateInterface();
	}
}

void Project::SetActiveModel(const QString& FileName)
{
	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		if (FileName.compare(mModels[ModelIdx]->GetFileName(), Qt::CaseInsensitive) == 0)
		{
			SetActiveModel(ModelIdx);
			return;
		}
	}
}

QString Project::GetNewModelName(QWidget* ParentWidget, const QString& DialogTitle, const QString& CurrentName, const QStringList& ExistingModels) const
{
	QString Name = CurrentName;

	if (Name.isEmpty())
	{
		const QString Prefix = tr("Submodel #");
		int Max = 0;

		for (int ModelIdx = 0; ModelIdx < ExistingModels.size(); ModelIdx++)
		{
			const QString& ExistingName = ExistingModels[ModelIdx];

			if (ExistingName.startsWith(Prefix))
			{
				QString NumberString = ExistingName.mid(Prefix.length());
				QTextStream Stream(&NumberString);
				int Number;
				Stream >> Number;
				Max = qMax(Max, Number);
			}
		}

		Name = Prefix + QString::number(Max + 1) + ".ldr";
	}

	for (;;)
	{
		bool Ok = false;

		Name = QInputDialog::getText(ParentWidget, DialogTitle, tr("Submodel Name:"), QLineEdit::Normal, Name, &Ok);

		if (!Ok)
			return QString();

		if (Name.isEmpty())
		{
			QMessageBox::information(ParentWidget, tr("Empty Name"), tr("The submodel name cannot be empty."));
			continue;
		}

		bool ExtensionValid = false;

		if (Name.length() < 5)
			ExtensionValid = false;
		else
		{
			QString Extension = Name.right(4);
			if (Extension.compare(".dat", Qt::CaseInsensitive) == 0 || Extension.compare(".ldr", Qt::CaseInsensitive) == 0 || Extension.compare(".mpd", Qt::CaseInsensitive) == 0)
				ExtensionValid = true;
		}

		if (!ExtensionValid)
			Name += ".ldr";

		if (ExistingModels.contains(Name, Qt::CaseInsensitive) && Name != CurrentName)
		{
			QMessageBox::information(ParentWidget, tr("Duplicate Submodel"), tr("A submodel named '%1' already exists, please enter a unique name.").arg(Name));
			continue;
		}

		break;
	}

	return Name;
}

lcModel* Project::CreateNewModel(bool ShowModel)
{
	QStringList ModelNames;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		ModelNames.append(mModels[ModelIdx]->GetProperties().mFileName);

	QString Name = GetNewModelName(gMainWindow, tr("New Submodel"), QString(), ModelNames);

	if (Name.isEmpty())
		return nullptr;

	mModified = true;
	lcModel* Model = new lcModel(Name, this, false);
	Model->CreatePieceInfo(this);
	Model->SetSaved();
	mModels.Add(Model);

	if (ShowModel)
	{
		SetActiveModel(mModels.GetSize() - 1);
		gMainWindow->UpdateTitle();
	}
	else
		SetActiveModel(mModels.FindIndex(mActiveModel));

	return Model;
}

void Project::ShowModelListDialog()
{
	lcQModelListDialog Dialog(gMainWindow, mModels);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	lcArray<lcModel*> NewModels;
	std::vector<lcModelListDialogEntry> Results = Dialog.GetResults();

	for (const lcModelListDialogEntry& Entry : Results)
	{
		lcModel* Model = Entry.ExistingModel;

		if (!Model)
		{
			const lcModel* Source = Entry.DuplicateSource;

			if (!Source)
			{
				Model = new lcModel(Entry.Name, this, false);
			}
			else
			{
				Model = new lcModel(Source->GetProperties().mFileName, this, false);

				QByteArray File;

				QTextStream SaveStream(&File);
				Source->SaveLDraw(SaveStream, false);
				SaveStream.flush();

				QBuffer Buffer(&File);
				Buffer.open(QIODevice::ReadOnly);

				Model->LoadLDraw(Buffer, this);
				Model->SetFileName(Entry.Name);
				Model->CreatePieceInfo(this);
			}

			Model->CreatePieceInfo(this);
			Model->SetSaved();

			mModified = true;
		}
		else if (Model->GetProperties().mFileName != Entry.Name)
		{
			Model->SetFileName(Entry.Name);
			lcGetPiecesLibrary()->RenamePiece(Model->GetPieceInfo(), Entry.Name.toLatin1().constData());

			for (lcModel* CheckModel : mModels)
				CheckModel->RenamePiece(Model->GetPieceInfo());

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

	gMainWindow->UpdateTitle();
	gMainWindow->UpdateModels();

	int ModelIndex = Dialog.GetActiveModelIndex();
	if (ModelIndex != -1)
		SetActiveModel(ModelIndex);
}

void Project::SetFileName(const QString& FileName)
{
	if (mFileName == FileName)
		return;

	if (!mIsPreview && !mFileName.isEmpty())
		mFileWatcher.removePath(mFileName);

	if (!mIsPreview && !FileName.isEmpty())
		mFileWatcher.addPath(FileName);

	mFileName = FileName;
}

bool Project::Load(const QString& FileName)
{
	QWidget *parent = nullptr;
	if (!mIsPreview)
		parent = gMainWindow;

	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(parent, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	mModels.DeleteAll();
	SetFileName(FileName);
	QFileInfo FileInfo(FileName);
	QString Extension = FileInfo.suffix().toLower();

	QByteArray FileData = File.readAll();
	bool LoadDAT;

	if (Extension == QLatin1String("dat") || Extension == QLatin1String("ldr") || Extension == QLatin1String("mpd"))
		LoadDAT = true;
	else if (Extension == QLatin1String("lcd") || Extension == QLatin1String("leocad"))
		LoadDAT = false;
	else
		LoadDAT = FileData.size() > 7 && memcmp(FileData, "LeoCAD ", 7);

	if (LoadDAT)
	{
		QBuffer Buffer(&FileData);
		Buffer.open(QIODevice::ReadOnly);
		std::vector<std::pair<int, lcModel*>> Models;

		while (!Buffer.atEnd())
		{
			lcModel* Model = new lcModel(QString(), this, mIsPreview);
			int Pos = Model->SplitMPD(Buffer);

			if (Models.empty() || !Model->GetFileName().isEmpty())
			{
				auto ModelCompare = [Model](const std::pair<int, lcModel*>& ModelIt)
				{
					return ModelIt.second->GetFileName() == Model->GetFileName();
				};

				if (std::find_if(Models.begin(), Models.end(), ModelCompare) == Models.end())
				{
					mModels.Add(Model);
					Models.emplace_back(std::make_pair(Pos, Model));
					Model->CreatePieceInfo(this);
				}
				else
					delete Model;
			}
			else
				delete Model;
		}

		for (size_t ModelIdx = 0; ModelIdx < Models.size(); ModelIdx++)
		{
			Buffer.seek(Models[ModelIdx].first);
			lcModel* Model = Models[ModelIdx].second;
			Model->LoadLDraw(Buffer, this);
			Model->SetSaved();
		}
	}
	else
	{
		lcMemFile MemFile;
		MemFile.WriteBuffer(FileData.constData(), FileData.size());
		MemFile.Seek(0, SEEK_SET);

		lcModel* Model = new lcModel(QString(), this, mIsPreview);

		if (Model->LoadBinary(&MemFile))
		{
			mModels.Add(Model);
			Model->CreatePieceInfo(this);
			Model->SetSaved();
		}
		else
			delete Model;
	}

	if (mModels.IsEmpty())
	{
		QMessageBox::warning(parent, tr("Error"), tr("Error loading file '%1':\nFile format is not recognized.").arg(FileName));
		return false;
	}

	if (mModels.GetSize() == 1)
	{
		lcModel* Model = mModels[0];

		if (Model->GetProperties().mFileName.isEmpty())
		{
			Model->SetFileName(FileInfo.fileName());
			lcGetPiecesLibrary()->RenamePiece(Model->GetPieceInfo(), FileInfo.fileName().toLatin1());
		}
	}

	std::vector<lcModel*> UpdatedModels;
	UpdatedModels.reserve(mModels.GetSize());

	for (lcModel* Model : mModels)
	{
		Model->UpdateMesh();
		Model->UpdatePieceInfo(UpdatedModels);
	}

	mModified = false;

	return true;
}

bool Project::Save(const QString& FileName)
{
	SetFileName(QString());

	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	QTextStream Stream(&File);
	bool Success = Save(Stream);
	File.close();

	if (Success)
	{
		SetFileName(FileName);
		mModified = false;
	}

	return Success;
}

bool Project::Save(QTextStream& Stream)
{
	bool MPD = mModels.GetSize() > 1;

	for (lcModel* Model : mModels)
	{
		if (MPD)
			Stream << QLatin1String("0 FILE ") << Model->GetProperties().mFileName << QLatin1String("\r\n");

		Model->SaveLDraw(Stream, false);
		Model->SetSaved();

		if (MPD)
			Stream << QLatin1String("0 NOFILE\r\n");
	}

	return true;
}

void Project::Merge(Project* Other)
{
	for (lcModel* Model : Other->mModels)
	{
		QString FileName = Model->GetProperties().mFileName;

		for (;;)
		{
			bool Duplicate = false;

			for (int SearchIdx = 0; SearchIdx < mModels.GetSize(); SearchIdx++)
			{
				if (mModels[SearchIdx]->GetProperties().mFileName == FileName)
				{
					Duplicate = true;
					break;
				}
			}

			if (!Duplicate)
				break;

			FileName = tr("Merged ") + FileName;
			Model->SetFileName(FileName);
		}

		mModels.Add(Model);
	}

	Other->mModels.RemoveAll();
	mModified = true;
}

bool Project::ImportLDD(const QString& FileName)
{
	lcZipFile ZipFile;

	if (!ZipFile.OpenRead(FileName))
		return false;

	lcMemFile XMLFile;
	if (!ZipFile.ExtractFile("IMAGE100.LXFML", XMLFile))
		return false;

	mModels.DeleteAll();
	QString ModelName = QFileInfo(FileName).completeBaseName();
	lcModel* Model = new lcModel(ModelName, this, false);

	if (Model->LoadLDD(QString::fromUtf8((const char*)XMLFile.mBuffer)))
	{
		mModels.Add(Model);
		Model->SetSaved();
	}
	else
	{
		delete Model;
		return false;
	}

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CreatePieceInfo(this);

	std::vector<lcModel*> UpdatedModels;
	UpdatedModels.reserve(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->UpdatePieceInfo(UpdatedModels);

	mModified = false;

	return true;
}

bool Project::ImportInventory(const QByteArray& Inventory, const QString& Name, const QString& Description)
{
	if (Inventory.isEmpty())
		return false;

	mModels.DeleteAll();
	lcModel* Model = new lcModel(Name, this, false);

	if (Model->LoadInventory(Inventory))
	{
		mModels.Add(Model);
		Model->SetSaved();
	}
	else
	{
		delete Model;
		return false;
	}

	Model->SetDescription(Description);

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CreatePieceInfo(this);

	std::vector<lcModel*> UpdatedModels;
	UpdatedModels.reserve(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->UpdatePieceInfo(UpdatedModels);

	mModified = false;

	return true;
}

std::vector<lcModelPartsEntry> Project::GetModelParts()
{
	std::vector<lcModelPartsEntry> ModelParts;

	if (mModels.IsEmpty())
		return ModelParts;

	for (lcModel* Model : mModels)
		Model->CalculateStep(LC_STEP_MAX);

	mModels[0]->GetModelParts(lcMatrix44Identity(), gDefaultColor, ModelParts);

	SetActiveModel(mModels.FindIndex(mActiveModel));

	return ModelParts;
}

bool Project::ExportModel(const QString& FileName, lcModel* Model) const
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	QTextStream Stream(&File);

	Model->SaveLDraw(Stream, false);

	return true;
}

QString Project::GetExportFileName(const QString& FileName, const QString& DefaultExtension, const QString& DialogTitle, const QString& DialogFilter) const
{
	if (!FileName.isEmpty())
		return FileName;

	QString SaveFileName;

	if (!mFileName.isEmpty())
		SaveFileName = mFileName;
	else
		SaveFileName = GetTitle();

	QString Extension = QFileInfo(SaveFileName).suffix().toLower();

	if (Extension.isEmpty())
		SaveFileName += "." + DefaultExtension;
	else if (Extension != DefaultExtension && SaveFileName.length() > 4)
	{
		SaveFileName = SaveFileName.left(SaveFileName.length() - Extension.length() - 1);
		SaveFileName += "." + DefaultExtension;
	}

	return QFileDialog::getSaveFileName(gMainWindow, DialogTitle, SaveFileName, DialogFilter);
}

void Project::Export3DStudio(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(FileName, "3ds", tr("Export 3D Studio"), tr("3DS Files (*.3ds);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile File(SaveFileName);

	if (!File.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	long M3DStart = File.GetPosition();
	File.WriteU16(0x4D4D); // CHK_M3DMAGIC
	File.WriteU32(0);

	File.WriteU16(0x0002); // CHK_M3D_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	long MDataStart = File.GetPosition();
	File.WriteU16(0x3D3D); // CHK_MDATA
	File.WriteU32(0);

	File.WriteU16(0x3D3E); // CHK_MESH_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	const int MaterialNameLength = 11;
	char MaterialName[32];

	for (size_t ColorIdx = 0; ColorIdx < gColorList.size(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		sprintf(MaterialName, "Material%03d", (int)ColorIdx);

		long MaterialStart = File.GetPosition();
		File.WriteU16(0xAFFF); // CHK_MAT_ENTRY
		File.WriteU32(0);

		File.WriteU16(0xA000); // CHK_MAT_NAME
		File.WriteU32(6 + MaterialNameLength + 1);
		File.WriteBuffer(MaterialName, MaterialNameLength + 1);

		File.WriteU16(0xA010); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA020); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA030); // CHK_MAT_SPECULAR
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0xA040); // CHK_MAT_SHININESS
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * 0.25 + 0.5));

		File.WriteU16(0xA041); // CHK_MAT_SHIN2PCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * 0.05 + 0.5));

		File.WriteU16(0xA050); // CHK_MAT_TRANSPARENCY
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * (1.0f - Color->Value[3]) + 0.5));

		File.WriteU16(0xA052); // CHK_MAT_XPFALL
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA053); // CHK_MAT_REFBLUR
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * 0.2 + 0.5));

		File.WriteU16(0xA100); // CHK_MAT_SHADING
		File.WriteU32(8);

		File.WriteS16(3);

		File.WriteU16(0xA084); // CHK_MAT_SELF_ILPCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((quint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA081); // CHK_MAT_TWO_SIDE
		File.WriteU32(6);

		File.WriteU16(0xA087); // CHK_MAT_WIRE_SIZE
		File.WriteU32(10);

		File.WriteFloat(1.0f);

		long MaterialEnd = File.GetPosition();
		File.Seek(MaterialStart + 2, SEEK_SET);
		File.WriteU32(MaterialEnd - MaterialStart);
		File.Seek(MaterialEnd, SEEK_SET);
	}

	const lcModelProperties& Properties = mModels[0]->GetProperties();

	File.WriteU16(0x0100); // CHK_MASTER_SCALE
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1400); // CHK_LO_SHADOW_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1420); // CHK_SHADOW_MAP_SIZE
	File.WriteU32(8);

	File.WriteS16(512);

	File.WriteU16(0x1450); // CHK_SHADOW_FILTER
	File.WriteU32(10);

	File.WriteFloat(3.0f);

	File.WriteU16(0x1460); // CHK_RAY_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1500); // CHK_O_CONSTS
	File.WriteU32(18);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);

	File.WriteU16(0x2100); // CHK_AMBIENT_LIGHT
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mAmbientColor, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mAmbientColor, 3);

	File.WriteU16(0x1200); // CHK_SOLID_BGND
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	const lcPreferences& Preferences = lcGetPreferences();
	lcVector3 BackgroundSolidColor = lcVector3FromColor(Preferences.mBackgroundSolidColor);

	File.WriteFloats(BackgroundSolidColor, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(BackgroundSolidColor, 3);

	File.WriteU16(0x1300); // CHK_V_GRADIENT
	File.WriteU32(118);

	File.WriteFloat(1.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	const lcVector3 BackgroundGradientColor1 = lcVector3FromColor(Preferences.mBackgroundGradientColorTop);
	const lcVector3 BackgroundGradientColor2 = lcVector3FromColor(Preferences.mBackgroundGradientColorBottom);

	File.WriteFloats(BackgroundGradientColor1, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(BackgroundGradientColor1, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((BackgroundGradientColor1 + BackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((BackgroundGradientColor1 + BackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(BackgroundGradientColor2, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(BackgroundGradientColor2, 3);

	if (Preferences.mBackgroundGradient)
	{
		File.WriteU16(0x1301); // LIB3DS_USE_V_GRADIENT
		File.WriteU32(6);
	}
	else
	{
		File.WriteU16(0x1201); // LIB3DS_USE_SOLID_BGND
		File.WriteU32(6);
	}

	int NumPieces = 0;
	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!Mesh || Mesh->mIndexType == GL_UNSIGNED_INT)
			continue;

		long NamedObjectStart = File.GetPosition();
		File.WriteU16(0x4000); // CHK_NAMED_OBJECT
		File.WriteU32(0);

		char Name[32];
		sprintf(Name, "Piece%.3d", NumPieces);
		NumPieces++;
		File.WriteBuffer(Name, strlen(Name) + 1);

		long TriObjectStart = File.GetPosition();
		File.WriteU16(0x4100); // CHK_N_TRI_OBJECT
		File.WriteU32(0);

		File.WriteU16(0x4110); // CHK_POINT_ARRAY
		File.WriteU32(8 + 12 * Mesh->mNumVertices);

		File.WriteU16(Mesh->mNumVertices);

		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;
		const lcMatrix44& ModelWorld = ModelPart.WorldMatrix;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Pos = lcMul31(Verts[VertexIdx].Position, ModelWorld);
			File.WriteFloat(Pos[0]);
			File.WriteFloat(Pos[1]);
			File.WriteFloat(Pos[2]);
		}

		File.WriteU16(0x4160); // CHK_MESH_MATRIX
		File.WriteU32(54);

		lcMatrix44 Matrix = lcMatrix44Identity();
		File.WriteFloats(Matrix[0], 3);
		File.WriteFloats(Matrix[1], 3);
		File.WriteFloats(Matrix[2], 3);
		File.WriteFloats(Matrix[3], 3);

		File.WriteU16(0x4165); // CHK_MESH_COLOR
		File.WriteU32(7);

		File.WriteU8(0);

		long FaceArrayStart = File.GetPosition();
		File.WriteU16(0x4120); // CHK_FACE_ARRAY
		File.WriteU32(0);

		int NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			NumTriangles += Section->NumIndices / 3;
		}

		File.WriteU16(NumTriangles);

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			quint16* Indices = (quint16*)Mesh->mIndexData + Section->IndexOffset / sizeof(quint16);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
			{
				File.WriteU16(Indices[IndexIdx + 0]);
				File.WriteU16(Indices[IndexIdx + 1]);
				File.WriteU16(Indices[IndexIdx + 2]);
				File.WriteU16(7);
			}
		}

		NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			int MaterialIndex = Section->ColorIndex == gDefaultColor ? ModelPart.ColorIndex : Section->ColorIndex;

			File.WriteU16(0x4130); // CHK_MSH_MAT_GROUP
			File.WriteU32(6 + MaterialNameLength + 1 + 2 + 2 * Section->NumIndices / 3);

			sprintf(MaterialName, "Material%03d", MaterialIndex);
			File.WriteBuffer(MaterialName, MaterialNameLength + 1);

			File.WriteU16(Section->NumIndices / 3);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
				File.WriteU16(NumTriangles++);
		}

		long FaceArrayEnd = File.GetPosition();
		File.Seek(FaceArrayStart + 2, SEEK_SET);
		File.WriteU32(FaceArrayEnd - FaceArrayStart);
		File.Seek(FaceArrayEnd, SEEK_SET);

		long TriObjectEnd = File.GetPosition();
		File.Seek(TriObjectStart + 2, SEEK_SET);
		File.WriteU32(TriObjectEnd - TriObjectStart);
		File.Seek(TriObjectEnd, SEEK_SET);

		long NamedObjectEnd = File.GetPosition();
		File.Seek(NamedObjectStart + 2, SEEK_SET);
		File.WriteU32(NamedObjectEnd - NamedObjectStart);
		File.Seek(NamedObjectEnd, SEEK_SET);
	}

	long MDataEnd = File.GetPosition();
	File.Seek(MDataStart + 2, SEEK_SET);
	File.WriteU32(MDataEnd - MDataStart);
	File.Seek(MDataEnd, SEEK_SET);

	long KFDataStart = File.GetPosition();
	File.WriteU16(0xB000); // CHK_KFDATA
	File.WriteU32(0);

	File.WriteU16(0xB00A); // LIB3DS_KFHDR
	File.WriteU32(6 + 2 + 1 + 4);

	File.WriteS16(5);
	File.WriteU8(0);
	File.WriteS32(100);

	long KFDataEnd = File.GetPosition();
	File.Seek(KFDataStart + 2, SEEK_SET);
	File.WriteU32(KFDataEnd - KFDataStart);
	File.Seek(KFDataEnd, SEEK_SET);

	long M3DEnd = File.GetPosition();
	File.Seek(M3DStart + 2, SEEK_SET);
	File.WriteU32(M3DEnd - M3DStart);
	File.Seek(M3DEnd, SEEK_SET);
}

void Project::ExportBrickLink()
{
	lcPartsList PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, true, false, PartsList);

	if (PartsList.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(QString(), "xml", tr("Export BrickLink"), tr("XML Files (*.xml);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile BrickLinkFile(SaveFileName);
	char Line[1024];

	if (!BrickLinkFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	BrickLinkFile.WriteLine("<INVENTORY>\n");

	for (const auto& PartIt : PartsList)
	{
		const PieceInfo* Info = PartIt.first;

		for (const auto& ColorIt : PartIt.second)
		{
			BrickLinkFile.WriteLine("  <ITEM>\n");
			BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");

			char FileName[LC_PIECE_NAME_LEN];
			strcpy(FileName, Info->mFileName);
			char* Ext = strchr(FileName, '.');
			if (Ext)
				*Ext = 0;

			sprintf(Line, "    <ITEMID>%s</ITEMID>\n", FileName);
			BrickLinkFile.WriteLine(Line);

			sprintf(Line, "    <MINQTY>%d</MINQTY>\n", ColorIt.second);
			BrickLinkFile.WriteLine(Line);

			int Color = lcGetBrickLinkColor(ColorIt.first);
			if (Color)
			{
				sprintf(Line, "    <COLOR>%d</COLOR>\n", Color);
				BrickLinkFile.WriteLine(Line);
			}

			BrickLinkFile.WriteLine("  </ITEM>\n");
		}
	}

	BrickLinkFile.WriteLine("</INVENTORY>\n");
}

void Project::ExportCOLLADA(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(FileName, "dae", tr("Export COLLADA"), tr("COLLADA Files (*.dae);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	QFile File(SaveFileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	QTextStream Stream(&File);

	Stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n";
	Stream << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\r\n";

	Stream << "<asset>\r\n";
	Stream << "\t<created>" << QDateTime::currentDateTime().toString(Qt::ISODate) << "</created>\r\n";
	Stream << "\t<modified>" << QDateTime::currentDateTime().toString(Qt::ISODate) << "</modified>\r\n";
	Stream << "<unit name=\"LeoCAD\" meter=\"0.0004\" />\r\n";
	Stream << "\t<up_axis>Z_UP</up_axis>\r\n";
	Stream << "</asset>\r\n";

	Stream << "<library_effects>\r\n";

	for (const lcColor& Color : gColorList)
	{
		const char* ColorName = Color.SafeName;

		Stream << QString("\t<effect id=\"%1-phong\">\r\n").arg(ColorName);
		Stream << "\t\t<profile_COMMON>\r\n";
		Stream << "\t\t\t<technique sid=\"phong1\">\r\n";
		Stream << "\t\t\t\t<phong>\r\n";
		Stream << "\t\t\t\t\t<emission>\r\n";
		Stream << "\t\t\t\t\t\t<color>0.0 0.0 0.0 0.0</color>\r\n";
		Stream << "\t\t\t\t\t</emission>\r\n";
		Stream << "\t\t\t\t\t<ambient>\r\n";
		Stream << QString("\t\t\t\t\t\t<color>%1 %2 %3 1.0</color>\r\n").arg(QString::number(Color.Value[0]), QString::number(Color.Value[1]), QString::number(Color.Value[2]));
		Stream << "\t\t\t\t\t</ambient>\r\n";
		Stream << "\t\t\t\t\t<diffuse>\r\n";
		Stream << QString("\t\t\t\t\t\t<color>%1 %2 %3 1.0</color>\r\n").arg(QString::number(Color.Value[0]), QString::number(Color.Value[1]), QString::number(Color.Value[2]));
		Stream << "\t\t\t\t\t</diffuse>\r\n";
		Stream << "\t\t\t\t\t<specular>\r\n";
		Stream << "\t\t\t\t\t\t<color>0.9 0.9 0.9 1.0</color>\r\n";
		Stream << "\t\t\t\t\t</specular>\r\n";
		Stream << "\t\t\t\t\t<shininess>\r\n";
		Stream << "\t\t\t\t\t\t<float>20.0</float>\r\n";
		Stream << "\t\t\t\t\t</shininess>\r\n";
		Stream << "\t\t\t\t\t<transparent>\r\n";
		Stream << QString("\t\t\t\t\t\t<color>%1 %2 %3 %4</color>\r\n").arg(QString::number(Color.Value[0]), QString::number(Color.Value[1]), QString::number(Color.Value[2]), QString::number(Color.Value[3]));
		Stream << "\t\t\t\t\t</transparent>\r\n";
		Stream << "\t\t\t\t\t<transparency>\r\n";
		Stream << "\t\t\t\t\t\t<float>1.0</float>\r\n";
		Stream << "\t\t\t\t\t</transparency>\r\n";
		Stream << "\t\t\t\t</phong>\r\n";
		Stream << "\t\t\t</technique>\r\n";
		Stream << "\t\t</profile_COMMON>\r\n";
		Stream << "\t</effect>\r\n";
	}

	Stream << "</library_effects>\r\n";
	Stream << "<library_materials>\r\n";

	for (const lcColor& Color : gColorList)
	{
		const char* ColorName = Color.SafeName;
		Stream << QString("\t<material id=\"%1-material\">\r\n").arg(ColorName);
		Stream << QString("\t\t<instance_effect url=\"#%1-phong\" />\r\n").arg(ColorName);
		Stream << "\t</material>\r\n";
	}

	Stream << "</library_materials>\r\n";
	Stream << "<library_geometries>\r\n";
	std::set<lcMesh*> AddedMeshes;

	auto GetMeshID = [](const lcModelPartsEntry& ModelPart)
	{
		const PieceInfo* Info = ModelPart.Info;
		QString ID = QString(Info->mFileName).replace('.', '_');

		if (ModelPart.Mesh)
			ID += "_" + QString::number((quintptr)ModelPart.Mesh, 16);

		return ID;
	};

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!AddedMeshes.insert(Mesh).second)
			continue;

		QString ID = GetMeshID(ModelPart);

		if (!Mesh)
			continue;

		Stream << QString("\t<geometry id=\"%1\">\r\n").arg(ID);
		Stream << "\t\t<mesh>\r\n";

		Stream << QString("\t\t\t<source id=\"%1-pos\">\r\n").arg(ID);
		Stream << QString("\t\t\t\t<float_array id=\"%1-pos-array\" count=\"%2\">\r\n").arg(ID, QString::number(Mesh->mNumVertices));

		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3& Position = Verts[VertexIdx].Position;
			Stream << QString("\t\t\t\t\t%1 %2 %3\r\n").arg(QString::number(Position.x), QString::number(Position.y), QString::number(Position.z));
		}

		Stream << "\t\t\t\t</float_array>\r\n";
		Stream << "\t\t\t\t<technique_common>\r\n";
		Stream << QString("\t\t\t\t\t<accessor source=\"#%1-pos-array\" count=\"%2\" stride=\"3\">\r\n").arg(ID, QString::number(Mesh->mNumVertices));
		Stream << "\t\t\t\t\t\t<param name=\"X\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t</accessor>\r\n";
		Stream << "\t\t\t\t</technique_common>\r\n";
		Stream << "\t\t\t</source>\r\n";

		Stream << QString("\t\t\t<source id=\"%1-normal\">\r\n").arg(ID);
		Stream << QString("\t\t\t\t<float_array id=\"%1-normal-array\" count=\"%2\">\r\n").arg(ID, QString::number(Mesh->mNumVertices));

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Normal = lcUnpackNormal(Verts[VertexIdx].Normal);
			Stream << QString("\t\t\t\t\t%1 %2 %3\r\n").arg(QString::number(Normal.x), QString::number(Normal.y), QString::number(Normal.z));
		}

		Stream << "\t\t\t\t</float_array>\r\n";
		Stream << "\t\t\t\t<technique_common>\r\n";
		Stream << QString("\t\t\t\t\t<accessor source=\"#%1-normal-array\" count=\"%2\" stride=\"3\">\r\n").arg(ID, QString::number(Mesh->mNumVertices));
		Stream << "\t\t\t\t\t\t<param name=\"X\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />\r\n";
		Stream << "\t\t\t\t\t</accessor>\r\n";
		Stream << "\t\t\t\t</technique_common>\r\n";
		Stream << "\t\t\t</source>\r\n";

		Stream << QString("\t\t\t<vertices id=\"%1-vertices\">\r\n").arg(ID);
		Stream << QString("\t\t\t\t<input semantic=\"POSITION\" source=\"#%1-pos\"/>\r\n").arg(ID);
		Stream << "\t\t\t</vertices>\r\n";

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			const char* ColorName = gColorList[Section->ColorIndex].SafeName;

			if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
			{
				quint16* Indices = (quint16*)Mesh->mIndexData + Section->IndexOffset / sizeof(quint16);

				Stream << QString("\t\t\t<triangles count=\"%1\" material=\"%2\">\r\n").arg(QString::number(Section->NumIndices / 3), ColorName);
				Stream << QString("\t\t\t<input semantic=\"VERTEX\" source=\"#%1-vertices\" offset=\"0\" />\r\n").arg(ID);
				Stream << QString("\t\t\t<input semantic=\"NORMAL\" source=\"#%1-normal\" offset=\"0\" />\r\n").arg(ID);
				Stream << "\t\t\t<p>\r\n";

				for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
				{
					QString idx1 = QString::number(Indices[Idx + 0]);
					QString idx2 = QString::number(Indices[Idx + 1]);
					QString idx3 = QString::number(Indices[Idx + 2]);

					Stream << QString("\t\t\t\t %1 %2 %3\r\n").arg(idx1, idx2, idx3);
				}
			}
			else
			{
				quint32* Indices = (quint32*)Mesh->mIndexData + Section->IndexOffset / sizeof(quint32);

				Stream << QString("\t\t\t<triangles count=\"%1\" material=\"%2\">\r\n").arg(QString::number(Section->NumIndices / 3), ColorName);
				Stream << QString("\t\t\t<input semantic=\"VERTEX\" source=\"#%1-vertices\" offset=\"0\" />\r\n").arg(ID);
				Stream << QString("\t\t\t<input semantic=\"NORMAL\" source=\"#%1-normal\" offset=\"0\" />\r\n").arg(ID);
				Stream << "\t\t\t<p>\r\n";

				for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
				{
					QString idx1 = QString::number(Indices[Idx + 0]);
					QString idx2 = QString::number(Indices[Idx + 1]);
					QString idx3 = QString::number(Indices[Idx + 2]);

					Stream << QString("\t\t\t\t %1 %2 %3\r\n").arg(idx1, idx2, idx3);
				}
			}

			Stream << "\t\t\t\t</p>\r\n";
			Stream << "\t\t\t</triangles>\r\n";
		}

		Stream << "\t\t</mesh>\r\n";
		Stream << "\t</geometry>\r\n";
	}

	Stream << "</library_geometries>\r\n";
	Stream << "<library_visual_scenes>\r\n";
	Stream << "\t<visual_scene id=\"DefaultScene\">\r\n";

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!Mesh)
			continue;

		QString ID = GetMeshID(ModelPart);

		Stream << "\t\t<node>\r\n";
		Stream << "\t\t\t<matrix>\r\n";

		const lcMatrix44& Matrix = ModelPart.WorldMatrix;
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][0]), QString::number(Matrix[1][0]), QString::number(Matrix[2][0]), QString::number(Matrix[3][0]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][1]), QString::number(Matrix[1][1]), QString::number(Matrix[2][1]), QString::number(Matrix[3][1]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][2]), QString::number(Matrix[1][2]), QString::number(Matrix[2][2]), QString::number(Matrix[3][2]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][3]), QString::number(Matrix[1][3]), QString::number(Matrix[2][3]), QString::number(Matrix[3][3]));

		Stream << "\t\t\t</matrix>\r\n";
		Stream << QString("\t\t\t<instance_geometry url=\"#%1\">\r\n").arg(ID);
		Stream << "\t\t\t\t<bind_material>\r\n";
		Stream << "\t\t\t\t\t<technique_common>\r\n";

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			const char* SourceColorName = gColorList[Section->ColorIndex].SafeName;
			const char* TargetColorName;
			if (Section->ColorIndex == gDefaultColor)
				TargetColorName = gColorList[ModelPart.ColorIndex].SafeName;
			else
				TargetColorName = gColorList[Section->ColorIndex].SafeName;

			Stream << QString("\t\t\t\t\t\t<instance_material symbol=\"%1\" target=\"#%2-material\"/>\r\n").arg(SourceColorName, TargetColorName);
		}

		Stream << "\t\t\t\t\t</technique_common>\r\n";
		Stream << "\t\t\t\t</bind_material>\r\n";
		Stream << "\t\t\t</instance_geometry>\r\n";
		Stream << "\t\t</node>\r\n";
	}

	Stream << "\t</visual_scene>\r\n";
	Stream << "</library_visual_scenes>\r\n";
	Stream << "<scene>\r\n";
	Stream << "\t<instance_visual_scene url=\"#DefaultScene\"/>\r\n";
	Stream << "</scene>\r\n";

	Stream << "</COLLADA>\r\n";
}

void Project::ExportCSV()
{
	lcPartsList PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, true, false, PartsList);

	if (PartsList.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(QString(), "csv", tr("Export CSV"), tr("CSV Files (*.csv);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile CSVFile(SaveFileName);
	char Line[1024];

	if (!CSVFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

	for (const auto& PartIt : PartsList)
	{
		const PieceInfo* Info = PartIt.first;

		for (const auto& ColorIt : PartIt.second)
		{
			sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", Info->m_strDescription, gColorList[ColorIt.first].Name, ColorIt.second, Info->mFileName, gColorList[ColorIt.first].Code);
			CSVFile.WriteLine(Line);
		}
	}
}

lcInstructions Project::GetInstructions()
{
	return lcInstructions(this);
}

void Project::ExportHTML(const lcHTMLExportOptions& Options)
{
	QDir Dir(Options.PathName);
	Dir.mkpath(QLatin1String("."));

	lcArray<lcModel*> Models;

	if (Options.CurrentOnly)
		Models.Add(mActiveModel);
	else if (Options.SubModels)
	{
		Models.Add(mActiveModel);
		mActiveModel->GetSubModels(Models);
	}
	else
		Models = mModels;

	QString ProjectTitle = GetTitle();

	auto AddPartsListImage = [&Dir](QTextStream& Stream, lcModel* Model, lcStep Step, const QString& BaseName)
	{
		QImage Image = Model->GetPartsListImage(1024, Step);

		if (!Image.isNull())
		{
			QString ImageName = BaseName + QLatin1String("-parts.png");
			QString FileName = QFileInfo(Dir, ImageName).absoluteFilePath();

			Image.save(FileName);

			Stream << QString::fromLatin1("<p><IMG SRC=\"%1\" /></p><br><br>\r\n").arg(ImageName);
		}
	};

	for (lcModel* Model : Models)
	{
		QString BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1);
		lcStep LastStep = Model->GetLastStep();
		QString PageTitle;

		if (Models.GetSize() > 1)
		{
			BaseName += '-' + Model->GetProperties().mFileName;
			PageTitle = Model->GetProperties().mFileName;
		}
		else
			PageTitle = ProjectTitle;
		BaseName.replace('#', '_');

		if (Options.SinglePage)
		{
			QString FileName = QFileInfo(Dir, BaseName + QLatin1String(".html")).absoluteFilePath();
			QFile File(FileName);

			if (!File.open(QIODevice::WriteOnly))
			{
				QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString::fromLatin1("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(PageTitle);

			for (lcStep Step = 1; Step <= LastStep; Step++)
			{
				QString StepString = QString::fromLatin1("%1").arg(Step, 2, 10, QLatin1Char('0'));
				Stream << QString::fromLatin1("<p><IMG SRC=\"%1-%2.png\" ALT=\"Step %3\" WIDTH=%4 HEIGHT=%5></p><BR><BR>\r\n").arg(BaseName, StepString, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

				if (Options.PartsListStep)
					AddPartsListImage(Stream, Model, Step, QString("%1-%2").arg(BaseName, StepString));
			}

			if (Options.PartsListEnd)
				AddPartsListImage(Stream, Model, 0, BaseName);

			Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR><B><I>Created by <A HREF=\"https://www.leocad.org\">LeoCAD</A></I></B><BR></HTML>\r\n");
		}
		else
		{
			if (Options.IndexPage)
			{
				QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-index.html")).absoluteFilePath();
				QFile File(FileName);

				if (!File.open(QIODevice::WriteOnly))
				{
					QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
					return;
				}

				QTextStream Stream(&File);

				Stream << QString::fromLatin1("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(PageTitle);

				for (lcStep Step = 1; Step <= LastStep; Step++)
					Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Step %3<BR>\r\n</A>").arg(BaseName, QString("%1").arg(Step, 2, 10, QLatin1Char('0')), QString::number(Step));

				if (Options.PartsListEnd)
					Stream << QString::fromLatin1("<A HREF=\"%1-pieces.html\">Pieces Used</A><BR>\r\n").arg(BaseName);

				Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR><B><I>Created by <A HREF=\"https://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
			}

			for (lcStep Step = 1; Step <= LastStep; Step++)
			{
				QString StepString = QString::fromLatin1("%1").arg(Step, 2, 10, QLatin1Char('0'));
				QString FileName = QFileInfo(Dir, QString("%1-%2.html").arg(BaseName, StepString)).absoluteFilePath();
				QFile File(FileName);

				if (!File.open(QIODevice::WriteOnly))
				{
					QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
					return;
				}

				QTextStream Stream(&File);

				Stream << QString::fromLatin1("<HTML>\r\n<HEAD>\r\n<TITLE>%1 - Step %2</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(PageTitle, QString::number(Step));
				Stream << QString::fromLatin1("<IMG SRC=\"%1-%2.png\" ALT=\"Step %3\" WIDTH=%4 HEIGHT=%5><BR><BR>\r\n").arg(BaseName, StepString, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

				if (Options.PartsListStep)
					AddPartsListImage(Stream, Model, Step, QString("%1-%2").arg(BaseName, StepString));

				Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR>");
				if (Step != 1)
					Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(Step - 1, 2, 10, QLatin1Char('0')));

				if (Options.IndexPage)
					Stream << QString::fromLatin1("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

				if (Step != LastStep)
					Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Next</A>").arg(BaseName, QString("%1").arg(Step + 1, 2, 10, QLatin1Char('0')));
				else if (Options.PartsListEnd)
					Stream << QString::fromLatin1("<A HREF=\"%1-pieces.html\">Pieces Used</A>").arg(BaseName);

				Stream << QLatin1String("<BR></HTML>\r\n");
			}

			if (Options.PartsListEnd)
			{
				QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-pieces.html")).absoluteFilePath();
				QFile File(FileName);

				if (!File.open(QIODevice::WriteOnly))
				{
					QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
					return;
				}

				QTextStream Stream(&File);

				Stream << QString::fromLatin1("<HTML>\r\n<HEAD>\r\n<TITLE>Pieces used by %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\n").arg(PageTitle);

				AddPartsListImage(Stream, Model, 0, BaseName);

				Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR>");
				Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(LastStep, 2, 10, QLatin1Char('0')));

				if (Options.IndexPage)
					Stream << QString::fromLatin1("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

				Stream << QLatin1String("<BR></HTML>\r\n");
			}
		}

		QString StepImageBaseName = QFileInfo(Dir, BaseName + QLatin1String("-%1.png")).absoluteFilePath();
		Model->SaveStepImages(StepImageBaseName, true, false, Options.StepImagesWidth, Options.StepImagesHeight, 1, LastStep);
	}

	if (Models.GetSize() > 1)
	{
		QString BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1);
		QString FileName = QFileInfo(Dir, BaseName + QLatin1String("-index.html")).absoluteFilePath();
		QFile File(FileName);

		if (!File.open(QIODevice::WriteOnly))
		{
			QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
			return;
		}

		QTextStream Stream(&File);

		Stream << QString::fromLatin1("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(ProjectTitle);

		for (int ModelIdx = 0; ModelIdx < Models.GetSize(); ModelIdx++)
		{
			lcModel* Model = Models[ModelIdx];
			BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1) + '-' + Model->GetProperties().mFileName;
			BaseName.replace('#', '_');

			if (Options.SinglePage)
			{
				FileName = BaseName + QLatin1String(".html");
				Stream << QString::fromLatin1("<p><a href=\"%1\">%2</a>").arg(FileName, Model->GetProperties().mFileName);
			}
			else if (Options.IndexPage)
			{
				FileName = BaseName + QLatin1String("-index.html");
				Stream << QString::fromLatin1("<p><a href=\"%1\">%2</a>").arg(FileName, Model->GetProperties().mFileName);
			}
			else
			{
				lcStep LastStep = Model->GetLastStep();

				Stream << QString::fromLatin1("<p>%1</p>").arg(Model->GetProperties().mFileName);

				for (lcStep Step = 1; Step <= LastStep; Step++)
					Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Step %3<BR>\r\n</A>").arg(BaseName, QString("%1").arg(Step, 2, 10, QLatin1Char('0')), QString::number(Step));

				if (Options.PartsListEnd)
					Stream << QString::fromLatin1("<A HREF=\"%1-pieces.html\">Pieces Used</A><BR>\r\n").arg(BaseName);
			}
		}

		Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR><B><I>Created by <A HREF=\"https://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
	}
}

bool Project::ExportPOVRay(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return false;
	}

	QString SaveFileName = GetExportFileName(FileName, QLatin1String("pov"), tr("Export POV-Ray"), tr("POV-Ray Files (*.pov);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return false;

	lcDiskFile POVFile(SaveFileName);

	if (!POVFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return false;
	}

	POVFile.WriteLine("#version 3.7;\n\nglobal_settings {\n  assumed_gamma 1.0\n}\n\n");

	char Line[1024];

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	std::map<const PieceInfo*, std::pair<char[LC_PIECE_NAME_LEN + 1], int>> PieceTable;
	size_t NumColors = gColorList.size();
	std::vector<std::array<char, LC_MAX_COLOR_NAME>> ColorTable(NumColors);

	enum
	{
		LGEO_PIECE_LGEO  = 0x01,
		LGEO_PIECE_AR    = 0x02,
		LGEO_PIECE_SLOPE = 0x04
	};

	enum
	{
		LGEO_COLOR_SOLID       = 0x01,
		LGEO_COLOR_TRANSPARENT = 0x02,
		LGEO_COLOR_CHROME      = 0x04,
		LGEO_COLOR_PEARL       = 0x08,
		LGEO_COLOR_METALLIC    = 0x10,
		LGEO_COLOR_RUBBER      = 0x20,
		LGEO_COLOR_GLITTER     = 0x40
	};

	QString LGEOPath; // todo: load lgeo from registry and make sure it still works

	if (!LGEOPath.isEmpty())
	{
		lcDiskFile TableFile(QFileInfo(QDir(LGEOPath), QLatin1String("lg_elements.lst")).absoluteFilePath());

		if (!TableFile.Open(QIODevice::ReadOnly))
		{
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(LGEOPath));
			return false;
		}

		while (TableFile.ReadLine(Line, sizeof(Line)))
		{
			char Src[129], Dst[129], Flags[11];

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%128s%128s%10s", Src, Dst, Flags) != 3)
				continue;

			strcat(Src, ".dat");

			PieceInfo* Info = Library->FindPiece(Src, nullptr, false, false);
			if (!Info)
				continue;

			if (strchr(Flags, 'L'))
			{
				std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_LGEO;
				sprintf(Entry.first, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_AR;
				sprintf(Entry.first, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
			{
				std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_SLOPE;
				Entry.first[0] = 0;
			}
		}

		lcDiskFile ColorFile(QFileInfo(QDir(LGEOPath), QLatin1String("lg_colors.lst")).absoluteFilePath());

		if (!ColorFile.Open(QIODevice::ReadOnly))
		{
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(LGEOPath));
			return false;
		}

		while (ColorFile.ReadLine(Line, sizeof(Line)))
		{
			char Name[1024], Flags[1024];
			int Code;

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%d%s%s", &Code, Name, Flags) != 3)
				continue;

			size_t Color = lcGetColorIndex(Code);
			if (Color >= NumColors)
				continue;

			strcpy(ColorTable[Color].data(), Name);
		}
	}

	std::set<lcMesh*> AddedMeshes;

	if (!LGEOPath.isEmpty())
	{
		POVFile.WriteLine("#include \"lg_defs.inc\"\n#include \"lg_color.inc\"\n\n");

		for (const lcModelPartsEntry& ModelPart : ModelParts)
		{
			if (ModelPart.Mesh)
				continue;

			auto Search = PieceTable.find(ModelPart.Info);
			if (Search == PieceTable.end())
				continue;

			lcMesh* Mesh = ModelPart.Info->GetMesh();

			if (!Mesh)
				continue;

			if (!AddedMeshes.insert(Mesh).second)
				continue;

			const std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = Search->second;
			if (Entry.first[0])
			{
				sprintf(Line, "#include \"%s.inc\"\n", Entry.first);
				POVFile.WriteLine(Line);
			}
		}

		POVFile.WriteLine("\n");
	}

	for (size_t ColorIdx = 0; ColorIdx < gColorList.size(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		if (lcIsColorTranslucent(ColorIdx))
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> filter 0.9 } finish { ambient 0.3 diffuse 0.2 reflection 0.25 phong 0.3 phong_size 60 } }\n",
					Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}
		else
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> } finish { ambient 0.1 phong 0.2 phong_size 20 } }\n",
					Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}

		POVFile.WriteLine(Line);

		if (!ColorTable[ColorIdx][0])
			sprintf(ColorTable[ColorIdx].data(), "lc_%s", Color->SafeName);
	}

	POVFile.WriteLine("\n");

	std::vector<const char*> ColorTablePointer;
	ColorTablePointer.resize(NumColors);
	for (size_t ColorIdx = 0; ColorIdx < NumColors; ColorIdx++)
		ColorTablePointer[ColorIdx] = ColorTable[ColorIdx].data();

	auto GetMeshName = [](const lcModelPartsEntry& ModelPart, char (&Name)[LC_PIECE_NAME_LEN])
	{
		strcpy(Name, ModelPart.Info->mFileName);

		for (char* c = Name; *c; c++)
			if (*c == '-' || *c == '.')
				*c = '_';

		if (ModelPart.Mesh)
		{
			char Suffix[32];
			sprintf(Suffix, "_%p", ModelPart.Mesh);
			strncat(Name, Suffix, sizeof(Name) - 1);
			Name[sizeof(Name) - 1] = 0;
		}
	};

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!AddedMeshes.insert(Mesh).second)
			continue;

		if (!Mesh)
			continue;

		char Name[LC_PIECE_NAME_LEN];
		GetMeshName(ModelPart, Name);

		if (!ModelPart.Mesh)
		{
			std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[ModelPart.Info];
			strcpy(Entry.first, "lc_");
			strncat(Entry.first, Name, sizeof(Entry.first) - 1);
			Entry.first[sizeof(Entry.first) - 1] = 0;
		}

		Mesh->ExportPOVRay(POVFile, Name, &ColorTablePointer[0]);

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	const lcCamera* Camera = gMainWindow->GetActiveView()->GetCamera();
	const lcVector3& Position = Camera->mPosition;
	const lcVector3& Target = Camera->mTargetPosition;
	const lcVector3& Up = Camera->mUpVector;

	sprintf(Line, "camera {\n  perspective\n  right x * image_width / image_height\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f * image_width / image_height\n}\n\n",
			Up[1], Up[0], Up[2], Position[1] / 25.0f, Position[0] / 25.0f, Position[2] / 25.0f, Target[1] / 25.0f, Target[0] / 25.0f, Target[2] / 25.0f, Camera->m_fovy);
	POVFile.WriteLine(Line);
	lcVector3 BackgroundColor = lcVector3FromColor(lcGetPreferences().mBackgroundSolidColor);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\n", BackgroundColor[0], BackgroundColor[1], BackgroundColor[2]);
	POVFile.WriteLine(Line);

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX);
	lcVector3 Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcVector3 Points[8];
		
		lcGetBoxCorners(ModelPart.Info->GetBoundingBox(), Points);

		for (int PointIdx = 0; PointIdx < 8; PointIdx++)
		{
			lcVector3 Point = lcMul31(Points[PointIdx], ModelPart.WorldMatrix);

			Min = lcMin(Point, Min);
			Max = lcMax(Point, Max);
		}
	}

	lcVector3 Center = (Min + Max) / 2.0f;
	float Radius = (Max - Center).Length() / 25.0f;
	Center = lcVector3(Center[1], Center[0], Center[2]) / 25.0f;

	sprintf(Line, "light_source{ <%f, %f, %f>\n  color rgb 0.75\n  area_light 200, 200, 10, 10\n  jitter\n}\n\n", 0.0f * Radius + Center.x, -1.5f * Radius + Center.y, -1.5f * Radius + Center.z);
	POVFile.WriteLine(Line);
	sprintf(Line, "light_source{ <%f, %f, %f>\n  color rgb 0.75\n  area_light 200, 200, 10, 10\n  jitter\n}\n\n", 1.5f * Radius + Center.x, -1.0f * Radius + Center.y, 0.866026f * Radius + Center.z);
	POVFile.WriteLine(Line);
	sprintf(Line, "light_source{ <%f, %f, %f>\n  color rgb 0.5\n  area_light 200, 200, 10, 10\n  jitter\n}\n\n", 0.0f * Radius + Center.x, -2.0f * Radius + Center.y, 0.0f * Radius + Center.z);
	POVFile.WriteLine(Line);
	sprintf(Line, "light_source{ <%f, %f, %f>\n  color rgb 0.5\n  area_light 200, 200, 10, 10\n  jitter\n}\n\n", 2.0f * Radius + Center.x, 0.0f * Radius + Center.y, -2.0f * Radius + Center.z);
	POVFile.WriteLine(Line);

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		int Color = ModelPart.ColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";
		const float* f = ModelPart.WorldMatrix;

		if (!ModelPart.Mesh)
		{
			std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[ModelPart.Info];

			if (Entry.second & LGEO_PIECE_SLOPE)
			{
				sprintf(Line, "merge {\n object {\n  %s%s\n  texture { %s }\n }\n"
						" object {\n  %s_slope\n  texture { %s normal { bumps 0.3 scale 0.02 } }\n }\n"
						" matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						Entry.first, Suffix, ColorTable[Color].data(), Entry.first, ColorTable[Color].data(),
						-f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
			}
			else
			{
				if (!ModelPart.Info || !ModelPart.Info->GetMesh())
					continue;

				sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						Entry.first, Suffix, ColorTable[Color].data(), -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);

			}
		}
		else
		{
			char Name[LC_PIECE_NAME_LEN];
			GetMeshName(ModelPart, Name);

			sprintf(Line, "object {\n lc_%s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					Name, Suffix, ColorTable[Color].data(), -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}

		POVFile.WriteLine(Line);
	}

	return true;
}

void Project::ExportWavefront(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(FileName, QLatin1String("obj"), tr("Export Wavefront"), tr("Wavefront Files (*.obj);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile OBJFile(SaveFileName);
	char Line[1024];

	if (!OBJFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	quint32 vert = 1;

	OBJFile.WriteLine("# Model exported from LeoCAD\n");

	QFileInfo SaveInfo(SaveFileName);
	QString MaterialFileName = QDir(SaveInfo.absolutePath()).absoluteFilePath(SaveInfo.completeBaseName() + QLatin1String(".mtl"));

	sprintf(Line, "#\n\nmtllib %s\n\n", QFileInfo(MaterialFileName).fileName().toLatin1().constData());
	OBJFile.WriteLine(Line);

	lcDiskFile MaterialFile(MaterialFileName);
	if (!MaterialFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(MaterialFileName));
		return;
	}

	MaterialFile.WriteLine("# Colors used by LeoCAD\n\n");
	for (const lcColor& Color : gColorList)
	{
		if (Color.Translucent)
			sprintf(Line, "newmtl %s\nKd %.2f %.2f %.2f\nD %.2f\n\n", Color.SafeName, Color.Value[0], Color.Value[1], Color.Value[2], Color.Value[3]);
		else
			sprintf(Line, "newmtl %s\nKd %.2f %.2f %.2f\n\n", Color.SafeName, Color.Value[0], Color.Value[1], Color.Value[2]);
		MaterialFile.WriteLine(Line);
	}

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!Mesh)
			continue;

		const lcMatrix44& ModelWorld = ModelPart.WorldMatrix;
		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Vertex = lcMul31(Verts[VertexIdx].Position, ModelWorld);
			sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
			OBJFile.WriteLine(Line);
		}

		OBJFile.WriteLine("#\n\n");
	}

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (!Mesh)
			continue;

		const lcMatrix44& ModelWorld = ModelPart.WorldMatrix;
		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Normal = lcMul30(lcUnpackNormal(Verts[VertexIdx].Normal), ModelWorld);
			sprintf(Line, "vn %.2f %.2f %.2f\n", Normal[0], Normal[1], Normal[2]);
			OBJFile.WriteLine(Line);
		}

		OBJFile.WriteLine("#\n\n");
	}

	int NumPieces = 0;
	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		sprintf(Line, "g Piece%.3d\n", NumPieces++);
		OBJFile.WriteLine(Line);

		lcMesh* Mesh = !ModelPart.Mesh ? ModelPart.Info->GetMesh() : ModelPart.Mesh;

		if (Mesh)
		{
			Mesh->ExportWavefrontIndices(OBJFile, ModelPart.ColorIndex, vert);
			vert += Mesh->mNumVertices;
		}
	}
}

void Project::SaveImage()
{
	lcQImageDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;
	
	QString Extension = QFileInfo(Dialog.mFileName).suffix();

	if (!Extension.isEmpty())
		lcSetProfileString(LC_PROFILE_IMAGE_EXTENSION, Dialog.mFileName.right(Extension.length() + 1));

	if (Dialog.mStart != Dialog.mEnd)
		Dialog.mFileName = Dialog.mFileName.insert(Dialog.mFileName.length() - Extension.length() - 1, QLatin1String("%1"));

	mActiveModel->SaveStepImages(Dialog.mFileName, Dialog.mStart != Dialog.mEnd, false, Dialog.mWidth, Dialog.mHeight, Dialog.mStart, Dialog.mEnd);
}

void Project::UpdatePieceInfo(PieceInfo* Info) const
{
	if (!mModels.IsEmpty())
	{
		std::vector<lcModel*> UpdatedModels;
		mModels[0]->UpdatePieceInfo(UpdatedModels);

		lcBoundingBox BoundingBox = mModels[0]->GetPieceInfo()->GetBoundingBox();
		Info->SetBoundingBox(BoundingBox.Min, BoundingBox.Max);
	}
}

void Project::MarkAsModified()
{
	mModified = true;
}
