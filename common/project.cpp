#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <locale.h>
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
#include "lc_qimagedialog.h"
#include "lc_qmodellistdialog.h"
#include "lc_qpovraydialog.h"

Project::Project()
{
	mModified = false;
	mActiveModel = new lcModel(tr("Model #1.ldr"));
	mActiveModel->CreatePieceInfo(this);
	mActiveModel->SetSaved();
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
	if (!mFileName.isEmpty())
		return QFileInfo(mFileName).fileName();

	return mModels.GetSize() == 1 ? tr("New Project.ldr") : tr("New Project.mpd");
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

QString Project::GetNewModelName(QWidget* ParentWidget, const QString& DialogTitle, const QString& CurrentName, const QStringList& ExistingModels) const
{
	QString Name = CurrentName;

	if (Name.isEmpty())
	{
		const QString Prefix = tr("Model #");
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

		Name = QInputDialog::getText(ParentWidget, DialogTitle, tr("Model Name:"), QLineEdit::Normal, Name, &Ok);

		if (!Ok)
			return QString();

		if (Name.isEmpty())
		{
			QMessageBox::information(ParentWidget, tr("Empty Name"), tr("The model name cannot be empty."));
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
		{
			QMessageBox::information(ParentWidget, tr("Invalid Extension"), tr("The model name must end with '.ldr', '.dat' or '.mpd'."));
			continue;
		}

		if (ExistingModels.contains(Name, Qt::CaseInsensitive) && Name != CurrentName)
		{
			QMessageBox::information(ParentWidget, tr("Duplicate Model"), tr("A model named '%1' already exists, please enter an unique name.").arg(Name));
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

	QString Name = GetNewModelName(gMainWindow, tr("New Model"), QString(), ModelNames);

	if (Name.isEmpty())
		return NULL;

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
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error reading file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	mModels.DeleteAll();
	mFileName = FileName;
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
				mModels.Add(Model);
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
			Model->SetName(FileInfo.fileName());
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

bool Project::Save(const QString& FileName)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
		return false;
	}

	QTextStream Stream(&File);
	bool Success = Save(Stream);

	mFileName = FileName;
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

void Project::GetModelParts(lcArray<lcModelPartsEntry>& ModelParts)
{
	if (mModels.IsEmpty())
		return;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
		mModels[ModelIdx]->CalculateStep(LC_STEP_MAX);

	mModels[0]->GetModelParts(lcMatrix44Identity(), gDefaultColor, ModelParts);

	SetActiveModel(mModels.FindIndex(mActiveModel));
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

	lcDiskFile File;

	if (!File.Open(SaveFileName, "wb"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(FileName));
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

		File.WriteS16((lcuint8)floor(100.0 * 0.25 + 0.5));

		File.WriteU16(0xA041); // CHK_MAT_SHIN2PCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.05 + 0.5));

		File.WriteU16(0xA050); // CHK_MAT_TRANSPARENCY
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * (1.0f - Color->Value[3]) + 0.5));

		File.WriteU16(0xA052); // CHK_MAT_XPFALL
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA053); // CHK_MAT_REFBLUR
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.2 + 0.5));

		File.WriteU16(0xA100); // CHK_MAT_SHADING
		File.WriteU32(8);

		File.WriteS16(3);

		File.WriteU16(0xA084); // CHK_MAT_SELF_ILPCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

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
	File.WriteU32(6 + 1 + (lcuint32)strlen(BackgroundImage.constData()));
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

	File.WriteU16(0x2200); // CHK_FOG
	File.WriteU32(46);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mFogColor, 3);

	File.WriteU16(0x2210); // CHK_FOG_BGND
	File.WriteU32(6);

	File.WriteU16(0x2302); // CHK_LAYER_FOG
	File.WriteU32(40);

	File.WriteFloat(0.0f);
	File.WriteFloat(100.0f);
	File.WriteFloat(50.0f);
	File.WriteU32(0x00100000);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(Properties.mFogColor, 3);

	File.WriteU16(0x2300); // CHK_DISTANCE_CUE
	File.WriteU32(28);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x2310); // CHK_DICHK_DCUE_BGNDSTANCE_CUE
	File.WriteU32(6);

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

		float* Verts = (float*)Mesh->mVertexData;
		const lcMatrix44& ModelWorld = ModelParts[PartIdx].WorldMatrix;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Pos(Verts[VertexIdx * 3], Verts[VertexIdx * 3 + 1], Verts[VertexIdx * 3 + 2]);
			Pos = lcMul31(Pos, ModelWorld);
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

			lcuint16* Indices = (lcuint16*)Mesh->mIndexData + Section->IndexOffset / sizeof(lcuint16);

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
		mModels[0]->GetPartsList(gDefaultColor, PartsList);

	if (PartsList.isEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(QString(), "xml", tr("Export BrickLink"), tr("XML Files (*.xml);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile BrickLinkFile;
	char Line[1024];

	if (!BrickLinkFile.Open(SaveFileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	BrickLinkFile.WriteLine("<INVENTORY>\n");

	for (lcPartsList::const_iterator PartIt = PartsList.constBegin(); PartIt != PartsList.constEnd(); PartIt++)
	{
		const PieceInfo* Info = PartIt.key();

		for (QMap<int, int>::const_iterator ColorIt = PartIt.value().constBegin(); ColorIt != PartIt.value().constEnd(); ColorIt++)
		{
			BrickLinkFile.WriteLine("  <ITEM>\n");
			BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");

			sprintf(Line, "    <ITEMID>%s</ITEMID>\n", Info->m_strName);
			BrickLinkFile.WriteLine(Line);

			int Count = ColorIt.value();
			if (Count > 1)
			{
				sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Count);
				BrickLinkFile.WriteLine(Line);
			}

			int Color = lcGetBrickLinkColor(ColorIt.key());
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

void Project::ExportCSV()
{
	lcPartsList PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, PartsList);

	if (PartsList.isEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString SaveFileName = GetExportFileName(QString(), "csv", tr("Export CSV"), tr("CSV Files (*.csv);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile CSVFile;
	char Line[1024];

	if (!CSVFile.Open(SaveFileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

	for (lcPartsList::const_iterator PartIt = PartsList.constBegin(); PartIt != PartsList.constEnd(); PartIt++)
	{
		const PieceInfo* Info = PartIt.key();

		for (QMap<int, int>::const_iterator ColorIt = PartIt.value().constBegin(); ColorIt != PartIt.value().constEnd(); ColorIt++)
		{
			sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", Info->m_strDescription, gColorList[ColorIt.key()].Name,
					ColorIt.value(), Info->m_strName, gColorList[ColorIt.key()].Code);
			CSVFile.WriteLine(Line);
		}
	}
}

void Project::CreateHTMLPieceList(QTextStream& Stream, lcModel* Model, lcStep Step, bool Images)
{
	QVector<int> ColorsUsed(gColorList.GetSize());
	int NumColors = 0;

	lcPartsList PartsList;

	if (Step == 0)
		Model->GetPartsList(gDefaultColor, PartsList);
	else
		Model->GetPartsListForStep(Step, gDefaultColor, PartsList);

	for (lcPartsList::const_iterator PartIt = PartsList.constBegin(); PartIt != PartsList.constEnd(); PartIt++)
		for (QMap<int, int>::const_iterator ColorIt = PartIt.value().constBegin(); ColorIt != PartIt.value().constEnd(); ColorIt++)
			ColorsUsed[ColorIt.key()]++;

	Stream << QLatin1String("<br><table border=1><tr><td><center>Piece</center></td>\r\n");

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		if (ColorsUsed[ColorIdx])
		{
			ColorsUsed[ColorIdx] = NumColors++;
			Stream << QString("<td><center>%1</center></td>\n").arg(gColorList[ColorIdx].Name);
		}
	}
	NumColors++;
	Stream << QLatin1String("</tr>\n");

	for (lcPartsList::const_iterator PartIt = PartsList.constBegin(); PartIt != PartsList.constEnd(); PartIt++)
	{
		const PieceInfo* Info = PartIt.key();

		if (Images)
			Stream << QString("<tr><td><IMG SRC=\"%1.png\" ALT=\"%2\"></td>\n").arg(Info->m_strName, Info->m_strDescription);
		else
			Stream << QString("<tr><td>%1</td>\r\n").arg(Info->m_strDescription);

		int CurrentColumn = 1;
		for (QMap<int, int>::const_iterator ColorIt = PartIt.value().constBegin(); ColorIt != PartIt.value().constEnd(); ColorIt++)
		{
			while (CurrentColumn != ColorsUsed[ColorIt.key()] + 1)
			{
				Stream << QLatin1String("<td><center>-</center></td>\r\n");
				CurrentColumn++;
			}

			Stream << QString("<td><center>%1</center></td>\r\n").arg(QString::number(ColorIt.value()));
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

void Project::ExportHTML()
{
	lcHTMLDialogOptions Options;

	if (!mFileName.isEmpty())
		Options.PathName = QFileInfo(mFileName).canonicalPath();

	int ImageOptions = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS);
	int HTMLOptions = lcGetProfileInt(LC_PROFILE_HTML_OPTIONS);

	Options.TransparentImages = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
	Options.SubModels = (HTMLOptions & (LC_HTML_SUBMODELS)) != 0;
	Options.CurrentOnly = (HTMLOptions & LC_HTML_CURRENT_ONLY) != 0;
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

	if (Options.SubModels)
		HTMLOptions |= LC_HTML_SUBMODELS;
	if (Options.CurrentOnly)
		HTMLOptions |= LC_HTML_CURRENT_ONLY;
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

	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, Options.TransparentImages ? LC_IMAGE_TRANSPARENT : 0);
	lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, Options.StepImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, Options.StepImagesHeight);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_COLOR, lcGetColorCode(Options.PartImagesColor));
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH, Options.PartImagesWidth);
	lcSetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT, Options.PartImagesHeight);

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
				CreateHTMLPieceList(Stream, Model, 0, Options.PartsListImages);

			Stream << QLatin1String("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
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

				Stream << QLatin1String("</CENTER>\r\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
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

				CreateHTMLPieceList(Stream, Model, 0, Options.PartsListImages);

				Stream << QLatin1String("</CENTER>\n<BR><HR><BR>");
				Stream << QString::fromLatin1("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(LastStep, 2, 10, QLatin1Char('0')));

				if (Options.IndexPage)
					Stream << QString::fromLatin1("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

				Stream << QLatin1String("<BR></HTML>\r\n");
			}
		}

		QString StepImageBaseName = QFileInfo(Dir, BaseName + QLatin1String("-%1.png")).absoluteFilePath();
		Model->SaveStepImages(StepImageBaseName, true, false, Options.StepImagesWidth, Options.StepImagesHeight, 1, LastStep);

		if (Options.PartsListImages)
		{
			View* View = gMainWindow->GetActiveView();
			View->MakeCurrent();
			lcContext* Context = View->mContext;
			int Width = Options.PartImagesWidth;
			int Height = Options.PartImagesHeight;

			if (!Context->BeginRenderToTexture(Width, Height))
			{
				QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
				return;
			}

			float aspect = (float)Width / (float)Height;
			Context->SetViewport(0, 0, Width, Height);

			lcPartsList PartsList;
			Model->GetPartsList(gDefaultColor, PartsList);

			lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(30.0f, aspect, 1.0f, 2500.0f);
			lcMatrix44 ViewMatrix;

			Context->SetDefaultState();
			Context->SetProjectionMatrix(ProjectionMatrix);
			Context->SetProgram(LC_PROGRAM_SIMPLE);

			for (lcPartsList::const_iterator PartIt = PartsList.constBegin(); PartIt != PartsList.constEnd(); PartIt++)
			{
				const PieceInfo* Info = PartIt.key();

				glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				lcVector3 CameraPosition(-100.0f, -100.0f, 75.0f);
				Info->ZoomExtents(ProjectionMatrix, ViewMatrix, CameraPosition);

				lcScene Scene;
				Scene.Begin(ViewMatrix);

				Info->AddRenderMeshes(Scene, lcMatrix44Identity(), Options.PartImagesColor, false, false);

				Scene.End();

				Context->SetViewMatrix(ViewMatrix);
				Context->DrawOpaqueMeshes(Scene.mOpaqueMeshes);
				Context->DrawTranslucentMeshes(Scene.mTranslucentMeshes);

				Context->UnbindMesh(); // context remove

				QString FileName = QFileInfo(Dir, QLatin1String(Info->m_strName) + QLatin1String(".png")).absoluteFilePath();
				if (!Context->SaveRenderToTextureImage(FileName, Width, Height))
					break;
			}
			Context->EndRenderToTexture();
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
			QString BaseName = ProjectTitle.left(ProjectTitle.length() - QFileInfo(ProjectTitle).suffix().length() - 1) + '-' + Model->GetProperties().mName;
			BaseName.replace('#', '_');

			if (Options.SinglePage)
				FileName = BaseName + QLatin1String(".html");
			else
				FileName = BaseName + QLatin1String("-index.html");

			Stream << QString::fromLatin1("<p><a href=\"%1\">%2</a>").arg(FileName, Model->GetProperties().mName);
		}

		Stream << QLatin1String("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\r\n");
	}
}

void Project::ExportPOVRay()
{
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	lcQPOVRayDialog Dialog(gMainWindow);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	lcDiskFile POVFile;

	if (!POVFile.Open(Dialog.mFileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(Dialog.mFileName));
		return;
	}

	char Line[1024];

	struct lcPieceTableEntry
	{
		char Name[LC_PIECE_NAME_LEN];
		int Flags;
	};

	struct lcColorTableEntry
	{
		char Name[LC_MAX_COLOR_NAME];
	};

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	QMap<PieceInfo*, lcPieceTableEntry> PieceTable;
	int NumColors = gColorList.GetSize();
	QVector<lcColorTableEntry> ColorTable(NumColors);

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

	if (!Dialog.mLGEOPath.isEmpty())
	{
		lcDiskFile TableFile, ColorFile;

		if (!TableFile.Open(QFileInfo(QDir(Dialog.mLGEOPath), QLatin1String("lg_elements.lst")).absoluteFilePath(), "rt"))
		{
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(Dialog.mLGEOPath));
			return;
		}

		while (TableFile.ReadLine(Line, sizeof(Line)))
		{
			char Src[1024], Dst[1024], Flags[1024];

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%s%s%s", Src, Dst, Flags) != 3)
				continue;

			strupr(Src);

			PieceInfo* Info = Library->FindPiece(Src, NULL, false, false);
			if (!Info)
				continue;

			if (strchr(Flags, 'L'))
			{
				lcPieceTableEntry& Entry = PieceTable[Info];
				Entry.Flags |= LGEO_PIECE_LGEO;
				sprintf(Entry.Name, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				lcPieceTableEntry& Entry = PieceTable[Info];
				Entry.Flags |= LGEO_PIECE_AR;
				sprintf(Entry.Name, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
			{
				lcPieceTableEntry& Entry = PieceTable[Info];
				Entry.Flags |= LGEO_PIECE_SLOPE;
				Entry.Name[0] = 0;
			}
		}

		if (!ColorFile.Open(QFileInfo(QDir(Dialog.mLGEOPath), QLatin1String("lg_colors.lst")).absoluteFilePath(), "rt"))
		{
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(Dialog.mLGEOPath));
			return;
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

			strcpy(ColorTable[Color].Name, Name);
		}
	}

	if (!Dialog.mLGEOPath.isEmpty())
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

				const lcPieceTableEntry& Entry = PieceTable.value(Info);

				if (Entry.Name[0])
				{
					sprintf(Line, "#include \"%s.inc\"\n", Entry.Name);
					POVFile.WriteLine(Line);
				}

				break;
			}
		}

		POVFile.WriteLine("\n");
	}
	else
		POVFile.WriteLine("#include \"colors.inc\"\n\n");

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

		if (!ColorTable[ColorIdx].Name[0])
			sprintf(ColorTable[ColorIdx].Name, "lc_%s", Color->SafeName);
	}

	POVFile.WriteLine("\n");

	lcArray<const char*> ColorTablePointer;
	ColorTablePointer.SetSize(NumColors);
	for (int ColorIdx = 0; ColorIdx < NumColors; ColorIdx++)
		ColorTablePointer[ColorIdx] = ColorTable[ColorIdx].Name;

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		PieceInfo* Info = ModelParts[PartIdx].Info;
		lcMesh* Mesh = Info->GetMesh();
		lcPieceTableEntry& Entry = PieceTable[Info];

		if (!Mesh || Entry.Name[0])
			continue;

		char Name[LC_PIECE_NAME_LEN];
		char* Ptr;

		strcpy(Name, Info->m_strName);
		while ((Ptr = strchr(Name, '-')))
			*Ptr = '_';

		sprintf(Entry.Name, "lc_%s", Name);

		Mesh->ExportPOVRay(POVFile, Name, &ColorTablePointer[0]);

		POVFile.WriteLine("}\n\n");

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	lcCamera* Camera = gMainWindow->GetActiveView()->mCamera;
	const lcVector3& Position = Camera->mPosition;
	const lcVector3& Target = Camera->mTargetPosition;
	const lcVector3& Up = Camera->mUpVector;
	const lcModelProperties& Properties = mModels[0]->GetProperties();

	sprintf(Line, "camera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
			Up[0], Up[1], Up[2], Position[1] / 25.0f, Position[0] / 25.0f, Position[2] / 25.0f, Target[1] / 25.0f, Target[0] / 25.0f, Target[2] / 25.0f, Camera->m_fovy);
	POVFile.WriteLine(Line);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
			Properties.mBackgroundSolidColor[0], Properties.mBackgroundSolidColor[1], Properties.mBackgroundSolidColor[2]);
	POVFile.WriteLine(Line);

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		lcPieceTableEntry& Entry = PieceTable[ModelParts[PartIdx].Info];
		int Color;

		Color = ModelParts[PartIdx].ColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = ModelParts[PartIdx].WorldMatrix;

		if (Entry.Flags & LGEO_PIECE_SLOPE)
		{
			sprintf(Line, "merge {\n object {\n  %s%s\n  texture { %s }\n }\n"
					" object {\n  %s_slope\n  texture { %s normal { bumps 0.3 scale 0.02 } }\n }\n"
					" matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					Entry.Name, Suffix, ColorTable[Color].Name, Entry.Name, ColorTable[Color].Name,
					-f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}
		else
		{
			sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					Entry.Name, Suffix, ColorTable[Color].Name, -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}

		POVFile.WriteLine(Line);
	}

	POVFile.Close();

	if (Dialog.mRender)
	{
		QStringList Arguments;

		Arguments.append(QString::fromLatin1("+I%1").arg(Dialog.mFileName));

		if (!Dialog.mLGEOPath.isEmpty())
		{
			Arguments.append(QString::fromLatin1("+L%1lg/").arg(Dialog.mLGEOPath));
			Arguments.append(QString::fromLatin1("+L%1ar/").arg(Dialog.mLGEOPath));
		}

		Arguments.append(QString::fromLatin1("/EXIT"));

#ifndef QT_NO_PROCESS
		QProcess::execute(Dialog.mPOVRayPath, Arguments);
#endif
	}
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

	QString SaveFileName = GetExportFileName(FileName, "obj", tr("Export Wavefront"), tr("Wavefront Files (*.obj);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	lcDiskFile OBJFile;
	char Line[1024];

	if (!OBJFile.Open(SaveFileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	char buf[LC_MAXPATH], *ptr;
	lcuint32 vert = 1;

	OBJFile.WriteLine("# Model exported from LeoCAD\n");

	strcpy(buf, SaveFileName.toLatin1().constData());
	ptr = strrchr(buf, '.');
	if (ptr)
		*ptr = 0;

	strcat(buf, ".mtl");
	ptr = strrchr(buf, '\\');
	if (ptr)
		ptr++;
	else
	{
		ptr = strrchr(buf, '/');
		if (ptr)
			ptr++;
		else
			ptr = buf;
	}

	sprintf(Line, "#\n\nmtllib %s\n\n", ptr);
	OBJFile.WriteLine(Line);

	FILE* mat = fopen(buf, "wt");
	fputs("# Colors used by LeoCAD\n# You need to add transparency values\n#\n\n", mat);
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];
		fprintf(mat, "newmtl %s\nKd %.2f %.2f %.2f\n\n", Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
	}
	fclose(mat);

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		lcMesh* Mesh = ModelParts[PartIdx].Info->GetMesh();

		if (!Mesh)
			continue;

		const lcMatrix44& ModelWorld = ModelParts[PartIdx].WorldMatrix;
		float* Verts = (float*)Mesh->mVertexData;

		for (int i = 0; i < Mesh->mNumVertices * 3; i += 3)
		{
			lcVector3 Vertex = lcMul31(lcVector3(Verts[i], Verts[i+1], Verts[i+2]), ModelWorld);
			sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
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

	mActiveModel->SaveStepImages(Dialog.mFileName, Dialog.mStart != Dialog.mEnd, false, Dialog.mWidth, Dialog.mHeight, Dialog.mStart, Dialog.mEnd);
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
