#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <locale.h>
#include "pieceinf.h"
#include "camera.h"
#include "project.h"
#include "lc_instructions.h"
#include "image.h"
#include "light.h"
#include "lc_mainwindow.h"
#include "lc_view.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_file.h"
#include "lc_zipfile.h"
#include "lc_qimagedialog.h"
#include "lc_modellistdialog.h"
#include "lc_bricklink.h"

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
			QStringList CachePathList = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
			FileName = CachePathList.first();
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

		lcView* ActiveView = gMainWindow ? gMainWindow->GetActiveView() : nullptr;
		if (ActiveView)
			ActiveView->GetCamera()->SetViewpoint(lcViewpoint::Home);

		gMainWindow->UpdateTitle();
	}
	else
		SetActiveModel(mModels.FindIndex(mActiveModel));

	return Model;
}

void Project::ShowModelListDialog()
{
	lcModelListDialog Dialog(gMainWindow, mModels);

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
				Source->SaveLDraw(SaveStream, false, 0);
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

bool Project::Load(const QString& FileName, bool ShowErrors)
{
	QWidget *parent = nullptr;
	if (!mIsPreview)
		parent = gMainWindow;

	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		if (ShowErrors)
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
					return ModelIt.second->GetFileName().compare(Model->GetFileName(), Qt::CaseInsensitive) == 0;
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
		if (ShowErrors)
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

		Model->SaveLDraw(Stream, false, 0);
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

bool Project::ExportCurrentStep(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	QStringList Models;

	Models.append(lcGetActiveModel()->GetProperties().mFileName);

	std::function<void(const QString&)> ParseStepModel = [&](const QString& ModelName)
	{
		Models.append(ModelName);

		for (lcModel* Model : mModels)
		{
			if (Model->GetProperties().mFileName == ModelName)
			{
				lcPartsList ModelParts;

				Model->GetPartsList(gDefaultColor, false, true, ModelParts);

				for (const auto& PartIt : ModelParts)
				{
					const PieceInfo* PartInfo = PartIt.first;

					if (PartInfo->IsModel())
					{
						ParseStepModel(PartInfo->mFileName);
					}
					else
						continue;
				}

				break;
			}
		}
	};

	const lcStep CurrentStep = lcGetActiveModel()->GetCurrentStep();

	bool MPD = mModels.GetSize() > 1;

	if (MPD)
	{
		lcPartsList StepParts;

		lcGetActiveModel()->GetPartsListForStep(CurrentStep, gDefaultColor, StepParts, true);

		if (!StepParts.empty())
		{
			for (const auto& PartIt : StepParts)
			{
				const PieceInfo *PartInfo = PartIt.first;

				if (PartInfo->IsModel())
				{
					ParseStepModel(PartInfo->mFileName);
				}
				else
					continue;
			}
		}
	}

	QTextStream Stream(&File);

	for (lcModel* Model : mModels)
	{
		if (!Models.contains(Model->GetProperties().mFileName))
			continue;

		const lcStep ModelStep = Model->GetCurrentStep();

		if (!Model->IsActive())
			Model->SetTemporaryStep(CurrentStep);

		if (MPD)
			Stream << QLatin1String("0 FILE ") << Model->GetProperties().mFileName << QLatin1String("\r\n");

		Model->SaveLDraw(Stream, false, CurrentStep);

		if (MPD)
			Stream << QLatin1String("0 NOFILE\r\n");

		if (!Model->IsActive())
		{
			Model->SetTemporaryStep(ModelStep);
			Model->CalculateStep(LC_STEP_MAX);
		}
	}

	File.close();

	lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(FileName).absolutePath());

	return true;
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

	Model->SaveLDraw(Stream, false, 0);

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

bool Project::Export3DStudio(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return false;
	}

	QString SaveFileName = GetExportFileName(FileName, "3ds", tr("Export 3D Studio"), tr("3DS Files (*.3ds);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return false;

	lcDiskFile File(SaveFileName);

	if (!File.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return false;
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

	return true;
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

	lcExportBrickLink(SaveFileName, PartsList);
}

bool Project::ExportCOLLADA(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return false;
	}

	QString SaveFileName = GetExportFileName(FileName, "dae", tr("Export COLLADA"), tr("COLLADA Files (*.dae);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return false;

	QFile File(SaveFileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return false;
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

	return true;
}

bool Project::ExportCSV(const QString& FileName)
{
	lcPartsList PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, true, false, PartsList);

	if (PartsList.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return false;
	}

	QString SaveFileName = GetExportFileName(FileName, "csv", tr("Export CSV"), tr("CSV Files (*.csv);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return false;

	lcDiskFile CSVFile(SaveFileName);
	char Line[1024];

	if (!CSVFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return false;
	}

	CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

	for (const auto& PartIt : PartsList)
	{
		const PieceInfo* Info = PartIt.first;

		for (const auto& ColorIt : PartIt.second)
		{
			std::string Description = Info->m_strDescription;
			Description.erase(std::remove(Description.begin(), Description.end(), ','), Description.end());

			sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", Description.c_str(), gColorList[ColorIt.first].Name, ColorIt.second, Info->mFileName, gColorList[ColorIt.first].Code);
			CSVFile.WriteLine(Line);
		}
	}

	return true;
}

lcInstructions* Project::GetInstructions()
{
	mInstructions.reset();
	mInstructions = std::unique_ptr<lcInstructions>(new lcInstructions(this));

	return mInstructions.get();
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
		QImage Image = Model->GetPartsListImage(1024, Step, LC_RGBA(255, 255, 255, 0), QFont("Arial", 16, QFont::Bold), Qt::black);

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

	char Line[1024];

	sprintf(Line, "// Generated By: LeoCAD %s\n// LDraw File: %s\n// Date: %s\n\n",
			LC_VERSION_TEXT,
			mModels[0]->GetProperties().mFileName.toLatin1().constData(),
			QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1().constData());
	POVFile.WriteLine(Line);

	POVFile.WriteLine("#version 3.7;\n\n");

	POVFile.WriteLine("global_settings { assumed_gamma 1.0 }\n\n");

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	std::map<const PieceInfo*, std::pair<char[LC_PIECE_NAME_LEN + 1], int>> PieceTable;
	size_t NumColors = gColorList.size();
	std::vector<std::array<char, LC_MAX_COLOR_NAME>> LgeoColorTable(NumColors);
	std::vector<std::array<char, LC_MAX_COLOR_NAME>> ColorTable(NumColors);

	const lcArray<lcLight*> Lights = gMainWindow->GetActiveModel()->GetLights();
	const lcCamera* Camera = gMainWindow->GetActiveView()->GetCamera();
	const QString CameraName = QString(Camera->GetName()).replace(" ","_");
	const lcVector3& Position = Camera->mPosition;
	const lcVector3& Target = Camera->mTargetPosition;
	const lcVector3& Up = Camera->mUpVector;
	const lcVector3 BackgroundColor = lcVector3FromColor(lcGetPreferences().mBackgroundSolidColor);
	const lcPOVRayOptions& POVRayOptions = mModels[0]->GetPOVRayOptions();
	const QString TopModelName = QString("LC_%1").arg(QString(mModels[0]->GetFileName()).replace(" ","_").replace(".","_dot_"));
	const QString LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	const bool UseLGEO = POVRayOptions.UseLGEO && !LGEOPath.isEmpty();
	const int TopModelColorCode = 7;
	QStringList ColorMacros, MaterialColors;

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

	lcVector3 FloorColor = POVRayOptions.FloorColor;
	float FloorAmbient = POVRayOptions.FloorAmbient;
	float FloorDiffuse = POVRayOptions.FloorDiffuse;
	char FloorLocation[32];
	char FloorAxis[16];
	if (POVRayOptions.FloorAxis == 0)
	{
		sprintf(FloorAxis, "x");
		sprintf(FloorLocation, "MaxX");
	}
	else if (POVRayOptions.FloorAxis == 1)
	{
		sprintf(FloorAxis, "y");
		sprintf(FloorLocation, "MaxY");
	}
	else
	{
		sprintf(FloorAxis, "z");
		sprintf(FloorLocation, "MaxZ");
	}

	for (const lcLight* Light : Lights)
	{
		if (Light->GetLightType() == lcLightType::Area)
		{
			if (FloorColor == lcVector3(0.8f,0.8f,0.8f))
				FloorColor = {1.0f,1.0f,1.0f};
			if (FloorAmbient == 0.4f)
				FloorAmbient = 0.0f;
			if (FloorDiffuse == 0.4f)
				FloorDiffuse = 0.9f;
			break;
		}
	}

	if (!POVRayOptions.HeaderIncludeFile.isEmpty())
	{
		sprintf(Line, "#include \"%s\"\n\n", POVRayOptions.HeaderIncludeFile.toLatin1().constData());
		POVFile.WriteLine(Line);
	}

	sprintf(Line,
			"#ifndef (MinX) #declare MinX = %g; #end\n"
			"#ifndef (MinY) #declare MinY = %g; #end\n"
			"#ifndef (MinZ) #declare MinZ = %g; #end\n"
			"#ifndef (MaxX) #declare MaxX = %g; #end\n"
			"#ifndef (MaxY) #declare MaxY = %g; #end\n"
			"#ifndef (MaxZ) #declare MaxZ = %g; #end\n"
			"#ifndef (CenterX) #declare CenterX = %g; #end\n"
			"#ifndef (CenterY) #declare CenterY = %g; #end\n"
			"#ifndef (CenterZ) #declare CenterZ = %g; #end\n"
			"#ifndef (Center) #declare Center = <CenterX,CenterY,CenterZ>; #end\n"
			"#ifndef (Radius) #declare Radius = %g; #end\n",
			Min[0], Min[1], Min[2], Max[0], -Max[1], Max[2], Center[0], Center[1], Center[2], Radius);
	POVFile.WriteLine(Line);
	sprintf(Line,
			"#ifndef (CameraSky) #declare CameraSky = <%g,%g,%g>; #end\n"
			"#ifndef (CameraLocation) #declare CameraLocation = <%g, %g, %g>; #end\n"
			"#ifndef (CameraTarget) #declare CameraTarget = <%g, %g, %g>; #end\n"
			"#ifndef (CameraAngle) #declare CameraAngle = %g; #end\n",
			Up[1], Up[0], Up[2], Position[1] / 25.0f, Position[0] / 25.0f, Position[2] / 25.0f, Target[1] / 25.0f, Target[0] / 25.0f, Target[2] / 25.0f, Camera->m_fovy);
	POVFile.WriteLine(Line);
	sprintf(Line,
			"#ifndef (BackgroundColor) #declare BackgroundColor = <%1g, %1g, %1g>; #end\n"
			"#ifndef (Background) #declare Background = %s; #end\n",
			BackgroundColor[0], BackgroundColor[1], BackgroundColor[2], (POVRayOptions.ExcludeBackground ? "false" : "true"));
	POVFile.WriteLine(Line);
	sprintf(Line,
			"#ifndef (FloorAxis) #declare FloorAxis = %s; #end\n"
			"#ifndef (FloorLocation) #declare FloorLocation = %s; #end\n"
			"#ifndef (FloorColor) #declare FloorColor = <%1g, %1g, %1g>; #end\n"
			"#ifndef (FloorAmbient) #declare FloorAmbient = %1g; #end\n"
			"#ifndef (FloorDiffuse) #declare FloorDiffuse = %1g; #end\n"
			"#ifndef (Floor) #declare Floor = %s; #end\n",
			FloorAxis, FloorLocation, FloorColor[0], FloorColor[1], FloorColor[2], FloorAmbient, FloorDiffuse, (POVRayOptions.ExcludeFloor ? "false" : "true"));
	POVFile.WriteLine(Line);
	sprintf(Line,
			"#ifndef (Ambient) #declare Ambient = 0.4; #end\n"
			"#ifndef (Diffuse) #declare Diffuse = 0.4; #end\n"
			"#ifndef (Reflection) #declare Reflection = 0.08; #end\n"
			"#ifndef (Phong) #declare Phong = 0.5; #end\n"
			"#ifndef (PhongSize) #declare PhongSize = 40; #end\n"
			"#ifndef (TransReflection) #declare TransReflection = 0.2; #end\n"
			"#ifndef (TransFilter) #declare TransFilter = 0.85; #end\n"
			"#ifndef (TransIoR) #declare TransIoR = 1.25; #end\n"
			"#ifndef (RubberReflection) #declare RubberReflection = 0; #end\n"
			"#ifndef (RubberPhong) #declare RubberPhong = 0.1; #end\n"
			"#ifndef (RubberPhongS) #declare RubberPhongS = 10; #end\n"
			"#ifndef (ChromeReflection) #declare ChromeReflection = 0.85; #end\n"
			"#ifndef (ChromeBrilliance) #declare ChromeBrilliance = 5; #end\n"
			"#ifndef (ChromeSpecular) #declare ChromeSpecular = 0.8; #end\n"
			"#ifndef (ChromeRough) #declare ChromeRough = 0.01; #end\n"
			"#ifndef (OpaqueNormal) #declare OpaqueNormal = normal { bumps 0.001 scale 0.5 }; #end\n"
			"#ifndef (TransNormal) #declare TransNormal = normal { bumps 0.001 scale 0.5 }; #end\n");
	POVFile.WriteLine(Line);
	sprintf(Line,
			"#ifndef (Quality) #declare Quality = 3; #end\n"
			"#ifndef (Studs) #declare Studs = 1; #end\n"
			"#ifndef (LgeoLibrary) #declare LgeoLibrary = %s; #end\n"
			"#ifndef (ModelReflection) #declare ModelReflection = %i; #end\n"
			"#ifndef (ModelShadow) #declare ModelShadow = %i; #end\n\n",
			(POVRayOptions.UseLGEO ? "true" : "false"), (POVRayOptions.NoReflection ? 0 : 1), (POVRayOptions.NoShadow ? 0 : 1));
	POVFile.WriteLine(Line);

	sprintf(Line,
			"#ifndef (SkipWriteLightMacro)\n"
			"#macro WriteLight(Type, Shadowless, Location, Target, Color, Power, SpotRadius, SpotFalloff, SpotTightness, AreaCircle, AreaWidth, AreaHeight, AreaRows, AreaColumns)\n"
			"  #local PointLight = %i;\n"
			"  #local Spotlight = %i;\n"
			"  #local DirectionalLight = %i;\n"
			"  #local AreaLight = %i;\n"
			"  light_source {\n"
			"    Location\n"
			"    color rgb Color*Power\n"
			"    #if (Shadowless > 0)\n"
			"      shadowless\n"
			"    #end\n"
			"    #if (Type = Spotlight)\n"
			"      spotlight\n"
			"      radius SpotRadius\n"
			"      falloff SpotFalloff\n"
			"      tightness SpotTightness\n"
			"      point_at Target\n"
			"    #elseif (Type = DirectionalLight)\n"
			"      parallel\n"
			"      point_at Target\n"
			"    #elseif (Type = AreaLight)\n"
			"      area_light AreaWidth, AreaHeight, AreaRows, AreaColumns\n"
			"      jitter\n"
			"      #if (AreaCircle > 0 & AreaWidth > 2 & AreaHeight > 2 & AreaRows > 1 & AreaColumns > 1 )\n"
			"        circular \n"
			"        #if (AreaWidth = AreaHeight & AreaRows = AreaColumns)\n"
			"          orient\n"
			"        #end\n"
			"      #end\n"
			"    #end\n"
			"  }\n"
			"#end\n"
			"#end\n\n",
			lcLightType::Point, lcLightType::Spot, lcLightType::Directional, lcLightType::Area);
	POVFile.WriteLine(Line);

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		int ColorIdx = ModelPart.ColorIndex;

		if (lcIsColorTranslucent(ColorIdx))
		{
			if (!ColorMacros.contains("TranslucentColor"))
			{
				sprintf(Line,
						"#ifndef (SkipTranslucentColorMacro)\n"
						"#macro TranslucentColor(r, g, b, f)\n"
						"  material {\n"
						"    texture {\n"
						"      pigment { srgbf <r,g,b,f> }\n"
						"      finish { emission 0 ambient Ambient diffuse Diffuse }\n"
						"      finish { phong Phong phong_size PhongSize reflection TransReflection }\n"
						"      normal { TransNormal }\n"
						"    }\n"
						"    interior { ior TransIoR }\n"
						"  }\n"
						"#end\n"
						"#end\n\n");
				POVFile.WriteLine(Line);
				ColorMacros.append("TranslucentColor");
			}
		}
		else if (lcIsColorChrome(ColorIdx))
		{
			if (!ColorMacros.contains("ChromeColor"))
			{
				sprintf(Line,
						"#ifndef (SkipChromeColorMacro)\n"
						"#macro ChromeColor(r, g, b)\n"
						"#if (LgeoLibrary) material { #end\n"
						"  texture {\n"
						"    pigment { srgbf <r,g,b,0> }\n"
						"    finish { emission 0 ambient Ambient diffuse Diffuse }\n"
						"    finish { phong Phong phong_size PhongSize reflection ChromeReflection brilliance ChromeBrilliance metallic specular ChromeSpecular roughness ChromeRough }\n"
						"  }\n"
						"#if (LgeoLibrary) } #end\n"
						"#end\n"
						"#end\n\n");
				POVFile.WriteLine(Line);
				ColorMacros.append("ChromeColor");
			}
		}
		else if (lcIsColorRubber(ColorIdx))
		{
			if (!ColorMacros.contains("RubberColor"))
			{
				sprintf(Line,
						"#ifndef (SkipRubberColorMacro)\n"
						"#macro RubberColor(r, g, b)\n"
						"#if (LgeoLibrary) material { #end\n"
						"  texture {\n"
						"    pigment { srgbf <r,g,b,0> }\n"
						"    finish { emission 0 ambient Ambient diffuse Diffuse }\n"
						"    finish { phong RubberPhong phong_size RubberPhongS reflection RubberReflection }\n"
						"  }\n"
						"#if (LgeoLibrary) } #end\n"
						"#end\n"
						"#end\n\n");
				POVFile.WriteLine(Line);
				ColorMacros.append("RubberColor");
			}
		}
		else
		{
			if (!ColorMacros.contains("OpaqueColor"))
			{
				sprintf(Line,
						"#ifndef (SkipOpaqueColorMacro)\n"
						"#macro OpaqueColor(r, g, b)\n"
						"#if (LgeoLibrary) material { #end\n"
						"  texture {\n"
						"    pigment { srgbf <r,g,b,0> }\n"
						"    finish { emission 0 ambient Ambient diffuse Diffuse }\n"
						"    finish { phong Phong phong_size PhongSize reflection Reflection }\n"
						"    normal { OpaqueNormal }\n"
						"  }\n"
						"#if (LgeoLibrary) } #end\n"
						"#end\n"
						"#end\n\n");
				POVFile.WriteLine(Line);
				ColorMacros.append("OpaqueColor");
			}
		}
	}

	sprintf(Line, "#if (Background)\n  background {\n    color rgb BackgroundColor\n  }\n#end\n\n");
	POVFile.WriteLine(Line);

	sprintf(Line, "#ifndef (Skip%s)\n  camera {\n    perspective\n    right x * image_width / image_height\n    sky CameraSky\n    location CameraLocation\n    look_at CameraTarget\n    angle CameraAngle * image_width / image_height\n  }\n#end\n\n",
			(CameraName.isEmpty() ? "Camera" : CameraName.toLatin1().constData()));
	POVFile.WriteLine(Line);

	lcVector2 AreaSize(200.0f, 200.0f), AreaGrid(10.0f, 10.0f);
	int AreaCircle = 0, Shadowless = 0;
	lcLightType LightType = lcLightType::Area;
	float Power = 0, SpotRadius = 0, SpotFalloff = 0, SpotTightness = 0;

	if (Lights.IsEmpty())
	{
		const lcVector3 LightTarget(0.0f, 0.0f, 0.0f), LightColor(1.0f, 1.0f, 1.0f);
		lcVector3 Location[4];

		Location[0] = {0.0f * Radius + Center[0], -1.5f * Radius + Center[1], -1.5f * Radius + Center[2]};
		Location[1] = {1.5f * Radius + Center[0], -1.0f * Radius + Center[1],  0.866026f * Radius + Center[2]};
		Location[2] = {0.0f * Radius + Center[0], -2.0f * Radius + Center[1],  0.0f * Radius + Center[2]};
		Location[3] = {2.0f * Radius + Center[0],  0.0f * Radius + Center[1], -2.0f * Radius + Center[2]};

		for (int Idx = 0; Idx < 4; Idx++)
		{
			Power = Idx < 2 ? 0.75f : 0.5f;
			sprintf(Line,"#ifndef (SkipLight%i)\nWriteLight(%i, %i, <%g, %g, %g>, <%g, %g, %g>, <%g, %g, %g>, %g, %g, %g, %g, %i, %i, %i, %i, %i)\n#end\n\n",
					Idx,
					LightType,
					Shadowless,
					Location[Idx][0], Location[Idx][1], Location[Idx][2],
					LightTarget[0], LightTarget[1], LightTarget[2],
					LightColor[0], LightColor[1], LightColor[2],
					Power,
					SpotRadius, SpotFalloff, SpotTightness,
					AreaCircle, (int)AreaSize[0], (int)AreaSize[1], (int)AreaGrid[0], (int)AreaGrid[1]);
			POVFile.WriteLine(Line);
		}
	}
	else
	{
		for (const lcLight* Light : Lights)
		{
			const lcVector3 LightPosition = Light->GetPosition();
			const lcVector3 LightTarget = LightPosition + Light->GetDirection();
			const lcVector3 LightColor = Light->GetColor();
			const QString LightName = QString(Light->GetName()).replace(" ", "_");
			LightType = Light->GetLightType();
			Shadowless = Light->GetCastShadow() ? 0 : 1;
			Power = Light->mPOVRayExponent;

			switch (LightType)
			{
			case lcLightType::Point:
				break;

			case lcLightType::Spot:
				SpotFalloff = Light->GetSpotConeAngle() / 2.0f;
				SpotRadius = SpotFalloff - Light->GetSpotPenumbraAngle();
				break;

			case lcLightType::Directional:
				break;

			case lcLightType::Area:
				AreaCircle = (Light->GetAreaShape() == lcLightAreaShape::Disk || Light->GetAreaShape() == lcLightAreaShape::Ellipse) ? 1 : 0;
				AreaSize = Light->GetSize();
				AreaGrid = Light->mAreaGrid;
				break;

			case lcLightType::Count:
				break;
			}

			sprintf(Line,"#ifndef (Skip%s)\n  WriteLight(%i, %i, <%g, %g, %g>, <%g, %g, %g>, <%g, %g, %g>, %g, %g, %g, %g, %i, %i, %i, %i, %i)\n#end\n\n",
					LightName.toLatin1().constData(),
					LightType,
					Shadowless,
					LightPosition[1], LightPosition[0], LightPosition[2],
					LightTarget[1], LightTarget[0], LightTarget[2],
					LightColor[0], LightColor[1], LightColor[2],
					Power,
					SpotRadius, SpotFalloff, SpotTightness,
					AreaCircle, (int)AreaSize[0], (int)AreaSize[1], (int)AreaGrid[0], (int)AreaGrid[1]);
			POVFile.WriteLine(Line);
		}
	}

	POVFile.WriteLine("#ifndef (lg_quality) #declare lg_quality = Quality; #end\n\n");

	if (UseLGEO)
	{
		memset(Line, 0, 1024);

		POVFile.WriteLine("#ifndef (lg_studs) #declare lg_studs = Studs; #end\n\n");

		POVFile.WriteLine("#if (lg_quality = 3) #declare lg_quality = 4; #end\n\n");

		POVFile.WriteLine("#include \"lg_defs.inc\"\n\n#include \"lg_color.inc\"\n\n");

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

			strncat(Src, ".dat", 4);

			PieceInfo* Info = Library->FindPiece(Src, nullptr, false, false);
			if (!Info)
				continue;

			bool LgeoPartFound = false;

			if ((LgeoPartFound = strchr(Flags, 'L')))
			{
				std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_LGEO;
				sprintf(Entry.first, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A') && !LgeoPartFound)
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

		lcDiskFile LgeoColorFile(QFileInfo(QDir(LGEOPath), QLatin1String("lg_colors.lst")).absoluteFilePath());

		if (!LgeoColorFile.Open(QIODevice::ReadOnly))
		{
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(LGEOPath));
			return false;
		}

		while (LgeoColorFile.ReadLine(Line, sizeof(Line)))
		{
			char Name[1024], Flags[1024];
			int Code;

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%d%s%s", &Code, Name, Flags) != 3)
				continue;

			size_t ColorIdx = lcGetColorIndex(Code);
			if (ColorIdx >= NumColors)
				continue;

			strncpy(LgeoColorTable[ColorIdx].data(), Name, LC_MAX_COLOR_NAME);
		}
	}

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		size_t ColorIdx = ModelPart.ColorIndex;

		if (!ColorTable[ColorIdx][0])
		{
			lcColor* Color = &gColorList[ColorIdx];

			if (!LgeoColorTable[ColorIdx][0])
			{
				sprintf(ColorTable[ColorIdx].data(), "lc_%s", Color->SafeName);

				if (lcIsColorTranslucent(ColorIdx))
				{
					if (!UseLGEO && !MaterialColors.contains(ColorTable[ColorIdx].data()))
						MaterialColors.append(ColorTable[ColorIdx].data());

					sprintf(Line, "#ifndef (lc_%s)\n#declare lc_%s = TranslucentColor(%g, %g, %g, TransFilter)\n#end\n\n",
							Color->SafeName, Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
				}
				else
				{
					char MacroName[LC_MAX_COLOR_NAME];
					if (lcIsColorChrome(ColorIdx))
						sprintf(MacroName, "Chrome");
					else if (lcIsColorRubber(ColorIdx))
						sprintf(MacroName, "Rubber");
					else
						sprintf(MacroName, "Opaque");

					sprintf(Line, "#ifndef (lc_%s)\n#declare lc_%s = %sColor(%g, %g, %g)\n#end\n\n",
							Color->SafeName, Color->SafeName, MacroName, Color->Value[0], Color->Value[1], Color->Value[2]);
				}
			}
			else
			{
				sprintf(ColorTable[ColorIdx].data(), "LDXColor%i", Color->Code);

				sprintf(Line,"#ifndef (LDXColor%i) // %s\n#declare LDXColor%i = material { texture { %s } }\n#end\n\n",
						Color->Code, Color->Name, Color->Code, LgeoColorTable[ColorIdx].data());
			}

			POVFile.WriteLine(Line);
		}
	}

	if (!ColorTable[lcGetColorIndex(TopModelColorCode)][0])
	{
		size_t ColorIdx = lcGetColorIndex(TopModelColorCode);

		lcColor* Color = &gColorList[ColorIdx];

		sprintf(ColorTable[ColorIdx].data(), "LDXColor%i", Color->Code);

		if (!LgeoColorTable[ColorIdx][0])
			sprintf(Line, "#ifndef (lc_%s)\n#declare lc_%s = OpaqueColor(%g, %g, %g)\n#end\n\n",
					Color->SafeName, Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		else
			sprintf(Line,"#ifndef (LDXColor%i) // %s\n#declare LDXColor%i = material { texture { %s } }\n#end\n\n",
					Color->Code, Color->Name, Color->Code, LgeoColorTable[ColorIdx].data());

		POVFile.WriteLine(Line);
	}

	std::set<lcMesh*> AddedMeshes;

	if (UseLGEO)
	{
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
				sprintf(Line, "#include \"%s.inc\" // %s\n", Entry.first, ModelPart.Info->m_strDescription);
				POVFile.WriteLine(Line);
			}
		}

		POVFile.WriteLine("\n");
	}

	std::vector<const char*> ColorTablePointer;
	ColorTablePointer.resize(NumColors);
	for (size_t ColorIdx = 0; ColorIdx < NumColors; ColorIdx++)
		ColorTablePointer[ColorIdx] = ColorTable[ColorIdx].data();

	auto GetMeshName = [](const lcModelPartsEntry& ModelPart, char (&Name)[LC_PIECE_NAME_LEN])
	{
		strncpy(Name, ModelPart.Info->mFileName, sizeof(Name));

		for (char* c = Name; *c; c++)
			if (*c == '-' || *c == '.')
				*c = '_';

		if (ModelPart.Mesh)
		{
			char Suffix[32];
			sprintf(Suffix, "_%p", ModelPart.Mesh);
			strncat(Name, Suffix, sizeof(Name) - strlen(Name) - 1);
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
			strncpy(Entry.first, "lc_", 3);
			strncat(Entry.first, Name, sizeof(Entry.first) - 1);
			Entry.first[sizeof(Entry.first) - 1] = 0;
		}

		Mesh->ExportPOVRay(POVFile, Name, &ColorTablePointer[0]);

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	sprintf(Line, "#declare %s = union {\n", TopModelName.toLatin1().constData());
	POVFile.WriteLine(Line);

	for (const lcModelPartsEntry& ModelPart : ModelParts)
	{
		int ColorIdx = ModelPart.ColorIndex;
		const char* Suffix = lcIsColorTranslucent(ColorIdx) ? "_clear" : "";
		const float* f = ModelPart.WorldMatrix;
		char Modifier[32];
		if (UseLGEO || MaterialColors.contains(ColorTable[ColorIdx].data()))
			sprintf(Modifier, "material");
		else
			sprintf(Modifier, "texture");
		if (!ModelPart.Mesh)
		{
			std::pair<char[LC_PIECE_NAME_LEN + 1], int>& Entry = PieceTable[ModelPart.Info];

			if (Entry.second & LGEO_PIECE_SLOPE)
			{
				sprintf(Line,
						"  merge {\n    object {\n      %s%s\n      %s { %s }\n    }\n"
						"    object {\n      %s_slope\n      texture {\n        %s normal { bumps 0.3 scale 0.02 }\n      }\n    }\n"
						"    matrix <%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g>\n  }\n",
						Entry.first, Suffix, Modifier, ColorTable[ColorIdx].data(), Entry.first, ColorTable[ColorIdx].data(),
						-f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
			}
			else
			{
				if (!ModelPart.Info || !ModelPart.Info->GetMesh())
					continue;

				sprintf(Line, "  object {\n    %s%s\n    %s { %s }\n    matrix <%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g>\n  }\n",
						Entry.first, Suffix, Modifier, ColorTable[ColorIdx].data(), -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);

			}
		}
		else
		{
			char Name[LC_PIECE_NAME_LEN];
			GetMeshName(ModelPart, Name);

			sprintf(Line, "  object {\n    lc_%s%s\n    %s { %s }\n    matrix <%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g>\n  }\n",
					Name, Suffix, Modifier, ColorTable[ColorIdx].data(), -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}

		POVFile.WriteLine(Line);
	}

	sprintf(Line, "\n  #if (ModelReflection = 0)\n    no_reflection\n  #end\n  #if (ModelShadow = 0)\n    no_shadow\n  #end\n}\n\n");
	POVFile.WriteLine(Line);

	sprintf(Line, "object {\n  %s\n  %s { %s }\n}\n\n",
			TopModelName.toLatin1().constData(), (UseLGEO ? "material" : "texture"), ColorTable[lcGetColorIndex(TopModelColorCode)].data());
	POVFile.WriteLine(Line);

	sprintf(Line, "#if (Floor)\n  object {\n    plane { FloorAxis, FloorLocation hollow }\n    texture {\n      pigment { color srgb FloorColor }\n      finish { emission 0 ambient FloorAmbient diffuse FloorDiffuse }\n    }\n  }\n#end\n");
	POVFile.WriteLine(Line);

	if (!POVRayOptions.FooterIncludeFile.isEmpty())
	{
		sprintf(Line, "\n#include \"%s\"\n", POVRayOptions.FooterIncludeFile.toLatin1().constData());
		POVFile.WriteLine(Line);
	}

	return true;
}

bool Project::ExportWavefront(const QString& FileName)
{
	std::vector<lcModelPartsEntry> ModelParts = GetModelParts();

	if (ModelParts.empty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return false;
	}

	QString SaveFileName = GetExportFileName(FileName, QLatin1String("obj"), tr("Export Wavefront"), tr("Wavefront Files (*.obj);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return false;

	lcDiskFile OBJFile(SaveFileName);
	char Line[1024];

	if (!OBJFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return false;
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
		return false;
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

	return true;
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
