#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <locale.h>
#include <array>
#include "pieceinf.h"
#include "camera.h"
#include "project.h"
#include "image.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_file.h"
#include "lc_zipfile.h"
#include "lc_qimagedialog.h"
#include "lc_qmodellistdialog.h"
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtConcurrent>
#endif

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
	HighlightNewParts = (HTMLOptions & LC_HTML_HIGHLIGHT) != 0;
	PartsListStep = (HTMLOptions & LC_HTML_LISTSTEP) != 0;
	PartsListEnd = (HTMLOptions & LC_HTML_LISTEND) != 0;
	PartsListImages = (HTMLOptions & LC_HTML_IMAGES) != 0;
	PartImagesColor = lcGetColorIndex(lcGetProfileInt(LC_PROFILE_HTML_PARTS_COLOR));
	PartImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH);
	PartImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT);
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
	if (HighlightNewParts)
		HTMLOptions |= LC_HTML_HIGHLIGHT;
	if (PartsListStep)
		HTMLOptions |= LC_HTML_LISTSTEP;
	if (PartsListEnd)
		HTMLOptions |= LC_HTML_LISTEND;
	if (PartsListImages)
		HTMLOptions |= LC_HTML_IMAGES;

	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, TransparentImages ? LC_IMAGE_TRANSPARENT : 0);
	lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, StepImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, StepImagesHeight);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_COLOR, lcGetColorCode(PartImagesColor));
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH, PartImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT, PartImagesHeight);
}

Project::Project()
{
	mModified = false;
	mActiveModel = new lcModel(tr("New Model.ldr"));
	mActiveModel->CreatePieceInfo(this);
	mActiveModel->SetSaved();
	mModels.Add(mActiveModel);

	QObject::connect(&mFileWatcher, SIGNAL(fileChanged(const QString&)), gMainWindow, SLOT(ProjectFileChanged(const QString&)));
}

Project::~Project()
{
	mModels.DeleteAll();
}

lcModel* Project::GetModel(const QString& Name) const
{
	for (lcModel* Model : mModels)
		if (Model->GetProperties().mName == Name)
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

QString Project::GetImageFileName() const
{
	QString FileName = QDir::toNativeSeparators(GetFileName());

	if (!FileName.isEmpty())
	{
		QString Extension = QFileInfo(FileName).suffix();
		if (!Extension.isEmpty())
			FileName = FileName.left(FileName.length() - Extension.length() - 1);
	}
	else
		FileName = QLatin1String("image");

	return FileName + lcGetProfileString(LC_PROFILE_IMAGE_EXTENSION);
}

void Project::SetActiveModel(int ModelIndex)
{
	if (ModelIndex < 0 || ModelIndex >= mModels.GetSize())
		return;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->SetActive(ModelIdx == ModelIndex);

	lcArray<lcModel*> UpdatedModels;
	UpdatedModels.AllocGrow(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->UpdatePieceInfo(UpdatedModels);

	mActiveModel = mModels[ModelIndex];
	gMainWindow->SetCurrentModelTab(mActiveModel);
	mActiveModel->UpdateInterface();
}

void Project::SetActiveModel(const QString& ModelName)
{
	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		if (ModelName.compare(mModels[ModelIdx]->GetName(), Qt::CaseInsensitive) == 0)
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
			const QString& Name = ExistingModels[ModelIdx];

			if (Name.startsWith(Prefix))
			{
				QString NumberString = Name.mid(Prefix.length());
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
		ModelNames.append(mModels[ModelIdx]->GetProperties().mName);

	QString Name = GetNewModelName(gMainWindow, tr("New Submodel"), QString(), ModelNames);

	if (Name.isEmpty())
		return nullptr;

	mModified = true;
	lcModel* Model = new lcModel(Name);
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
	QList<QPair<QString, lcModel*>> Models;
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	Models.reserve(mModels.GetSize());
#endif

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];
		Models.append(QPair<QString, lcModel*>(Model->GetProperties().mName, Model));
	}

	lcQModelListDialog Dialog(gMainWindow, Models);

	if (Dialog.exec() != QDialog::Accepted || Models.isEmpty())
		return;

	lcArray<lcModel*> NewModels;

	for (QList<QPair<QString, lcModel*>>::iterator it = Models.begin(); it != Models.end(); it++)
	{
		lcModel* Model = it->second;

		if (!Model)
		{
			Model = new lcModel(it->first);
			Model->CreatePieceInfo(this);
			Model->SetSaved();
			mModified = true;
		}
		else if (Model->GetProperties().mName != it->first)
		{
			Model->SetName(it->first);
			lcGetPiecesLibrary()->RenamePiece(Model->GetPieceInfo(), it->first.toLatin1().constData());

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

	SetActiveModel(Dialog.mActiveModel);
	gMainWindow->UpdateTitle();
}

void Project::SetFileName(const QString& FileName)
{
	if (!mFileName.isEmpty())
		mFileWatcher.removePath(mFileName);

	if (!FileName.isEmpty())
		mFileWatcher.addPath(FileName);

	mFileName = FileName;
}

bool Project::Load(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
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
		LoadDAT = memcmp(FileData, "LeoCAD ", 7);

	if (LoadDAT)
	{
		QBuffer Buffer(&FileData);
		Buffer.open(QIODevice::ReadOnly);

		while (!Buffer.atEnd())
		{
			lcModel* Model = new lcModel(QString());
			Model->SplitMPD(Buffer);

			if (mModels.IsEmpty() || !Model->GetProperties().mName.isEmpty())
			{
				mModels.Add(Model);
				Model->CreatePieceInfo(this);
			}
			else
				delete Model;
		}

		Buffer.seek(0);

		for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		{
			lcModel* Model = mModels[ModelIdx];
			Model->LoadLDraw(Buffer, this);
			Model->SetSaved();
		}
	}
	else
	{
		lcMemFile MemFile;
		MemFile.WriteBuffer(FileData.constData(), FileData.size());
		MemFile.Seek(0, SEEK_SET);

		lcModel* Model = new lcModel(QString());

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
		return false;

	if (mModels.GetSize() == 1)
	{
		lcModel* Model = mModels[0];

		if (Model->GetProperties().mName.isEmpty())
		{
			Model->SetName(FileInfo.fileName());
			lcGetPiecesLibrary()->RenamePiece(Model->GetPieceInfo(), FileInfo.fileName().toLatin1());
		}
	}

	lcArray<lcModel*> UpdatedModels;
	UpdatedModels.AllocGrow(mModels.GetSize());

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

	SetFileName(FileName);
	mModified = false;

	return Success;
}

bool Project::Save(QTextStream& Stream)
{
	bool MPD = mModels.GetSize() > 1;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];

		if (MPD)
			Stream << QLatin1String("0 FILE ") << Model->GetProperties().mName << QLatin1String("\r\n");

		Model->SaveLDraw(Stream, false);
		Model->SetSaved();

		if (MPD)
			Stream << QLatin1String("0 NOFILE\r\n");
	}

	return true;
}

void Project::Merge(Project* Other)
{
	for (int ModelIdx = 0; ModelIdx < Other->mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = Other->mModels[ModelIdx];
		QString Name = Model->GetProperties().mName;

		for (;;)
		{
			bool Duplicate = false;

			for (int SearchIdx = 0; SearchIdx < mModels.GetSize(); SearchIdx++)
			{
				if (mModels[SearchIdx]->GetProperties().mName == Name)
				{
					Duplicate = true;
					break;
				}
			}

			if (!Duplicate)
				break;

			Name = tr("Merged ") + Name;
			Model->SetName(Name);
		}

		mModels.Add(Model);
	}

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
	lcModel* Model = new lcModel(QString());

	if (Model->LoadLDD(QString::fromUtf8((const char*)XMLFile.mBuffer)))
	{
		mModels.Add(Model);
		Model->SetSaved();
	}
	else
		delete Model;

	if (mModels.IsEmpty())
		return false;

	if (mModels.GetSize() == 1)
	{
		lcModel* Model = mModels[0];

		if (Model->GetProperties().mName.isEmpty())
			Model->SetName(QFileInfo(FileName).completeBaseName());
	}

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CreatePieceInfo(this);

	lcArray<lcModel*> UpdatedModels;
	UpdatedModels.AllocGrow(mModels.GetSize());

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
	lcModel* Model = new lcModel(QString());

	if (Model->LoadInventory(Inventory))
	{
		mModels.Add(Model);
		Model->SetSaved();
	}
	else
		delete Model;

	if (mModels.IsEmpty())
		return false;

	if (mModels.GetSize() == 1)
	{
		lcModel* Model = mModels[0];

		Model->SetName(Name);
		Model->SetDescription(Description);
	}

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CreatePieceInfo(this);

	lcArray<lcModel*> UpdatedModels;
	UpdatedModels.AllocGrow(mModels.GetSize());

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->UpdatePieceInfo(UpdatedModels);

	mModified = false;

	return true;
}

void Project::GetModelParts(lcArray<lcModelPartsEntry>& ModelParts)
{
	if (mModels.IsEmpty())
		return;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CalculateStep(LC_STEP_MAX);

	mModels[0]->GetModelParts(lcMatrix44Identity(), gDefaultColor, ModelParts);

	SetActiveModel(mModels.FindIndex(mActiveModel));
}

bool Project::ExportModel(const QString& FileName, lcModel* Model)
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
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
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

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		sprintf(MaterialName, "Material%03d", ColorIdx);

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

	File.WriteFloats(Properties.mBackgroundSolidColor, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mBackgroundSolidColor, 3);

	File.WriteU16(0x1100); // CHK_BIT_MAP
	QByteArray BackgroundImage = Properties.mBackgroundImage.toLatin1();
	File.WriteU32(6 + 1 + (quint32)strlen(BackgroundImage.constData()));
	File.WriteBuffer(BackgroundImage.constData(), strlen(BackgroundImage.constData()) + 1);

	File.WriteU16(0x1300); // CHK_V_GRADIENT
	File.WriteU32(118);

	File.WriteFloat(1.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mBackgroundGradientColor1, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mBackgroundGradientColor1, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((Properties.mBackgroundGradientColor1 + Properties.mBackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((Properties.mBackgroundGradientColor1 + Properties.mBackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mBackgroundGradientColor2, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mBackgroundGradientColor2, 3);

	if (Properties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		File.WriteU16(0x1301); // LIB3DS_USE_V_GRADIENT
		File.WriteU32(6);
	}
	else if (Properties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		File.WriteU16(0x1101); // LIB3DS_USE_BIT_MAP
		File.WriteU32(6);
	}
	else
	{
		File.WriteU16(0x1201); // LIB3DS_USE_SOLID_BGND
		File.WriteU32(6);
	}

	int NumPieces = 0;
	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		PieceInfo* Info = ModelParts[PartIdx].Info;
		lcMesh* Mesh = Info->GetMesh();

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
		const lcMatrix44& ModelWorld = ModelParts[PartIdx].WorldMatrix;

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

			int MaterialIndex = Section->ColorIndex == gDefaultColor ? ModelParts[PartIdx].ColorIndex : Section->ColorIndex;

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
		mModels[0]->GetPartsList(gDefaultColor, true, PartsList);

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

			int Count = ColorIt.second;
			if (Count > 1)
			{
				sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Count);
				BrickLinkFile.WriteLine(Line);
			}

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
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
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
	QSet<PieceInfo*> AddedPieces;

	for (const lcModelPartsEntry& Entry : ModelParts)
	{
		PieceInfo* Info = Entry.Info;

		if (AddedPieces.contains(Info))
			continue;
		AddedPieces.insert(Info);

		QString ID = QString(Info->mFileName).replace('.', '_');
		lcMesh* Mesh = Info->GetMesh();

		if (!Mesh)
			Mesh = gPlaceholderMesh;

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

	for (const lcModelPartsEntry& Entry : ModelParts)
	{
		PieceInfo* Info = Entry.Info;
		QString ID = QString(Info->mFileName).replace('.', '_');

		Stream << "\t\t<node>\r\n";
		Stream << "\t\t\t<matrix>\r\n";

		const lcMatrix44& Matrix = Entry.WorldMatrix;
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][0]), QString::number(Matrix[1][0]), QString::number(Matrix[2][0]), QString::number(Matrix[3][0]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][1]), QString::number(Matrix[1][1]), QString::number(Matrix[2][1]), QString::number(Matrix[3][1]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][2]), QString::number(Matrix[1][2]), QString::number(Matrix[2][2]), QString::number(Matrix[3][2]));
		Stream << QString("\t\t\t\t%1 %2 %3 %4\r\n").arg(QString::number(Matrix[0][3]), QString::number(Matrix[1][3]), QString::number(Matrix[2][3]), QString::number(Matrix[3][3]));

		Stream << "\t\t\t</matrix>\r\n";
		Stream << QString("\t\t\t<instance_geometry url=\"#%1\">\r\n").arg(ID);
		Stream << "\t\t\t\t<bind_material>\r\n";
		Stream << "\t\t\t\t\t<technique_common>\r\n";

		lcMesh* Mesh = Info->GetMesh();

		if (!Mesh)
			Mesh = gPlaceholderMesh;

		for (int SectionIdx = 0; SectionIdx < Mesh->mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

			if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
				continue;

			const char* SourceColorName = gColorList[Section->ColorIndex].SafeName;
			const char* TargetColorName;
			if (Section->ColorIndex == gDefaultColor)
				TargetColorName = gColorList[Entry.ColorIndex].SafeName;
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
		mModels[0]->GetPartsList(gDefaultColor, true, PartsList);

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

QImage Project::CreatePartsListImage(lcModel* Model, lcStep Step)
{
	lcPartsList PartsList;
	if (Step == 0)
		Model->GetPartsList(gDefaultColor, true, PartsList);
	else
		Model->GetPartsListForStep(Step, gDefaultColor, PartsList);

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

	auto ImageCompare = [this](const lcPartsListImage& Image1, const lcPartsListImage& Image2)
	{
		if (Image1.ColorIndex != Image2.ColorIndex)
			return Image1.ColorIndex < Image2.ColorIndex;

		return strcmp(Image1.Info->m_strDescription, Image2.Info->m_strDescription) < 0;
	};

	std::sort(Images.begin(), Images.end(), ImageCompare);

	View* View = gMainWindow->GetActiveView();
	View->MakeCurrent();
	lcContext* Context = View->mContext;
	const int ThumbnailWidth = 512;
	const int ThumbnailHeight = 512;

	std::pair<lcFramebuffer, lcFramebuffer> RenderFramebuffer = Context->CreateRenderFramebuffer(ThumbnailWidth, ThumbnailHeight);

	if (!RenderFramebuffer.first.IsValid())
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
		return QImage();
	}

	Context->BindFramebuffer(RenderFramebuffer.first);

	float Aspect = (float)ThumbnailWidth / (float)ThumbnailHeight;
	float OrthoHeight = 200.0f;
	float OrthoWidth = OrthoHeight * Aspect;

	lcMatrix44 ProjectionMatrix = lcMatrix44Ortho(-OrthoWidth, OrthoWidth, -OrthoHeight, OrthoHeight, 1.0f, 50000.0f);
	lcMatrix44 ViewMatrix = lcMatrix44LookAt(lcVector3(-100.0f, -100.0f, 75.0f), lcVector3(0.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f));

	Context->SetViewport(0, 0, ThumbnailWidth, ThumbnailHeight);
	Context->SetDefaultState();
	Context->SetProjectionMatrix(ProjectionMatrix);

	for (lcPartsListImage& Image : Images)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lcScene Scene;
		Scene.Begin(ViewMatrix);

		Image.Info->AddRenderMeshes(Scene, lcMatrix44Identity(), Image.ColorIndex, lcRenderMeshState::NORMAL, true);

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
	int MaxWidth = 1024;

	for (lcPartsListImage& Image : Images)
	{
		CurrentHeight = qMax(Image.Bounds.height() + Ascent, CurrentHeight);
		MaxWidth = qMax(MaxWidth, Image.Bounds.width());
	}

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
			MaxWidth = CurrentWidth;
			break;
		}

		CurrentHeight += NextHeightIncrease;
	}

	QImage Image(MaxWidth + 40, CurrentHeight + 40, QImage::Format_ARGB32);
	Image.fill(QColor(255, 255, 255, 0));

	QPainter Painter(&Image);
	Painter.setFont(Font);

	for (lcPartsListImage& Image : Images)
	{
		QPoint Position = Image.Position + QPoint(20, 20);
		Painter.drawImage(Position, Image.Thumbnail, Image.Bounds);
		Painter.drawText(QPoint(Position.x(), Position.y() + Image.Bounds.height() + Ascent), QString::number(Image.Count) + 'x');
	}

	Painter.end();

	return Image;
}

void Project::CreateHTMLPieceList(QTextStream& Stream, lcModel* Model, lcStep Step, bool Images)
{
	std::vector<int> ColorsUsed(gColorList.GetSize(), 0);
	int NumColors = 0;

	lcPartsList PartsList;

	if (Step == 0)
		Model->GetPartsList(gDefaultColor, true, PartsList);
	else
		Model->GetPartsListForStep(Step, gDefaultColor, PartsList);

	for (const auto& PartIt : PartsList)
		for (const auto& ColorIt : PartIt.second)
			ColorsUsed[ColorIt.first]++;

	Stream << QLatin1String("<br><table border=1><tr><td><center>Piece</center></td>\r\n");

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		if (ColorsUsed[ColorIdx])
		{
			ColorsUsed[ColorIdx] = NumColors++;
			Stream << QString("<td><center>%1</center></td>\r\n").arg(gColorList[ColorIdx].Name);
		}
	}
	NumColors++;
	Stream << QLatin1String("</tr>\r\n");

	for (const auto& PartIt : PartsList)
	{
		const PieceInfo* Info = PartIt.first;

		if (Images)
			Stream << QString("<tr><td><IMG SRC=\"%1.png\" ALT=\"%2\"></td>\r\n").arg(Info->mFileName, Info->m_strDescription);
		else
			Stream << QString("<tr><td>%1</td>\r\n").arg(Info->m_strDescription);

		int CurrentColumn = 1;
		for (const auto& ColorIt : PartIt.second)
		{
			while (CurrentColumn != ColorsUsed[ColorIt.first] + 1)
			{
				Stream << QLatin1String("<td><center>-</center></td>\r\n");
				CurrentColumn++;
			}

			Stream << QString("<td><center>%1</center></td>\r\n").arg(QString::number(ColorIt.second));
			CurrentColumn++;
		}

		while (CurrentColumn != NumColors)
		{
			Stream << QLatin1String("<td><center>-</center></td>\r\n");
			CurrentColumn++;
		}

		Stream << QLatin1String("</tr>\r\n");
	}
	Stream << QLatin1String("</table>\r\n<br>");
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

	auto AddPartsListImage = [this, &Dir](QTextStream& Stream, lcModel* Model, lcStep Step, const QString& BaseName)
	{
		QImage Image = CreatePartsListImage(Model, Step);

		if (!Image.isNull())
		{
			QString ImageName = BaseName + QLatin1String("-parts.png");
			QString FileName = QFileInfo(Dir, ImageName).absoluteFilePath();

			Image.save(FileName);

			Stream << QString::fromLatin1("<IMG SRC=\"%1\" />\r\n").arg(ImageName);
		}
	};

	for (int ModelIdx = 0; ModelIdx < Models.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];
		QString BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1);
		lcStep LastStep = Model->GetLastStep();
		QString PageTitle;

		if (Models.GetSize() > 1)
		{
			BaseName += '-' + Model->GetProperties().mName;
			PageTitle = Model->GetProperties().mName;
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
				Stream << QString::fromLatin1("<IMG SRC=\"%1-%2.png\" ALT=\"Step %3\" WIDTH=%4 HEIGHT=%5><BR><BR>\r\n").arg(BaseName, StepString, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

				if (Options.PartsListStep)
					CreateHTMLPieceList(Stream, Model, Step, Options.PartsListImages);
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
					CreateHTMLPieceList(Stream, Model, Step, Options.PartsListImages);

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
		Model->SaveStepImages(StepImageBaseName, true, false, Options.HighlightNewParts, Options.StepImagesWidth, Options.StepImagesHeight, 1, LastStep);

		if (Options.PartsListImages)
		{
			View* View = gMainWindow->GetActiveView();
			View->MakeCurrent();
			lcContext* Context = View->mContext;
			int Width = Options.PartImagesWidth;
			int Height = Options.PartImagesHeight;

			std::pair<lcFramebuffer, lcFramebuffer> RenderFramebuffer = Context->CreateRenderFramebuffer(Width, Height);

			if (!RenderFramebuffer.first.IsValid())
			{
				QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
				return;
			}

			Context->BindFramebuffer(RenderFramebuffer.first);

			float AspectRatio = (float)Width / (float)Height;
			Context->SetViewport(0, 0, Width, Height);

			lcPartsList PartsList;
			Model->GetPartsList(gDefaultColor, true, PartsList);

			lcMatrix44 ProjectionMatrix, ViewMatrix;

			Context->SetDefaultState();

			for (const auto& PartIt : PartsList)
			{
				const PieceInfo* Info = PartIt.first;

				glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				Info->ZoomExtents(30.0f, AspectRatio, ProjectionMatrix, ViewMatrix);

				Context->SetProjectionMatrix(ProjectionMatrix);

				lcScene Scene;
				Scene.Begin(ViewMatrix);

				Info->AddRenderMeshes(Scene, lcMatrix44Identity(), Options.PartImagesColor, lcRenderMeshState::NORMAL, true);

				Scene.End();

				Scene.Draw(Context);

				QString FileName = QFileInfo(Dir, QLatin1String(Info->mFileName) + QLatin1String(".png")).absoluteFilePath();
				QImage Image = Context->GetRenderFramebufferImage(RenderFramebuffer);

				QImageWriter Writer(FileName);

				if (!Writer.write(Image))
				{
					QMessageBox::information(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, Writer.errorString()));
					break;
				}
			}

			Context->ClearFramebuffer();
			Context->DestroyRenderFramebuffer(RenderFramebuffer);
			Context->ClearResources();
		}
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
			BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1) + '-' + Model->GetProperties().mName;
			BaseName.replace('#', '_');

			if (Options.SinglePage)
			{
				FileName = BaseName + QLatin1String(".html");
				Stream << QString::fromLatin1("<p><a href=\"%1\">%2</a>").arg(FileName, Model->GetProperties().mName);
			}
			else if (Options.IndexPage)
			{
				FileName = BaseName + QLatin1String("-index.html");
				Stream << QString::fromLatin1("<p><a href=\"%1\">%2</a>").arg(FileName, Model->GetProperties().mName);
			}
			else
			{
				lcStep LastStep = Model->GetLastStep();

				Stream << QString::fromLatin1("<p>%1</p>").arg(Model->GetProperties().mName);

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
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
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
	std::map<PieceInfo*, std::pair<char[LC_PIECE_NAME_LEN], int>> PieceTable;
	int NumColors = gColorList.GetSize();
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
			char Src[1024], Dst[1024], Flags[1024];

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%s%s%s", Src, Dst, Flags) != 3)
				continue;

			strcat(Src, ".dat");

			PieceInfo* Info = Library->FindPiece(Src, nullptr, false, false);
			if (!Info)
				continue;

			if (strchr(Flags, 'L'))
			{
				std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_LGEO;
				sprintf(Entry.first, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = PieceTable[Info];
				Entry.second |= LGEO_PIECE_AR;
				sprintf(Entry.first, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
			{
				std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = PieceTable[Info];
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

			int Color = lcGetColorIndex(Code);
			if (Color >= NumColors)
				continue;

			strcpy(ColorTable[Color].data(), Name);
		}
	}

	if (!LGEOPath.isEmpty())
	{
		POVFile.WriteLine("#include \"lg_defs.inc\"\n#include \"lg_color.inc\"\n\n");

		for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
		{
			PieceInfo* Info = ModelParts[PartIdx].Info;

			for (int CheckIdx = 0; CheckIdx < ModelParts.GetSize(); CheckIdx++)
			{
				if (ModelParts[CheckIdx].Info != Info)
					continue;

				if (CheckIdx != PartIdx)
					break;

				auto Search = PieceTable.find(Info);

				if (Search != PieceTable.end())
				{
					const std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = Search->second;
					if (Entry.first[0])
					{
						sprintf(Line, "#include \"%s.inc\"\n", Entry.first);
						POVFile.WriteLine(Line);
					}
				}

				break;
			}
		}

		POVFile.WriteLine("\n");
	}

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
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

	lcArray<const char*> ColorTablePointer;
	ColorTablePointer.SetSize(NumColors);
	for (int ColorIdx = 0; ColorIdx < NumColors; ColorIdx++)
		ColorTablePointer[ColorIdx] = ColorTable[ColorIdx].data();

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		PieceInfo* Info = ModelParts[PartIdx].Info;
		lcMesh* Mesh = Info->GetMesh();
		std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = PieceTable[Info];

		if (!Mesh || Entry.first[0])
			continue;

		char Name[LC_PIECE_NAME_LEN];
		char* Ptr;

		strcpy(Name, Info->mFileName);
		while ((Ptr = strchr(Name, '-')))
			*Ptr = '_';
		while ((Ptr = strchr(Name, '.')))
			*Ptr = '_';

		sprintf(Entry.first, "lc_%s", Name);

		Mesh->ExportPOVRay(POVFile, Name, &ColorTablePointer[0]);

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	lcCamera* Camera = gMainWindow->GetActiveView()->mCamera;
	const lcVector3& Position = Camera->mPosition;
	const lcVector3& Target = Camera->mTargetPosition;
	const lcVector3& Up = Camera->mUpVector;
	const lcModelProperties& Properties = mModels[0]->GetProperties();

	sprintf(Line, "camera {\n  perspective\n  right x * image_width / image_height\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f * image_width / image_height\n}\n\n",
			Up[1], Up[0], Up[2], Position[1] / 25.0f, Position[0] / 25.0f, Position[2] / 25.0f, Target[1] / 25.0f, Target[0] / 25.0f, Target[2] / 25.0f, Camera->m_fovy);
	POVFile.WriteLine(Line);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\n", Properties.mBackgroundSolidColor[0], Properties.mBackgroundSolidColor[1], Properties.mBackgroundSolidColor[2]);
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

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		std::pair<char[LC_PIECE_NAME_LEN], int>& Entry = PieceTable[ModelParts[PartIdx].Info];
		int Color;

		Color = ModelParts[PartIdx].ColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = ModelParts[PartIdx].WorldMatrix;

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
			sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					Entry.first, Suffix, ColorTable[Color].data(), -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}

		POVFile.WriteLine(Line);
	}

	POVFile.Close();

	return true;
}

void Project::ExportWavefront(const QString& FileName)
{
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
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
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];
		if (Color->Translucent)
			sprintf(Line, "newmtl %s\nKd %.2f %.2f %.2f\nD %.2f\n\n", Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2], Color->Value[3]);
		else
			sprintf(Line, "newmtl %s\nKd %.2f %.2f %.2f\n\n", Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		MaterialFile.WriteLine(Line);
	}

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		lcMesh* Mesh = ModelParts[PartIdx].Info->GetMesh();

		if (!Mesh)
			continue;

		const lcMatrix44& ModelWorld = ModelParts[PartIdx].WorldMatrix;
		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Vertex = lcMul31(Verts[VertexIdx].Position, ModelWorld);
			sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
			OBJFile.WriteLine(Line);
		}

		OBJFile.WriteLine("#\n\n");
	}

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		lcMesh* Mesh = ModelParts[PartIdx].Info->GetMesh();

		if (!Mesh)
			continue;

		const lcMatrix44& ModelWorld = ModelParts[PartIdx].WorldMatrix;
		lcVertex* Verts = (lcVertex*)Mesh->mVertexData;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Normal = lcMul30(lcUnpackNormal(Verts[VertexIdx].Normal), ModelWorld);
			sprintf(Line, "vn %.2f %.2f %.2f\n", Normal[0], Normal[1], Normal[2]);
			OBJFile.WriteLine(Line);
		}

		OBJFile.WriteLine("#\n\n");
	}

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		PieceInfo* Info = ModelParts[PartIdx].Info;

		sprintf(Line, "g Piece%.3d\n", PartIdx);
		OBJFile.WriteLine(Line);

		lcMesh* Mesh = Info->GetMesh();

		if (Mesh)
		{
			Mesh->ExportWavefrontIndices(OBJFile, ModelParts[PartIdx].ColorIndex, vert);
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

	mActiveModel->SaveStepImages(Dialog.mFileName, Dialog.mStart != Dialog.mEnd, false, false, Dialog.mWidth, Dialog.mHeight, Dialog.mStart, Dialog.mEnd);
}

void Project::UpdatePieceInfo(PieceInfo* Info) const
{
	if (!mModels.IsEmpty())
	{
		lcArray<lcModel*> UpdatedModels;
		mModels[0]->UpdatePieceInfo(UpdatedModels);

		lcBoundingBox BoundingBox = mModels[0]->GetPieceInfo()->GetBoundingBox();
		Info->SetBoundingBox(BoundingBox.Min, BoundingBox.Max);
	}
}
