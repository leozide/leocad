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
	mActiveModel->UpdateInterface();

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

		Name = QInputDialog::getText(gMainWindow, tr("New Model"), tr("Name:"), QLineEdit::Normal, Name, &Ok);

		if (!Ok)
			return;

		if (IsModelNameValid(Name))
			break;

		if (Name.isEmpty())
			QMessageBox::information(gMainWindow, tr("Empty Name"), tr("The model name cannot be empty."));
		else
			QMessageBox::information(gMainWindow, tr("Duplicate Model"), tr("A model named '%1' already exists in this project, please enter an unique name.").arg(Name));
	}

	if (!Name.isEmpty())
	{
		mModified = true;
		lcModel* Model = new lcModel(Name);
		Model->CreatePieceInfo(this);
		Model->SetSaved();
		mModels.Add(Model);
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
	QFileInfo FileInfo(FileName);
	QString Extension = FileInfo.suffix().toLower();

	if (Extension == QLatin1String("dat") || Extension == QLatin1String("ldr") || Extension == QLatin1String("mpd"))
	{
		QByteArray FileData = File.readAll();
		QBuffer Buffer(&FileData);
		Buffer.open(QIODevice::ReadOnly);

		while (!Buffer.atEnd())
		{
			lcModel* Model = new lcModel(QString());
			Model->LoadLDraw(Buffer, this);

			if (mModels.IsEmpty() || !Model->GetProperties().mName.isEmpty())
			{
				mModels.Add(Model);
				Model->SetSaved();
			}
			else
				delete Model;
		}
	}
	else
	{
		lcMemFile MemFile;
		QByteArray FileData = File.readAll();
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

	mFileName = FileName;
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
	bool MPD = mModels.GetSize() > 1;

	for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
	{
		lcModel* Model = mModels[ModelIdx];

		if (MPD)
			Stream << QLatin1String("0 FILE ") << Model->GetProperties().mName << QLatin1String("\r\n");

		Model->SaveLDraw(Stream, MPD, false);
		Model->SetSaved();

		if (MPD)
			Stream << QLatin1String("0 NOFILE\r\n");
	}

	mFileName = FileName;
	mModified = false;

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

void Project::Export3DStudio()
{
	lcArray<lcModelPartsEntry> ModelParts;

	GetModelParts(ModelParts);

	if (ModelParts.IsEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString FileName = QFileDialog::getSaveFileName(gMainWindow, tr("Export 3D Studio"), QString(), tr("3DS Files (*.3ds);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcDiskFile File;

	if (!File.Open(FileName, "wb"))
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
	File.WriteU32(6 + 1 + strlen(BackgroundImage.constData()));
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

		float* Verts = (float*)Mesh->mVertexBuffer.mData;
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

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			NumTriangles += Section->NumIndices / 3;
		}

		File.WriteU16(NumTriangles);

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			lcuint16* Indices = (lcuint16*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(lcuint16);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
			{
				File.WriteU16(Indices[IndexIdx + 0]);
				File.WriteU16(Indices[IndexIdx + 1]);
				File.WriteU16(Indices[IndexIdx + 2]);
				File.WriteU16(7);
			}
		}

		NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
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
	lcArray<lcPartsListEntry> PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, PartsList);

	if (PartsList.IsEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString FileName = QFileDialog::getSaveFileName(gMainWindow, tr("Export BrickLink"), QString(), tr("XML Files (*.xml);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcDiskFile BrickLinkFile;
	char Line[1024];

	if (!BrickLinkFile.Open(FileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(FileName));
		return;
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
	BrickLinkFile.WriteLine("<INVENTORY>\n");

	for (int PieceIdx = 0; PieceIdx < PartsList.GetSize(); PieceIdx++)
	{
		BrickLinkFile.WriteLine("  <ITEM>\n");
		BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");

		sprintf(Line, "    <ITEMID>%s</ITEMID>\n", PartsList[PieceIdx].Info->m_strName);
		BrickLinkFile.WriteLine(Line);

		int Count = PartsList[PieceIdx].Count;
		if (Count > 1)
		{
			sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Count);
			BrickLinkFile.WriteLine(Line);
		}

		int Color = lcGetBrickLinkColor(PartsList[PieceIdx].ColorIndex);
		if (Color)
		{
			sprintf(Line, "    <COLOR>%d</COLOR>\n", Color);
			BrickLinkFile.WriteLine(Line);
		}

		BrickLinkFile.WriteLine("  </ITEM>\n");
	}

	BrickLinkFile.WriteLine("</INVENTORY>\n");

	setlocale(LC_NUMERIC, OldLocale);
}

void Project::ExportCSV()
{
	lcArray<lcPartsListEntry> PartsList;

	if (!mModels.IsEmpty())
		mModels[0]->GetPartsList(gDefaultColor, PartsList);

	if (PartsList.IsEmpty())
	{
		QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Nothing to export."));
		return;
	}

	QString FileName = QFileDialog::getSaveFileName(gMainWindow, tr("Export CSV"), QString(), tr("CSV Files (*.csv);;All Files (*.*)"));

	if (FileName.isEmpty())
		return;

	lcDiskFile CSVFile;
	char Line[1024];

	if (!CSVFile.Open(FileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(FileName));
		return;
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
	CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

	for (int PieceIdx = 0; PieceIdx < PartsList.GetSize(); PieceIdx++)
	{
		sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", PartsList[PieceIdx].Info->m_strDescription, gColorList[PartsList[PieceIdx].ColorIndex].Name,
				PartsList[PieceIdx].Count, PartsList[PieceIdx].Info->m_strName, gColorList[PartsList[PieceIdx].ColorIndex].Code);
		CSVFile.WriteLine(Line);
	}

	setlocale(LC_NUMERIC, OldLocale);
}

void Project::CreateHTMLPieceList(QTextStream& Stream, lcModel* Model, lcStep Step, bool Images, const QString& ImageExtension)
{
	int* ColorsUsed = new int[gColorList.GetSize()];
	memset(ColorsUsed, 0, sizeof(ColorsUsed[0]) * gColorList.GetSize());
	int* PiecesUsed = new int[gColorList.GetSize()];
	int NumColors = 0;

	lcArray<lcPartsListEntry> PartsList;

	if (Step == 0)
		Model->GetPartsList(gDefaultColor, PartsList);
	else
		Model->GetPartsListForStep(Step, gDefaultColor, PartsList);

	for (int PieceIdx = 0; PieceIdx < PartsList.GetSize(); PieceIdx++)
		ColorsUsed[PartsList[PieceIdx].ColorIndex]++;

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

	for (int j = 0; j < lcGetPiecesLibrary()->mPieces.GetSize(); j++)
	{
		bool Add = false;
		memset(PiecesUsed, 0, sizeof(PiecesUsed[0]) * gColorList.GetSize());
		PieceInfo* pInfo = lcGetPiecesLibrary()->mPieces[j];

		for (int PieceIdx = 0; PieceIdx < PartsList.GetSize(); PieceIdx++)
		{
			if (PartsList[PieceIdx].Info == pInfo)
			{
				PiecesUsed[PartsList[PieceIdx].ColorIndex] += PartsList[PieceIdx].Count;
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
	
	lcModel* Model = mModels[0];
	lcStep LastStep = Model->GetLastStep();

	if (Options.SinglePage)
	{
		QString FileName = QFileInfo(Dir, BaseName + HTMLExtension).absoluteFilePath();
		QFile File(FileName);

		if (!File.open(QIODevice::WriteOnly))
		{
			QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
			return;
		}

		QTextStream Stream(&File);

		Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>Instructions for %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(Title);

		for (lcStep Step = 1; Step <= LastStep; Step++)
		{
			QString StepString = QString("%1").arg(Step, 2, 10, QLatin1Char('0'));
			Stream << QString("<IMG SRC=\"%1-%2%3\" ALT=\"Step %4\" WIDTH=%5 HEIGHT=%6><BR><BR>\r\n").arg(BaseName, StepString, ImageExtension, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

			if (Options.PartsListStep)
				CreateHTMLPieceList(Stream, Model, Step, Options.PartsListImages, ImageExtension);
		}

		if (Options.PartsListEnd)
			CreateHTMLPieceList(Stream, Model, 0, Options.PartsListImages, ImageExtension);

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
				QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
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
				QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>%1 - Step %2</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\r\n").arg(Title, QString::number(Step));
			Stream << QString("<IMG SRC=\"%1-%2%3\" ALT=\"Step %4\" WIDTH=%5 HEIGHT=%6><BR><BR>\r\n").arg(BaseName, StepString, ImageExtension, StepString, QString::number(Options.StepImagesWidth), QString::number(Options.StepImagesHeight));

			if (Options.PartsListStep)
				CreateHTMLPieceList(Stream, Model, Step, Options.PartsListImages, ImageExtension);

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
				QMessageBox::warning(gMainWindow, tr("Error"), tr("Error writing to file '%1':\n%2").arg(FileName, File.errorString()));
				return;
			}

			QTextStream Stream(&File);

			Stream << QString("<HTML>\r\n<HEAD>\r\n<TITLE>Pieces used by %1</TITLE>\r\n</HEAD>\r\n<BR>\r\n<CENTER>\n").arg(Title);

			CreateHTMLPieceList(Stream, Model, 0, Options.PartsListImages, ImageExtension);

			Stream << QLatin1String("</CENTER>\n<BR><HR><BR>");
			Stream << QString("<A HREF=\"%1-%2.html\">Previous</A> ").arg(BaseName, QString("%1").arg(LastStep, 2, 10, QLatin1Char('0')));

			if (Options.IndexPage)
				Stream << QString("<A HREF=\"%1-index.html\">Index</A> ").arg(BaseName);

			Stream << QLatin1String("<BR></HTML>\r\n");
		}
	}

	QString StepImageBaseName = QFileInfo(Dir, BaseName + QLatin1String("-%1") + ImageExtension).absoluteFilePath();
	Model->SaveStepImages(StepImageBaseName, Options.StepImagesWidth, Options.StepImagesHeight, 1, LastStep);

	if (Options.PartsListImages)
	{
		gMainWindow->mPreviewWidget->MakeCurrent();
		lcContext* Context = gMainWindow->mPreviewWidget->mContext;
		int Width = Options.PartImagesWidth;
		int Height = Options.PartImagesHeight;

		if (!Context->BeginRenderToTexture(Width, Height))
		{
			QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Error creating images."));
			return;
		}

		float aspect = (float)Width/(float)Height;
		Context->SetViewport(0, 0, Width, Height);

		lcArray<lcPartsListEntry> PartsList;
		Model->GetPartsList(gDefaultColor, PartsList);

		lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(30.0f, aspect, 1.0f, 2500.0f);
		lcMatrix44 ViewMatrix;

		Context->SetDefaultState();
		Context->SetProjectionMatrix(ProjectionMatrix);

		for (int PieceIdx = 0; PieceIdx < PartsList.GetSize(); PieceIdx++)
		{
			PieceInfo* Info = PartsList[PieceIdx].Info;

			glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lcVector3 CameraPosition(-100.0f, -100.0f, 75.0f);
			Info->ZoomExtents(ProjectionMatrix, ViewMatrix, CameraPosition);

			lcScene Scene;
			Scene.Begin(ViewMatrix);

			Info->AddRenderMeshes(Scene, lcMatrix44Identity(), Options.PartImagesColor, false, false);

			Scene.End();

			Context->DrawOpaqueMeshes(ViewMatrix, Scene.mOpaqueMeshes);
			Context->DrawTranslucentMeshes(ViewMatrix, Scene.mTranslucentMeshes);

			Context->UnbindMesh(); // context remove

			QString FileName = QFileInfo(Dir, Info->m_strName + ImageExtension).absoluteFilePath();
			if (!Context->SaveRenderToTextureImage(FileName, Width, Height))
				break;
		}
		Context->EndRenderToTexture();
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

	lcPOVRayDialogOptions Options;

	Options.POVRayPath = lcGetProfileString(LC_PROFILE_POVRAY_PATH);
	Options.LGEOPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
	Options.Render = lcGetProfileInt(LC_PROFILE_POVRAY_RENDER);

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_POVRAY, &Options))
		return;

	lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
	lcSetProfileInt(LC_PROFILE_POVRAY_RENDER, Options.Render);

	lcDiskFile POVFile;

	if (!POVFile.Open(Options.FileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(Options.FileName));
		return;
	}

	char Line[1024];

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	char* PieceTable = new char[Library->mPieces.GetSize() * LC_PIECE_NAME_LEN];
	int* PieceFlags = new int[Library->mPieces.GetSize()];
	int NumColors = gColorList.GetSize();
	char* ColorTable = new char[NumColors * LC_MAX_COLOR_NAME];

	memset(PieceTable, 0, Library->mPieces.GetSize() * LC_PIECE_NAME_LEN);
	memset(PieceFlags, 0, Library->mPieces.GetSize() * sizeof(int));
	memset(ColorTable, 0, NumColors * LC_MAX_COLOR_NAME);

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

	if (!Options.LGEOPath.isEmpty())
	{
		lcDiskFile TableFile, ColorFile;

		if (!TableFile.Open(QFileInfo(QDir(Options.LGEOPath), QLatin1String("lg_elements.lst")).absoluteFilePath(), "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(Options.LGEOPath));
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

			PieceInfo* Info = Library->FindPiece(Src, NULL, false);
			if (!Info)
				continue;

			int Index = Library->mPieces.FindIndex(Info);

			if (strchr(Flags, 'L'))
			{
				PieceFlags[Index] |= LGEO_PIECE_LGEO;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				PieceFlags[Index] |= LGEO_PIECE_AR;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
				PieceFlags[Index] |= LGEO_PIECE_SLOPE;
		}

		if (!ColorFile.Open(QFileInfo(QDir(Options.LGEOPath), QLatin1String("lg_colors.lst")).absoluteFilePath(), "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			QMessageBox::information(gMainWindow, tr("LeoCAD"), tr("Could not find LGEO files in folder '%1'.").arg(Options.LGEOPath));
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

			strcpy(&ColorTable[Color * LC_MAX_COLOR_NAME], Name);
		}
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");

	if (!Options.LGEOPath.isEmpty())
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

				int Index = Library->mPieces.FindIndex(Info);

				if (PieceTable[Index * LC_PIECE_NAME_LEN])
				{
					sprintf(Line, "#include \"%s.inc\"\n", PieceTable + Index * LC_PIECE_NAME_LEN);
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

		if (!ColorTable[ColorIdx * LC_MAX_COLOR_NAME])
			sprintf(&ColorTable[ColorIdx * LC_MAX_COLOR_NAME], "lc_%s", Color->SafeName);
	}

	POVFile.WriteLine("\n");

	for (int PartIdx = 0; PartIdx < ModelParts.GetSize(); PartIdx++)
	{
		PieceInfo* Info = ModelParts[PartIdx].Info;
		lcMesh* Mesh = Info->GetMesh();
		int Index = Library->mPieces.FindIndex(Info);

		if (!Mesh || PieceTable[Index * LC_PIECE_NAME_LEN])
			continue;

		char Name[LC_PIECE_NAME_LEN];
		char* Ptr;

		strcpy(Name, Info->m_strName);
		while ((Ptr = strchr(Name, '-')))
			*Ptr = '_';

		sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lc_%s", Name);

		Mesh->ExportPOVRay(POVFile, Name, ColorTable);

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
		int Index = Library->mPieces.FindIndex(ModelParts[PartIdx].Info);
		int Color;

		Color = ModelParts[PartIdx].ColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = ModelParts[PartIdx].WorldMatrix;

		if (PieceFlags[Index] & LGEO_PIECE_SLOPE)
		{
			sprintf(Line, "merge {\n object {\n  %s%s\n  texture { %s }\n }\n"
					" object {\n  %s_slope\n  texture { %s normal { bumps 0.3 scale 0.02 } }\n }\n"
					" matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], PieceTable + Index * LC_PIECE_NAME_LEN, &ColorTable[Color * LC_MAX_COLOR_NAME],
					-f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}
		else
		{
			sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13] / 25.0f, f[12] / 25.0f, f[14] / 25.0f);
		}

		POVFile.WriteLine(Line);
	}

	delete[] PieceTable;
	delete[] PieceFlags;
	setlocale(LC_NUMERIC, OldLocale);
	POVFile.Close();

	if (Options.Render)
	{
		QStringList Arguments;

		Arguments.append(QString::fromLatin1("+I%1").arg(Options.FileName));

		if (!Options.LGEOPath.isEmpty())
		{
			Arguments.append(QString::fromLatin1("+L%1lg/").arg(Options.LGEOPath));
			Arguments.append(QString::fromLatin1("+L%1ar/").arg(Options.LGEOPath));
		}

		QString AbsolutePath = QFileInfo(Options.FileName).absolutePath();
		if (!AbsolutePath.isEmpty())
			Arguments.append(QString::fromLatin1("+o%1").arg(AbsolutePath));

		QProcess::execute(Options.POVRayPath, Arguments);
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

	QString SaveFileName = FileName;
	if (SaveFileName.isEmpty())
	{
		SaveFileName = QFileDialog::getSaveFileName(gMainWindow, tr("Export Wavefront"), QString(), tr("Wavefront Files (*.obj);;All Files (*.*)"));

		if (SaveFileName.isEmpty())
			return;
	}

	lcDiskFile OBJFile;
	char Line[1024];

	if (!OBJFile.Open(SaveFileName, "wt"))
	{
		QMessageBox::warning(gMainWindow, tr("LeoCAD"), tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	char buf[LC_MAXPATH], *ptr;
	lcuint32 vert = 1;

	const char* OldLocale = setlocale(LC_NUMERIC, "C");

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
		float* Verts = (float*)Mesh->mVertexBuffer.mData;

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

	setlocale(LC_NUMERIC, OldLocale);
}

void Project::SaveImage()
{
	lcImageDialogOptions Options;
	lcStep LastStep = mActiveModel->GetLastStep();

	Options.Width = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
	Options.Height = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);
	Options.Start = mActiveModel->GetCurrentStep();
	Options.End = LastStep;

	if (!mFileName.isEmpty())
	{
		Options.FileName = mFileName;
		QString Extension = QFileInfo(Options.FileName).suffix();
		Options.FileName = Options.FileName.left(Options.FileName.length() - Extension.length() - 1);
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

	mActiveModel->SaveStepImages(Options.FileName, Options.Width, Options.Height, Options.Start, Options.End);
}
