#include "lc_global.h"
#include "lc_application.h"
#include "lc_commandline.h"
#include "lc_colors.h"
#include "lc_context.h"
#include "lc_findreplacewidget.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "lc_partselectionwidget.h"
#include "lc_partselectionpopup.h"
#include "lc_profile.h"
#include "lc_model.h"
#include "lc_modelhistory.h"
#include "lc_meshloader.h"
#include "lc_file.h"
#include "lc_piecedata.h"
#include "lc_scene.h"
#include "lc_selectdialog.h"
#include "lc_shortcuts.h"
#include "lc_thumbnailmanager.h"
#include "lc_texture.h"
#include "lc_view.h"
#include "lc_viewwidget.h"
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
	void RotateToolMouseShortcuts_data();
	void RotateToolMouseShortcuts();
	void RepeatedInstances();
	void InterfaceTransforms();
	void CommandLineBackground_data();
	void CommandLineBackground();
	void GradientBackground();
	void CommandLineOptions();
	void CommandLineParsing_data();
	void CommandLineParsing();
	void CommandLineColors_data();
	void CommandLineColors();
	void CommandLineColorValidation_data();
	void CommandLineColorValidation();
	void TranslucentThumbnails_data();
	void TranslucentThumbnails();
	void TextureDecalAlpha();
	void TextureMipmaps();
	void CompletedExportEvaluatesExternalModels();
	void ProjectFilesCategory();
	void ProjectFilesRecoveryAndCycles();
	void ProjectFilePopupOwnership();
	void ProjectFileInsertionValidation();
	void FindReplaceViewIsolation();
	void TexturedDirectGeometry3DS();

private:
	void SeparateRotations(lcMainWindow* Window, lcModel* Model);
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

	lcPieceDataRef Piece = Library->AcquirePieceData("test.dat", nullptr, lcAcquirePieceDataFlag::None);

	QVERIFY(Piece);
	QVERIFY(Library->EnsurePieceDataAssetsReady(Piece));
	QVERIFY(Piece->GetMesh());
	QCOMPARE(Piece->GetMesh()->mNumTexturedVertices, 0);
	QCOMPARE(Piece->GetMesh()->mBoundingBox.Max.x, 21.0f);

	// The shared primitive cache must still hold its original textured branch.
	lcLibraryPrimitive* Primitive = Library->FindPrimitive("MAPPED.DAT");

	QVERIFY(Primitive);
	QVERIFY(Primitive->mMeshData.mHasTextures);

	lcPieceDataRef Cycle = Library->AcquirePieceData("cycle.dat", nullptr, lcAcquirePieceDataFlag::None);

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

void lcModelInstanceTests::ProjectFilesCategory()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());

	const auto WriteFile = [&Directory](const QString& Name, const QByteArray& Data)
	{
		QFile File(Directory.filePath(Name));
		return File.open(QIODevice::WriteOnly) && File.write(Data) == Data.size();
	};

	QVERIFY(WriteFile("host.mpd", "0 FILE host.ldr\n0 Host\n0 FILE embedded.ldr\n0 Embedded child\n"));
	QVERIFY(WriteFile("child.ldr", "0 External child\n3 16 0 0 0 10 0 0 0 10 0\n"));
	QVERIFY(WriteFile("embedded.ldr", "0 Shadowed external child\n3 16 0 0 0 10 0 0 0 10 0\n"));
	QVERIFY(WriteFile("part.DAT", "0 Local part\n3 16 0 0 0 20 0 0 0 20 0\n"));
	QVERIFY(WriteFile("notes.txt", "Not a model"));
	QVERIFY(QDir(Directory.path()).mkdir("nested"));
	QVERIFY(WriteFile("nested/hidden.ldr", "0 Nested model\n"));
#ifdef Q_OS_UNIX
	QVERIFY(QFile::link(Directory.filePath("host.mpd"), Directory.filePath("alias.mpd")));
#endif

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;

	lcMainWindow Window;
	lcPartSelectionWidget Widget(false, nullptr);
	Widget.SetCategory(lcPartCategoryType::Files, 0);
	lcPartSelectionListModel* List = Widget.findChild<lcPartSelectionListModel*>();
	QVERIFY(List);
	QCOMPARE(List->rowCount(), 0);

	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd"), false));
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	QVERIFY(!Library->AcquirePieceData("child.ldr", &Scope.Current, lcAcquirePieceDataFlag::None));
	Widget.UpdateProjectCategories();
	QCOMPARE(List->rowCount(), 2);
	QCOMPARE(List->data(List->index(0, 0), Qt::DisplayRole).toString(), QString("child.ldr"));
	QVERIFY(List->GetPieceData(0)->IsProject());
	QCOMPARE(QFileInfo(List->GetPieceData(0)->GetProject()->GetFileName()).canonicalFilePath(),
		QFileInfo(Directory.filePath("child.ldr")).canonicalFilePath());
	QVERIFY(List->GetPieceData(0) != Scope.Current.GetModels()[1]->GetPieceData());

	// Case-sensitive filesystems can contain distinct files sharing one registry key.
	if (!QFileInfo(Directory.filePath("CHILD.ldr")).exists())
	{
		QVERIFY(WriteFile("CHILD.ldr", "0 Different child\n3 16 0 0 0 30 0 0 0 30 0\n"));
		Widget.UpdateProjectCategories();
		QCOMPARE(List->rowCount(), 2);

		for (int Row = 0; Row < List->rowCount(); Row++)
		{
			const QString FileName = List->data(List->index(Row, 0), Qt::DisplayRole).toString();
			QCOMPARE(QFileInfo(List->GetPieceData(Row)->GetProject()->GetFileName()).canonicalFilePath(),
				QFileInfo(Directory.filePath(FileName)).canonicalFilePath());
		}

		QVERIFY(QFile::remove(Directory.filePath("CHILD.ldr")));
		Widget.UpdateProjectCategories();
	}

	QTreeWidget* Categories = Widget.findChild<QTreeWidget*>();
	QVERIFY(Categories);
	QCOMPARE(Categories->topLevelItem(2)->text(0), QString("Submodels"));
	QCOMPARE(Categories->topLevelItem(3)->text(0), QString("Files"));

	// References selected from Files keep their relative disk identity on save/reload.
	std::unique_ptr<lcPiece> Piece = std::make_unique<lcPiece>(lcPieceDataRef(List->GetPieceData(0)));
	Piece->Initialize(lcMatrix44Identity(), 1);
	Scope.Current.GetMainModel()->AddPiece(std::move(Piece), 0);
	QCOMPARE(Scope.Current.GetMainModel()->GetPieces()[0]->GetID(), QString("child.ldr"));
	QVERIFY(Scope.Current.Save(Directory.filePath("host.mpd")));
	QFile SavedFile(Directory.filePath("host.mpd"));
	QVERIFY(SavedFile.open(QIODevice::ReadOnly));
	const QByteArray SavedData = SavedFile.readAll();
	QVERIFY(SavedData.contains(" child.ldr"));
	QVERIFY(!SavedData.contains("./"));
	Project Reloaded;
	QVERIFY(Reloaded.Load(Directory.filePath("host.mpd"), false));
	QCOMPARE(Reloaded.GetMainModel()->GetPieces().size(), size_t(1));
	QVERIFY(Reloaded.GetMainModel()->GetPieces()[0]->mPieceData->IsProject());
	QCOMPARE(Reloaded.GetMainModel()->GetPieces()[0]->GetID(), QString("child.ldr"));

	QVERIFY(WriteFile("new.mpd", "0 New file\n3 16 0 0 0 30 0 0 0 30 0\n"));
	QTRY_COMPARE(List->rowCount(), 3);
	QVERIFY(QFile::remove(Directory.filePath("new.mpd")));
	QTRY_COMPARE(List->rowCount(), 2);

	QTemporaryDir OtherDirectory;
	QVERIFY(OtherDirectory.isValid());
	QVERIFY(Scope.Current.Save(OtherDirectory.filePath("host.mpd")));
	Widget.UpdateProjectCategories();
	QCOMPARE(List->rowCount(), 0);
}

void lcModelInstanceTests::ProjectFilesRecoveryAndCycles()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkdir("nested"));

	const auto WriteFile = [&Directory](const QString& Name, const QByteArray& Data)
	{
		QFile File(Directory.filePath(Name));
		return File.open(QIODevice::WriteOnly) && File.write(Data) == Data.size();
	};

	QVERIFY(WriteFile("host.mpd", "0 FILE host.ldr\n0 Host\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 recovered.ldr\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 blocked.ldr\n"));
	QVERIFY(WriteFile("direct.ldr", "0 Direct cycle\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 host.mpd\n"));
	QVERIFY(WriteFile("indirect.mpd", "0 FILE root.ldr\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 branch.ldr\n0 FILE branch.ldr\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 nested/intermediate.ldr\n"));
	QVERIFY(WriteFile("nested/intermediate.ldr", "0 Intermediate\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 ../host.mpd\n"));
	QVERIFY(WriteFile("safe.ldr", "0 Safe\n3 16 0 0 0 10 0 0 0 10 0\n"));

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;

	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd"), false));
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	lcPieceData* Missing = Scope.Current.GetMainModel()->GetPieces()[0]->mPieceData.Get();
	lcPieceData* Blocked = Scope.Current.GetMainModel()->GetPieces()[1]->mPieceData.Get();
	QCOMPARE(Missing->mState, lcAssetState::Failed);
	QCOMPARE(Blocked->mState, lcAssetState::Failed);

	// The cycle check must also apply to identities already loaded in the project.
	lcPieceDataRef Cached = Library->AcquirePieceData("direct.ldr", &Scope.Current, lcAcquirePieceDataFlag::SearchProjectFolder);
	QVERIFY(Cached && Cached->IsProject());
	QVERIFY(!Library->AcquirePieceData("direct.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly));
	QVERIFY(!Library->AcquirePieceData("indirect.mpd", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly));

	lcMainWindow Window;
	lcPartSelectionWidget Widget(false, nullptr);
	Widget.SetCategory(lcPartCategoryType::Files, 0);
	lcPartSelectionListModel* List = Widget.findChild<lcPartSelectionListModel*>();
	QVERIFY(List);
	QCOMPARE(List->rowCount(), 1);
	QCOMPARE(QString::fromLatin1(List->GetPieceData(0)->mFileName), QString("safe.ldr"));

	QVERIFY(WriteFile("recovered.ldr", "0 Recovered\n3 16 0 0 0 20 0 0 0 20 0\n"));
	QVERIFY(WriteFile("blocked.ldr", "0 Blocked cycle\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 host.mpd\n"));
	QTRY_COMPARE(List->rowCount(), 2);
	QVERIFY(Missing->IsProject());
	QCOMPARE(Scope.Current.GetMainModel()->GetPieces()[0]->mPieceData.Get(), Missing);
	QCOMPARE(List->GetPieceData(0), Missing);
	QCOMPARE(Missing->mState, lcAssetState::Loaded);
	QVERIFY(!Blocked->IsProject());
	QCOMPARE(Blocked->mState, lcAssetState::Failed);

	// Retained identities must reload changed root files before checking dependencies.
	lcPieceDataRef Safe = Library->AcquirePieceData("safe.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly);
	QVERIFY(Safe);
	const Project* OriginalProject = Safe->GetProject();
	QVERIFY(WriteFile("safe.ldr", "0 Changed safe file\n3 16 0 0 0 30 0 0 0 30 0\n"));
	lcPieceDataRef Updated = Library->AcquirePieceData("safe.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly);
	QCOMPARE(Updated, Safe);
	QVERIFY(Updated->GetProject() != OriginalProject);
	QVERIFY(!Updated->GetProject()->HasFileChangedOnDisk());
	QVERIFY(WriteFile("safe.ldr", "0 Changed to cycle\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 host.mpd\n"));
	QVERIFY(!Library->AcquirePieceData("safe.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly));

	// An unchanged root can acquire a cycle through a changed external dependency.
	QVERIFY(WriteFile("reload.ldr", "0 Reload\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 nested/leaf.ldr\n"));
	QVERIFY(WriteFile("nested/leaf.ldr", "0 Leaf\n3 16 0 0 0 10 0 0 0 10 0\n"));
	lcPieceDataRef Root = Library->AcquirePieceData("reload.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly);
	QVERIFY(Root);
	QVERIFY(WriteFile("nested/leaf.ldr", "0 Changed leaf\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 ../host.mpd\n"));
	QVERIFY(!Library->AcquirePieceData("reload.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly));

	// Dependencies that were missing at load time must be revalidated when they appear.
	QVERIFY(WriteFile("missing-root.ldr", "0 Missing dependency\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 nested/new-leaf.ldr\n"));
	lcPieceDataRef MissingRoot = Library->AcquirePieceData("missing-root.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly);
	QVERIFY(MissingRoot);
	QVERIFY(WriteFile("nested/new-leaf.ldr", "0 New cycle\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 ../host.mpd\n"));
	QVERIFY(!Library->AcquirePieceData("missing-root.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly));
}

void lcModelInstanceTests::ProjectFilePopupOwnership()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile HostFile(Directory.filePath("host.ldr"));
	QVERIFY(HostFile.open(QIODevice::WriteOnly));
	HostFile.write("0 Host\n");
	HostFile.close();
	QFile ChildFile(Directory.filePath("child.ldr"));
	QVERIFY(ChildFile.open(QIODevice::WriteOnly));
	ChildFile.write("0 Child\n3 16 0 0 0 10 0 0 0 10 0\n");
	ChildFile.close();

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;
	QVERIFY(Scope.Current.Load(HostFile.fileName(), false));
	lcMainWindow Window;
	bool Picked = false;
	QTimer::singleShot(0, [&Picked]()
	{
		QMenu* Menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
		QVERIFY(Menu);
		lcPartSelectionPopup* Popup = Menu->findChild<lcPartSelectionPopup*>();
		QVERIFY(Popup);
		lcPartSelectionWidget* Picker = Popup->GetPartSelectionWidget();
		Picker->SetCategory(lcPartCategoryType::Files, 0);
		lcPartSelectionListModel* List = Picker->findChild<lcPartSelectionListModel*>();
		QCOMPARE(List->rowCount(), 1);
		Picker->SetCurrentPart(List->GetPieceData(0));
		Picked = QMetaObject::invokeMethod(Popup, "Accept", Qt::DirectConnection);
	});

	std::optional<lcPieceDataRef> Result = lcShowPartSelectionPopup(nullptr, {}, gDefaultColor, &Window, QPoint(0, 0));
	QVERIFY(Picked);
	QVERIFY(Result && Result.value());
	QVERIFY(Result.value()->IsProject());
	QCOMPARE(lcGetPiecesLibrary()->FindPieceData("child.ldr", &Scope.Current), Result.value().Get());
	QCOMPARE(Result.value()->GetProject()->GetFileName(), ChildFile.fileName());

	// Find/Replace keeps ownership after the popup result itself is released.
	lcFindReplaceParams Params;
	Params.ReplacePieceData = Result.value();
	Result.reset();
	QVERIFY(Params.ReplacePieceData->IsProject());
	QCOMPARE(lcGetPiecesLibrary()->FindPieceData("child.ldr", &Scope.Current), Params.ReplacePieceData.Get());
}

void lcModelInstanceTests::ProjectFileInsertionValidation()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkdir("nested"));
	const auto WriteFile = [&Directory](const QString& Name, const QByteArray& Data)
	{
		QFile File(Directory.filePath(Name));
		return File.open(QIODevice::WriteOnly) && File.write(Data) == Data.size();
	};
	QVERIFY(WriteFile("host.mpd", "0 FILE host.ldr\n0 Host\n"));
	QVERIFY(WriteFile("child.ldr", "0 Child\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 nested/leaf.ldr\n"));
	QVERIFY(WriteFile("nested/leaf.ldr", "0 Leaf\n3 16 0 0 0 10 0 0 0 10 0\n"));

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;
	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd"), false));
	lcModel* Model = Scope.Current.GetMainModel();
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	lcPieceDataRef Data = lcGetPiecesLibrary()->AcquirePieceData("child.ldr", &Scope.Current, lcAcquirePieceDataFlag::ProjectFileOnly);
	QVERIFY(Data);
	const Project* Original = Data->GetProject();

	Model->SetPreviewInsertPieceInfo({ { Data.Get(), lcMatrix44Identity(), gDefaultColor } });
	QVERIFY(WriteFile("nested/leaf.ldr", "0 Updated leaf\n3 16 0 0 0 20 0 0 0 20 0\n"));
	QVERIFY(Model->InsertPieceToolClicked());
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	QCOMPARE(Model->GetPieces()[0]->mPieceData, Data);
	QVERIFY(Data->GetProject() != Original);

	// Edit the dependency after creating the preview, without refreshing the category.
	Model->SetPreviewInsertPieceInfo({ { Data.Get(), lcMatrix44Identity(), gDefaultColor } });
	QVERIFY(WriteFile("nested/leaf.ldr", "0 Cycle\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 ../host.mpd\n"));
	QVERIFY(!Model->InsertPieceToolClicked());
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	QVERIFY(Model->GetPreviewInsertPieceInfo().empty());
	QVERIFY(!Model->AddPiece(Data.Get(), LC_PIECE_SECTION_INVALID));
	QCOMPARE(Model->GetPieces().size(), size_t(1));

	// Keyboard insertion also reloads valid edits, and rejects a deleted file.
	QVERIFY(WriteFile("child.ldr", "0 Changed child\n3 16 0 0 0 30 0 0 0 30 0\n"));
	QVERIFY(Model->AddPiece(Data.Get(), LC_PIECE_SECTION_INVALID));
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	Model->SetPreviewInsertPieceInfo({ { Data.Get(), lcMatrix44Identity(), gDefaultColor } });
	QVERIFY(QFile::remove(Directory.filePath("child.ldr")));
	QVERIFY(!Model->InsertPieceToolClicked());
	QVERIFY(!Model->AddPiece(Data.Get(), LC_PIECE_SECTION_INVALID));
	QCOMPARE(Model->GetPieces().size(), size_t(2));
}

void lcModelInstanceTests::FindReplaceViewIsolation()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile File(Directory.filePath("host.ldr"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Host\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 alpha.dat\n"
		"1 16 50 0 0 1 0 0 0 1 0 0 0 1 beta.dat\n");
	File.close();

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;
	QVERIFY(Scope.Current.Load(File.fileName(), false));
	lcModel* Model = Scope.Current.GetMainModel();
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	Window.mActions[LC_VIEW_SPLIT_HORIZONTAL]->trigger();
	const std::vector<lcView*> Views = lcView::GetModelViews(Model);
	QCOMPARE(Views.size(), size_t(2));
	lcView* First = Views[0];
	lcView* Second = Views[1];
	First->ShowFindReplaceWidget(true);
	Second->ShowFindReplaceWidget(true);
	QPointer<lcFindReplaceWidget> FirstWidget = First->GetWidget()->findChild<lcFindReplaceWidget*>();
	QPointer<lcFindReplaceWidget> SecondWidget = Second->GetWidget()->findChild<lcFindReplaceWidget*>();
	QVERIFY(FirstWidget && SecondWidget);
	QVERIFY(FirstWidget != SecondWidget);
	QVERIFY(&First->GetFindReplaceParams() != &Second->GetFindReplaceParams());
	QVERIFY(QMetaObject::invokeMethod(FirstWidget, "FindTextEdited", Qt::DirectConnection, Q_ARG(QString, QString("alpha"))));
	QVERIFY(QMetaObject::invokeMethod(SecondWidget, "FindTextEdited", Qt::DirectConnection, Q_ARG(QString, QString("beta"))));
	First->GetFindReplaceParams().FindColorIndex = lcGetColorIndex(LC_COLOR_NOCOLOR);
	Second->GetFindReplaceParams().FindColorIndex = lcGetColorIndex(LC_COLOR_NOCOLOR);
	First->GetFindReplaceParams().ReplaceColorIndex = lcGetColorIndex(4);
	Second->GetFindReplaceParams().ReplaceColorIndex = lcGetColorIndex(1);
	First->SetFocus(true);
	Second->SetFocus(true);
	QVERIFY(FirstWidget && SecondWidget);
	QCOMPARE(First->GetFindReplaceParams().FindString, QString("alpha"));
	QCOMPARE(Second->GetFindReplaceParams().FindString, QString("beta"));

	const auto FindAction = [](lcFindReplaceWidget* Widget, QAction* Source) -> QAction*
	{
		for (QAction* Action : Widget->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly))
			if (Action->text() == Source->text())
				return Action;

		return nullptr;
	};

	// A button in the first view must use its owner even while the second is active.
	QAction* FirstFind = FindAction(FirstWidget, Window.mActions[LC_EDIT_FIND_ALL]);
	QVERIFY(FirstFind);
	FirstFind->trigger();
	QVERIFY(Model->GetPieces()[0]->IsSelected());
	QVERIFY(!Model->GetPieces()[1]->IsSelected());
	QCOMPARE(Window.GetActiveView(), First);
	Second->SetFocus(true);
	Window.mActions[LC_EDIT_FIND_ALL]->trigger();
	QVERIFY(!Model->GetPieces()[0]->IsSelected());
	QVERIFY(Model->GetPieces()[1]->IsSelected());

	QAction* FirstReplace = FindAction(FirstWidget, Window.mActions[LC_EDIT_REPLACE_ALL]);
	QVERIFY(FirstReplace);
	FirstReplace->trigger();
	QCOMPARE(Model->GetPieces()[0]->GetColorCode(), quint32(4));
	QCOMPARE(Model->GetPieces()[1]->GetColorCode(), quint32(16));
	Second->SetFocus(true);
	Window.mActions[LC_EDIT_REPLACE_ALL]->trigger();
	QCOMPARE(Model->GetPieces()[0]->GetColorCode(), quint32(4));
	QCOMPARE(Model->GetPieces()[1]->GetColorCode(), quint32(1));

	Second->GetFindReplaceParams().FindString.clear();
	Window.mActions[LC_EDIT_FIND_NEXT]->trigger();
	QCOMPARE(Model->GetFocusObject(), Model->GetPieces()[0].get());
	Window.mActions[LC_EDIT_FIND_NEXT]->trigger();
	QCOMPARE(Model->GetFocusObject(), Model->GetPieces()[1].get());
	Window.mActions[LC_EDIT_FIND_PREVIOUS]->trigger();
	QCOMPARE(Model->GetFocusObject(), Model->GetPieces()[0].get());
	Second->GetFindReplaceParams().ReplaceColorIndex = lcGetColorIndex(14);
	Window.mActions[LC_EDIT_REPLACE_NEXT]->trigger();
	QCOMPARE(Model->GetPieces()[0]->GetColorCode(), quint32(14));
	QCOMPARE(Model->GetPieces()[1]->GetColorCode(), quint32(1));
	QCOMPARE(Model->GetFocusObject(), Model->GetPieces()[1].get());

	// Reopening one view's panel leaves the other view's parameters untouched.
	Second->ShowFindReplaceWidget(false);
	QVERIFY(FirstWidget);
	QCOMPARE(First->GetFindReplaceParams().FindString, QString("alpha"));
	QCOMPARE(First->GetFindReplaceParams().ReplaceColorIndex, lcGetColorIndex(4));

	{
		lcView Temporary(lcViewType::PartsList, Model);
		Temporary.GetFindReplaceParams().ReplacePieceData = lcGetPiecesLibrary()->AcquirePieceData(
			"find-view-temporary.dat", &Scope.Current, lcAcquirePieceDataFlag::CreateMissing);
		QVERIFY(lcGetPiecesLibrary()->FindPieceData("find-view-temporary.dat", &Scope.Current));
	}
	QVERIFY(!lcGetPiecesLibrary()->FindPieceData("find-view-temporary.dat", &Scope.Current));
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

void lcModelInstanceTests::RotateToolMouseShortcuts_data()
{
	QTest::addColumn<int>("Button");
	QTest::addColumn<bool>("Selected");

	for (bool Selected : { false, true })
	{
		const QByteArray Suffix = Selected ? "-selected" : "-unselected";
		QTest::newRow(("orbit" + Suffix).constData()) << int(Qt::LeftButton) << Selected;
		QTest::newRow(("pan" + Suffix).constData()) << int(Qt::MiddleButton) << Selected;
		QTest::newRow(("zoom" + Suffix).constData()) << int(Qt::RightButton) << Selected;
	}
}

void lcModelInstanceTests::RotateToolMouseShortcuts()
{
	QFETCH(int, Button);
	QFETCH(bool, Selected);

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile File(Directory.filePath("rotate.ldr"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Rotate shortcuts\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 alpha.dat\n");
	File.close();

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		lcMouseShortcuts PreviousShortcuts = gMouseShortcuts;
		Project Current;
		ScopedProject()
		{
			gApplication->mProject = &Current;
			gMouseShortcuts.Reset();
		}
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
			gMouseShortcuts = PreviousShortcuts;
		}
	} Scope;
	QVERIFY(Scope.Current.Load(File.fileName(), false));
	lcModel* Model = Scope.Current.GetMainModel();
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	lcView* View = Window.GetActiveView();
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(0, -500, 300), lcVector3(0, 0, 0), lcVector3(0, 0, 1));
	View->SetCamera(&Camera, true);
	lcPiece* Piece = Model->GetPieces()[0].get();
	if (Selected)
		Model->SetSelectionAndFocusAction({ Piece }, Piece, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	const lcMatrix44 PieceBefore = Piece->mModelWorld;
	const lcCameraHistoryState CameraBefore = View->GetCamera()->GetHistoryState();
	Window.SetTool(lcTool::Rotate);
	View->SetMouseModifiers(Qt::AltModifier);
	// Empty viewport space outside the centered rotation gizmo and view sphere.
	View->SetMousePosition(100, 100);
	View->OnMouseMove();

	if (Button == Qt::LeftButton)
		View->OnLeftButtonDown();
	else if (Button == Qt::MiddleButton)
		View->OnMiddleButtonDown();
	else
		View->OnRightButtonDown();

	const bool Tracking = View->IsTracking();
	View->SetMousePosition(150, 140);
	View->OnMouseMove();

	if (Button == Qt::LeftButton)
		View->OnLeftButtonUp();
	else if (Button == Qt::MiddleButton)
		View->OnMiddleButtonUp();
	else if (Tracking)
		View->OnRightButtonUp();

	QVERIFY(Tracking);
	QVERIFY(!View->IsTracking());
	QVERIFY(!(View->GetCamera()->GetHistoryState() == CameraBefore));
	QVERIFY(Piece->mModelWorld == PieceBefore);
	QCOMPARE(Piece->IsSelected(), Selected);
	QCOMPARE(Window.GetTool(), lcTool::Rotate);
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

	{
		QLabel* PositionLabel = nullptr;

		for (QLabel* Label : Window->statusBar()->findChildren<QLabel*>())
			if (Label->text().startsWith("X: "))
				PositionLabel = Label;

		QVERIFY(PositionLabel);

		const auto PositionText = [](const lcVector3& Position)
		{
			return QString("X: %1 Y: %2 Z: %3").arg(QLocale::system().toString(Position.x, 'f', 2),
				QLocale::system().toString(Position.y, 'f', 2), QLocale::system().toString(Position.z, 'f', 2));
		};

		Main->SetSelectionAndFocusAction({ SecondA }, SecondA, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
		const QString FocusText = PositionLabel->text();
		const size_t PieceCount = Main->GetPieces().size();
		const lcVector3 PreviewPosition(123.0f, -45.0f, 67.0f);
		const lcMatrix44 PreviewTransform(lcMatrix33Identity(), PreviewPosition);

		View->BeginDrag(lcDragState::Piece);
		Main->SetPreviewInsertPieceInfo({ { Leaf->mPieceData.Get(), PreviewTransform, Leaf->GetColorIndex() } });
		QCOMPARE(PositionLabel->text(), PositionText(PreviewPosition));
		QCOMPARE(Main->GetPieces().size(), PieceCount);
		QCOMPARE(Main->GetFocusObject(), static_cast<lcObject*>(SecondA));
		Window->UpdateSelectedObjects(false);
		QCOMPARE(PositionLabel->text(), PositionText(PreviewPosition));

		const lcVector3 MovedPosition(-20.0f, 30.0f, 40.0f);
		Main->SetPreviewInsertPieceInfo({ { Leaf->mPieceData.Get(), lcMatrix44(lcMatrix33Identity(), MovedPosition), Leaf->GetColorIndex() } });
		QCOMPARE(PositionLabel->text(), PositionText(MovedPosition));
		View->EndDrag(false);
		QCOMPARE(PositionLabel->text(), FocusText);
		QCOMPARE(Main->GetPieces().size(), PieceCount);

		View->BeginDrag(lcDragState::Piece);
		Main->SetPreviewInsertPieceInfo({ { Leaf->mPieceData.Get(), PreviewTransform, Leaf->GetColorIndex() } });
		View->EndDrag(true);
		QCOMPARE(Main->GetPieces().size(), PieceCount + 1);
		QCOMPARE(PositionLabel->text(), PositionText(PreviewPosition));
		Main->UndoAction();
		QCOMPARE(PositionLabel->text(), FocusText);

		Main->ClearSelectionAction();
		View->BeginDrag(lcDragState::Piece);
		Main->SetPreviewInsertPieceInfo({ { Leaf->mPieceData.Get(), PreviewTransform, Leaf->GetColorIndex() } });
		QCOMPARE(PositionLabel->text(), PositionText(PreviewPosition));
		View->EndDrag(false);
		QCOMPARE(PositionLabel->text(), PositionText(lcVector3(0.0f, 0.0f, 0.0f)));
	}

	{
		lcCamera* Camera = Main->GetCameras()[0].get();
		const lcCameraHistoryState Before = Camera->GetHistoryState();

		// Opposite camera moves should remove the merged edit when it returns to
		// the saved state, while an ordinary move must remain undoable.
		Main->SetSaved();
		Main->MoveCamera(Camera, lcVector3(1.0f, 0.0f, 0.0f));
		QVERIFY(Main->IsModified());
		QVERIFY(!(Camera->GetHistoryState() == Before));
		Main->UndoAction();
		QVERIFY(!Main->IsModified());
		QVERIFY(Camera->GetHistoryState() == Before);
		Main->RedoAction();
		QVERIFY(Main->IsModified());
		Main->MoveCamera(Camera, lcVector3(-1.0f, 0.0f, 0.0f));
		QVERIFY(Camera->GetHistoryState() == Before);
		QVERIFY(!Main->IsModified());
	}

	{
		lcView ZoomView(lcViewType::View, Main);
		ZoomView.SetCameraIndex(0);
		lcCamera* Camera = ZoomView.GetCamera();
		const lcCameraHistoryState Before = Camera->GetHistoryState();
		const bool AddKeys = Window->GetAddKeys();
		Window->SetAddKeys(false);
		Main->SetSaved();
		const QString UndoText = Window->mActions[LC_EDIT_UNDO]->text();
		const bool UndoEnabled = Window->mActions[LC_EDIT_UNDO]->isEnabled();

		Main->BeginMouseTool(lcTool::Zoom, &ZoomView);
		Main->UpdateZoomTool(Camera, 10.0f);
		Main->EndMouseTool(lcTool::Zoom, &ZoomView, true);
		QVERIFY(Main->IsModified());
		QVERIFY(!(Camera->GetHistoryState() == Before));
		Main->UndoAction();
		QVERIFY(Camera->GetHistoryState() == Before);
		QVERIFY(!Main->IsModified());
		Main->RedoAction();
		QVERIFY(Main->IsModified());

		Main->BeginMouseTool(lcTool::Zoom, &ZoomView);
		Main->UpdateZoomTool(Camera, -10.0f);
		Main->EndMouseTool(lcTool::Zoom, &ZoomView, true);
		QVERIFY(Camera->GetHistoryState() == Before);
		QVERIFY(!Main->IsModified());
		QCOMPARE(Window->mActions[LC_EDIT_UNDO]->text(), UndoText);
		QCOMPARE(Window->mActions[LC_EDIT_UNDO]->isEnabled(), UndoEnabled);

		Main->BeginMouseTool(lcTool::Zoom, &ZoomView);
		Main->UpdateZoomTool(Camera, 5.0f);
		Main->EndMouseTool(lcTool::Zoom, &ZoomView, false);
		QVERIFY(Camera->GetHistoryState() == Before);
		QVERIFY(!Main->IsModified());
		Window->SetAddKeys(AddKeys);
	}

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

	// Force the insertion fallback beyond the clipping segment in a translated instance.
	View = Window->GetActiveView();
	View->SetTopSubmodelActive();
	Main->SetSelectionAndFocusAction({ SecondA }, SecondA, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	for (const std::unique_ptr<lcPiece>& Piece : A->GetPieces())
	{
		Piece->SetSelected(false);
		Piece->SetHidden(true);
	}

	lcCamera FallbackCamera(true);
	FallbackCamera.SetViewpoint(lcVector3(1000, 0, 10000), lcVector3(1000, 0, 9000), lcVector3(0, 1, 0));
	FallbackCamera.m_zNear = 1;
	FallbackCamera.m_zFar = 10;
	View->SetCamera(&FallbackCamera, true);
	View->SetSize(1200, 600);
	View->SetMousePosition(600, 300);
	const lcVector3 WorldFallback = View->UnprojectPoint(lcVector3(600, 300, 0.9f));
	const lcVector3 LocalFallback = lcMul31(WorldFallback, lcMatrix44AffineInverse(View->GetActiveSubmodelTransform()));
	const std::pair<std::vector<lcInsertPieceInfo>, bool> Insert = View->GetMouseInsertPieceInfo(false, true, Leaf->mPieceData, nullptr);
	QCOMPARE(Insert.first.size(), size_t(1));
	QVERIFY(lcLength(Insert.first[0].Transform.GetTranslation() - LocalFallback) < 0.01f);

	for (const std::unique_ptr<lcPiece>& Piece : A->GetPieces())
		Piece->SetHidden(false);

	// A hidden view must leave a deleted edit instance before any project redraw.
	A->SetSelectionAndFocusAction({ FirstB }, FirstB, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	View->SetCameraIndex(0);
	lcCamera BeforeDelete(true);
	View->CopyCameraPosition(&BeforeDelete);
	Window->SetCurrentModelTab(A, false);
	A->EraserToolClicked(FirstB);
	QCOMPARE(View->GetActiveModel(), A);
	QCOMPARE(View->GetModelEditingPath().size(), size_t(2));
	QVERIFY(View->GetCamera()->IsSimple());
	for (int Row = 0; Row < 4; Row++)
		for (int Column = 0; Column < 4; Column++)
			QVERIFY(fabsf(View->GetWorldViewMatrix()[Row][Column] - BeforeDelete.mWorldView[Row][Column]) < 0.001f);

	A->UndoAction();
	Window->SetCurrentModelTab(Main, false);
	FirstB = A->GetPieces()[0].get();
	FirstB->SetFocused(LC_PIECE_SECTION_POSITION, true);
	View->SetSelectedSubmodelActive();
	Window->SetCurrentModelTab(A, false);
	A->RedoAction();
	QCOMPARE(View->GetActiveModel(), A);
	Window->SetCurrentModelTab(Main, false);
	QCOMPARE(Window->GetActiveView()->GetActiveModel(), A);

	A->UndoAction();
	FirstB = A->GetPieces()[0].get();
	A->SetSelectionAndFocusAction({ FirstB }, FirstB, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	Window->SetCurrentModelTab(A, false);
	A->DeleteSelectedObjects();
	QCOMPARE(View->GetActiveModel(), A);

	A->UndoAction();
	Window->SetCurrentModelTab(Main, false);
	FirstB = A->GetPieces()[0].get();
	A->SetSelectionAndFocusAction({ FirstB }, FirstB, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	View->SetSelectedSubmodelActive();
	Window->SetCurrentModelTab(A, false);
	FirstB->SetPieceData(Leaf->mPieceData, QString(), false, true);
	QCOMPARE(View->GetActiveModel(), A);
	Window->SetCurrentModelTab(Main, false);
	QCOMPARE(Window->GetActiveView()->GetActiveModel(), A);

	SeparateRotations(Window, Main);

}

void lcModelInstanceTests::SeparateRotations(lcMainWindow* Window, lcModel* Model)
{
	Window->GetActiveView()->SetTopSubmodelActive();
	const bool Separate = Window->GetSeparateTransform();
	const bool Relative = Window->GetRelativeTransform();
	lcCamera* Camera = Model->GetCameras()[0].get();
	lcLight* Light = Model->GetLights()[0].get();
	Camera->SetPosition(lcVector3(-500, 200, 100), 1, false);
	Camera->SetTargetPosition(lcVector3(-400, 0, 0), 1, false);
	Camera->UpdatePosition(Model->GetCurrentStep());

	const auto CompareTransform = [](const lcMatrix44& First, const lcMatrix44& Second, int Rows)
	{
		for (int Row = 0; Row < Rows; Row++)
			for (int Column = 0; Column < 4; Column++)
				QVERIFY(fabsf(First[Row][Column] - Second[Row][Column]) < 0.001f);
	};

	const auto CheckRotation = [&](lcObject* Object)
	{
		Model->SetSelectionAndFocusAction({ Object }, Object, 0, lcSelectionMode::Single);
		const auto GetTransform = [&]()
		{
			return Object->IsCamera() ? Camera->mWorldView : Light->GetWorldMatrix();
		};

		for (bool RelativeRotation : { false, true })
		{
			const lcMatrix44 Before = GetTransform();
			lcModelTransformFlags Flags = lcModelTransformFlag::Checkpoint;
			if (RelativeRotation)
				Flags |= lcModelTransformFlag::Relative;

			Window->SetRelativeTransform(RelativeRotation);
			if (Object->IsCamera())
			{
				const lcMatrix33 ExpectedFrame = RelativeRotation ? lcMatrix33AffineInverse(lcMatrix33(Before)) : lcMatrix33Identity();
				QVERIFY(lcMatrix33Similar(Model->GetRelativeRotation(), ExpectedFrame));
			}

			Window->SetSeparateTransform(false);
			Model->RotateSelectedObjects(lcVector3(15, 25, 35), Flags, lcModelHistoryEditMerge::None);
			const lcMatrix44 Expected = GetTransform();
			Model->UndoAction();
			CompareTransform(GetTransform(), Before, 4);

			Window->SetSeparateTransform(true);
			Model->RotateSelectedObjects(lcVector3(15, 25, 35), Flags, lcModelHistoryEditMerge::None);
			CompareTransform(GetTransform(), Expected, 3);

			// Local rotations must compose without discarding the original orientation.
			const lcMatrix44 Rotated = GetTransform();
			const lcMatrix33 Rotation = lcMul(lcMatrix33RotationZ(35 * LC_DTOR), lcMul(lcMatrix33RotationY(25 * LC_DTOR), lcMatrix33RotationX(15 * LC_DTOR)));
			if (RelativeRotation)
			{
				const lcMatrix33 BeforeOrientation = Object->IsCamera() ? lcMatrix33AffineInverse(lcMatrix33(Before)) : lcMatrix33(Before);
				const lcMatrix33 AfterOrientation = Object->IsCamera() ? lcMatrix33AffineInverse(lcMatrix33(Rotated)) : lcMatrix33(Rotated);
				QVERIFY(lcMatrix33Similar(AfterOrientation, lcMul(Rotation, BeforeOrientation)));
			}

			Model->RotateSelectedObjects(lcMatrix33AffineInverse(Rotation), Flags, lcModelHistoryEditMerge::None);
			CompareTransform(GetTransform(), Before, 4);
			Model->UndoAction();
			CompareTransform(GetTransform(), Rotated, 4);
			Model->UndoAction();
			CompareTransform(GetTransform(), Before, 4);
		}
	};

	CheckRotation(Camera);
	for (lcLightType Type : { lcLightType::Spot, lcLightType::Directional, lcLightType::Area })
	{
		Light->SetLightType(Type);
		Light->SetRotation(lcMul(lcMatrix33RotationX(0.4f), lcMatrix33RotationZ(0.7f)), 1, false);
		Light->UpdatePosition(Model->GetCurrentStep());
		CheckRotation(Light);
	}

	Window->SetSeparateTransform(Separate);
	Window->SetRelativeTransform(Relative);
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

	const auto SendGesture = [&](Qt::NativeGestureType Type, qreal Value)
	{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
		QNativeGestureEvent Event(Type, QPointingDevice::primaryPointingDevice(), 2, QPointF(), QPointF(), QPointF(), Value, QPointF());
#elif (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		QNativeGestureEvent Event(Type, QPointingDevice::primaryPointingDevice(), QPointF(), QPointF(), QPointF(), Value, 0, 0);
#else
		QNativeGestureEvent Event(Type, QPointF(), QPointF(), QPointF(), Value, 0, 0);
#endif
		Event.setAccepted(false);
		QVERIFY(QApplication::sendEvent(View->GetWidget(), &Event));
		QVERIFY(Event.isAccepted());
	};

	const auto ApplyZoom = [&](lcModelHistoryEditMerge Source)
	{
		switch (Source)
		{
		case lcModelHistoryEditMerge::KeyboardZoom:
			Window->mActions[LC_VIEW_ZOOM_IN]->trigger();
			break;

		case lcModelHistoryEditMerge::WheelZoom:
		{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
			QWheelEvent Event(QPointF(100, 100), QPointF(100, 100), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
#else
			QWheelEvent Event(QPointF(100, 100), QPointF(100, 100), QPoint(), QPoint(0, 120), 120, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
#endif
			QApplication::sendEvent(View->GetWidget(), &Event);
			break;
		}

		case lcModelHistoryEditMerge::PinchZoom:
			SendGesture(Qt::BeginNativeGesture, 0);
			SendGesture(Qt::ZoomNativeGesture, 0.1);
			SendGesture(Qt::EndNativeGesture, 0);
			break;

		case lcModelHistoryEditMerge::DragZoom:
			A->BeginMouseTool(lcTool::Zoom, View);
			A->UpdateZoomTool(ACamera, 5.0f);
			A->UpdateZoomTool(ACamera, 10.0f);
			A->EndMouseTool(lcTool::Zoom, View, true);
			break;

		default:
			QFAIL("Unexpected zoom source");
		}
	};

	const lcModelHistoryEditMerge ZoomSources[] = { lcModelHistoryEditMerge::KeyboardZoom, lcModelHistoryEditMerge::WheelZoom,
		lcModelHistoryEditMerge::PinchZoom, lcModelHistoryEditMerge::DragZoom };

	// Repeat each action, then switch sources in both directions. Undo must keep
	// each run of one source together without absorbing a different source.
	for (lcModelHistoryEditMerge First : ZoomSources)
		for (lcModelHistoryEditMerge Second : ZoomSources)
		{
			const lcCameraHistoryState Before = ACamera->GetHistoryState();
			ApplyZoom(First);
			ApplyZoom(First);
			const lcCameraHistoryState AfterFirst = ACamera->GetHistoryState();
			QVERIFY(!(AfterFirst == Before));
			ApplyZoom(Second);
			ApplyZoom(Second);
			const lcCameraHistoryState AfterSecond = ACamera->GetHistoryState();
			QVERIFY(!(AfterSecond == AfterFirst));
			A->UndoAction();
			if (First != Second)
			{
				QVERIFY(ACamera->GetHistoryState() == AfterFirst);
				A->UndoAction();
			}
			QVERIFY(ACamera->GetHistoryState() == Before);
			A->RedoAction();
			if (First != Second)
			{
				QVERIFY(ACamera->GetHistoryState() == AfterFirst);
				A->RedoAction();
			}
			QVERIFY(ACamera->GetHistoryState() == AfterSecond);
			A->UndoAction();
			if (First != Second)
				A->UndoAction();
			QVERIFY(ACamera->GetHistoryState() == Before);
			QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
		}

	// Keyboard and wheel input must finish a roll before starting another edit.
	for (lcModelHistoryEditMerge Source : { lcModelHistoryEditMerge::KeyboardZoom, lcModelHistoryEditMerge::WheelZoom })
	{
		const lcCameraHistoryState Before = ACamera->GetHistoryState();
		SendGesture(Qt::BeginNativeGesture, 0);
		SendGesture(Qt::RotateNativeGesture, 8);
		const lcCameraHistoryState Rolled = ACamera->GetHistoryState();
		QVERIFY(!(Rolled == Before));
		if (Source == lcModelHistoryEditMerge::KeyboardZoom)
		{
			QKeyEvent Shortcut(QEvent::ShortcutOverride, Qt::Key_Plus, Qt::ControlModifier);
			QApplication::sendEvent(View->GetWidget(), &Shortcut);
		}
		ApplyZoom(Source);
		ApplyZoom(Source);
		const lcCameraHistoryState Zoomed = ACamera->GetHistoryState();
		QVERIFY(!(Zoomed == Rolled));
		SendGesture(Qt::EndNativeGesture, 0);
		A->UndoAction();
		QVERIFY(ACamera->GetHistoryState() == Rolled);
		A->UndoAction();
		QVERIFY(ACamera->GetHistoryState() == Before);
		A->RedoAction();
		QVERIFY(ACamera->GetHistoryState() == Rolled);
		A->RedoAction();
		QVERIFY(ACamera->GetHistoryState() == Zoomed);
		A->UndoAction();
		A->UndoAction();
		QVERIFY(ACamera->GetHistoryState() == Before);
		QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
	}

	const lcCameraHistoryState BeforeRoll = ACamera->GetHistoryState();
	lcCamera ExpectedRoll(false);
	ExpectedRoll.SetHistoryState(BeforeRoll);
	ExpectedRoll.Roll(15.0f * LC_DTOR, A->GetCurrentStep(), Window->GetAddKeys());

	SendGesture(Qt::BeginNativeGesture, 0);
	SendGesture(Qt::RotateNativeGesture, 10);
	SendGesture(Qt::RotateNativeGesture, 5);
	SendGesture(Qt::EndNativeGesture, 0);
	QVERIFY(SameMatrix(ACamera->mWorldView, ExpectedRoll.mWorldView));
	const lcCameraHistoryState AfterRoll = ACamera->GetHistoryState();
	QVERIFY(!(AfterRoll == BeforeRoll));
	QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == BeforeRoll);
	A->RedoAction();
	QVERIFY(ACamera->GetHistoryState() == AfterRoll);

	// A new gesture must start from zero and create a separate undo entry.
	SendGesture(Qt::BeginNativeGesture, 0);
	SendGesture(Qt::RotateNativeGesture, -5);
	SendGesture(Qt::EndNativeGesture, 0);
	ExpectedRoll.Roll(-5.0f * LC_DTOR, A->GetCurrentStep(), Window->GetAddKeys());
	QVERIFY(SameMatrix(ACamera->mWorldView, ExpectedRoll.mWorldView));
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == AfterRoll);
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == BeforeRoll);

	// Losing focus must close the edit even if the native end event is lost.
	SendGesture(Qt::BeginNativeGesture, 0);
	SendGesture(Qt::RotateNativeGesture, 8);
	QFocusEvent FocusOut(QEvent::FocusOut);
	QApplication::sendEvent(View->GetWidget(), &FocusOut);
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == BeforeRoll);
	SendGesture(Qt::EndNativeGesture, 0);

	View->SetCamera(ACamera, true);
	QVERIFY(View->GetCamera()->IsSimple());
	const lcCameraHistoryState SimpleBefore = View->GetCamera()->GetHistoryState();
	SendGesture(Qt::BeginNativeGesture, 0);
	SendGesture(Qt::RotateNativeGesture, 12);
	SendGesture(Qt::EndNativeGesture, 0);
	QVERIFY(!(View->GetCamera()->GetHistoryState() == SimpleBefore));
	QVERIFY(ACamera->GetHistoryState() == BeforeRoll);
	View->SetCamera(ACamera, false);

	const auto SendPanWheel = [&](Qt::ScrollPhase Phase, const QPoint& Delta)
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		QWheelEvent Event(QPointF(100, 100), QPointF(100, 100), Delta, QPoint(), Qt::NoButton, Qt::NoModifier, Phase, false, Qt::MouseEventSynthesizedBySystem);
#else
		QWheelEvent Event(QPointF(100, 100), QPointF(100, 100), Delta, QPoint(), 0, Qt::Vertical, Qt::NoButton, Qt::NoModifier, Phase, Qt::MouseEventSynthesizedBySystem);
#endif
		Event.setAccepted(false);
		QApplication::sendEvent(View->GetWidget(), &Event);
		QVERIFY(Event.isAccepted());
	};

	const lcCameraHistoryState BeforePan = ACamera->GetHistoryState();
	const QPoint PanDelta(15, 10);
	const QPoint ScaledPanDelta = PanDelta * View->GetWidget()->GetDeviceScale();
	View->StartPanGesture();
	View->UpdatePanGesture(ScaledPanDelta.x(), -ScaledPanDelta.y());
	View->EndPanGesture(true);
	const lcCameraHistoryState ExpectedPan = ACamera->GetHistoryState();
	QVERIFY(!(ExpectedPan == BeforePan));
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == BeforePan);

	SendPanWheel(Qt::ScrollBegin, QPoint());
	SendPanWheel(Qt::ScrollUpdate, PanDelta);
	SendPanWheel(Qt::ScrollEnd, QPoint());
	QVERIFY(ACamera->GetHistoryState() == ExpectedPan);
	A->UndoAction();
	QVERIFY(ACamera->GetHistoryState() == BeforePan);

	for (bool LoseFocus : { false, true })
		for (Qt::NativeGestureType Type : { Qt::ZoomNativeGesture, Qt::SmartZoomNativeGesture, Qt::RotateNativeGesture })
		{
			SendPanWheel(Qt::ScrollBegin, QPoint());
			SendPanWheel(Qt::ScrollUpdate, PanDelta);
			QVERIFY(ACamera->GetHistoryState() == ExpectedPan);

			if (LoseFocus)
			{
				QFocusEvent PanFocusOut(QEvent::FocusOut);
				QApplication::sendEvent(View->GetWidget(), &PanFocusOut);
			}

			SendGesture(Type, 0.1);
			SendGesture(Qt::EndNativeGesture, 0);
			const lcCameraHistoryState AfterInterrupt = ACamera->GetHistoryState();
			QVERIFY(!(AfterInterrupt == ExpectedPan));

			// Late momentum and end events must not move the camera or close another edit.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
			SendPanWheel(Qt::ScrollMomentum, PanDelta);
#else
			SendPanWheel(Qt::ScrollUpdate, PanDelta);
#endif
			SendPanWheel(Qt::ScrollEnd, QPoint());
			QVERIFY(ACamera->GetHistoryState() == AfterInterrupt);
			A->UndoAction();
			QVERIFY(ACamera->GetHistoryState() == ExpectedPan);
			A->UndoAction();
			QVERIFY(ACamera->GetHistoryState() == BeforePan);
			QVERIFY(ParentCamera->GetHistoryState() == ParentBefore);
		}

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
	// Orbit an off-center nested model through its translated/rotated instance.
	// With no selection the pivot is the visible geometry center; selecting a
	// piece must still use that piece's pivot in either camera coordinate frame.
	{
		std::vector<std::pair<lcObject*, bool>> Selection;
		for (const std::unique_ptr<lcPiece>& Piece : B->GetPieces())
			Selection.emplace_back(Piece.get(), Piece->IsSelected());
		for (const std::unique_ptr<lcCamera>& Camera : B->GetCameras())
			Selection.emplace_back(Camera.get(), Camera->IsSelected());
		for (const std::unique_ptr<lcLight>& Light : B->GetLights())
			Selection.emplace_back(Light.get(), Light->IsSelected());

		const lcCameraHistoryState Before = BCamera->GetHistoryState();
		for (bool Simple : { true, false })
			for (bool Selected : { false, true })
			{
				for (const std::pair<lcObject*, bool>& Entry : Selection)
					Entry.first->SetSelected(false);
				B->GetPieces()[0]->SetSelected(Selected);

				lcVector3 Min, Max;
				QVERIFY(B->GetVisiblePiecesBoundingBox(Min, Max));
				lcVector3 Center = (Min + Max) / 2.0f;
				if (Selected)
					Center = B->GetPieces()[0]->GetSectionPosition(LC_PIECE_SECTION_POSITION);
				QVERIFY(lcLength(Center) > 1.0f);

				View->SetCamera(BCamera, Simple);
				lcCamera* Camera = View->GetCamera();
				const lcMatrix44 ModelToCamera = Simple ? View->GetActiveSubmodelTransform() : lcMatrix44Identity();
				const lcVector3 Pivot = lcMul31(Center, ModelToCamera);
				const lcVector3 Position = Pivot + lcVector3(200, -300, 400);
				Camera->SetViewpoint(Position, Pivot, lcVector3(0, 0, 1));
				B->BeginMouseTool(lcTool::RotateView, View);
				B->UpdateOrbitTool(Camera, ModelToCamera, 0.3f, -0.2f);
				QVERIFY(lcLength(Camera->mTargetPosition - Pivot) < 0.001f);
				QVERIFY(lcLength(Camera->mPosition - Position) > 1.0f);
				QVERIFY(fabsf(lcLength(Camera->mPosition - Pivot) - lcLength(Position - Pivot)) < 0.001f);
				B->EndMouseTool(lcTool::RotateView, View, false);
			}

		BCamera->SetHistoryState(Before);
		for (const std::pair<lcObject*, bool>& Entry : Selection)
			Entry.first->SetSelected(Entry.second);
	}

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


void lcModelInstanceTests::TranslucentThumbnails_data()
{
	QTest::addColumn<int>("Samples");
	QTest::newRow("single-sample") << 0;
	QTest::newRow("multisample") << 4;
}

void lcModelInstanceTests::TranslucentThumbnails()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test thumbnail rendering.");

	QVERIFY(lcContext::InitializeRenderer(true));
	lcLoadDefaultColors(lcStudStyle::Plain);

	struct ScopedSettings
	{
		lcPreferences Preferences = lcGetPreferences();
		QPalette Palette = QApplication::palette();
		QSurfaceFormat Format = QSurfaceFormat::defaultFormat();
		lcMainWindow* Window = gMainWindow;
		~ScopedSettings()
		{
			lcGetPreferences() = Preferences;
			QApplication::setPalette(Palette);
			QSurfaceFormat::setDefaultFormat(Format);
			gMainWindow = Window;
		}
	} Settings;
	gMainWindow = nullptr;
	QFETCH(int, Samples);
	QSurfaceFormat Format = Settings.Format;
	Format.setSamples(Samples);
	QSurfaceFormat::setDefaultFormat(Format);

	lcGetPreferences().mShadingMode = lcShadingMode::Flat;
	lcGetPreferences().mDrawEdgeLines = false;
	lcGetPreferences().mDrawConditionalLines = false;

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile File(Directory.filePath("transparent.mpd"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 FILE transparent.ldr\n"
		"4 47 -20 0 -20 20 0 -20 20 0 20 -20 0 20\n"
		"4 47 -20 4 -20 20 4 -20 20 4 20 -20 4 20\n"
		"0 FILE inherited.ldr\n"
		"4 16 -20 0 -20 20 0 -20 20 0 20 -20 0 20\n"
		"4 16 -20 4 -20 20 4 -20 20 4 20 -20 4 20\n");
	File.close();
	Project Project;
	QVERIFY(Project.Load(File.fileName(), false));
	QCOMPARE(Project.GetModels().size(), size_t(2));

	for (const std::unique_ptr<lcModel>& Model : Project.GetModels())
	{
		Model->WaitForAssets();
		QVERIFY(Model->GetPieceData()->GetMesh());
		QImage Previous;

		for (int Background : { 48, 255 })
		{
			QPalette Palette = Settings.Palette;
			Palette.setColor(QPalette::Base, QColor(Background, Background, Background));
			QApplication::setPalette(Palette);
			lcThumbnailManager Thumbnails(lcGetPiecesLibrary());
			const int ColorIndex = Model.get() == Project.GetMainModel() ? lcGetColorIndex(4) : lcGetColorIndex(47);
			const auto [Id, Pixmap] = Thumbnails.RequestThumbnail(Model->GetPieceData(), ColorIndex, 64, 1.0f);
			QVERIFY(!Pixmap.isNull());
			const QImage Image = Pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
			int TranslucentPixels = 0;

			for (int y = 0; y < Image.height(); y++)
				for (int x = 0; x < Image.width(); x++)
				{
					const QRgb Pixel = Image.pixel(x, y);
					if (qAlpha(Pixel) > 100 && qAlpha(Pixel) < 240)
					{
						QVERIFY(qRed(Pixel) > 245 && qGreen(Pixel) > 245 && qBlue(Pixel) > 245);
						TranslucentPixels++;
					}
				}

			QVERIFY(TranslucentPixels > 100);
			if (!Previous.isNull())
				QCOMPARE(Image, Previous);
			Previous = Image;
			Thumbnails.ReleaseThumbnail(Id);
		}
	}

	// Compare tile assembly with framebuffer readback, then force a partial final tile.
	lcView View(lcViewType::PartsList, Project.GetMainModel());
	View.SetOffscreenContext();
	View.MakeCurrent();
	for (const QSize Size : { QSize(64, 64), QSize(2051, 64) })
	{
		View.SetSize(Size.width(), Size.height());
		View.ZoomExtents(lcGeometryBoundsMode::AvailableOnly, false);
		QVERIFY(View.BeginRenderToImage(Size.width(), Size.height()));
		View.OnDraw();
		const QImage Image = View.GetRenderImage();
		QCOMPARE(Image.size(), Size);
		QCOMPARE(Image.format(), QImage::Format_ARGB32_Premultiplied);
		QCOMPARE(Image.pixel(0, 0), QRgb(0));
		QCOMPARE(Image.pixel(Size.width() - 1, Size.height() - 1), QRgb(0));
		const QImage Straight = Image.convertToFormat(QImage::Format_ARGB32);
		int TranslucentPixels = 0;
		for (int y = 0; y < Straight.height(); y++)
			for (int x = 0; x < Straight.width(); x++)
			{
				const QRgb Pixel = Straight.pixel(x, y);
				if (qAlpha(Pixel) > 100 && qAlpha(Pixel) < 240)
				{
					QVERIFY(qRed(Pixel) > 245 && qGreen(Pixel) > 245 && qBlue(Pixel) > 245);
					TranslucentPixels++;
				}
			}
		QVERIFY(TranslucentPixels > 100);
		if (Size.width() == 64)
			QCOMPARE(Image, View.GetRenderFramebufferImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
		View.EndRenderToImage();
	}

	// Instruction backgrounds retain their color and alpha; mode changes replace them.
	for (int Alpha : { 0, 128, 255 })
	{
		View.SetBackgroundOptions({ lcBackgroundMode::CustomColor, LC_RGBA(200, 100, 50, Alpha) });
		QVERIFY(View.BeginRenderToImage(64, 64));
		View.OnDraw();
		const QRgb Pixel = View.GetRenderImage().pixel(0, 0);
		QCOMPARE(qAlpha(Pixel), Alpha);
		QVERIFY(qAbs(qRed(Pixel) - qRound(200.0 * Alpha / 255.0)) <= 1);
		QVERIFY(qAbs(qGreen(Pixel) - qRound(100.0 * Alpha / 255.0)) <= 1);
		QVERIFY(qAbs(qBlue(Pixel) - qRound(50.0 * Alpha / 255.0)) <= 1);

		View.SetBackgroundOptions({ lcBackgroundMode::Transparent });
		View.OnDraw();
		QCOMPARE(View.GetRenderImage().pixel(0, 0), QRgb(0));
		View.EndRenderToImage();
	}

	View.SetBackgroundOptions({ lcBackgroundMode::CustomColor, LC_RGBA(200, 100, 50, 128) });
	View.SetBackgroundOptions({ lcBackgroundMode::Opaque });
	for (bool Gradient : { false, true })
	{
		lcGetPreferences().mBackgroundGradient = Gradient;
		QVERIFY(View.BeginRenderToImage(64, 64));
		View.OnDraw();
		QCOMPARE(qAlpha(View.GetRenderImage().pixel(0, 0)), 255);
		View.EndRenderToImage();
	}

	lcModel PartsModel(QString(), nullptr, true);
	PartsModel.SetPreviewPieceData(Project.GetMainModel()->GetPieceData(), lcGetColorIndex(4));
	const lcResult<QImage> PartsImage = PartsModel.GetPartsListImage(128, 1, LC_RGBA(48, 48, 48, 255), QFont(), Qt::transparent, false);
	QVERIFY(PartsImage);
	QVERIFY(!PartsImage.value().isNull());
	int BrightPixels = 0;
	for (int y = 0; y < PartsImage.value().height(); y++)
		for (int x = 0; x < PartsImage.value().width(); x++)
		{
			const QRgb Pixel = PartsImage.value().pixel(x, y);
			QVERIFY(qRed(Pixel) >= 48 && qGreen(Pixel) >= 48 && qBlue(Pixel) >= 48);
			if (qRed(Pixel) > 100)
				BrightPixels++;
		}
	QVERIFY2(BrightPixels > 10, qPrintable(QString("Only %1 bright pixels in %2x%3 parts list").arg(BrightPixels).arg(PartsImage.value().width()).arg(PartsImage.value().height())));
}

void lcModelInstanceTests::TextureDecalAlpha()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test decal alpha.");

	QVERIFY(lcContext::InitializeRenderer(true));
	lcContext Context;
	Context.SetOffscreenContext();
	Context.MakeCurrent();
	Context.SetDefaultState();
	Context.SetWorldMatrix(lcMatrix44Identity());
	Context.SetViewMatrix(lcMatrix44Identity());
	Context.SetProjectionMatrix(lcMatrix44Identity());
	Context.EnableDepthTest(false);

	QOpenGLFramebufferObject Framebuffer(8, 8);
	QVERIFY(Framebuffer.isValid());
	QVERIFY(Framebuffer.bind());
	Context.SetViewport(0, 0, 8, 8);
	const quint32 Normal = lcPackNormal(lcVector3(0, 0, 1));
	const lcVertexTextured Vertices[] =
	{
		{ lcVector3(-1, -1, 0), Normal, lcVector2(0, 0) },
		{ lcVector3(3, -1, 0), Normal, lcVector2(2, 0) },
		{ lcVector3(-1, 3, 0), Normal, lcVector2(0, 2) }
	};
	Context.SetVertexBufferPointer(Vertices);
	Context.SetVertexFormat(0, 3, 1, 2, 0, true);

	for (int TexelAlpha : { 0, 128, 255 })
	{
		QImage Image(2, 2, QImage::Format_RGBA8888);
		Image.fill(QColor(0, 0, 255, TexelAlpha));
		lcTexture Texture;
		Texture.SetImage(Image, LC_TEXTURE_POINT);
		Texture.Upload(&Context);
		QVERIFY(Texture.IsReady());
		Context.BindTexture2D(&Texture);

		for (int MaterialAlpha : { 0, 128, 255 })
			for (lcMaterialType Material : { lcMaterialType::UnlitTextureDecal, lcMaterialType::FakeLitTextureDecal })
				for (bool Blend : { false, true })
				{
					const float TextureAlpha = TexelAlpha / 255.0f;
					const float BaseAlpha = MaterialAlpha / 255.0f;
					const float Alpha = TextureAlpha + BaseAlpha * (1.0f - TextureAlpha);
					Context.ClearColorAndDepth(lcVector4(0, 0, 0, 0));
					Context.SetMaterial(Material);
					Context.SetColor(1, 0, 0, BaseAlpha);
					Context.EnableColorBlend(Blend);
					Context.DrawPrimitives(GL_TRIANGLES, 0, 3);
					quint8 Pixel[4] = {};
					Context.glReadPixels(4, 4, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, Pixel);
					QVERIFY(qAbs(int(Pixel[3]) - qRound(Alpha * 255)) <= 1);
					if (Material == lcMaterialType::UnlitTextureDecal && Alpha > 0.0f)
					{
						const float Scale = Blend ? 255.0f : 255.0f / Alpha;
						QVERIFY(qAbs(int(Pixel[0]) - qRound(BaseAlpha * (1.0f - TextureAlpha) * Scale)) <= 1);
						QCOMPARE(Pixel[1], quint8(0));
						QVERIFY(qAbs(int(Pixel[2]) - qRound(TextureAlpha * Scale)) <= 1);
					}
				}

		Context.ClearTexture2D();
	}
	Context.ClearResources();
}

void lcModelInstanceTests::TextureMipmaps()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test uploaded mipmaps.");

	QVERIFY(lcContext::InitializeRenderer(true));
	lcContext Context;
	Context.SetOffscreenContext();
	Context.MakeCurrent();
	Context.SetDefaultState();
	QOpenGLFramebufferObject Framebuffer(4, 4);
	QVERIFY(Framebuffer.isValid());
	QVERIFY(Framebuffer.bind());

	for (QImage::Format Format : { QImage::Format_RGBA8888, QImage::Format_RGB888, QImage::Format_Alpha8 })
		for (const QSize Size : { QSize(4, 4), QSize(4, 1), QSize(1, 4) })
		{
			QImage Image(Size, Format);
			for (int y = 0; y < Size.height(); y++)
				for (int x = 0; x < Size.width(); x++)
				{
					const bool Opaque = (x + y) % 2 == 0;
					if (Format == QImage::Format_Alpha8)
						Image.scanLine(y)[x] = Opaque ? 255 : 0;
					else if (Format == QImage::Format_RGBA8888)
						Image.setPixel(x, y, Opaque ? qRgba(255, 0, 0, 255) : qRgba(0, 0, 255, 0));
					else
						Image.setPixel(x, y, qRgb(255, 0, 0));
				}

			lcTexture Texture;
			Texture.SetImage(Image, LC_TEXTURE_MIPMAPS);
			Texture.Upload(&Context);
			QVERIFY(Texture.IsReady());

			// Sample the uploaded 1x1 mip through the production decal shader.
			QVERIFY(Framebuffer.bind());
			Context.SetViewport(0, 0, 1, 1);
			Context.SetWorldMatrix(lcMatrix44Identity());
			Context.SetViewMatrix(lcMatrix44Identity());
			Context.SetProjectionMatrix(lcMatrix44Identity());
			Context.EnableDepthTest(false);
			Context.EnableColorBlend(false);
			Context.SetMaterial(lcMaterialType::UnlitTextureDecal);
			Context.SetColor(0, 0, 0, 0);
			Context.BindTexture2D(&Texture);
			const float Vertices[] = { -1, -1, 0, 0, 3, -1, 2, 0, -1, 3, 0, 2 };
			Context.SetVertexBufferPointer(Vertices);
			Context.SetVertexFormat(0, 2, 0, 2, 0, false);
			Context.DrawPrimitives(GL_TRIANGLES, 0, 3);
			quint8 Pixel[4] = {};
			Context.glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, Pixel);
			QVERIFY(qAbs(int(Pixel[3]) - (Format == QImage::Format_RGB888 ? 255 : 128)) <= 1);
			if (Format != QImage::Format_Alpha8)
			{
				QCOMPARE(Pixel[0], quint8(255));
				QCOMPARE(Pixel[1], quint8(0));
				QCOMPARE(Pixel[2], quint8(0));
			}
			Context.ClearTexture2D();
		}
	Context.ClearResources();
}

void lcModelInstanceTests::CommandLineBackground_data()
{
	QTest::addColumn<QStringList>("Arguments");
	QTest::addColumn<bool>("Valid");
	QTest::addColumn<int>("Mode");
	QTest::addColumn<quint32>("Top");
	QTest::addColumn<quint32>("Bottom");
	const auto Row = [](const char* Name, const QStringList& Arguments, bool Valid, lcBackgroundMode Mode, quint32 Top, quint32 Bottom)
	{
		QTest::newRow(Name) << Arguments << Valid << int(Mode) << Top << Bottom;
	};
	Row("default-png", { "-i", "out.png" }, true, lcBackgroundMode::Transparent, 0, 0);
	Row("default-jpeg", { "-i", "out.jpg" }, true, lcBackgroundMode::CustomColor, LC_RGBA(255, 255, 255, 255), 0);
	Row("default-extensionless", { "-i", "out" }, true, lcBackgroundMode::Transparent, 0, 0);
	Row("transparent", { "--background", "transparent", "-i", "out.png" }, true, lcBackgroundMode::Transparent, 0, 0);
	Row("rgb", { "-i", "out.png", "--background", "#123456" }, true, lcBackgroundMode::CustomColor, LC_RGBA(0x12, 0x34, 0x56, 255), 0);
	Row("rgba", { "-i", "out.png", "--background", "#12345678" }, true, lcBackgroundMode::CustomColor, LC_RGBA(0x12, 0x34, 0x56, 0x78), 0);
	Row("gradient", { "-i", "out.png", "--background-gradient", "#FF000080", "#0000FF00" }, true, lcBackgroundMode::CustomGradient, LC_RGBA(255, 0, 0, 128), LC_RGBA(0, 0, 255, 0));
	Row("gradient-transparent", { "-i", "out.png", "--background-gradient", "#12345678", "transparent" }, true, lcBackgroundMode::CustomGradient, LC_RGBA(0x12, 0x34, 0x56, 0x78), 0);
	Row("bare-rgb", { "-i", "out.png", "--background", "123456" }, true, lcBackgroundMode::CustomColor, LC_RGBA(0x12, 0x34, 0x56, 255), 0);
	Row("bare-rgba", { "-i", "out.png", "--background", "12345678" }, true, lcBackgroundMode::CustomColor, LC_RGBA(0x12, 0x34, 0x56, 0x78), 0);
	Row("bare-gradient", { "-i", "out.png", "--background-gradient", "FF000080", "0000FF00" }, true, lcBackgroundMode::CustomGradient, LC_RGBA(255, 0, 0, 128), LC_RGBA(0, 0, 255, 0));
	Row("jpeg-opaque", { "-i", "out.jpg", "--background", "#102030FF" }, true, lcBackgroundMode::CustomColor, LC_RGBA(16, 32, 48, 255), 0);
	Row("jpeg-gradient", { "-i", "out.jpg", "--background-gradient", "#102030", "#405060" }, true, lcBackgroundMode::CustomGradient, LC_RGBA(16, 32, 48, 255), LC_RGBA(64, 80, 96, 255));
	const std::vector<std::pair<const char*, QStringList>> Invalid =
	{
		{ "missing-color", { "-i", "out.png", "--background" } },
		{ "missing-bottom", { "-i", "out.png", "--background-gradient", "#123456" } },
		{ "missing-both", { "-i", "out.png", "--background-gradient" } },
		{ "empty-color", { "-i", "out.png", "--background", "" } },
		{ "short-color", { "-i", "out.png", "--background", "#123" } },
		{ "named-color", { "-i", "out.png", "--background", "red" } },
		{ "invalid-hex", { "-i", "out.png", "--background", "#12345G" } },
		{ "invalid-bottom", { "-i", "out.png", "--background-gradient", "#123456", "bad" } },
		{ "duplicate", { "-i", "out.png", "--background", "transparent", "--background", "#123456" } },
		{ "conflicting", { "-i", "out.png", "--background", "transparent", "--background-gradient", "#123456", "#654321" } },
		{ "repeated-gradient", { "-i", "out.png", "--background-gradient", "#123456", "#654321", "--background-gradient", "#000000", "#FFFFFF" } },
		{ "jpeg-transparent", { "-i", "out.jpg", "--background", "transparent" } },
		{ "jpeg-translucent", { "-i", "out.jpg", "--background", "#FFFFFF80" } },
		{ "jpeg-alpha-gradient", { "-i", "out.jpg", "--background-gradient", "#FFFFFF", "#00000080" } },
		{ "without-image", { "--background", "transparent" } },
		{ "unsupported-format", { "-i", "out.not-an-image" } }
	};
	for (const auto& [Name, Arguments] : Invalid)
		Row(Name, Arguments, false, lcBackgroundMode::Default, 0, 0);
}

void lcModelInstanceTests::CommandLineBackground()
{
	QFETCH(QStringList, Arguments);
	QFETCH(bool, Valid);
	QFETCH(int, Mode);
	QFETCH(quint32, Top);
	QFETCH(quint32, Bottom);
	QTemporaryFile Model;
	QVERIFY(Model.open());
	Arguments.prepend(Model.fileName());
	Arguments.prepend("leocad");
	const lcCommandLineOptions Options(Arguments);
	QCOMPARE(Options.ParseOK, Valid);
	if (Valid)
	{
		QCOMPARE(int(Options.ImageBackground.Mode), Mode);
		QCOMPARE(Options.ImageBackground.Color, Top);
		QCOMPARE(Options.ImageBackground.GradientBottomColor, Bottom);
	}
	else
		QVERIFY(!Options.StdErr.isEmpty());
}

void lcModelInstanceTests::CommandLineOptions()
{
	const lcCommandLineOptions Empty(QStringList{});
	QVERIFY(Empty.ParseOK);
	QVERIFY(!Empty.Exit);
	QVERIFY(!Empty.SaveImage);

	const lcCommandLineOptions Help({ "leocad", "--help" });
	QVERIFY(Help.ParseOK);
	QVERIFY(Help.Exit);
	QVERIFY(Help.StdOut.contains("--background-gradient"));

	QTemporaryFile Model;
	QVERIFY(Model.open());
	const lcCommandLineOptions Image({ "leocad", Model.fileName(), "-i", "out.png",
		"--width", "123", "--height", "456", "--aa-samples", "4", "--stud-style", "3", "--viewpoint", "front" });
	QVERIFY(Image.ParseOK);
	QVERIFY(Image.SaveImage);
	QCOMPARE(Image.ImageWidth, 123);
	QCOMPARE(Image.ImageHeight, 456);
	QCOMPARE(Image.AASamples, 4);
	QCOMPARE(int(Image.StudStyle), 3);
	QCOMPARE(Image.Viewpoint, lcViewpoint::Front);
}

void lcModelInstanceTests::CommandLineParsing_data()
{
	QTest::addColumn<QStringList>("Arguments");
	QTest::addColumn<bool>("Valid");
	QTest::addColumn<QString>("Message");
	QTest::newRow("image-range") << QStringList{ "-w", "64", "-h", "32", "--from", "1", "--to", "2" } << true << QString();
	QTest::newRow("negative-camera-angles") << QStringList{ "--camera-angles", "-30", "45", "--orthographic" } << true << QString();
	QTest::newRow("fade-color") << QStringList{ "--fade-steps-color", "#11223380" } << true << QString();
	QTest::newRow("optional-export-name") << QStringList{ "--export-wavefront", "--width", "160" } << true << QString();
	QTest::newRow("camera-precedence") << QStringList{ "--camera", "Camera", "--viewpoint", "front" } << true << QString("--viewpoint is ignored when --camera is set.");
	QTest::newRow("missing-integer") << QStringList{ "--width" } << false << QString("Not enough parameters for the '--width' option.");
	QTest::newRow("invalid-integer") << QStringList{ "--width", "0" } << false << QString("Invalid parameter value");
	QTest::newRow("invalid-step") << QStringList{ "--from", "0" } << false << QString("Invalid parameter value");
	QTest::newRow("invalid-float") << QStringList{ "--line-width", "bad" } << false << QString("Invalid parameter value");
	QTest::newRow("missing-array") << QStringList{ "--camera-angles", "30" } << false << QString("Not enough parameters");
	QTest::newRow("invalid-camera") << QStringList{ "--camera-position", "0", "0", "0", "0", "0", "0", "0", "1", "0" } << false << QString("camera position is too close");
	QTest::newRow("invalid-clipping") << QStringList{ "--zplanes", "2", "1" } << false << QString("1 <= <near> < <far>");
	QTest::newRow("invalid-color") << QStringList{ "--highlight-color", "not-a-color" } << false << QString("Invalid color");
	QTest::newRow("invalid-shading") << QStringList{ "--shading", "unknown" } << false << QString("Invalid parameter value");
	QTest::newRow("invalid-samples") << QStringList{ "--aa-samples", "3" } << false << QString("Invalid parameter value");
	QTest::newRow("unknown-option") << QStringList{ "--unknown" } << false << QString("Unknown option");
}

void lcModelInstanceTests::CommandLineParsing()
{
	QFETCH(QStringList, Arguments);
	QFETCH(bool, Valid);
	QFETCH(QString, Message);
	QTemporaryFile Model;
	QVERIFY(Model.open());
	Arguments.prepend(Model.fileName());
	Arguments.prepend("leocad");
	const lcCommandLineOptions Options(Arguments);
	QCOMPARE(Options.ParseOK, Valid);
	if (!Message.isEmpty())
		QVERIFY(Options.StdErr.contains(Message));
	if (Options.SaveWavefront)
	{
		QVERIFY(Options.SaveWavefrontName.isEmpty());
		QCOMPARE(Options.ImageWidth, 160);
	}
	if (Options.SetFadeStepsColor)
		QCOMPARE(Options.FadeStepsColor, LC_RGBA(0x11, 0x22, 0x33, 0x80));
}

void lcModelInstanceTests::CommandLineColors_data()
{
	QTest::addColumn<QString>("Input");
	QTest::addColumn<bool>("Valid");
	QTest::addColumn<quint32>("Expected");
	QTest::newRow("rgb") << QString("#123456") << true << LC_RGBA(0x12, 0x34, 0x56, 255);
	QTest::newRow("rgba") << QString("#12345678") << true << LC_RGBA(0x12, 0x34, 0x56, 0x78);
	QTest::newRow("lowercase") << QString("#abcdef80") << true << LC_RGBA(0xab, 0xcd, 0xef, 0x80);
	QTest::newRow("zero-alpha") << QString("#12345600") << true << LC_RGBA(0x12, 0x34, 0x56, 0);
	QTest::newRow("bare-rgb") << QString("123456") << true << LC_RGBA(0x12, 0x34, 0x56, 255);
	QTest::newRow("bare-rgba") << QString("12345678") << true << LC_RGBA(0x12, 0x34, 0x56, 0x78);
	QTest::newRow("bare-lowercase") << QString("abcdef80") << true << LC_RGBA(0xab, 0xcd, 0xef, 0x80);
	QTest::newRow("bare-zero-alpha") << QString("12345600") << true << LC_RGBA(0x12, 0x34, 0x56, 0);
	QTest::newRow("transparent") << QString("transparent") << true << quint32(0);
	QTest::newRow("transparent-case") << QString("TRANSPARENT") << true << quint32(0);
	for (const QString& Input : { QString(), QString("red"), QString("#123"), QString("#12345G"),
		QString("#123456789"), QString("#123456789ABC"), QString("#12345678 "),
		QString("123"), QString("1234567"), QString("12345G"), QString("0x123456"), QString("##123456"), QString("123456\n") })
		QTest::newRow(qPrintable("invalid-" + Input)) << Input << false << quint32(0);
}

void lcModelInstanceTests::CommandLineColors()
{
	QFETCH(QString, Input);
	QFETCH(bool, Valid);
	QFETCH(quint32, Expected);
	const std::pair<const char*, quint32 lcCommandLineOptions::*> Options[] =
	{
		{ "--fade-steps-color", &lcCommandLineOptions::FadeStepsColor },
		{ "--highlight-color", &lcCommandLineOptions::HighlightColor },
		{ "--stud-cylinder-color", &lcCommandLineOptions::StudCylinderColor },
		{ "-scc", &lcCommandLineOptions::StudCylinderColor },
		{ "--edge-color", &lcCommandLineOptions::PartEdgeColor },
		{ "-ec", &lcCommandLineOptions::PartEdgeColor },
		{ "--black-edge-color", &lcCommandLineOptions::BlackEdgeColor },
		{ "-bec", &lcCommandLineOptions::BlackEdgeColor },
		{ "--dark-edge-color", &lcCommandLineOptions::DarkEdgeColor },
		{ "-dec", &lcCommandLineOptions::DarkEdgeColor }
	};

	for (const auto& [Option, Member] : Options)
	{
		const lcCommandLineOptions Parsed({ "leocad", "--stud-style", "6", Option, Input });
		QCOMPARE(Parsed.ParseOK, Valid);
		if (Valid)
			QCOMPARE(Parsed.*Member, Expected);
		else
			QVERIFY(Parsed.StdErr.contains("Invalid color"));
	}
}

void lcModelInstanceTests::CommandLineColorValidation_data()
{
	QTest::addColumn<bool>("Automatic");
	QTest::addColumn<int>("SavedStyle");
	QTest::addColumn<bool>("OverridesEnabled");
	QTest::addColumn<QStringList>("Arguments");
	QTest::addColumn<int>("Warnings");
	QTest::addColumn<QString>("Message");
	const QString RequiresAuto = "requires automatic edge colors";
	const QString RequiresStyle = "requires a high contrast stud style";
	const QString Precedence = "Automatic edge colors take precedence";
	const auto Row = [](const char* Name, bool Automatic, int SavedStyle, bool Enabled, const QStringList& Arguments, int Warnings, const QString& Message)
	{
		QTest::newRow(Name) << Automatic << SavedStyle << Enabled << Arguments << Warnings << Message;
	};
	Row("inherited-combination-no-warning", true, 6, true, {}, 0, {});
	Row("contrast-default-inactive", false, 0, true, { "--color-contrast", "0.5" }, 1, RequiresAuto);
	Row("contrast-changed-inactive", false, 0, true, { "--color-contrast", "0.8" }, 1, RequiresAuto);
	Row("contrast-inherited-auto", true, 0, true, { "--color-contrast", "0.5" }, 0, {});
	Row("contrast-cli-auto", false, 0, true, { "--color-contrast", "0.5", "--automate-edge-color" }, 0, {});
	Row("contrast-high-contrast-only", false, 6, true, { "--color-contrast", "0.5" }, 1, RequiresAuto);
	Row("threshold-cli-high-contrast", false, 0, true, { "--stud-style", "6", "--light-dark-value", "0.8" }, 0, {});
	Row("threshold-inherited-high-contrast", false, 6, true, { "--light-dark-value", "0.5" }, 0, {});
	Row("threshold-overridden-style", false, 6, true, { "--stud-style", "0", "--light-dark-value", "0.5" }, 1, "requires automatic edge colors or a high contrast stud style");
	Row("threshold-auto", true, 0, true, { "--light-dark-value", "0.5" }, 0, {});
	Row("threshold-neither-mode", false, 0, true, { "--light-dark-value", "0.5" }, 1, "requires automatic edge colors or a high contrast stud style");
	Row("saved-style-does-not-suppress-warning", false, 6, true, { "--stud-style", "0", "--edge-color", "#123456" }, 1, RequiresStyle);
	Row("edge-cli-high-contrast", false, 0, true, { "--stud-style", "6", "--edge-color", "#123456" }, 0, {});
	Row("manual-edge-under-auto", true, 6, true, { "--edge-color", "#123456" }, 1, Precedence);
	Row("manual-edge-cli-auto", false, 6, true, { "--edge-color", "#123456", "--automate-edge-color" }, 1, Precedence);
	Row("cylinder-under-auto", true, 6, true, { "--stud-cylinder-color", "#010203" }, 0, {});
	Row("disable-cylinder-under-auto", true, 6, true, { "--disable-stud-cylinder-color" }, 0, {});
	Row("same-cylinder-color-inactive", false, 0, true, { "--stud-cylinder-color", "#1B2A34" }, 1, RequiresStyle);
	Row("already-disabled-cylinder-inactive", false, 0, false, { "--disable-stud-cylinder-color" }, 1, RequiresStyle);
	Row("same-edge-color-inactive", false, 0, true, { "--edge-color", "#000000" }, 1, RequiresStyle);
	Row("already-disabled-edge-inactive", false, 0, false, { "--disable-edge-color" }, 1, RequiresStyle);
	Row("already-disabled-black-inactive", false, 0, false, { "--disable-black-edge-color" }, 1, RequiresStyle);
	Row("already-disabled-dark-inactive", false, 0, false, { "--disable-dark-edge-color" }, 1, RequiresStyle);
	Row("black-under-auto", true, 6, true, { "--black-edge-color", "#FFFFFF" }, 1, Precedence);
	Row("dark-under-auto", true, 6, true, { "--dark-edge-color", "#1B2A34" }, 1, Precedence);
	Row("only-edge-warning-with-cylinder", true, 6, true, { "--stud-cylinder-color", "#010203", "--edge-color", "#000000" }, 1, "'--edge-color'");
	Row("repeated-edge-warning-once", false, 0, true, { "--edge-color", "#000000", "--edge-color", "#123456" }, 1, RequiresStyle);
	Row("alias-preserved", false, 0, true, { "-ec", "#000000" }, 1, "'-ec'");
}

void lcModelInstanceTests::CommandLineColorValidation()
{
	QFETCH(bool, Automatic);
	QFETCH(int, SavedStyle);
	QFETCH(bool, OverridesEnabled);
	QFETCH(QStringList, Arguments);
	QFETCH(int, Warnings);
	QFETCH(QString, Message);
	struct ScopedProfile
	{
		std::map<QString, QVariant> Values;
		ScopedProfile()
		{
			QSettings Settings;
			const QStringList Keys = { "StudStyle", "AutomateEdgeColor", "PartEdgeContrast", "PartColorValueLDIndex",
				"StudCylinderColorEnabled", "StudCylinderColor", "PartEdgeColorEnabled", "PartEdgeColor",
				"BlackEdgeColorEnabled", "BlackEdgeColor", "DarkEdgeColorEnabled", "DarkEdgeColor" };
			for (const QString& Key : Keys)
				Values.emplace("Settings/" + Key, Settings.value("Settings/" + Key));
		}
		~ScopedProfile()
		{
			QSettings Settings;
			for (const auto& [Key, Value] : Values)
			{
				if (Value.isValid())
					Settings.setValue(Key, Value);
				else
					Settings.remove(Key);
			}
		}
	} Profile;
	lcSetProfileStudStyle(static_cast<lcStudStyle>(SavedStyle));
	lcSetProfileInt(LC_PROFILE_AUTOMATE_EDGE_COLOR, Automatic);
	lcSetProfileFloat(LC_PROFILE_PART_EDGE_CONTRAST, 0.5f);
	lcSetProfileFloat(LC_PROFILE_PART_COLOR_VALUE_LD_INDEX, 0.5f);
	lcSetProfileInt(LC_PROFILE_STUD_CYLINDER_COLOR_ENABLED, OverridesEnabled);
	lcSetProfileInt(LC_PROFILE_PART_EDGE_COLOR_ENABLED, OverridesEnabled);
	lcSetProfileInt(LC_PROFILE_BLACK_EDGE_COLOR_ENABLED, OverridesEnabled);
	lcSetProfileInt(LC_PROFILE_DARK_EDGE_COLOR_ENABLED, OverridesEnabled);
	lcSetProfileUInt(LC_PROFILE_STUD_CYLINDER_COLOR, LC_RGBA(27, 42, 52, 255));
	lcSetProfileUInt(LC_PROFILE_PART_EDGE_COLOR, LC_RGBA(0, 0, 0, 255));
	lcSetProfileUInt(LC_PROFILE_BLACK_EDGE_COLOR, LC_RGBA(255, 255, 255, 255));
	lcSetProfileUInt(LC_PROFILE_DARK_EDGE_COLOR, LC_RGBA(27, 42, 52, 255));
	Arguments.prepend("leocad");
	const lcCommandLineOptions Options(Arguments);
	QVERIFY(Options.ParseOK);
	QCOMPARE(Options.StdErr.count(QLatin1Char('\n')), Warnings);
	if (!Message.isEmpty())
		QVERIFY2(Options.StdErr.contains(Message), qPrintable(Options.StdErr));
	if (Options.AutomateEdgeColor && lcIsHighContrast(Options.StudStyle))
		QVERIFY(!Options.StdErr.contains("stud-cylinder-color"));
}

void lcModelInstanceTests::GradientBackground()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test gradient backgrounds.");
	QVERIFY(lcContext::InitializeRenderer(true));
	lcModel Model(QString(), nullptr, true);
	lcView View(lcViewType::PartsList, &Model);
	View.SetOffscreenContext();
	View.MakeCurrent();
	lcBackgroundOptions Background;
	Background.Mode = lcBackgroundMode::CustomGradient;
	Background.Color = LC_RGBA(255, 0, 0, 128);
	Background.GradientBottomColor = LC_RGBA(0, 0, 255, 0);
	View.SetBackgroundOptions(Background);
	for (const QSize Size : { QSize(32, 32), QSize(32, 2051) })
	{
		QVERIFY(View.BeginRenderToImage(Size.width(), Size.height()));
		View.OnDraw();
		const QImage Image = View.GetRenderImage();
		QCOMPARE(Image.size(), Size);
		for (int y = 0; y < Size.height(); y++)
		{
			const QRgb Pixel = Image.pixel(0, y);
			const int ExpectedAlpha = qRound(128.0 * (1.0 - (y + 0.5) / Size.height()));
			QVERIFY(qAbs(qAlpha(Pixel) - ExpectedAlpha) <= 2);
			QVERIFY(qAbs(qRed(Pixel) - ExpectedAlpha) <= 2);
			QCOMPARE(qGreen(Pixel), 0);
			QCOMPARE(qBlue(Pixel), 0); // A transparent blue endpoint must not tint the gradient.
		}
		QByteArray Data;
		QBuffer Buffer(&Data);
		QVERIFY(Buffer.open(QIODevice::WriteOnly));
		QVERIFY(Image.save(&Buffer, "PNG"));
		const QImage Reloaded = QImage::fromData(Data, "PNG");
		QVERIFY(qAbs(qAlpha(Reloaded.pixel(0, Size.height() / 2)) - 64) <= 3);
		QVERIFY(qRed(Reloaded.pixel(0, Size.height() / 2)) > 245);
		QCOMPARE(qBlue(Reloaded.pixel(0, Size.height() / 2)), 0);
		View.EndRenderToImage();
	}

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	View.SetBackgroundOptions({ lcBackgroundMode::CustomColor, LC_RGBA(255, 255, 255, 255) });
	View.SetSize(32, 32);
	const QString JPEGPath = Directory.filePath("opaque.jpg");
	QVERIFY(View.SaveStepImages(JPEGPath, false, 1, 1, {}));
	QCOMPARE(QImageReader::imageFormat(JPEGPath), QByteArray("jpeg"));
	const QImage JPEG(JPEGPath);
	QVERIFY(!JPEG.isNull());
	QVERIFY(!JPEG.hasAlphaChannel());
	QCOMPARE(JPEG.pixelColor(0, 0), QColor(Qt::white));

	// Model exports accept the same options, including color and gradient alpha.
	for (const lcBackgroundOptions& OutputBackground : { Background,
		lcBackgroundOptions{ lcBackgroundMode::CustomColor, LC_RGBA(200, 100, 50, 128) },
		lcBackgroundOptions{ lcBackgroundMode::Transparent }, lcBackgroundOptions{ lcBackgroundMode::Opaque } })
	{
		const QString Path = Directory.filePath(QString("model-%1.png").arg(int(OutputBackground.Mode)));
		QVERIFY(Model.SaveStepImages(Path, false, false, 32, 32, 1, 1, OutputBackground));
		const QImage Image(Path);
		QVERIFY(!Image.isNull());
		const QColor Color = Image.pixelColor(0, 16);
		if (OutputBackground.Mode == lcBackgroundMode::CustomGradient)
		{
			QVERIFY(qAbs(Color.alpha() - 62) <= 2);
			QVERIFY(Color.red() > 245);
			QCOMPARE(Color.blue(), 0);
		}
		else if (OutputBackground.Mode == lcBackgroundMode::CustomColor)
			QCOMPARE(Color.alpha(), 128);
		else if (OutputBackground.Mode == lcBackgroundMode::Transparent)
			QCOMPARE(Color.alpha(), 0);
		else
			QCOMPARE(Color.alpha(), 255);
	}
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
