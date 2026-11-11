#include "lc_global.h"
#include "lc_application.h"
#include "lc_assetloader.h"
#include "lc_blenderpreferences.h"
#include "lc_commandline.h"
#include "lc_colors.h"
#include "lc_context.h"
#include "lc_findreplacewidget.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "lc_partselectionwidget.h"
#include "lc_partselectionpopup.h"
#include "lc_profile.h"
#include "lc_qutils.h"
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
#include "lc_traintrack.h"
#include "lc_view.h"
#include "lc_viewwidget.h"
#include "lc_zipfile.h"
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

class lcModelInstanceTestTrackInfo : public lcTrainTrackInfo
{
public:
	using lcTrainTrackInfo::mSleepers;
};

struct lcModelInstanceTestSleepers
{
	std::map<size_t, lcPieceData*> Previous = lcModelInstanceTestTrackInfo::mSleepers;
	~lcModelInstanceTestSleepers() { lcModelInstanceTestTrackInfo::mSleepers = std::move(Previous); }
};

class lcModelInstanceTests : public QObject
{
	Q_OBJECT

private slots:
	void AssetLoaderIndependentBuilds();
	void AssetLoaderCancelPreparedBuild();
	void AssetUpdateThrottle();
	void ArchiveSubpartPrecedence();
	void LgeoLongColorTokens();
	void ZipExtractionIntegrity_data();
	void ZipExtractionIntegrity();
#if !LC_DISABLE_BLENDER
	void AddonExtraction_data();
	void AddonExtraction();
#endif
	void TexmapFallbackBranches();
	void TexmapFallbackLoading();
	void TexmapFallbackIncludes();
	void MeshCacheValidation_data();
	void MeshCacheValidation();
	void MeshSectionCountOverflow();
	void EmbeddedDataAfterLastPieceDeletion();
	void AxisAlignedRotations_data();
	void AxisAlignedRotations();
	void AxisAlignedRotationTolerance();
	void TrainTrackConnections_data();
	void TrainTrackConnections();
	void TrainTrackFocusBeforeDraw();
	void TrainTrackConnectionVisibility();
	void TrainTrackInsertionSection_data();
	void TrainTrackInsertionSection();
	void TrainTrackDragSnap_data();
	void TrainTrackDragSnap();
	void TrainTrackScreenSnap_data();
	void TrainTrackScreenSnap();
	void TrainTrackSnapDepth_data();
	void TrainTrackSnapDepth();
	void TrainTrackDragMovement();
	void TrainTrackDragView_data();
	void TrainTrackDragView();
	void TrainTrackSideViewDrag_data();
	void TrainTrackSideViewDrag();
	void TrainTrackDragSleepers_data();
	void TrainTrackDragSleepers();
	void KeyboardRotationAxes_data();
	void KeyboardRotationAxes();
	void RotateToolMouseShortcuts_data();
	void RotateToolMouseShortcuts();
	void RepeatedInstances();
	void InterfaceTransforms();
	void CommandLineBackground_data();
	void CommandLineBackground();
	void GradientBackground();
	void SaveDialogSuffix();
	void CommandLineOptions();
	void CommandLineParsing_data();
	void CommandLineParsing();
	void CommandLineColors_data();
	void CommandLineColors();
	void CommandLineColorValidation_data();
	void CommandLineColorValidation();
	void ThumbnailShading();
	void PreferencesPreservePartFilter_data();
	void PreferencesPreservePartFilter();
	void TranslucentThumbnails_data();
	void TranslucentThumbnails();
	void TextureDecalAlpha();
	void TextureMipmaps();
	void ProjectLoadErrors();
	void ProjectLegacyFormats_data();
	void ProjectLegacyFormats();
	void WavefrontSaveFailures();
	void CompletedExportEvaluatesExternalModels();
	void ProjectFilesCategory();
	void ProjectFilesRecoveryAndCycles();
	void ProjectFilePopupOwnership();
	void ProjectFileInsertionValidation();
	void FindReplaceViewIsolation();
	void TexturedDirectGeometry3DS();

private:
	static QByteArray CreateAddonArchive(const QByteArray& Name, const QByteArray& Data, quint32 Mode, quint32 DeclaredSize, bool BadCrc, bool Deflated);
	void SeparateRotations(lcMainWindow* Window, lcModel* Model);
	void CameraContexts(lcMainWindow* Window, lcView* View, lcModel* Main, lcModel* A, lcModel* B);
};

QByteArray lcModelInstanceTests::CreateAddonArchive(const QByteArray& Name, const QByteArray& Data, quint32 Mode, quint32 DeclaredSize, bool BadCrc, bool Deflated)
{
	quint32 Crc = 0xffffffff;
	for (unsigned char Byte : Data)
	{
		Crc ^= Byte;
		for (int Bit = 0; Bit < 8; Bit++)
			Crc = (Crc >> 1) ^ (0xedb88320u & (0u - (Crc & 1u)));
	}
	Crc ^= 0xffffffff;
	if (BadCrc)
		Crc ^= 1;

	QByteArray Compressed = Data;
	if (Deflated)
	{
		const QByteArray Zlib = qCompress(Data);
		Compressed = Zlib.mid(6, Zlib.size() - 10);
	}
	const quint16 Method = Deflated ? 8 : 0;
	QByteArray Zip;
	QDataStream Stream(&Zip, QIODevice::WriteOnly);
	Stream.setByteOrder(QDataStream::LittleEndian);
	Stream << quint32(0x04034b50) << quint16(20) << quint16(0) << Method << quint32(0)
		<< Crc << quint32(Compressed.size()) << DeclaredSize << quint16(Name.size()) << quint16(0);
	Stream.writeRawData(Name.constData(), Name.size());
	Stream.writeRawData(Compressed.constData(), Compressed.size());
	const quint32 CentralOffset = static_cast<quint32>(Zip.size());
	Stream << quint32(0x02014b50) << quint16(0x0314) << quint16(20) << quint16(0) << Method << quint32(0)
		<< Crc << quint32(Compressed.size()) << DeclaredSize << quint16(Name.size()) << quint16(0)
		<< quint16(0) << quint16(0) << quint16(0) << quint32(Mode << 16) << quint32(0);
	Stream.writeRawData(Name.constData(), Name.size());
	const quint32 CentralSize = static_cast<quint32>(Zip.size()) - CentralOffset;
	Stream << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(1) << quint16(1)
		<< CentralSize << CentralOffset << quint16(0);
	return Zip;
}

void lcModelInstanceTests::ZipExtractionIntegrity_data()
{
	QTest::addColumn<bool>("Deflated");
	QTest::addColumn<bool>("BadCrc");
	QTest::addColumn<int>("ExtraBytes");
	QTest::addColumn<quint32>("Limit");
	QTest::addColumn<bool>("Accepted");
	for (bool Deflated : { false, true })
	{
		const QByteArray Prefix = Deflated ? "deflated-" : "stored-";
		QTest::newRow((Prefix + "valid").constData()) << Deflated << false << 0 << quint32(100) << true;
		QTest::newRow((Prefix + "prefix").constData()) << Deflated << false << 0 << quint32(3) << true;
		QTest::newRow((Prefix + "bad-crc").constData()) << Deflated << true << 0 << quint32(100) << false;
		QTest::newRow((Prefix + "truncated").constData()) << Deflated << false << 10 << quint32(100) << false;
	}
}

void lcModelInstanceTests::ZipExtractionIntegrity()
{
	QFETCH(bool, Deflated);
	QFETCH(bool, BadCrc);
	QFETCH(int, ExtraBytes);
	QFETCH(quint32, Limit);
	QFETCH(bool, Accepted);
	const QByteArray Data("payload");
	const QByteArray Zip = CreateAddonArchive("test", Data, 0100644, Data.size() + ExtraBytes, BadCrc, Deflated);
	std::unique_ptr<lcMemFile> Source = std::make_unique<lcMemFile>();
	Source->WriteBuffer(Zip.constData(), Zip.size());
	lcZipFile Archive;
	QVERIFY(Archive.OpenRead(std::move(Source)));
	lcMemFile Output;
	QCOMPARE(Archive.ExtractFile("test", Output, Limit), Accepted);
	if (Accepted)
		QCOMPARE(QByteArray(reinterpret_cast<const char*>(Output.mBuffer), Output.GetLength()), Data.left(Limit));
}

#if !LC_DISABLE_BLENDER
void lcModelInstanceTests::AddonExtraction_data()
{
	QTest::addColumn<QByteArray>("Name");
	QTest::addColumn<quint32>("Mode");
	QTest::addColumn<int>("ExistingLink");
	QTest::addColumn<int>("ExtraBytes");
	QTest::addColumn<bool>("BadCrc");
	QTest::addColumn<bool>("Accepted");
	QTest::addColumn<QByteArray>("Data");
	QTest::newRow("normal") << QByteArray("addons/test.py") << quint32(0100644) << 0 << 0 << false << true << QByteArray("payload");
	QTest::newRow("empty-file") << QByteArray("addons/empty.py") << quint32(0100644) << 0 << 0 << false << true << QByteArray();
	QTest::newRow("directory") << QByteArray("addons/") << quint32(0040755) << 0 << 0 << false << true << QByteArray();
	for (const QByteArray& Name : { QByteArray("../victim"), QByteArray("addons/../../victim"),
		QByteArray("addons\\..\\..\\victim"), QByteArray("/victim"), QByteArray("C:/victim"),
		QByteArray("\\\\server\\share"), QByteArray("addons/file:stream"), QByteArray("addons/.. /victim"),
		QByteArray("addons./victim"), QByteArray("./victim"), QByteArray(), QByteArray(256, 'A'), QByteArray("victim\0suffix", 13) })
		QTest::newRow(qPrintable(QString::fromLatin1(Name.toHex()))) << Name << quint32(0100644) << 0 << 0 << false << false << QByteArray("payload");
	QTest::newRow("archive-symlink") << QByteArray("addons/link") << quint32(0120777) << 0 << 0 << false << false << QByteArray("payload");
	QTest::newRow("bad-crc") << QByteArray("addons/test.py") << quint32(0100644) << 0 << 0 << true << false << QByteArray("payload");
	QTest::newRow("truncated") << QByteArray("addons/test.py") << quint32(0100644) << 0 << 10 << false << false << QByteArray("payload");
#ifdef Q_OS_UNIX
	QTest::newRow("parent-symlink") << QByteArray("addons/victim") << quint32(0100644) << 1 << 0 << false << false << QByteArray("payload");
	QTest::newRow("leaf-symlink") << QByteArray("victim") << quint32(0100644) << 2 << 0 << false << false << QByteArray("payload");
#endif
}

void lcModelInstanceTests::AddonExtraction()
{
	QFETCH(QByteArray, Name);
	QFETCH(quint32, Mode);
	QFETCH(int, ExistingLink);
	QFETCH(int, ExtraBytes);
	QFETCH(bool, BadCrc);
	QFETCH(bool, Accepted);
	QFETCH(QByteArray, Data);
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkdir("extract"));
	const QString Root = Directory.filePath("extract");
	QFile Victim(Directory.filePath("victim"));
	QVERIFY(Victim.open(QIODevice::WriteOnly));
	Victim.write("original");
	Victim.close();
	if (ExistingLink == 1)
		QVERIFY(QFile::link(Directory.path(), QDir(Root).filePath("addons")));
	else if (ExistingLink == 2)
		QVERIFY(QFile::link(Victim.fileName(), QDir(Root).filePath("victim")));

	const quint32 DeclaredSize = ExtraBytes > 0 ? Data.size() + ExtraBytes : Data.size();
	const QByteArray Zip = CreateAddonArchive(Name, Data, Mode, DeclaredSize, BadCrc, false);
	QFile Archive(QDir(Root).filePath("addon.zip"));
	QVERIFY(Archive.open(QIODevice::WriteOnly));
	QCOMPARE(Archive.write(Zip), qint64(Zip.size()));
	Archive.close();

	class TestPreferences : public lcBlenderPreferences
	{
	public:
		using lcBlenderPreferences::ExtractAddon;
	};
	QString Result;
	QCOMPARE(TestPreferences::ExtractAddon(Archive.fileName(), Result), Accepted);
	QVERIFY(Victim.open(QIODevice::ReadOnly));
	QCOMPARE(Victim.readAll(), QByteArray("original"));
	if (Accepted)
	{
		const QString OutputPath = QDir(Root).filePath(QString::fromUtf8(Name));
		if ((Mode & 0170000) == 0040000)
			QVERIFY(QFileInfo(OutputPath).isDir());
		else
		{
			QFile Output(OutputPath);
			QVERIFY(Output.open(QIODevice::ReadOnly));
			QCOMPARE(Output.readAll(), Data);
		}
	}
	else
	{
		QVERIFY(!Result.isEmpty());
		QVERIFY(!QFileInfo(QDir(Root).filePath("addons/test.py")).exists());
	}
}
#endif

void lcModelInstanceTests::LgeoLongColorTokens()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile Elements(Directory.filePath("lg_elements.lst"));
	QVERIFY(Elements.open(QIODevice::WriteOnly));
	Elements.close();
	QFile Colors(Directory.filePath("lg_colors.lst"));
	QVERIFY(Colors.open(QIODevice::WriteOnly));
	Colors.write("7 safe_color L\n");
	Colors.write("7 " + QByteArray(1800, 'A') + " L\n");
	Colors.write("7 color " + QByteArray(1800, 'B') + "\n");
	Colors.write(QByteArray(1800, '9') + " color L\n");
	Colors.write("4294967303 wrapped_code L\n");
	Colors.write("-4294967289 wrapped_code L\n");
	Colors.write("7x invalid_code L\n");
	Colors.close();
	QFile ModelFile(Directory.filePath("model.ldr"));
	QVERIFY(ModelFile.open(QIODevice::WriteOnly));
	ModelFile.write("0 LGEO test\n0 !LEOCAD POV_RAY USE_LGEO\n");
	ModelFile.close();

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		QString PreviousLgeoPath = lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH);
		Project Current;
		ScopedProject() { gApplication->mProject = &Current; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
			lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, PreviousLgeoPath);
		}
	} Scope;
	lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Directory.path());
	lcLoadDefaultColors(lcStudStyle::Plain);
	QVERIFY(Scope.Current.Load(ModelFile.fileName()));
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Scope.Current.GetMainModel(), false);
	QVERIFY(Scope.Current.ExportPOVRay(Directory.filePath("model.pov")));
	QFile Output(Directory.filePath("model.pov"));
	QVERIFY(Output.open(QIODevice::ReadOnly));
	QVERIFY(Output.readAll().contains("safe_color"));
}

void lcModelInstanceTests::SaveDialogSuffix()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	const bool DontUseNativeDialogs = QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
	QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);

	const QString Filter = QStringLiteral("Images (*.bmp *.png *.jpg);;JPEG (*.jpg);;All Files (*)");
	for (int Case = 0; Case < 4; Case++)
	{
		QTimer::singleShot(0, [&, Case]()
		{
			QFileDialog* Dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
			QVERIFY(Dialog);

			if (Case == 2)
			{
				Dialog->selectNameFilter(QStringLiteral("JPEG (*.jpg)"));
				QMetaObject::invokeMethod(Dialog, "filterSelected", Q_ARG(QString, QStringLiteral("JPEG (*.jpg)")));
			}

			Dialog->selectFile(Directory.filePath(Case == 1 ? QStringLiteral("model.custom") : QStringLiteral("model")));
			QMetaObject::invokeMethod(Dialog, Case == 3 ? "reject" : "accept");
		});

		const QString Result = lcGetSaveFileName(nullptr, QStringLiteral("Save"), Directory.path(), Filter, QStringLiteral("png"));
		QString Expected;
		if (Case == 0)
			Expected = Directory.filePath(QStringLiteral("model.png"));
		else if (Case == 1)
			Expected = Directory.filePath(QStringLiteral("model.custom"));
		else if (Case == 2)
			Expected = Directory.filePath(QStringLiteral("model.jpg"));
		QCOMPARE(Result, Expected);
	}

	QFile Existing(Directory.filePath(QStringLiteral("existing.obj")));
	QVERIFY(Existing.open(QIODevice::WriteOnly));
	Existing.close();
	bool OverwritePrompted = false;
	QTimer::singleShot(0, [&]()
	{
		QFileDialog* Dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
		QVERIFY(Dialog);
		Dialog->selectFile(Directory.filePath(QStringLiteral("existing")));
		QTimer::singleShot(0, [&]()
		{
			QMessageBox* Prompt = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
			OverwritePrompted = Prompt != nullptr;
			if (Prompt)
				Prompt->done(QMessageBox::No);
			Dialog->reject();
		});
		QMetaObject::invokeMethod(Dialog, "accept");
	});
	QVERIFY(lcGetSaveFileName(nullptr, QStringLiteral("Save"), Directory.path(), QStringLiteral("Wavefront (*.obj);;All Files (*)"), QStringLiteral("obj")).isEmpty());
	QVERIFY(OverwritePrompted);

	QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, DontUseNativeDialogs);
}

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

void lcModelInstanceTests::AssetLoaderIndependentBuilds()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkpath("parts"));
	QVERIFY(QDir(Directory.path()).mkpath("p"));

	for (int Index = 0; Index < 200; Index++)
	{
		QFile File(Directory.filePath(QString("parts/backlog%1.dat").arg(Index)));
		QVERIFY(File.open(QIODevice::WriteOnly));
		QVERIFY(File.write("0 Backlog test\n3 16 0 0 0 1 0 0 0 1 0\n") > 0);
	}

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	QVERIFY(Library->Load(Directory.path(), false, false));
	std::vector<lcPieceDataRef> Pieces;

	for (int Index = 0; Index < 200; Index++)
		Pieces.push_back(Library->AcquirePieceData(QString("backlog%1.dat").arg(Index).toLatin1().constData(), nullptr, lcAcquirePieceDataFlag::None));

	// Exercise the loader through its public scheduling and readiness interfaces.
	lcAssetLoader Loader(Library);
	qRegisterMetaType<lcPieceData*>("lcPieceData*");
	QSignalSpy Loaded(Library, &lcPiecesLibrary::PartLoaded);

	for (const lcPieceDataRef& Piece : Pieces)
	{
		QVERIFY(Piece);
		Piece->mState = lcAssetState::Loading;
		Loader.RequestPieceLoad(Piece.Get(), lcLoadPriority::Normal);
	}

	// Joining the pool does not pump UI events. Once workers finish, hide their sources:
	// readiness can succeed only if every queued build already produced its private result.
	QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
	QCOMPARE(Loaded.count(), 0);
	QDir Root(Directory.path());
	QVERIFY(Root.rename("parts", "built-parts"));

	// Pause consumes worker results without public callbacks.
	Loader.PauseQueuedWork();
	Loader.ResumeQueuedWork();
	std::vector<lcPieceData*> Required;
	for (const lcPieceDataRef& Piece : Pieces)
		Required.push_back(Piece.Get());
	QVERIFY(Loader.WaitForPieces(Required));
	QVERIFY(!Loader.HasPendingWork());
	QCOMPARE(Loaded.count(), 0); // Scoped waits defer public callbacks.
	for (const lcPieceDataRef& Piece : Pieces)
		QCOMPARE(Piece->mState, lcAssetState::Loaded);

	QVERIFY(Root.rename("built-parts", "parts"));
	QTRY_COMPARE(Loaded.count(), 200);

	// Teardown must also drain outstanding results and discard prepared/cancelled jobs.
	{
		lcAssetLoader CancelledLoader(Library);
		for (const lcPieceDataRef& Piece : Pieces)
		{
			Piece->mState = lcAssetState::Loading;
			CancelledLoader.RequestPieceLoad(Piece.Get(), lcLoadPriority::Normal);
		}
		CancelledLoader.InvalidatePieceLoad(Pieces.front().Get());
	}
	QCOMPARE(Loaded.count(), 200);
}

void lcModelInstanceTests::AssetLoaderCancelPreparedBuild()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkpath("parts"));
	QVERIFY(QDir(Directory.path()).mkpath("p"));
	QFile File(Directory.filePath("parts/cancel.dat"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Cancellation test\n3 16 0 0 0 1 0 0 0 1 0\n");
	File.close();
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	QVERIFY(Library->Load(Directory.path(), false, false));

	// Keep prepared jobs in the queue so releasing a consumer cannot race a build.
	struct BlockPool
	{
		QThreadPool* Pool = QThreadPool::globalInstance();
		int PreviousMaximum = Pool->maxThreadCount();
		QSemaphore Started;
		QSemaphore Release;
		QFuture<void> Future;
		BlockPool()
		{
			Pool->setMaxThreadCount(1);
			Future = QtConcurrent::run([this]() { Started.release(); Release.acquire(); });
			Started.acquire();
		}
		~BlockPool()
		{
			Release.release();
			Pool->waitForDone();
			Pool->setMaxThreadCount(PreviousMaximum);
		}
	} Blocker;

	lcPieceDataRef Piece = Library->AcquirePieceData("cancel.dat", nullptr, lcAcquirePieceDataFlag::None);
	QVERIFY(Piece);
	Library->RequestPieceDataLoad(Piece.Get(), lcLoadPriority::Normal);
	QCOMPARE(Piece->mState, lcAssetState::Loading);
	Piece.Reset();
	Piece = Library->AcquirePieceData("cancel.dat", nullptr, lcAcquirePieceDataFlag::None);
	QVERIFY(Piece);
	QCOMPARE(Piece->mState, lcAssetState::Unloaded);
}

void lcModelInstanceTests::AssetUpdateThrottle()
{
	class TestWindow : public lcMainWindow
	{
	public:
		using lcMainWindow::CreateStatusBar;
		using lcMainWindow::mAssetUpdateScheduled;
	};
	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		ScopedProject() { gApplication->mProject = nullptr; }
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
		}
	} Scope;
	TestWindow Window;
	Window.CreateStatusBar();
	QTemporaryDir Directory;
	QVERIFY(QDir(Directory.path()).mkpath("parts"));
	QVERIFY(QDir(Directory.path()).mkpath("p"));
	QFile File(Directory.filePath("parts/throttle.dat"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Throttle test\n3 16 0 0 0 1 0 0 0 1 0\n");
	File.close();
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	QVERIFY(Library->Load(Directory.path(), false, false));
	lcPieceDataRef Piece = Library->AcquirePieceData("throttle.dat", nullptr, lcAcquirePieceDataFlag::None);
	QVERIFY(Piece);
	Library->RequestPieceDataLoad(Piece.Get(), lcLoadPriority::Normal);
	QVERIFY(Window.mAssetUpdateScheduled);
	QSignalSpy Loaded(Library, &lcPiecesLibrary::PartLoaded);
	Window.ScheduleAssetUpdate();

	// Blocking readiness publishes the mesh without waiting for the UI timer or callbacks.
	QVERIFY(Library->WaitForPieceData(Piece));
	QCOMPARE(Piece->mState, lcAssetState::Loaded);
	QVERIFY(Piece->GetMesh());
	QCOMPARE(Loaded.count(), 0);
	QVERIFY(Window.mAssetUpdateScheduled);
	QCoreApplication::sendPostedEvents(nullptr, QEvent::User + 1);
	QCOMPARE(Loaded.count(), 1);
	QVERIFY(Window.mAssetUpdateScheduled); // Settlement uses the same pending update.
	QTRY_VERIFY(!Window.mAssetUpdateScheduled);

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

		QVERIFY(Document.Load(File.fileName()));

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
	QVERIFY(Document.Load(File.fileName()));
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
	QVERIFY(Reloaded.Load(File.fileName()));
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

	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd")));
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
	QVERIFY(Reloaded.Load(Directory.filePath("host.mpd")));
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

	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd")));
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
	QVERIFY(Scope.Current.Load(HostFile.fileName()));
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
	QVERIFY(Scope.Current.Load(Directory.filePath("host.mpd")));
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
	QVERIFY(Scope.Current.Load(File.fileName()));
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

void lcModelInstanceTests::WavefrontSaveFailures()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	lcLoadDefaultColors(lcStudStyle::Plain);
	Project Model;
	const QString OBJPath = Directory.filePath("model.obj");
	QFile OBJFile(OBJPath);
	QVERIFY(OBJFile.open(QIODevice::WriteOnly));
	QCOMPARE(OBJFile.write("existing OBJ"), qint64(12));
	OBJFile.close();

	const QString MaterialPath = Directory.filePath("model.mtl");
	QVERIFY(QDir().mkdir(MaterialPath));
	const lcResult<void> Result = Model.ExportWavefront(OBJPath);
	QVERIFY(!Result);
	QVERIFY(Result.error().contains(MaterialPath));
	QVERIFY(OBJFile.open(QIODevice::ReadOnly));
	QCOMPARE(OBJFile.readAll(), QByteArray("existing OBJ"));
	OBJFile.close();

	QVERIFY(QDir().rmdir(MaterialPath));
	QVERIFY(Model.ExportWavefront(OBJPath));
	QVERIFY(OBJFile.open(QIODevice::ReadOnly));
	QVERIFY(OBJFile.readAll().contains("mtllib model.mtl"));
	QFile MaterialFile(MaterialPath);
	QVERIFY(MaterialFile.open(QIODevice::ReadOnly));
	QVERIFY(MaterialFile.readAll().contains("newmtl"));
}

void lcModelInstanceTests::ProjectLoadErrors()
{
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	Project Model;
	const QString MissingPath = Directory.filePath("missing.ldr");
	const lcResult<void> Missing = Model.Load(MissingPath);
	QVERIFY(!Missing);
	QVERIFY(Missing.error().contains(MissingPath));
	QVERIFY(Missing.error().contains("Error reading"));

	const QString InvalidPath = Directory.filePath("invalid.lcd");
	QFile InvalidFile(InvalidPath);
	QVERIFY(InvalidFile.open(QIODevice::WriteOnly));
	QCOMPARE(InvalidFile.write("invalid binary file"), qint64(19));
	InvalidFile.close();
	const lcResult<void> Invalid = Model.Load(InvalidPath);
	QVERIFY(!Invalid);
	QVERIFY(Invalid.error().contains(InvalidPath));
	QVERIFY(Invalid.error().contains("older version of LeoCAD"));
}

void lcModelInstanceTests::ProjectLegacyFormats_data()
{
	QTest::addColumn<QString>("FileName");
	QTest::addColumn<QByteArray>("Data");
	QTest::addColumn<bool>("Supported");

	const QByteArray LDraw = "0 Test model\n3 16 0 0 0 10 0 0 0 10 0\n";
	const QByteArray Binary = QByteArray("LeoCAD 0.4") + QByteArray(256, '\0');
	for (const QString& Extension : { QString("ldr"), QString("dat"), QString("mpd"), QString("bin"), QString() })
	{
		const QString FileName = Extension.isEmpty() ? QString("model") : QString("model.") + Extension;
		QTest::newRow(qPrintable(FileName + "-text")) << FileName << LDraw << true;
		QTest::newRow(qPrintable(FileName + "-binary")) << FileName << Binary << false;
	}
	for (const QString& Extension : { QString("lcd"), QString("leocad"), QString("LCD"), QString("LEOCAD") })
	{
		const QString FileName = "model." + Extension;
		QTest::newRow(qPrintable(FileName + "-text")) << FileName << LDraw << false;
		QTest::newRow(qPrintable(FileName + "-binary")) << FileName << Binary << false;
		QTest::newRow(qPrintable(FileName + "-empty")) << FileName << QByteArray() << false;
	}
	QTest::newRow("truncated-magic") << QString("model.ldr") << QByteArray("LeoCAD ") << false;
}

void lcModelInstanceTests::ProjectLegacyFormats()
{
	QFETCH(QString, FileName);
	QFETCH(QByteArray, Data);
	QFETCH(bool, Supported);
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile InitialFile(Directory.filePath("initial.ldr"));
	QVERIFY(InitialFile.open(QIODevice::WriteOnly));
	InitialFile.write("0 Original model\n");
	InitialFile.close();
	Project Model;
	QVERIFY(Model.Load(InitialFile.fileName()));
	lcModel* InitialModel = Model.GetMainModel();

	QFile File(Directory.filePath(FileName));
	QVERIFY(File.open(QIODevice::WriteOnly));
	QCOMPARE(File.write(Data), qint64(Data.size()));
	File.close();
	const lcResult<void> Result = Model.Load(File.fileName());
	QCOMPARE(Result.has_value(), Supported);
	if (!Supported)
	{
		QVERIFY(Result.error().contains(File.fileName()));
		QVERIFY(Result.error().contains("older version of LeoCAD"));
		QVERIFY(Result.error().contains("LDraw format (.ldr or .mpd)"));
		QCOMPARE(Model.GetMainModel(), InitialModel);
		QCOMPARE(Model.GetFileName(), InitialFile.fileName());
	}
	else
		QCOMPARE(Model.GetModels().size(), size_t(1));
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
	QVERIFY(Host.Load(MainFile.fileName()));
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

	const QString ColladaName = Directory.filePath("completed.dae");
	QFile ColladaFile(ColladaName);
	QVERIFY(ColladaFile.open(QIODevice::WriteOnly));
	QCOMPARE(ColladaFile.write("old contents"), qint64(12));
	ColladaFile.close();
	QVERIFY(Host.ExportCOLLADA(ColladaName));
	QVERIFY(ColladaFile.open(QIODevice::ReadOnly));
	QVERIFY(ColladaFile.readAll().contains("</COLLADA>"));
	const lcResult<void> ColladaError = Host.ExportCOLLADA(Directory.filePath("missing/completed.dae"));
	QVERIFY(!ColladaError);
	QVERIFY(ColladaError.error().contains("completed.dae"));

	const QString CSVName = Directory.filePath("completed.csv");
	QFile CSVFile(CSVName);
	QVERIFY(CSVFile.open(QIODevice::WriteOnly));
	QCOMPARE(CSVFile.write("old contents"), qint64(12));
	CSVFile.close();
	QVERIFY(Host.ExportCSV(CSVName));
	QVERIFY(CSVFile.open(QIODevice::ReadOnly));
	QVERIFY(CSVFile.readAll().startsWith("Part Name,Color,Quantity"));
	QVERIFY(!Host.ExportCSV(Directory.filePath("missing/completed.csv")));
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
	QVERIFY(Host.Load(MainFile.fileName()));
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

void lcModelInstanceTests::TrainTrackConnections_data()
{
	QTest::addColumn<int>("FirstSleeper");
	QTest::addColumn<int>("SecondSleeper");
	QTest::addColumn<bool>("SameGroup");
	QTest::addColumn<float>("Roll");
	QTest::addColumn<float>("Offset");
	QTest::addColumn<bool>("Connected");

	for (int First = 0; First < 3; First++)
		for (int Second = 0; Second < 3; Second++)
			for (bool SameGroup : { false, true })
			{
				const bool Compatible = (First == 0 && Second == 0) || (First == 1 && Second == 2) || (First == 2 && Second == 1);
				QTest::newRow(qPrintable(QString("sleepers-%1-%2-group-%3").arg(First).arg(Second).arg(SameGroup)))
					<< First << Second << SameGroup << 0.0f << 0.0f << (SameGroup && Compatible);
			}

	QTest::newRow("upside-down") << 0 << 0 << true << 180.0f << 0.0f << false;
	QTest::newRow("rolled") << 0 << 0 << true << 90.0f << 0.0f << false;
	QTest::newRow("position-tolerance") << 0 << 0 << true << 0.0f << 0.1f << true;
	QTest::newRow("separated") << 0 << 0 << true << 0.0f << 0.4f << false;
}

void lcModelInstanceTests::TrainTrackConnections()
{
	QFETCH(int, FirstSleeper);
	QFETCH(int, SecondSleeper);
	QFETCH(bool, SameGroup);
	QFETCH(float, Roll);
	QFETCH(float, Offset);
	QFETCH(bool, Connected);

	Project Document(true);
	lcModel* Model = Document.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 track-first.dat\n"
		"1 16 0 0 0 1 0 0 0 1 0 0 0 1 track-second.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Document);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* First = Model->GetPieces()[0].get();
	lcPiece* Second = Model->GetPieces()[1].get();
	First->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44Identity(), { 1, static_cast<lcTrainTrackConnectionSleeper>(FirstSleeper) } } }, 16, true));
	Second->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44Identity(), { SameGroup ? size_t(1) : size_t(2), static_cast<lcTrainTrackConnectionSleeper>(SecondSleeper) } } }, 16, true));

	// Test both ordinary placement and tracks tilted/translated together.
	for (const lcMatrix44& Parent : { lcMatrix44Identity(), lcMatrix44(lcMatrix33RotationY(0.7f), lcVector3(100, -50, 20)) })
	{
		First->Initialize(Parent, 1);
		lcMatrix44 Transform = lcMul(lcMatrix44RotationX(Roll * LC_DTOR), lcMatrix44RotationZ(LC_PI));
		Transform.SetTranslation(lcVector3(Offset, 0, 0));
		Second->Initialize(lcMul(Transform, Parent), 1);
		QCOMPARE(lcTrainTrackInfo::GetPieceConnectionIndex(First, 0, Second), Connected ? 0 : -1);
		QCOMPARE(lcTrainTrackInfo::GetPieceConnectionIndex(Second, 0, First), Connected ? 0 : -1);
	}
}

void lcModelInstanceTests::TrainTrackFocusBeforeDraw()
{
	lcPiece Uninitialized;
	QVERIFY(!Uninitialized.IsTrainTrackConnected(0));
	QVERIFY(!Uninitialized.IsTrainTrackConnected(-1));
	QVERIFY(!Uninitialized.IsTrainTrackConnected(100));

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
	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 focus-track.dat\n"
		"1 16 320 0 0 1 0 0 0 1 0 0 0 1 focus-track.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Piece = Model->GetPieces()[0].get();
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	Mesh->mBoundingBox = { lcVector3(-160, -40, 0), lcVector3(160, 40, 4) };
	lcVertex* Vertices = static_cast<lcVertex*>(Mesh->mVertexData);
	for (int Index = 0; Index < Mesh->mNumVertices; Index++)
	{
		Vertices[Index].Position.x *= 16;
		Vertices[Index].Position.y *= 4;
		Vertices[Index].Position.z = lcMax(0.0f, Vertices[Index].Position.z);
	}
	Piece->mPieceData->SetMesh(Mesh);
	Piece->mPieceData->mState = lcAssetState::Loaded;
	Piece->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } }
	}, 16, true));
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	lcView* View = Window.GetActiveView();
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(0, 0, 1000), lcVector3(0, 0, 0), lcVector3(0, 1, 0));
	View->SetCamera(&Camera, true);
	Window.SetTool(lcTool::Select);
	const lcVector3 Screen = View->ProjectPoint(lcVector3(140, 30, 4));
	View->SetMousePosition(qRound(Screen.x), qRound(Screen.y));
	View->OnMouseMove();
	QCOMPARE(View->FindObjectUnderPointer(true, false).Object, static_cast<lcObject*>(Piece));
	View->OnLeftButtonDown();
	QCOMPARE(Model->GetFocusObject(), static_cast<lcObject*>(Piece));
	// No scene has been drawn: releasing the click must compute occupancy before overlay picking.
	View->OnLeftButtonUp();
	QVERIFY(!View->IsTracking());
	QVERIFY(Piece->IsTrainTrackConnected(0));
	QVERIFY(!Piece->IsTrainTrackConnected(1));
	QVERIFY(!Piece->IsTrainTrackConnected(100));

	// A subsequent geometry change must also be reflected without waiting for a redraw.
	Model->GetPieces()[1]->SetHidden(true);
	View->OnMouseMove();
	QVERIFY(!Piece->IsTrainTrackConnected(0));
	QVERIFY(!Piece->IsTrainTrackConnected(1));

	// Keyboard insertion on a focused track also needs current occupancy without a preceding draw.
	Model->GetPieces()[1]->SetHidden(false);
	Piece->SetTrainTrackConnections({});
	lcPiece* Inserted = Model->AddPiece(Piece->mPieceData.Get(), LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST);
	QVERIFY(Inserted);
	QVERIFY(lcLength(Inserted->mModelWorld.GetTranslation() - lcVector3(-320, 0, 0)) < 0.001f);
}

void lcModelInstanceTests::TrainTrackConnectionVisibility()
{
	Project Document(true);
	lcModel* Model = Document.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 track-visibility.dat\n"
		"1 16 320 0 0 1 0 0 0 1 0 0 0 1 track-visibility.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Document);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* First = Model->GetPieces()[0].get();
	lcPiece* Second = Model->GetPieces()[1].get();
	First->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } }
	}, 16, true));

	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(First->IsTrainTrackConnected(0));
	QVERIFY(!First->IsTrainTrackConnected(1));
	Second->SetHidden(true);
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(!First->IsTrainTrackConnected(0));
	Second->SetHidden(false);
	Second->SetStepShow(2);
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(!First->IsTrainTrackConnected(0));
	Model->SetTemporaryStep(2);
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(First->IsTrainTrackConnected(0));
	Second->SetStepHide(3);
	Model->SetTemporaryStep(3);
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(!First->IsTrainTrackConnected(0));
	Model->SetTemporaryStep(2);
	Second->SetSelected(true);
	Model->UpdateTrainTrackConnections(First, true);
	QVERIFY(!First->IsTrainTrackConnected(0));
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(First->IsTrainTrackConnected(0));
}

void lcModelInstanceTests::TrainTrackInsertionSection_data()
{
	QTest::addColumn<bool>("ExplicitSection");
	QTest::newRow("clicked-endpoint") << true;
	QTest::newRow("focused-endpoint") << false;
}

void lcModelInstanceTests::TrainTrackInsertionSection()
{
	QFETCH(bool, ExplicitSection);
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

	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 track-insertion.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	lcPiece* First = Model->GetPieces()[0].get();
	First->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } }
	}, 16, true));

	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	Model->SetSelectionAndFocusAction({}, First, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST, lcSelectionMode::Single);
	Model->UpdateTrainTrackConnections(First, false);
	lcPiece* Inserted = Model->AddPiece(First->mPieceData.Get(), ExplicitSection ? LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST + 1 : LC_PIECE_SECTION_INVALID);
	QVERIFY(Inserted);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	const lcVector3 Expected(ExplicitSection ? -320.0f : 320.0f, 0, 0);
	QVERIFY(lcLength(Inserted->mModelWorld.GetTranslation() - Expected) < 0.001f);
	Model->UpdateTrainTrackConnections(First, false);
	QVERIFY(First->IsTrainTrackConnected(ExplicitSection ? 1 : 0));
}

void lcModelInstanceTests::TrainTrackDragSnap_data()
{
	QTest::addColumn<QString>("Scenario");
	QTest::addColumn<bool>("Snaps");
	for (const QString& Scenario : { QString("right"), QString("left"), QString("nearest"), QString("rotated"), QString("sleeper") })
		QTest::newRow(qPrintable(Scenario)) << Scenario << true;
	for (const QString& Scenario : { QString("distant"), QString("hidden"), QString("future"), QString("incompatible"),
		QString("occupied-target"), QString("occupied-moving"), QString("selected-target"), QString("needs-sleeper"), QString("zero-radius") })
		QTest::newRow(qPrintable(Scenario)) << Scenario << false;
}

void lcModelInstanceTests::TrainTrackDragSnap()
{
	QFETCH(QString, Scenario);
	QFETCH(bool, Snaps);
	Project Document(true);
	lcModel* Model = Document.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 snap-target.dat\n"
		"1 16 -1000 0 0 1 0 0 0 1 0 0 0 1 snap-moving.dat\n"
		"1 16 10000 0 0 1 0 0 0 1 0 0 0 1 snap-other.dat\n"
		"1 16 10000 0 0 1 0 0 0 1 0 0 0 1 snap-blocker.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Document);
	QCOMPARE(Model->GetPieces().size(), size_t(4));
	lcPiece* Target = Model->GetPieces()[0].get();
	lcPiece* Moving = Model->GetPieces()[1].get();
	lcPiece* Other = Model->GetPieces()[2].get();
	lcPiece* Blocker = Model->GetPieces()[3].get();
	const lcTrainTrackConnectionSleeper TargetSleeper = Scenario == "sleeper" ? lcTrainTrackConnectionSleeper::HasSleeper : lcTrainTrackConnectionSleeper::None;
	lcTrainTrackConnectionSleeper MovingSleeper = lcTrainTrackConnectionSleeper::None;
	if (Scenario == "sleeper" || Scenario == "needs-sleeper")
		MovingSleeper = lcTrainTrackConnectionSleeper::NeedsSleeper;

	for (lcPiece* Piece : { Target, Moving, Other })
	{
		const size_t Group = Piece == Moving && Scenario == "incompatible" ? 2 : 1;
		lcTrainTrackConnectionSleeper Sleeper = Piece == Moving ? MovingSleeper : TargetSleeper;
		if (Scenario == "needs-sleeper")
			Sleeper = lcTrainTrackConnectionSleeper::NeedsSleeper;
		Piece->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
			{ lcMatrix44Translation(lcVector3(160, 0, 0)), { Group, Sleeper } },
			{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { Group, Sleeper } }
		}, 16, true));
	}
	Blocker->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44RotationZ(LC_PI), { 1, lcTrainTrackConnectionSleeper::None } }
	}, 16, true));
	Moving->SetSelected(true);
	Target->SetHidden(Scenario == "hidden");
	Target->SetSelected(Scenario == "selected-target");
	Blocker->SetSelected(Scenario == "occupied-moving");

	for (const lcMatrix44& Parent : { lcMatrix44Identity(), lcMatrix44(lcMatrix33RotationY(0.7f), lcVector3(100, -50, 20)) })
	{
		Target->Initialize(Parent, Scenario == "future" ? 2 : 1);
		Moving->Initialize(lcMul(lcMatrix44Translation(lcVector3(-1000, 0, 0)), Parent), 1);
		Other->Initialize(lcMul(lcMatrix44Translation(lcVector3(Scenario == "nearest" ? 630 : 10000, 0, 0)), Parent), 1);
		lcVector3 BlockerPosition(10000, 0, 0);
		if (Scenario == "occupied-target")
			BlockerPosition.x = 160;
		else if (Scenario == "occupied-moving")
			BlockerPosition.x = -840;
		Blocker->Initialize(lcMul(lcMatrix44Translation(BlockerPosition), Parent), 1);

		lcVector3 ProposedPosition(310, 0, 0);
		float ExpectedX = 320;
		if (Scenario == "left" || Scenario == "occupied-moving")
		{
			ProposedPosition.x = -310;
			ExpectedX = -320;
		}
		else if (Scenario == "distant")
			ProposedPosition.x = 400;
		else if (Scenario == "nearest")
			ExpectedX = 310;
		lcMatrix33 ProposedRotation = lcMatrix33Identity();
		if (Scenario == "rotated")
		{
			ProposedPosition = lcVector3(160, 150, 0);
			ProposedRotation = lcMatrix33RotationZ(LC_PI / 2.0f);
		}

		const lcMatrix44 Before = Moving->mModelWorld;
		const lcMatrix44 Proposed = lcMul(lcMatrix44(ProposedRotation, ProposedPosition), Parent);
		const lcTrainTrackSnapContext SnapContext
		{
			lcMul(lcMatrix44AffineInverse(Parent), lcMul(lcMatrix44Translation(lcVector3(0, 0, -2000)), lcMatrix44Ortho(-1000, 1000, -1000, 1000, 1, 4000))),
			lcVector2(2000, 2000),
			lcMul30(lcVector3(0, 0, -1), Parent)
		};
		const std::vector<lcInsertPieceInfo> Snap = Model->GetTrainTrackSnapInfo(Moving, Proposed, SnapContext, Scenario == "zero-radius" ? 0.0f : 20.0f);
		QCOMPARE(!Snap.empty(), Snaps);
		QVERIFY(Moving->mModelWorld == Before);
		if (!Snap.empty())
		{
			QVERIFY(lcLength(Snap.back().Transform.GetTranslation() - lcMul31(lcVector3(ExpectedX, 0, 0), Parent)) < 0.001f);
			Moving->Initialize(Snap.back().Transform, 1);
			const int MovingConnection = Scenario == "left" || Scenario == "nearest" ? 0 : 1;
			lcPiece* ConnectedTarget = Scenario == "nearest" ? Other : Target;
			QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Moving, MovingConnection, ConnectedTarget) != -1);
		}
	}
}

void lcModelInstanceTests::TrainTrackScreenSnap_data()
{
	QTest::addColumn<bool>("Orthographic");
	QTest::addColumn<float>("CameraDistance");
	QTest::addColumn<int>("DeviceScale");
	QTest::addColumn<int>("PixelGap");
	for (bool Orthographic : { false, true })
		for (float CameraDistance : { 500.0f, 5000.0f, 50000.0f })
			for (int DeviceScale : { 1, 2 })
				for (int PixelGap : { 8, 16 })
					QTest::newRow(qPrintable(QString("ortho-%1-distance-%2-scale-%3-gap-%4").arg(Orthographic).arg(CameraDistance).arg(DeviceScale).arg(PixelGap)))
						<< Orthographic << CameraDistance << DeviceScale << PixelGap;
}

void lcModelInstanceTests::TrainTrackScreenSnap()
{
	QFETCH(bool, Orthographic);
	QFETCH(float, CameraDistance);
	QFETCH(int, DeviceScale);
	QFETCH(int, PixelGap);
	Project Document(true);
	lcModel* Model = Document.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 pixel-target.dat\n"
		"1 16 10000 0 0 1 0 0 0 1 0 0 0 1 pixel-moving.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Document);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Target = Model->GetPieces()[0].get();
	lcPiece* Moving = Model->GetPieces()[1].get();
	Target->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44Identity(), { 1, lcTrainTrackConnectionSleeper::None } } }, 16, true));
	Moving->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44RotationZ(LC_PI), { 1, lcTrainTrackConnectionSleeper::None } } }, 16, true));
	Moving->SetSelected(true);
	const lcMatrix44 ModelToView = lcMatrix44Translation(lcVector3(0, 0, -CameraDistance));
	lcMatrix44 Projection;
	if (Orthographic)
	{
		const float HalfHeight = CameraDistance * tanf(15.0f * LC_DTOR);
		Projection = lcMatrix44Ortho(-2 * HalfHeight, 2 * HalfHeight, -HalfHeight, HalfHeight, 1, 100000);
	}
	else
		Projection = lcMatrix44Perspective(30, 2, 1, 100000);
	const int PhysicalViewport[4] = { 0, 0, 1200 * DeviceScale, 600 * DeviceScale };
	lcVector3 ScreenPoint = lcProjectPoint(lcVector3(0, 0, 0), ModelToView, Projection, PhysicalViewport);
	ScreenPoint.x += PixelGap * DeviceScale;
	const lcVector3 Position = lcUnprojectPoint(ScreenPoint, ModelToView, Projection, PhysicalViewport);
	// Keep exact coplanarity: perspective depth-buffer roundoff is unrelated to the pixel metric.
	const lcMatrix44 Proposed = lcMatrix44Translation(lcVector3(Position.x, Position.y, 0));
	const lcTrainTrackSnapContext SnapContext
	{
		lcMul(ModelToView, Projection),
		lcVector2(PhysicalViewport[2] / DeviceScale, PhysicalViewport[3] / DeviceScale),
		lcVector3(0, 0, -1)
	};
	const std::vector<lcInsertPieceInfo> Snap = Model->GetTrainTrackSnapInfo(Moving, Proposed, SnapContext, 12);
	QCOMPARE(!Snap.empty(), PixelGap < 12);
	if (!Snap.empty())
	{
		QVERIFY(lcLength(Snap.back().Transform.GetTranslation()) < 0.001f);
		Moving->Initialize(Snap.back().Transform, 1);
		QCOMPARE(lcTrainTrackInfo::GetPieceConnectionIndex(Moving, 0, Target), 0);
	}
}

void lcModelInstanceTests::TrainTrackSnapDepth_data()
{
	QTest::addColumn<QString>("Scenario");
	for (const QString& Scenario : { QString("coplanar"), QString("different-height"), QString("different-plane"), QString("upside-down"),
		QString("behind-camera"), QString("near-clipped"), QString("far-clipped"), QString("edge-on-distant"), QString("edge-on-near") })
		QTest::newRow(qPrintable(Scenario)) << Scenario;
}

void lcModelInstanceTests::TrainTrackSnapDepth()
{
	QFETCH(QString, Scenario);
	Project Document(true);
	lcModel* Model = Document.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 depth-target.dat\n"
		"1 16 10000 0 0 1 0 0 0 1 0 0 0 1 depth-moving.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Document);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Target = Model->GetPieces()[0].get();
	lcPiece* Moving = Model->GetPieces()[1].get();
	Target->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44Identity(), { 1, lcTrainTrackConnectionSleeper::None } } }, 16, true));
	Moving->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({ { lcMatrix44RotationZ(LC_PI), { 1, lcTrainTrackConnectionSleeper::None } } }, 16, true));
	Moving->SetSelected(true);
	lcMatrix44 Proposed = lcMatrix44Identity();
	lcMatrix44 ModelToView = lcMatrix44Translation(lcVector3(0, 0, -1000));
	lcVector3 Direction(0, 0, -1);
	if (Scenario == "different-height")
		Proposed = lcMatrix44Translation(lcVector3(0, 0, 5));
	else if (Scenario == "different-plane")
		Proposed = lcMatrix44RotationX(LC_PI / 2);
	else if (Scenario == "upside-down")
		Proposed = lcMatrix44RotationX(LC_PI);
	else if (Scenario == "behind-camera" || Scenario == "near-clipped" || Scenario == "far-clipped")
	{
		float Height = 2000;
		if (Scenario == "near-clipped")
			Height = 999.5f;
		else if (Scenario == "far-clipped")
			Height = -10000;
		Target->Initialize(lcMatrix44Translation(lcVector3(0, 0, Height)), 1);
		Proposed = Target->mModelWorld;
	}
	else if (Scenario.startsWith("edge-on"))
	{
		ModelToView = lcMatrix44LookAt(lcVector3(0, -1000, 0), lcVector3(0, 0, 0), lcVector3(0, 0, 1));
		Direction = lcVector3(0, 1, 0);
		Proposed = lcMatrix44Translation(lcVector3(0, Scenario == "edge-on-near" ? 10 : 500, 0));
	}
	const lcTrainTrackSnapContext SnapContext
	{
		lcMul(ModelToView, lcMatrix44Perspective(30, 2, 1, 4000)),
		lcVector2(1200, 600), Direction
	};
	const std::vector<lcInsertPieceInfo> Snap = Model->GetTrainTrackSnapInfo(Moving, Proposed, SnapContext, 12);
	QCOMPARE(!Snap.empty(), Scenario == "coplanar" || Scenario == "edge-on-near");
}

void lcModelInstanceTests::TrainTrackDragMovement()
{
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
	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 drag-moving.dat\n"
		"1 16 -320 0 0 1 0 0 0 1 0 0 0 1 drag-neighbour.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Moving = Model->GetPieces()[0].get();
	lcPiece* Neighbour = Model->GetPieces()[1].get();
	for (lcPiece* Piece : { Moving, Neighbour })
		Piece->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
			{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } },
			{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } }
		}, 16, true));

	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	Model->SetSelectionAndFocusAction({ Neighbour }, Moving, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	const lcMatrix44 Start = Moving->mModelWorld;
	const lcVector3 Offset = Neighbour->mModelWorld.GetTranslation() - Start.GetTranslation();
	Model->BeginMouseTool(lcTool::Move, Window.GetActiveView());

	// Same-orientation snapping must retain fractional coordinates instead of rounding to the move grid.
	const lcMatrix44 Snapped = lcMatrix44Translation(lcVector3(313.25f, 27.5f, 0));
	Model->UpdateFreeMoveTool(Moving, Start, Snapped, true, false);
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - Snapped.GetTranslation()) < 0.001f);
	QVERIFY(lcLength(Neighbour->mModelWorld.GetTranslation() - Snapped.GetTranslation() - Offset) < 0.001f);
	const lcMatrix44 Released = lcMatrix44Translation(lcVector3(400, 40, 0));
	Model->UpdateFreeMoveTool(Moving, Start, Released, false, false);
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - Released.GetTranslation()) < 0.001f);
	QVERIFY(lcLength(Neighbour->mModelWorld.GetTranslation() - Released.GetTranslation() - Offset) < 0.001f);

	// Rotating onto a connection moves the selected group as one rigid assembly.
	const lcMatrix44 Rotated(lcMatrix33RotationZ(LC_PI / 2.0f), lcVector3(300, 100, 0));
	Model->UpdateFreeMoveTool(Moving, Start, Rotated, true, false);
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - Rotated.GetTranslation()) < 0.001f);
	QVERIFY(lcLength(Neighbour->mModelWorld.GetTranslation() - lcMul31(Offset, Rotated)) < 0.001f);
	Model->UpdateFreeMoveTool(Moving, Start, Released, false, false);
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - Released.GetTranslation()) < 0.001f);
	QVERIFY(lcLength(Neighbour->mModelWorld.GetTranslation() - Released.GetTranslation() - Offset) < 0.001f);
	Model->EndMouseTool(lcTool::Move, Window.GetActiveView(), true);
}

void lcModelInstanceTests::TrainTrackDragView_data()
{
	QTest::addColumn<float>("GrabX");
	QTest::addColumn<bool>("NeedsSleeper");
	QTest::addColumn<bool>("ConnectionFocused");
	QTest::newRow("grab-right-end") << 140.0f << false << false;
	QTest::newRow("grab-left-end") << -140.0f << false << false;
	QTest::newRow("grab-right-end-with-sleeper") << 140.0f << true << false;
	QTest::newRow("grab-left-end-with-sleeper") << -140.0f << true << false;
	QTest::newRow("focused-connection") << -140.0f << false << true;
	QTest::newRow("focused-connection-with-sleeper") << -140.0f << true << true;
}

void lcModelInstanceTests::TrainTrackDragView()
{
	QFETCH(float, GrabX);
	QFETCH(bool, NeedsSleeper);
	QFETCH(bool, ConnectionFocused);
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
	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 3.25 -48 0 1 0 0 0 1 0 0 0 1 drag-view.dat\n"
		"1 16 -1000 -48 0 1 0 0 0 1 0 0 0 1 drag-view.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Target = Model->GetPieces()[0].get();
	lcPiece* Moving = Model->GetPieces()[1].get();
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	Mesh->mBoundingBox = { lcVector3(-160, -40, 0), lcVector3(160, 40, 4) };
	lcVertex* Vertices = static_cast<lcVertex*>(Mesh->mVertexData);
	for (int VertexIndex = 0; VertexIndex < Mesh->mNumVertices; VertexIndex++)
	{
		Vertices[VertexIndex].Position.x *= 16;
		Vertices[VertexIndex].Position.y *= 4;
		Vertices[VertexIndex].Position.z = lcMax(0.0f, Vertices[VertexIndex].Position.z);
	}
	Target->mPieceData->SetMesh(Mesh);
	Target->mPieceData->SetBoundingBox(Mesh->mBoundingBox.Min, Mesh->mBoundingBox.Max);
	Target->mPieceData->mState = lcAssetState::Loaded;
	const lcTrainTrackConnectionSleeper SleeperType = NeedsSleeper ? lcTrainTrackConnectionSleeper::NeedsSleeper : lcTrainTrackConnectionSleeper::None;
	Target->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, SleeperType } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, SleeperType } }
	}, 16, true));
	lcPieceDataRef SleeperData;
	lcModelInstanceTestSleepers Sleepers;
	if (NeedsSleeper)
	{
		SleeperData = lcGetPiecesLibrary()->AcquirePieceData("view-sleeper.dat", &Scope.Current, lcAcquirePieceDataFlag::CreateMissing);
		QVERIFY(SleeperData);
		lcMesh* SleeperMesh = new lcMesh;
		SleeperMesh->CreateBox();
		SleeperData->SetMesh(SleeperMesh);
		SleeperData->mState = lcAssetState::Loaded;
		SleeperData->SetTrainTrackInfo(new lcTrainTrackInfo({
			{ lcMatrix44(lcMatrix33RotationZ(LC_PI / 2), lcVector3(0, 0, 16)), { 1, lcTrainTrackConnectionSleeper::HasSleeper } },
			{ lcMatrix44(lcMatrix33RotationZ(-LC_PI / 2), lcVector3(0, 0, 16)), { 1, lcTrainTrackConnectionSleeper::HasSleeper } }
		}, 0, true));
		lcModelInstanceTestTrackInfo::mSleepers[1] = SleeperData.Get();
	}
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	lcView* View = Window.GetActiveView();
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(-400, 0, 2000), lcVector3(-400, 0, 0), lcVector3(0, 1, 0));
	View->SetCamera(&Camera, true);
	if (ConnectionFocused)
		Model->SetSelectionAndFocusAction({}, Moving, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST, lcSelectionMode::Single);
	else
		Model->SetSelectionAndFocusAction({ Moving }, nullptr, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	Window.SetTool(lcTool::Select);
	Window.SetMoveXYSnapIndex(9);
	const lcVector3 Start = View->ProjectPoint(lcVector3(-1000 + GrabX, 30, 52));
	View->SetMousePosition(qRound(Start.x), qRound(Start.y));
	View->OnMouseMove();
	View->OnLeftButtonDown();
	QVERIFY(View->IsTracking());
	QVERIFY(Moving->IsFocused(LC_PIECE_SECTION_POSITION));
	QVERIFY(View->ShouldDrawInsertPreview());
	View->OnMouseMove();
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - lcVector3(-1000, 0, 48)) < 0.001f);

	// The grab offset stays fixed; endpoint snapping precedes the coarse 160-unit move grid.
	const lcVector3 End = View->ProjectPoint(lcVector3(316 + GrabX, 30, 52));
	View->SetMousePosition(qRound(End.x), qRound(End.y));
	QVERIFY(!View->FindObjectUnderPointer(true, true).Object);
	View->OnMouseMove();
	QCOMPARE(Model->GetPreviewInsertPieceInfo().size(), NeedsSleeper ? size_t(1) : size_t(0));
	View->OnLeftButtonUp();
	QVERIFY(!View->IsTracking());
	QVERIFY(!View->ShouldDrawInsertPreview());
	QVERIFY(lcLength(Moving->mModelWorld.GetTranslation() - lcVector3(323.25f, 0, 48)) < 0.001f);
	QCOMPARE(Model->GetPieces().size(), NeedsSleeper ? size_t(3) : size_t(2));
	if (NeedsSleeper)
	{
		lcPiece* Sleeper = Model->GetPieces()[2].get();
		QCOMPARE(Sleeper->mPieceData, SleeperData);
		QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Moving, 1, Sleeper) != -1);
		QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Target, 0, Sleeper) != -1);
	}
	else
		QCOMPARE(lcTrainTrackInfo::GetPieceConnectionIndex(Moving, 1, Target), 0);
	Model->UndoAction();
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	QVERIFY(lcLength(Model->GetPieces()[1]->mModelWorld.GetTranslation() - lcVector3(-1000, 0, 48)) < 0.001f);
	Model->RedoAction();
	QCOMPARE(Model->GetPieces().size(), NeedsSleeper ? size_t(3) : size_t(2));
	QVERIFY(lcLength(Model->GetPieces()[1]->mModelWorld.GetTranslation() - lcVector3(323.25f, 0, 48)) < 0.001f);
}

void lcModelInstanceTests::TrainTrackSideViewDrag_data()
{
	QTest::addColumn<bool>("Orthographic");
	QTest::addColumn<float>("CameraDistance");
	QTest::newRow("perspective") << false << 2000.0f;
	QTest::newRow("perspective-close") << false << 400.0f;
	QTest::newRow("orthographic") << true << 2000.0f;
	QTest::newRow("orthographic-close") << true << 400.0f;
}

void lcModelInstanceTests::TrainTrackSideViewDrag()
{
	QFETCH(bool, Orthographic);
	QFETCH(float, CameraDistance);
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
	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 side-track.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	lcPiece* Moving = Model->GetPieces()[0].get();
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	Mesh->mBoundingBox = { lcVector3(-160, -40, 0), lcVector3(160, 40, 40) };
	lcVertex* Vertices = static_cast<lcVertex*>(Mesh->mVertexData);
	for (int Index = 0; Index < Mesh->mNumVertices; Index++)
	{
		Vertices[Index].Position.x *= 16;
		Vertices[Index].Position.y *= 4;
		Vertices[Index].Position.z = Vertices[Index].Position.z < 0 ? 0 : 40;
	}
	Moving->mPieceData->SetMesh(Mesh);
	Moving->mPieceData->SetBoundingBox(Mesh->mBoundingBox.Min, Mesh->mBoundingBox.Max);
	Moving->mPieceData->mState = lcAssetState::Loaded;
	Moving->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { 1, lcTrainTrackConnectionSleeper::None } }
	}, 16, true));
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	lcView* View = Window.GetActiveView();
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(0, -CameraDistance, 30), lcVector3(0, 0, 30), lcVector3(0, 0, 1));
	Camera.SetProjection(Orthographic ? lcCameraProjection::Orthographic : lcCameraProjection::Perspective);
	View->SetCamera(&Camera, true);
	Model->SetSelectionAndFocusAction({ Moving }, nullptr, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	Window.SetTool(lcTool::Select);
	Window.SetMoveXYSnapIndex(0);
	Window.SetMoveZSnapIndex(0);
	const lcVector3 Start = View->ProjectPoint(lcVector3(140, -40, 30));
	View->SetMousePosition(qRound(Start.x), qRound(Start.y));
	View->OnMouseMove();
	QCOMPARE(View->FindObjectUnderPointer(true, false).Object, static_cast<lcObject*>(Moving));
	const lcObjectRayTest GrabRay = View->RayTest(true, false);
	const lcVector3 GrabPosition = GrabRay.Start + lcNormalize(GrabRay.End - GrabRay.Start) * GrabRay.Distance;
	QVERIFY(!View->ShouldDrawInsertPreview());
	View->OnLeftButtonDown();
	QVERIFY(View->IsTracking());
	QVERIFY(View->ShouldDrawInsertPreview());

	// The initial ray is parallel to the track plane. Move above and then below it.
	for (const lcVector3& Point : { lcVector3(150, -40, 40), lcVector3(180, -40, 50), lcVector3(240, -40, 130), lcVector3(340, -40, -70) })
	{
		const lcVector3 Screen = View->ProjectPoint(Point);
		View->SetMousePosition(qRound(Screen.x), qRound(Screen.y));
		View->OnMouseMove();
		const lcVector3 Position = Moving->mModelWorld.GetTranslation();
		QVERIFY(Position.x > Point.x - 160);
		QVERIFY(fabsf(Position.y) < 0.001f);
		if (Point.z > 100)
			QVERIFY(Position.z > 80);
		else if (Point.z < 0)
			QVERIFY(Position.z < -80);

		const lcVector3 GrabbedScreen = View->ProjectPoint(lcMul31(GrabPosition, Moving->mModelWorld));
		const float GrabError = lcLength(lcVector3(GrabbedScreen.x - qRound(Screen.x), GrabbedScreen.y - qRound(Screen.y), 0));
		QVERIFY2(GrabError < 1.0f, qPrintable(QString("Grab point drifts %1 framebuffer pixels from the cursor").arg(GrabError)));
	}
	View->OnLeftButtonUp();
	QVERIFY(!View->IsTracking());
	QVERIFY(!View->ShouldDrawInsertPreview());

	// Deleting the hovered piece leaves the cached hover pointer stale until the next mouse move.
	View->OnMouseMove();
	QCOMPARE(View->FindObjectUnderPointer(true, false).Object, static_cast<lcObject*>(Moving));
	const lcPieceDataRef TrackData = Moving->mPieceData;
	Model->DeleteSelectedObjects();
	QVERIFY(Model->GetPieces().empty());
	QVERIFY(!View->ShouldDrawInsertPreview());

	// Exercise heap reuse while the view still caches the deleted piece's address.
	std::vector<std::unique_ptr<lcPiece>> Allocations;
	for (int Index = 0; Index < 64; Index++)
		Allocations.push_back(std::make_unique<lcPiece>());

	Model->UndoAction();
	QCOMPARE(Model->GetPieces().size(), size_t(1));
	QVERIFY(!View->ShouldDrawInsertPreview());
	lcPiece* Restored = Model->GetPieces()[0].get();
	QCOMPARE(Restored->mPieceData, TrackData);
	Model->SelectAllPiecesAction();
	const lcMatrix44 Before = Restored->mModelWorld;

	// Start another drag without any intervening mouse movement.
	View->OnLeftButtonDown();
	QVERIFY(View->IsTracking());
	QCOMPARE(Model->GetFocusObject(), static_cast<lcObject*>(Restored));
	View->OnMouseMove();
	QVERIFY(lcLength(Restored->mModelWorld.GetTranslation() - Before.GetTranslation()) < 0.001f);
	const lcVector3 BeforeScreen = View->ProjectPoint(lcMul31(GrabPosition, Before));
	View->SetMousePosition(qRound(BeforeScreen.x) + 10, qRound(BeforeScreen.y) + 5);
	View->OnMouseMove();
	QVERIFY(lcLength(Restored->mModelWorld.GetTranslation() - Before.GetTranslation()) > 0.01f);
	View->OnLeftButtonUp();
	QVERIFY(!View->IsTracking());
	Model->UndoAction();
	QVERIFY(lcLength(Model->GetPieces()[0]->mModelWorld.GetTranslation() - Before.GetTranslation()) < 0.001f);
}

void lcModelInstanceTests::TrainTrackDragSleepers_data()
{
	QTest::addColumn<QString>("Group");
	QTest::addColumn<QString>("Outcome");
	for (const QString& Group : { QString("4.5V"), QString("12V") })
		for (const QString& Outcome : { QString("accept"), QString("cancel"), QString("release-snap"), QString("insert") })
			QTest::newRow(qPrintable(Group + "-" + Outcome)) << Group << Outcome;
}

void lcModelInstanceTests::TrainTrackDragSleepers()
{
	QFETCH(QString, Group);
	QFETCH(QString, Outcome);
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
	lcModel* Model = Scope.Current.GetMainModel();
	QBuffer Buffer;
	Buffer.setData("1 16 0 0 0 1 0 0 0 1 0 0 0 1 sleeper-track.dat\n"
		"1 3 -1000 0 0 1 0 0 0 1 0 0 0 1 sleeper-track.dat\n");
	QVERIFY(Buffer.open(QIODevice::ReadOnly));
	Model->LoadLDraw(Buffer, &Scope.Current);
	QCOMPARE(Model->GetPieces().size(), size_t(2));
	lcPiece* Target = Model->GetPieces()[0].get();
	lcPiece* Moving = Model->GetPieces()[1].get();
	const size_t ConnectionGroup = qHash(Group);
	Target->mPieceData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44Translation(lcVector3(160, 0, 0)), { ConnectionGroup, lcTrainTrackConnectionSleeper::NeedsSleeper } },
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI), lcVector3(-160, 0, 0)), { ConnectionGroup, lcTrainTrackConnectionSleeper::NeedsSleeper } }
	}, 7, true));
	lcPieceDataRef SleeperData = lcGetPiecesLibrary()->AcquirePieceData("drag-sleeper.dat", &Scope.Current, lcAcquirePieceDataFlag::CreateMissing);
	QVERIFY(SleeperData);
	lcMesh* Mesh = new lcMesh;
	Mesh->CreateBox();
	SleeperData->SetMesh(Mesh);
	SleeperData->mState = lcAssetState::Loaded;
	SleeperData->SetTrainTrackInfo(new lcTrainTrackInfo({
		{ lcMatrix44(lcMatrix33RotationZ(LC_PI / 2), lcVector3(0, 0, 16)), { ConnectionGroup, lcTrainTrackConnectionSleeper::HasSleeper } },
		{ lcMatrix44(lcMatrix33RotationZ(-LC_PI / 2), lcVector3(0, 0, 16)), { ConnectionGroup, lcTrainTrackConnectionSleeper::HasSleeper } }
	}, 0, true));

	lcModelInstanceTestSleepers Sleepers;
	lcModelInstanceTestTrackInfo::mSleepers[ConnectionGroup] = SleeperData.Get();

	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	if (Outcome == "insert")
	{
		Model->SetSelectionAndFocusAction({}, Target, LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST, lcSelectionMode::Single);
		Model->UpdateTrainTrackConnections(Target, false);
		QVERIFY(Model->AddPiece(Target->mPieceData.Get(), LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST));
		QCOMPARE(Model->GetPieces().size(), size_t(4));
		QCOMPARE(Model->GetPieces()[2]->mPieceData, SleeperData);
		QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Model->GetPieces()[3].get(), 0, Model->GetPieces()[2].get()) != -1);
		Model->UndoAction();
		QCOMPARE(Model->GetPieces().size(), size_t(2));
		Model->RedoAction();
		QCOMPARE(Model->GetPieces().size(), size_t(4));
		return;
	}

	Model->SetSelectionAndFocusAction({}, Moving, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);
	const lcMatrix44 Start = Moving->mModelWorld;
	Model->BeginMouseTool(lcTool::Move, Window.GetActiveView());
	const lcTrainTrackSnapContext SnapContext
	{
		lcMul(lcMatrix44Translation(lcVector3(0, 0, -2000)), lcMatrix44Ortho(-1000, 1000, -1000, 1000, 1, 4000)),
		lcVector2(2000, 2000), lcVector3(0, 0, -1)
	};
	std::vector<lcInsertPieceInfo> Snap = Model->GetTrainTrackSnapInfo(Moving, lcMatrix44Translation(lcVector3(310, 0, 0)), SnapContext, 20);
	QCOMPARE(Snap.size(), size_t(2));
	QCOMPARE(Snap[0].PieceData, SleeperData.Get());
	QCOMPARE(Snap[0].ColorIndex, lcGetColorIndex(0));
	QCOMPARE(Snap[1].ColorIndex, Moving->GetColorIndex());
	const lcMatrix44 Snapped = Snap.back().Transform;
	Snap.pop_back();
	Model->SetPreviewInsertPieceInfo(std::move(Snap));
	Model->UpdateFreeMoveTool(Moving, Start, Snapped, true, false);
	QCOMPARE(Model->GetPreviewInsertPieceInfo().size(), size_t(1));
	QCOMPARE(Model->GetPieces().size(), size_t(2));

	if (Outcome == "release-snap")
	{
		QVERIFY(Model->GetTrainTrackSnapInfo(Moving, lcMatrix44Translation(lcVector3(500, 0, 0)), SnapContext, 20).empty());
		Model->SetPreviewInsertPieceInfo({});
		Model->UpdateFreeMoveTool(Moving, Start, lcMatrix44Translation(lcVector3(500, 0, 0)), false, false);
	}
	Model->EndMouseTool(lcTool::Move, Window.GetActiveView(), Outcome != "cancel");
	QVERIFY(Model->GetPreviewInsertPieceInfo().empty());
	if (Outcome == "accept")
	{
		QCOMPARE(Model->GetPieces().size(), size_t(3));
		lcPiece* Sleeper = Model->GetPieces()[2].get();
		QCOMPARE(Sleeper->mPieceData, SleeperData);
		QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Target, 0, Sleeper) != -1);
		QVERIFY(lcTrainTrackInfo::GetPieceConnectionIndex(Moving, 1, Sleeper) != -1);
		Model->UndoAction();
		QCOMPARE(Model->GetPieces().size(), size_t(2));
		QVERIFY(lcLength(Model->GetPieces()[1]->mModelWorld.GetTranslation() - Start.GetTranslation()) < 0.001f);
		Model->RedoAction();
		QCOMPARE(Model->GetPieces().size(), size_t(3));
	}
	else
	{
		QCOMPARE(Model->GetPieces().size(), size_t(2));
		if (Outcome == "cancel")
			QVERIFY(lcLength(Model->GetPieces()[1]->mModelWorld.GetTranslation() - Start.GetTranslation()) < 0.001f);
	}
}

void lcModelInstanceTests::AxisAlignedRotations_data()
{
	QTest::addColumn<int>("Axis");
	QTest::addColumn<int>("Increment");
	QTest::addColumn<bool>("AxisAngle");
	QTest::addColumn<bool>("Relative");

	for (int Axis = 0; Axis < 3; Axis++)
		for (int Increment : { -90, -30, -15, 15, 30, 90 })
			for (bool AxisAngle : { false, true })
				for (bool Relative : { false, true })
					QTest::newRow(qPrintable(QString("%1-%2-%3-%4").arg(Axis).arg(Increment).arg(AxisAngle).arg(Relative))) << Axis << Increment << AxisAngle << Relative;
}

void lcModelInstanceTests::AxisAlignedRotations()
{
	QFETCH(int, Axis);
	QFETCH(int, Increment);
	QFETCH(bool, AxisAngle);
	QFETCH(bool, Relative);

	lcVector3 Unit(0, 0, 0);
	Unit[Axis] = 1.0f;
	lcMatrix33 Rotation;

	if (AxisAngle)
		Rotation = lcMatrix33FromAxisAngle(Unit, Increment * LC_DTOR);
	else if (Axis == 0)
		Rotation = lcMatrix33RotationX(Increment * LC_DTOR);
	else if (Axis == 1)
		Rotation = lcMatrix33RotationY(Increment * LC_DTOR);
	else
		Rotation = lcMatrix33RotationZ(Increment * LC_DTOR);

	const lcVector3 Center(100, 200, 300);
	const lcVector3 Offset(20, 40, 60);
	lcPiece CenterPiece;
	lcPiece OrbitPiece;
	CenterPiece.Initialize(lcMatrix44(lcMatrix33Identity(), Center), 1);
	OrbitPiece.Initialize(lcMatrix44(lcMatrix33Identity(), Center + Offset), 1);
	CenterPiece.SetFocused(LC_PIECE_SECTION_POSITION, true);
	OrbitPiece.SetFocused(LC_PIECE_SECTION_POSITION, true);

	lcMatrix33 Expected = lcMatrix33Identity();
	const int First = (Axis + 1) % 3;
	const int Second = (Axis + 2) % 3;
	Expected[First][First] = 0;
	Expected[Second][Second] = 0;
	Expected[First][Second] = Increment > 0 ? 1.0f : -1.0f;
	Expected[Second][First] = -Expected[First][Second];
	const int StepsPerQuarter = 90 / std::abs(Increment);

	for (int Step = 1; Step <= StepsPerQuarter * 4; Step++)
	{
		for (lcPiece* Piece : { &CenterPiece, &OrbitPiece })
		{
			const lcMatrix33 Orientation(Piece->mModelWorld);
			const lcMatrix33 Frame = Relative ? lcMatrix33AffineInverse(Orientation) : lcMatrix33Identity();
			const lcMatrix33 Delta = Relative ? lcMul(Rotation, Orientation) : Rotation;
			Piece->Rotate(1, false, Delta, Center, Frame);
			Piece->UpdatePosition(1);
		}

		if (Step % StepsPerQuarter != 0)
			continue;

		const int Quarter = Step / StepsPerQuarter;
		lcMatrix33 QuarterRotation = lcMatrix33Identity();

		for (int Turn = 0; Turn < Quarter; Turn++)
			QuarterRotation = lcMul(QuarterRotation, Expected);

		for (lcPiece* Piece : { &CenterPiece, &OrbitPiece })
		{
			for (int Row = 0; Row < 3; Row++)
				for (int Column = 0; Column < 3; Column++)
					QCOMPARE(Piece->mModelWorld[Row][Column], QuarterRotation[Row][Column]);
		}

		const lcVector3 ExpectedPosition = Center + lcMul(Offset, QuarterRotation);

		// Repeated rotations accumulate float rounding in the position, especially
		// on platforms without fused multiply-add. Orientations above remain exact.
		for (int Component = 0; Component < 3; Component++)
		{
			QCOMPARE(CenterPiece.mModelWorld.GetTranslation()[Component], Center[Component]);
			const float Actual = OrbitPiece.mModelWorld.GetTranslation()[Component];
			QVERIFY2(fabsf(Actual - ExpectedPosition[Component]) < 0.001f,
				qPrintable(QString("Step %1, component %2: position %3, expected %4")
					.arg(Step).arg(Component).arg(Actual, 0, 'g', 9).arg(ExpectedPosition[Component], 0, 'g', 9)));
		}

		// Relative grid movement must introduce no fractional coordinates.
		const lcVector3 Distance = lcMul(lcVector3(20, 0, 0), CenterPiece.GetRelativeRotation());
		CenterPiece.MoveSelected(1, false, Distance);
		CenterPiece.UpdatePosition(1);

		for (int Component = 0; Component < 3; Component++)
			QCOMPARE(CenterPiece.mModelWorld.GetTranslation()[Component], Center[Component] + 20 * QuarterRotation[0][Component]);

		CenterPiece.MoveSelected(1, false, -Distance);
		CenterPiece.UpdatePosition(1);
	}

	CenterPiece.ResetPivotPoint();
	for (int Step = 0; Step < StepsPerQuarter; Step++)
		CenterPiece.RotatePivotPoint(Rotation);

	for (int Row = 0; Row < 3; Row++)
		for (int Column = 0; Column < 3; Column++)
			QCOMPARE(CenterPiece.mPivotMatrix[Row][Column], Expected[Row][Column]);
}

void lcModelInstanceTests::AxisAlignedRotationTolerance()
{
	// Check all 48 signed permutations, including mirrored parts.
	for (int First = 0; First < 3; First++)
		for (int Second = 0; Second < 3; Second++)
		{
			if (First == Second)
				continue;

			for (int Signs = 0; Signs < 8; Signs++)
			{
				lcMatrix33 Expected = lcMatrix33Scale(lcVector3(0, 0, 0));
				Expected[0][First] = (Signs & 1) ? -1.0f : 1.0f;
				Expected[1][Second] = (Signs & 2) ? -1.0f : 1.0f;
				Expected[2][3 - First - Second] = (Signs & 4) ? -1.0f : 1.0f;
				lcMatrix33 Perturbed = Expected;

				for (int Row = 0; Row < 3; Row++)
					for (int Column = 0; Column < 3; Column++)
						Perturbed[Row][Column] += 0.0000005f;

				Perturbed.SnapToAxisAligned();

				for (int Row = 0; Row < 3; Row++)
					for (int Column = 0; Column < 3; Column++)
						QCOMPARE(Perturbed[Row][Column], Expected[Row][Column]);
			}
		}

	for (float Degrees : { 0.001f, 15.0f, 30.0f, 45.0f, 89.999f, 90.001f })
	{
		const lcMatrix33 Original = lcMatrix33RotationZ(Degrees * LC_DTOR);
		lcMatrix33 Rotation = Original;
		Rotation.SnapToAxisAligned();

		for (int Row = 0; Row < 3; Row++)
			for (int Column = 0; Column < 3; Column++)
				QCOMPARE(Rotation[Row][Column], Original[Row][Column]);
	}

	// Reject matrices with duplicate axes even if every entry is an integer.
	lcMatrix33 Invalid(lcVector3(1, 0, 0), lcVector3(1, 0, 0), lcVector3(0, 0, 1));
	const lcMatrix33 Original = Invalid;
	Invalid.SnapToAxisAligned();

	for (int Row = 0; Row < 3; Row++)
		for (int Column = 0; Column < 3; Column++)
			QCOMPARE(Invalid[Row][Column], Original[Row][Column]);
}

void lcModelInstanceTests::KeyboardRotationAxes_data()
{
	QTest::addColumn<int>("Axis");
	QTest::addColumn<int>("Sign");
	QTest::addColumn<bool>("Fixed");
	QTest::addColumn<bool>("Relative");
	QTest::addColumn<bool>("Separate");

	for (int Axis = 0; Axis < 3; Axis++)
		for (int Sign : { -1, 1 })
			for (bool Fixed : { false, true })
				for (bool Relative : { false, true })
					for (bool Separate : { false, true })
						QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5").arg(Axis).arg(Sign).arg(Fixed).arg(Relative).arg(Separate))) << Axis << Sign << Fixed << Relative << Separate;
}

void lcModelInstanceTests::KeyboardRotationAxes()
{
	QFETCH(int, Axis);
	QFETCH(int, Sign);
	QFETCH(bool, Fixed);
	QFETCH(bool, Relative);
	QFETCH(bool, Separate);

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile File(Directory.filePath("rotate.ldr"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Keyboard rotation\n1 16 0 0 0 1 0 0 0 1 0 0 0 1 alpha.dat\n");
	File.close();

	struct ScopedProject
	{
		Project* Previous = gApplication->mProject;
		lcMainWindow* PreviousWindow = gMainWindow;
		bool PreviousFixed = lcGetPreferences().mFixedAxes;
		Project Current;
		ScopedProject()
		{
			gApplication->mProject = &Current;
		}
		~ScopedProject()
		{
			gApplication->mProject = Previous;
			gMainWindow = PreviousWindow;
			lcGetPreferences().mFixedAxes = PreviousFixed;
		}
	} Scope;
	QVERIFY(Scope.Current.Load(File.fileName()));
	lcModel* Model = Scope.Current.GetMainModel();
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Model, false);
	Window.SetRelativeTransform(Relative);
	Window.SetSeparateTransform(Separate);
	Window.SetAngleSnapEnabled(true);
	lcGetPreferences().mFixedAxes = Fixed;
	lcView* View = Window.GetActiveView();
	View->SetSize(1200, 600);
	lcCamera Camera(true);
	Camera.SetViewpoint(lcVector3(400, -500, 300), lcVector3(0, 0, 0), lcVector3(0, 0, 1));
	View->SetCamera(&Camera, true);
	lcPiece* Piece = Model->GetPieces()[0].get();
	const lcMatrix33 Initial = lcMul(lcMatrix33RotationX(LC_PI / 2), lcMatrix33RotationZ(LC_PI / 6));
	Piece->Initialize(lcMatrix44(Initial, lcVector3(0, 0, 0)), 1);
	Model->SetSelectionAndFocusAction({ Piece }, Piece, LC_PIECE_SECTION_POSITION, lcSelectionMode::Single);

	lcVector3 Input(0, 0, 0);
	Input[Axis] = Sign * Window.GetAngleSnap();
	// Capture the camera mapping before rotating; it must remain stable for
	// every command, even when the piece's local axes turn past the camera.
	const lcVector3 Angles = View->GetMoveDirection(Input, false);
	const lcMatrix33 Delta = lcMul(lcMul(lcMatrix33RotationX(Angles.x * LC_DTOR), lcMatrix33RotationY(Angles.y * LC_DTOR)), lcMatrix33RotationZ(Angles.z * LC_DTOR));
	const lcCommandId Commands[3][2] =
	{
		{ LC_PIECE_ROTATE_MINUSX, LC_PIECE_ROTATE_PLUSX },
		{ LC_PIECE_ROTATE_MINUSY, LC_PIECE_ROTATE_PLUSY },
		{ LC_PIECE_ROTATE_MINUSZ, LC_PIECE_ROTATE_PLUSZ }
	};
	lcMatrix33 Expected = Initial;

	for (int Turn = 0; Turn < 24; Turn++)
	{
		Window.HandleCommand(Commands[Axis][Sign > 0 ? 1 : 0]);
		Expected = Fixed && (Relative || Separate) ? lcMul(Delta, Expected) : lcMul(Expected, Delta);

		for (int Row = 0; Row < 3; Row++)
			for (int Column = 0; Column < 3; Column++)
				QVERIFY(fabsf(Piece->mModelWorld[Row][Column] - Expected[Row][Column]) < 0.001f);
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
	QVERIFY(Scope.Current.Load(File.fileName()));
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
	QVERIFY(Project->Load(File.fileName()));

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


void lcModelInstanceTests::PreferencesPreservePartFilter_data()
{
	QTest::addColumn<bool>("ListMode");
	QTest::addColumn<bool>("ManyMatches");
	QTest::newRow("icons-single") << false << false;
	QTest::newRow("list-single") << true << false;
	QTest::newRow("icons-many") << false << true;
	QTest::newRow("list-many") << true << true;
}

void lcModelInstanceTests::PreferencesPreservePartFilter()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test preferences and filtering.");

	QFETCH(bool, ListMode);
	QFETCH(bool, ManyMatches);
	QVERIFY(lcContext::InitializeRenderer(true));
	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QVERIFY(QDir(Directory.path()).mkpath("parts"));
	QVERIFY(QDir(Directory.path()).mkpath("p"));

	for (int Index = 0; Index < 200; Index++)
	{
		QFile File(Directory.filePath(QString("parts/filter%1.dat").arg(Index)));
		QVERIFY(File.open(QIODevice::WriteOnly));
		QVERIFY(File.write(ManyMatches && Index % 2 == 0 ? "0 Filter 100 fixture\n3 16 0 0 0 1 0 0 0 1 0\n" : "0 Filter fixture\n3 16 0 0 0 1 0 0 0 1 0\n") > 0);
	}

	QVERIFY(lcGetPiecesLibrary()->Load(Directory.path(), false, false));

	struct ScopedSettings
	{
		lcPreferences Preferences = lcGetPreferences();
		lcMainWindow* Window = gMainWindow;
		Project* ActiveProject = gApplication->mProject;
		Project Current;
		ScopedSettings() { gApplication->mProject = &Current; }
		~ScopedSettings()
		{
			lcGetPreferences() = Preferences;
			gMainWindow = Window;
			gApplication->mProject = ActiveProject;
		}
	} Settings;

	lcGetPreferences().mViewSphereEnabled = false;
	lcGetPreferences().mColorTheme = lcColorTheme::Dark;
	lcGetPreferences().mDrawAxes = false;
	lcGetPreferences().mDrawGridStuds = false;
	lcGetPreferences().mDrawGridLines = false;
	lcMainWindow Window;
	Window.CreateWidgets();
	Window.SetCurrentModelTab(Settings.Current.GetMainModel(), false);
	lcPartSelectionWidget* Picker = Window.GetPartSelectionWidget();
	Picker->SetCategory(lcPartCategoryType::AllParts, 0);
	lcPartSelectionListView* View = Picker->findChild<lcPartSelectionListView*>();
	QVERIFY(View);
	lcPartSelectionListModel* List = View->GetListModel();
	QCOMPARE(List->rowCount(), 200);
	if (List->IsListMode() != ListMode)
		List->ToggleListMode();
	QLineEdit* Filter = nullptr;
	for (QLineEdit* Edit : Picker->findChildren<QLineEdit*>())
		if (Edit->accessibleName() == QStringLiteral("Filter Pieces"))
			Filter = Edit;
	QVERIFY(Filter);
	Filter->setText(QStringLiteral("100"));
	const auto VisibleRows = [View, List]()
	{
		int Count = 0;
		for (int Row = 0; Row < List->rowCount(); Row++)
			Count += !View->isRowHidden(Row);
		return Count;
	};
	const int ExpectedVisible = ManyMatches ? 100 : 1;
	QCOMPARE(VisibleRows(), ExpectedVisible);
	Window.show();
	QTest::qWait(100);
	const auto RequestedRows = [List, View]()
	{
		int Count = 0;
		for (int Row = 0; Row < List->rowCount(); Row++)
			Count += View->isRowHidden(Row) && List->GetPieceData(Row)->mState != lcAssetState::Unloaded;
		return Count;
	};
	const int InitiallyRequested = RequestedRows();
	QVERIFY(InitiallyRequested <= 1);
	// A filtered-out row must not start a load even if a stale paint requests it.
	for (int Row = 0; Row < List->rowCount(); Row++)
	{
		if (View->isRowHidden(Row) && List->GetPieceData(Row)->mState == lcAssetState::Unloaded)
		{
			List->RequestThumbnail(Row);
			QCOMPARE(List->GetPieceData(Row)->mState, lcAssetState::Unloaded);
			break;
		}
	}

	for (int Pass = 0; Pass < 2; Pass++)
	{
		QTimer::singleShot(0, []()
		{
			QDialog* Dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
			QVERIFY(Dialog);
			QMetaObject::invokeMethod(Dialog, "accept");
		});
		gApplication->ShowPreferencesDialog();
		QCOMPARE(Filter->text(), QStringLiteral("100"));
		QCOMPARE(VisibleRows(), ExpectedVisible);
		QCoreApplication::processEvents();
		QCOMPARE(VisibleRows(), ExpectedVisible);
		QTest::qWait(100);
		QCOMPARE(RequestedRows(), InitiallyRequested);
	}
}

void lcModelInstanceTests::ThumbnailShading()
{
	if (!qEnvironmentVariableIsSet("LEOCAD_TEST_NATIVE_INTERFACE"))
		QSKIP("Set LEOCAD_TEST_NATIVE_INTERFACE=1 with a native display to test thumbnail rendering.");

	QVERIFY(lcContext::InitializeRenderer(true));
	lcLoadDefaultColors(lcStudStyle::Plain);

	struct ScopedSettings
	{
		lcPreferences Preferences = lcGetPreferences();
		lcMainWindow* Window = gMainWindow;
		Project* ActiveProject = gApplication->mProject;
		~ScopedSettings()
		{
			lcGetPreferences() = Preferences;
			gMainWindow = Window;
			gApplication->mProject = ActiveProject;
		}
	} Settings;
	gMainWindow = nullptr;
	lcGetPreferences().mShadingMode = lcShadingMode::Flat;
	lcGetPreferences().mThumbnailShadingMode = lcThumbnailShadingMode::UseViewShading;

	QTemporaryDir Directory;
	QVERIFY(Directory.isValid());
	QFile File(Directory.filePath("shading.ldr"));
	QVERIFY(File.open(QIODevice::WriteOnly));
	File.write("0 Shading\n4 4 -20 0 -20 20 0 -20 20 0 20 -20 0 20\n"
		"4 4 -20 0 -20 20 0 -20 20 -40 -20 -20 -40 -20\n"
		"4 4 20 0 -20 20 0 20 20 -40 20 20 -40 -20\n"
		"4 4 -20 -40 -20 -20 -40 20 20 -40 20 20 -40 -20\n"
		"2 24 -20 0 -20 20 0 -20\n");
	File.close();
	Project Project;
	QVERIFY(Project.Load(File.fileName()));
	lcModel* Model = Project.GetMainModel();
	Model->WaitForAssets();

	lcThumbnailManager Thumbnails(lcGetPiecesLibrary());
	const auto [Id, Flat] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QVERIFY(!Flat.isNull());
	const auto [SharedId, SharedFlat] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QCOMPARE(Id, SharedId);
	QCOMPARE(Flat.toImage(), SharedFlat.toImage());

	int ReadyCount = 0;
	int InvalidatedCount = 0;
	lcPartThumbnailId ReadyId = lcPartThumbnailId::Invalid;
	QPixmap ReadyPixmap;
	connect(&Thumbnails, &lcThumbnailManager::ThumbnailReady, this, [&](lcPartThumbnailId ThumbnailId, QPixmap Pixmap)
	{
		ReadyCount++;
		ReadyId = ThumbnailId;
		ReadyPixmap = Pixmap;
	});
	connect(&Thumbnails, &lcThumbnailManager::ThumbnailsInvalidated, this, [&InvalidatedCount]()
	{
		InvalidatedCount++;
	});

	lcGetPreferences().mThumbnailShadingMode = lcThumbnailShadingMode::DefaultLights;
	Thumbnails.RefreshThumbnails();
	QCOMPARE(InvalidatedCount, 1);
	QCOMPARE(ReadyCount, 0);
	const auto [LitId, LitPixmap] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QCOMPARE(ReadyCount, 1);
	QCOMPARE(ReadyId, LitId);
	QVERIFY(LitId != Id);
	const QImage Lit = LitPixmap.toImage();
	QCOMPARE(ReadyPixmap.toImage(), Lit);
	if (lcContext::SupportsDefaultLights())
		QVERIFY(Lit != Flat.toImage());
	QCOMPARE(lcGetPreferences().mShadingMode, lcShadingMode::Flat);

	// An explicit selector setting stays independent of subsequent main view changes.
	lcGetPreferences().mThumbnailShadingMode = lcThumbnailShadingMode::Flat;
	lcGetPreferences().mShadingMode = lcShadingMode::Wireframe;
	Thumbnails.RefreshThumbnails();
	QCOMPARE(InvalidatedCount, 2);
	QCOMPARE(ReadyCount, 1);
	const auto [FlatId, ExplicitFlat] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QCOMPARE(ExplicitFlat.toImage(), Flat.toImage());
	QCOMPARE(ReadyCount, 2);

	// Inherited wireframe falls back to flat shading for thumbnails.
	lcGetPreferences().mThumbnailShadingMode = lcThumbnailShadingMode::UseViewShading;
	Thumbnails.RefreshThumbnails();
	QCOMPARE(InvalidatedCount, 3);
	QCOMPARE(ReadyCount, 2);
	const auto [InheritedId, Inherited] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QCOMPARE(Inherited.toImage(), Flat.toImage());
	QCOMPARE(ReadyCount, 3);
	const auto [CachedId, Cached] = Thumbnails.RequestThumbnail(Model->GetPieceData(), gDefaultColor, 64, 1.0f);
	QCOMPARE(CachedId, InheritedId);
	QCOMPARE(Cached.toImage(), Flat.toImage());
	QCOMPARE(ReadyCount, 3);
	Thumbnails.ReleaseThumbnail(InheritedId);
	Thumbnails.ReleaseThumbnail(CachedId);

	// Both selectors drop stale pixmaps without losing selection or drawing offscreen items.
	gApplication->mProject = &Project;
	lcMainWindow Window;
	lcPartSelectionWidget Docked(false, nullptr);
	lcPartSelectionWidget Popup(true, nullptr);
	const std::vector<std::pair<lcPieceData*, std::string>> Parts = { { Model->GetPieceData(), "Shading" } };
	Docked.SetCustomParts(Parts, gDefaultColor);
	Popup.SetCustomParts(Parts, gDefaultColor);
	lcPartSelectionListView* DockedView = Docked.findChild<lcPartSelectionListView*>();
	lcPartSelectionListView* PopupView = Popup.findChild<lcPartSelectionListView*>();
	QVERIFY(DockedView);
	QVERIFY(PopupView);
	lcPartSelectionListModel* DockedList = DockedView->GetListModel();
	lcPartSelectionListModel* PopupList = PopupView->GetListModel();
	for (lcPartSelectionListView* View : { DockedView, PopupView })
	{
		lcPartSelectionListModel* List = View->GetListModel();
		List->SetIconSize(64, 1.0f);
		View->setCurrentIndex(List->index(0, 0));
		List->RequestThumbnail(0);
		QVERIFY(!qvariant_cast<QPixmap>(List->data(List->index(0, 0), Qt::DecorationRole)).isNull());
	}

	lcThumbnailManager* SharedThumbnails = lcGetPiecesLibrary()->GetThumbnailManager();
	int SharedReadyCount = 0;
	QObject ReadyContext;
	connect(SharedThumbnails, &lcThumbnailManager::ThumbnailReady, &ReadyContext, [&SharedReadyCount](lcPartThumbnailId, QPixmap)
	{
		SharedReadyCount++;
	});
	lcGetPreferences().mThumbnailShadingMode = lcThumbnailShadingMode::Flat;
	SharedThumbnails->RefreshThumbnails();
	QCOMPARE(SharedReadyCount, 0);
	for (lcPartSelectionListView* View : { DockedView, PopupView })
	{
		lcPartSelectionListModel* List = View->GetListModel();
		QCOMPARE(View->currentIndex(), List->index(0, 0));
		QVERIFY(qvariant_cast<QPixmap>(List->data(List->index(0, 0), Qt::DecorationRole)).isNull());
	}
	DockedList->RequestThumbnail(0);
	QCOMPARE(SharedReadyCount, 1);
	PopupList->RequestThumbnail(0);
	QCOMPARE(SharedReadyCount, 1);
	QCOMPARE(qvariant_cast<QPixmap>(DockedList->data(DockedList->index(0, 0), Qt::DecorationRole)).toImage(), Flat.toImage());
	QCOMPARE(qvariant_cast<QPixmap>(PopupList->data(PopupList->index(0, 0), Qt::DecorationRole)).toImage(), Flat.toImage());

	// Color changes use the same lazy invalidation and retain the current selection.
	DockedList->ToggleColorLocked();
	DockedList->SetColorIndex(lcGetColorIndex(1));
	QCOMPARE(DockedList->GetColorIndex(), lcGetColorIndex(1));
	QCOMPARE(DockedView->currentIndex(), DockedList->index(0, 0));
	QCOMPARE(SharedReadyCount, 1);
	QVERIFY(qvariant_cast<QPixmap>(DockedList->data(DockedList->index(0, 0), Qt::DecorationRole)).isNull());
	QCOMPARE(qvariant_cast<QPixmap>(PopupList->data(PopupList->index(0, 0), Qt::DecorationRole)).toImage(), Flat.toImage());
	DockedList->RequestThumbnail(0);
	QCOMPARE(SharedReadyCount, 2);
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
	QVERIFY(Project.Load(File.fileName()));
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
	QCOMPARE(Options.mParseOK, Valid);
	if (Valid)
	{
		QCOMPARE(int(Options.mImageBackground.Mode), Mode);
		QCOMPARE(Options.mImageBackground.Color, Top);
		QCOMPARE(Options.mImageBackground.GradientBottomColor, Bottom);
	}
	else
		QVERIFY(!Options.mStdErr.isEmpty());
}

void lcModelInstanceTests::CommandLineOptions()
{
	const lcCommandLineOptions Empty(QStringList{});
	QVERIFY(Empty.mParseOK);
	QVERIFY(!Empty.mExit);
	QVERIFY(!Empty.mSaveImage);
	QVERIFY(!Empty.HasExportRequest());
	for (const QString& Option : { QStringLiteral("--image"), QStringLiteral("--export-wavefront"), QStringLiteral("--export-3ds"),
		QStringLiteral("--export-collada"), QStringLiteral("--export-csv"), QStringLiteral("--export-html") })
		QVERIFY(lcCommandLineOptions({ QStringLiteral("leocad"), Option }).HasExportRequest());
	QCOMPARE(Empty.GetExportFileName(QStringLiteral("/tmp/model"), QStringLiteral("obj")), QStringLiteral("/tmp/model.obj"));
	QCOMPARE(Empty.GetExportFileName(QStringLiteral("/tmp/model.ldr"), QStringLiteral("obj")), QStringLiteral("/tmp/model.obj"));
	QCOMPARE(Empty.GetExportFileName(QStringLiteral("/tmp/model.OBJ"), QStringLiteral("obj")), QStringLiteral("/tmp/model.OBJ"));
	QCOMPARE(lcCommandLineOptions::GetImageFrameName(QStringLiteral("/tmp/model"), false), QStringLiteral("/tmp/model"));
	QCOMPARE(lcCommandLineOptions::GetImageFrameName(QStringLiteral("/tmp/model"), true).arg(1, 2, 10, QLatin1Char('0')), QStringLiteral("/tmp/model01"));
	QCOMPARE(lcCommandLineOptions::GetImageFrameName(QStringLiteral("/tmp/model."), true).arg(1, 2, 10, QLatin1Char('0')), QStringLiteral("/tmp/model01."));
	QCOMPARE(lcCommandLineOptions::GetImageFrameName(QStringLiteral("/tmp/model.png"), true).arg(2, 2, 10, QLatin1Char('0')), QStringLiteral("/tmp/model02.png"));
	QVERIFY(!Empty.mSetFoV && !Empty.mSetZPlanes && !Empty.mSetFadeStepsColor && !Empty.mSetHighlightColor);

	const lcCommandLineOptions Camera({ "leocad", "--fov", "45", "--zplanes", "2", "300", "--fade-steps-color", "transparent", "--highlight-color", "transparent" });
	QVERIFY(Camera.mParseOK);
	QVERIFY(Camera.mSetFoV && Camera.mSetZPlanes && Camera.mSetFadeStepsColor && Camera.mSetHighlightColor);
	QCOMPARE(Camera.mFoV, 45.0f);
	QCOMPARE(Camera.mZPlanes[0], 2.0f);
	QCOMPARE(Camera.mZPlanes[1], 300.0f);
	QCOMPARE(Camera.mFadeStepsColor, quint32(0));
	QCOMPARE(Camera.mHighlightColor, quint32(0));

	const lcCommandLineOptions Invalid({ "leocad", "--fov", "45", "--fov", "bad", "--zplanes", "2", "300", "--zplanes", "bad", "4" });
	QVERIFY(!Invalid.mParseOK);
	QVERIFY(!Invalid.mSetFoV && !Invalid.mSetZPlanes);

	const lcCommandLineOptions Help({ "leocad", "--help" });
	QVERIFY(Help.mParseOK);
	QVERIFY(Help.mExit);
	QVERIFY(Help.mStdOut.contains("--background-gradient"));

	QTemporaryFile Model;
	QVERIFY(Model.open());
	QTemporaryDir MissingDirectory;
	QVERIFY(MissingDirectory.isValid());
	const QString MissingPath = MissingDirectory.filePath("missing.ldr");
	for (const QStringList& Arguments : {
		QStringList{ "leocad", MissingPath },
		QStringList{ "leocad", "--export-csv", "out.csv", MissingPath },
		QStringList{ "leocad", MissingPath, "--export-csv", "out.csv" },
		QStringList{ "leocad", Model.fileName(), MissingPath } })
	{
		const lcCommandLineOptions MissingInput(Arguments);
		QVERIFY(MissingInput.mParseOK);
		QCOMPARE(MissingInput.mProjectName, MissingPath);
		QVERIFY(MissingInput.mStdErr.isEmpty());
	}
	const lcCommandLineOptions Image({ "leocad", Model.fileName(), "-i", "out.png",
		"--width", "123", "--height", "456", "--aa-samples", "4", "--stud-style", "3", "--viewpoint", "front" });
	QVERIFY(Image.mParseOK);
	QVERIFY(Image.mSaveImage);
	QVERIFY(Image.HasExportRequest());
	const QString ModelSuffix = QFileInfo(Model.fileName()).suffix();
	const QString ModelBase = ModelSuffix.isEmpty() ? Model.fileName() : Model.fileName().left(Model.fileName().length() - ModelSuffix.length() - 1);
	QCOMPARE(Image.GetExportFileName(QString(), QStringLiteral("csv")), ModelBase + QStringLiteral(".csv"));
	QCOMPARE(Image.mImageWidth, 123);
	QCOMPARE(Image.mImageHeight, 456);
	QCOMPARE(Image.mAASamples, 4);
	QCOMPARE(int(Image.mStudStyle), 3);
	QCOMPARE(Image.mViewpoint, lcViewpoint::Front);
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
	QCOMPARE(Options.mParseOK, Valid);
	if (!Message.isEmpty())
		QVERIFY(Options.mStdErr.contains(Message));
	if (Options.mSaveWavefront)
	{
		QVERIFY(Options.mSaveWavefrontName.isEmpty());
		QCOMPARE(Options.mImageWidth, 160);
	}
	if (Options.mSetFadeStepsColor)
		QCOMPARE(Options.mFadeStepsColor, LC_RGBA(0x11, 0x22, 0x33, 0x80));
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
		{ "--fade-steps-color", &lcCommandLineOptions::mFadeStepsColor },
		{ "--highlight-color", &lcCommandLineOptions::mHighlightColor },
		{ "--stud-cylinder-color", &lcCommandLineOptions::mStudCylinderColor },
		{ "-scc", &lcCommandLineOptions::mStudCylinderColor },
		{ "--edge-color", &lcCommandLineOptions::mPartEdgeColor },
		{ "-ec", &lcCommandLineOptions::mPartEdgeColor },
		{ "--black-edge-color", &lcCommandLineOptions::mBlackEdgeColor },
		{ "-bec", &lcCommandLineOptions::mBlackEdgeColor },
		{ "--dark-edge-color", &lcCommandLineOptions::mDarkEdgeColor },
		{ "-dec", &lcCommandLineOptions::mDarkEdgeColor }
	};
	for (const auto& [Option, Member] : Options)
	{
		const lcCommandLineOptions Parsed({ "leocad", "--stud-style", "6", Option, Input });
		QCOMPARE(Parsed.mParseOK, Valid);
		if (Valid)
			QCOMPARE(Parsed.*Member, Expected);
		else
			QVERIFY(Parsed.mStdErr.contains("Invalid color"));
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
	Row("edge-enables-saved-disabled", false, 6, false, { "--edge-color", "#123456" }, 0, {});
	Row("edge-alias-enables-after-disable", false, 6, true, { "--disable-edge-color", "-ec", "#123456" }, 0, {});
	Row("edge-disable-after-color", false, 6, false, { "--edge-color", "#123456", "--disable-edge-color" }, 0, {});
	Row("cylinder-enables", false, 6, false, { "--stud-cylinder-color", "#123456" }, 0, {});
	Row("cylinder-disable-last", false, 6, false, { "-scc", "#123456", "-nscc" }, 0, {});
	Row("black-enables", false, 6, false, { "--black-edge-color", "#123456" }, 0, {});
	Row("black-disable-last", false, 6, false, { "-bec", "#123456", "-nbec" }, 0, {});
	Row("dark-enables", false, 6, false, { "--dark-edge-color", "#123456" }, 0, {});
	Row("dark-disable-last", false, 6, false, { "-dec", "#123456", "-ndec" }, 0, {});
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
	QVERIFY(Options.mParseOK);
	const QByteArray TestName = QTest::currentDataTag();
	if (TestName == "edge-enables-saved-disabled" || TestName == "edge-alias-enables-after-disable")
		QVERIFY(Options.mPartEdgeColorEnabled);
	else if (TestName == "edge-disable-after-color")
		QVERIFY(!Options.mPartEdgeColorEnabled);
	else if (TestName == "cylinder-enables" || TestName == "cylinder-disable-last")
		QCOMPARE(Options.mStudCylinderColorEnabled, TestName == "cylinder-enables");
	else if (TestName == "black-enables" || TestName == "black-disable-last")
		QCOMPARE(Options.mBlackEdgeColorEnabled, TestName == "black-enables");
	else if (TestName == "dark-enables" || TestName == "dark-disable-last")
		QCOMPARE(Options.mDarkEdgeColorEnabled, TestName == "dark-enables");
	QCOMPARE(Options.mStdErr.count(QLatin1Char('\n')), Warnings);
	if (!Message.isEmpty())
		QVERIFY2(Options.mStdErr.contains(Message), qPrintable(Options.mStdErr));
	if (Options.mAutomateEdgeColor && lcIsHighContrast(Options.mStudStyle))
		QVERIFY(!Options.mStdErr.contains("stud-cylinder-color"));
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
	const QString Frame = lcCommandLineOptions::GetImageFrameName(Directory.filePath("model"), true);
	QVERIFY(View.SaveStepImages(Frame, true, 1, 2, {}));
	QCOMPARE(QImageReader::imageFormat(Directory.filePath("model01")), QByteArray("png"));
	QCOMPARE(QImageReader::imageFormat(Directory.filePath("model02")), QByteArray("png"));

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
	// Native UI tests share the renderer's resources with QOpenGLWidget views.
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
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
