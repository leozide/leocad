#include "lc_global.h"
#include "lc_application.h"
#include "lc_colors.h"
#include "lc_context.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "lc_model.h"
#include "lc_modelhistory.h"
#include "lc_meshloader.h"
#include "lc_file.h"
#include "lc_piecedata.h"
#include "lc_scene.h"
#include "lc_selectdialog.h"
#include "lc_view.h"
#include "camera.h"
#include "light.h"
#include "piece.h"
#include "project.h"
#include <QtTest/QtTest>

class lcModelInstanceTestScene : public lcScene
{
public:
	const std::vector<lcRenderMesh>& GetMeshes() const
	{
		return mRenderMeshes;
	}

	const std::vector<const lcObject*>& GetInterfaceObjects() const
	{
		return mInterfaceObjects;
	}
};

class lcModelInstanceTestContext : public lcContext
{
public:
	const lcMatrix44& GetWorldMatrix() const
	{
		return mWorldMatrix;
	}
};

class lcModelInstanceTests : public QObject
{
	Q_OBJECT

private slots:
	void ArchiveSubpartPrecedence();
	void TexmapFallbackBranches();
	void TexmapFallbackLoading();
	void TexmapFallbackIncludes();
	void MeshCacheValidation_data();
	void MeshCacheValidation();
	void MeshSectionCountOverflow();
	void EmbeddedDataAfterLastPieceDeletion();
	void RepeatedInstances();
	void InterfaceTransforms();
	void CompletedExportEvaluatesExternalModels();
	void TexturedDirectGeometry3DS();

private:
	void CameraContexts(lcMainWindow* Window, lcView* View, lcModel* Main, lcModel* A, lcModel* B);
};

void lcModelInstanceTests::ArchiveSubpartPrecedence()
{
	struct ScopedLibrary
	{
		lcPiecesLibrary* Previous = gApplication->mLibrary;
		std::unique_ptr<lcPiecesLibrary> Library = std::make_unique<lcPiecesLibrary>(lcStudStyle::Plain, false);
		ScopedLibrary() { gApplication->mLibrary = Library.get(); }
		~ScopedLibrary()
		{
			Library.reset();
			gApplication->mLibrary = Previous;
		}
	} Scope;

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	const QByteArray Data = "0 Fixture\n3 16 0 0 0 1 0 0 0 1 0\n";
	quint32 Crc = 0xffffffff;

	for (unsigned char Byte : Data)
	{
		Crc ^= Byte;

		for (int Bit = 0; Bit < 8; Bit++)
			Crc = (Crc >> 1) ^ (0xedb88320u & (0u - (Crc & 1u)));
	}

	Crc ^= 0xffffffff;

	for (bool SubpartFirst : { false, true })
	{
		std::vector<QByteArray> Names = { "ldraw/parts/root.dat", "ldraw/p/s/test.dat", "ldraw/parts/s/test.dat" };

		if (SubpartFirst)
			std::swap(Names[1], Names[2]);

		// A small stored ZIP fixture lets the test control central-directory order.
		QByteArray Zip;
		QDataStream Stream(&Zip, QIODevice::WriteOnly);
		Stream.setByteOrder(QDataStream::LittleEndian);
		std::vector<quint32> Offsets;

		for (const QByteArray& Name : Names)
		{
			Offsets.push_back(static_cast<quint32>(Zip.size()));
			Stream << quint32(0x04034b50) << quint16(20) << quint16(0) << quint16(0) << quint32(0)
				<< Crc << quint32(Data.size()) << quint32(Data.size()) << quint16(Name.size()) << quint16(0);
			QCOMPARE(Stream.writeRawData(Name.constData(), Name.size()), qint64(Name.size()));
			QCOMPARE(Stream.writeRawData(Data.constData(), Data.size()), qint64(Data.size()));
		}

		const quint32 CentralOffset = static_cast<quint32>(Zip.size());

		for (size_t Index = 0; Index < Names.size(); Index++)
		{
			const QByteArray& Name = Names[Index];
			Stream << quint32(0x02014b50) << quint16(20) << quint16(20) << quint16(0) << quint16(0) << quint32(0)
				<< Crc << quint32(Data.size()) << quint32(Data.size()) << quint16(Name.size()) << quint16(0)
				<< quint16(0) << quint16(0) << quint16(0) << quint32(0) << Offsets[Index];
			QCOMPARE(Stream.writeRawData(Name.constData(), Name.size()), qint64(Name.size()));
		}

		const quint32 CentralSize = static_cast<quint32>(Zip.size()) - CentralOffset;
		Stream << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(Names.size())
			<< quint16(Names.size()) << CentralSize << CentralOffset << quint16(0);
		QCOMPARE(Stream.status(), QDataStream::Ok);

		const QString Path = Directory.filePath(SubpartFirst ? "subpart-first.zip" : "primitive-first.zip");
		QFile File(Path);
		QVERIFY(File.open(QIODevice::WriteOnly));
		QCOMPARE(File.write(Zip), qint64(Zip.size()));
		File.close();

		for (int Pass = 0; Pass < 2; Pass++)
		{
			QVERIFY(Scope.Library->Load(Path, false, false));
			lcLibraryPrimitive* Primitive = Scope.Library->FindPrimitive("S/TEST.DAT");
			QVERIFY(Primitive);
			QVERIFY(Primitive->mSubFile);
			const lcResult<lcOpenedLibraryFile> Include = Scope.Library->OpenPieceDataFile("S/TEST.DAT");
			QVERIFY(Include);
			QVERIFY(Include.value().Source == Primitive->mSource);
		}
	}
}

void lcModelInstanceTests::TexmapFallbackBranches()
{
	const QByteArray Start = "0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n";
	const QByteArray Triangle = "3 16 0 0 0 1 0 0 0 1 0\n";
	const QByteArray Fallback = "0 !TEXMAP FALLBACK\n3 4 20 0 0 21 0 0 20 1 0\n";
	const QByteArray End = "0 !TEXMAP END\n";

	const auto Build = [](const QByteArray& Text, const std::set<std::string>& Unavailable, bool Expected, bool Textured, float Maximum)
	{
		lcMemFile File;

		File.WriteBuffer(Text.constData(), Text.size());
		File.Seek(0, SEEK_SET);

		lcLibraryMeshData Data;
		lcMeshLoader Loader(Data, nullptr, lcMeshLoaderFlag::RequireAllIncludes);

		Loader.SetUnavailableTextures(Unavailable);

		QCOMPARE(Loader.LoadMesh(File, LC_MESHDATA_SHARED), Expected);

		if (!Expected)
		{
			QVERIFY(!Loader.GetError().isEmpty());

			return;
		}

		std::unique_ptr<lcMesh> Mesh(Data.CreateMesh(nullptr));

		QVERIFY(Mesh);
		QCOMPARE(Data.mHasTextures, Textured);
		QCOMPARE(Mesh->mBoundingBox.Max.x, Maximum);
	};

	Build(Start + Triangle + Fallback + End, {}, true, true, 1);
	Build(Start + Triangle + Fallback + End, { "MISSING" }, true, false, 21);
	Build(Start + Triangle + End, { "MISSING" }, false, false, 0);

	// An inactive outer branch must suppress nested maps and their diagnostics.
	const QByteArray Nested = "0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 nested.png\n";

	Build(Start + Nested + Triangle + End + Fallback + End, { "MISSING", "NESTED" }, true, false, 21);
	Build(Start + Triangle + "0 !TEXMAP FALLBACK\n" + Nested + Triangle + End + End, { "NESTED" }, true, true, 1);

	// Available mappings remain textured beside a selected fallback.
	Build(Start + Triangle + Fallback + End + Nested + Triangle + End, { "MISSING" }, true, true, 21);
	Build("0 !TEXMAP NEXT PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n" + Triangle + Triangle, {}, true, true, 1);
	Build("0 !TEXMAP NEXT PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n" + Triangle, { "MISSING" }, false, false, 0);
}

void lcModelInstanceTests::TexmapFallbackLoading()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	const QByteArray Start = "0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n";
	const QByteArray Triangle = "3 16 0 0 0 1 0 0 0 1 0\n";
	const QByteArray Fallback = "0 !TEXMAP FALLBACK\n3 4 20 0 0 21 0 0 20 1 0\n";

	for (int Case = 0; Case < 4; Case++)
	{
		if (Case == 1)
		{
			QFile Texture(Directory.filePath("missing.png"));

			QVERIFY(Texture.open(QIODevice::WriteOnly));
			QVERIFY(Texture.write("invalid image") > 0);
		}

		if (Case == 3)
		{
			QImage Image(4, 4, QImage::Format_RGB888);

			Image.fill(Qt::red);
			QVERIFY(Image.save(Directory.filePath("missing.png")));
		}

		QFile File(Directory.filePath(QString("case%1.ldr").arg(Case)));

		QVERIFY(File.open(QIODevice::WriteOnly));

		const QByteArray Text = "0 Fallback test\n" + Start + Triangle + (Case == 2 ? QByteArray() : Fallback) + "0 !TEXMAP END\n";

		QCOMPARE(File.write(Text), qint64(Text.size()));
		File.close();

		Project Document;

		QVERIFY(Document.Load(File.fileName(), false));

		lcModel* Model = Document.GetMainModel();

		Model->WaitForAssets();

		lcMesh* Mesh = Model->GetPieceData()->GetMesh();

		if (Case == 2)
		{
			QVERIFY(!Mesh);
			QCOMPARE(Model->GetPieceData()->mState, lcAssetState::Failed);
		}
		else
		{
			QVERIFY(Mesh);
			QCOMPARE(Mesh->mNumTexturedVertices, 0);
			QCOMPARE(Mesh->mBoundingBox.Max.x, 21.0f);
		}
	}
}

void lcModelInstanceTests::TexmapFallbackIncludes()
{
	struct ScopedLibrary
	{
		lcPiecesLibrary* Previous = gApplication->mLibrary;
		std::unique_ptr<lcPiecesLibrary> Library = std::make_unique<lcPiecesLibrary>(lcStudStyle::Plain, false);

		ScopedLibrary() { gApplication->mLibrary = Library.get(); }

		~ScopedLibrary()
		{
			Library.reset();

			gApplication->mLibrary = Previous;
		}
	} Scope;

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkpath("parts"));
	QVERIFY(QDir(Directory.path()).mkpath("p"));

	const auto Write = [&](const char* Name, const QByteArray& Text)
	{
		QFile File(Directory.filePath(QString::fromLatin1(Name)));

		return File.open(QIODevice::WriteOnly) && File.write(Text) == Text.size();
	};

	QVERIFY(Write("parts/test.dat", "0 Test\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 mapped.dat\n"));
	QVERIFY(Write("p/mapped.dat",
		"0 Mapped primitive\n"
		"0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n"
		"3 16 0 0 0 1 0 0 0 1 0\n"
		"0 !TEXMAP FALLBACK\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 fallback.dat\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 fallback.dat\n"
		"0 !TEXMAP END\n"));
	QVERIFY(Write("p/fallback.dat", "0 Fallback primitive\n0 !TEXMAP END\n3 4 20 0 0 21 0 0 20 1 0\n"));

	QVERIFY(Write("parts/cycle.dat", "0 Cycle test\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 loop.dat\n"));
	QVERIFY(Write("p/loop.dat",
		"0 Loop primitive\n"
		"0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 missing.png\n"
		"3 16 0 0 0 1 0 0 0 1 0\n"
		"0 !TEXMAP FALLBACK\n"
		"0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 available.png\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 loop.dat\n"
		"0 !TEXMAP END\n"
		"0 !TEXMAP END\n"));

	// A primitive and a part may share a name. TEXMAP file lookup selects the part.
	QVERIFY(Write("parts/same.dat", "0 Same-name part\n3 16 0 0 0 1 0 0 0 1 0\n"));
	QVERIFY(Write("p/same.dat",
		"0 Same-name primitive\n"
		"0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 available.png\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 same.dat\n"
		"0 !TEXMAP END\n"));
	QVERIFY(Write("parts/collision.dat",
		"0 Source folder collision\n"
		"0 !TEXMAP START PLANAR 0 0 0 1 0 0 0 1 0 other.png\n"
		"3 16 0 0 0 1 0 0 0 1 0\n"
		"0 !TEXMAP FALLBACK\n"
		"3 16 0 0 0 1 0 0 0 1 0\n"
		"0 !TEXMAP END\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 same.dat\n"));

	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	QVERIFY(Library->Load(Directory.path(), false, false));

	lcPieceDataRef Piece = Library->AcquirePieceData("test.dat", nullptr, false, false);

	QVERIFY(Piece);
	QVERIFY(Library->EnsurePieceDataAssetsReady(Piece));
	QVERIFY(Piece->GetMesh());
	QCOMPARE(Piece->GetMesh()->mNumTexturedVertices, 0);
	QCOMPARE(Piece->GetMesh()->mBoundingBox.Max.x, 21.0f);

	// The shared primitive cache must still hold its original textured branch.
	lcLibraryPrimitive* Primitive = Library->FindPrimitive("MAPPED.DAT");

	QVERIFY(Primitive);
	QVERIFY(Primitive->mMeshData.mHasTextures);

	lcPieceDataRef Cycle = Library->AcquirePieceData("cycle.dat", nullptr, false, false);

	QVERIFY(Cycle);
	QVERIFY(!Library->EnsurePieceDataAssetsReady(Cycle));
	QVERIFY(Library->GetPieceDataLoadError(Cycle.Get()).contains("Circular include"));

	lcLibraryPrimitive* SamePrimitive = Library->FindPrimitive("SAME.DAT");
	QVERIFY(SamePrimitive);
	const lcResult<lcOpenedLibraryFile> PrimitiveFile = Library->OpenPrimitiveFile(SamePrimitive);
	QVERIFY(PrimitiveFile);
	QVERIFY(PrimitiveFile.value().Source == SamePrimitive->mSource);
	QCOMPARE(PrimitiveFile.value().Source.FilePath, QFileInfo(Directory.filePath("p/same.dat")).canonicalFilePath());
	const lcResult<lcOpenedLibraryFile> SameFile = Library->OpenPieceDataFile("SAME.DAT");
	QVERIFY(SameFile);
	QVERIFY(SameFile.value().Source != PrimitiveFile.value().Source);
	QCOMPARE(SameFile.value().Source.FilePath, QFileInfo(Directory.filePath("parts/same.dat")).canonicalFilePath());
	const lcResult<lcOpenedLibraryFile> AliasFile = Library->OpenPieceDataFile("../parts/same.dat");
	QVERIFY(AliasFile);
	QVERIFY(AliasFile.value().Source == SameFile.value().Source);
	const lcResult<lcOpenedLibraryFile> CollisionFile = Library->OpenPieceDataFile("COLLISION.DAT");
	QVERIFY(CollisionFile);
	lcLibraryMeshData CollisionData;
	lcMeshLoader CollisionLoader(CollisionData, nullptr, lcMeshLoaderFlag::Optimize | lcMeshLoaderFlag::RequireAllIncludes);
	CollisionLoader.SetUnavailableTextures({ "OTHER" });
	QVERIFY(CollisionLoader.LoadMesh(*CollisionFile.value().File, LC_MESHDATA_SHARED));
	QVERIFY(CollisionLoader.GetError().isEmpty());
	QVERIFY(!CollisionData.IsEmpty());

	Cycle.Reset();
	Piece.Reset();
}

void lcModelInstanceTests::MeshCacheValidation_data()
{
	QTest::addColumn<bool>("WideIndices");
	QTest::addColumn<quint16>("Primitive");
	QTest::addColumn<quint32>("Offset");
	QTest::addColumn<quint32>("Count");
	QTest::addColumn<quint32>("VertexIndex");
	QTest::addColumn<bool>("Accepted");

	for (bool Wide : { false, true })
	{
		const QByteArray Prefix = Wide ? "wide-" : "short-";
		const quint32 ElementSize = Wide ? 4 : 2;
		const auto Row = [&](const char* Name, quint16 Primitive, quint32 Offset, quint32 Count, quint32 VertexIndex, bool Accepted)
		{
			QTest::newRow((Prefix + Name).constData()) << Wide << Primitive << Offset << Count << VertexIndex << Accepted;
		};

		Row("triangles", LC_MESH_TRIANGLES, 3 * ElementSize, 3, 2, true);
		Row("lines", LC_MESH_LINES, 0, 6, 2, true);
		Row("textured", LC_MESH_TEXTURED_TRIANGLES, 0, 6, 2, true);
		Row("conditional", LC_MESH_CONDITIONAL_LINES, 0, 2, 0, true);
		Row("empty", LC_MESH_TRIANGLES, 6 * ElementSize, 0, 0, true);
		Row("offset", LC_MESH_TRIANGLES, 1000, 3, 0, false);
		Row("alignment", LC_MESH_TRIANGLES, 1, 3, 0, false);
		Row("count", LC_MESH_TRIANGLES, 0, 999, 0, false);
		Row("triangle-count", LC_MESH_TRIANGLES, 0, 2, 0, false);
		Row("line-count", LC_MESH_LINES, 0, 3, 0, false);
		Row("vertex", LC_MESH_TRIANGLES, 0, 3, Wide ? 65536 : 3, false);
		Row("textured-vertex", LC_MESH_TEXTURED_TRIANGLES, 0, 3, 3, false);
		Row("conditional-offset", LC_MESH_CONDITIONAL_LINES, 2, 2, 0, false);
		Row("conditional-count", LC_MESH_CONDITIONAL_LINES, 0, 1, 0, false);
		Row("primitive", 3, 0, 0, 0, false);
		Row("signed-offset", LC_MESH_TRIANGLES, 0xffffffff, 0, 0, false);
		Row("signed-count", LC_MESH_TRIANGLES, 0, 0xffffffff, 0, false);
	}
}

void lcModelInstanceTests::MeshCacheValidation()
{
	QFETCH(bool, WideIndices);
	QFETCH(quint16, Primitive);
	QFETCH(quint32, Offset);
	QFETCH(quint32, Count);
	QFETCH(quint32, VertexIndex);
	QFETCH(bool, Accepted);

	lcMemFile File;
	File.WriteU32(LC_FOURCC('M', 'E', 'S', 'H'));
	File.WriteU32(0x0123);
	File.WriteU32(0);
	for (int Index = 0; Index < 7; Index++)
		File.WriteFloat(0);

	const quint32 VertexCount = WideIndices ? 65536 : 3;
	File.WriteU32(VertexCount);
	File.WriteU32(3);
	File.WriteU32(2);
	File.WriteU32(6);
	File.WriteU16(LC_NUM_MESH_LODS);
	File.WriteU16(1);
	File.WriteU16(0);
	File.WriteU32(16);
	File.WriteU32(Offset);
	File.WriteU32(Count);
	File.WriteU16(Primitive);
	for (int Index = 0; Index < 7; Index++)
		File.WriteFloat(0);
	File.WriteU16(0);

	const QByteArray Vertices(VertexCount * sizeof(lcVertex) + 3 * sizeof(lcVertexTextured) + 2 * sizeof(lcVertexConditional), 0);
	File.WriteBuffer(Vertices.constData(), Vertices.size());
	for (int Index = 0; Index < 6; Index++)
	{
		if (WideIndices)
			File.WriteU32(VertexIndex);
		else
			File.WriteU16(VertexIndex);
	}
	File.Seek(0, SEEK_SET);

	lcMesh Mesh;
	QCOMPARE(Mesh.FileLoad(File), Accepted);
}

void lcModelInstanceTests::MeshSectionCountOverflow()
{
	lcLibraryMeshData Data;
	Data.mData[LC_MESHDATA_HIGH].mVertices.resize(2);
	for (lcMeshLoaderVertex& Vertex : Data.mData[LC_MESHDATA_HIGH].mVertices)
	{
		Vertex.Position = lcVector3(0, 0, 0);
		Vertex.Normal = lcVector3(0, 0, 1);
		Vertex.NormalWeight = 1;
	}

	std::vector<lcMeshLoaderMaterial> Materials(65536);
	for (size_t Index = 0; Index < Materials.size(); Index++)
	{
		Materials[Index].Color = static_cast<quint32>(Index);
		std::unique_ptr<lcMeshLoaderSection> Section(new lcMeshLoaderSection(LC_MESH_LINES, &Materials[Index]));
		Section->mIndices = { 0, 1 };
		Data.mData[LC_MESHDATA_HIGH].mSections.push_back(std::move(Section));
	}

	std::unique_ptr<lcMesh> Mesh(Data.CreateMesh(nullptr));
	QVERIFY(!Mesh);
}

void lcModelInstanceTests::EmbeddedDataAfterLastPieceDeletion()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	const QByteArray DataBlock = "0 !DATA unused.bin\n0 !: YQ==\n";
	const QByteArray Fixture =
		"0 Ordinary model\n"
		"0 !LEOCAD CAMERA POSITION -500 0 0\n"
		"0 !LEOCAD CAMERA TARGET_POSITION -500 0 -100\n"
		"0 !LEOCAD CAMERA UP_VECTOR 0 1 0\n"
		"0 !LEOCAD CAMERA NAME Saved camera\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 missing.dat\n" + DataBlock;
	QFile File(Directory.filePath("implicit.ldr"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	QCOMPARE(File.write(Fixture), qint64(Fixture.size()));
	File.close();

	Project Document(true);
	QVERIFY(Document.Load(File.fileName(), false));
	QVERIFY(Document.Save(File.fileName()).has_value());
	QVERIFY(File.open(QIODevice::ReadOnly));
	QCOMPARE(File.readAll(), Fixture);
	File.close();

	lcModel* Model = Document.GetMainModel();
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	QCOMPARE(Model->GetCameras().size(), size_t(1));
	Model->GetPieces().front()->SetSelected(true);
	Model->DeleteSelectedObjects();
	QVERIFY(Model->GetPieces().empty());
	QVERIFY(Document.Save(File.fileName()).has_value());

	Project Reloaded(true);
	QVERIFY(Reloaded.Load(File.fileName(), false));
	QCOMPARE(Reloaded.GetModels().size(), size_t(1));
	QVERIFY(Reloaded.GetMainModel()->GetPieces().empty());
	QCOMPARE(Reloaded.GetMainModel()->GetCameras().size(), size_t(1));
	QCOMPARE(Reloaded.GetEmbeddedData().size(), size_t(1));
	QCOMPARE(Reloaded.GetEmbeddedData().front()->RawBlock, DataBlock);
}

void lcModelInstanceTests::CompletedExportEvaluatesExternalModels()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	QFile MainFile(Directory.filePath("main.ldr"));
	QVERIFY(MainFile.open(QIODevice::WriteOnly));
	MainFile.write("0 Host\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 child.ldr\n");
	MainFile.close();

	QFile ChildFile(Directory.filePath("child.ldr"));
	QVERIFY(ChildFile.open(QIODevice::WriteOnly));
	ChildFile.write("0 Child\n0 !LEOCAD PIECE POSITION_KEY 1 0 0 0\n0 !LEOCAD PIECE POSITION_KEY 2 100 0 0\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 missing.dat\n");
	ChildFile.close();

	Project Host;
	QVERIFY(Host.Load(MainFile.fileName(), false));
	lcModel* External = Host.GetMainModel()->GetPieces()[0]->mPieceData->GetProject()->GetMainModel();
	QCOMPARE(External->GetCurrentStep(), lcStep(1));
	QCOMPARE(External->GetLastStep(), lcStep(2));
	QCOMPARE(External->GetEvaluatedStep(), lcStep(1));
	lcLoadDefaultColors(lcStudStyle::Plain);
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	External->GetPieces()[0]->mPieceData->SetMesh(Mesh);
	External->GetPieces()[0]->mPieceData->mState = lcAssetState::Loaded;

	const QString ExportName = Directory.filePath("completed.ldr");
	QVERIFY(Host.ExportMainModel(ExportName));

	QFile ExportFile(ExportName);
	QVERIFY(ExportFile.open(QIODevice::ReadOnly));
	QVERIFY(ExportFile.readAll().contains("1 16 100 0 0 1 0 0 0 1 0 0 0 1 missing.dat"));
	QCOMPARE(External->GetCurrentStep(), lcStep(1));
	QCOMPARE(External->GetEvaluatedStep(), lcStep(1));
	QCOMPARE(External->GetPieces()[0]->mModelWorld.GetTranslation().x, 0.0f);

	const QString OBJName = Directory.filePath("completed.obj");
	QVERIFY(Host.ExportWavefront(OBJName));
	QFile OBJFile(OBJName);
	QVERIFY(OBJFile.open(QIODevice::ReadOnly));
	QVERIFY(OBJFile.readAll().contains("v 90.00 "));
	QCOMPARE(External->GetEvaluatedStep(), lcStep(1));
}

void lcModelInstanceTests::TexturedDirectGeometry3DS()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	QFile MainFile(Directory.filePath("main.ldr"));
	QVERIFY(MainFile.open(QIODevice::WriteOnly));
	MainFile.write("0 Direct\n3 16 0 0 0 1 0 0 0 1 0\n");
	MainFile.close();

	Project Host;
	QVERIFY(Host.Load(MainFile.fileName(), false));
	lcLoadDefaultColors(lcStudStyle::Plain);
	lcModel* Main = Host.GetMainModel();
	QVERIFY(Main->HasDirectGeometry());
	Main->WaitForAssets();
	QVERIFY(Main->GetPieceData()->GetMesh());

	lcMesh* Mesh = new lcMesh;
	quint16 Sections[LC_NUM_MESH_LODS] = { 2, 0 };
	Mesh->Create(Sections, 3, 3, 0, 6);

	lcVertex* Verts = static_cast<lcVertex*>(Mesh->mVertexData);
	Verts[0].Position = lcVector3(0, 0, 0);
	Verts[1].Position = lcVector3(1, 0, 0);
	Verts[2].Position = lcVector3(0, 1, 0);

	lcVertexTextured* TexturedVerts = reinterpret_cast<lcVertexTextured*>(Verts + 3);
	TexturedVerts[0].Position = lcVector3(0, 0, 1);
	TexturedVerts[1].Position = lcVector3(1, 0, 1);
	TexturedVerts[2].Position = lcVector3(0, 1, 1);

	quint16* Indices = static_cast<quint16*>(Mesh->mIndexData);
	for (int Index = 0; Index < 6; Index++)
		Indices[Index] = Index % 3;

	for (int Section = 0; Section < 2; Section++)
	{
		lcMeshSection& MeshSection = Mesh->mLods[LC_MESH_LOD_HIGH].Sections[Section];
		MeshSection.ColorIndex = gDefaultColor;
		MeshSection.DrawOffset = Section * 3 * sizeof(quint16);
		MeshSection.DrawCount = 3;
		MeshSection.PrimitiveType = Section ? LC_MESH_TEXTURED_TRIANGLES : LC_MESH_TRIANGLES;
	}

	Main->GetPieceData()->SetMesh(Mesh);
	Main->GetPieceData()->mState = lcAssetState::Loaded;

	const QString ExportName = Directory.filePath("direct.3ds");
	QVERIFY(Host.Export3DStudio(ExportName));

	QFile ExportFile(ExportName);
	QVERIFY(ExportFile.open(QIODevice::ReadOnly));
	const QByteArray Bytes = ExportFile.readAll();
	const auto ReadU16 = [&Bytes](qsizetype Offset)
	{
		return quint16(quint8(Bytes.at(Offset)) | (quint16(quint8(Bytes.at(Offset + 1))) << 8));
	};
	const auto ReadU32 = [&ReadU16](qsizetype Offset)
	{
		return quint32(ReadU16(Offset)) | (quint32(ReadU16(Offset + 2)) << 16);
	};

	const qsizetype PointChunk = Bytes.indexOf(QByteArray::fromHex("1041"));
	QVERIFY(PointChunk >= 0);
	QVERIFY(Bytes.size() >= PointChunk + 8);
	QCOMPARE(ReadU32(PointChunk + 2), quint32(8 + 12 * 6));
	QCOMPARE(ReadU16(PointChunk + 6), quint16(6));

	const qsizetype FaceChunk = Bytes.indexOf(QByteArray::fromHex("2041"), PointChunk + ReadU32(PointChunk + 2));
	QVERIFY(FaceChunk >= 0);
	QVERIFY(Bytes.size() >= FaceChunk + 24);
	QCOMPARE(ReadU16(FaceChunk + 6), quint16(2));
	for (int Index = 0; Index < 3; Index++)
	{
		QCOMPARE(ReadU16(FaceChunk + 8 + Index * 2), quint16(Index));
		QCOMPARE(ReadU16(FaceChunk + 16 + Index * 2), quint16(Index + 3));
	}
}

void lcModelInstanceTests::RepeatedInstances()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	QFile File(Directory.filePath("instances.mpd"));
	QVERIFY(File.open(QIODevice::WriteOnly));

	const QByteArray Fixture =
		"0 FILE main.ldr\n"
		"0 !LEOCAD CAMERA POSITION -500 0 0\n"
		"0 !LEOCAD CAMERA TARGET_POSITION -500 0 -100\n"
		"0 !LEOCAD CAMERA UP_VECTOR 0 1 0\n"
		"0 !LEOCAD CAMERA NAME Parent camera\n"
		"0 !LEOCAD LIGHT POSITION -500 -200 0\n"
		"0 !LEOCAD LIGHT TYPE POINT NAME Parent light\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 a.ldr\n"
		"1 16 1000 0 0 1 0 0 0 1 0 0 0 1 a.ldr\n"
		"0 FILE a.ldr\n"
		"0 !LEOCAD PIECE POSITION_KEY 1 100 0 0\n"
		"0 !LEOCAD PIECE POSITION_KEY 2 200 0 0\n"
		"1 16 100 0 0 1 0 0 0 1 0 0 0 1 b.ldr\n"
		"1 16 500 0 0 1 0 0 0 1 0 0 0 1 b.ldr\n"
		"0 !LEOCAD PIECE POSITION_KEY 1 -100 0 0\n"
		"0 !LEOCAD PIECE POSITION_KEY 2 -200 0 0\n"
		"1 4 -100 0 0 1 0 0 0 1 0 0 0 1 3001.dat\n"
		"0 STEP\n"
		"1 7 0 -100 0 1 0 0 0 1 0 0 0 1 3002.dat\n"
		"0 FILE b.ldr\n"
		"0 !LEOCAD CAMERA POSITION -300 200 0\n"
		"0 !LEOCAD CAMERA TARGET_POSITION -300 200 -100\n"
		"0 !LEOCAD CAMERA UP_VECTOR 0 1 0\n"
		"0 !LEOCAD CAMERA NAME Test camera\n"
		"0 !LEOCAD LIGHT POSITION -300 -200 0\n"
		"0 !LEOCAD LIGHT TYPE POINT NAME Test light\n"
		"0 !LEOCAD PIECE POSITION_KEY 1 10 0 0\n"
		"0 !LEOCAD PIECE POSITION_KEY 2 30 0 0\n"
		"1 1 10 0 0 1 0 0 0 1 0 0 0 1 3003.dat\n"
		"0 !LEOCAD PIECE STEP_HIDE 2\n"
		"1 2 60 0 0 1 0 0 0 1 0 0 0 1 3004.dat\n"
		"0 STEP\n"
		"1 14 90 0 0 1 0 0 0 1 0 0 0 1 3005.dat\n";

	QCOMPARE(File.write(Fixture), qint64(Fixture.size()));
	File.close();

	lcApplication* Application = gApplication;
	Application->mProject = new Project();
	Project* Project = Application->mProject;
	QVERIFY(Project->Load(File.fileName(), false));

	lcLoadDefaultColors(lcStudStyle::Plain);

	for (const std::unique_ptr<lcModel>& Model : Project->GetModels())
	{
		Model->GetPieceData()->mState = lcAssetState::Loaded;

		for (const std::unique_ptr<lcPiece>& Piece : Model->GetPieces())
		{
			if (Piece->mPieceData->IsModel() || Piece->mPieceData->GetMesh())
				continue;

			lcMesh* Mesh = new lcMesh;
			Mesh->CreateBox();
			Piece->mPieceData->SetMesh(Mesh);
			Piece->mPieceData->mState = lcAssetState::Loaded;
		}
	}

	lcModel* Main = Project->GetModels()[0].get();
	lcModel* A = Project->GetModels()[1].get();
	lcModel* B = Project->GetModels()[2].get();
	lcPiece* FirstA = Main->GetPieces()[0].get();
	lcPiece* SecondA = Main->GetPieces()[1].get();
	lcPiece* FirstB = A->GetPieces()[0].get();
	lcPiece* Leaf = B->GetPieces()[0].get();
	B->GetLights()[0]->SetSelected(true);

	lcMainWindow* Window = new lcMainWindow();
	Window->CreateWidgets();
	Window->SetCurrentModelTab(Main, false);

	lcView* View = Window->GetActiveView();
	lcModelInstanceTestScene Scene;

	const auto Render = [&](const std::vector<lcPiece*>& EditingPath)
	{
		struct ModelState
		{
			lcModel* Model;
			lcStep Step;
			bool Active;
			std::vector<lcMatrix44> Matrices;
		};

		std::vector<ModelState> Before;

		for (const std::unique_ptr<lcModel>& Model : Project->GetModels())
		{
			ModelState State{ Model.get(), Model->GetCurrentStep(), Model->IsActive(), {} };

			for (const std::unique_ptr<lcPiece>& Piece : Model->GetPieces())
				State.Matrices.push_back(Piece->mModelWorld);

			Before.push_back(std::move(State));
		}

		Scene.Begin(lcMatrix44Identity());
		Scene.SetDrawInterface(true);
		Scene.SetModelEditingPath(EditingPath, View->GetActiveSubmodelTransform());
		Main->GetScene(&Scene, View->GetCamera(), false, false);
		Scene.End();

		for (const ModelState& State : Before)
		{
			QCOMPARE(State.Model->GetCurrentStep(), State.Step);
			QCOMPARE(State.Model->IsActive(), State.Active);

			for (size_t Index = 0; Index < State.Matrices.size(); Index++)
				QVERIFY(State.Model->GetPieces()[Index]->mModelWorld == State.Matrices[Index]);
		}
	};

	const auto HasMesh = [&](float X, float Z, lcRenderMeshState State)
	{
		return std::any_of(Scene.GetMeshes().begin(), Scene.GetMeshes().end(), [=](const lcRenderMesh& Mesh)
		{
			return fabsf(Mesh.WorldMatrix[3].x - X) < 0.001f && fabsf(Mesh.WorldMatrix[3].z - Z) < 0.001f && Mesh.State == State;
		});
	};

	FirstA->SetFocused(LC_PIECE_SECTION_POSITION, true);
	Window->HandleCommand(LC_PIECE_EDIT_SELECTED_SUBMODEL);
	Window->HandleCommand(LC_VIEW_TIME_FIRST);
	Render({ FirstA });

	QCOMPARE(A->GetCurrentStep(), lcStep(1));
	QVERIFY(HasMesh(-100, 0, lcRenderMeshState::Default));
	QVERIFY(HasMesh(130, 0, lcRenderMeshState::Default));
	QVERIFY(!HasMesh(0, 100, lcRenderMeshState::Default));
	QVERIFY(HasMesh(800, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(1230, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(1000, 100, lcRenderMeshState::Faded));

	FirstB->SetFocused(LC_PIECE_SECTION_POSITION, true);
	Window->HandleCommand(LC_PIECE_EDIT_SELECTED_SUBMODEL);
	Window->HandleCommand(LC_VIEW_TIME_FIRST);
	Leaf->SetFocused(LC_PIECE_SECTION_POSITION, true);
	Render({ FirstA, FirstB });

	QVERIFY(HasMesh(110, 0, lcRenderMeshState::Focused));
	QVERIFY(HasMesh(160, 0, lcRenderMeshState::Default));
	QVERIFY(!HasMesh(190, 0, lcRenderMeshState::Default));
	QVERIFY(HasMesh(530, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(1230, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(1530, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(-100, 0, lcRenderMeshState::Faded));
	QVERIFY(!HasMesh(-200, 0, lcRenderMeshState::Faded));
	QCOMPARE(Scene.GetInterfaceObjects().size(), size_t(3));
	QCOMPARE(Scene.GetInterfaceObjects()[0], static_cast<const lcObject*>(Leaf));
	QCOMPARE(FirstB->mModelWorld[3].x, 100.0f);
	QCOMPARE(Leaf->mModelWorld[3].x, 10.0f);

	// Rendering evaluates keys even if shared caches represent a different step.
	FirstB->UpdatePosition(LC_STEP_MAX);
	Leaf->UpdatePosition(LC_STEP_MAX);
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(110, 0, lcRenderMeshState::Focused));
	QVERIFY(HasMesh(1230, 0, lcRenderMeshState::Faded));
	QCOMPARE(FirstB->mModelWorld[3].x, 200.0f);
	QCOMPARE(Leaf->mModelWorld[3].x, 30.0f);
	FirstB->UpdatePosition(A->GetCurrentStep());
	Leaf->UpdatePosition(B->GetCurrentStep());

	// Mouse queries still operate in the chosen instance's local coordinates.
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(600, 0, 2000), lcVector3(600, 0, 0), lcVector3(0, 1, 0));
	View->SetCamera(&Camera, true);

	const lcVector3 ActivePoint = View->ProjectPoint(lcVector3(110, 0, 0));
	View->SetMousePosition(qRound(ActivePoint.x), qRound(ActivePoint.y));
	QCOMPARE(View->RayTest(true, false).ObjectSection.Object, static_cast<lcObject*>(Leaf));

	const lcVector3 OtherPoint = View->ProjectPoint(lcVector3(1230, 0, 0));
	View->SetMousePosition(qRound(OtherPoint.x), qRound(OtherPoint.y));
	QVERIFY(!View->RayTest(true, false).ObjectSection.Object);
	QVERIFY(View->FindObjectsInBox(OtherPoint.x - 8, OtherPoint.y - 8, OtherPoint.x + 8, OtherPoint.y + 8).empty());
	QVERIFY(!View->FindObjectsInBox(ActivePoint.x - 8, ActivePoint.y - 8, ActivePoint.x + 8, ActivePoint.y + 8).empty());

	QCOMPARE(B->GetCameras().size(), size_t(1));
	QCOMPARE(B->GetLights().size(), size_t(1));

	lcCamera* ModelCamera = B->GetCameras()[0].get();
	lcLight* ModelLight = B->GetLights()[0].get();
	const std::vector<lcObject*> InterfaceObjects = { ModelCamera, ModelLight };

	for (lcObject* Object : InterfaceObjects)
	{
		const lcVector3 WorldPosition = lcMul31(Object->GetSectionPosition(0), View->GetActiveSubmodelTransform());
		const lcVector3 Point = View->ProjectPoint(WorldPosition);

		View->SetMousePosition(qRound(Point.x), qRound(Point.y));
		QCOMPARE(View->RayTest(false, false).ObjectSection.Object, Object);
		QVERIFY(!View->RayTest(true, false).ObjectSection.Object);

		const std::vector<lcObject*> Objects = View->FindObjectsInBox(Point.x - 12, Point.y - 12, Point.x + 12, Point.y + 12);
		QVERIFY(std::find(Objects.begin(), Objects.end(), Object) != Objects.end());
		QCOMPARE(std::count(Scene.GetInterfaceObjects().begin(), Scene.GetInterfaceObjects().end(), Object), std::ptrdiff_t(1));
	}

	FirstA->SetRotation(lcMatrix33RotationZ(LC_PI / 2.0f), 1, true);
	FirstA->UpdatePosition(1);
	View->RestoreModelEditingContext();
	Render({ FirstA, FirstB });

	for (lcObject* Object : InterfaceObjects)
	{
		const lcVector3 WorldPosition = lcMul31(Object->GetSectionPosition(0), View->GetActiveSubmodelTransform());
		const lcVector3 Point = View->ProjectPoint(WorldPosition);

		View->SetMousePosition(qRound(Point.x), qRound(Point.y));
		QCOMPARE(View->RayTest(false, false).ObjectSection.Object, Object);

		const std::vector<lcObject*> Objects = View->FindObjectsInBox(Point.x - 12, Point.y - 12, Point.x + 12, Point.y + 12);
		QVERIFY(std::find(Objects.begin(), Objects.end(), Object) != Objects.end());
	}

	FirstA->SetRotation(lcMatrix33Identity(), 1, true);
	FirstA->UpdatePosition(1);
	View->RestoreModelEditingContext();

	Window->SetCurrentModelTab(B, false);

	lcView* ModelView = Window->GetActiveView();
	ModelView->SetSize(1200, 600);
	ModelView->SetCamera(&Camera, true);

	for (lcObject* Object : InterfaceObjects)
	{
		const lcVector3 Point = ModelView->ProjectPoint(Object->GetSectionPosition(0));

		ModelView->SetMousePosition(qRound(Point.x), qRound(Point.y));
		QCOMPARE(ModelView->RayTest(false, false).ObjectSection.Object, Object);

		const std::vector<lcObject*> Objects = ModelView->FindObjectsInBox(Point.x - 12, Point.y - 12, Point.x + 12, Point.y + 12);
		QVERIFY(std::find(Objects.begin(), Objects.end(), Object) != Objects.end());
	}

	ModelCamera->SetFocused(LC_CAMERA_SECTION_POSITION, true);
	ModelLight->SetSelected(true);
	Window->SetCurrentModelTab(Main, false);
	QCOMPARE(Window->GetActiveView(), View);
	QVERIFY(ModelCamera->IsSelected());
	QVERIFY(ModelCamera->IsFocused());
	QVERIFY(ModelLight->IsSelected());

	const auto DialogContains = [&](lcObject* Object)
	{
		lcSelectDialog Dialog(Window, B);
		QTreeWidget* Tree = Dialog.findChild<QTreeWidget*>();

		for (QTreeWidgetItemIterator Item(Tree); *Item; ++Item)
			if ((*Item)->data(0, lcSelectDialog::IndexRole).value<uintptr_t>() == reinterpret_cast<uintptr_t>(Object))
				return true;

		return false;
	};

	QVERIFY(DialogContains(ModelCamera));
	QVERIFY(DialogContains(ModelLight));
	QVERIFY(DialogContains(Leaf));

	ModelCamera->SetHidden(true);
	lcLightHistoryState HiddenLight = ModelLight->GetHistoryState();
	HiddenLight.Hidden = true;
	ModelLight->SetHistoryState(HiddenLight);
	Render({ FirstA, FirstB });
	QCOMPARE(Scene.GetInterfaceObjects().size(), size_t(1));
	QVERIFY(!DialogContains(ModelCamera));
	QVERIFY(!DialogContains(ModelLight));
	ModelCamera->SetHidden(false);
	HiddenLight.Hidden = false;
	ModelLight->SetHistoryState(HiddenLight);
	B->SetSelectionAndFocusAction({ Leaf }, Leaf, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	// Edits at the same step also appear immediately in every rendered instance.
	if (!Window->GetAddKeys())
		Window->HandleCommand(LC_VIEW_TIME_ADD_KEYS);

	Leaf->SetSelected(true);
	B->MoveSelectedObjects(lcVector3(10, 0, 0), lcModelTransformFlag::Checkpoint, lcModelHistoryEditMerge::None);
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(120, 0, lcRenderMeshState::Focused));
	QVERIFY(HasMesh(1230, 0, lcRenderMeshState::Faded));
	QCOMPARE(Leaf->mModelWorld[3].x, 20.0f);

	B->UndoAction();
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(110, 0, lcRenderMeshState::Focused));
	B->RedoAction();
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(120, 0, lcRenderMeshState::Focused));

	const int SharedColor = lcGetColorIndex(6);
	B->SetObjectsProperty({ Leaf }, lcObjectPropertyId::PieceColor, SharedColor);
	Render({ FirstA, FirstB });
	QCOMPARE(std::count_if(Scene.GetMeshes().begin(), Scene.GetMeshes().end(), [SharedColor](const lcRenderMesh& Mesh)
	{
		return Mesh.ColorIndex == SharedColor;
	}), std::ptrdiff_t(4));

	Leaf->SetRotation(lcMatrix33Identity(), 1, true);
	Leaf->SetRotation(lcMatrix33RotationZ(LC_PI / 2.0f), 2, true);
	B->CalculateStep(1);
	Render({ FirstA, FirstB });
	QCOMPARE(std::count_if(Scene.GetMeshes().begin(), Scene.GetMeshes().end(), [SharedColor](const lcRenderMesh& Mesh)
	{
		return Mesh.ColorIndex == SharedColor && fabsf(Mesh.WorldMatrix[0].x) < 0.001f;
	}), std::ptrdiff_t(3));
	QCOMPARE(Leaf->mModelWorld[0].x, 1.0f);

	Window->HandleCommand(LC_VIEW_TIME_LAST);
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(130, 0, lcRenderMeshState::Focused));
	QVERIFY(HasMesh(190, 0, lcRenderMeshState::Default));
	QVERIFY(!HasMesh(160, 0, lcRenderMeshState::Default));
	QCOMPARE(A->GetCurrentStep(), lcStep(1));

	Window->SetCurrentModelTab(B, false);
	Window->HandleCommand(LC_VIEW_TIME_FIRST);
	Window->SetCurrentModelTab(Main, false);
	Render({ FirstA, FirstB });
	QVERIFY(HasMesh(120, 0, lcRenderMeshState::Focused));
	QCOMPARE(View->GetActiveSubmodelTransform()[3].x, 100.0f);

	View->SetSubmodelActive(1);
	Render({ FirstA });
	QVERIFY(HasMesh(130, 0, lcRenderMeshState::Focused));
	QCOMPARE(A->GetCurrentStep(), lcStep(1));

	View->SetTopSubmodelActive();
	FirstB->SetFocused(LC_PIECE_SECTION_POSITION, false);
	FirstB->SetSelected(false);
	FirstA->SetFocused(LC_PIECE_SECTION_POSITION, false);
	SecondA->SetFocused(LC_PIECE_SECTION_POSITION, true);
	Window->HandleCommand(LC_PIECE_EDIT_SELECTED_SUBMODEL);
	Window->HandleCommand(LC_VIEW_TIME_FIRST);
	Render({ SecondA });
	QVERIFY(HasMesh(-200, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(230, 0, lcRenderMeshState::Faded));
	QVERIFY(HasMesh(900, 0, lcRenderMeshState::Default));
	QVERIFY(HasMesh(1130, 0, lcRenderMeshState::Default));

	A->InsertCameraToolClicked(lcVector3(200, 200, 100));
	Render({ SecondA });
	QCOMPARE(A->GetCameras().size(), size_t(1));
	QCOMPARE(A->GetFocusObject(), static_cast<lcObject*>(A->GetCameras()[0].get()));
	QVERIFY(std::find(Scene.GetInterfaceObjects().begin(), Scene.GetInterfaceObjects().end(), A->GetCameras()[0].get()) != Scene.GetInterfaceObjects().end());
	A->UndoAction();
	QVERIFY(A->GetCameras().empty());
	A->RedoAction();
	Render({ SecondA });
	QVERIFY(A->GetCameras()[0]->IsFocused());
	QVERIFY(std::find(Scene.GetInterfaceObjects().begin(), Scene.GetInterfaceObjects().end(), A->GetCameras()[0].get()) != Scene.GetInterfaceObjects().end());

	A->InsertLightToolClicked(lcVector3(200, -200, 100), lcLightType::Point);
	Render({ SecondA });
	QCOMPARE(A->GetLights().size(), size_t(1));
	QVERIFY(A->GetLights()[0]->IsFocused());
	QVERIFY(std::find(Scene.GetInterfaceObjects().begin(), Scene.GetInterfaceObjects().end(), A->GetLights()[0].get()) != Scene.GetInterfaceObjects().end());
	A->UndoAction();
	QVERIFY(A->GetLights().empty());
	A->RedoAction();
	Render({ SecondA });
	QVERIFY(A->GetLights()[0]->IsFocused());
	QVERIFY(std::find(Scene.GetInterfaceObjects().begin(), Scene.GetInterfaceObjects().end(), A->GetLights()[0].get()) != Scene.GetInterfaceObjects().end());
	CameraContexts(Window, View, Main, A, B);

}

void lcModelInstanceTests::CameraContexts(lcMainWindow* Window, lcView* View, lcModel* Main, lcModel* A, lcModel* B)
{
	const auto SameMatrix = [](const lcMatrix44& First, const lcMatrix44& Second)
	{
		for (int Row = 0; Row < 4; Row++)
			for (int Column = 0; Column < 4; Column++)
				if (fabsf(First[Row][Column] - Second[Row][Column]) > 0.001f)
					return false;

		return true;
	};

	const auto CheckMenu = [&](lcCamera* Camera)
	{
		QVERIFY(QMetaObject::invokeMethod(Window, "CameraMenuAboutToShow", Qt::DirectConnection));
		QVERIFY(Window->mActions[LC_VIEW_CAMERA_FIRST]->isVisible());
		QCOMPARE(Window->mActions[LC_VIEW_CAMERA_FIRST]->text(), Camera->GetName());
	};

	View->SetTopSubmodelActive();

	lcCamera* ParentCamera = Main->GetCameras()[0].get();
	QVERIFY(ParentCamera->SetName(QStringLiteral("Parent cam\u00e9ra")));

	const lcCameraHistoryState ParentBefore = ParentCamera->GetHistoryState();
	lcPiece* AInstance = Main->GetPieces()[1].get();
	lcPiece* BInstance = A->GetPieces()[0].get();

	View->SetCameraIndex(0);
	QCOMPARE(View->GetCamera(), ParentCamera);
	CheckMenu(ParentCamera);

	const lcMatrix44 ParentView = View->GetWorldViewMatrix();
	Main->SetSelectionAndFocusAction({ AInstance }, AInstance, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	QCOMPARE(View->GetActiveModel(), A);
	QVERIFY(View->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), ParentView));

	lcCamera* ACamera = A->GetCameras()[0].get();
	CheckMenu(ACamera);
	View->SetCamera(ACamera->GetName());
	QCOMPARE(View->GetCamera(), ACamera);
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), lcMul(lcMatrix44AffineInverse(View->GetActiveSubmodelTransform()), ACamera->mWorldView)));

	View->SetCameraProjection(lcCameraProjection::Orthographic);
	QCOMPARE(ACamera->GetProjection(), lcCameraProjection::Orthographic);
	QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
	A->UndoAction();
	QCOMPARE(ACamera->GetProjection(), lcCameraProjection::Perspective);
	QCOMPARE(View->GetCamera(), ACamera);

	const lcCameraHistoryState ABeforeZoom = ACamera->GetHistoryState();
	View->SetMouseModifiers(Qt::NoModifier);
	View->OnMouseWheel(1.0f);
	QVERIFY(!(ACamera->GetHistoryState() == ABeforeZoom));
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == ABeforeZoom);
	QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);

	const lcMatrix44 AView = View->GetWorldViewMatrix();
	A->SetSelectionAndFocusAction({ BInstance }, BInstance, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	QCOMPARE(View->GetActiveModel(), B);
	QVERIFY(View->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), AView));

	AInstance->SetRotation(lcMatrix33RotationZ(LC_PI / 2.0f), 1, true);
	AInstance->UpdatePosition(1);
	View->RestoreModelEditingContext();

	lcCamera* BCamera = B->GetCameras()[0].get();
	BCamera->SetPosition(lcVector3(-300, 200, 200), 1, true);
	BCamera->SetTargetPosition(lcVector3(-300, 200, 0), 1, true);
	BCamera->SetPosition(lcVector3(300, 200, 300), 2, true);
	BCamera->SetTargetPosition(lcVector3(300, 200, 0), 2, true);
	B->SetTemporaryStep(1);
	CheckMenu(BCamera);
	View->SetCameraIndex(0);
	QCOMPARE(View->GetCamera(), BCamera);

	const lcMatrix44 ExpectedView = lcMul(lcMatrix44AffineInverse(View->GetActiveSubmodelTransform()), BCamera->mWorldView);
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), ExpectedView));
	View->SetSize(1200, 600);

	const lcVector3 Target = lcMul31(BCamera->mTargetPosition, View->GetActiveSubmodelTransform());
	const lcVector3 Projected = View->ProjectPoint(Target);
	QVERIFY(fabsf(Projected.x - 600.0f) < 0.01f);
	QVERIFY(fabsf(Projected.y - 300.0f) < 0.01f);
	QVERIFY(lcLength(View->UnprojectPoint(Projected) - Target) < 0.1f);

	// A new root view must copy the world pose, rather than attach a foreign camera.
	{
		lcView OtherView(lcViewType::View, Main);
		QVERIFY(OtherView.GetCamera()->IsSimple());
		QVERIFY(SameMatrix(OtherView.GetWorldViewMatrix(), View->GetWorldViewMatrix()));
	}

	B->ShowNextStep();
	QVERIFY(!SameMatrix(View->GetWorldViewMatrix(), ExpectedView));
	B->ShowFirstStep();
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), ExpectedView));

	// Copying a camera must use its editing keys even during final-state export.
	BCamera->UpdatePosition(LC_STEP_MAX);
	lcCamera Copy(true);
	View->CopyCameraPosition(&Copy);
	QVERIFY(SameMatrix(Copy.mWorldView, ExpectedView));
	BCamera->UpdatePosition(1);

	View->SetSubmodelActive(1);
	QCOMPARE(View->GetActiveModel(), A);
	QVERIFY(View->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), ExpectedView));
	CheckMenu(ACamera);
	View->SetTopSubmodelActive();
	QVERIFY(View->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), ExpectedView));
	CheckMenu(ParentCamera);

	Main->SetSelectionAndFocusAction({ AInstance }, AInstance, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	A->SetSelectionAndFocusAction({ BInstance }, BInstance, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	B->ShowFirstStep();
	View->SetCameraIndex(0);
	const lcMatrix44 BeforeDelete = View->GetWorldViewMatrix();

	Window->SetCurrentModelTab(B, false);
	lcView* BView = Window->GetActiveView();
	BView->SetCameraIndex(0);
	const lcMatrix44 BTabView = BView->GetWorldViewMatrix();
	Window->SetCurrentModelTab(Main, false);
	QCOMPARE(Window->GetActiveView(), View);
	QCOMPARE(View->GetCamera(), BCamera);

	B->EraserToolClicked(BCamera);
	QVERIFY(B->GetCameras().empty());
	QVERIFY(View->GetCamera()->IsSimple());
	QVERIFY(BView->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(View->GetWorldViewMatrix(), BeforeDelete));
	QVERIFY(SameMatrix(BView->GetWorldViewMatrix(), BTabView));
	B->UndoAction();
	QCOMPARE(B->GetCameras().size(), size_t(1));
	View->SetCameraIndex(0);
	CheckMenu(B->GetCameras()[0].get());

	QTemporaryDir ExportDirectory;
	QVERIFY(ExportDirectory.isValid());

	const auto CheckLightExport = [&](lcLight* Light, const lcMatrix44& Transform)
	{
		const QString FileName = ExportDirectory.filePath("lights.pov");
		QVERIFY(lcGetActiveProject()->ExportPOVRay(FileName));
		QFile File(FileName);
		QVERIFY(File.open(QIODevice::ReadOnly));
		const QByteArray Contents = File.readAll();
		const int Start = Contents.indexOf("  WriteLight(");
		QVERIFY(Start >= 0);
		const QString LightLine = QString::fromLatin1(Contents.mid(Start, Contents.indexOf('\n', Start) - Start));
		QRegularExpressionMatchIterator Vectors = QRegularExpression("<([^>]+)>").globalMatch(LightLine);
		std::vector<lcVector3> Exported;

		while (Vectors.hasNext())
		{
			const QStringList Components = Vectors.next().captured(1).split(',');
			QCOMPARE(Components.size(), 3);
			Exported.emplace_back(Components[1].toFloat(), Components[0].toFloat(), Components[2].toFloat());
		}

		QCOMPARE(Exported.size(), size_t(5));
		const lcVector3 Position = lcMul31(Light->GetPosition(), Transform);
		const lcVector3 Direction = lcMul30(Light->GetDirection(), Transform);
		const lcVector3 AreaX = lcMul30(lcVector3(Light->GetWorldMatrix()[0]) * Light->GetAreaSizeX(), Transform);
		const lcVector3 AreaY = lcMul30(lcVector3(Light->GetWorldMatrix()[1]) * Light->GetAreaSizeY(), Transform);
		QVERIFY(lcLength(Exported[0] - Position) < 0.01f);
		QVERIFY(lcLength(Exported[1] - Position - Direction) < 0.01f);
		QVERIFY(lcLength(Exported[3] - AreaX) < 0.01f);
		QVERIFY(lcLength(Exported[4] - AreaY) < 0.01f);
	};

	lcLight* EditedLight = B->GetLights()[0].get();
	EditedLight->SetRotation(lcMatrix33RotationX(0.5f), 1, false);
	EditedLight->UpdatePosition(1);

	for (lcLightType Type : { lcLightType::Point, lcLightType::Spot, lcLightType::Directional, lcLightType::Area })
	{
		EditedLight->SetLightType(Type);
		CheckLightExport(EditedLight, View->GetActiveSubmodelTransform());
	}

	View->SetCameraProjection(lcCameraProjection::Orthographic);
	const lcMatrix44 SavedView = View->GetWorldViewMatrix();
	const QByteArray Layout = Window->GetTabLayout();
	Window->RestoreTabLayout(Layout);
	lcView* RestoredView = Window->GetActiveView();
	QCOMPARE(RestoredView->GetActiveModel(), Main);
	QVERIFY(RestoredView->GetCamera()->IsSimple());
	QVERIFY(SameMatrix(RestoredView->GetWorldViewMatrix(), SavedView));
	QCOMPARE(RestoredView->GetCamera()->GetProjection(), lcCameraProjection::Orthographic);

	// Version 1 layouts remain readable when they contain perspective free views.
	RestoredView->SetCameraProjection(lcCameraProjection::Perspective);
	QByteArray LegacyLayout = Window->GetTabLayout();
	LegacyLayout.replace(0, 4, QByteArray::fromHex("00000001"));
	Window->RestoreTabLayout(LegacyLayout);
	QVERIFY(SameMatrix(Window->GetActiveView()->GetWorldViewMatrix(), SavedView));
	QCOMPARE(Window->GetActiveView()->GetCamera()->GetProjection(), lcCameraProjection::Perspective);

	lcView* TabView = Window->GetActiveView();
	TabView->SetCameraIndex(0);
	QCOMPARE(TabView->GetCamera(), ParentCamera);
	const QByteArray NamedLayout = Window->GetTabLayout();
	Window->RestoreTabLayout(NamedLayout);
	QCOMPARE(Window->GetActiveView()->GetActiveModel(), Main);
	QCOMPARE(Window->GetActiveView()->GetCamera(), ParentCamera);
	QVERIFY(!Window->GetActiveView()->GetCamera()->IsSimple());
	QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
	CheckLightExport(Main->GetLights()[0].get(), lcMatrix44Identity());
}


void lcModelInstanceTests::InterfaceTransforms()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test interface drawing.");

	const lcResult<void> Renderer = lcContext::InitializeRenderer(true);
	QVERIFY(Renderer.has_value());

	lcModelInstanceTestContext Context;
	Context.SetOffscreenContext();
	Context.MakeCurrent();
	Context.SetDefaultState();

	QOpenGLFramebufferObject Framebuffer(64, 64, QOpenGLFramebufferObject::CombinedDepthStencil);
	QVERIFY(Framebuffer.isValid());
	QVERIFY(Framebuffer.bind());
	Context.SetViewport(0, 0, 64, 64);

	lcPiece Instance(nullptr);
	lcScene Scene;
	Scene.Begin(lcMatrix44Identity());
	Scene.SetDrawInterface(true);

	const lcMatrix44 ParentTransform = lcMul(lcMatrix44RotationZ(LC_PI / 2.0f), lcMatrix44Translation(lcVector3(400, -200, 50)));
	Scene.SetModelEditingPath({ &Instance }, ParentTransform);

	lcCamera Camera(false, lcVector3(20, 30, 40), lcVector3(50, 60, 0));
	Camera.SetSelected(true);
	Camera.DrawInterface(&Context, Scene);

	lcMatrix44 CameraMatrix = lcMatrix44AffineInverse(Camera.mWorldView);
	CameraMatrix.SetTranslation(Camera.GetSectionPosition(LC_CAMERA_SECTION_POSITION));
	QVERIFY(Context.GetWorldMatrix() == lcMul(CameraMatrix, ParentTransform));

	for (lcLightType Type : { lcLightType::Point, lcLightType::Spot, lcLightType::Directional, lcLightType::Area })
	{
		lcLight Light(lcVector3(20, 30, 40), Type);
		Light.SetSelected(true);
		Light.DrawInterface(&Context, Scene);
		QVERIFY(Context.GetWorldMatrix() == lcMul(Light.GetWorldMatrix(), ParentTransform));
	}

	Scene.SetModelEditingPath({}, lcMatrix44Identity());
	Camera.DrawInterface(&Context, Scene);
	QVERIFY(Context.GetWorldMatrix() == CameraMatrix);

	lcLight Light(lcVector3(20, 30, 40), lcLightType::Point);
	Light.DrawInterface(&Context, Scene);
	QVERIFY(Context.GetWorldMatrix() == Light.GetWorldMatrix());

	Scene.End();
	QVERIFY(Framebuffer.release());
}

int main(int argc, char** argv)
{
	QTemporaryDir SettingsDirectory;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, SettingsDirectory.path());
	QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, SettingsDirectory.path());

	lcApplication Application(argc, argv);
	QCoreApplication::setOrganizationName("LeoCAD Tests");
	QCoreApplication::setApplicationName("Model Instances");
	Application.mLibrary = new lcPiecesLibrary(lcStudStyle::Plain, false);

	lcModelInstanceTests Tests;
	const int Result = QTest::qExec(&Tests, argc, argv);

	Application.Shutdown();

	return Result;
}

#include "lc_modelinstance_test.moc"
